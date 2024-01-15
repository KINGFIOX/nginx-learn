#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAX_SOCKETS 64 // 定义最大连接数
#define SOCKET_FREE 0 // 表示Socket空闲
#define SOCKET_BUSY 1 // 表示Socket正在使用
#define SOCKET_INVALID -1 // 表示Socket无效

// 定义Socket结构体
typedef struct {
    int socket_fd; // Socket文件描述符
    struct sockaddr_in address; // Socket地址
    int status; // 连接状态
} SocketConnection;

// 定义连接池结构体
typedef struct {
    SocketConnection sockets[MAX_SOCKETS]; // 连接数组
    int count; // 当前连接数
    pthread_mutex_t lock; // 互斥锁，用于线程同步
} SocketPool;

// 初始化连接池
void init_socket_pool(SocketPool* pool)
{
    pool->count = 0;
    pthread_mutex_init(&pool->lock, NULL);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        pool->sockets[i].socket_fd = SOCKET_INVALID;
        pool->sockets[i].status = SOCKET_FREE;
        memset(&pool->sockets[i].address, 0, sizeof(pool->sockets[i].address));
    }
}

// 获取一个空闲的Socket连接
int acquire_socket(SocketPool* pool)
{
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (pool->sockets[i].status == SOCKET_FREE) {
            pool->sockets[i].status = SOCKET_BUSY;
            pthread_mutex_unlock(&pool->lock);
            return pool->sockets[i].socket_fd;
        }
    }
    pthread_mutex_unlock(&pool->lock);
    return -1; // 没有空闲的连接
}

// 释放一个Socket连接，将其标记为FREE
void release_socket(SocketPool* pool, int socket_fd)
{
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (pool->sockets[i].socket_fd == socket_fd) {
            pool->sockets[i].status = SOCKET_FREE;
            break;
        }
    }
    pthread_mutex_unlock(&pool->lock);
}

// 创建一个新的Socket连接并将其添加到连接池中
int create_socket(SocketPool* pool, struct sockaddr_in* address)
{
    pthread_mutex_lock(&pool->lock);
    if (pool->count >= MAX_SOCKETS) {
        pthread_mutex_unlock(&pool->lock);
        return -1; // 连接池已满
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        pthread_mutex_unlock(&pool->lock);
        return -1; // 创建Socket失败
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (pool->sockets[i].socket_fd == SOCKET_INVALID) {
            pool->sockets[i].socket_fd = socket_fd;
            pool->sockets[i].address = *address;
            pool->sockets[i].status = SOCKET_FREE;
            pool->count++;
            pthread_mutex_unlock(&pool->lock);
            return socket_fd;
        }
    }
    pthread_mutex_unlock(&pool->lock);
    return -1; // 没有找到合适的位置添加新的Socket
}

// 销毁指定的Socket连接
void destroy_socket(SocketPool* pool, int socket_fd)
{
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (pool->sockets[i].socket_fd == socket_fd) {
            close(pool->sockets[i].socket_fd);
            pool->sockets[i].socket_fd = SOCKET_INVALID;
            pool->sockets[i].status = SOCKET_FREE;
            pool->count--;
            break;
        }
    }
    pthread_mutex_unlock(&pool->lock);
}
