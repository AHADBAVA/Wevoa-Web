#include "cli/project_creator.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace wevoaweb {

namespace {

std::string humanizeProjectName(const std::filesystem::path& projectPath) {
    const std::string rawName = projectPath.filename().string();
    if (rawName.empty()) {
        return "WevoaWeb App";
    }

    std::string displayName;
    displayName.reserve(rawName.size());

    bool capitalizeNext = true;
    for (const char ch : rawName) {
        if (ch == '-' || ch == '_' || ch == ' ') {
            if (!displayName.empty() && displayName.back() != ' ') {
                displayName.push_back(' ');
            }
            capitalizeNext = true;
            continue;
        }

        const auto unsignedChar = static_cast<unsigned char>(ch);
        displayName.push_back(static_cast<char>(capitalizeNext ? std::toupper(unsignedChar)
                                                               : std::tolower(unsignedChar)));
        capitalizeNext = false;
    }

    return displayName.empty() ? "WevoaWeb App" : displayName;
}

std::string escapeForQuotedLiteral(const std::string& value) {
    std::ostringstream escaped;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped << "\\\\";
                break;
            case '"':
                escaped << "\\\"";
                break;
            case '\n':
                escaped << "\\n";
                break;
            case '\t':
                escaped << "\\t";
                break;
            default:
                escaped << ch;
                break;
        }
    }

    return escaped.str();
}

}  // namespace

ProjectCreator::ProjectCreator(FileWriter writer) : writer_(std::move(writer)) {}

std::filesystem::path ProjectCreator::createProject(const std::string& projectName) const {
    const auto targetPath = resolveTargetPath(projectName);
    const std::string displayName = humanizeProjectName(targetPath);

    std::error_code error;
    if (std::filesystem::exists(targetPath, error)) {
        if (error) {
            throw std::runtime_error("Failed to inspect target path: " + targetPath.string());
        }

        throw std::runtime_error("Project directory already exists: " + targetPath.string());
    }

    writer_.createDirectory(targetPath / "views");
    writer_.createDirectory(targetPath / "public");

    writer_.writeTextFile(targetPath / "views" / "layout.wev", layoutTemplate(displayName));
    writer_.writeTextFile(targetPath / "views" / "home.wev", homeTemplate());
    writer_.writeTextFile(targetPath / "views" / "index.wev", indexTemplate(displayName));
    writer_.writeTextFile(targetPath / "public" / "style.css", styleTemplate());
    writer_.writeTextFile(targetPath / "wevoa.config.json", configTemplate(displayName));

    return targetPath;
}

std::filesystem::path ProjectCreator::resolveTargetPath(const std::string& projectName) const {
    if (projectName.empty()) {
        throw std::runtime_error("Project name must not be empty.");
    }

    const std::filesystem::path projectPath(projectName);
    if (projectPath.has_root_path()) {
        throw std::runtime_error("Project name must be a relative folder name.");
    }

    return std::filesystem::current_path() / projectPath;
}

std::string ProjectCreator::layoutTemplate(const std::string& displayName) {
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "<!DOCTYPE html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "  <title>" +
           escapedName +
           "</title>\n"
           "  <link rel=\"stylesheet\" href=\"/style.css\">\n"
           "</head>\n"
           "<body>\n"
           "  <div class=\"site-shell\">\n"
           "    <header class=\"site-nav\">\n"
           "      <a class=\"brand\" href=\"/\">{{ title }}</a>\n"
           "      <nav class=\"nav-links\">\n"
           "        <a href=\"/\">Home</a>\n"
           "      </nav>\n"
           "    </header>\n"
           "    <main class=\"main-frame\">\n"
           "      {{content}}\n"
           "    </main>\n"
           "  </div>\n"
           "</body>\n"
           "</html>\n";
}

std::string ProjectCreator::homeTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"hero-panel\">\n"
           "  <p class=\"label\">Simple starter</p>\n"
           "  <h1>{{ title }}</h1>\n"
           "  <p class=\"subtitle\">{{ tagline }}</p>\n"
           "  <div class=\"hero-actions\">\n"
           "    <a class=\"button primary\" href=\"#\">Start editing</a>\n"
           "    <span class=\"soft-note\">views/home.wev</span>\n"
           "  </div>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::indexTemplate(const std::string& displayName) {
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "let app = {\n"
           "  title: \"" +
           escapedName +
           "\",\n"
           "  tagline: \"A clean WevoaWeb starter with server-rendered templates, zero browser JavaScript, and room to grow.\"\n"
           "}\n"
           "\n"
           "route \"/\" {\n"
           "return view(\"home.wev\", app)\n"
           "}\n";
}

