/*
Class request is self explanatory.
*/

#ifndef REQUEST_H
#define REQUEST_H

#include "asio.hpp"
#include <string>

#define MAX_BODY_LEN 256

// Details for each request_type (excluding response) (parameters and return values) are in Requests.txt
enum class request_type {
    response,
    register_account,
    login,
    logout,
    get_balance,
    get_id,
    get_quote,
    deposit,
    withdraw,
    transfer,
    change_password
};

// Asio read functions require us to know how many bytes to read, so request objects have
// a reaquest_header member in the beginning to indicate how many bytes the body will be.
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

// Asio also requires the data we read/write be Plain Old Data (POD) types, so we can't
// have a custom constructor. Hence the need for this function.
request new_request(request_type type, std::string body) {
    request req;
    req.header.type = type;
    if (body.length() > MAX_BODY_LEN - 1)
        body = body.substr(0, MAX_BODY_LEN - 1);
    req.header.body_size = body.length() + 1; // + 1 because of C strings' ending null terminator
    strcpy(req.body, body.c_str());
    return req;
}

#endif // REQUEST_H