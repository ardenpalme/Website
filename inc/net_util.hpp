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

class Client{
public:
    Client(int _connfd) : connfd{_connfd} {
        Rio_readinitb(&rio, connfd);
        parse_request(connfd);
        serve_static();
    }

    ~Client() {
        Close(connfd);
    }

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

    void serve_static(void) {
        string req = requests[0];
        size_t start_idx = req.find("/");
        size_t end_idx = req.find("HTTP");

        string filename = req.substr(start_idx+1, end_idx-5);
        if(filename.size() < 4) filename = "index.html";

        ifstream ifs(filename, std::ifstream::binary);
        if(!ifs) {
            cout << filename << " non existant\n";
            return;
        }

        std::filebuf* pbuf = ifs.rdbuf();
        std::size_t file_size = pbuf->pubseekoff (0,ifs.end,ifs.in);
        pbuf->pubseekpos (0,ifs.in);

        char* buf=new char[file_size];
        pbuf->sgetn (buf,file_size);
        ifs.close();

        Rio_writen(connfd, buf, file_size);

        delete[] buf;
    }
};