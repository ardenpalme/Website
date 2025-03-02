#pragma once

#include <iostream>
#include <fstream>
#include <string>
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

#include "connection.hpp"
#include "cache.hpp"

using namespace std;

enum class WebProtocol {HTTPS, HTTP};

class ClientHandler {
public:
    ClientHandler(unique_ptr<ConnectionHandler> &_connex_hndl,
                  char *_hostname,
                  char *_port, 
                  WebProtocol protocol) : protocol{protocol} 
    {
        connex_hndl = std::move(_connex_hndl);

        if(_hostname != NULL) hostname = string(_hostname);
        else hostname = nullptr;

        if(_port != NULL) port = string(_port);
        else port = nullptr;
    }

    WebProtocol get_protocol() { return protocol; }

    void parse_request(void);

    void serve_client(shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache);

    void redirect_cli();

    friend ostream& operator<<(ostream& os, const ClientHandler& cli);

private:
    unique_ptr<ConnectionHandler> connex_hndl;
    string hostname, port;
    WebProtocol protocol;
    vector<string> request_line;
    map<string,string> request_hdrs;

    void redirect_cli_404();

    void send_resp_hdr(string request_target, size_t file_size);

    void serve_static(string request_target);

    void serve_static_compress(string request_target, shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache);

    void redirect(string target);
};