#include "account.h"
#include "request.h"
#include <asio.hpp>
#include <fstream>
#include <iostream>

using asio::ip::tcp;

int term_menu(std::string prompt, int argc, std::string argv[]) {
    std::cout << prompt << std::endl;
    for (int i = 0; i < argc; i++) {
        std::cout << "[" << i + 1 << "] " << argv[i] << std::endl;
    }
    std::cout << "> ";
    int n;
    while (!(std::cin >> n) || n <= 0 || n > argc) {
        std::cin.clear();
        std::string str;
        std::getline(std::cin, str);
        std::cout << "Please enter a valid option number." << std::endl
                  << "> ";
    }
    std::string str;
    std::getline(std::cin, str);
    return n;
}

std::string yes_no[] = {"Yes", "No"};
std::string login_options[] = {"Log in", "Register", "Quit"};
int login_options_length = 3;
std::string main_menu_options[] = {"Deposit", "Withdraw", "Transfer", "Change password", "Logout"};
int main_menu_length = 5;

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
    exit
};

int main() {
    try {
        asio::io_context io_context;

        tcp::resolver resolver(io_context);

        // 127.0.0.1
        std::string host = "206.189.165.91", port = "4567";
        std::ifstream hostfile("host.ini");
        if (!hostfile.fail()) {
            hostfile >> host;
            hostfile >> port;
        }
        hostfile.close();
        tcp::resolver::results_type endpoints = resolver.resolve(host, port);

        tcp::socket socket(io_context);

        state current_state;
        request response;
        account user_account;

        try {
            asio::connect(socket, endpoints);
            current_state = state::entrance;
        } catch (std::system_error &e) {
            current_state = state::connection_failed;
        }

        while (current_state != state::exit) {
            switch (current_state) {
            case state::connection_failed: {
                int reconnect = term_menu("Connection to server failed. Try another host?", 2, yes_no);
                if (reconnect == 1) {
                    std::string host;
                    std::string port;
                    std::string buf;
                    std::cout << "Host: ";
                    std::cin >> host;
                    std::getline(std::cin, buf);
                    std::cout << "Port: ";
                    std::cin >> port;
                    std::getline(std::cin, buf);
                    endpoints = resolver.resolve(host, port);
                    try {
                        asio::connect(socket, endpoints);
                        std::ofstream hostwriter("host.ini", std::ofstream::trunc);
                        hostwriter << host << " " << port;
                        hostwriter.close();
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
                std::string account_name;
                std::string password;
                std::string buf;
                std::cout << "Account name: ";
                std::cin >> account_name;
                std::getline(std::cin, buf);
                std::cout << "Password: ";
                std::cin >> password;
                std::getline(std::cin, buf);
                unsigned long long pw_hash = std::hash<std::string>()(password);
                new_request(request_type::login, account_name + " " + std::to_string(pw_hash)).send(socket);
                asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                asio::read(socket, asio::buffer(response.body, response.header.body_size));
                if (!atoi(response.body)) {
                    user_account.account_name = account_name;
                    user_account.pw_hash = pw_hash;
                    new_request(request_type::get_balance, "").send(socket);
                    asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                    asio::read(socket, asio::buffer(response.body, response.header.body_size));
                    user_account.balance = strtoull(response.body, nullptr, 10);
                    current_state = state::main_menu;
                } else {
                    std::cout << "Invalid account name or password." << std::endl;
                    current_state = state::entrance;
                }
                break;
            }
            case state::registration: {
                std::string account_name;
                std::string password;
                std::string buf;
                std::cout << "New account name: ";
                std::cin >> account_name;
                std::getline(std::cin, buf);
                std::cout << "Password: ";
                std::cin >> password;
                std::getline(std::cin, buf);
                unsigned long long pw_hash = std::hash<std::string>()(password);
                new_request(request_type::register_account, account_name + " " + std::to_string(pw_hash)).send(socket);
                asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                asio::read(socket, asio::buffer(response.body, response.header.body_size));
                if (!atoi(response.body)) {
                    user_account.account_name = account_name;
                    user_account.pw_hash = pw_hash;
                    new_request(request_type::get_balance, "").send(socket);
                    asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                    asio::read(socket, asio::buffer(response.body, response.header.body_size));
                    user_account.balance = strtoull(response.body, nullptr, 10);
                    current_state = state::main_menu;
                } else {
                    std::cout << "The account name has already been taken." << std::endl;
                    current_state = state::entrance;
                }
                break;
            }
            case state::main_menu: {
                std::cout << "Hello " << user_account.account_name << "." << std::endl;
                std::cout << "Your balance is currently $" << (user_account.balance / 100) << ".";
                if (user_account.balance % 100 < 10) {
                    std::cout << "0";
                }
                std::cout << user_account.balance % 100 << std::endl;
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
                    new_request(request_type::logout, "").send(socket);
                    user_account = account();
                    current_state = state::entrance;
                    break;
                }
                break;
            }
            case state::deposit: {
                double amount;
                std::string buf;
                std::cout << "Deposit amount: $";
                if (std::cin >> amount && amount > 0) {
                    unsigned long long int_amount = (unsigned long long)(amount * 100);
                    new_request(request_type::deposit, std::to_string(int_amount)).send(socket);
                    asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                    asio::read(socket, asio::buffer(response.body, response.header.body_size));
                    user_account.balance = strtoull(response.body, nullptr, 10);
                } else {
                    std::cin.clear();
                    std::cout << "Please enter a positive number." << std::endl;
                }
                std::getline(std::cin, buf);
                current_state = state::main_menu;
                break;
            }
            case state::withdraw: {
                double amount;
                std::string buf;
                std::cout << "Withdraw amount: $";
                if (std::cin >> amount) {
                    unsigned long long int_amount = (unsigned long long)(amount * 100);
                    if (0 < int_amount && int_amount <= user_account.balance) {
                        new_request(request_type::withdraw, std::to_string(int_amount)).send(socket);
                        asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                        asio::read(socket, asio::buffer(response.body, response.header.body_size));
                        user_account.balance = strtoull(response.body, nullptr, 10);
                    } else {
                        std::cout << "You can't withdraw that much!" << std::endl;
                    }
                } else {
                    std::cin.clear();
                    std::cout << "Please enter a number." << std::endl;
                }
                std::getline(std::cin, buf);
                current_state = state::main_menu;
                break;
            }
            case state::transfer: {
                std::string account_name;
                double amount;
                std::string buf;
                std::cout << "Destination account name: ";
                std::cin >> account_name;
                std::getline(std::cin, buf);
                std::cout << "Transfer amount: $";
                if (std::cin >> amount) {
                    unsigned long long int_amount = (unsigned long long)(amount * 100);
                    if (0 < int_amount && int_amount <= user_account.balance) {
                        new_request(request_type::transfer, account_name + " " + std::to_string(int_amount)).send(socket);
                        asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                        asio::read(socket, asio::buffer(response.body, response.header.body_size));
                        std::string str_response(response.body);
                        if (str_response == "e2") {
                            std::cout << "The account you are trying to transfer to is currently being used. You can't transfer money to them right now." << std::endl;
                        } else {
                            unsigned long long int_response = strtoull(response.body, nullptr, 10);
                            user_account.balance = int_response;
                        }
                    } else {
                        std::cout << "You can't transfer that much!" << std::endl;
                    }
                } else {
                    std::cin.clear();
                    std::cout << "Please enter a number." << std::endl;
                }
                std::getline(std::cin, buf);
                current_state = state::main_menu;
                break;
            }
            case state::change_password: {
                std::string old_password;
                std::string new_password;
                std::string buf;
                std::cout << "Current password: ";
                std::cin >> old_password;
                std::getline(std::cin, buf);
                std::cout << "New password: ";
                std::cin >> new_password;
                std::getline(std::cin, buf);
                unsigned long long old_pw_hash = std::hash<std::string>()(new_password);
                if (old_pw_hash == user_account.pw_hash) {
                    unsigned long long new_pw_hash = std::hash<std::string>()(new_password);
                    new_request(request_type::login, std::to_string(old_pw_hash) + " " + std::to_string(new_pw_hash)).send(socket);
                    asio::read(socket, asio::buffer(&response.header, sizeof(request_header)));
                    asio::read(socket, asio::buffer(response.body, response.header.body_size));
                    if (!atoi(response.body)) {
                        user_account.pw_hash = new_pw_hash;
                    } else {
                        std::cout << "Failed to change password. Try again later." << std::endl;
                    }
                } else {
                    std::cout << "Old password incorrect." << std::endl;
                }
                current_state = state::main_menu;
                break;
            }
            }
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cout << "Connection with server failed." << std::endl;
    }
}