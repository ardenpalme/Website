#include<iostream>
#include<fstream>
#include<string>
#include<thread>
#include<chrono>

#include "csapp.h"
#include "net_util.hpp"

int main(int argc, char *argv[])
{
    uint16_t srv_port = 80;
    std::string srv_port_str = std::to_string((int)srv_port);
    int listenfd = Open_listenfd((char*)srv_port_str.c_str());

    struct sockaddr_storage addr;
    socklen_t msg_len = sizeof(struct sockaddr_storage);
    
    while(1) {
        char cli_name[100];
        char cli_port[100];
        cout << "waiting for connections..." << endl;
        int connfd = Accept(listenfd, (struct sockaddr*)&addr, &msg_len);
        Getnameinfo((struct sockaddr*)&addr, msg_len, cli_name, 
            100, cli_port, 100, NI_NUMERICHOST | NI_NUMERICSERV);

        printf("Accepted connection from %s:%s\n", cli_name, cli_port);
        ClientHandler client_hndlr(connfd);
        std::thread cli_thread(handle_client, std::ref(client_hndlr));
        cli_thread.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}