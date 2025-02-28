#pragma once

#include <string>
#include <map>
#include <shared_mutex>
#include <mutex>
#include <sstream>
#include <iostream>
#include <ctime>
#include <utility>

#include "util.hpp"

using namespace std;

// For print formatting in BASH
const static string RED_COLOR = "\033[31m";
const static string RESET_COLOR = "\033[0m";

template<typename T> 
class Cache {
public:
    T get_cached_page(string filename);

    void set_cached_page(pair<string, T> page_data);

    void check_updates(void);
    void print(bool verbose=false);

private:
    map<string, T> cached_pages;
    shared_mutex mtx;
    size_t num_cache_entries;

};


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
void Cache<T>::print(bool verbose)
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

    if(verbose) cout << oss.str() << endl;
}

template<typename T>
void Cache<T>::check_updates(void)
{
    bool cache_grew = false;
    {
        std::shared_lock lck {mtx};
        if(cached_pages.size() > num_cache_entries) cache_grew = true;
    }

    if(cache_grew) {
        //this->print();
        num_cache_entries = cached_pages.size();
    }
}