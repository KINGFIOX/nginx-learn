#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

/**
 * @brief 创建守护进程
 * 
 * @return int ：-1 失败; 1 父进程; 0 子进程
 */
int ngx_daemon()
{
    // （1）fork() 一个子进程出来
    switch (fork()) {
    case -1:
        // 创建子进程失败
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中fork()失败!");
        return -1;
    case 0:
        // 子进程
        break;
    default:
        // 父进程
        return 1;
    } // end switch

    // 只有fork() 出来的子进程才能走到这个流程
    ngx_parent = ngx_pid;
    ngx_pid = getpid();

    // （2）脱离终端
    if (setsid() == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中setsid()失败!");
        return -1;
    }

    // （3）umask，获取最高的读写权限
    umask(0);

    // （4）打开黑洞设备，重定向
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中open(\"/dev/null\")失败!");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中dup2(STDIN)失败!");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (dup2(fd, STDERR_FILENO) == -1) {
        ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中dup2(STDERR)失败!");
        return -1;
    }
    return 0;
}
