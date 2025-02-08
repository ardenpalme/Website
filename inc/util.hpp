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

template<typename T> 
class Cache {
public:
    T get_cached_page(string filename);

    void set_cached_page(pair<string, T> page_data);

private:
    map<string, T> cached_pages;
    shared_mutex mtx;
};

void get_filetype(char *filename, char *filetype);

vector<string> splitline(string line, char delim);

pair<char *, size_t> deflate_file(string filename, int compression_level);

pair<char*, size_t> deflate_object(char *obj, size_t obj_size, int compression_level);

template<typename T>
T Cache<T>::get_cached_page(string filename) {
    std::shared_lock lck {mtx};
    auto iter = cached_pages.find(filename);

    if(iter == cached_pages.end()) return T();
    else return iter->second;
}

template<typename T>
void Cache<T>::set_cached_page(pair<string, T> page_data) {
    std::unique_lock lck {mtx};
    cached_pages.insert(page_data);
}

#endif /* __UTIL_HPP__ */