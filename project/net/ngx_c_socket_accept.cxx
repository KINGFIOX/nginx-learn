#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h> //errno
#include <fcntl.h> //open
#include <stdarg.h> //va_start....
#include <stdint.h> //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h> //gettimeofday
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
 * @brief 回收连接池中的连接，不然连接池很快就耗光了
 * 
 * @param c 
 */
void CSocket::ngx_close_accepted_connection(lpngx_connection_t c)
{
    int fd = c->fd;
    ngx_free_connection(c); // 从 连接池 中释放连接
    c->fd = -1; // 清空c.fd
    if (close(fd) == -1) {
        ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocket::ngx_close_accepted_connection()中close(%d)失败!", fd);
    }
    return;
}

/**
 * @brief 添加新连接，并加入到红黑树中
 * 
 * @param oldc 监听socket 对应的 ngx_connection_t 对象（内存）
 */
void CSocket::ngx_event_accept(lpngx_connection_t oldc)
{
    int s;
    static int use_accept4 = 1; // 默认使用use_accept4

    socklen_t socklen = sizeof(struct sockaddr);
    do {
        struct sockaddr mysockaddr;
        if (use_accept4) {
            /* 非阻塞socket，不管有没有客户端连接，直接返回 */
            s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
        } else {
            /* 相当于accept4的flags=0 */
            s = accept(oldc->fd, &mysockaddr, &socklen);
        }

        /* 进入错误处理阶段 */
        if (s == -1) {
            int err = errno;
            if (err == EAGAIN) { /* 已完成连接队列中，没有用户了，然而我这里是 非阻塞的，就会发生这个错误 */
                return;
            }
            int level = NGX_LOG_ALERT; // 错误等级

            if (err == ECONNABORTED) {
                /* 客户端在连接被接受之前终止 */
                level = NGX_LOG_ERR;
            } else if (err == EMFILE || err == ENFILE) { /* 连入的用户太多了 */
                /* EMFILE: fd的数量已经达到了 进程的上限; ENFILE: fd的数量已经达到了 系统的上限 */
                level = NGX_LOG_CRIT;
            }
            ngx_log_error_core(level, errno, "CSocket::ngx_event_accept()中accept4()失败!");

            /* enosys: invalid system call，系统没有实现accept4，坑爹，改用accept */
            if (use_accept4 && err == ENOSYS) {
                use_accept4 = 0;
                continue;
            }

            /* 对方关闭socket */
            if (err == ECONNABORTED) {
                /* TODO 可以啥也不用做，当然也可以写日志 */
            }

            if (err == EMFILE || err == ENFILE) {
                // TODO do sth.
            }
            return;
        } // end if (s == -1)

        lpngx_connection_t newc = ngx_get_connection(s);
        if (newc == NULL) { /* 连接池满了 */
            if (close(s) == -1) {
                ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocket::ngx_event_accept()中close(%d)失败!", s);
            }
            return;
        }

        /* 一股脑拷贝 */
        memcpy(&newc->s_sockaddr, &mysockaddr, socklen);

        if (!use_accept4) {
            /* 如果不是 accept4，手动设置非阻塞 */
            if (setnonblocking(s) == false) {
                ngx_close_accepted_connection(newc);
                return;
            }
        }

        newc->listening = oldc->listening;
        newc->w_ready = 1; /* 准备好写数据了 */
        newc->rhandler = &CSocket::ngx_wait_request_handler; /* 读取到数据改如何处理 */

        /* 红黑树中添加敏感事件 */
        if (ngx_epoll_add_event(s,
                1, /* 默认client连接进来是可读的，因为是客户端主动连接的，这个肯定是客户端有求于服务器 */
                0,
                EPOLLET, /* epoll_et 边缘触发 */
                EPOLL_CTL_ADD,
                newc)
            == -1) {
            ngx_close_accepted_connection(newc);
            return;
        }

        break; /* 这种 while(1) + break 写法还是挺有意思的 */
    } while (1);
}