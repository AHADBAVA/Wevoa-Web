#include "server/http_server.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "interpreter/callable.h"
#include "utils/error.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace wevoaweb::server {

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
using SocketLength = int;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;

std::string lastSocketErrorMessage() {
    return "WSA error " + std::to_string(WSAGetLastError());
}

int closeNativeSocket(NativeSocket socket) {
    return closesocket(socket);
}

class SocketSystem final {
  public:
    SocketSystem() {
        WSADATA data {};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed: " + lastSocketErrorMessage());
        }
    }

    ~SocketSystem() {
        WSACleanup();
    }
};
#else
using NativeSocket = int;
using SocketLength = socklen_t;
constexpr NativeSocket kInvalidSocket = -1;
constexpr int kSocketError = -1;

std::string lastSocketErrorMessage() {
    return std::strerror(errno);
}

int closeNativeSocket(NativeSocket socket) {
    return close(socket);
}

class SocketSystem final {
  public:
    SocketSystem() = default;
};
#endif

#ifdef _WIN32
constexpr int kSocketError = SOCKET_ERROR;
#endif

class Socket final {
  public:
    Socket() = default;
    explicit Socket(NativeSocket socket) : socket_(socket) {}

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : socket_(other.socket_) {
        other.socket_ = kInvalidSocket;
    }

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            reset();
            socket_ = other.socket_;
            other.socket_ = kInvalidSocket;
        }
        return *this;
    }

    ~Socket() {
        reset();
    }

    [[nodiscard]] bool valid() const {
        return socket_ != kInvalidSocket;
    }

    [[nodiscard]] NativeSocket native() const {
        return socket_;
    }

    void reset(NativeSocket replacement = kInvalidSocket) {
        if (valid()) {
            closeNativeSocket(socket_);
        }
        socket_ = replacement;
    }

  private:
    NativeSocket socket_ = kInvalidSocket;
};

Socket createListeningSocket(std::uint16_t port) {
    Socket listenSocket(::socket(AF_INET, SOCK_STREAM, 0));
    if (!listenSocket.valid()) {
        throw std::runtime_error("Unable to create socket: " + lastSocketErrorMessage());
    }

    int reuseAddress = 1;
    if (::setsockopt(listenSocket.native(),
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     reinterpret_cast<const char*>(&reuseAddress),
                     sizeof(reuseAddress)) < 0) {
        throw std::runtime_error("Unable to configure socket: " + lastSocketErrorMessage());
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(listenSocket.native(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Unable to bind socket on port " + std::to_string(port) + ": " +
                                 lastSocketErrorMessage());
    }

    if (::listen(listenSocket.native(), 16) < 0) {
        throw std::runtime_error("Unable to listen on socket: " + lastSocketErrorMessage());
    }

    return listenSocket;
}

std::string trimLineEnd(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

std::string readRequest(NativeSocket clientSocket) {
    std::string request;
    std::array<char, 4096> buffer {};
    std::size_t contentLength = 0;
    std::size_t headerEnd = std::string::npos;

    while (request.size() < 65536) {
#ifdef _WIN32
        const int received = ::recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
        const int received = static_cast<int>(::recv(clientSocket, buffer.data(), buffer.size(), 0));
#endif
        if (received <= 0) {
            break;
        }

        request.append(buffer.data(), static_cast<std::size_t>(received));

        if (headerEnd == std::string::npos) {
            headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::istringstream headerStream(request.substr(0, headerEnd));
                std::string line;
                std::getline(headerStream, line);

                while (std::getline(headerStream, line)) {
                    line = trimLineEnd(line);
                    const auto separator = line.find(':');
                    if (separator == std::string::npos) {
                        continue;
                    }

                    std::string name = line.substr(0, separator);
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    });

                    if (name == "content-length") {
                        const std::string value = line.substr(separator + 1);
                        contentLength = static_cast<std::size_t>(std::stoul(value));
                        break;
                    }
                }
            }
        }

        if (headerEnd != std::string::npos && request.size() >= headerEnd + 4 + contentLength) {
            break;
        }
    }

    return request;
}

std::string normalizePath(const std::string& target) {
    const auto queryPos = target.find_first_of("?#");
    std::string path = target.substr(0, queryPos);
    if (path.empty()) {
        path = "/";
    }
    return path;
}

