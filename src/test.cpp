#include <iostream>
#include <string>
#include <memory>
#include "request.h"

using namespace std;

struct str {
    int a;
} x{4};

void func(std::string s) {
    cout << s << endl;
}

int main(int argc, char **argv) {
    std::shared_ptr<str> u(&x);
    func("u");
    std::is_pod<request>::value;
}