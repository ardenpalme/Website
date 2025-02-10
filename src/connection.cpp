#include "connection.hpp"
#include "csapp.h"

const static size_t LISTENQ  = 1024;
const static size_t MAX_TLS_RECORD_SIZE = 16384; 

size_t ConnectionHandler::read_data(char *data, size_t num_req_bytes) 
{
    int count = 0;
    if(connex_type == ConnectionType::SOCKFD) {

    }else if(connex_type == ConnectionType::TLS_SESSION) {
        count = gnutls_record_recv(session, data, num_req_bytes);
        if (count <= 0) {
            switch (count) {
                case 0:
                    throw GenericError::TLS_CONNEX_CLOSE;

                case GNUTLS_E_AGAIN:
                case GNUTLS_E_INTERRUPTED:
                    throw GenericError::TLS_RETRY;

                /* TLS Re-negotiation unsupported */
                case GNUTLS_E_REHANDSHAKE: 
                default:
                    throw GenericError::FATAL;
            }
        }

    }else{
        throw GenericError::INVALID_SOCKET_TYPE;
    }

    return count;
}

int ConnectionHandler::readline(char *data, size_t max_lines)
{
    if(connex_type != ConnectionType::SOCKFD) {
        throw GenericError::INVALID_SOCKET_TYPE;
    }
    
    int num_lines, read_ct;
    char c, *usrbuf_ptr = data;

    for(int num_lines=1; num_lines<max_lines; num_lines++) {
        if((read_ct=read_data(&c, 1)) == 1) {
            *usrbuf_ptr++ = c;

            if(c == '\n') {
                num_lines++;
                break;
            }

        }else if(read_ct == 0) {
            if(num_lines == 1) return 0; // EOF no lines read
            else break;

        }else{
            throw GenericError::FATAL;
        }
    }

    *usrbuf_ptr = 0; 
    return num_lines-1;
}

size_t ConnectionHandler::write_data(char *payload, size_t payload_sz)
{
    int ret;
    size_t num_written = 0;
    size_t bytes_left = payload_sz;
    char *userbuf_ptr = payload;

    if(connex_type == ConnectionType::SOCKFD) {
        while(bytes_left > 0) {
            if((num_written=write(connfd, userbuf_ptr, bytes_left)) <= 0) {
                if(errno == EINTR) num_written=0; // retry 
                else throw GenericError::SOCKFD_SEND;
            }
            bytes_left -= num_written;
            userbuf_ptr += num_written;
        }

    }else if(connex_type == ConnectionType::TLS_SESSION) {
        while (bytes_left > 0) {
            size_t chunk_size = std::min(MAX_TLS_RECORD_SIZE, bytes_left);
            if((num_written = gnutls_record_send(session, userbuf_ptr, chunk_size)) < 0) {
                throw GenericError::TLS_SEND;

            }else if(ret == 0) {
                std::cerr << "GnuTLS session closed unexpectedly" << std::endl;
                break;
            }
            bytes_left -= num_written;
            userbuf_ptr += num_written;
        }
    }else{
        throw GenericError::INVALID_SOCKET_TYPE;
    }
    return num_written;
}

size_t ConnectionHandler::write_str(const string &str)
{
    size_t num_bytes = str.length();
    char *payload = (char*)str.data();  //TODO check type conversion
    return write_data(payload, num_bytes);
}


/* CSAPP Functions - BEGIN */

int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Connection failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        if (close(listenfd) < 0) { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }


    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}

// Wrappers for Error-checking convenience
int Open_listenfd(char *port) 
{
    int rc;
    if ((rc = open_listenfd(port)) < 0)
	cerr << "Open_listenfd error: " << strerror(errno) << endl;
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;
    if ((rc = accept(s, addr, addrlen)) < 0)
	cerr << "Accept error: " << strerror(errno) << endl;
    return rc;
}

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;
    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, 
                          servlen, flags)) != 0) 
        cerr << "Getnameinfo error " <<  gai_strerror(rc) << endl;
}

/* CSAPP Functions - END */