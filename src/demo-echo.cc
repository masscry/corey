#include <corey.hh>
#include <sys/types.h>

corey::Log logger("demo-echo");

corey::Future<> echo(int client_id, corey::Client client) {
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

            size = co_await client.write(std::span(buffer.data(), size));
            logger.info("[{}] sent {} bytes", client_id, size);
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
        .name = "demo-echo",
        .description = "Echo Server using corey",
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
                std::ignore = echo(client_id++, std::move(client));
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