#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2)
        printError("Excepted port");

    char buffer[bufferLen];
    int mainSock;

    auto port = strtol(argv[1], nullptr, 10);
    if (errno == ERANGE || port > UINT16_MAX || port <= 0)
        printError("Number of port should be uint16_t");
    int epoll;
    if ((epoll = epoll_create1(0)) == -1)
        printSysError("epoll_create1");

    struct epoll_event events[NUMBER_OF_EVENTS], event;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = INADDR_ANY;


    if ((mainSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) <= 0)
        printSysError("socket");

    if (bind(mainSock, (struct sockaddr *) &address, sizeof(address)) < 0)
        printSysError("bind");

    int backlog = 10;
    if (listen(mainSock, backlog) < 0)
        printSysError("listen");

    epollAdding(epoll, mainSock, &event);

    int numfd, resfd;
    char size;
    while (true) {
        if ((numfd = epoll_wait(epoll, events, NUMBER_OF_EVENTS, -1)) == -1)
            printSysError("epoll_wait");

        for (int i = 0; i < numfd; ++i) {
            if (events[i].data.fd == mainSock) {
                struct sockaddr_in client;
                socklen_t socklen = sizeof(client);
                if ((resfd = accept(mainSock, reinterpret_cast<sockaddr *>(&client), &socklen)) == -1)
                    printSysError("accept");

                auto name = inet_ntoa(client.sin_addr);
                printf("Client %d: connected.(%s)\n", resfd, name);
                epollAdding(epoll, resfd, &event);
            } else {
                resfd = events[i].data.fd;

                if (recvAll(resfd, buffer, 1, true)) {
                    epollEnding(epoll, resfd, "recv");
                    continue;
                }

                size = buffer[0];
                if (recvAll(resfd, buffer, size, true)) {
                    epollEnding(epoll, resfd, "recv");
                    continue;
                }

                printf("%s\n", buffer);
                if (sendAll(resfd, &size, 1, true)) {
                    epollEnding(epoll, resfd, "send");
                    continue;
                }

                if (sendAll(resfd, buffer, size, true)) {
                    epollEnding(epoll, resfd, "send");
                    continue;
                }

                printf("%d: Processed.\n", resfd);
            }
        }
    }
    closeSocket(mainSock);
    return 0;
}