std::string decodeUrlComponent(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
            continue;
        }

        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            decoded.push_back(static_cast<char>(std::stoi(hex, nullptr, 16)));
            i += 2;
            continue;
        }

        decoded.push_back(value[i]);
    }

    return decoded;
}

std::unordered_map<std::string, std::string> parseUrlEncoded(const std::string& value) {
    std::unordered_map<std::string, std::string> pairs;
    std::size_t cursor = 0;

    while (cursor <= value.size()) {
        const auto separator = value.find('&', cursor);
        const std::string segment =
            separator == std::string::npos ? value.substr(cursor) : value.substr(cursor, separator - cursor);

        if (!segment.empty()) {
            const auto equals = segment.find('=');
            if (equals == std::string::npos) {
                pairs.insert_or_assign(decodeUrlComponent(segment), "");
            } else {
                pairs.insert_or_assign(decodeUrlComponent(segment.substr(0, equals)),
                                       decodeUrlComponent(segment.substr(equals + 1)));
            }
        }

        if (separator == std::string::npos) {
            break;
        }

        cursor = separator + 1;
    }

    return pairs;
}

std::string getHeaderValue(const std::vector<std::pair<std::string, std::string>>& headers, const std::string& name) {
    for (const auto& [headerName, headerValue] : headers) {
        if (headerName == name) {
            return headerValue;
        }
    }

    return "";
}

HttpRequest parseRequest(const std::string& rawRequest) {
    const auto headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Malformed HTTP request.");
    }

    std::istringstream stream(rawRequest.substr(0, headerEnd));
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        throw std::runtime_error("Empty HTTP request.");
    }

    requestLine = trimLineEnd(requestLine);

    std::istringstream requestLineStream(requestLine);
    HttpRequest request;
    if (!(requestLineStream >> request.method >> request.target >> request.version)) {
        throw std::runtime_error("Malformed HTTP request line.");
    }

    std::transform(request.method.begin(), request.method.end(), request.method.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    request.path = normalizePath(request.target);
    request.body = rawRequest.substr(headerEnd + 4);

    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        headerLine = trimLineEnd(headerLine);
        if (headerLine.empty()) {
            continue;
        }

        const auto separator = headerLine.find(':');
        if (separator == std::string::npos) {
            throw std::runtime_error("Malformed HTTP header.");
        }

        std::string name = headerLine.substr(0, separator);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        std::string value = headerLine.substr(separator + 1);
        if (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }

        request.headers.emplace_back(std::move(name), std::move(value));
    }

    const auto queryPos = request.target.find('?');
    if (queryPos != std::string::npos) {
        const auto fragmentPos = request.target.find('#', queryPos);
        const auto queryString = request.target.substr(queryPos + 1, fragmentPos - queryPos - 1);
        request.queryParameters = parseUrlEncoded(queryString);
    }

    const std::string contentType = getHeaderValue(request.headers, "content-type");
    if (request.method == "POST" && contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        request.formParameters = parseUrlEncoded(request.body);
    }

    return request;
}

Value::Object stringMapToObject(const std::unordered_map<std::string, std::string>& values) {
    Value::Object object;
    for (const auto& [key, value] : values) {
        object.insert_or_assign(key, Value(value));
    }
    return object;
}

Value makeLookupFunctionValue(std::string name, std::unordered_map<std::string, std::string> values) {
    return Value(std::make_shared<NativeFunction>(
        std::move(name),
        1,
        [values = std::move(values)](Interpreter&, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
            if (arguments.size() != 1 || !arguments.front().isString()) {
                throw RuntimeError("Request lookup expects one string key.", span);
            }

            const auto found = values.find(arguments.front().asString());
            if (found == values.end()) {
                return Value(std::string());
            }

            return Value(found->second);
        }));
}

Value makeRequestValue(const HttpRequest& request) {
    Value::Object object;
    object.insert_or_assign("method", Value(request.method));
    object.insert_or_assign("path", Value(request.path));
    object.insert_or_assign("target", Value(request.target));
    object.insert_or_assign("body", Value(request.body));
    object.insert_or_assign("query", makeLookupFunctionValue("request.query", request.queryParameters));
    object.insert_or_assign("form", makeLookupFunctionValue("request.form", request.formParameters));
    object.insert_or_assign("query_data", Value(stringMapToObject(request.queryParameters)));
    object.insert_or_assign("form_data", Value(stringMapToObject(request.formParameters)));
    return Value(std::move(object));
}

