#ifndef __NGX_GLOBAL_H__
#define __NGX_GLOBAL_H__

#include <unistd.h>

// 配置文件读取 ---------------
typedef struct {
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

// 进程标题 ------------------
extern char** g_os_argv;
extern char* gp_envmem;
extern int g_environlen;
void ngx_init_setproctitle();
void ngx_setproctitle(const char* title);

// 全局变量
extern pid_t ngx_pid;

// 与日志相关 ----------------
typedef struct {
    int log_level; // 日志级别
    int fd; // 文件描述符
} ngx_log_t;

#endif