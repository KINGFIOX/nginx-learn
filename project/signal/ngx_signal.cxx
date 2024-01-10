#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

typedef struct {
    int signo;
    const char* signame;

    // 信号处理函数，函数指针
    void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
} ngx_signal_t;

// 信号处理函数
static void ngx_signal_handler(int signo, siginfo_t* siginfo, void* ucontext);

// 获取子进程结束状态，防止子进程成为 僵尸进程
static void ngx_process_get_status(void);

ngx_signal_t signals[] = {
    // signo      signame             handler
    { SIGHUP, "SIGHUP", ngx_signal_handler },
    { SIGINT, "SIGINT", ngx_signal_handler },
    { SIGTERM, "SIGTERM", ngx_signal_handler },
    { SIGCHLD, "SIGCHLD", ngx_signal_handler },
    { SIGQUIT, "SIGQUIT", ngx_signal_handler },
    { SIGIO, "SIGIO", ngx_signal_handler },
    { SIGSYS, "SIGSYS, SIG_IGN", NULL },
    { 0, NULL, NULL } // 哨兵
};

int ngx_init_signals()
{
    // 遍历列表，设置信号处理函数
    for (ngx_signal_t* sig = signals; sig->signo != 0; sig++) {
        // 为sigaction准备 ------------------------
        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        if (sig->handler) {
            sa.sa_sigaction = sig->handler; // 用来指定信号处理函数的
            sa.sa_flags = SA_SIGINFO; // 简而言之：想要sa_sigaction有效的话，sa_flags要设置为SA_SIGINFO就行啦！套路固定
        } else {
            sa.sa_handler = SIG_IGN; // 设置为忽略
        }
        sigemptyset(&sa.sa_mask); // 把屏蔽信号集设置为空

        // 真正的设置信号处理函数sigaction --------------------
        if (sigaction(sig->signo, &sa, NULL) == -1) {
            // 没有成功设置信号
            ngx_log_error_core(NGX_LOG_EMERG, errno, "sigaction(%s) failed", sig->signame);
            return -1;
        } else {
            ngx_log_stderr(0, "sigaction(%s) succeed!", sig->signame);
        }

    } // end for
    return 0;
}

/**
 * @brief 
 * 
 * @param signo 
 * @param siginfo siginfo->si_pid = 发送该信号的进程id
 * @param ucontext 
 */
static void ngx_signal_handler(int signo, siginfo_t* siginfo, void* ucontext)
{
    printf("萨尼铁塔！\n");

    ngx_signal_t* sig;
    char* action;

    // 找到了对应的信号，即可处理
    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    action = (char*)"";

    if (ngx_process == NGX_PROCESS_MASTER) {
        switch (signo) {
        case SIGCHLD: // 一般是子进程退出
            ngx_reap = 1;
            break;
        /* 其他信号 */
        default:
            break;
        } // end swtich
    } else if (ngx_process == NGX_PROCESS_WORKER) {
        // worker 的信号处理，后续再说
    } else {
        // 这种情况 先啥也不干
    }

    // 信号来了，记录日志
    if (signo && siginfo->si_pid) {
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "signal %d (%s) received from %P%s",
            signo, sig->signame, siginfo->si_pid, action);
    } else {
        /* 如果没有发送该信号的进程id，那么就不显示 */
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "signal %d (%s) received %s",
            signo, sig->signame, action);
    }

    /* 这里以后还是可以扩展的 */

    if (signo == SIGCHLD) {
        ngx_process_get_status(); // 获取子进程结束的状态
    }
    return;
}

/**
 * @brief 
 * 
 */
static void ngx_process_get_status(void)
{
    int one = 0; // 标记信号正常处理过一次

    // 杀死子进程的时候，父进程会收到sigchld信号
    for (;;) {
        int status;
        // arg1 = -1，表示等待任何子进程
        // arg2 返回 子进程状态
        // arg3 WNOHANG no hang，不要阻塞
        // return 成功，返回子进程id
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) { // 子进程还没有结束
            return;
        }

        if (pid == -1) {
            // 说明这个waitpid调用有误
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            if (err == ECHILD && one) { // 没有子进程
                return;
            }
            if (err == ECHILD) {
                ngx_log_error_core(NGX_LOG_INFO, err, "waitpid() failed!");
                return;
            }
            ngx_log_error_core(NGX_LOG_ALERT, err, "waitpid() failed!");
            return;
        }

        // 走到这里，表明【waitpid返回进程id】
        one = 1; // 标记waitpid()返回了正常的返回值
        if (WTERMSIG(status)) { // 获取使 子进程 终止 的 信号编号
            ngx_log_error_core(NGX_LOG_ALERT, 0, "pid = %P exited on signal %d!", pid, WTERMSIG(status));
        } else {
            ngx_log_error_core(NGX_LOG_NOTICE, 0, "pid = %P exited with code %d!", pid, WEXITSTATUS(status));
        }
    } // end for
    return;
}