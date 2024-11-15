#include<iostream>
#include<fstream>
#include<string>
#include<thread>
#include<chrono>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "csapp.h"
#include "util.hpp"
#include "client.hpp"
#include "server.hpp"

void handle_client(ClientHandler &cli_hndl, ServerContext &srv_hndl);

int main(int argc, char *argv[]) {
    int listenfd, ret;
    struct sockaddr_storage addr;
    uint16_t srv_port = 443;
    socklen_t msg_len;

    ServerContext srv_ctx(srv_port);

    std::string srv_port_str = std::to_string((int)srv_port);
    listenfd = Open_listenfd((char*)srv_port_str.c_str());

    auto ssl_mutex_ptr = make_shared<mutex>();
    msg_len = sizeof(struct sockaddr_storage);
    
    while(1) {
        char cli_name[100];
        char cli_port[100];
        int connfd = Accept(listenfd, (struct sockaddr*)&addr, &msg_len);
        Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
            100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);
        
        ssl_mutex_ptr->lock();
        SSL *ssl = srv_ctx.make_ssl();
        if(!ssl){
            cout << "SSL is NULL!\n";
            ssl_mutex_ptr->unlock();
            Close(connfd);
            continue;
        } 
        if((ret=SSL_set_fd(ssl, connfd)) < 0){
            cerr << "SSL error during setting fd: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
            SSL_free(ssl);
            ssl_mutex_ptr->unlock();
            Close(connfd);
            continue;
        }

        if (ret = SSL_accept(ssl) <= 0) {
            cerr << "SSL accept error: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
            SSL_free(ssl);
            ssl_mutex_ptr->unlock();
            Close(connfd);
            continue;
        } 
        ssl_mutex_ptr->unlock();

        auto cli_hndl_ptr = std::make_shared<ClientHandler>(connfd, ssl, ssl_mutex_ptr, cli_name, cli_port);
        srv_ctx.enqueue_client(cli_hndl_ptr);
        std::thread cli_thread(handle_client, std::ref(*cli_hndl_ptr), std::ref(srv_ctx));
        cli_thread.join();
    }
    return 0;
}

void handle_client(ClientHandler &cli_hndl, ServerContext &srv_ctx) {
    cli_err err;
    err = cli_hndl.parse_request();
    while(err == cli_err::RETRY) {
        err = cli_hndl.parse_request();
    }

    if (err == cli_err::PARSE_ERROR) {
        cerr << "Request parsing failed." << endl;
        cli_hndl.cleanup();
        return;

    } else if (err == cli_err::CLI_CLOSED_CONN) {
        cerr << "Client closed connection during request parsing." << endl;
        cli_hndl.cleanup();
        return;

    } else if (err != cli_err::NONE) {
        cerr << "Error during request parsing." << endl;
        cli_hndl.cleanup();
        return;
    }

    auto cache = srv_ctx.get_cache();
    if((err=cli_hndl.serve_client(cache)) != cli_err::NONE) {
        cerr << "Error serving client request." << endl;
        cli_hndl.cleanup();
        return;
    }

    cli_hndl.cleanup();
}