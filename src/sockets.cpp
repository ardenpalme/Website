#include "sockets.hpp"

const static int LISTENQ  = 1024;

size_t SocketHandle::read_data(char *data, size_t num_req_bytes) 
{
    int count = 0;
    while(byte_count <= 0) {
        byte_count = read(connfd, buf, sizeof(buf));

        if(byte_count < 0) {
            throw SocketError::FATAL;

        }else if(byte_count == 0) {
            throw SocketError::READ_EOF;

        }else{
            bufptr = buf; // reset buffer ptr
        }
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    if(byte_count < num_req_bytes) count = byte_count;
    else count = num_req_bytes;

    memcpy(data, bufptr, count);
    bufptr += count;
    byte_count -= count;

    return count;
}

int SocketHandle::readline(char *data, size_t bytes)
{
    int num_lines, read_ct;
    char c, *usrbuf_ptr = data;

    for(int i=0; i<bytes; i++) {
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
            throw SocketError::LINE_READ;
        }
    }

    *usrbuf_ptr = 0; 
    return num_lines-1;
}

size_t SocketHandle::write_data(char *payload, size_t payload_sz)
{
    size_t num_written = 0;
    size_t bytes_left = payload_sz;
    char *userbuf_ptr = payload;

    while(bytes_left > 0) {
        if((num_written=write(connfd, userbuf_ptr, bytes_left)) <= 0) {
            if(errno == EINTR) num_written=0; // retry 
            else throw SocketError::WRITE;
        }
        bytes_left -= num_written;
        userbuf_ptr += num_written;
    }
    return num_written;
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
            continue;  /* Socket failed, try the next */

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