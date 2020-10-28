/*
Class database represents an interface for the server to acceess the data file (accounts.db).

You wouldn't want different client connections all accessing (or worse, writing) to the file
at the same time, so this class uses a mutex to ensure all operations stay exclusive.

In server.cpp, class server has the only instance of a database, whose reference is passed to
all connections (i.e. all connections use the same database).
*/

#ifndef DATABASE_H
#define DATABASE_H

#include "account.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <vector>

#define DB_FILE "accounts.db"

class database {
public:
    database() {
        std::string name;
        unsigned long long pw_hash;
        unsigned long long balance;
        std::ifstream in(DB_FILE);
        if (in.fail()) {
            // create the file if it doesn't already exist
            std::ofstream file(DB_FILE);
            file.close();
        } else {
            while (in >> name) {
                in >> pw_hash >> balance;
                accounts.push_back(account::create(name, pw_hash, balance));
            }
        }
        in.close();
    }

    account::pointer register_account(std::string name, unsigned long long pw_hash) {
        const std::lock_guard<std::mutex> lock(mutex);
        for (account::pointer a : accounts)
            if (a->name == name)
                return account::pointer(); // failed to create account due to name conflict
        account::pointer new_account = account::create(name, pw_hash, 0);
        accounts.push_back(new_account);
        std::ofstream out(DB_FILE, std::ofstream::app);
        out << std::setw(NAME_WIDTH) << name << std::setw(PW_HASH_WIDTH) << pw_hash << std::setw(BALANCE_WIDTH) << 0 << std::endl;
        out.close();
        return new_account;
    }

    // We don't actually commit any account updates to the data file until the user logs out
    int commit_updates(account::pointer updated) {
        const std::lock_guard<std::mutex> lock(mutex);
        auto i = std::find(accounts.begin(), accounts.end(), updated);
        if (i == accounts.end())
            return 1; // could not find account
        std::fstream file(DB_FILE, std::fstream::in | std::fstream::out | std::fstream::binary);
        std::string garbage;
        for (auto j = accounts.begin(); j < i; j++) {
            std::getline(file, garbage);
        }
        std::string name;
        if (file >> name && name == updated->name) {
            file << std::setw(PW_HASH_WIDTH) << updated->pw_hash << std::setw(BALANCE_WIDTH) << updated->balance;
            file.close();
            return 0;
        }
        file.close();
        return 2; // error editing account info
    }

    account::pointer get_account(std::string name, unsigned long long pw_hash) {
        const std::lock_guard<std::mutex> lock(mutex);
        for (account::pointer a : accounts) {
            if (a->name == name && a->pw_hash == pw_hash) {
                return a;
            }
        }
        return account::pointer(); // could not find account; return an empty pointer instead
    }

    int transfer(std::string dest_account, unsigned long long amount) {
        const std::lock_guard<std::mutex> lock(mutex);
        auto i = std::find_if(accounts.begin(), accounts.end(), [&](account::pointer a) {
            return a->name == dest_account;
        });
        if (i == accounts.end())
            return 2; // could not find account
        if ((*i).use_count() > 1) // another tcp_connection has a copy of the account pointer, i.e. the user is logged in
            return 3; // can't transfer while someone else is using the account
        (*i)->balance += amount;
        std::fstream file(DB_FILE, std::fstream::in | std::fstream::out | std::fstream::binary);
        std::string garbage;
        for (auto j = accounts.begin(); j < i; j++) {
            std::getline(file, garbage);
        }
        std::string name;
        unsigned long long pw_hash;
        if (file >> name && name == dest_account && file >> pw_hash) {
            file << std::setw(BALANCE_WIDTH) << (*i)->balance;
            file.close();
            return 0;
        }
        file.close();
        return 4; // error editing account info
    }

    std::string get_quote(unsigned long long parameters[]) {
        const std::lock_guard<std::mutex> lock(mutex);
        int seed = (int) parameters[0];
        std::string filename = *((std::string *) parameters[1]);
        std::vector<std::string> quotes;
        std::ifstream in(filename);
        std::string line;
        while (std::getline(in, line))
            quotes.push_back(line);
        in.close();
        srand(seed);
        int r = rand() % quotes.size(); // note: quotes.txt better not be empty!
        return quotes[r];
    }

private:
    std::vector<account::pointer> accounts;
    std::mutex mutex;
};

#endif // DATABASE_H