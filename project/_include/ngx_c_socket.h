#ifndef __NGX_C_SOCKET_H__
#define __NGX_C_SOCKET_H__

#include <vector>

#define NGX_LISTEN_BACKLOG 511 /* 已完成队列 */

typedef struct ngx_listening_s {
    int port; // 监听的端口号
    int fd; // socket句柄
} ngx_listening_t, *lpngx_listening_t;

class CSocket {
public:
    CSocket();
    virtual ~CSocket();

public:
    virtual bool Initialize();

private:
    // 监听必须的端口，支持多端口
    bool ngx_open_listening_sockets();

    // 关闭监听socket
    void ngx_close_listening_sockets();

    // 设置非阻塞socket
    bool setnonblocking(int sockfd);

private:
    int m_ListenPortCount; // 监听的端口数量
    std::vector<lpngx_listening_t> m_ListenSocketList; // 监听socket队列
};

#endif