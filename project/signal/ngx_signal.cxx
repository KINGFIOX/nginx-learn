#include <csignal>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <signal.h>

#include "ngx_func.h"
#include "ngx_macro.h"

typedef struct {
    int signo;
    const char* signame;

    // 信号处理函数，函数指针
    void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
} ngx_signal_t;

static void ngx_signal_handler(int signo, siginfo_t* siginfo, void* ucontext)
{
    // Function implementation goes here
    printf("萨尼铁塔！\n");
}

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
