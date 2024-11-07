#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "csapp.h"

using namespace std;

class Client {
public:
    Client(int _connfd) : connfd{_connfd} {
        Rio_readinitb(&rio, connfd);
        parse_request(connfd);
    }

    ~Client() {
        Close(connfd);
    }

    void serve_static(void);

    friend ostream &operator<<(ostream &os, Client &cli);

private:
    int connfd;
    rio_t rio;
    vector<string> requests;

    void parse_request(int connfd) {
        char buf[MAXLINE];
        do{
            Rio_readlineb(&rio, buf, MAXLINE);
            string req_line(buf);
            requests.push_back(req_line);
        }while(strcmp(buf, "\r\n"));
    }

    void get_filetype(char *filename, char *filetype) 
    {
        if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
        else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
        else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
        else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
        else
        strcpy(filetype, "text/plain");
    }  

};

/* Client-specific handler */
void handle_client(Client &client);