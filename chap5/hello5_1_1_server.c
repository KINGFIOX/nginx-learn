#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERV_PORT 9000

int main(int argc, char* argv[])
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 这里是用了ipv4

    struct sockaddr_in serv_addr; // 服务器的地址结构体
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT); // 绑定我们自定义的端口
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本地所有的ip地址，可能有多个网卡

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) { // 绑定服务器地址结构体
        perror("bind failed");
        return -1; // or handle the error in an appropriate way
    }

    // 参数2表示：服务器可以积压未处理完的链接总个数
    listen(listenfd, 32);

    const char* pcontent = "I sent sth. to client!";

    int i = 0;
    for (;;) {
        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        write(connfd, pcontent, strlen(pcontent));
        printf("连接%d\n", i++);
        close(connfd); // 发送完成，就关闭与客户端的连接
    }

    close(listenfd);
    return 0;
}