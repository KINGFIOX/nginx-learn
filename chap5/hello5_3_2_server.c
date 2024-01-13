#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERV_PORT 9000

int main(int argc, char* argv[])
{
    // sock_stream，流socket，使用tcp
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* 封装ip地址、端口号 */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // ipv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有ip
    serv_addr.sin_port = htons(SERV_PORT); // 绑定端口

    int reuseaddr = 1; // 这里的1表示确实启用该选项，这个函数的接口很奇怪
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddr, sizeof(reuseaddr)) == -1) {
        char* peeorinfo = strerror(errno);
        printf("setsockopt(SO_REUSEADDR)返回值为-1, 错误码为:%d, 错误信息为:%s;\n", errno, peeorinfo);
    }

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        char* perrorinfo = strerror(errno);
        printf("bind返回值为-1，错误码为: %d，错误信息为: %s;\n", errno, perrorinfo);
        return -1;
    }
    if (listen(listenfd, 32) == -1) {
        char* perrorinfo = strerror(errno);
        printf("listen返回值为-1，错误码为: %d，错误信息为: %s;\n", errno, perrorinfo);
        return -1;
    }

    for (;;) {
        // 卡在这里
        const char* pcontent = "I sent sth. to client!\n";
        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        write(connfd, pcontent, strlen(pcontent));
        printf("本服务器给客户端方发送了一串字符~~~~~~~~~~!\n");
        close(connfd);
    }
    close(listenfd);
    return 0;
}