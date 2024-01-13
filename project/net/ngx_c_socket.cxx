#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

CSocket::CSocket()
{
    m_worker_connections = 1; // epoll连接最大项数
    m_ListenPortCount = 1; // 监听一个端口

    m_epollhandle = -1; // epoll返回的句柄
    m_pconnections = NULL; // 连接池，先置空
    m_pfree_connections = NULL; // 连接池中空闲的连接链

    return;
}

CSocket::~CSocket()
{
    /* TODO 来一点新特性 */
    for (auto pos : m_ListenSocketList) {
        delete pos;
    } // end for
    m_ListenSocketList.clear();

    /* 释放连接池 */
    if (m_pconnections != NULL) {
        delete[] m_pconnections;
    }
    return;
}

// 监听所有的端口
bool CSocket::ngx_open_listening_sockets()
{

    /* 读取配置 */
    CConfig* p_config = CConfig::GetInstance();
    m_ListenPortCount = p_config->GetIntDefault("ListenPortCount", m_ListenPortCount);

    /*  */
    // int isock; // socket
    char strinfo[100]; // 临时字符串

    struct sockaddr_in serv_addr; // 服务器地址结构体
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 为每个port创建socket
    for (int i = 0; i < m_ListenPortCount; i++) {
        int isock = socket(AF_INET, SOCK_STREAM, 0);
        if (isock == -1) {
            ngx_log_stderr(errno, "CSocket::ngx_open_listening_sockets()中socket()失败, i=%d", i);
            // 其实也不应该直接退出，因为可能会有没释放socket的情况，但是程序能走到这一步，直接退出得了
            return false;
        }

        int reuseaddr = 1;
        if (setsockopt(isock, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddr, sizeof(reuseaddr))) {
            ngx_log_stderr(errno, "CSocket::ngx_open_listening_sockets()中setsockopt()失败, i=%d", i);
            close(isock);
            return false;
        }

        // 默认端口是10000
        strinfo[0] = 0;
        sprintf(strinfo, "ListenPort%d", i);
        int iport = p_config->GetIntDefault(strinfo, 10000);
        serv_addr.sin_port = htons((in_port_t)iport); // in_port_t是2个字节，而int是4个字节，强转一次

        // 绑定 socket 与 addr ip
        if (bind(isock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            ngx_log_stderr(errno, "CSocket::ngx_open_listening_sockets()中bind()失败, i=%d", i);
            close(isock);
            return false;
        }

        // 开始监听，设置最大积压数
        if (listen(isock, NGX_LISTEN_BACKLOG) == -1) {
            ngx_log_stderr(errno, "CSocket::ngx_open_listening_sockets()中listen()失败, i=%d", i);
            close(isock);
            return false;
        }

        // p_listen_socket_item
        lpngx_listening_t p_listensocketitem = new ngx_listening_t;
        memset(p_listensocketitem, 0, sizeof(ngx_listening_t));
        p_listensocketitem->port = iport;
        p_listensocketitem->fd = isock;
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听%d端口成功!", iport);
        m_ListenSocketList.push_back(p_listensocketitem);
    } // end for (int i = 0; i < m_ListenPortCount; i++)

    // 如果一个端口都不监听，那太搞了
    if (m_ListenSocketList.size() <= 0) {
        return false;
    }
    return true;
}

// 把socket连接设置为非阻塞模式
bool CSocket::setnonblocking(int sockfd)
{
    int nb = 1; // 0清除，1设置
    if (ioctl(sockfd, FIONBIO, &nb) == -1) {
        return false;
    }
    // 也有人会用fcntl设置 非阻塞IO 标记
    return true;
}

// 关闭socket
void CSocket::ngx_close_listening_sockets()
{
    /* 来一点新特性 */
    for (auto v : m_ListenSocketList) {
        close(v->fd);
        ngx_log_error_core(NGX_LOG_INFO, 0, "关闭监听端口%d!", v->port);
    }
    return;
}

bool CSocket::Initialize()
{
    ReadConf();
    bool reco = ngx_open_listening_sockets();
    return reco;
}

void CSocket::ReadConf()
{
    CConfig* p_config = CConfig::GetInstance();
    // 默认最大连接 项数
    m_worker_connections = p_config->GetIntDefault("worker_connections", m_worker_connections);
    // 监听几个端口
    m_ListenPortCount = p_config->GetIntDefault("ListenPortCount", m_ListenPortCount);
    return;
}

int CSocket::ngx_epoll_init()
{
    // （1）创建 epoll，返回一个句柄
    m_epollhandle = epoll_create(m_worker_connections);
    if (m_epollhandle == -1) {
        ngx_log_stderr(errno, "CSocket::ngx_epoll_init()中epoll_create()失败.");
        exit(2); // 致命错误，直接退
    }

    // （2）创建一个连接池 数组，用于处理所有客户端的连接
    m_connection_n = m_worker_connections; // 记录当前连接池中连接总数
    // 连接池
    m_pconnections = new ngx_connection_t[m_connection_n];

    int i = m_connection_n;
    lpngx_connection_t c = m_pconnections;
    lpngx_connection_t next = NULL; // c[i-1].next == c[i]
    do {
        i--;
        c[i].data = next;
        c[i].fd = -1;
        c[i].instance = 1; // 1失效
        c[i].iCurrsequence = 0; // 当前序号统一从0开始
        next = &c[i];
    } while (i); // 循环结束时: next = &c[0]
    m_pfree_connections = next;
    m_free_connection_n = m_connection_n;

    // （3）遍历所有监听socket，为每个监听 socket 与一段内存绑定
    for (auto v : m_ListenSocketList) {
        c = ngx_get_connection(v->fd); // 从连接池中获取一个空闲连接对象
        if (c == NULL) {
            ngx_log_stderr(errno, "const char *fmt, ...");
            exit(2); // 致命错误，直接退出
        }
        c->listening = v;
        v->connection = c;

        c->rhandler = &CSocket::ngx_event_accept;

        // 往socket上增加监听事件
        if (ngx_epoll_add_event(v->fd, 1, 0, 0, EPOLL_CTL_ADD, c) == -1) {
            exit(2);
        }
    } // end for

    return 1;
}

int CSocket::ngx_epoll_add_event(int fd, int readevent, int writeevent, uint32_t otherflag, uint32_t eventtype, lpngx_connection_t c)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    if (readevent == -1) {
        ev.events = EPOLLIN | EPOLLRDHUP; //
    } else {
        // 其他事件类型待处理
    }

    // TODO
    ev.data.ptr = (void*)((uintptr_t)c | c->instance);

    if (epoll_ctl(m_epollhandle, eventtype, fd, &ev)) {
        ngx_log_stderr(errno, "const char *fmt, ...");
        return -1;
    }
    return 1;
}