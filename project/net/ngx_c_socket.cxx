#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
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
    m_ListenPortCount = 1; // 监听一个端口
    return;
}

CSocket::~CSocket()
{
    /* TODO 来一点新特性 */
    for (auto pos : m_ListenSocketList) {
        delete pos;
    } // end for
    m_ListenSocketList.clear();
    return;
}

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

        // TODO 没懂为什么要这么操作
        strinfo[0] = 0;
        sprintf(strinfo, "ListenPort%d", i);
        int iport = p_config->GetIntDefault(strinfo, 10000);
        serv_addr.sin_port = htons((in_port_t)iport);

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

        lpngx_listening_t p_listensocketitem = new ngx_listening_t; // p_listen_socket_item
        memset(p_listensocketitem, 0, sizeof(ngx_listening_t));
        p_listensocketitem->port = iport;
        p_listensocketitem->fd = isock;
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听%d端口成功!", iport);
        m_ListenSocketList.push_back(p_listensocketitem);
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