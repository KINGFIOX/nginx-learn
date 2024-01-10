#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

#include <csignal>
#include <errno.h>
#include <unistd.h>

// 主进程名字
static u_char master_process[] = "master process";

// 函数声明 ---------------
static void ngx_start_worker_processes(int threadnums);
static int ngx_spawn_process(int inum, const char* pprocname);
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
        // 掩码失败，写日志
        ngx_log_error_core(NGX_LOG_ALERT, errno, "ngx_master_process_cycle()中sigprocmask()失败!");
    }

    size_t size = sizeof(master_process);
    size += g_argvneedmem;
    if (size < 1000) {
        char title[1000] = { 0 };
        strcpy(title, (const char*)master_process);
        strcat(title, " ");
        for (int i = 0; i < g_os_argc; i++) {
            strcat(title, g_os_argv[i]);
        }
        ngx_setproctitle(title);
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......!", title, ngx_pid);
    } // 设置主进程标题

    // 创建子进程 -------------------
    CConfig* p_config = CConfig::GetInstance();
    int workprocess = p_config->GetIntDefault("Workerprocesses", 1);
    ngx_start_worker_processes(workprocess);

    // 走到这里，只有父进程会返回，子进程不会返回
    for (;;) {
        sigsuspend(&set);
    }

    return;
}

// 子进程的循环可能会有：资源释放的问题

static void ngx_start_worker_processes(int threadnums)
{
    for (int i = 0; i < threadnums; i++) {
        ngx_spawn_process(i, "worker process");
    }
    return;
}

/**
 * @brief 产生一个子进程
 * 
 * @param inum 进程编号
 * @param pprocname 子进程名字
 * @return int 
 */
static int ngx_spawn_process(int inum, const char* pprocname)
{
    pid_t pid;

    pid = fork();

    switch (pid) {
    case -1:
        ngx_log_error_core(NGX_LOG_ALERT, errno,
            "ngx_spawn_process()中fork()产生子进程num=%d procname=\"%s\"失败", inum, pprocname);
        return -1;
    case 0: // 子进程分支
        ngx_parent = ngx_pid; // ngx_pid是滞后的
        ngx_pid = getpid();
        ngx_worker_process_cycle(inum, pprocname);
        break;
    default: // 父进程分支
        break;
    } // end switch (pid)

    return 0;
}

/**
 * @brief 子进程工作
 * 
 * @param inum 进程编号
 * @param pprocname  子进程名字
 */
static void ngx_worker_process_cycle(int inum, const char* pprocname)
{
    ngx_process = NGX_PROCESS_WORKER;

    ngx_worker_process_init(inum);
    ngx_setproctitle(pprocname);
    ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......", pprocname, ngx_pid);

    // worker工作
    for (;;) {
        sleep(1);
    }
    return;
}

/**
 * @brief 初始化子进程
 * 
 * @param inum 
 */
static void ngx_worker_process_init(int inum)
{

    sigset_t set;

    sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error_core(NGX_LOG_ALERT, errno, "ngx_worker_process_init()中sigprocmask()失败!");
    }

    return;
}