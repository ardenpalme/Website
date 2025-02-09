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

using namespace std;

#define RIO_BUFSIZE 8192

enum class ConnectionType {
    SOCKFD, TLS_SESSION 
};

class ConnectionHandler {
public:
    ConnectionHandler(int _connfd) : connfd{_connfd} {
        connex_type = ConnectionType::SOCKFD;
        byte_count = 0;
        bufptr = buf;
    }

    ConnectionHandler(gnutls_session_t _session) : session{_session} {
        connex_type = ConnectionType::TLS_SESSION;
    }

    ConnectionHandler(ConnectionHandler &&hndl) {
        connex_type = hndl.connex_type;
        connfd = hndl.connfd;
        session = hndl.session;

        // ignore the from object's internal buffers
        byte_count = 0;
        bufptr = buf;
    }

    ConnectionHandler& operator=(ConnectionHandler &&hndl) {
        connex_type = hndl.connex_type;
        connfd = hndl.connfd;
        session = hndl.session;

        // ignore the from object's internal buffers
        byte_count = 0;
        bufptr = buf;
        return *this;
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

    int readline(char *data, size_t max_lines);
    size_t read_data(char *data, size_t num_req_bytes);
    size_t write_data(char *data, size_t bytes);
    size_t write_str(const string &str);

private:
    ConnectionType connex_type;
    int connfd;
    gnutls_session_t session;

    int byte_count;         /* Unread bytes in internal buf */
    char *bufptr;           /* Next unread byte in internal buf */
    char buf[RIO_BUFSIZE];  /* Internal buffer */
};

/* CSAPP Functions - BEGIN */

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags);

int Open_listenfd(char *port);

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags);

/* CSAPP Functions - END */