#include "account.h"
#include "asio.hpp"
#include "database.h"
#include "request.h"
#include "tcp_connection.h"
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using asio::ip::tcp;

// Like tcp_connection, tcp_server accepts incoming connections asynchronously
class tcp_server {
public:
    tcp_server(asio::io_context &io_context, int port) : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        tcp_connection::pointer new_connection = tcp_connection::create(io_context_, db);
        acceptor_.async_accept(new_connection->socket(), std::bind(&tcp_server::handle_accept, this, new_connection, std::placeholders::_1));
    }

    void handle_accept(tcp_connection::pointer new_connection, const std::error_code &error) {
        if (!error)
            new_connection->start();
        start_accept();
    }

    asio::io_context &io_context_;
    tcp::acceptor acceptor_;
    database db;
};

int main(int argc, char **argv) {
    try {
        int port = 4567;
        if (argc > 1) {
            port = atoi(argv[1]); // Careful! No safeguards here
        }
        asio::io_context io_context;
        tcp_server server(io_context, port);
        io_context.run();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
