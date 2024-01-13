#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    // 开启epoll
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    // 将 fd 添加到
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[10];
    int num_events = epoll_wait(epoll_fd, events, 10, -1);
    if (num_events == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_events; i++) {
        if (events[i].events & EPOLLIN) {
            // 处理读事件
            printf("File descriptor %d is ready for reading\n", events[i].data.fd);
        }
    }

    close(fd);
    close(epoll_fd);
    return 0;
}
