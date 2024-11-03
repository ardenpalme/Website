#include <iostream>
#include <fstream>
#include "net_util.hpp"

using namespace std;

ostream &operator<<(ostream &os, Client &cli) {
    for (auto ele : cli.requests) {
        os << ele << " ";
    }
    return os;
}