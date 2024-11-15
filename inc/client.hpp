#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

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
    ClientHandler(int _connfd, SSL *_ssl) : connfd{_connfd}, ssl{_ssl} {}

    cli_err cleanup(void);

    cli_err parse_request(void);

    cli_err serve_client(void);

private:
    int connfd;
    rio_t rio;
    SSL *ssl;
    vector<string> request_line;
    map<string,string> request_hdrs;

    void send_resp_hdr(string request_target, size_t file_size);

    void serve_static(string request_target);

    void redirect(string target);
};


#endif /* __CLIENT_HPP__ */