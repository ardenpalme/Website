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

    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);
    auto ssl_mutex_ptr = make_shared<mutex>();

    int listenfd = Open_listenfd((char*)srv_port_str.c_str());

    struct sockaddr_storage addr;
    socklen_t msg_len = sizeof(struct sockaddr_storage);
    int ret;
    
    while(1) {
        char cli_name[100];
        char cli_port[100];
        int connfd = Accept(listenfd, (struct sockaddr*)&addr, &msg_len);
        Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
            100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

        SSL *ssl = SSL_new(ctx);
        if(!ssl){
            cout << "SSL is NULL!\n";
            Close(connfd);
            continue;
        } 
        if((ret=SSL_set_fd(ssl, connfd)) < 0){
            cerr << "SSL error during setting fd: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
            SSL_free(ssl);
            Close(connfd);
            continue;
        }

        if ((ret = SSL_accept(ssl)) <= 0) {
            int ssl_err = SSL_get_error(ssl, ret);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
                cerr << "retry SSL_accept" <<endl;
                continue;
            }else{
                cerr << "SSL accept error: " << ERR_error_string(ERR_get_error(), nullptr) << endl;
                SSL_free(ssl);
                Close(connfd);
                continue;
            }
        } 

        auto cli_hndl_ptr = std::make_shared<ClientHandler>(connfd, ssl);
        std::thread cli_thread(handle_client, cli_hndl_ptr);
        cli_thread.detach();
    }
    return 0;
}
void handle_client(shared_ptr<ClientHandler> cli_hndl) {
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

    if((err=cli_hndl->serve_client()) != cli_err::NONE) {
        cerr << "Error serving client request." << endl;
        cli_hndl->cleanup();
        return;
    }

    cli_hndl->cleanup();
}