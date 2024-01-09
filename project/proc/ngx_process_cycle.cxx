#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_macro.h"

#include <signal.h>

// 函数声明 ---------------
static void ngx_start_worker_processes(int threadnums);
static int ngx_spawn_process_cycle(int inum, const char* pprocname);
static void ngx_worker_process_cycle(int inum, const char* pprocname);
static void ngx_worker_process_init(int inum);

// 描述：创建 worker 子进程
void ngx_master_process_cycle()
{
    sigset_t set; // 信号集

    sigemptyset(&set); // 清空信号集

    // 建议fork()子进程时学习下面这种写法，防止信号的干扰
    sigaddset(&set, SIGCHLD); // 子进程状态改变
    sigaddset(&set, SIGALRM); // 定时器超时
    sigaddset(&set, SIGIO); // 异步IO
    sigaddset(&set, SIGINT); // 终端中断符
    sigaddset(&set, SIGHUP); // 连接断开
    sigaddset(&set, SIGUSR1); // 自定义信号1
    sigaddset(&set, SIGUSR2); // 自定义信号2
    sigaddset(&set, SIGWINCH); // 终端大小改变
    sigaddset(&set, SIGTERM); // 终止
    sigaddset(&set, SIGQUIT); // 退出

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
    }
}