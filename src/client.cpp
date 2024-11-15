#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "client.hpp"
#include "util.hpp"

#define MAX_NUM_HEADERS 120
#define MAX_FILESIZE (10 * 1024)

using namespace std;

const string srv_hostname = "ardenpalme.com";
const string srv_static_ip = "54.177.178.124";
const string dashboard_port = "8050";

cli_err ClientHandler::serve_client(shared_ptr<Cache> cache) {
    if (request_line.size() < 3) {
        cerr << "Invalid request line format." << endl;
        return cli_err::SERVE_ERROR;
    }

    string method = request_line[0];
    string uri = request_line[1];
    string protocol = request_line[2];

    if(method == "GET") {
        if(uri == "/dashboard") { 
            string redirect_url = "https://" + srv_hostname + ":" + dashboard_port;
            redirect(redirect_url);
        }else{
            if(uri == "/") {
                uri = "index.html";
            }else{ 
                uri = uri.substr(1);
            }
            uri.insert(0, "data/");
            serve_static_compress(uri, cache);
        }
    } else {
        cerr << "Unsupported HTTP method: " << method << endl;
        return cli_err::SERVE_ERROR;
    }
    return cli_err::NONE;
}

void ClientHandler::redirect(string target) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.1 302 Found\r\n"); 
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Location: %s\r\n\r\n", target.c_str());
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Connection: close\r\n\r\n");
    SSL_write(ssl, buf, strlen(buf));
}

void ClientHandler::serve_static(string filename) {
    char filetype[MAXLINE], buf[MAXLINE];

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

    /* Send response headers to client */
    get_filetype((char*)filename.c_str(), filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    SSL_write(ssl, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    SSL_write(ssl, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n", file_size);
    SSL_write(ssl, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    SSL_write(ssl, buf, strlen(buf));

    SSL_write(ssl, file_buf, file_size);

    delete[] file_buf;
}

void ClientHandler::serve_static_compress(string filename, shared_ptr<Cache> cache) {
    char filetype[MAXLINE], buf[MAXLINE];
    pair<char*, size_t> zipped_data;

    auto query_result = cache->get_cached_page(filename);
    if(query_result.second == 0) {
        zipped_data = deflate_file(filename, Z_DEFAULT_COMPRESSION);
        cache->set_cached_page({filename, zipped_data});
    }else{
        zipped_data = query_result;
    }


    get_filetype((char*)filename.c_str(), filetype);    
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Server: Tiny Web Server\r\n");
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-Encoding: deflate\r\n");
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-length: %lu\r\n", zipped_data.second);
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    SSL_write(ssl, buf, strlen(buf));

    SSL_write(ssl, zipped_data.first, zipped_data.second);
}

cli_err ClientHandler::parse_request() {
    char *raw_req = new char[MAX_FILESIZE];

    int bytes_read = SSL_read(ssl, raw_req, MAX_FILESIZE);

    if (bytes_read <= 0) {
        int ssl_error = SSL_get_error(ssl, bytes_read);
        delete[] raw_req; 
        
        switch (ssl_error) {
            case SSL_ERROR_ZERO_RETURN:
                cerr << "Client closed connection." << endl;
                return cli_err::CLI_CLOSED_CONN;

            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                cerr << "SSL read wants retry." << endl;
                return cli_err::RETRY;

            default:
                cerr << "SSL read error: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
                return cli_err::FATAL;
        }
    }

    string req_str(raw_req);
    delete[] raw_req;

    vector<string> request_lines = splitline(req_str, '\r');

    if (request_lines.empty()) {
        cerr << "Empty request received." << endl;
        return cli_err::PARSE_ERROR;
    }

    int line_ct = 0;
    for(auto line : request_lines){

        if(line == "\n" || line.empty()) break;

        if(line_ct == 0) {
            request_line = splitline(line, ' ');

        }else{
            line = line.substr(1); // remove the newline character at the beginning
            size_t sep_idx = line.find(':');
            if (sep_idx == string::npos || sep_idx == 0) {
                cerr << "Malformed header: " << line << endl;
                return cli_err::PARSE_ERROR;
            }

            request_hdrs.insert({line.substr(0, sep_idx), 
                                 line.substr(sep_idx+2)});
        }

        line_ct++;
    }
    return cli_err::NONE;
}

cli_err ClientHandler::cleanup() {
    if (SSL_shutdown(ssl) < 0) {
        cerr << "Error shutting down SSL: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
        SSL_free(ssl);
        Close(connfd);
        return cli_err::CLEANUP_ERROR;
    }

    SSL_free(ssl);
    Close(connfd);
    return cli_err::NONE;
}
