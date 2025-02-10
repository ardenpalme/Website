#include "connection.hpp"
#include "csapp.h"

const static size_t MAX_TLS_RECORD_SIZE = 16384; 

int ConnectionHandler::read_data(char *userbuf, size_t num_req_bytes) 
{
    int count = 0;
    if(connex_type == ConnectionType::SOCKFD) {
        rio_t rio;
        rio_readinitb(&rio, connfd);
        count = rio_readlineb(&rio, userbuf, num_req_bytes);
        if(count < 0) throw GenericError::SOCKFD_SEND;

    }else if(connex_type == ConnectionType::TLS_SESSION) {
        count = gnutls_record_recv(session, userbuf, num_req_bytes);
        if (count <= 0) {
            switch (count) {
                case 0:
                    throw GenericError::TLS_CONNEX_CLOSE;

                case GNUTLS_E_AGAIN:
                case GNUTLS_E_INTERRUPTED:
                    throw GenericError::TLS_RETRY;

                /* TLS Re-negotiation unsupported */
                case GNUTLS_E_REHANDSHAKE: 
                default:
                    throw GenericError::FATAL;
            }
        }

    }else{
        throw GenericError::INVALID_CONNEX_TYPE;
    }

    return count;
}

size_t send_large_data(gnutls_session_t session, const char* data, size_t data_size) {
    size_t bytes_sent = 0;

    while (bytes_sent < data_size) {
        size_t chunk_size = std::min(MAX_TLS_RECORD_SIZE, data_size - bytes_sent);

        int ret = gnutls_record_send(session, data + bytes_sent, chunk_size);
        if (ret < 0) {
            std::cerr << "GnuTLS send error: " << gnutls_strerror(ret) << std::endl;
            throw GenericError::TLS_SEND;
        }

        bytes_sent += ret;

        if (ret == 0) {
            std::cerr << "Connection closed unexpectedly." << std::endl;
            break;
        }
    }

    return bytes_sent;
}

size_t ConnectionHandler::write_data(char *payload, size_t payload_sz)
{
    size_t bytes_sent = 0;
    if(connex_type == ConnectionType::SOCKFD) {
        bytes_sent = rio_writen(connfd, payload, payload_sz);

    }else if(connex_type == ConnectionType::TLS_SESSION) {
        while (bytes_sent < payload_sz) {
            size_t chunk_size = std::min(MAX_TLS_RECORD_SIZE, payload_sz - bytes_sent);

            int ret = gnutls_record_send(session, payload + bytes_sent, chunk_size);
            if (ret < 0) {
                std::cerr << "GnuTLS send error: " << gnutls_strerror(ret) << std::endl;
                throw GenericError::TLS_SEND;
            }

            bytes_sent += ret;

            if (ret == 0) {
                std::cerr << "Connection closed unexpectedly." << std::endl;
                break;
            }
        }

    }else{
        throw GenericError::INVALID_CONNEX_TYPE;
    }
    return bytes_sent;
}

size_t ConnectionHandler::write_str(const string &str)
{
    int ret;
    char buf[MAXLINE];
    size_t bytes_sent;

    if(connex_type == ConnectionType::TLS_SESSION) {
        sprintf(buf, "%s", str.c_str());
        if((ret = send_large_data(session, buf, strlen(buf))) < 0) {
            std::cerr << "GnuTLS write error: " << gnutls_strerror(ret) << std::endl;
            return 0;
        }
        bytes_sent = ret;

    }else if(connex_type == ConnectionType::SOCKFD) {
        sprintf(buf, "%s", str.c_str());
        bytes_sent = rio_writen(connfd, buf, strlen(buf));
    }

    return bytes_sent;
}
