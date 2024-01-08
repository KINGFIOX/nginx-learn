#include <bits/types/sigset_t.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号处理函数
void sig_quit(int signo)
{
    printf("收到了SIGQUIT信号!\n");
    // 这里是重新设置成了缺省处理，也就是第二次 ^\ 的时候就变成了杀死
    // if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
    //     printf("无法为SIGQUIT信号设置缺省处理(终止进程)!\n");
    //     exit(1);
    // }
}

int main(int argc, const char* argv[])
{
    // pendmask被挂起的信号集
    sigset_t newmask, oldmask, pendmask;

    // 可以通过ctrl + \  触发，这一步是注册
    if (signal(SIGQUIT, sig_quit) == SIG_ERR) {
        printf("无法捕捉sigusr1信号");
        exit(1);
    }

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGQUIT);

    int* psigset = (int*)&newmask;
    for (int i = 0; i < 16 * 2; i++) {
        printf("%d\t", psigset[i]);
    }

    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask)) {
        printf("sigprocmask(SIG_BLOCK)失败!\n");
        exit(1);
    }

    printf("sizeof(long) = %ld\n", sizeof(long));
    printf("sizeof(sigset_t) = %ld\n", sizeof(newmask));

    printf("我要开始休息10s了 --- begin ---，此时我无法接受sigquit信号!\n");
    sleep(10);
    printf("我已经休息了10s了 --- end ---!\n");

    if (sigismember(&newmask, SIGQUIT)) {
        printf("SIGQUIT 信号被屏蔽了!\n");
    } else {
        printf("sigquit信号没有被屏蔽!!!!!!\n");
    }

    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        printf("sigprocmask(SIG_SETMASK)失败!\n");
        exit(1);
    }

    printf("sigprocmask(SIG_SETMASK)成功!\n");

    if (sigismember(&oldmask, SIGQUIT)) {
        printf("SIGQUIT信号被屏蔽了!\n");
    } else {
        printf("SIGQUIT信号屏蔽没有成功，你可以发送sigquit信号了，我要sleep(10)!!\n");
        int mysl = sleep(10);
        if (mysl > 0) {
            printf("sleep还没睡够，剩余%d秒\n", mysl);
        }
    }
    printf("再见");
    return 0;
}