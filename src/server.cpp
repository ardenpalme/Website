#include <iostream>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "server.hpp"

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

    // Load certificate and key
    ret = gnutls_certificate_set_x509_key_file(x509_cred,
                                               "/etc/pki/tls/private/ardendiak_com.crt",
                                               "/etc/pki/tls/private/ardendiak_com.key",
                                               GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load certificate or key: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load certificate and key
    ret = gnutls_certificate_set_x509_key_file(x509_cred,
                                               "/etc/pki/tls/private/ardenpalme_com.crt",
                                               "/etc/pki/tls/private/ardenpalme_com.key",
                                               GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load certificate or key: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load CA bundle to trust certificates
    ret = gnutls_certificate_set_x509_trust_file(x509_cred, 
                                                 "/etc/pki/tls/private/ardendiak_com_ca_bundle.crt", 
                                                 GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load CA bundle: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load CA bundle to trust certificates
    ret = gnutls_certificate_set_x509_trust_file(x509_cred, 
                                                 "/etc/pki/tls/private/diakhatepalme_com_ca_bundle.crt", 
                                                 GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load CA bundle: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load CA bundle to trust certificates
    ret = gnutls_certificate_set_x509_trust_file(x509_cred, 
                                                 "/etc/pki/tls/private/ardenpalme_com_ca_bundle.crt", 
                                                 GNUTLS_X509_FMT_PEM);
    if (ret < 0) {
        std::cerr << "Failed to load CA bundle: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Assign credentials to session
    gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

    gnutls_db_set_cache_expiration(session, 3600); 
    gnutls_handshake_set_timeout(session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
    ret = gnutls_priority_set_direct(session, "NORMAL:%SERVER_PRECEDENCE", NULL);
    if (ret < 0) {
        std::cerr << "Failed to set priority: " << gnutls_strerror(ret) << std::endl;
        exit(EXIT_FAILURE);
    }
}