std::string ProjectCreator::styleTemplate() {
    return ":root {\n"
           "  --bg: #f7fbf7;\n"
           "  --bg-soft: #edf6ee;\n"
           "  --surface: #ffffff;\n"
           "  --text: #183225;\n"
           "  --muted: #5a7365;\n"
           "  --line: rgba(24, 50, 37, 0.10);\n"
           "  --primary: #1b8a52;\n"
           "  --primary-deep: #136640;\n"
           "  --shadow: 0 18px 44px rgba(24, 50, 37, 0.08);\n"
           "}\n"
           "\n"
           "* {\n"
           "  box-sizing: border-box;\n"
           "}\n"
           "\n"
           "html,\n"
           "body {\n"
           "  margin: 0;\n"
           "  min-height: 100vh;\n"
           "  min-height: 100dvh;\n"
           "}\n"
           "\n"
           "body {\n"
           "  font-family: Georgia, \"Times New Roman\", serif;\n"
           "  color: var(--text);\n"
           "  background: linear-gradient(180deg, var(--bg) 0%, #ffffff 100%);\n"
           "  overflow: hidden;\n"
           "}\n"
           "\n"
           "a {\n"
           "  color: inherit;\n"
           "  text-decoration: none;\n"
           "}\n"
           "\n"
           ".site-shell {\n"
           "  width: min(960px, calc(100% - 2rem));\n"
           "  margin: 0 auto;\n"
           "  min-height: 100vh;\n"
           "  min-height: 100dvh;\n"
           "  padding: 1.1rem 0;\n"
           "  display: grid;\n"
           "  grid-template-rows: auto 1fr;\n"
           "}\n"
           "\n"
           ".site-nav {\n"
           "  display: flex;\n"
           "  align-items: center;\n"
           "  justify-content: space-between;\n"
           "  gap: 1rem;\n"
           "  padding: 1rem 1.2rem;\n"
           "  border: 1px solid var(--line);\n"
           "  border-radius: 18px;\n"
           "  background: rgba(255, 255, 255, 0.92);\n"
           "  box-shadow: 0 8px 20px rgba(24, 50, 37, 0.04);\n"
           "}\n"
           "\n"
           ".brand {\n"
           "  font-size: 1rem;\n"
           "  font-weight: 700;\n"
           "  letter-spacing: 0.03em;\n"
           "}\n"
           "\n"
           ".nav-links {\n"
           "  display: flex;\n"
           "  align-items: center;\n"
           "  gap: 1.1rem;\n"
           "  color: var(--muted);\n"
           "  font-weight: 600;\n"
           "  font-size: 0.95rem;\n"
           "}\n"
           "\n"
           ".main-frame {\n"
           "  display: grid;\n"
           "  place-items: center;\n"
           "}\n"
           "\n"
           ".hero-panel {\n"
           "  width: min(720px, 100%);\n"
           "  padding: 3.2rem 2.6rem;\n"
           "  border: 1px solid var(--line);\n"
           "  border-radius: 28px;\n"
           "  background: var(--surface);\n"
           "  box-shadow: var(--shadow);\n"
           "  background:\n"
           "    radial-gradient(circle at top, rgba(27, 138, 82, 0.10), transparent 38%),\n"
           "    linear-gradient(180deg, #ffffff 0%, var(--bg-soft) 100%);\n"
           "  text-align: center;\n"
           "}\n"
           "\n"
           ".label {\n"
           "  margin: 0 0 1rem;\n"
           "  color: var(--primary);\n"
           "  font-size: 0.78rem;\n"
           "  font-weight: 700;\n"
           "  letter-spacing: 0.16em;\n"
           "  text-transform: uppercase;\n"
           "}\n"
           "\n"
           ".hero-panel h1 {\n"
           "  margin: 0;\n"
           "  font-size: clamp(2.2rem, 5vw, 4rem);\n"
           "  line-height: 1.06;\n"
           "  letter-spacing: -0.04em;\n"
           "}\n"
           "\n"
           ".subtitle,\n"
           ".soft-note {\n"
           "  color: var(--muted);\n"
           "}\n"
           "\n"
           ".subtitle {\n"
           "  max-width: 34rem;\n"
           "  margin: 1rem auto 0;\n"
           "  font-size: 1rem;\n"
           "  line-height: 1.75;\n"
           "}\n"
           "\n"
           ".hero-actions {\n"
           "  display: flex;\n"
           "  align-items: center;\n"
           "  justify-content: center;\n"
           "  flex-wrap: wrap;\n"
           "  gap: 0.9rem;\n"
           "  margin-top: 1.8rem;\n"
           "}\n"
           "\n"
           ".button {\n"
           "  display: inline-flex;\n"
           "  align-items: center;\n"
           "  justify-content: center;\n"
           "  min-height: 46px;\n"
           "  padding: 0 1.35rem;\n"
           "  border-radius: 999px;\n"
           "  border: 1px solid transparent;\n"
           "  font-weight: 700;\n"
           "  font-size: 0.95rem;\n"
           "}\n"
           "\n"
           ".button.primary {\n"
           "  color: #ffffff;\n"
           "  background: linear-gradient(135deg, var(--primary), #2aa866);\n"
           "  box-shadow: 0 14px 28px rgba(27, 138, 82, 0.18);\n"
           "}\n"
           "\n"
           ".soft-note {\n"
           "  font-family: Arial, sans-serif;\n"
           "  font-size: 0.92rem;\n"
           "}\n"
           "\n"
           "@media (max-width: 640px) {\n"
           "  body {\n"
           "    overflow: auto;\n"
           "  }\n"
           "\n"
           "  .site-shell {\n"
           "    width: min(100% - 1rem, 960px);\n"
           "  }\n"
           "\n"
           "  .site-nav {\n"
           "    padding: 0.95rem 1rem;\n"
           "  }\n"
           "\n"
           "  .hero-panel {\n"
           "    padding: 2.4rem 1.4rem;\n"
           "  }\n"
           "}\n";
}

std::string ProjectCreator::configTemplate(const std::string& displayName) {
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "{\n"
           "  \"port\": 3000,\n"
           "  \"env\": \"dev\",\n"
           "  \"name\": \"" +
           escapedName +
           "\"\n"
           "}\n";
}

}  // namespace wevoaweb
