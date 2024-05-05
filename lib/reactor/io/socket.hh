#pragma once

#include "io.hh"
#include "reactor/future.hh"

#include <cstdint>

namespace corey {

class Client;
class Server;

class Socket {
public:

    static Future<Server> make_tcp_listener(uint16_t port);
    static Future<Client> make_tcp_connect(const char* host, uint16_t port);
    static Future<Client> make_accept(Socket& accepter);

    Socket() noexcept;
    Socket(const Socket& other) = delete;
    Socket& operator=(const Socket& other) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    ~Socket();

    Future<> close();

    int fd() const { return _fd; }

private:
    Socket(int fd) : _fd(fd) {}    
    int _fd;
};

class Client {
    friend class ServerSocket;
public:

    Client() noexcept;
    explicit Client(Socket&&);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    ~Client();

    Future<uint64_t> read(std::span<char>);
    Future<uint64_t> write(std::span<const char>);
    Future<> close();

    const Socket& socket() const { return _socket; }

private:
    Socket _socket;
};

class Server {
public:

    Server() noexcept;
    explicit Server(Socket&&) noexcept;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;
    ~Server();

    Future<Client> accept();
    Future<> close();

    const Socket& socket() const { return _socket; }

private:
    Socket _socket;
};

} // namespace corey