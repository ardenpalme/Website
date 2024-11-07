#include <iostream>
#include <fstream>
#include "net_util.hpp"

using namespace std;

ostream &operator<<(ostream &os, Client &cli) {
    for (auto ele : cli.requests) {
        os << ele << " ";
    }
    return os;
}

void Client::serve_static(void) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

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


    /* Send response headers to client */
    get_filetype((char*)filename.c_str(), filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n", file_size);
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(connfd, buf, strlen(buf));    //line:netp:servestatic:endserve


    char* buffer=new char[file_size];
    pbuf->sgetn (buffer,file_size);
    ifs.close();

    Rio_writen(connfd, buffer, file_size);

    delete[] buffer;
}

// Client Thread
void handle_client(Client &client) {
    cout << "client handler\n";
    client.serve_static();
    cout << client << endl;
}