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
 * @brief 
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
 * @brief 建立新连接的专用函数，当新链接进入时，本函数会被ngx_epoll_process_events()调用  // TODO
 * 
 * @param lodc 
 */
void CSocket::ngx_event_accept(lpngx_connection_t oldc)
{
    struct sockaddr mysockaddr;
    socklen_t socklen;
    int err;
    int level;
    int s;
    static int use_accept4 = 1;
    lpngx_connection_t newc;

    socklen = sizeof(mysockaddr);
    do {
        if (use_accept4) {
            accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
        } else {
            s = accept(oldc->fd, &mysockaddr, &socklen);
        }

        if (s == -1) {
            err = errno;
            if (err == EAGAIN) {
                return;
            }
            if (err == ECONNABORTED) {
                level = NGX_LOG_ERR;
            } else if (err == EMFILE || err == ENFILE) {
                level = NGX_LOG_CRIT;
            }
            ngx_log_error_core(level, errno, "CSocket::ngx_event_accept()中accept4()失败!");
            if (use_accept4 && err == ENOSYS) {
                use_accept4 = 0;
                continue;
            }
            if (err == EMFILE || err == ENFILE) {
                // TODO do sth.
            }
            return;
        } // end if (s == -1)

        newc = ngx_get_connection(s);
        if (newc == NULL) {
            if (close(s) == -1) {
                ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocket::ngx_event_accept()中close(%d)失败!", s);
            }
            return;
        }
        memcpy(&newc->s_sockaddr, &mysockaddr, socklen);

        if (!use_accept4) {
            if (setnonblocking(s) == false) {
                ngx_close_accepted_connection(newc);
                return;
            }
        }

        newc->listening = oldc->listening;
        newc->w_ready = 1;
        newc->rhandler = &CSocket::ngx_wait_request_handler;

        if (ngx_epoll_add_event(s, 1, 0, EPOLLET, EPOLL_CTL_ADD, newc) == -1) {
            ngx_close_accepted_connection(newc);
            return;
        }

        break;

    } while (1);
}