#include <asm-generic/errno-base.h>
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
    m_worker_connections = 1;
    m_ListenPortCount = 1; // 监听一个端口

    m_epollhandle = -1; // epoll返回的句柄
    m_pconnections = NULL; // 连接池，先置空
    m_pfree_connections = NULL; // 连接池中空闲的连接链
    // m_pread_events = NULL;
    // m_pwrite_events = NULL;

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

/**
 * @brief 从配置中读取 要监听的端口，并添加到监听队列中
 * 
 * @return true 
 * @return false 
 */
bool CSocket::ngx_open_listening_sockets()
{

    /* 读取配置：监听端口的数量 */
    CConfig* p_config = CConfig::GetInstance();
    this->m_ListenPortCount = p_config->GetIntDefault("ListenPortCount", m_ListenPortCount);

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

        if (setnonblocking(isock) == false) {
            ngx_log_stderr(errno, "CSocket::ngx_open_listening_sockets()中setnonblocking()失败, i=%d", i);
            close(isock);
            return false;
        }

        char strinfo[100]; // 临时字符串
        // 默认端口是10000
        strinfo[0] = 0;
        // 这里的 strinfo，就是：ListenPort1、ListenPort2 了
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

        // 添加到 监听队列中
        lpngx_listening_t p_listensocketitem = new ngx_listening_t;
        memset(p_listensocketitem, 0, sizeof(ngx_listening_t));
        p_listensocketitem->port = iport;
        p_listensocketitem->fd = isock;
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听%d端口成功!", iport);
        this->m_ListenSocketList.push_back(p_listensocketitem);

    } // end for (int i = 0; i < m_ListenPortCount; i++)

    // 如果一个端口都不监听，那太搞了
    if (m_ListenSocketList.size() <= 0) {
        return false;
    }
    return true;
}

/**
 * @brief 把 sockfd 对应的 socket 连接设置为非阻塞模式
 * 
 * @param sockfd 
 * @return true 
 * @return false 
 */
bool CSocket::setnonblocking(int sockfd)
{
    int nb = 1; // 0清除，1设置
    if (ioctl(sockfd, FIONBIO, &nb) == -1) {
        return false;
    }
    // 也有人会用fcntl设置 非阻塞IO 标记
    return true;
}

// 关闭 监听 的 socket
void CSocket::ngx_close_listening_sockets()
{
    /* 来一点新特性 */
    for (auto v : m_ListenSocketList) {
        close(v->fd);
        ngx_log_error_core(NGX_LOG_INFO, 0, "关闭监听端口%d!", v->port);
    }
    return;
}

// 打开对应端口的监听 socket
bool CSocket::Initialize()
{
    ReadConf();
    bool reco = ngx_open_listening_sockets();
    return reco;
}

// 读取配置文件
void CSocket::ReadConf()
{
    CConfig* p_config = CConfig::GetInstance();

    // 表示 worker 进程可以打开的 最大连接数，如果在配置文件中没有定义，那么默认是1
    this->m_worker_connections = p_config->GetIntDefault("worker_connections", m_worker_connections);

    // 监听几个端口
    this->m_ListenPortCount = p_config->GetIntDefault("ListenPortCount", m_ListenPortCount);
    return;
}

/**
 * @brief 初始化epoll，创建连接池，将监听fd 一直保存在连接池中
 * 
 * @return int 
 */
int CSocket::ngx_epoll_init()
{
    // ---（1）创建 epoll，返回一个句柄 epoll_fd
    this->m_epollhandle = epoll_create(m_worker_connections);
    if (m_epollhandle == -1) {
        ngx_log_stderr(errno, "CSocket::ngx_epoll_init()中epoll_create()失败.");
        exit(2); // 致命错误，直接退
    }

    // ---（2）创建一个 连接池 数组，并初始化（用于处理所有客户端的连接）
    this->m_connection_n = this->m_worker_connections; // 连接池大小
    this->m_pconnections = new ngx_connection_t[this->m_connection_n]; // 创建连接池

    int i = this->m_connection_n;
    lpngx_connection_t c = this->m_pconnections;
    lpngx_connection_t next = NULL; // c[i-1].next == c[i]
    do {
        i--;
        c[i].data = next;
        c[i].fd = -1;
        c[i].instance = 1; // 1失效
        c[i].iCurrsequence = 0; // 当前序号统一从0开始
        next = &c[i];
    } while (i); // 循环结束时: next = &c[0]
    this->m_pfree_connections = next;
    this->m_free_connection_n = m_connection_n;

    // ---（3）将监听连接一直保存在 连接池 中
    for (auto v : m_ListenSocketList) {
        lpngx_connection_t c = ngx_get_connection(v->fd); // 从连接池中获取一个空闲连接对象
        if (c == NULL) {
            ngx_log_stderr(errno, "const char *fmt, ...");
            exit(2); // 致命错误，直接退出
        }

        /* 连接对象 与 监听对象 关联 */
        c->listening = v;
        v->connection = c;

        c->rhandler = &CSocket::ngx_event_accept; /* 对于 listenfd，那么他的处理函数设置为 添加新连接 */

        // 往红黑树中 添加 敏感事件
        if (ngx_epoll_add_event(v->fd, 1, 0, 0, EPOLL_CTL_ADD, c) == -1) {
            exit(2);
        }
    } // end for

    return 1;
}

