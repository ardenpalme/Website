#include <iostream>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "server.hpp"

void ServerContext::init_ssl() {
}

void ServerContext::cleanup_openssl() {
    gnutls_global_deinit();  
}

gnutls_session_t ServerContext::create_session() {
    gnutls_session_t session;

    int ret = gnutls_init(&session, GNUTLS_SERVER);
    if (ret < 0) {
        std::cerr << "Unable to create GnuTLS session: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }
    return session;
}

void ServerContext::configure_session(gnutls_session_t session) {
    gnutls_certificate_credentials_t x509_cred;
    int ret;

    // Allocate certificate credentials
    ret = gnutls_certificate_allocate_credentials(&x509_cred);
    if (ret < 0) {
        std::cerr << "Failed to allocate credentials: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load certificate and key
    ret = gnutls_certificate_set_x509_key_file(x509_cred,
                                               "/etc/pki/tls/private/diakhatepalme_com.crt",
                                               "/etc/pki/tls/private/diakhatepalme_com.key",
                                               GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load certificate or key: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Assign credentials to session
    gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

    // Enforce minimum TLS version 1.2
    ret = gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3", NULL);
    if (ret < 0) {
        std::cerr << "Failed to set priority: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // TODO Disable TLS renegotiation
    //gnutls_session_set_flags(session, GNUTLS_NO_RENEGOTIATION);
}


