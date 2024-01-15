#include <errno.h> //errno
#include <fcntl.h> //open
#include <stdarg.h> //va_start....
#include <stdint.h> //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
 * @brief 来数据处理的时候，当链接上有数据来的时候，本函数会被ngx_epoll_process_events()调用
 * 
 * @param c 
 */
void CSocket::ngx_wait_request_handler(lpngx_connection_t c)
{
    //
    return;
}
