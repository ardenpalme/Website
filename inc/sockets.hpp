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

using namespace std;

#define RIO_BUFSIZE 8192

enum class SocketError {
    FATAL,
    LINE_READ,
    READ_EOF,
    WRITE,
    CLOSE,
    NONE
};

class SocketHandle {
public:
    SocketHandle(int _connfd) : connfd{_connfd} {
        byte_count = 0;
        bufptr = buf;
    }

    ~SocketHandle() {
        close(connfd);
    }

    int readline(char *data, size_t bytes);
    size_t read_data(char *data, size_t num_req_bytes);
    size_t write_data(char *data, size_t bytes);

private:
    int connfd;
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