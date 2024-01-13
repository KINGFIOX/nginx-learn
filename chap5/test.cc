#include <cstdio>
#include <iostream>
#include <sys/epoll.h>

using namespace std;

int main(int argc, char* argv[])
{
    struct epoll_event ev;
    printf("event = %p\n", &ev.events);
    printf("data = %p\n", &ev.data);
    cout << sizeof(struct epoll_event) << endl;
    cout << sizeof(epoll_data_t) << endl;
}