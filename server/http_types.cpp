#include "server/http_types.h"

namespace wevoaweb::server {

std::string HttpResponse::serialize() const {
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + reasonPhrase + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n";

    for (const auto& [name, value] : headers) {
        response += name + ": " + value + "\r\n";
    }

    response += "\r\n";
    response += body;
    return response;
}

}  // namespace wevoaweb::server
