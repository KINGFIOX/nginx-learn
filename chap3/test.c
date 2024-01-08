#include <stdio.h>
#include <unistd.h>

#include <signal.h>

int main(int argc, const char* argv[])
{
    printf("hello");
    // // 子进程
    // for (;;) {
    //     sleep(1);
    //     printf("sleep 1s");
    // }
    printf("sizeof(long) = %ld\n", sizeof(long));
    printf("goodbye\n");
    printf("%ld", sysconf(_SC_CHILD_MAX));
    return 0;
}