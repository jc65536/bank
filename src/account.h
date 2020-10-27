/*
Simply a data object representing bank accounts.
*/

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <memory>
#include <string>

class account {
public:
    typedef std::shared_ptr<account> pointer;

    std::string account_name;
    unsigned long long pw_hash; // hash of the user's password
    unsigned long long balance;

    account() {
    }

    account(std::string account_name, unsigned long long pw_hash, unsigned long long balance) {
        if (account_name.length() > 30)
            this->account_name = account_name.substr(0, 30);
        else
            this->account_name = account_name;
        this->pw_hash = pw_hash;
        this->balance = balance;
    }

    // In the server, we need to use smart shared pointers instead of dangerous raw pointers
    static pointer create(std::string account_name, unsigned long long pw_hash, unsigned long long balance) {
        return pointer(new account(account_name, pw_hash, balance));
    }
};

#endif // ACCOUNT_H