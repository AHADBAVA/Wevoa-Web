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
    writer_.createDirectory(targetPath / "storage");

    writer_.writeTextFile(targetPath / "app" / "shared.wev", sharedTemplate(displayName));
    writer_.writeTextFile(targetPath / "app" / "main.wev", mainTemplate());
    writer_.writeTextFile(targetPath / "views" / "layout.wev", layoutTemplate(displayName));
    writer_.writeTextFile(targetPath / "views" / "home.wev", homeTemplate());
    writer_.writeTextFile(targetPath / "views" / "about.wev", aboutTemplate());
    writer_.writeTextFile(targetPath / "views" / "users.wev", usersTemplate());
    writer_.writeTextFile(targetPath / "views" / "contact.wev", contactTemplate());
    writer_.writeTextFile(targetPath / "views" / "submitted.wev", submittedTemplate());
    writer_.writeTextFile(targetPath / "views" / "submissions.wev", submissionsTemplate());
    writer_.writeTextFile(targetPath / "public" / "style.css", styleTemplate());
    writer_.writeTextFile(targetPath / "storage" / ".gitkeep", "");
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
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "<!DOCTYPE html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "  <title>{{ title }}</title>\n"
           "  <link rel=\"stylesheet\" href=\"/style.css\">\n"
           "</head>\n"
           "<body>\n"
           "  <div class=\"page-shell\">\n"
           "    <header class=\"site-header\">\n"
           "      <a class=\"brand\" href=\"/\">" +
           escapedName +
           "</a>\n"
           "      <nav class=\"site-nav\">\n"
           "        <a href=\"/\">Home</a>\n"
           "        <a href=\"/about\">About</a>\n"
           "        <a href=\"/users\">Users</a>\n"
           "        <a href=\"/contact\">Contact</a>\n"
           "        <a href=\"/submissions\">Submissions</a>\n"
           "      </nav>\n"
           "    </header>\n"
           "\n"
           "    <main class=\"page-frame\">\n"
           "      {{ content }}\n"
           "    </main>\n"
           "  </div>\n"
           "</body>\n"
           "</html>\n";
}

std::string ProjectCreator::sharedTemplate(const std::string& displayName) {
    const std::string escapedName = escapeForQuotedLiteral(displayName);

    return "const site = {\n"
           "  name: \"" +
           escapedName +
           "\",\n"
           "  hero: \"Welcome to " +
           escapedName +
           "\",\n"
           "  tagline: \"A full-stack WevoaWeb starter with server-rendered templates and SQLite-backed storage.\",\n"
           "  about: \"" +
           escapedName +
           " is a clean WevoaWeb project template with backend routes in app/, templates in views/, and assets in public/.\"\n"
           "}\n"
           "\n"
           "const db = sqlite.open(config.database)\n"
           "\n"
           "db.exec(\"CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL)\")\n"
           "db.exec(\"CREATE TABLE IF NOT EXISTS contact_submissions (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, email TEXT NOT NULL, message TEXT NOT NULL)\")\n"
           "\n"
           "if (db.scalar(\"SELECT COUNT(*) FROM users\") == 0) {\n"
           "  db.exec(\"INSERT INTO users (name) VALUES (?)\", [\"Ahad\"])\n"
           "  db.exec(\"INSERT INTO users (name) VALUES (?)\", [\"Ali\"])\n"
           "  db.exec(\"INSERT INTO users (name) VALUES (?)\", [\"John\"])\n"
           "}\n";
}

