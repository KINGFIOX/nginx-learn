#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_macro.h"

void ngx_process_events_and_times()
{
    // g_socket.ngx_epoll_process_events(-1); // TODO
    return;
}