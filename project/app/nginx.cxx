#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

char** g_os_argv = NULL;
char* gp_envmem = NULL;
int g_environlen = 0;
pid_t ngx_pid;

int main(int argc, char* argv[])
{
    // （1）无伤大雅也不需要释放的放最上面
    int exitcode = 0;
    ngx_pid = getpid();
    g_os_argv = (char**)argv;

    // （2）初始化失败就要退出的
    // 这个好像是 饿汉模式
    CConfig* p_config = CConfig::GetInstance();
    if (p_config->Load("./nginx.conf") == false) {
        ngx_log_stderr(0, "配置文件[%s]载入失败，退出！", "nginx.conf");
        exitcode = 2;
        goto lblexit;
    }

    // （3）一些初始化函数，准备放这里
    ngx_log_init();
    if (ngx_init_signals() != 0) {
        exitcode = 1;
        goto lblexit;
    }

    // （4）不会归类的
    ngx_init_setproctitle();
    ngx_setproctitle("nginx: master");

    // （5）开始正式的主工作流程，主流程一致再下边这个函数里循环，暂时不会走下来，资源释放啥的日后再慢慢完善和考虑
    ngx_master_process_cycle();

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

    /*
        if (gp_envmem) {
            delete[] gp_envmem;
            gp_envmem = NULL;
        }
    */
    printf("程序退出，再见！\n");

lblexit:
    // freeresource();
    printf("程序退出，再见！\n");
    return 0;
}
