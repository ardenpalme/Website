#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <tuple>
#include <format>

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#include "client.hpp"
#include "util.hpp"
#include "connection.hpp"
#include "cache.hpp"

#define MAX_FILESIZE (1024 * 1024) /* 1MB */
#define	MAXLINE	 8192  /* Max text line length */

using namespace std;

void ClientHandler::serve_client(shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache) {
    if (request_line.size() < 3) throw GenericError::CLI_REQ_LINE_INVALID;

    string method = request_line[0];
    string uri = request_line[1];

    if(method == "GET") {
        if(uri == "/") {
            uri = "index.html";
        }else{ 
            uri = uri.substr(1);
        }
        uri.insert(0, "assets/");
        serve_static_compress(uri, cache);
    } else {
        throw GenericError::CLI_HTTP_METHOD;
    }
}

void ClientHandler::serve_static_compress(string filename, shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache) {
    int ret, fd;
    time_t file_modified_time;
    struct stat sb;
    char filetype[MAXLINE/2], buf[MAXLINE];
    tuple<char*, size_t, time_t> zipped_data;
    pair<char*, size_t> compressed_file;

    if((fd=open(filename.c_str(), O_RDONLY)) == -1) {
        cerr << "Error opening " << filename << endl;
        return;
    }

    if((ret=fstat(fd, &sb)) == -1){
        cerr << "fstat() error for " << filename << endl;
        close(fd);
        return;
    }
    file_modified_time = sb.st_mtime;
    close(fd);

    auto query_result = cache->get_cached_page(filename);
    if(get<0>(query_result) == nullptr) {
        compressed_file = deflate_file(filename, Z_DEFAULT_COMPRESSION);
        zipped_data = tuple(compressed_file.first, compressed_file.second, file_modified_time);
        cache->set_cached_page({filename, zipped_data});

    }else{
        double diff_sec = difftime(file_modified_time, get<2>(query_result));
        
        // file was modified - cached version no longer up-to-date
        if(diff_sec > 0) {
            compressed_file = deflate_file(filename, Z_DEFAULT_COMPRESSION);
            zipped_data = tuple(compressed_file.first, compressed_file.second, file_modified_time);
            cache->set_cached_page({filename, zipped_data});
        }else{
            zipped_data = query_result;
        }
    }

    cache->check_updates();

    get_filetype((char*)filename.c_str(), filetype);    
    connex_hndl.write_str("HTTP/1.0 200 OK\r\n"); 
    connex_hndl.write_str("Server: Web Server\r\n"); 
    connex_hndl.write_str("Content-Encoding: deflate\r\n"); 
    connex_hndl.write_str(format("Content-Length: {}\r\n.", get<1>(zipped_data)));
    connex_hndl.write_str(format("Content-Type: {}\r\n.", filetype));
    connex_hndl.write_str("\r\n"); 

    connex_hndl.write_data(get<0>(zipped_data), static_cast<int>(get<1>(zipped_data)));
}

void ClientHandler::parse_request() {
    size_t bytes_read;
    char raw_req[MAX_FILESIZE];

    bytes_read = connex_hndl.read_data(raw_req, MAX_FILESIZE);

    std::string req_str(raw_req, bytes_read);  // Ensure the string is built with actual bytes read
    std::vector<std::string> request_lines = splitline(req_str, '\r');

    if (request_lines.empty()) throw GenericError::CLI_REQ_LINE_INVALID;

    int line_ct = 0;
    for (auto &line : request_lines) {
        if (line == "\n" || line.empty()) break;

        if (line_ct == 0) {
            request_line = splitline(line, ' ');
        } else {
            line = line.substr(1);  // Remove newline character at the beginning
            size_t sep_idx = line.find(':');
            if (sep_idx == std::string::npos || sep_idx == 0) {
                throw GenericError::CLI_INVALID_HEADER;
            }

            request_hdrs.insert({
                line.substr(0, sep_idx),
                line.substr(sep_idx + 2)
            });
        }

        line_ct++;
    }
}

void ClientHandler::redirect_cli_404() {
    string filename = "assets/404.html";
    char filetype[MAXLINE/2], buf[MAXLINE];
    ssize_t ret;

    ifstream ifs(filename, std::ifstream::binary);
    if(!ifs) {
        cerr << filename << " non found." << endl;
        return;
    }
    std::filebuf* pbuf = ifs.rdbuf();
    std::size_t file_size = pbuf->pubseekoff (0,ifs.end,ifs.in);
    pbuf->pubseekpos (0,ifs.in);
    char* file_buf=new char[file_size];
    pbuf->sgetn (file_buf,file_size);
    ifs.close();
    get_filetype((char*)filename.c_str(), filetype);    

    connex_hndl.write_str("HTTP/1.1 404 Not Found\r\n");
    connex_hndl.write_str("Server: Web Server\r\n");
    connex_hndl.write_str(format("Content-Length: {}\r\n", file_size));
    connex_hndl.write_str(format("Content-Type: {}\r\n", filetype));
    connex_hndl.write_str("Connection: close\r\n");
    connex_hndl.write_str("\r\n");

    connex_hndl.write_data(file_buf, file_size);
}

void ClientHandler::redirect_cli() 
{
    char buf[MAXLINE];
    connex_hndl.read_data(buf, MAXLINE); // TODO CHECK read vs readline

    connex_hndl.write_str("HTTP/1.0 301 Moved Permanently\r\n"); 
    connex_hndl.write_str("Location: https://ardendiak.com\r\n");
    connex_hndl.write_str("Content-Length: 0\r\n");
    connex_hndl.write_str("Connection: close\r\n");
    connex_hndl.write_str("\r\n");
}

std::ostream& operator<<(std::ostream &os, const ClientHandler &cli)
{
    os << cli.hostname << ":" << cli.port << endl;
    return os;
}