/*
This program is what users will use to interact with the bank server.
*/

#include "account.h"
#include "request.h"
#include <asio.hpp>
#include <fstream>
#include <iostream>

using asio::ip::tcp;

enum class state {
    connection_failed,
    entrance,
    login,
    registration,
    main_menu,
    deposit,
    withdraw,
    transfer,
    change_password,
    quote,
    exit
};

state current_state;
account user;
request response;
std::stringstream response_scanner; // makes it easier to parse ints and ulls

// Blocks until the data is read
void read_response(tcp::socket &socket) {
    asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
    asio::read(socket, asio::buffer(response.body, response.header.body_size));
    response_scanner = std::stringstream(response.body);
}

// Handy util function for multiple choice menus
int term_menu(std::string prompt, int argc, const std::string argv[]) {
    std::cout << prompt << std::endl;
    for (int i = 0; i < argc; i++) {
        std::cout << "[" << i + 1 << "] " << argv[i] << std::endl;
    }
    std::cout << "> ";
    int n;
    std::string garbage;
    while (!(std::cin >> n) || n <= 0 || n > argc) {
        std::cin.clear();
        std::getline(std::cin, garbage);
        std::cout << "Please enter a valid option number." << std::endl
                  << "> ";
    }
    std::string str;
    std::getline(std::cin, str);
    return n;
}

template <typename T>
T input(std::string prompt) {
    T input;
    std::string garbage;
    std::cout << prompt;
    std::cin >> input;
    if (std::cin.fail()) {
        std::cin.clear();
        std::getline(std::cin, garbage);
        throw 1;
    }
    std::getline(std::cin, garbage);
    return input;
}

const std::string yes_no[] = {"Yes", "No"};
const std::string login_options[] = {"Log in", "Register", "Quit"};
const int login_options_length = 3;
const std::string main_menu_options[] = {"Deposit", "Withdraw", "Transfer", "Change password", "Inspirational quote", "Log out"};
const int main_menu_length = 6;

