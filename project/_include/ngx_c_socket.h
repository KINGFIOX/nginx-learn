#ifndef __NGX_C_SOCKET_H__
#define __NGX_C_SOCKET_H__

#include <cstddef>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define NGX_LISTEN_BACKLOG 511 /* 已完成队列 */
#define NGX_MAX_EVENTS 512 /* epoll 一次最多接收这么多事件 */

// 先取一些类型别名 ------------------
typedef struct ngx_listening_s ngx_listening_t, *lpngx_listening_t;
typedef struct ngx_connection_s ngx_connection_t, *lpngx_connection_t;
typedef class CSocket CSocket;

// 定义成员函数指针
typedef void (CSocket::*ngx_event_handler_pt)(lpngx_connection_t c);

struct ngx_listening_s {
    int port; // 监听的端口号
    int fd; // socket句柄
    lpngx_connection_t connection; // 连接池的一项连接
};

// 该结构体表示一个tcp连接
/*
    就是我们服务器会打开两个socket，一个是listen，还有一个就是这个客户端连接的socket了
*/
struct ngx_connection_s {
    int fd;
    lpngx_listening_t listening;
    unsigned instance : 1; // 位域：0有效，1失效
    uint64_t iCurrsequence; // 具体用法，用到了再说
    struct sockaddr s_sockaddr;
    // char addr_text[100]; // 地址的文本信息

    // uint8_t r_ready; // 读准备好的标记
    uint8_t w_ready; // 写准备好的标记

    ngx_event_handler_pt rhandler; // 读的相关处理方法
    ngx_event_handler_pt whandler; // 写的相关处理方法

    lpngx_connection_t data; // 静态链表指针，空闲链串起来
};

/*
typedef struct ngx_event_s {
    每个tcp连接至少需要 一个读事件 和 一个写事件，所以定义事件结构
} ngx_event_t, *lpngx_event_t;
*/

class CSocket {
public:
    CSocket();
    virtual ~CSocket();

public:
    virtual bool Initialize();

public:
    int ngx_epoll_init();

    // void ngx_epoll_listenportstart();  // 监听端口开始工作

    // epoll增加事件
    int ngx_epoll_add_event(int fd, int readevent, int writeevent, uint32_t otherflag, uint32_t eventtype, lpngx_connection_t c);

    int epoll_process_events(int timer); // epoll等待接收和处理事件

private:
    // 读配置
    void ReadConf();

    // 监听必须的端口，支持多端口
    bool ngx_open_listening_sockets();

    // 关闭监听socket
    void ngx_close_listening_sockets();

    // 设置非阻塞socket
    bool setnonblocking(int sockfd);

    void ngx_event_accept(lpngx_connection_t oldc);
    void ngx_wait_request_handler(lpngx_connection_t c);

    size_t ngx_sock_ntop(struct sockaddr* sa, int port, u_char* text, size_t len);

    lpngx_connection_t ngx_get_connection(int isock);

    void ngx_free_connection(lpngx_connection_t c);

private:
    int m_epollhandle;
    int m_worker_connections;
    int m_ListenPortCount; // 监听的端口数量

    // 和连接池有关
    lpngx_connection_t m_pconnections;
    lpngx_connection_t m_pfree_connections;

    // lpngx_event_t m_pread_events;
    // lpngx_event_t m_pwrite_events;

    int m_connection_n;
    int m_free_connection_n;

    std::vector<lpngx_listening_t> m_ListenSocketList; // 监听socket队列

    struct epoll_event m_events[NGX_MAX_EVENTS];
};

#endif