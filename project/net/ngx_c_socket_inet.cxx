#include <errno.h> //errno
#include <fcntl.h> //open
#include <netinet/in.h>
#include <stdarg.h> //va_start....
#include <stdint.h> //uintptr_t --> unsigned long int
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> //gettimeofday
#include <sys/types.h>
#include <time.h> //localtime_r
#include <unistd.h> //STDERR_FILENO等
//#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> //ioctl

#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

/**
 * @brief 网络地址结构体 --> 
 * 
 * @param sa 保存有客户端的ip地址
 * @param port 1: 端口信息也放到组合成的字符串; 0：则不包含端口信息
 * @param text 
 * @param len 
 * @return size_t ip:port的长度
 */
size_t ngx_sock_ntop(struct sockaddr* sa, int port, u_char* text, size_t len)
{

    switch (sa->sa_family) {
    case AF_INET: {
        // 如果是ipv4，那么转换成sockaddr_in结构体
        struct sockaddr_in* sin = (struct sockaddr_in*)sa;
        u_char* p = (u_char*)&sin->sin_addr; //
        // 日志打印 ip:port
        if (port) {
            p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
        } else {
            p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
        }
        return (p - text); /* text 是 写入缓冲区的起始位置，p 是 写入缓冲区的 下一个可写入位置，两个相减得到的是ip:port的长度 */
        break;
    }
    default:
        return 0;
        break;
    }

    return 0;
}
