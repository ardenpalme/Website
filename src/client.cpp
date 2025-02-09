#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <tuple>

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "client.hpp"
#include "util.hpp"

#define MAX_NUM_HEADERS 120
#define MAX_FILESIZE (1024 * 1024)
#define MIN_RESP_SZ 5

using namespace std;

int send_large_data(gnutls_session_t session, const char* data, size_t data_size);

cli_err ClientHandler::serve_client(shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache) {
    cli_err err_code = cli_err::NONE;

    if (request_line.size() < 3) {
        cerr << "Invalid request line format." << endl;
        return cli_err::SERVE_ERROR;
    }

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
        cerr << "Unsupported HTTP method: " << method << endl;
        err_code = cli_err::SERVE_ERROR;
    }
    return err_code;
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
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Server: Web Server\r\n");
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Content-Encoding: deflate\r\n");
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Content-Length: %lu\r\n", get<1>(zipped_data));
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Content-Type: %s\r\n\r\n", filetype);
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    //SSL_write(ssl, static_cast<const void*>(get<0>(zipped_data)), static_cast<int>(get<1>(zipped_data)));
    if((ret = send_large_data(session, 
                get<0>(zipped_data), 
                static_cast<int>(get<1>(zipped_data)))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

}

cli_err ClientHandler::parse_request() {
    char *raw_req = new char[MAX_FILESIZE];

    // Read from the GnuTLS session
    ssize_t bytes_read = gnutls_record_recv(session, raw_req, MAX_FILESIZE);

    if (bytes_read <= 0) {
        delete[] raw_req;

        // Handle different GnuTLS error cases
        switch (bytes_read) {
            case 0:
                std::cerr << "Client closed connection." << std::endl;
                return cli_err::CLI_CLOSED_CONN;

            case GNUTLS_E_AGAIN:
            case GNUTLS_E_INTERRUPTED:
                std::cerr << "GnuTLS read wants retry." << std::endl;
                return cli_err::RETRY;

            case GNUTLS_E_REHANDSHAKE:
                std::cerr << "GnuTLS re-handshake request received, not supported." << std::endl;
                return cli_err::FATAL;

            default:
                std::cerr << "GnuTLS read error: " << gnutls_strerror(bytes_read) << std::endl;
                return cli_err::FATAL;
        }
    }

    std::string req_str(raw_req, bytes_read);  // Ensure the string is built with actual bytes read
    delete[] raw_req;

    std::vector<std::string> request_lines = splitline(req_str, '\r');

    if (request_lines.empty()) {
        std::cerr << "Empty request received." << std::endl;
        return cli_err::PARSE_ERROR;
    }

    int line_ct = 0;
    for (auto &line : request_lines) {
        if (line == "\n" || line.empty()) break;

        if (line_ct == 0) {
            request_line = splitline(line, ' ');
        } else {
            line = line.substr(1);  // Remove newline character at the beginning
            size_t sep_idx = line.find(':');
            if (sep_idx == std::string::npos || sep_idx == 0) {
                std::cerr << "Malformed header: " << line << std::endl;
                return cli_err::PARSE_ERROR;
            }

            request_hdrs.insert({
                line.substr(0, sep_idx),
                line.substr(sep_idx + 2)
            });
        }

        line_ct++;
    }

    return cli_err::NONE;
}

cli_err ClientHandler::cleanup() {
    // Attempt to gracefully close the TLS session
    int ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
    if (ret < 0) {
        std::cerr << "Error shutting down GnuTLS session: " << gnutls_strerror(ret) << std::endl;
        gnutls_deinit(session);  // Free session resources even if shutdown fails
        Close(connfd);
        return cli_err::CLEANUP_ERROR;
    }

    // Free GnuTLS session resources
    gnutls_deinit(session);
    Close(connfd);
    return cli_err::NONE;
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

    sprintf(buf, "HTTP/1.1 404 Not Found\r\n"); 
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Server: Web Server\r\n");
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Content-type: %s\r\n", filetype);
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Content-length: %lu\r\n", file_size);
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    sprintf(buf, "Connection: close\r\n\r\n");
    if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }

    //SSL_write(ssl, file_buf, file_size);
    if((ret = send_large_data(session, file_buf, file_size)) < 0) {
        std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
    }
    delete[] file_buf;
}

int send_large_data(gnutls_session_t session, const char* data, size_t data_size) {
    const size_t MAX_TLS_RECORD_SIZE = 16384;  // 16KB
    size_t bytes_sent = 0;

    while (bytes_sent < data_size) {
        size_t chunk_size = std::min(MAX_TLS_RECORD_SIZE, data_size - bytes_sent);

        int ret = gnutls_record_send(session, data + bytes_sent, chunk_size);
        if (ret < 0) {
            std::cerr << "GnuTLS send error: " << gnutls_strerror(ret) << std::endl;
            return ret;
        }

        bytes_sent += ret;

        if (ret == 0) {
            std::cerr << "Connection closed unexpectedly." << std::endl;
            break;
        }
    }

    //std::cout << "Total bytes sent: " << bytes_sent << std::endl;
    return 0;  // Success
}

void ClientHandler::redirect_cli() {
    rio_t rio;
    char buf[MAXLINE];
  
    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio, buf, MAXLINE); 
    sprintf(buf, "HTTP/1.0 301 Moved Permanently\r\n"); 
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Location: https://ardendiak.com\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Content-Length: 0\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n\r\n");
    Rio_writen(connfd, buf, strlen(buf));
}