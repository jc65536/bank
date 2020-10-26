#ifndef DATABASE_H
#define DATABASE_H

#include "account.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <vector>

#define FILE_NAME "accounts.data"

class database {
public:
    database() {
        std::string account_name;
        unsigned long long pw_hash;
        unsigned long long balance;
        std::ifstream in(FILE_NAME);
        while (in >> account_name) {
            in >> pw_hash >> balance;
            accounts.push_back(account::create(account_name, pw_hash, balance));
        }
        in.close();
    }

    account::pointer register_account(std::string account_name, unsigned long long pw_hash) {
        const std::lock_guard<std::mutex> lock(mutex);
        for (account::pointer a : accounts)
            if (a->account_name == account_name)
                return account::pointer(); // failed to create account due to name conflict
        account::pointer new_account = account::create(account_name, pw_hash);
        accounts.push_back(new_account);
        std::ofstream out(FILE_NAME, std::ofstream::app);
        out << std::setw(30) << account_name << std::setw(21) << pw_hash << std::setw(21) << 0 << std::endl;
        out.close();
        return new_account;
    }

    int commit_updates(account::pointer updated) {
        const std::lock_guard<std::mutex> lock(mutex);
        auto i = std::find(accounts.begin(), accounts.end(), updated);
        if (i == accounts.end())
            return 1; // could not find account
        std::fstream file(FILE_NAME, std::fstream::in | std::fstream::out | std::fstream::binary);
        for (auto j = accounts.begin(); j < i; j++) {
            file.ignore(999, '\n');
        }
        std::string account_name;
        if (file >> account_name && account_name == updated->account_name) {
            file << std::setw(21) << updated->pw_hash << std::setw(21) << updated->balance;
            file.close();
            return 0;
        }
        file.close();
        return 2; // error editing account info
    }

    account::pointer get_account(std::string account_name, unsigned long long pw_hash) {
        const std::lock_guard<std::mutex> lock(mutex);
        for (account::pointer a : accounts) {
            if (a->account_name == account_name && a->pw_hash == pw_hash) {
                return a;
            }
        }
        return account::pointer(); // could not find account; returning an empty pointer instead
    }

    int transfer(std::string account_name, unsigned long long amount) {
        const std::lock_guard<std::mutex> lock(mutex);
        auto i = std::find_if(accounts.begin(), accounts.end(), [&](account::pointer a) {
            return a->account_name == account_name;
        });
        if (i == accounts.end())
            return 1; // could not find account
        if ((*i).use_count() > 1)
            return 2; // can't transfer while someone else is using the account
        (*i)->balance += amount;
        std::fstream file(FILE_NAME, std::fstream::in | std::fstream::out | std::fstream::binary);
        for (auto j = accounts.begin(); j < i; j++) {
            file.ignore(999, '\n');
        }
        std::string read_account_name;
        unsigned long long pw_hash;
        if (file >> read_account_name && read_account_name == account_name && file >> pw_hash) {
            file << std::setw(21) << (*i)->balance;
            file.close();
            return 0;
        }
        file.close();
        return 3; // error editing account info
    }

    int delete_account(account *account) {
        // to do
        return 0;
    }

private:
    std::vector<account::pointer> accounts;
    std::mutex mutex;
};

#endif // DATABASE_H