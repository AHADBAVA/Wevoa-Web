#include "server/http_server.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

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

std::string readRequest(NativeSocket clientSocket) {
    std::string request;
    std::array<char, 4096> buffer {};

    while (request.find("\r\n\r\n") == std::string::npos && request.size() < 16384) {
#ifdef _WIN32
        const int received = ::recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
        const int received = static_cast<int>(::recv(clientSocket, buffer.data(), buffer.size(), 0));
#endif
        if (received <= 0) {
            break;
        }

        request.append(buffer.data(), static_cast<std::size_t>(received));
    }

    return request;
}

std::string trimLineEnd(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

std::string normalizePath(const std::string& target) {
    const auto queryPos = target.find_first_of("?#");
    std::string path = target.substr(0, queryPos);
    if (path.empty()) {
        path = "/";
    }
    return path;
}

HttpRequest parseRequest(const std::string& rawRequest) {
    std::istringstream stream(rawRequest);
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

    request.path = normalizePath(request.target);
    return request;
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
    if (request.method != "GET") {
        return HttpResponse {
            405,
            "Method Not Allowed",
            "text/html; charset=utf-8",
            "<h1>405 Method Not Allowed</h1><p>Only GET requests are supported.</p>",
            {{"Allow", "GET"}},
        };
    }

    if (const auto asset = application_.loadStaticAsset(request.path); asset.has_value()) {
        return HttpResponse {200, "OK", asset->contentType, asset->body, {}};
    }

    if (!application_.hasRoute(request.path)) {
        return HttpResponse {
            404,
            "Not Found",
            "text/html; charset=utf-8",
            "<h1>404 Not Found</h1><p>No WevoaWeb route matched " + request.path + ".</p>",
            {},
        };
    }

    return HttpResponse {200, "OK", "text/html; charset=utf-8", application_.render(request.path), {}};
}

}  // namespace wevoaweb::server
