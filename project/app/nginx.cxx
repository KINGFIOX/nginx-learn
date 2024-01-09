#include "ngx_c_conf.h"
#include "ngx_global.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

char** g_os_argv = NULL;
char* gp_envmem = NULL;
int g_environlen = 0;

int main(int argc, char* argv[])
{
    g_os_argv = (char**)argv;

    ngx_init_setproctitle();

    ngx_setproctitle("nginx: master");

    // 这个好像是 饿汉模式
    CConfig* p_config = CConfig::GetInstance();
    if (p_config->Load("./nginx.conf") == false) {
        printf("配置文件载入失败，退出!\n");
        exit(1);
    }

    /*
        // 测试读取配置是否成功
        // int port = p_config->GetIntDefault("ListenPort", 7890);
        // printf("%d\n", port);
    */

    /*
        // 测试 environ 是否搬家成功
        for (int i = 0; environ[i]; i++) {
            printf("environ[%d]地址 = %p\t", i, environ[i]);
            printf("environ[%d]内容 = %s\n", i, environ[i]);
        }
        printf("------------------------------------\n");
        ngx_init_setproctitle();
        // 新的地址
        for (int i = 0; environ[i]; i++) {
            printf("environ[%d]地址 = %p\t", i, environ[i]);
            printf("environ[%d]内容 = %s\n", i, environ[i]);
        }
    */

    for (;;) {
        sleep(1);
        printf("休息1s\n");
    }

    if (gp_envmem) {
        delete[] gp_envmem;
        gp_envmem = NULL;
    }
    printf("程序退出，再见！\n");

    return 0;
}