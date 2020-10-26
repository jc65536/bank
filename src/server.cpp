#include "account.h"
#include "asio.hpp"
#include "database.h"
#include "request.h"
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
public:
    typedef std::shared_ptr<tcp_connection> pointer;

    static pointer create(asio::io_context &io_context, database &db) {
        return pointer(new tcp_connection(io_context, db));
    }

    tcp::socket &socket() {
        return socket_;
    }

    void start() {
        read_header();
    }

private:
    tcp_connection(asio::io_context &io_context, database &db) : socket_(io_context), db(db) {
    }

    void read_header() {
        asio::async_read(socket_, asio::buffer(&req.header, sizeof(request_header)), std::bind(read_body, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void read_body(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            asio::async_read(socket_, asio::buffer(req.body, req.header.body_size), std::bind(handle_request, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        }
    }

    void handle_request(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            // handle request
            std::cout << "Received: " << req.body << std::endl;
            std::stringstream body;
            body << req.body;
            switch (req.header.type) {
            case request_type::register_account: {
                std::string account_name;
                unsigned long long pw_hash;
                body >> account_name >> pw_hash;
                current_account = db.register_account(account_name, pw_hash);
                new_request(request_type::response, current_account ? "0" : "1").send(socket_);
                break;
            }
            case request_type::login: {
                std::string account_name;
                unsigned long long pw_hash;
                body >> account_name >> pw_hash;
                current_account = db.get_account(account_name, pw_hash);
                new_request(request_type::response, current_account ? "0" : "1").send(socket_);
                break;
            }
            case request_type::logout: {
                if (current_account) {
                    db.commit_updates(current_account);
                    current_account.reset();
                }
                break;
            }
            case request_type::get_balance: {
                if (current_account) {
                    new_request(request_type::response, std::to_string(current_account->balance)).send(socket_);
                }
                break;
            }
            case request_type::deposit: {
                if (current_account) {
                    unsigned long long amount;
                    body >> amount;
                    current_account->balance += amount;
                    new_request(request_type::response, std::to_string(current_account->balance)).send(socket_);
                }
                break;
            }
            case request_type::withdraw: {
                if (current_account) {
                    unsigned long long amount;
                    body >> amount;
                    if (current_account->balance >= amount) {
                        current_account->balance -= amount;
                        new_request(request_type::response, std::to_string(current_account->balance)).send(socket_);
                    } else {
                        new_request(request_type::response, "e1").send(socket_);
                    }
                }
                break;
            }
            case request_type::transfer: {
                if (current_account) {
                    std::string account_name;
                    unsigned long long amount;
                    body >> account_name >> amount;
                    if (current_account->balance >= amount) {
                        int status = db.transfer(account_name, amount);
                        if (status == 0) {
                            current_account->balance -= amount;
                            new_request(request_type::response, std::to_string(current_account->balance)).send(socket_);
                        } else {
                            new_request(request_type::response, "e" + std::to_string(status)).send(socket_);
                        }
                    } else {
                        new_request(request_type::response, "e1").send(socket_);
                    }
                }
                break;
            }
            case request_type::change_password: {
                if (current_account) {
                    unsigned long long old_pw;
                    unsigned long long new_pw;
                    body >> old_pw >> new_pw;
                    if (old_pw == current_account->pw_hash) {
                        current_account->pw_hash = new_pw;
                        new_request(request_type::response, "0").send(socket_);
                    } else {
                        new_request(request_type::response, "1").send(socket_);
                    }
                }
            }
            }

            read_header();
        }
    }

    void close() {
        std::cout << "Client disconnected." << std::endl;
        socket_.close();
        socket_.release();
        if (current_account) {
            db.commit_updates(current_account);
            current_account.reset();
        }
    }

    tcp::socket socket_;
    database &db;
    request req;
    account::pointer current_account;
};

class tcp_server {
public:
    tcp_server(asio::io_context &io_context, int port) : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        tcp_connection::pointer new_connection = tcp_connection::create(io_context_, db);
        acceptor_.async_accept(new_connection->socket(), std::bind(tcp_server::handle_accept, this, new_connection, std::placeholders::_1));
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
            port = atoi(argv[1]); // Careful!
        }
        asio::io_context io_context;
        tcp_server server(io_context, port);
        io_context.run();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}