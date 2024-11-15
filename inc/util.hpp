#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <shared_mutex>
#include <tuple>
#include <mutex>

#include <zlib.h>

using namespace std;

class Cache {
public:
    Cache() {}
    pair<char*, size_t> get_cached_page(string filename) {
        std::shared_lock lck {mtx};
        auto iter = cached_pages.find(filename);

        if(iter == cached_pages.end()) return {NULL, 0};
        else return iter->second;
    }

    void set_cached_page(pair<string, pair<char*,size_t>> page_data) {
        std::unique_lock lck {mtx};
        cached_pages.insert(page_data);
    }
private:
    map<string, pair<char*, size_t>> cached_pages;
    shared_mutex mtx;
};

void get_filetype(char *filename, char *filetype);

vector<string> splitline(string line, char delim);

pair<char *, size_t> deflate_file(string filename, int compression_level);

#endif /* __UTIL_HPP__ */