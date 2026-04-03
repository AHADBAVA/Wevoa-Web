#pragma once

#include <string>
#include <vector>

namespace wevoaweb::server {

struct HttpRequest {
    std::string method;
    std::string target;
    std::string path;
    std::string version;
};

struct HttpResponse {
    int statusCode = 200;
    std::string reasonPhrase = "OK";
    std::string contentType = "text/html; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;

    std::string serialize() const;
};

}  // namespace wevoaweb::server
