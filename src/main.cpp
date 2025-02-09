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

#include "csapp.h"
#include "util.hpp"
#include "client.hpp"
#include "server.hpp"

void handle_client(shared_ptr<ClientHandler> cli_hndl, shared_ptr<Cache<tuple<char*,size_t,time_t>>> cache);

int main(int argc, char *argv[]) {
    int listenfd, ret;
    struct sockaddr_storage addr;
    uint16_t srv_port = 443;
    socklen_t msg_len;

    ServerContext srv_ctx(srv_port);
    auto cache_ptr = srv_ctx.get_cache();

    std::string srv_port_str = std::to_string((int)srv_port);
    listenfd = Open_listenfd((char*)srv_port_str.c_str());
    msg_len = sizeof(struct sockaddr_storage);
    
    while(1) {
        char cli_name[100];
        char cli_port[100];
        int connfd = Accept(listenfd, (struct sockaddr*)&addr, &msg_len);
        Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
            100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

        // Initialize a GnuTLS session
        gnutls_session_t session = srv_ctx.new_session();
        if((ret=gnutls_record_set_max_size(session, 1024 * 2) != 0)) {
            cout << "couldn't set max record size!!!!!!\n";
        }

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
    }
    return 0;
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