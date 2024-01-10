#ifndef __NGX_GLOBAL_H__
#define __NGX_GLOBAL_H__

#include <signal.h>
#include <unistd.h>

// 与日志相关 ----------------
typedef struct {
    int log_level; // 日志级别
    int fd; // 文件描述符
} ngx_log_t;

// 配置文件读取 ---------------
typedef struct {
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

// 进程标题 ------------------
void ngx_init_setproctitle();
void ngx_setproctitle(const char* title);

// 全局变量
extern size_t g_argvneedmem; // argv需要的内存大小
extern size_t g_envneedmem;
extern char** g_os_argv;
extern char* gp_envmem;
extern int g_os_argc; // 参数个数
extern int g_daemonized; // 是否使用守护进程

// 进程相关
extern pid_t ngx_pid;
extern pid_t ngx_parent;
extern ngx_log_t ngx_log;
extern int ngx_process;
extern sig_atomic_t ngx_reap; // 标记子进程有没有发生变化

#endif