int main() {
    // Asio stuff
    asio::io_context io_context;
    tcp::resolver resolver(io_context);
    tcp::socket socket(io_context);
    tcp::resolver::results_type endpoints;

    try {
        // 206.189.165.91 is my server
        std::string host = "206.189.165.91", port = "4567";

        // use the host defined in in host.ini if the file exists
        std::ifstream hostfile("host.ini");
        if (!hostfile.fail())
            hostfile >> host >> port;
        hostfile.close();
        endpoints = resolver.resolve(host, port);

        try {
            asio::connect(socket, endpoints);
            current_state = state::entrance;
        } catch (std::system_error &e) {
            current_state = state::connection_failed;
        }

        // client can be synchronous (unlike server) since we only have one thing to do here
        while (current_state != state::exit) {
            switch (current_state) {
            case state::connection_failed: {
                int reconnect = term_menu("Connection to server failed. Try another host?", 2, yes_no);
                if (reconnect == 1) {
                    host = input<std::string>("Host: ");
                    port = input<std::string>("Port: ");
                    endpoints = resolver.resolve(host, port);
                    try {
                        asio::connect(socket, endpoints);
                        std::ofstream host_save("host.ini", std::ofstream::trunc);
                        host_save << host << " " << port;
                        host_save.close();
                        current_state = state::entrance;
                    } catch (std::system_error &e) {
                        current_state = state::connection_failed;
                    }
                } else {
                    current_state = state::exit;
                }
                break;
            }
            case state::entrance:
                switch (term_menu("Welcome to CTF Bank", login_options_length, login_options)) {
                case 1:
                    current_state = state::login;
                    break;
                case 2:
                    current_state = state::registration;
                    break;
                case 3:
                    current_state = state::exit;
                    break;
                }
                break;
            case state::login: {
                std::string name = input<std::string>("Account name: ");
                std::string password = input<std::string>("Password: ");
                unsigned long long pw_hash = std::hash<std::string>()(password);
                new_request(request_type::login, name + " " + std::to_string(pw_hash)).send(socket);
                read_response(socket);
                int error;
                if (!(error = atoi(response.body))) {
                    user.name = name;
                    user.pw_hash = pw_hash;
                    new_request(request_type::get_balance, "").send(socket);
                    read_response(socket);
                    response_scanner >> user.balance;
                    current_state = state::main_menu;
                } else {
                    if (error == 1)
                        std::cout << "Invalid account name or password." << std::endl;
                    else if (error == 2)
                        std::cout << "Your account is being used somewhere else! Please take action if you think your account has been stolen." << std::endl;
                    current_state = state::entrance;
                }
                break;
            }
            case state::registration: {
                std::string name = input<std::string>("New account name: ");
                std::string password = input<std::string>("Password: ");
                unsigned long long pw_hash = std::hash<std::string>()(password);
                new_request(request_type::register_account, name + " " + std::to_string(pw_hash)).send(socket);
                read_response(socket);
                if (!atoi(response.body)) {
                    user.name = name;
                    user.pw_hash = pw_hash;
                    new_request(request_type::get_balance, "").send(socket);
                    read_response(socket);
                    response_scanner >> user.balance;
                    current_state = state::main_menu;
                } else {
                    std::cout << "That account name has already been taken." << std::endl;
                    current_state = state::entrance;
                }
                break;
            }
            case state::main_menu: {
                std::cout << "Hello " << user.name << "." << std::endl;
                new_request(request_type::get_id, "").send(socket);
                read_response(socket);
                std::cout << "ID: " << strtoull(response.body, nullptr, 10) << std::endl;
                std::cout << "Your balance is currently $" << (user.balance / 100) << ".";
                if (user.balance % 100 < 10)
                    std::cout << "0";
                std::cout << user.balance % 100 << std::endl;
                switch (term_menu("What would you like to do?", main_menu_length, main_menu_options)) {
                case 1:
                    current_state = state::deposit;
                    break;
                case 2:
                    current_state = state::withdraw;
                    break;
                case 3:
                    current_state = state::transfer;
                    break;
                case 4:
                    current_state = state::change_password;
                    break;
                case 5:
                    current_state = state::quote;
                    break;
                case 6:
                    new_request(request_type::logout, "").send(socket);
                    user = account();
                    current_state = state::entrance;
                    break;
                }
                break;
            }
            case state::deposit: {
                try {
                    double amount = input<double>("Deposit amount: $");
                    if (amount <= 0) throw 1;
                    unsigned long long int_amount = (unsigned long long) (amount * 100);
                    new_request(request_type::deposit, std::to_string(int_amount)).send(socket);
                    read_response(socket);
                    response_scanner >> user.balance;
                } catch (int e) {
                    std::cout << "Please enter a positive number." << std::endl;
                }
                current_state = state::main_menu;
                break;
            }
            case state::withdraw: {
                try {
                    double amount = input<double>("Withdraw amount: $");
                    if (amount < 0) throw 1;
                    unsigned long long int_amount = (unsigned long long) (amount * 100);
                    new_request(request_type::withdraw, std::to_string(int_amount)).send(socket);
                    read_response(socket);
                    int error;
                    response_scanner >> error >> user.balance;
                    if (error)
                        std::cout << "You can't withdraw that amount." << std::endl;
                } catch (int e) {
                    std::cout << "Please enter a positive number." << std::endl;
                }
                current_state = state::main_menu;
                break;
            }
            case state::transfer: {
                std::string name = input<std::string>("Destination account name: ");
                try {
                    double amount = input<double>("Transfer amount: $");
                    if (amount < 0) throw 1;
                    unsigned long long int_amount = (unsigned long long) (amount * 100);
                    new_request(request_type::transfer, name + " " + std::to_string(int_amount)).send(socket);
                    read_response(socket);
                    int error;
                    response_scanner >> error >> user.balance;
                    switch (error) {
                    case 1:
                        std::cout << "You can't transfer that amount." << std::endl;
                        break;
                    case 2:
                        std::cout << "That account does not exist." << std::endl;
                        break;
                    case 3:
                        std::cout << "The account you are trying to transfer to is currently being used. You can't transfer money to them right now." << std::endl;
                        break;
                    }
                } catch (int e) {
                    std::cout << "Please enter a positive number." << std::endl;
                }
                current_state = state::main_menu;
                break;
            }
            case state::change_password: {
                std::string old_password = input<std::string>("Current password: ");
                std::string new_password = input<std::string>("New password: ");
                unsigned long long old_pw_hash = std::hash<std::string>()(old_password);
                if (old_pw_hash == user.pw_hash) {
                    unsigned long long new_pw_hash = std::hash<std::string>()(new_password);
                    new_request(request_type::change_password, std::to_string(old_pw_hash) + " " + std::to_string(new_pw_hash)).send(socket);
                    read_response(socket);
                    if (!atoi(response.body))
                        user.pw_hash = new_pw_hash;
                    else
                        std::cout << "Failed to change password. Try again later." << std::endl;
                } else {
                    std::cout << "Current password incorrect." << std::endl;
                }
                current_state = state::main_menu;
                break;
            }
            case state::quote: {
                try {
                    int seed = input<int>("Enter your lucky number: ");
                    new_request(request_type::get_quote, std::to_string(seed)).send(socket);
                    read_response(socket);
                    std::cout << response.body << std::endl;
                    current_state = state::main_menu;
                } catch (int e) {
                    std::cout << "Please enter a number." << std::endl;
                }
                break;
            }
            }
            std::cout << std::endl;
        }
    } catch (std::exception &e) {
        std::cout << "Connection with server failed." << std::endl;
        std::cerr << e.what() << std::endl;
    }
}