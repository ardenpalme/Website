#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <cstring>        
#include <cstdio>         
#include <cstdlib>
#include <sys/types.h>    
#include <sys/socket.h>   
#include <netdb.h>        
#include <arpa/inet.h>    
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "util.hpp"
#include "csapp.h"

using namespace std;

#define RIO_BUFSIZE 8192

enum class ConnectionType {
    SOCKFD, TLS_SESSION 
};

class ConnectionHandler {
public:
    ConnectionHandler(int _connfd) : connfd{_connfd} {
        connex_type = ConnectionType::SOCKFD;
    }

    ConnectionHandler(gnutls_session_t _session) : session{_session} {
        connex_type = ConnectionType::TLS_SESSION;
    }

    ~ConnectionHandler() {
        int ret = 0;
        if(connex_type == ConnectionType::TLS_SESSION) {
            if((ret = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0) {
                std::cerr << "Error shutting down GnuTLS session: " << gnutls_strerror(ret) << std::endl;
            }
            gnutls_deinit(session);

        }else if(connex_type == ConnectionType::SOCKFD) {
            close(connfd);
        }
    }

    int read_data(char *userbuf, size_t num_req_bytes);
    size_t write_data(char *data, size_t bytes);
    size_t write_str(const string &str);

private:
    int connfd;
    gnutls_session_t session;
    ConnectionType connex_type;
};