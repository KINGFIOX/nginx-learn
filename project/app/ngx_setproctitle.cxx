#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "ngx_global.h"

void ngx_init_setproctitle()
{
    // 环境变量搬家
    for (int i = 0; environ[i]; i++) {
        g_environlen += strlen(environ[i]) + 1; // 最后这个 +1 是 \0
    }

    gp_envmem = new char[g_environlen];

    char* ptmp = gp_envmem;

    for (int i = 0; environ[i]; i++) {
        size_t size = strlen(environ[i]) + 1;
        strcpy(ptmp, environ[i]);
        environ[i] = ptmp; // 改变环境变量的指向
        ptmp += size;
    }

    return;
}

// 设置新标题，需要title后面有一个\0
void ngx_setproctitle(const char* title)
{
    // 我们假设所有的命令行参数都不用了
    size_t ititlelen = strlen(title);

    size_t e_environlen = 0; // e表示局部变量
    for (int i = 0; g_os_argv[i]; i++) {
        e_environlen += strlen(g_os_argv[i]) + 1;
    }

    size_t esy = e_environlen + g_environlen; // argv和environment内存综合

    if (esy <= ititlelen) {
        // 设置的标题太长了
        return;
    }

    // 设置后续的命令行参数为NULL，相当于只有一个参数了
    // 比方说上面的 for 循环结束条件就是判断：g_os_argv[i]是否为NULL
    g_os_argv[1] = NULL;

    char* ptmp = g_os_argv[0];
    strcpy(ptmp, title);
    ptmp += ititlelen;

    // 把标题后面的元素 清0（防止ps -ef出现垃圾信息）
    size_t cha = esy - ititlelen;
    memset(ptmp, 0, cha);
    return;
}
