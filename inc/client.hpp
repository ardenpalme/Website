#ifndef __NET_UTIL_HPP__
#define __NET_UTIL_HPP__

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <memory>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "csapp.h"

using namespace std;

enum class cli_err {
    CLI_CLOSED_CONN,
    RETRY,
    FATAL,
    PARSE_ERROR,
    SERVE_ERROR,
    CLEANUP_ERROR,
    NONE
};

class ClientHandler {
public:
    ClientHandler(int _connfd, SSL *_ssl, shared_ptr<mutex> _ssl_mutex) : 
        connfd{_connfd}, ssl{_ssl}, ssl_mutex{_ssl_mutex} {}

    cli_err cleanup(); 

    cli_err parse_request();

    cli_err serve_client(void);

    friend ostream &operator<<(ostream &os, ClientHandler &cli);

private:
    int connfd;
    rio_t rio;
    SSL *ssl;
    vector<string> request_line;
    map<string,string> request_hdrs;
    shared_ptr<mutex> ssl_mutex;

    void send_resp_hdr(string request_target, size_t file_size);

    void serve_static(string request_target);
};

void handle_client(ClientHandler &client);


#endif /* __NET_UTIL_HPP__ */