/**
 * @brief 对红黑树操作
 * 
 * @param fd 添加到红黑树上 敏感事件 的 socket fd
 * @param readevent 是否对 读事件 敏感
 * @param writeevent 是否对 写事件 敏感
 * @param otherflag // ET 还是 LT ？
 * @param eventtype  对红黑树的三种操作：ADD MOD DEL
 * @param c 连接信息
 * @return int 
 */
int CSocket::ngx_epoll_add_event(int fd, int readevent, int writeevent, uint32_t otherflag, uint32_t eventtype, lpngx_connection_t c)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    if (readevent == 1) {
        ev.events = EPOLLIN | EPOLLRDHUP; // EPOLLIN 读事件
    } else {
        // 其他事件类型待处理
    }

    /* 设置 ET触发 还是 LT触发 */
    if (otherflag != 0) {
        ev.events |= otherflag;
    }

    // TODO
    /* 这里的 ev.data 只会表现ptr的特征，union，这个ev会在epoll_wait的时候 收回片头 */
    ev.data.ptr = (void*)((uintptr_t)c | c->instance);

    //
    if (epoll_ctl(this->m_epollhandle, eventtype, fd, &ev)) {
        ngx_log_stderr(errno, "const char *fmt, ...");
        return -1;
    }
    return 1;
}

/**
 * @brief 
 * 
 * @param timer 超时
 * @return int 
 */
int CSocket::ngx_epoll_process_events(int timer)
{
    int events = epoll_wait(this->m_epollhandle, this->m_events, NGX_MAX_EVENTS, timer);
    if (events == -1) {
        if (errno == EINTR) {
            /* 如果是信号导致的中断 */
            ngx_log_error_core(NGX_LOG_INFO, errno, "CSocket::ngx_epoll_process_events()中epoll_wait()失败!");
            return 1;
        } else {
            ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocket::ngx_epoll_process_events()中epoll_wait()失败!");
            return 0;
        }
    }

    if (events == 0) { /* 超时，但是没事件来 */
        if (timer != -1) {
            return 1;
        }
        /* 超时了，但是timer == -1 */
        ngx_log_error_core(NGX_LOG_ALERT, 0, "CSocket::ngx_epoll_process_events()中epoll_wait()没超时却没返回任何事件!");
        return 0;
    }

    for (int i = 0; i < events; ++i) {

        lpngx_connection_t c = (lpngx_connection_t)(this->m_events[i].data.ptr);
        uintptr_t instance = (uintptr_t)c & 1;
        c = (lpngx_connection_t)((uintptr_t)c & ~((uintptr_t)1));

        /* 过期事件处理 */
        if (c->fd == -1) {
            /* 比方说epoll_wait取得三个事件:
				处理第一个事件的时候，因为业务需要，我们把这个连接关闭，
				那么1. 连接池回收节点 2. close(fd) 3. fd = -1
				第二个事件照常处理
				第三个事件，假如这第三个事件，也是在第一个事件的socket上发生的，那么这个条件成立;
					然而我们在第一个事件的时候已经关闭了，这个socket已经废了，属于过期事件。
			 */
            ngx_log_error_core(NGX_LOG_DEBUG, 0, "CSocket::ngx_epoll_process_events()中遇到了fd=-1的过期事件:%p.", c);

            /* 跳过过期事件，处理下一个事件 */
            continue;
        }

        /* TODO 过期事件处理 */
        if (c->instance != instance) {
            /* 比如用epoll_wait取得三件事，
					a) 处理第一个事件时，我们把连接关闭（假设socket=50），同时设置c.fd=-1并且调用连接池回收
					b) 处理第二个事件时，恰好第二个事件时建立新连接的事件，调用ngx_get_connection从连接池中取出新连接
					c) 因为a中socket=50被释放了，所以操作系统拿来复用; 这里 c新连接，不仅复用了连接（连接池），也复用socket
					d) 这里过期的含义就是：这块内存 以及 socket已经只是原本的东西了。
				这里 instance 是在 有新连接的时候，进行了翻转，判断：你已经不是原来的你了。但是，万一出现了 空翻 呢？
			*/
            ngx_log_error_core(NGX_LOG_DEBUG, 0, "CSocket::ngx_epoll_process_events()中遇到了instance值改变的过期事件:%p.", c);
            continue;
        }

        /* revents = 获取 epoll 返回的事件类型 */
        uint32_t revents = this->m_events[i].events;

        /* epoll_err 表示发生了错误，epoll_hup 表示发生了 挂起 或者 断开连接 */
        if (revents & (EPOLLERR | EPOLLHUP)) {
            /* 如果发生了错误或者挂起，那么也能处理 读事件 和 写事件 */
            revents |= EPOLLIN | EPOLLOUT;
        }

        /* 检查是否有EPOLLIN， */
        if (revents & EPOLLIN) {
            /* CSocket::ngx_event_accept --> listen */
            /* CSocket::ngx_wait_request_handler --> 非 listen */
            auto p_func = c->rhandler; // 函数成员指针
            (this->*p_func)(c);
        }

        if (revents & EPOLLOUT) {
            // TODO ... 待扩展
            ngx_log_stderr(errno, "1111111111111");
        }

    } // end for (int i = 0; i < events; ++i)
    return 1;
}
