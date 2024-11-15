#include<string>
#include<iostream>

#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<assert.h>

#include "util.hpp"

#define CHUNK_SZ 16384

using namespace std;


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

pair<char *, size_t> deflate_file(string filename, int compression_level) {
    int ret, flush, file_len, num_chunks;
    char *compressed_bytes;
    unsigned char in_buf[CHUNK_SZ];
    unsigned char out_buf[CHUNK_SZ];
    z_stream strm;
    vector<unsigned char> vec;
    ifstream ifs;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, compression_level);
    if (ret != Z_OK) {
        cerr << "Error initializing lzip deflate" << endl;
        return {NULL, 0};
    }

    ifs.open (filename, ifstream::binary);
    if (!ifs.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return {nullptr, 0};
    }

    ifs.seekg (0, ifs.end);
    file_len = ifs.tellg();
    ifs.seekg (0, ifs.beg);

    while(1) {
        ifs.read((char*)in_buf, CHUNK_SZ);
        size_t bytes_read = ifs.gcount();

        if (bytes_read == 0) break;

        strm.avail_in = bytes_read;
        strm.next_in = in_buf;

        flush = ifs.eof() ? Z_FINISH : Z_NO_FLUSH;

        do {
            strm.next_out = out_buf;
            strm.avail_out = CHUNK_SZ;

            if((ret=deflate(&strm, flush)) == Z_STREAM_ERROR) {
                cerr << "error: compressing " << CHUNK_SZ << " bytes from " << filename << endl;
                ifs.close();
                deflateEnd(&strm);
                return {NULL, 0};
            }

            vec.insert(vec.end(), out_buf, out_buf + (CHUNK_SZ - strm.avail_out));
        } while(strm.avail_out == 0);

        if (flush == Z_FINISH) break;
    }

    ifs.close();
    deflateEnd(&strm);

    compressed_bytes = new char[vec.size()];
    memcpy(compressed_bytes, vec.data(), vec.size());
    return {compressed_bytes, vec.size()};
}