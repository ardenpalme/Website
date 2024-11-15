#include <iostream>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server.hpp"

void ServerContext::init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void ServerContext::cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *ServerContext::create_context() {
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

void ServerContext::configure_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "/etc/ssl/certs/www_ardenpalme_com.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "/etc/ssl/private/custom.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Ensure the private key is valid
    if (!SSL_CTX_check_private_key(ctx)) {
        cerr << "Private key does not match the public certificate" << endl;
        exit(EXIT_FAILURE);
    }

    // Enforce TLSv1.2
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
}
