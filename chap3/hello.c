#include <stdio.h>
#include <unistd.h>

int main(int argc, const char* argv[])
{
    printf("hello");

    // 信号处理程序
    // sig_ign表示：我要求忽略这个信号
    // 请操作系统不要用缺省的处理方式来对待我
    // signal(SIGHUP, SIG_IGN);

    pid_t pid = fork();
    if (pid < 0) {
        printf("fork()进程出错!\n");
    } else if (pid == 0) {
        // 子进程
        printf("子进程开始执行\n");
        setsid();
        for (;;) {
            sleep(1);
            printf("子进程休息1s");
        }
    } else {
        // 父进程，得到子进程的pid
        printf("子进程开始执行\n");
        setsid();
        for (;;) {
            sleep(1);
            printf("父进程休息1s");
        }
    }
    printf("goodbye");
    return 0;
}