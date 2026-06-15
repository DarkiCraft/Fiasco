#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "fiasco/internal/http/request.hpp"
#include "fiasco/internal/http/response.hpp"

namespace fiasco::detail {

using request_handler = std::function<response(request)>;

class tcp_server {
  public:
    tcp_server(uint16_t port,
               const std::string& host,
               request_handler handler,
               unsigned int num_threads = 0);

    void run();
    void stop() noexcept;

    ~tcp_server();

  private:
    void accept();

    struct impl;
    std::unique_ptr<impl> p_impl;
};

}  // namespace fiasco::detail