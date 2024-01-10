#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

// 初始化全局变量 ----------------
size_t g_argvneedmem = 0;
size_t g_envneedmem = 0;
int g_os_argc;
char** g_os_argv = NULL;
char* gp_envmem = NULL;
int g_daemonized = 0; // 默认不使用守护进程

// 进程相关 -----------
pid_t ngx_pid;
pid_t ngx_parent;
int ngx_process;

// 标记子进程的状态 -------------
sig_atomic_t ngx_reap;

// socket相关 ----------------
CSocket g_socket;

// 集中释放资源
void freeresource();

int main(int argc, char* argv[])
{
    int exitcode = 0;

    // （1）无伤大雅也不需要释放的放最上面
    ngx_pid = getpid();
    ngx_parent = getppid();
    g_argvneedmem = 0;
    for (int i = 0; i < argc; i++) {
        g_argvneedmem += strlen(argv[i]) + 1; // 最后这个+1是'\0'
    }
    for (int i = 0; environ[i]; i++) {
        g_envneedmem += strlen(environ[i]) + 1;
    }

    g_os_argv = (char**)argv;
    g_os_argc = argc;

    // 初始化全局量 ---------------
    ngx_log.fd = -1;
    ngx_process = NGX_PROCESS_MASTER; // 先标记本进程是master进程
    ngx_reap = 0; // 标记子进程有没有发生变化

    // （2）初始化失败就要退出的
    // 这个好像是 饿汉模式
    CConfig* p_config = CConfig::GetInstance();
    if (p_config->Load("./nginx.conf") == false) {
        ngx_log_init(); // 如果失败，也是写入日志中
        ngx_log_stderr(0, "配置文件[%s]载入失败，退出！", "nginx.conf");
        exitcode = 2;
        goto lblexit;
    }

    // （3）一些初始化函数，准备放这里
    ngx_log_init();
    // 初始化信号
    if (ngx_init_signals() != 0) {
        exitcode = 1;
        goto lblexit;
    }
    // 初始化socket
    if (g_socket.Initialize() == false) {
        exitcode = 1;
        goto lblexit;
    }

    // （4）不好归类的
    ngx_init_setproctitle();

    // （5）创建守护进程
    if (p_config->GetIntDefault("Daemon", 0) == -1) {
        int cdaemonresult = ngx_daemon();
        // fork()失败
        if (cdaemonresult == -1) {
            exitcode = 1;
            goto lblexit;
        }
        if (cdaemonresult == 1) {
            // 原始的父进程退出
            freeresource();
            exitcode = 0;
            return exitcode;
        }
        // 只有子进程会走到这里
        g_daemonized = 1;
    }

    // （6）正式开始主工作流程，但是这里目前还没有考虑子进程的释放资源
    ngx_master_process_cycle();

lblexit:
    freeresource();
    ngx_log_stderr(0, "程序退出，再见了!");
    return 0;
}

// 集中释放资源 ---------
void freeresource()
{
    // （1）设置了进程标题，释放分配的空间
    if (gp_envmem) {
        delete[] gp_envmem;
        gp_envmem = NULL;
    }

    // （2）设置了进程标题，释放分配的空间
    if (ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1) {
        // 关闭日志文件
        close(ngx_log.fd);
        ngx_log.fd = -1;
    }
}
