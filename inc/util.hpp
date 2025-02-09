#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <utility>

using namespace std;

string humanReadableSize(size_t bytes);

vector<string> splitline(string line, char delim);

pair<char *, size_t> deflate_file(string filename, int compression_level);

/* CSAPP Functions - BEGIN */

void get_filetype(char *filename, char *filetype);

/* CSAPP Functions - END */