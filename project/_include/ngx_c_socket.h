#ifndef __NGX_C_SOCKET_H__
#define __NGX_C_SOCKET_H__

#include "ngx_comm.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <list>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define NGX_LISTEN_BACKLOG 511 /* 已完成队列 */
#define NGX_MAX_EVENTS 512 /* epoll 一次最多接收这么多事件 */

// 先取一些类型别名 ------------------

/* 与监听队列有关的 */
typedef struct ngx_listening_s ngx_listening_t, *lpngx_listening_t;

/* 与连接池有关的 */
typedef struct ngx_connection_s ngx_connection_t, *lpngx_connection_t;
typedef class CSocket CSocket;

// 定义成员函数指针
typedef void (CSocket::*ngx_event_handler_pt)(lpngx_connection_t c);

/**
 * @brief 监听相关的信息
 * @port 监听的端口号
 * @fd 监听的socket句柄
 */
struct ngx_listening_s {
    int port; // 监听的端口号
    int fd; // socket句柄
    lpngx_connection_t connection; // 连接池的一项连接
};

/**
 * @brief 连接池中，元素的结构体，保存着一些数据。
			有了连接池的存在，这段内存就不用重新开辟
 * @listening 被谁监听的
 */
struct ngx_connection_s {
    int fd;
    lpngx_listening_t listening;

    // -------------
    unsigned instance : 1; // 位域：0有效，1失效
    uint64_t iCurrsequence; // 时间戳：分配的时候++、释放的时候++
    struct sockaddr s_sockaddr;
    // char addr_text[100]; // 地址的文本信息

    // ------------
    // uint8_t r_ready; // 读准备好的标记
    uint8_t w_ready; // 写准备好的标记

    // ---------------
    ngx_event_handler_pt rhandler; // 读的相关处理方法
    ngx_event_handler_pt whandler; // 写的相关处理方法

    // 和收包有关 -------------
    unsigned char curStat;
    char dataHeadInfo[_DATA_BUFSIZE_];
    char* precvbuf;
    unsigned int irecvlen; // 指示收多少个字节

    bool ifnewrecvMem;
    char* pnewMemPointer;

    // --------------
    lpngx_connection_t data; /* 相当于是 next，空闲链 */
};

/*
typedef struct ngx_event_s {
    每个tcp连接至少需要 一个读事件 和 一个写事件，所以定义事件结构
} ngx_event_t, *lpngx_event_t;
*/

/**
 * @brief 消息头
 * @pConn 记录对应的连接（连接池）
 * @iCurrsequence 时间戳
 */
typedef struct _STRUC_MSG_HEADER {
    lpngx_connection_t pConn; // 连接（连接池）
    uint64_t iCurrsequence; // 时间戳
} STRUC_MSG_HEADER, *LPSTRUC_MSG_HEADER;

class CSocket {
public:
    CSocket();
    virtual ~CSocket();

public:
    virtual bool Initialize();

public:
    int ngx_epoll_init(); /*  */
    // void ngx_epoll_listenportstart();  // 监听端口开始工作
    int ngx_epoll_add_event(int fd, int readevent, int writeevent, uint32_t otherflag, uint32_t eventtype, lpngx_connection_t c); /* epoll增加事件 */
    int ngx_epoll_process_events(int timer); /* epoll等待接收和处理事件 */

private:
    void ReadConf(); /* 读配置 */
    bool ngx_open_listening_sockets(); /* 为每个port创建listenfd socket，加入到监听队列中 */
    void ngx_close_listening_sockets(); /* 关闭监听socket */
    bool setnonblocking(int sockfd); /* 设置非阻塞socket */

    // ---------
    void ngx_event_accept(lpngx_connection_t oldc); /*  */
    void ngx_wait_request_handler(lpngx_connection_t c); /*  */
    void ngx_close_connection(lpngx_connection_t c);

    // ---------
    ssize_t recvproc(lpngx_connection_t c, char* buff, ssize_t buflen); /*  */
    void ngx_wait_request_handler_proc_p1(lpngx_connection_t c); /*  */
    void ngx_wait_request_handler_proc_plast(lpngx_connection_t c); /*  */
    void inMsgRecvQueue(char* buf); /*  */
    void tmpoutMsgRecvQueue(void);
    void clearMsgRecvQueue(); /* 各种清理函数 */

    // ---------
    size_t ngx_sock_ntop(struct sockaddr* sa, int port, u_char* text, size_t len);

    // ---------
    lpngx_connection_t ngx_get_connection(int isock); /* 连接池相关操作 */
    void ngx_free_connection(lpngx_connection_t c); /*  */

private:
    /* 连接池相关 */
    int m_epollhandle; /* epoll的句柄 */
    int m_worker_connections; /* 每个worker最多连接数  */
    int m_ListenPortCount; // 监听的端口数量

    lpngx_connection_t m_pconnections;
    lpngx_connection_t m_pfree_connections; /* 连接池中，下一个空闲的位置 */

    // lpngx_event_t m_pread_events;
    // lpngx_event_t m_pwrite_events;

    int m_connection_n; /* 连接池的大小 */
    int m_free_connection_n; /* 连接池中有多少位置是空闲的 */

    std::vector<lpngx_listening_t> m_ListenSocketList; // 监听socket队列
    struct epoll_event m_events[NGX_MAX_EVENTS];

    size_t m_iLenPkgHeader; /* 包头的长度 */
    size_t m_iLenMsgHeader; /* 消息头的长度 */

    // 消息队列
    std::list<char*> m_MsgRecvQueue;
};

#endif