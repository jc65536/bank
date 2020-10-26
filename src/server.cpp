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
        asio::async_read(socket_, asio::buffer(&request.header, sizeof(request_header)), std::bind(read_body, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void read_body(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            asio::async_read(socket_, asio::buffer(request.body, request.header.body_size), std::bind(handle_request, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        }
    }

    void handle_request(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            // handle request
            std::cout << "Received: " << request.body << std::endl;
            std::stringstream body;
            body << request.body;
            switch (request.header.type) {
            case request_type::register_account: {
                std::string account_name;
                unsigned long long pw_hash;
                body >> account_name >> pw_hash;
                account = db.register_account(account_name, pw_hash);
                new_request(request_type::response, account ? "0" : "1").send(socket_);
                break;
            }
            case request_type::login: {
                std::string account_name;
                unsigned long long pw_hash;
                body >> account_name >> pw_hash;
                account = db.get_account(account_name, pw_hash);
                new_request(request_type::response, account ? "0" : "1").send(socket_);
                break;
            }
            case request_type::logout: {
                if (account) {
                    db.commit_updates(account);
                    account.reset();
                }
                break;
            }
            case request_type::get_balance: {
                if (account) {
                    new_request(request_type::response, std::to_string(account->balance)).send(socket_);
                }
                break;
            }
            case request_type::deposit: {
                if (account) {
                    unsigned long long amount;
                    body >> amount;
                    account->balance += amount;
                    new_request(request_type::response, std::to_string(account->balance)).send(socket_);
                }
                break;
            }
            case request_type::withdraw: {
                if (account) {
                    unsigned long long amount;
                    body >> amount;
                    if (account->balance >= amount) {
                        account->balance -= amount;
                        new_request(request_type::response, std::to_string(account->balance)).send(socket_);
                    } else {
                        new_request(request_type::response, "e1").send(socket_);
                    }
                }
                break;
            }
            case request_type::transfer: {
                if (account) {
                    std::string account_name;
                    unsigned long long amount;
                    body >> account_name >> amount;
                    if (account->balance >= amount) {
                        int status = db.transfer(account_name, amount);
                        if (status == 0) {
                            account->balance -= amount;
                            new_request(request_type::response, std::to_string(account->balance)).send(socket_);
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
                if (account) {
                    unsigned long long old_pw;
                    unsigned long long new_pw;
                    body >> old_pw >> new_pw;
                    if (old_pw == account->pw_hash) {
                        account->pw_hash = new_pw;
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
        if (account) {
            db.commit_updates(account);
            account.reset();
        }
    }

    tcp::socket socket_;
    database &db;
    request request;
    account::pointer account;
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