#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "csapp.h"

using namespace std;

class ClientHandler {
public:
    ClientHandler(int _connfd) : connfd{_connfd} {
        Rio_readinitb(&rio, connfd);
        parse_request();
    }

    void cleanup(void) {
        if (fsync(connfd) < 0) {
            perror("fsync error");
        }
        Close(connfd);
    }

    void serve_client(void);

    friend ostream &operator<<(ostream &os, ClientHandler &cli);

private:
    int connfd;
    rio_t rio;
    vector<string> request_line;
    map<string,string> request_hdrs;

    void parse_request(void);

    void send_resp_hdr(string request_target, size_t file_size);

    void serve_static(string request_target);
};

void handle_client(ClientHandler &client);
