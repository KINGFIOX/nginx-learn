#include <asm-generic/errno.h>
#include <errno.h> //errno
#include <fcntl.h> //open
#include <netinet/in.h>
#include <stdarg.h> //va_start....
#include <stdint.h> //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> //gettimeofday
#include <time.h> //localtime_r
#include <unistd.h> //STDERR_FILENO等
//#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> //ioctl

#include "ngx_c_conf.h"
#include "ngx_c_memory.h"
#include "ngx_c_socket.h"
#include "ngx_comm.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

/**
 * @brief 来数据处理的时候，当链接上有数据来的时候，本函数会被ngx_epoll_process_events()调用
 * 
 * @param c 
 */
void CSocket::ngx_wait_request_handler(lpngx_connection_t c)
{
    /* reco = recvproc 返回的字节数 */
    ssize_t reco = recvproc(c, c->precvbuf, c->irecvlen);
    if (reco <= 0) {
        return;
    }
    switch (c->curStat) {
    case _PKG_HD_INIT:
        if (reco == this->m_iLenPkgHeader) {
            ngx_wait_request_handler_proc_p1(c);
        } else {
            c->curStat = _PKG_BD_RECVING;
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco; // 要接收的字节数减少
        }
        break;
    case _PKG_HD_RECVING:
        if (c->irecvlen == reco) {
            ngx_wait_request_handler_proc_p1(c);
        } else {
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
        break;
    case _PKG_BD_INIT:
        if (reco == c->irecvlen) {
            ngx_wait_request_handler_proc_plast(c);
        } else {
            c->curStat = _PKG_BD_RECVING;
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
        break;
    case _PKG_BD_RECVING:
        if (c->irecvlen == reco) {
            ngx_wait_request_handler_proc_plast(c);
        } else {
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
        break;
    }
    return;
}

/**
 * @brief 接收数据专用函数
 * 
 * @param c 连接（连接池）
 * @param buff 缓冲区
 * @param buflen 缓冲区长度
 * @return ssize_t 
 */
ssize_t CSocket::recvproc(lpngx_connection_t c, char* buff, ssize_t buflen)
{
    ssize_t n = recv(c->fd, buff, buflen, 0); /*  */
    if (n == 0) {
        ngx_close_connection(c);
        return -1;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ngx_log_stderr(errno, "CSocket::recvproc()中errno == EAGAIN || errno == EWOULDBLOCK成立，出乎意料!");
            return -1;
        }
        if (errno == EINTR) {
            ngx_log_stderr(errno, "CSocket::recvproc()中errno == EINTR成立，出乎意料!");
            return -1;
        }
        if (errno == ECONNRESET) {
        } else {
            ngx_log_stderr(errno, "CSocket::recvproc()中发生错误，我打印出来看看是啥错误");
        }
        ngx_close_connection(c);
        return -1;
    }
    return n;
}

/**
 * @brief 处理 消息头 包头，将 buffer 与 连接（连接池）绑定
 * 
 * @param c 
 */
void CSocket::ngx_wait_request_handler_proc_p1(lpngx_connection_t c)
{
    CMemory* p_memory = CMemory::GetInstance();

    /* 包头指针 */
    LPCOMM_PKG_HEADER pPkgHeader;
    pPkgHeader = (LPCOMM_PKG_HEADER)c->dataHeadInfo;

    /* 获取数据包的长度 */
    unsigned short e_pkgLen;
    e_pkgLen = ntohs(pPkgHeader->pkgLen);

    if (e_pkgLen < m_iLenMsgHeader) {
        /* 如果 整个包的长度 < 包头的长度，过滤了 */
        c->curStat = _PKG_HD_INIT; // header init
        c->precvbuf = c->dataHeadInfo;
        c->irecvlen = this->m_iLenPkgHeader;
    } else if (e_pkgLen > (_PKG_MAX_LENGTH - 1000)) {
        /* 太大了，恶意包，过滤 */
        c->curStat = _PKG_HD_INIT; // header init
        c->precvbuf = c->dataHeadInfo;
        c->irecvlen = this->m_iLenPkgHeader;
    } else {
        char* pTmpBuffer = (char*)p_memory->AllocMemory(this->m_iLenMsgHeader + e_pkgLen, false); /* 分配 包体 + 消息头 的长度 */
        c->ifnewrecvMem = true; // 标记我们new了内存
        c->pnewMemPointer = pTmpBuffer; // 将buffer绑定到c

        // a) 填写消息头
        LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn = c; // 与连接绑定
        ptmpMsgHeader->iCurrsequence = c->iCurrsequence;

        // b) 填写包头
        pTmpBuffer += this->m_iLenMsgHeader; // 跳过 消息头
        memcpy(pTmpBuffer, pPkgHeader, this->m_iLenPkgHeader); // 拷贝
        if (e_pkgLen == this->m_iLenPkgHeader) {
            ngx_wait_request_handler_proc_plast(c);
        } else {
            c->curStat = _PKG_BD_INIT;
            c->precvbuf = pTmpBuffer + this->m_iLenPkgHeader;
            c->irecvlen = e_pkgLen - this->m_iLenPkgHeader;
        }
    } // end if (e_pkgLen < m_iLenMsgHeader)
}

/**
 * @brief 将buffer从连接池中解绑定，将buffer加入消息队列中
 * 
 * @param c 
 */
void CSocket::ngx_wait_request_handler_proc_plast(lpngx_connection_t c)
{
    inMsgRecvQueue(c->pnewMemPointer);

    c->ifnewrecvMem = false;
    c->pnewMemPointer = NULL;
    c->curStat = _PKG_HD_INIT;
    c->precvbuf = c->dataHeadInfo;
    c->irecvlen = this->m_iLenPkgHeader;
    return;
}

/**
 * @brief 当收到一个完整包之后，将完整包入消息队列
 * 
 * @param buf 
 */
void CSocket::inMsgRecvQueue(char* buf)
{
    this->m_MsgRecvQueue.push_back(buf);
    tmpoutMsgRecvQueue();
    ngx_log_stderr(0, "非常好，收到了一个完整的数据包【包头+包体】!");
}

/**
 * @brief 杀死前500条消息
 * 
 */
void CSocket::tmpoutMsgRecvQueue(void)
{
    if (this->m_MsgRecvQueue.empty()) {
        return;
    }
    int size = this->m_MsgRecvQueue.size();

    if (size < 1000) {
        /* 消息不超过1000条就不处理先 */
        return;
    }

    /* 消息达到1000条 */
    CMemory* p_memory = CMemory::GetInstance();
    int cha = size - 500;
    for (int i = 0; i < cha; ++i) {
        char* sTmpMsgBuf = this->m_MsgRecvQueue.front();
        this->m_MsgRecvQueue.pop_front();
        p_memory->FreeMemory(sTmpMsgBuf);
    }
    return;
}