#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

#include "server/http_types.h"
#include "server/web_app.h"

namespace wevoaweb::server {

// HttpServer owns the socket accept loop and dispatches each GET request to
// a registered WevoaWeb route, returning only HTML to the browser.
class HttpServer {
  public:
    using RequestObserver = std::function<void(const HttpRequest&, const HttpResponse&)>;

    HttpServer(WebApplication& application, std::uint16_t port, RequestObserver observer = {});

    void run();
    void stop();

  private:
    HttpResponse dispatch(const HttpRequest& request);

    WebApplication& application_;
    std::uint16_t port_;
    RequestObserver observer_;
    std::atomic<bool> stopRequested_ = false;
};

}  // namespace wevoaweb::server
