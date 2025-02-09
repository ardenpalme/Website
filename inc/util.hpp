#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <shared_mutex>
#include <sstream>
#include <ctime>
#include <iostream>
#include <tuple>
#include <mutex>

#include <zlib.h>

using namespace std;

std::string humanReadableSize(size_t bytes);

const static std::string RED_COLOR = "\033[31m";
const static std::string RESET_COLOR = "\033[0m";

template<typename T> 
class Cache {
public:
    T get_cached_page(string filename);

    void set_cached_page(pair<string, T> page_data);

    void check_updates(void);

private:
    map<string, T> cached_pages;
    shared_mutex mtx;
    size_t num_cache_entries;

    void print();
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

template<typename T>
void Cache<T>::print()
{
    int elem_count = 0;
    ostringstream oss;
    size_t total_size = 0;

    {
        std::shared_lock lck {mtx};

        for(auto &[key,value] : cached_pages) 
        {
            const time_t modified_time = static_cast<time_t>(std::get<2>(value));
            string time_str = std::ctime(&modified_time);

            oss << key << " : " 
                << humanReadableSize(get<1>(value))
                << RED_COLOR << " [" <<  time_str.substr(0, time_str.length() - 1) << "]" << RESET_COLOR << endl;
            total_size += get<1>(value);
            elem_count++;
        }
        
    }

    cout << "Cache: " << elem_count << " entries " 
        << "(" << humanReadableSize(total_size) << ")" << endl;

    cout << oss.str();
    cout << endl;
}

template<typename T>
void Cache<T>::check_updates(void)
{
    bool cache_grew = false;
    {
        std::shared_lock lck {mtx};
        if(cached_pages.size() > num_cache_entries) cache_grew = true;
    }
    if(cache_grew) this->print();
}

#endif /* __UTIL_HPP__ */