#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <utility>

enum class GenericError {
    FATAL,
    SOCKFD_READ_EOF,
    INVALID_CONNEX_TYPE,
    TLS_SEND,
    SOCKFD_SEND,
    TLS_CONNEX_CLOSE,
    TLS_RETRY,
    CLI_HTTP_METHOD,
    CLI_INVALID_HEADER,
    CLI_REQ_LINE_INVALID,
    FILE_NOT_FOUND
};

std::string report_error(GenericError &err);

std::string humanReadableSize(size_t bytes);

std::vector<std::string> splitline(std::string line, char delim);

std::pair<char *, size_t> deflate_file(std::string filename, int compression_level);

void get_filetype(char *filename, char *filetype);