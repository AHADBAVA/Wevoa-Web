#include "runtime/http_module.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "interpreter/callable.h"
#include "interpreter/interpreter.h"
#include "runtime/config_loader.h"
#include "utils/error.h"
#include "utils/version.h"

namespace wevoaweb {

namespace {

struct HttpFetchResult {
    int status = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::string error;
};

std::string trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string lowerCase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool isHttpUrl(const std::string& value) {
    return value.rfind("http://", 0) == 0 || value.rfind("https://", 0) == 0;
}

bool hasDisallowedControlCharacters(const std::string& value) {
    for (const unsigned char ch : value) {
        if (ch == '\n' || ch == '\r') {
            return true;
        }
    }
    return false;
}

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void writeTextFile(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to write temporary file: " + path.string());
    }
    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string uniqueSuffix() {
    static std::atomic<std::uint64_t> counter {0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    return std::to_string(micros) + "-" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

std::filesystem::path temporaryPath(const std::string& prefix, const std::string& extension) {
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error);
    const auto root = error ? std::filesystem::current_path() : directory;
    return root / (prefix + uniqueSuffix() + extension);
}

void cleanupTemporaryFiles(const std::vector<std::filesystem::path>& paths) {
    std::error_code error;
    for (const auto& path : paths) {
        std::filesystem::remove(path, error);
        error.clear();
    }
}

Value::Object normalizeHeaderObject(const Value& value, const SourceSpan& span) {
    if (value.isNil()) {
        return {};
    }
    if (!value.isObject()) {
        throw RuntimeError("http.get() headers must be an object when provided.", span);
    }

    Value::Object normalized;
    for (const auto& [name, entry] : value.asObject()) {
        if (!entry.isString()) {
            throw RuntimeError("http.get() header values must be strings.", span);
        }
        if (name.empty()) {
            throw RuntimeError("http.get() header names must not be empty.", span);
        }
        if (hasDisallowedControlCharacters(name) || hasDisallowedControlCharacters(entry.asString())) {
            throw RuntimeError("http.get() header names and values must not contain newlines.", span);
        }

        normalized.insert_or_assign(name, entry);
    }

    return normalized;
}

std::vector<std::pair<std::string, std::string>> headerPairs(const Value::Object& headers) {
    std::vector<std::pair<std::string, std::string>> pairs;
    pairs.reserve(headers.size());
    for (const auto& [name, value] : headers) {
        pairs.emplace_back(name, value.asString());
    }
    return pairs;
}

Value::Object headerValueObject(const std::unordered_map<std::string, std::string>& headers) {
    Value::Object object;
    for (const auto& [name, value] : headers) {
        object.insert_or_assign(name, Value(value));
    }
    return object;
}

void parseHeaderLines(const std::string& source,
                      int& status,
                      std::unordered_map<std::string, std::string>& headers) {
    std::istringstream stream(source);
    std::string line;
    int currentStatus = 0;
    std::unordered_map<std::string, std::string> currentHeaders;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string cleaned = trim(line);
        if (cleaned.empty()) {
            if (currentStatus != 0) {
                status = currentStatus;
                headers = currentHeaders;
            }
            continue;
        }

        if (cleaned.rfind("HTTP/", 0) == 0) {
            currentHeaders.clear();
            currentStatus = 0;

            std::istringstream statusLine(cleaned);
            std::string version;
            statusLine >> version >> currentStatus;
            continue;
        }

        const auto separator = cleaned.find(':');
        if (separator != std::string::npos && currentStatus != 0) {
            const std::string name = lowerCase(trim(cleaned.substr(0, separator)));
            const std::string value = trim(cleaned.substr(separator + 1));
            currentHeaders.insert_or_assign(name, value);
        }
    }

    if (currentStatus != 0) {
        status = currentStatus;
        headers = currentHeaders;
    }
}

#ifdef _WIN32
std::string quoteForWindowsCommand(const std::filesystem::path& path) {
    return "\"" + path.string() + "\"";
}

HttpFetchResult fetchGetWindows(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    HttpFetchResult result;

    const auto requestPath = temporaryPath("wevoa-http-request-", ".json");
    const auto bodyPath = temporaryPath("wevoa-http-body-", ".txt");
    const auto headerPath = temporaryPath("wevoa-http-header-", ".txt");
    const auto statusPath = temporaryPath("wevoa-http-status-", ".txt");
    const auto errorPath = temporaryPath("wevoa-http-error-", ".txt");
    const auto scriptPath = temporaryPath("wevoa-http-script-", ".ps1");
    const std::vector<std::filesystem::path> cleanupPaths {requestPath, bodyPath, headerPath, statusPath, errorPath, scriptPath};

    try {
        Value::Object requestHeaders;
        for (const auto& [name, value] : headers) {
            requestHeaders.insert_or_assign(name, Value(value));
        }

        Value::Object requestPayload;
        requestPayload.insert_or_assign("url", Value(url));
        requestPayload.insert_or_assign("headers", Value(std::move(requestHeaders)));
        writeTextFile(requestPath, serializeJson(Value(std::move(requestPayload))));

        const std::string script = R"(param(
    [string]$RequestFile,
    [string]$BodyFile,
    [string]$HeaderFile,
    [string]$StatusFile,
    [string]$ErrorFile
)

$request = Get-Content -Raw -LiteralPath $RequestFile | ConvertFrom-Json
$headers = @{}
if ($null -ne $request.headers) {
    foreach ($prop in $request.headers.PSObject.Properties) {
        $headers[$prop.Name] = [string]$prop.Value
    }
}

$body = ""
$status = 0
$errorText = ""
$headerLines = New-Object System.Collections.Generic.List[string]

try {
    $response = Invoke-WebRequest -Uri $request.url -Headers $headers -UseBasicParsing -Method Get
    if ($null -ne $response.Content) {
        $body = [string]$response.Content
    }
    $status = [int]$response.StatusCode
    if ($null -ne $response.Headers) {
        foreach ($key in $response.Headers.Keys) {
            $headerLines.Add(([string]$key) + ': ' + ([string]$response.Headers[$key]))
        }
    }
} catch {
    $errorText = [string]$_.Exception.Message
    if ($null -ne $_.Exception.Response) {
        try {
            $status = [int]$_.Exception.Response.StatusCode.value__
        } catch {}
        try {
            foreach ($key in $_.Exception.Response.Headers.Keys) {
                $headerLines.Add(([string]$key) + ': ' + ([string]$_.Exception.Response.Headers[$key]))
            }
        } catch {}
        try {
            $stream = $_.Exception.Response.GetResponseStream()
            if ($null -ne $stream) {
                $reader = New-Object System.IO.StreamReader($stream)
                $body = $reader.ReadToEnd()
                $reader.Close()
            }
        } catch {}
    }
}

[System.IO.File]::WriteAllText($BodyFile, $body, [System.Text.UTF8Encoding]::new($false))
if ($status -gt 0) {
    $headerLines.Insert(0, 'HTTP/1.1 ' + [string]$status)
}
[System.IO.File]::WriteAllText($HeaderFile, ($headerLines -join [Environment]::NewLine), [System.Text.UTF8Encoding]::new($false))
[System.IO.File]::WriteAllText($StatusFile, [string]$status, [System.Text.UTF8Encoding]::new($false))
[System.IO.File]::WriteAllText($ErrorFile, $errorText, [System.Text.UTF8Encoding]::new($false))
exit 0
)";

