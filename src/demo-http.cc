#include <corey.hh>

#include "demo-http-parse.hh"

#include <fmt/std.h>

corey::Log logger("demo-http");

corey::Future<> http(int client_id, corey::Client client) {
    std::exception_ptr eptr;
    try {
        std::array<char, 1024> buffer;
        while (true) {
            auto size = co_await client.read(buffer);
            if (size == 0) {
                logger.info("[{}] connection closed", client_id);
                break;
            }
            logger.info("[{}] received {} bytes", client_id, size);
            auto request = corey::http::parse_request(std::span(buffer.data(), size));
            logger.info("[{}] parsed request: {}", client_id, request);

            if (request.method != "GET") {
                co_await client.write(fmt::format(
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 17\r\n"
                    "\r\n"
                    "Method Not Allowed"
                ));
                continue;
            }
            if (request.path == "/") {
                co_await client.write(fmt::format(
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 12\r\n"
                    "\r\n"
                    "Hello, World"
                ));
                continue;
            }
            co_await client.write(fmt::format(
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 9\r\n"
                "\r\n"
                "Not Found"
            ));
        }
    } catch(...) {
        eptr = std::current_exception();
    }

    logger.info("[{}] closing connection", client_id);
    co_await client.close();
    if (eptr) {
        std::rethrow_exception(eptr);
    }
}

int main(int argc, char* argv[]) {
    corey::Application app(argc, argv, corey::ApplicationInfo{
        .name = "demo-http",
        .description = "HTTP Server using corey",
        .version = "0.1.0"
    });

    app.add_options()
        ("port", "Port number", cxxopts::value<uint16_t>()->default_value("8080"));
    app.add_positional_options("port");

    return app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        auto port = opts["port"].as<uint16_t>();

        auto listener = co_await corey::Socket::make_tcp_listener(port);
        std::exception_ptr eptr;
        try {
            logger.info("Listening on port {}", port);

            uint64_t client_id = 0;
            while (true) {
                auto client = co_await listener.accept();
                logger.info("New connection accepted");
                std::ignore = http(client_id++, std::move(client));
            }
        } catch (...) {
            eptr = std::current_exception();
        }
        co_await listener.close();
        if (eptr) {
            std::rethrow_exception(eptr);
        }
        co_return EXIT_SUCCESS;
    });
}