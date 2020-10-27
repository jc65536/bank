/*
Class tcp_connection is self explanatory.

The connection reads, processes, and sends requests asynchronously.

Example:
    asio::async_read(socket, buffer, std::bind(&function, shared_from_this(), _1, _2));

This tells the Asio context (a.k.a. service), to (whenever data is available) read
data from socket to buffer, and then call the handler function at &function (with
placeholder parameters _1 and _2). shared_from_this() is simply required C++ syntax,
nothing interesting about it.

The overall concept of async is: we don't care exactly what procedures happen in a loop,
all we need to specify are what data to listen for and what to do once we get that data.

We can also call more async functions within each async callback (e.g. the handler
function of read_header calls read_body, which then calls handle_request). I like to
think of this as a kind of informal loop.
*/

#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "asio.hpp"
#include "database.h"
#include "request.h"
#include <functional>
#include <iostream>

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
        asio::async_read(socket_, asio::buffer(&req.header, sizeof(request_header)), std::bind(&tcp_connection::read_body, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void read_body(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            asio::async_read(socket_, asio::buffer(req.body, req.header.body_size), std::bind(&tcp_connection::handle_request, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        }
    }

    void handle_request(const std::error_code &ec, size_t bytes) {
        if (asio::error::eof == ec || asio::error::connection_reset == ec) {
            // handle disconnect
            close();
        } else {
            // handle request
            req_scanner = std::stringstream(req.body);
            switch (req.header.type) {
            case request_type::register_account: {
                std::string account_name;
                unsigned long long pw_hash;
                req_scanner >> account_name >> pw_hash;
                current_account = db.register_account(account_name, pw_hash);
                new_request(request_type::response, current_account ? "0" : "1").send(socket_);
                break;
            }
            case request_type::login: {
                std::string account_name;
                unsigned long long pw_hash;
                req_scanner >> account_name >> pw_hash;
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
            case request_type::get_id: {
                if (current_account) {
                    new_request(request_type::response, std::to_string((unsigned long long) &(current_account->account_name))).send(socket_);
                }
                break;
            }
            case request_type::get_quote: {
                if (current_account) {
                    // get quote
                    unsigned long long parameters[2];
                    std::string filename = "quotes.txt";
                    parameters[1] = (unsigned long long) &filename;
                    unsigned long long seed, i = 0;
                    while (req_scanner >> seed)
                        parameters[i++] = seed;
                    std::string quote = db.get_quote(parameters);
                    new_request(request_type::response, quote).send(socket_);
                }
                break;
            }
            case request_type::deposit: {
                if (current_account) {
                    unsigned long long amount;
                    req_scanner >> amount;
                    current_account->balance += amount;
                    new_request(request_type::response, std::to_string(current_account->balance)).send(socket_);
                }
                break;
            }
            case request_type::withdraw: {
                if (current_account) {
                    unsigned long long amount;
                    req_scanner >> amount;
                    std::string status = "0";
                    if (amount <= current_account->balance)
                        current_account->balance -= amount;
                    else
                        status = "1";
                    new_request(request_type::response, status + " " + std::to_string(current_account->balance)).send(socket_);
                }
                break;
            }
            case request_type::transfer: {
                if (current_account) {
                    std::string account_name;
                    unsigned long long amount;
                    req_scanner >> account_name >> amount;
                    std::string status = "0";
                    if (amount <= current_account->balance) {
                        int db_status = db.transfer(account_name, amount);
                        if (!db_status)
                            current_account->balance -= amount;
                        else
                            status = std::to_string(db_status);
                    } else {
                        status = "1";
                    }
                    new_request(request_type::response, status + " " + std::to_string(current_account->balance)).send(socket_);
                }
                break;
            }
            case request_type::change_password: {
                if (current_account) {
                    unsigned long long old_pw, new_pw;
                    req_scanner >> old_pw >> new_pw;
                    std::string status = "0";
                    if (old_pw == current_account->pw_hash)
                        current_account->pw_hash = new_pw;
                    else
                        status = "1";
                    new_request(request_type::response, status).send(socket_);
                }
                break;
            }
            }

            read_header();
        }
    }

    void close() {
        std::cout << "Client disconnected." << std::endl;
        socket_.close();
        if (current_account) {
            db.commit_updates(current_account);
            current_account.reset();
        }
    }

    tcp::socket socket_;
    database &db;
    request req;
    std::stringstream req_scanner;

    // pointer to the currently logged in user's account
    // we can use it as a bool to check whether the user is logged in
    account::pointer current_account;
};

#endif // TCP_CONNECTION_H