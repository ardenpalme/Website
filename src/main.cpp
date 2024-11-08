#include<iostream>
#include<fstream>
#include<string>
#include<thread>
#include<chrono>

#include "csapp.h"
#include "net_util.hpp"

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    // Load server certificate and private key
    if (SSL_CTX_use_certificate_file(ctx, "/etc/ssl/certs/www_ardenpalme_com.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "/etc/ssl/private/custom.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    uint16_t srv_port = 443;
    std::string srv_port_str = std::to_string((int)srv_port);

    // Initialize OpenSSL
    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    int listenfd = Open_listenfd((char*)srv_port_str.c_str());

    struct sockaddr_storage addr;
    socklen_t msg_len = sizeof(struct sockaddr_storage);
    
    while(1) {
        char cli_name[100];
        char cli_port[100];
        cout << "waiting for connections..." << endl;
        int connfd = Accept(listenfd, (struct sockaddr*)&addr, &msg_len);
        Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
            100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

        printf("Accepted connection from %s:%s\n", cli_name, cli_port);
        /*
        ClientHandler client_hndlr(connfd);
        std::thread cli_thread(handle_client, std::ref(client_hndlr));
        cli_thread.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        */


        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, connfd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            std::cout << "TLS handshake successful" << std::endl;

            // Simple HTTPS response
            const char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 12\r\n\r\n"
                "Hello World";
            
            SSL_write(ssl, response, strlen(response));
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        Close(connfd);
    }
    return 0;
}