void sendAll(NativeSocket socket, const std::string& payload) {
    std::size_t totalSent = 0;
    while (totalSent < payload.size()) {
        const auto remaining = payload.size() - totalSent;
#ifdef _WIN32
        const int sent =
            ::send(socket, payload.data() + totalSent, static_cast<int>(remaining), 0);
#else
        const int sent = static_cast<int>(::send(socket, payload.data() + totalSent, remaining, 0));
#endif
        if (sent <= 0) {
            throw std::runtime_error("Failed to send HTTP response: " + lastSocketErrorMessage());
        }

        totalSent += static_cast<std::size_t>(sent);
    }
}

}  // namespace

HttpServer::HttpServer(WebApplication& application, std::uint16_t port, RequestObserver observer)
    : application_(application), port_(port), observer_(std::move(observer)) {}

void HttpServer::run() {
    stopRequested_ = false;
    SocketSystem socketSystem;
    Socket listenSocket = createListeningSocket(port_);

    while (!stopRequested_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket.native(), &readSet);

        timeval timeout {};
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;

#ifdef _WIN32
        const int ready = ::select(0, &readSet, nullptr, nullptr, &timeout);
#else
        const int ready = ::select(listenSocket.native() + 1, &readSet, nullptr, nullptr, &timeout);
#endif

        if (stopRequested_) {
            break;
        }

        if (ready == 0) {
            continue;
        }

        if (ready == kSocketError) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            throw std::runtime_error("Socket wait failed: " + lastSocketErrorMessage());
        }

        sockaddr_in clientAddress {};
        SocketLength addressLength = sizeof(clientAddress);
        Socket clientSocket(::accept(listenSocket.native(),
                                     reinterpret_cast<sockaddr*>(&clientAddress),
                                     &addressLength));

        if (!clientSocket.valid()) {
            if (stopRequested_) {
                break;
            }
            throw std::runtime_error("Unable to accept client connection: " + lastSocketErrorMessage());
        }

        HttpResponse response;
        HttpRequest request;
        bool hasRequest = false;

        try {
            const std::string rawRequest = readRequest(clientSocket.native());
            if (rawRequest.empty()) {
                continue;
            }

            request = parseRequest(rawRequest);
            hasRequest = true;
            response = dispatch(request);
        } catch (const WevoaError& error) {
            response.statusCode = 500;
            response.reasonPhrase = "Internal Server Error";
            response.body = "<h1>500 Internal Server Error</h1><p>" + std::string(error.what()) + "</p>";
        } catch (const std::exception& error) {
            response.statusCode = 400;
            response.reasonPhrase = "Bad Request";
            response.body = "<h1>400 Bad Request</h1><p>" + std::string(error.what()) + "</p>";
        }

        sendAll(clientSocket.native(), response.serialize());
        if (observer_ && hasRequest) {
            observer_(request, response);
        }
    }
}

void HttpServer::stop() {
    stopRequested_ = true;
}

HttpResponse HttpServer::dispatch(const HttpRequest& request) {
    if (request.method != "GET" && request.method != "POST") {
        return HttpResponse {
            405,
            "Method Not Allowed",
            "text/html; charset=utf-8",
            "<h1>405 Method Not Allowed</h1><p>Only GET and POST requests are supported.</p>",
            {{"Allow", "GET, POST"}},
        };
    }

    if (request.method == "GET") {
        if (const auto asset = application_.loadStaticAsset(request.path); asset.has_value()) {
            return HttpResponse {200, "OK", asset->contentType, asset->body, {}};
        }
    }

    if (!application_.hasRoute(request.path, request.method)) {
        return HttpResponse {
            404,
            "Not Found",
            "text/html; charset=utf-8",
            "<h1>404 Not Found</h1><p>No WevoaWeb route matched " + request.method + " " + request.path + ".</p>",
            {},
        };
    }

    try {
        return HttpResponse {
            200,
            "OK",
            "text/html; charset=utf-8",
            application_.render(request.path, request.method, makeRequestValue(request)),
            {},
        };
    } catch (const std::exception& error) {
        return HttpResponse {
            500,
            "Internal Server Error",
            "text/html; charset=utf-8",
            "<h1>500 Internal Server Error</h1><p>" + std::string(error.what()) + "</p>",
            {},
        };
    }
}

}  // namespace wevoaweb::server