std::string ProjectCreator::mainTemplate() {
    return "import \"shared.wev\"\n"
           "\n"
           "route \"/\" {\n"
           "return view(\"home.wev\", {\n"
           "  title: \"Home\",\n"
           "  site: site,\n"
           "  env: config.env\n"
           "})\n"
           "}\n"
           "\n"
           "route \"/about\" {\n"
           "return view(\"about.wev\", {\n"
           "  title: \"About\",\n"
           "  site: site\n"
           "})\n"
           "}\n"
           "\n"
           "route \"/users\" {\n"
           "let user_rows = db.query(\"SELECT name FROM users ORDER BY id ASC LIMIT 3\")\n"
           "let user_one = \"No user yet\"\n"
           "let user_two = \"Waiting for another user\"\n"
           "let user_three = \"Waiting for another user\"\n"
           "\n"
           "if (len(user_rows) > 0) {\n"
           "  user_one = user_rows[0].name\n"
           "}\n"
           "\n"
           "if (len(user_rows) > 1) {\n"
           "  user_two = user_rows[1].name\n"
           "}\n"
           "\n"
           "if (len(user_rows) > 2) {\n"
           "  user_three = user_rows[2].name\n"
           "}\n"
           "\n"
           "return view(\"users.wev\", {\n"
           "  title: \"Users\",\n"
           "  site: site,\n"
           "  user_one: user_one,\n"
           "  user_two: user_two,\n"
           "  user_three: user_three\n"
           "})\n"
           "}\n"
           "\n"
           "route \"/contact\" {\n"
           "return view(\"contact.wev\", {\n"
           "  title: \"Contact\",\n"
           "  site: site\n"
           "})\n"
           "}\n"
           "\n"
           "route \"/submissions\" {\n"
           "let submission_rows = db.query(\"SELECT name, email, message FROM contact_submissions ORDER BY id DESC LIMIT 3\")\n"
           "let total = db.scalar(\"SELECT COUNT(*) FROM contact_submissions\")\n"
           "\n"
           "let latest = {\n"
           "  name: \"No submission yet\",\n"
           "  email: \"\",\n"
           "  message: \"Submit the contact form to see data appear here.\"\n"
           "}\n"
           "\n"
           "let previous = {\n"
           "  name: \"Waiting for more submissions\",\n"
           "  email: \"\",\n"
           "  message: \"The second-most-recent message will appear here.\"\n"
           "}\n"
           "\n"
           "let older = {\n"
           "  name: \"Waiting for more submissions\",\n"
           "  email: \"\",\n"
           "  message: \"A third recent message will appear here.\"\n"
           "}\n"
           "\n"
           "if (len(submission_rows) > 0) {\n"
           "  latest = submission_rows[0]\n"
           "}\n"
           "\n"
           "if (len(submission_rows) > 1) {\n"
           "  previous = submission_rows[1]\n"
           "}\n"
           "\n"
           "if (len(submission_rows) > 2) {\n"
           "  older = submission_rows[2]\n"
           "}\n"
           "\n"
           "return view(\"submissions.wev\", {\n"
           "  title: \"Submissions\",\n"
           "  site: site,\n"
           "  total: total,\n"
           "  latest: latest,\n"
           "  previous: previous,\n"
           "  older: older\n"
           "})\n"
           "}\n"
           "\n"
           "route \"/submit\" method POST {\n"
           "let name = request.form(\"name\")\n"
           "let email = request.form(\"email\")\n"
           "let message = request.form(\"message\")\n"
           "\n"
           "if (!name || name == \"\") {\n"
           "  name = \"Guest\"\n"
           "}\n"
           "\n"
           "if (!email || email == \"\") {\n"
           "  email = \"not provided\"\n"
           "}\n"
           "\n"
           "if (!message || message == \"\") {\n"
           "  message = \"No message was submitted.\"\n"
           "}\n"
           "\n"
           "db.exec(\"INSERT INTO contact_submissions (name, email, message) VALUES (?, ?, ?)\", [name, email, message])\n"
           "\n"
           "return view(\"submitted.wev\", {\n"
           "  title: \"Submitted\",\n"
           "  site: site,\n"
           "  name: name,\n"
           "  email: email,\n"
           "  message: message\n"
           "})\n"
           "}\n";
}

