#pragma once

#include <asio.hpp>
#include <cstdint>
#include <functional>

#include "fiasco/internal/http/parser.hpp"
#include "fiasco/internal/http/request.hpp"
#include "fiasco/internal/http/response.hpp"

namespace fiasco::detail {

using asio::ip::tcp;
using request_handler = std::function<response(request)>;

struct session : std::enable_shared_from_this<session> {
    tcp::socket socket;
    llhttp_parser parser;
    std::array<char, 4096> buf;
    request_handler handler;

    session(tcp::socket sock, request_handler h)
        : socket(std::move(sock)),
          handler(std::move(h)) {}

    void start() { read(); }

    void read() {
        auto self = shared_from_this();
        socket.async_read_some(asio::buffer(buf), [self](asio::error_code ec, std::size_t n) {
            if (ec) {
                return;
            }
            self->parser.feed(self->buf.data(), n);
            if (self->parser.is_complete()) {
                self->respond();
            } else {
                self->read();  // keep reading
            }
        });
    }

    void respond() {
        request req = parser.take_request();
        response res = handler(req);

        auto self = shared_from_this();
        auto raw = std::make_shared<std::string>(res.serialize());
        asio::async_write(
            socket, asio::buffer(*raw), [self, raw](asio::error_code ec, std::size_t) {
                // raw kept alive by capture until write completes
            });
    }
};

class tcp_server {
  public:
    tcp_server(uint16_t port,
               const std::string& host,
               request_handler handler,
               unsigned int num_threads = 0)
        : m_acceptor(m_ioc, tcp::endpoint(asio::ip::make_address(host), port)),
          m_handler(std::move(handler)),
          m_threads(num_threads == 0 ? std::max(std::thread::hardware_concurrency(), 2u)
                                     : num_threads) {}

    void run() {
        accept();
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < m_threads; i++) {
            threads.emplace_back([this] { m_ioc.run(); });
        }
        for (auto& t : threads) {
            t.join();
        }
    }

    void stop() { m_ioc.stop(); }

  private:
    void accept() {
        m_acceptor.async_accept(m_ioc, [this](asio::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<session>(std::move(socket), m_handler)->start();
            }
            accept();
        });
    }

    asio::io_context m_ioc;
    tcp::acceptor m_acceptor;
    request_handler m_handler;
    unsigned int m_threads;
};

}  // namespace fiasco::detail