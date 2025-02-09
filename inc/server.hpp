#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <memory>
#include <tuple>

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "util.hpp"
#include "client.hpp"

class ServerContext {
public:
    ServerContext(int _port) : port{_port} {
        gnutls_global_init();
        cache = make_shared<Cache<tuple<char*, size_t, time_t>>>();
    }

    gnutls_session_t new_session(void) {
        session = create_session();
        configure_session(session);
        return session;
    }

    shared_ptr<Cache<tuple<char*, size_t, time_t>>> get_cache(void) {
        return cache;
    }

private:
    int port;
    gnutls_session_t session;
    shared_ptr<Cache<tuple<char*, size_t, time_t>>> cache;

    void init_ssl();

    void cleanup_openssl();

    gnutls_session_t create_session();

    void configure_session(gnutls_session_t session);
};

#endif /* __SERVER_HPP__ */