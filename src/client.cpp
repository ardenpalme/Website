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
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "client.hpp"
#include "util.hpp"

#define MAX_NUM_HEADERS 120
#define MAX_FILESIZE (8000000)
#define MIN_RESP_SZ 5

using namespace std;

cli_err ClientHandler::serve_client(shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache) {
    cli_err err_code = cli_err::NONE;

    if (request_line.size() < 3) {
        cerr << "Invalid request line format." << endl;
        return cli_err::SERVE_ERROR;
    }

    string method = request_line[0];
    string uri = request_line[1];
    string protocol = request_line[2];

    string dashboard_prefix = "/dashboard/";

    if(method == "GET") {
        cout << uri << endl;
        if(uri.find(dashboard_prefix) != string::npos) {
            cout << "retrieve " <<  uri << " from localhost:8050\n";
            err_code = retrieve_local("localhost", "8050");

        }else{
            if(uri == "/") {
                uri = "index.html";
            }else{ 
                uri = uri.substr(1);
            }
            uri.insert(0, "assets/");
            serve_static_compress(uri, cache);
        }
    } else {
        cerr << "Unsupported HTTP method: " << method << endl;
        err_code = cli_err::SERVE_ERROR;
    }
    return err_code;
}

cli_err ClientHandler::retrieve_local(string host, string port) {
    char buf[MAXLINE];
    int clientfd;
    char *raw_resp = new char[MAX_FILESIZE];
    rio_t rio;

    clientfd = Open_clientfd((char*)host.c_str(), (char*)port.c_str());
    cout << "Connected to " << host << ":" << port << " ..." << endl;

    string first_req_line;
    for(auto ele : request_line) first_req_line = first_req_line + " " + ele;

    cout << first_req_line << endl;
    sprintf(buf, "%s\r\n", first_req_line.c_str());
    Rio_writen(clientfd, buf, strlen(buf));

    for(auto hdr : request_hdrs) {
        if(hdr.first.find("Host") != string::npos) {
            sprintf(buf, "Host: %s:%s\r\n", host.c_str(), port.c_str());
            Rio_writen(clientfd, buf, strlen(buf));
            cout << " " << buf << endl;

        }else{
            sprintf(buf, "%s: %s\r\n", hdr.first.c_str(), hdr.second.c_str());
            Rio_writen(clientfd, buf, strlen(buf));
            cout << "[" << hdr.first << ": " << hdr.second << "]" << endl;
        }
    }

    cout << endl;
    sprintf(buf, "\r\n");
    Rio_writen(clientfd, buf, strlen(buf));

    int bytes_read = Rio_readn(clientfd, raw_resp, MAX_FILESIZE);
    cout << "read " << bytes_read << " bytes from " << host << ":" << port << endl;

    vector<string> resp_hdrs;

    int line_start_idx = 0;
    for(int i=0; i<bytes_read; i++) {
        if(raw_resp[i] == '\r') {
            string hdr(&raw_resp[line_start_idx], (i-line_start_idx));
            if(hdr.length() >= MIN_RESP_SZ) { // detects CRLF
                resp_hdrs.push_back(hdr);
            }else{
                line_start_idx+=2; 
                break;
            }
            line_start_idx = i+2;
        }
    }

    int resp_payload_idx = line_start_idx;

    if(resp_hdrs[0].find("200 OK") == string::npos) {
        cerr << "Internal Resource Response Invalid\n";
        for(auto hdr : resp_hdrs) cout << "[" << hdr << "]" << endl;

        delete [] raw_resp;
        return cli_err::SERVE_ERROR;
    }

    cout << "uncompressed size: " << bytes_read - resp_payload_idx << " bytes" << endl;
    auto compressed_obj = deflate_object(&raw_resp[resp_payload_idx], 
                                        bytes_read - resp_payload_idx, 
                                        Z_DEFAULT_COMPRESSION);
    char *payload = compressed_obj.first;
    size_t payload_sz = compressed_obj.second;
    cout << "payload size: " << payload_sz << " bytes" << endl;

    for(auto hdr : resp_hdrs) {
        if(hdr.find("Server:") != string::npos){
            sprintf(buf, "Server: Web Server\r\n");
            SSL_write(ssl, buf, strlen(buf));
            cout << " " << buf << endl;

        }else if(hdr.find("Content-Length") != string::npos){
            sprintf(buf, "Content-Length: %lu\r\n", payload_sz);
            SSL_write(ssl, buf, strlen(buf));
            cout << " " << buf << endl;

            sprintf(buf, "Content-Encoding: deflate\r\n");
            SSL_write(ssl, buf, strlen(buf));
            cout << " " << buf << endl;

        }else{
            sprintf(buf, "%s\r\n", hdr.c_str());
            SSL_write(ssl, buf, strlen(buf));
            cout << "[" << hdr << "]" << endl;
        }
    }

    // HTTP Response header termination CRLF
    cout << endl;
    sprintf(buf, "\r\n");
    SSL_write(ssl, buf, strlen(buf));

    SSL_write(ssl, payload, static_cast<int>(payload_sz));

    delete [] raw_resp;
    return cli_err::NONE;
}

void ClientHandler::serve_static(string filename) {
    char filetype[MAXLINE/2], buf[MAXLINE];

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
    sprintf(buf, "Server: Web Server\r\n");
    SSL_write(ssl, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n", file_size);
    SSL_write(ssl, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    SSL_write(ssl, buf, strlen(buf));

    SSL_write(ssl, file_buf, file_size);

    delete[] file_buf;
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


    get_filetype((char*)filename.c_str(), filetype);    
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Server: Web Server\r\n");
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-Encoding: deflate\r\n");
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-Length: %lu\r\n", get<1>(zipped_data));
    SSL_write(ssl, buf, strlen(buf));

    sprintf(buf, "Content-Type: %s\r\n\r\n", filetype);
    SSL_write(ssl, buf, strlen(buf));

    SSL_write(ssl, static_cast<const void*>(get<0>(zipped_data)), static_cast<int>(get<1>(zipped_data)));

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
