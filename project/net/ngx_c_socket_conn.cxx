#include <arpa/inet.h>
#include <cstdint>
#include <errno.h> // errno
#include <fcntl.h> // open
#include <stdarg.h> // va_start
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // uintptr_t
#include <sys/ioctl.h> // ioctl
#include <sys/time.h> // localtime_r
#include <unistd.h> // STDERR_FILENO

#include "ngx_c_conf.h"
#include "ngx_c_memory.h"
#include "ngx_c_socket.h"
#include "ngx_comm.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

/**
 * @brief 释放连接，空闲链表头插
 * 
 * @param c 
 */
void CSocket::ngx_free_connection(lpngx_connection_t c)
{
    if (c->ifnewrecvMem == true) {
        /* TODO 获取单例，释放 */
        CMemory::GetInstance()->FreeMemory(c->pnewMemPointer);
        c->pnewMemPointer = NULL;
        c->ifnewrecvMem = false;
    }

    /* c.data = m_pfree_connections */
    c->data = this->m_pfree_connections;

    /* 序列号++，用于判断网络事件是否过期 */
    ++c->iCurrsequence;

    /* 空闲链表 头插 */
    this->m_pfree_connections = c;
    ++this->m_free_connection_n;
    return;
}

/**
 * @brief 获取连接，空闲链表头删
 * 
 * @param isock 
 * @return lpngx_connection_t 
 */
lpngx_connection_t CSocket::ngx_get_connection(int isock)
{
    /* 空闲链表表头 */
    lpngx_connection_t c = this->m_pfree_connections;
    if (c == NULL) {
        ngx_log_stderr(0, "CSocket::ngx_get_connection()中空闲链表为空!");
        return NULL;
    }

    /* 把空闲链表表头 设置为 c.next */
    this->m_pfree_connections = c->data;
    --this->m_free_connection_n;

    /* using uintptr_t = unsigned long int */
    uintptr_t instance = c->instance; // 看是否有效
    uint64_t iCurrsequence = c->iCurrsequence;

    memset(c, 0, sizeof(ngx_connection_t));
    c->fd = isock;
    c->curStat = _PKG_HD_INIT; /* 收包状态 初始状态 */
    c->precvbuf = c->dataHeadInfo; /* buffer 头指针 */
    c->irecvlen = this->m_iLenPkgHeader; /* 包头长度 */
    c->ifnewrecvMem = false;
    c->pnewMemPointer = NULL;

    c->instance = !instance; // instance 翻转
    c->iCurrsequence = iCurrsequence;
    ++c->iCurrsequence;

    return c;
}

/**
 * @brief 回收连接池中的连接，不然连接池很快就耗光了
 * 
 * @param c 
 */
void CSocket::ngx_close_connection(lpngx_connection_t c)
{
    if (close(c->fd) == -1) {
        ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocket::ngx_close_accepted_connection()中close(%d)失败!", c->fd);
    }
    c->fd = -1; // 清空c.fd
    ngx_free_connection(c); // 从 连接池 中释放连接
    return;
}