std::string ProjectCreator::homeTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card hero-card\">\n"
           "  <p class=\"eyebrow\">{{ env }}</p>\n"
           "  <h1>{{ site.hero }}</h1>\n"
           "  <p class=\"lead\">{{ site.tagline }}</p>\n"
           "  <div class=\"quick-links\">\n"
           "    <a href=\"/about\">Read about the project</a>\n"
           "    <a href=\"/users\">View SQL-backed users</a>\n"
           "    <a href=\"/contact\">Try the contact form</a>\n"
           "    <a href=\"/submissions\">See stored submissions</a>\n"
           "  </div>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::aboutTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card\">\n"
           "  <h1>About</h1>\n"
           "  <p class=\"lead\">{{ site.about }}</p>\n"
           "  <p class=\"body-copy\">This starter separates backend route code into app/, frontend templates into views/, public assets into public/, and SQLite data into storage/.</p>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::usersTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card\">\n"
           "  <h1>Users</h1>\n"
           "  <p class=\"lead\">These rows come from SQLite through WevoaWeb's native database API.</p>\n"
           "  <ul class=\"user-list\">\n"
           "    <li>{{ user_one }}</li>\n"
           "    <li>{{ user_two }}</li>\n"
           "    <li>{{ user_three }}</li>\n"
           "  </ul>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::contactTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card\">\n"
           "  <h1>Contact Us</h1>\n"
           "  <p class=\"lead\">Submit this form to test POST handling and SQLite-backed persistence.</p>\n"
           "  <p class=\"body-copy\">The form writes to the project's main SQL database and the submissions page reads the latest rows back.</p>\n"
           "\n"
           "  <form class=\"contact-form\" method=\"POST\" action=\"/submit\">\n"
           "    <input type=\"text\" name=\"name\" placeholder=\"Your Name\">\n"
           "    <input type=\"email\" name=\"email\" placeholder=\"Your Email\">\n"
           "    <textarea name=\"message\" placeholder=\"Your Message\"></textarea>\n"
           "    <button type=\"submit\">Send</button>\n"
           "  </form>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::submittedTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card\">\n"
           "  <h1>Thank You {{ name }}!</h1>\n"
           "  <p class=\"lead\">Your message has been stored by the WevoaWeb backend.</p>\n"
           "  <div class=\"submission-box\">\n"
           "    <p><strong>Email:</strong> {{ email }}</p>\n"
           "    <p><strong>Your message:</strong></p>\n"
           "    <p>{{ message }}</p>\n"
           "  </div>\n"
           "  <div class=\"quick-links\">\n"
           "    <a href=\"/contact\">Send another message</a>\n"
           "    <a href=\"/submissions\">View stored submissions</a>\n"
           "  </div>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::submissionsTemplate() {
    return "extend \"layout.wev\"\n"
           "\n"
           "section content {\n"
           "<section class=\"page-card\">\n"
           "  <h1>Submitted Data</h1>\n"
           "  <p class=\"lead\">This page is rendered from SQLite data read by the backend.</p>\n"
           "  <p class=\"body-copy\">Total submissions in the database: {{ total }}</p>\n"
           "\n"
           "  <div class=\"submission-list\">\n"
           "    <article class=\"submission-box\">\n"
           "      <p><strong>Latest</strong></p>\n"
           "      <p><strong>Name:</strong> {{ latest.name }}</p>\n"
           "      <p><strong>Email:</strong> {{ latest.email }}</p>\n"
           "      <p>{{ latest.message }}</p>\n"
           "    </article>\n"
           "\n"
           "    <article class=\"submission-box\">\n"
           "      <p><strong>Previous</strong></p>\n"
           "      <p><strong>Name:</strong> {{ previous.name }}</p>\n"
           "      <p><strong>Email:</strong> {{ previous.email }}</p>\n"
           "      <p>{{ previous.message }}</p>\n"
           "    </article>\n"
           "\n"
           "    <article class=\"submission-box\">\n"
           "      <p><strong>Older</strong></p>\n"
           "      <p><strong>Name:</strong> {{ older.name }}</p>\n"
           "      <p><strong>Email:</strong> {{ older.email }}</p>\n"
           "      <p>{{ older.message }}</p>\n"
           "    </article>\n"
           "  </div>\n"
           "</section>\n"
           "}\n";
}

