/*
Simply a data object representing bank accounts.
*/

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <memory>
#include <string>

// formatting widths for when writing out to data file
#define NAME_WIDTH 30
#define PW_HASH_WIDTH 21
#define BALANCE_WIDTH 21

class account {
public:
    typedef std::shared_ptr<account> pointer;

    std::string name;
    unsigned long long pw_hash; // hash of the user's password
    unsigned long long balance;

    account() {
    }

    account(std::string name, unsigned long long pw_hash, unsigned long long balance) {
        if (name.length() > 30)
            this->name = name.substr(0, 30);
        else
            this->name = name;
        this->pw_hash = pw_hash;
        this->balance = balance;
    }

    // In the server, we need to use smart shared pointers instead of dangerous raw pointers
    static pointer create(std::string name, unsigned long long pw_hash, unsigned long long balance) {
        return pointer(new account(name, pw_hash, balance));
    }
};

#endif // ACCOUNT_H