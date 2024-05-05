#include "socket.hh"
#include "io.hh"
#include "reactor/coroutine.hh"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <system_error>

namespace corey {

constexpr int max_backlog = 128;

Future<Server> Socket::make_tcp_listener(uint16_t port) {
    auto sock = co_await IoEngine::instance().socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        co_await std::make_exception_ptr(std::system_error(-sock, std::system_category(), "socket failed"));
    }

    int optval = 1;
    if (auto result = co_await IoEngine::instance().setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "setsockopt failed"));
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (auto result = co_await IoEngine::instance().bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "bind failed"));
    }

    if (auto result = co_await IoEngine::instance().listen(sock, max_backlog)) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "listen failed"));
    }

    co_return Server(Socket(sock));
}

Future<Client> Socket::make_tcp_connect(const char* host, uint16_t port) {
    auto sock = co_await IoEngine::instance().socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        co_await std::make_exception_ptr(std::system_error(-sock, std::system_category(), "socket failed"));
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (auto result = co_await IoEngine::instance().connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "connect failed"));
    }

    co_return Client(Socket(sock));
}

Future<Client> Socket::make_accept(Socket& accepter) {
    auto sock = co_await IoEngine::instance().accept(accepter._fd, nullptr, nullptr);
    if (sock < 0) {
        co_await std::make_exception_ptr(std::system_error(-sock, std::system_category(), "accept failed"));
    }
    co_return Client(Socket(sock));
}

Socket::Socket() noexcept : Socket(invalid_fd) {}

Socket::Socket(Socket&& other) noexcept : _fd(other._fd) {
    other._fd = invalid_fd;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        this->~Socket();
        new (this) Socket(std::move(other));
    }
    return *this;
}

Socket::~Socket() {
    if (_fd != invalid_fd) {
        panic("Socket not closed");
    }
}

Future<> Socket::close() {
    if (_fd == invalid_fd) {
        co_await std::make_exception_ptr(std::system_error(EBADF, std::system_category(), "Socket already closed"));
    }
    auto ret = co_await IoEngine::instance().close(_fd);
    _fd = invalid_fd;
    if (ret < 0) {
        co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "close failed"));
    }
}

Client::Client() noexcept = default;
Client::Client(Socket&& sock) : _socket(std::move(sock)) {}
Client::Client(Client&& other) noexcept= default;
Client& Client::operator=(Client&& other) noexcept = default;
Client::~Client() = default;

Future<uint64_t> Client::read(std::span<char> data) {
    auto result = co_await IoEngine::instance().recv(_socket.fd(), data, 0);
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "recv failed"));
    }
    co_return static_cast<uint64_t>(result);
}

Future<uint64_t> Client::write(std::span<const char> data) {
    auto result = co_await IoEngine::instance().send(_socket.fd(), data, 0);
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "send failed"));
    }
    co_return static_cast<uint64_t>(result);
} 

Future<> Client::close() {
    return _socket.close();
}

Server::Server() noexcept = default;
Server::Server(Socket&& socket) noexcept : _socket(std::move(socket)) {}
Server::Server(Server&&) noexcept = default;
Server& Server::operator=(Server&&) noexcept = default;
Server::~Server() {
    if (_socket.fd() != invalid_fd) {
        panic("Server not closed");
    }
}

Future<Client> Server::accept() {
    return Socket::make_accept(_socket);
}

Future<> Server::close() {
    return _socket.close();
}

} // namespace corey
