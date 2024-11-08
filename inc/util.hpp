#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <string>
#include <vector>

using namespace std;

void get_filetype(char *filename, char *filetype);

vector<string> splitline(string line, char delim);

#endif /* __UTIL_HPP__ */