        writeTextFile(scriptPath, script);

        const std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File " + quoteForWindowsCommand(scriptPath) +
                                    " " + quoteForWindowsCommand(requestPath) + " " + quoteForWindowsCommand(bodyPath) + " " +
                                    quoteForWindowsCommand(headerPath) + " " + quoteForWindowsCommand(statusPath) + " " +
                                    quoteForWindowsCommand(errorPath);

        const int exitCode = std::system(command.c_str());
        result.body = readFileContents(bodyPath);

        const std::string statusText = trim(readFileContents(statusPath));
        if (!statusText.empty()) {
            try {
                result.status = std::stoi(statusText);
            } catch (...) {
            }
        }
        parseHeaderLines(readFileContents(headerPath), result.status, result.headers);
        result.error = trim(readFileContents(errorPath));

        if (exitCode != 0 && result.error.empty()) {
            result.error = "PowerShell HTTP fetch failed.";
        }
    } catch (const std::exception& error) {
        result.error = error.what();
    }

    cleanupTemporaryFiles(cleanupPaths);
    return result;
}
#else
std::string escapeForPosixSingleQuotes(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value) {
        if (ch == '\'') {
            escaped += "'\\''";
        } else {
            escaped.push_back(ch);
        }
    }
    return escaped;
}

bool commandExists(const std::string& command) {
    const std::string probe = "command -v '" + escapeForPosixSingleQuotes(command) + "' >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

HttpFetchResult fetchWithCurl(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    HttpFetchResult result;
    const auto headerPath = temporaryPath("wevoa-http-header-", ".txt");
    const auto bodyPath = temporaryPath("wevoa-http-body-", ".txt");
    const auto statusPath = temporaryPath("wevoa-http-status-", ".txt");
    const auto errorPath = temporaryPath("wevoa-http-error-", ".txt");
    const std::vector<std::filesystem::path> cleanupPaths {headerPath, bodyPath, statusPath, errorPath};

    std::string command = "curl -sSL -D '" + escapeForPosixSingleQuotes(headerPath.string()) + "' -o '" +
                          escapeForPosixSingleQuotes(bodyPath.string()) + "' -w '%{http_code}'";
    for (const auto& [name, value] : headers) {
        command += " -H '" + escapeForPosixSingleQuotes(name + ": " + value) + "'";
    }
    command += " '" + escapeForPosixSingleQuotes(url) + "' > '" + escapeForPosixSingleQuotes(statusPath.string()) +
               "' 2> '" + escapeForPosixSingleQuotes(errorPath.string()) + "'";

    const int exitCode = std::system(command.c_str());
    result.body = readFileContents(bodyPath);
    const std::string statusText = trim(readFileContents(statusPath));
    const std::string headerText = readFileContents(headerPath);
    const std::string errorText = trim(readFileContents(errorPath));

    if (!statusText.empty()) {
        try {
            result.status = std::stoi(statusText);
        } catch (...) {
        }
    }

    parseHeaderLines(headerText, result.status, result.headers);

    if (exitCode != 0 && result.status == 0) {
        result.error = errorText.empty() ? "curl failed to fetch URL." : errorText;
    }

    cleanupTemporaryFiles(cleanupPaths);
    return result;
}

HttpFetchResult fetchWithWget(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    HttpFetchResult result;
    const auto headerPath = temporaryPath("wevoa-http-header-", ".txt");
    const auto bodyPath = temporaryPath("wevoa-http-body-", ".txt");
    const std::vector<std::filesystem::path> cleanupPaths {headerPath, bodyPath};

    std::string command =
        "wget -q -S -O '" + escapeForPosixSingleQuotes(bodyPath.string()) + "'";
    for (const auto& [name, value] : headers) {
        command += " --header='" + escapeForPosixSingleQuotes(name + ": " + value) + "'";
    }
    command += " '" + escapeForPosixSingleQuotes(url) + "' > /dev/null 2> '" +
               escapeForPosixSingleQuotes(headerPath.string()) + "'";

    const int exitCode = std::system(command.c_str());
    result.body = readFileContents(bodyPath);
    const std::string headerText = readFileContents(headerPath);
    parseHeaderLines(headerText, result.status, result.headers);

    if (exitCode != 0 && result.status == 0) {
        result.error = trim(headerText);
        if (result.error.empty()) {
            result.error = "wget failed to fetch URL.";
        }
    }

    cleanupTemporaryFiles(cleanupPaths);
    return result;
}

HttpFetchResult fetchGetPosix(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    if (commandExists("curl")) {
        return fetchWithCurl(url, headers);
    }
    if (commandExists("wget")) {
        return fetchWithWget(url, headers);
    }

    HttpFetchResult result;
    result.error = "No supported HTTP fetch tool was found. Install curl or wget.";
    return result;
}
#endif

HttpFetchResult fetchGet(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
#ifdef _WIN32
    return fetchGetWindows(url, headers);
#else
    return fetchGetPosix(url, headers);
#endif
}

Value buildResponseValue(const HttpFetchResult& result) {
    const bool ok = result.error.empty() && result.status >= 200 && result.status < 300;

    Value jsonData {};
    bool jsonValid = false;
    std::string jsonError;
    if (!result.body.empty()) {
        try {
            jsonData = parseJsonString(result.body);
            jsonValid = true;
        } catch (const std::exception& error) {
            jsonError = error.what();
        }
    }

    Value::Object response;
    response.insert_or_assign("ok", Value(ok));
    response.insert_or_assign("status", Value(static_cast<std::int64_t>(result.status)));
    response.insert_or_assign("status_code", Value(static_cast<std::int64_t>(result.status)));
    response.insert_or_assign("body", Value(result.body));
    response.insert_or_assign("error", Value(result.error));
    response.insert_or_assign("headers_data", Value(headerValueObject(result.headers)));
    response.insert_or_assign("json_data", jsonData);
    response.insert_or_assign("json_valid", Value(jsonValid));
    response.insert_or_assign("json_error", Value(jsonError));
    response.insert_or_assign(
        "header",
        Value(std::make_shared<NativeFunction>(
            "http.response.header",
            1,
            [headers = result.headers](Interpreter&, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
                if (!arguments.front().isString()) {
                    throw RuntimeError("response.header() expects a header name string.", span);
                }

                const auto found = headers.find(lowerCase(arguments.front().asString()));
                if (found == headers.end()) {
                    return Value {};
                }
                return Value(found->second);
            })));
    response.insert_or_assign(
        "json",
        Value(std::make_shared<NativeFunction>(
            "http.response.json",
            0,
            [jsonData, jsonValid](Interpreter&, const std::vector<Value>&, const SourceSpan&) -> Value {
                if (!jsonValid) {
                    return Value {};
                }
                return jsonData;
            })));
    return Value(std::move(response));
}

Value makeHttpModuleObject() {
    Value::Object object;
    object.insert_or_assign(
        "get",
        Value(std::make_shared<NativeFunction>(
            "http.get",
            std::nullopt,
            [](Interpreter&, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
                if (arguments.empty() || arguments.size() > 2) {
                    throw RuntimeError("http.get() expects a URL string and optional headers object.", span);
                }
                if (!arguments.front().isString()) {
                    throw RuntimeError("http.get() URL must be a string.", span);
                }

                const std::string& url = arguments.front().asString();
                if (!isHttpUrl(url)) {
                    throw RuntimeError("http.get() only supports http:// and https:// URLs.", span);
                }
                if (hasDisallowedControlCharacters(url)) {
                    throw RuntimeError("http.get() URL must not contain newlines.", span);
                }

                Value::Object headers = arguments.size() == 2 ? normalizeHeaderObject(arguments[1], span) : Value::Object {};
                if (headers.find("User-Agent") == headers.end() && headers.find("user-agent") == headers.end()) {
                    headers.insert_or_assign("User-Agent",
                                             Value(std::string(kWevoaRuntimeName) + "/" + std::string(kWevoaVersion)));
                }

                const auto result = fetchGet(url, headerPairs(headers));
                return buildResponseValue(result);
            })));

    return Value(std::move(object));
}

}  // namespace

void registerHttpModule(Interpreter& interpreter) {
    interpreter.defineGlobal("http", makeHttpModuleObject(), true);
}

}  // namespace wevoaweb