std::string ProjectCreator::styleTemplate() {
    return "body {\n"
           "  margin: 0;\n"
           "  font-family: Arial, sans-serif;\n"
           "  background: #0f172a;\n"
           "  color: white;\n"
           "}\n"
           "\n"
           "a {\n"
           "  color: inherit;\n"
           "  text-decoration: none;\n"
           "}\n"
           "\n"
           ".page-shell {\n"
           "  width: min(960px, calc(100% - 2rem));\n"
           "  margin: 0 auto;\n"
           "  padding: 2rem 0 3rem;\n"
           "}\n"
           "\n"
           ".site-header {\n"
           "  display: flex;\n"
           "  align-items: center;\n"
           "  justify-content: space-between;\n"
           "  gap: 1rem;\n"
           "  margin-bottom: 2rem;\n"
           "}\n"
           "\n"
           ".brand {\n"
           "  font-size: 1.2rem;\n"
           "  font-weight: 700;\n"
           "}\n"
           "\n"
           ".site-nav {\n"
           "  display: flex;\n"
           "  flex-wrap: wrap;\n"
           "  gap: 1rem;\n"
           "}\n"
           "\n"
           ".site-nav a {\n"
           "  color: cyan;\n"
           "}\n"
           "\n"
           ".page-frame {\n"
           "  display: grid;\n"
           "  place-items: center;\n"
           "}\n"
           "\n"
           ".page-card {\n"
           "  width: min(720px, 100%);\n"
           "  padding: 2rem;\n"
           "  border: 1px solid rgba(255, 255, 255, 0.12);\n"
           "  border-radius: 20px;\n"
           "  background: rgba(15, 23, 42, 0.88);\n"
           "  box-shadow: 0 20px 50px rgba(0, 0, 0, 0.28);\n"
           "  text-align: center;\n"
           "}\n"
           "\n"
           ".hero-card {\n"
           "  padding-top: 2.5rem;\n"
           "  padding-bottom: 2.5rem;\n"
           "}\n"
           "\n"
           ".eyebrow {\n"
           "  margin: 0 0 1rem;\n"
           "  color: cyan;\n"
           "  font-size: 0.8rem;\n"
           "  letter-spacing: 0.18em;\n"
           "  text-transform: uppercase;\n"
           "}\n"
           "\n"
           "h1 {\n"
           "  margin: 0;\n"
           "  font-size: clamp(2rem, 5vw, 3.2rem);\n"
           "}\n"
           "\n"
           ".lead,\n"
           ".body-copy {\n"
           "  color: #cbd5e1;\n"
           "  line-height: 1.7;\n"
           "}\n"
           "\n"
           ".lead {\n"
           "  margin: 1rem 0 0;\n"
           "}\n"
           "\n"
           ".body-copy {\n"
           "  margin-top: 1rem;\n"
           "}\n"
           "\n"
           ".quick-links {\n"
           "  display: flex;\n"
           "  justify-content: center;\n"
           "  flex-wrap: wrap;\n"
           "  gap: 0.8rem;\n"
           "  margin-top: 1.75rem;\n"
           "}\n"
           "\n"
           ".quick-links a,\n"
           ".back-link,\n"
           ".contact-form button {\n"
           "  display: inline-flex;\n"
           "  align-items: center;\n"
           "  justify-content: center;\n"
           "  min-height: 44px;\n"
           "  padding: 0 1.1rem;\n"
           "  border: 0;\n"
           "  border-radius: 999px;\n"
           "  background: cyan;\n"
           "  color: #042f2e;\n"
           "  font-weight: 700;\n"
           "}\n"
           "\n"
           ".user-list {\n"
           "  margin: 1.5rem auto 0;\n"
           "  padding: 0;\n"
           "  list-style: none;\n"
           "  display: grid;\n"
           "  gap: 0.75rem;\n"
           "  max-width: 320px;\n"
           "}\n"
           "\n"
           ".user-list li,\n"
           ".submission-box {\n"
           "  padding: 1rem;\n"
           "  border-radius: 14px;\n"
           "  background: rgba(148, 163, 184, 0.12);\n"
           "}\n"
           "\n"
           ".submission-list {\n"
           "  display: grid;\n"
           "  gap: 1rem;\n"
           "  margin-top: 1.5rem;\n"
           "}\n"
           "\n"
           ".contact-form {\n"
           "  display: grid;\n"
           "  gap: 0.9rem;\n"
           "  margin-top: 1.5rem;\n"
           "}\n"
           "\n"
           ".contact-form input,\n"
           ".contact-form textarea {\n"
           "  width: 100%;\n"
           "  padding: 0.95rem 1rem;\n"
           "  border: 1px solid rgba(255, 255, 255, 0.12);\n"
           "  border-radius: 14px;\n"
           "  background: rgba(15, 23, 42, 0.65);\n"
           "  color: white;\n"
           "  font: inherit;\n"
           "}\n"
           "\n"
           ".contact-form textarea {\n"
           "  min-height: 140px;\n"
           "  resize: vertical;\n"
           "}\n"
           "\n"
           ".submission-box {\n"
           "  margin-top: 1.5rem;\n"
           "  text-align: left;\n"
           "  color: #cbd5e1;\n"
           "}\n"
           "\n"
           ".submission-box p {\n"
           "  margin: 0.5rem 0 0;\n"
           "}\n"
           "\n"
           ".back-link {\n"
           "  margin-top: 1.5rem;\n"
           "}\n"
           "\n"
           "@media (max-width: 720px) {\n"
           "  .site-header {\n"
           "    flex-direction: column;\n"
           "    align-items: flex-start;\n"
           "  }\n"
           "\n"
           "  .page-card {\n"
           "    padding: 1.5rem;\n"
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
           "\",\n"
           "  \"database\": \"storage/app.sqlite\"\n"
           "}\n";
}

std::string ProjectCreator::readmeTemplate(const std::string& displayName) {
    return "# " + displayName +
           "\n\n"
           "Generated with WevoaWeb.\n\n"
           "## Structure\n\n"
           "- `app/` backend routes and database bootstrap\n"
           "- `views/` HTML templates and layouts\n"
           "- `public/` static frontend assets\n"
           "- `storage/` SQLite database files\n"
           "- `wevoa.config.json` project configuration\n";
}

}  // namespace wevoaweb
