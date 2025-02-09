#include<iostream>
#include<fstream>
#include<string>
#include<thread>
#include<chrono>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "csapp.h"
#include "util.hpp"
#include "client.hpp"
#include "server.hpp"

#define HTTP_PORT (80)
#define HTTPS_PORT (443)
#define NUM_LISTEN_PORTS (2)

void redirect_client(shared_ptr<ClientHandler> cli_hndl);
void handle_client(shared_ptr<ClientHandler> cli_hndl, shared_ptr<Cache<tuple<char*,size_t,time_t>>> cache);

int main(int argc, char *argv[]) {
    int ret;
    int listen_fd1, listen_fd2;
    int nfds, epoll_fd;
    struct sockaddr_storage addr;
    socklen_t msg_len = sizeof(struct sockaddr_storage);
    std::string srv_port_str;
    struct epoll_event ev, events[NUM_LISTEN_PORTS];


    ServerContext srv_ctx;
    auto cache_ptr = srv_ctx.get_cache();
    srv_port_str = std::to_string((int)HTTPS_PORT);
    listen_fd1 = Open_listenfd((char*)srv_port_str.c_str());

    srv_port_str = std::to_string((int)HTTP_PORT);
    listen_fd2 = Open_listenfd((char*)srv_port_str.c_str());
    
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        cerr << "Failed to create epoll file descriptor";
    }

    ev.events = EPOLLIN;  
    ev.data.fd = listen_fd1;     
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd1, &ev) == -1) {
        cerr << "Failed to add fd1 to epoll";
    }

    ev.events = EPOLLIN;  
    ev.data.fd = listen_fd2;     
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd2, &ev) == -1) {
        cerr << "Failed to add fd1 to epoll";
    }

    while (1) {
        char cli_name[100];
        char cli_port[100];

        nfds = epoll_wait(epoll_fd, events, NUM_LISTEN_PORTS, -1);
        if (nfds == -1) {
            cerr << "epoll_wait failed";
            break;
        }
        for (int i = 0; i < nfds; i++) {
            // Handle HTTPS
            if (events[i].data.fd == listen_fd1) {
                int connfd = Accept(listen_fd1, (struct sockaddr*)&addr, &msg_len);
                Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
                    100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

                // Initialize a GnuTLS session
                gnutls_session_t session = srv_ctx.new_session();
                if(!session){
                    cout << "GnuTLS session is NULL!\n";
                    Close(connfd);
                    continue;
                } 

                // Set the connection file descriptor
                gnutls_transport_set_int(session, connfd);

                // Perform TLS handshake
                ret = gnutls_handshake(session);
                if (ret < 0) {
                    std::cerr << "GnuTLS handshake failed: " << gnutls_strerror(ret) << std::endl;
                    gnutls_deinit(session);
                    Close(connfd);
                    continue;
                }

                auto cli_hndl_ptr = std::make_shared<ClientHandler>(connfd, session, cli_name, cli_port);
                std::thread cli_thread(handle_client, cli_hndl_ptr, cache_ptr);
                cli_thread.detach();

            // Redirect all HTTP requests to HTTPS
            }else if(events[i].data.fd == listen_fd2) {
                int connfd = Accept(listen_fd2, (struct sockaddr*)&addr, &msg_len);
                Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
                    100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

                gnutls_session_t tmp_session;

                auto cli_hndl_ptr = std::make_shared<ClientHandler>(connfd, tmp_session, cli_name, cli_port);
                std::thread cli_thread(redirect_client, cli_hndl_ptr);
                cli_thread.detach();
            }
        }
    }

    return 0;
}

void redirect_client(shared_ptr<ClientHandler> cli_hndl) {
    cli_hndl->redirect_cli();
    cli_hndl->close_socket();
}

void handle_client(shared_ptr<ClientHandler> cli_hndl, shared_ptr<Cache<tuple<char*,size_t,time_t>>> cache) {
    cli_err err;
    err = cli_hndl->parse_request();
    while(err == cli_err::RETRY) {
        err = cli_hndl->parse_request();
    }

    if (err == cli_err::PARSE_ERROR) {
        cerr << "Request parsing failed." << endl;
        cli_hndl->cleanup();
        return;

    } else if (err == cli_err::CLI_CLOSED_CONN) {
        cerr << "Client closed connection during request parsing." << endl;
        cli_hndl->cleanup();
        return;

    } else if (err != cli_err::NONE) {
        cerr << "Error during request parsing." << endl;
        cli_hndl->cleanup();
        return;
    }

    if((err=cli_hndl->serve_client(cache)) != cli_err::NONE) {
        cerr << "Error serving client request." << endl;
        cli_hndl->cleanup();
        return;
    }

    cli_hndl->cleanup();
}