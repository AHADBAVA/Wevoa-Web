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

    writer_.createDirectory(targetPath / "app");
    writer_.createDirectory(targetPath / "views");
    writer_.createDirectory(targetPath / "public");

    writer_.writeTextFile(targetPath / "app" / "main.wev", mainTemplate(displayName));
    writer_.writeTextFile(targetPath / "views" / "layout.wev", layoutTemplate(displayName));
    writer_.writeTextFile(targetPath / "views" / "index.wev", indexTemplate());
    writer_.writeTextFile(targetPath / "public" / "style.css", styleTemplate());
    writer_.writeTextFile(targetPath / "wevoa.config.json", configTemplate(displayName));
    writer_.writeTextFile(targetPath / "README.md", readmeTemplate(displayName));

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
    (void)displayName;

    return R"WEV(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{{ title }}</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <main class="page-shell">
    {{ content }}
  </main>
</body>
</html>
)WEV";
}

std::string ProjectCreator::mainTemplate(const std::string& displayName) {
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "const site = {\n"
           "  project_name: \"" +
           escapedName +
           "\",\n"
           "  framework_name: \"WevoaWeb\",\n"
           "  title: \"Welcome to WevoaWeb\",\n"
           "  subtitle: \"Create your first web app with WevoaWeb\"\n"
           "}\n"
           "\n"
           "route \"/\" {\n"
           "return view(\"index.wev\", {\n"
           "  title: site.framework_name,\n"
           "  site: site\n"
           "})\n"
           "}\n";
}

std::string ProjectCreator::indexTemplate() {
    return R"WEV(extend "layout.wev"

section content {
<section class="welcome-panel fade-in">
  <div class="welcome-mark" aria-hidden="true">
    <span></span>
    <span></span>
  </div>

  <div class="welcome-copy">
    <h1>{{ site.title }}</h1>
    <p>{{ site.subtitle }}</p>
  </div>
</section>
}
)WEV";
}

std::string ProjectCreator::styleTemplate() {
    return R"WEV(:root {
  color-scheme: light;
  --page: #ffffff;
  --page-soft: #f7faf8;
  --text: #0f172a;
  --muted: #526072;
  --accent: #22c55e;
}

* {
  box-sizing: border-box;
}

html,
body {
  min-height: 100%;
}

body {
  margin: 0;
  overflow: hidden;
  font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
  color: var(--text);
  background:
    radial-gradient(circle at center, rgba(34, 197, 94, 0.08), transparent 26%),
    linear-gradient(180deg, var(--page) 0%, var(--page-soft) 100%);
}

a {
  color: inherit;
  text-decoration: none;
}

.page-shell {
  min-height: 100vh;
  display: grid;
  place-items: center;
  width: min(100% - 32px, 980px);
  margin: 0 auto;
}

.welcome-panel {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 16px;
  padding: 20px 24px;
}

.welcome-mark {
  display: inline-flex;
  gap: 6px;
  align-items: center;
  transform: translateY(-1px);
}

.welcome-mark span {
  display: block;
  width: 5px;
  height: 34px;
  background: var(--accent);
  transform: skewX(-18deg);
}

.welcome-copy {
  text-align: left;
}

h1,
p {
  margin: 0;
}

h1 {
  font-size: clamp(2.4rem, 5vw, 3.4rem);
  line-height: 1.05;
  font-weight: 500;
  letter-spacing: -0.04em;
}

p {
  margin-top: 4px;
  color: var(--muted);
  font-size: 1.1rem;
  line-height: 1.5;
}

.fade-in {
  animation: rise-in 320ms ease both;
}

@keyframes rise-in {
  from {
    opacity: 0;
    transform: translateY(10px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 640px) {
  .page-shell {
    width: min(100% - 24px, 980px);
  }

  .welcome-panel {
    width: 100%;
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
    padding: 0;
  }

  .welcome-copy {
    text-align: left;
  }

  h1 {
    font-size: clamp(2rem, 10vw, 2.8rem);
  }

  p {
    font-size: 1rem;
  }
}
)WEV";
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

std::string ProjectCreator::readmeTemplate(const std::string& displayName) {
    return "# " + displayName +
           "\n\n"
           "Generated with WevoaWeb.\n\n"
           "## Structure\n\n"
           "- `app/` route definitions\n"
           "- `views/` layout and page templates\n"
           "- `public/` static styling assets\n"
           "- `wevoa.config.json` project configuration\n\n"
           "## Starter Page\n\n"
           "- `/` minimal welcome page\n\n"
           "## Run In Development\n\n"
           "```powershell\n"
           "wevoa start\n"
           "```\n\n"
           "## Build For Production\n\n"
           "```powershell\n"
           "wevoa build\n"
           "wevoa serve\n"
           "```\n";
}

}  // namespace wevoaweb
