#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERV_PORT 9000
#define MAX_EVENTS 10

int main(int argc, char* argv[])
{
    int reuseaddr = 1;

    /* Create a socket */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Set socket options */
    /* 防止出现 TIME_OUT端口占用的情况 */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddr, sizeof(reuseaddr)) == -1) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EXIT_FAILURE);
    }

    /* Set up the address struct */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    /* Bind and listen */
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 32) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Create epoll instance */
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; // 如果fd上有数据可供读取，且未阻塞，则会触发该事件
    // EPOLLRDHUP 用于指示对等端已启动优雅关机
    ev.data.fd = listenfd;
    // 修改epoll的兴趣列表
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        // 如果发生了 形如ev的事件，那么就会返回到
        struct epoll_event events[MAX_EVENTS];
        // 返回状态改变的fd的数量，这个 nfds 是 n_fd_s
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
                if (connfd == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                // Set connfd to non-blocking mode (optional, depending on your use case)

                // Add connfd to epoll instance
                ev.events = EPOLLIN | EPOLLET; // 边沿触发，就是只有fd状态有改变的时候就触发（输电）
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
                    perror("epoll_ctl: connfd");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Handle read/write events here
                const char* pcontent = "I sent sth. to client!\n";
                write(events[n].data.fd, pcontent, strlen(pcontent));
                printf("The server sent a string to the client~~~~~~~~~~!\n");
                close(events[n].data.fd);
            }
        }
    }

    close(listenfd);
    return 0;
}
