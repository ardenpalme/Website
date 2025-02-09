#pragma once

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

#include "connection.hpp"
#include "cache.hpp"

class ClientHandler {
public:
    ClientHandler(ConnectionHandler& _connex_hndl,
                  char *_hostname,
                  char *_port) : connex_hndl{std::move(_connex_hndl)} {
        if(_hostname != NULL) hostname = std::string(_hostname);
        else hostname = nullptr;

        if(_port != NULL) port = std::string(_port);
        else port = nullptr;
    }

    void parse_request(void);

    void serve_client(std::shared_ptr<Cache<std::tuple<char*, size_t, time_t>>> cache);

    void redirect_cli();

    friend std::ostream& operator<<(std::ostream& os, const ClientHandler& cli);

private:
    ConnectionHandler connex_hndl;
    std::string hostname, port;
    std::vector<std::string> request_line;
    std::map<std::string,std::string> request_hdrs;

    void redirect_cli_404();

    void send_resp_hdr(std::string request_target, size_t file_size);

    void serve_static(std::string request_target);

    void serve_static_compress(std::string request_target, std::shared_ptr<Cache<std::tuple<char*, size_t, time_t>>> cache);

    void redirect(std::string target);
};