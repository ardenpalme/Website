#include <iostream>
#include <fstream>
#include <string>

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "net_util.hpp"

#define MAX_NUM_HEADERS 120
#define MAX_FILESIZE (10 * 1024)

using namespace std;

void handle_client(ClientHandler &hndl) {
    hndl.serve_client();
}

void ClientHandler::serve_client(void) {
    string method = request_line[0];
    string uri = request_line[1].substr(1); // removes leading '/'
    string protocol = request_line[2];

    cout << "method = [" << method << "]" << endl
         << "uri = [" << uri << "]" << endl
         << "protocol = [" << protocol << "]" << endl;

    if(method == "GET") {
        if(uri.size() == 0) uri = "index.html";
        serve_static(uri);
    }
}

ostream &operator<<(ostream &os, ClientHandler &cli) {
    return os;
}

string get_filetype(string filename) {
    string ret;
    if(filename == ".html")
        ret = "text/html";
    else if(filename == ".png")
        ret = "image/png";
    else if(filename == ".jpg")
        ret = "image/jpeg";
    else
        ret = "text/plain";

    return ret;
}  

void ClientHandler::send_resp_hdr(string request_target, size_t file_size)
{
    char filetype[MAXLINE], buf[MAXLINE];

    if (request_target == ".html")
        strcpy(filetype, "text/html");
    else if (request_target == ".gif")
        strcpy(filetype, "image/gif");
    else if (request_target == ".png")
        strcpy(filetype, "image/png");
    else if (request_target == ".jpg")
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");

    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n", file_size);
    Rio_writen(connfd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(connfd, buf, strlen(buf)); 
}

void get_filetype(char *filename, char *filetype) {
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

void ClientHandler::serve_static(string filename) {
    char filetype[MAXLINE], buf[MAXLINE];

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

vector<string> splitline(string line, char delim) {
    size_t idx = 0, start_idx = 0;
    vector<string> vec;
    string scan_str;

    while(idx != string::npos) {
        idx = line.find(delim, start_idx);
        if(idx == string::npos) {
            scan_str = line.substr(start_idx);
        }else{
            scan_str = line.substr(start_idx, (idx - start_idx));
        }
        vec.push_back(scan_str);
        start_idx = idx + 1;
    }
    return vec;
}

void ClientHandler::parse_request(void) {
    char buf[MAXLINE];
    int line_ct = 0;

    while(line_ct < MAX_NUM_HEADERS) {
        Rio_readlineb(&rio, buf, MAXLINE);
        string line(buf);

        if(line == "\r\n") break;

        if(line_ct == 0) {
            request_line = splitline(line, ' ');

        }else{
            size_t sep_idx = line.find(':');
            size_t end_idx = line.find("\r\n");
            request_hdrs.insert({line.substr(0, sep_idx), 
                                 line.substr(sep_idx+2, (end_idx - (sep_idx+2)))});
        }
        line_ct++;
    }
}
