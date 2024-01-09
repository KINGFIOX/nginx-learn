#include "ngx_c_conf.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[])
{
    // 这个好像是 饿汉模式
    CConfig* p_config = CConfig::GetInstance();
    if (p_config->Load("./nginx.conf") == false) {
        printf("配置文件载入失败，退出!\n");
        exit(1);
    }
    int port = p_config->GetIntDefault("ListenPort", 7890);
    printf("%d\n", port);

    for (;;) {
        sleep(1);
        printf("休息1s\n");
    }

    return 0;
}