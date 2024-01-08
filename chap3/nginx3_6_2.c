#include <signal.h>
#include <stdio.h>
#include <stdlib.h> //malloc,exit
#include <sys/types.h> // Include the necessary header file for waitpid function
#include <sys/wait.h> // Include the necessary header file for waitpid function
#include <unistd.h> //fork

// 信号处理函数
void sig_usr(int signo)
{
    int status;
    switch (signo) {
    case SIGUSR1:
        printf("收到了SIGUSR1信号，进程id=%d!\n", getpid());
        break;
    case SIGCHLD:
        printf("收到了sigchld信号，进程id=%d!\n", getpid());
        // arg1: -1，表示等待任何子进程
        // arg2: 保存子进程的状态信息
        // arg3: 提供额外的选项，WNOHANG(no hang)表示不受阻塞，让这个waitpid()立即返回
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) { // 子进程还没有结束，会立即返回这个数字
            return;
        }

        if (pid == -1) { // 这表示watipid调用有错误
            return;
        }
        break;
    }
    return; // 走到这里，成功，那么就return吧
}

int main(int argc, char* const* argv)
{
    pid_t pid;

    printf("进程开始执行!\n");

    // 先简单处理一个信号
    if (signal(SIGUSR1, sig_usr) == SIG_ERR) // 系统函数，参数1：是个信号，参数2：是个函数指针，代表一个针对该信号的捕捉处理函数
    {
        printf("无法捕捉SIGUSR1信号!\n");
        exit(1);
    }

    //---------------------------------
    pid = fork(); // 创建一个子进程

    // 要判断子进程是否创建成功
    if (pid < 0) {
        printf("子进程创建失败，很遗憾!\n");
        exit(1);
    }

    // 现在，父进程和子进程同时开始 运行了
    for (;;) {
        sleep(1); // 休息1秒
        printf("休息1秒，进程id=%d!\n", getpid());
    }
    printf("再见了!\n");
    return 0;
}
