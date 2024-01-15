#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_SOCKETS 10
#define MAX_EVENTS 10
#define LISTEN_PORT 8080

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    int is_used;
} SocketConnection;

typedef struct {
    SocketConnection pool[MAX_SOCKETS];
    int count;
    int epoll_fd;
} SocketPool;

void init_socket_pool(SocketPool* sp)
{
    sp->count = 0;
    sp->epoll_fd = epoll_create1(0);
    if (sp->epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        sp->pool[i].socket_fd = -1;
        sp->pool[i].is_used = 0;
        memset(&(sp->pool[i].address), 0, sizeof(sp->pool[i].address));
    }
}

int add_socket_to_pool(SocketPool* sp, int port)
{
    if (sp->count >= MAX_SOCKETS) {
        fprintf(stderr, "Socket pool is full.\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd == -1) {
        perror("Cannot create socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        close(sockfd);
        perror("Cannot bind socket to port");
        return -1;
    }

    if (listen(sockfd, SOMAXCONN) == -1) {
        close(sockfd);
        perror("Cannot listen on socket");
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; // We are interested in read events
    ev.data.fd = sockfd;

    if (epoll_ctl(sp->epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        close(sockfd);
        perror("epoll_ctl: listen_sock");
        return -1;
    }

    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (sp->pool[i].is_used == 0) {
            sp->pool[i].socket_fd = sockfd;
            sp->pool[i].is_used = 1;
            sp->count++;
            break;
        }
    }

    return sockfd;
}

void run_event_loop(SocketPool* sp)
{
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int n = epoll_wait(sp->epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
                // An error has occured on this fd, or the socket is not ready for reading
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (events[i].data.fd == sp->pool[0].socket_fd) { // Assuming the first socket is the listening socket
                // We have a notification on the listening socket, which means one or more incoming connections
                while (1) {
                    struct sockaddr in_addr;
                    socklen_t in_len = sizeof(in_addr);
                    int infd = accept(sp->pool[0].socket_fd, &in_addr, &in_len);
                    if (infd == -1) {
                        break; // Process all of the connections that have come in
                    }
                    // Do something with the accepted connection
                }
            } else {
                // We have data on the fd waiting to be read
                int done = 0;
                // Read and display data, handle client connection
            }
        }
    }
}

int main()
{
    SocketPool sp;
    init_socket_pool(&sp);
    add_socket_to_pool(&sp, LISTEN_PORT);
    run_event_loop(&sp);
    return 0;
}
