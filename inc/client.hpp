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
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "csapp.h"
#include "util.hpp"

using namespace std;

enum class cli_err {
    CLI_CLOSED_CONN,
    RETRY,
    FATAL,
    PARSE_ERROR,
    SERVE_ERROR,
    CLEANUP_ERROR,
    CONN_ERROR,
    ADDR_ERROR,
    NONE
};

class ClientHandler {
public:
    ClientHandler(int _connfd, 
                  gnutls_session_t _session, 
                  char *_cli_name,
                  char *_cli_port) : 
        connfd{_connfd}, session{_session} {
            if(_cli_name != NULL) cli_name = string(_cli_name);
            else cli_name = nullptr;

            if(_cli_port != NULL) cli_port = string(_cli_port);
            else cli_port = nullptr;
        }

    cli_err cleanup(void);

    cli_err parse_request(void);

    cli_err serve_client(shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache);

    cli_err retrieve_local(string host, string port);

    friend ostream &operator<<(ostream &os, ClientHandler &cli);

private:
    int connfd;
    string cli_name, cli_port;
    rio_t rio;
    gnutls_session_t session;
    vector<string> request_line;
    map<string,string> request_hdrs;

    void redirect_cli_404();

    void send_resp_hdr(string request_target, size_t file_size);

    void serve_static(string request_target);

    void serve_static_compress(string request_target, shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache);

    void redirect(string target);
};

#endif /* __CLIENT_HPP__ */