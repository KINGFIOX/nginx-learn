#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int g_mysign = 0;

// 能够修改全局变量
void muNEfunc(int value)
{
    // ...
    g_mysign = value;
    // ...
}

void sig_usr(int sign)
{
    int tmpsign = g_mysign;

    // 因为一些操蛋的实际需求，需要修改这个全局变量
    muNEfunc(22);
    if (sign == SIGUSR1) { // SIGUSR1是自定义信号
        printf("收到了SIGUSR1信号!\n");
    } else if (sign == SIGUSR1) {
        printf("收到了SIGUSR1信号!\n");
    } else {
        printf("受到了未捕捉的信号%d\n", sign);
    }

    g_mysign = tmpsign;
}

int main(int argc, const char* argv[])
{
    // 系统函数
    // arg1 信号
    // arg2 是一个函数指针，信号处理函数
    if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
        printf("无法捕捉SIGUSR1信号!\n");
    }
    if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
        printf("无法捕捉SIGUSR1信号!\n");
    }
    for (;;) {
        sleep(1);
        printf("休息1s\n");
        muNEfunc(15);
        printf("g_mysign = %d\n", g_mysign);
    }
    printf("再见!\n");
}