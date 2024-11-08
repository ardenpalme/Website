#include<string>

#include<string.h>
#include<stdio.h>
#include<unistd.h>

#include "util.hpp"

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".css"))
        strcpy(filetype, "text/css");
    else if (strstr(filename, ".js"))
        strcpy(filetype, "application/javascript");
    else if (strstr(filename, ".svg"))
        strcpy(filetype, "image/svg+xml");
    else
        strcpy(filetype, "text/plain");
}

vector<string> splitline(string line, char delim) {
    size_t idx = 0, start_idx = 0;
    vector<string> vec;
    string scan_str;

    while(idx != string::npos) {
        idx = line.find(delim, start_idx);
        if(idx == string::npos) {
            scan_str = line.substr(start_idx);
        }else{
            scan_str = line.substr(start_idx, (idx - start_idx));
        }
        vec.push_back(scan_str);
        start_idx = idx + 1;
    }
    return vec;
}

