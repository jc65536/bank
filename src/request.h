#ifndef REQUEST_H
#define REQUEST_H

#include "asio.hpp"
#include <string>

#define MAX_BODY_LEN 256

enum class request_type {
    response,
    register_account,
    login,
    logout,
    get_balance,
    deposit,
    withdraw,
    transfer,
    change_password
};

struct request_header {
    request_type type; // 4 bytes
    int body_size;     // 4 bytes
};

struct request {
    request_header header;
    char body[MAX_BODY_LEN];
    void send(asio::ip::tcp::socket &socket) {
        asio::write(socket, asio::buffer(this, sizeof(request_header) + header.body_size));
    }
};

request new_request(request_type type, std::string body) {
    request req;
    req.header.type = type;
    if (body.length() > MAX_BODY_LEN - 1)
        body = body.substr(0, MAX_BODY_LEN - 1);
    req.header.body_size = body.length() + 1;
    strcpy(req.body, body.c_str());
    return req;
}

#endif // REQUEST_H