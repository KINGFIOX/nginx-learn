# chap5 - comment

## epoll 相关

### 创建 epoll 实例

```cpp
/* Create epoll instance */
int epollfd = epoll_create1(0);
if (epollfd == -1) {
	perror("epoll_create1");
	exit(EXIT_FAILURE);
}
```

王健伟老师的教程中，创建 epoll 使用的是`int epoll_create(int size);`。
然而，这里的`int size`实际上已经失去了原本的含义了，只要是`size > 0`即可

### struct_event

定义

```cpp
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;	/* Epoll events */ // 事件的类型，或者是发生了什么样的事情，比方说 EPOLLIN
  epoll_data_t data;	/* User data variable */ 用来存放 listenfd 的
} __EPOLL_PACKED;
```

他的内存模型如下：

```c
struct epoll_event {
        uint32_t                   events;               /*     0     4 */

        /* XXX 4 bytes hole, try to pack */  // 这里有填充

        epoll_data_t               data;                 /*     8     8 */

        /* size: 16, cachelines: 1, members: 2 */
        /* sum members: 12, holes: 1, sum holes: 4 */
        /* last cacheline: 16 bytes */
};
```

### 存放在 红黑树上 ？ 还是 双向链表上 ？

在 Linux 的 epoll 实现中，文件描述符（在你的代码中是 `listenfd`）被存储在一个红黑树中，这个红黑树是 `epoll` 实例的一部分。
当你调用 `epoll_ctl` 函数并传入 `EPOLL_CTL_ADD` 操作时，文件描述符和对应的 `epoll_event` 结构体（在你的代码中是 `ev`）会被添加到这个红黑树中。

当 `epoll_wait` 函数被调用时，内核会检查红黑树中的所有文件描述符，看它们是否有符合 `epoll_event` 中指定的事件发生。
如果有，这个文件描述符会被添加到一个双向链表中，这个双向链表用于存储所有就绪的文件描述符。

所以，`ev.data.fd = listenfd;` 这行代码将 `listenfd` 存储在 `epoll_event` 结构体中，
然后这个 `epoll_event` 结构体被添加到 `epoll` 实例的红黑树中。
当 `listenfd` 上有事件发生时，它会被添加到就绪的文件描述符的双向链表中。

### epoll_ctl

```cpp
struct epoll_event ev;
ev.events = EPOLLIN; // 如果fd上有数据可供读取，且未阻塞，则会触发该事件
// EPOLLRDHUP 用于指示对等端已启动优雅关机
ev.data.fd = listenfd;
// 修改epoll的兴趣列表
if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
	perror("epoll_ctl: listen_sock");
	exit(EXIT_FAILURE);
}
```

### epoll_wait

```cpp
struct epoll_event events[MAX_EVENTS];
// 返回状态改变的fd的数量，这个 nfds 是 n_fd_s
int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
```

这里就是：红黑树动了，我不玩了！（返回！）

### 为什么 if (events[n].data.fd == listenfd)

我们在 epoll_wait 后添加了一些日志输出

```sh
The server sent a string to the client~~~~~~~~~~!
nfds = 1
listenfd        connfd
3               3       ok
nfds = 1
listenfd        connfd
3               3       ok
nfds = 1
listenfd        connfd
3               3       ok
nfds = 1
listenfd        connfd
3               3       ok
nfds = 1
listenfd        connfd
                8
The server sent a string to the client~~~~~~~~~~!
nfds = 1
listenfd        connfd
                7
The server sent a string to the client~~~~~~~~~~!
nfds = 1
listenfd        connfd
                6
The server sent a string to the client~~~~~~~~~~!
nfds = 1
listenfd        connfd
                5
The server sent a string to the client~~~~~~~~~~!
```

看代码

```cpp
for (int n = 0; n < nfds; ++n) {
	if (events[n].data.fd == listenfd) {
		int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		if (connfd == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// Set connfd to non-blocking mode (optional, depending on your use case)

		// Add connfd to epoll instance
		ev.events = EPOLLIN | EPOLLET; // 边沿触发，就是只有fd状态有改变的时候就触发（输电）
		ev.data.fd = connfd;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
			perror("epoll_ctl: connfd");
			exit(EXIT_FAILURE);
		}
	} else {
		// Handle read/write events here
		const char* pcontent = "I sent sth. to client!\n";
		write(events[n].data.fd, pcontent, strlen(pcontent));
		printf("The server sent a string to the client~~~~~~~~~~!\n");
		close(events[n].data.fd);
	}
}
```

如果有新的客户端连入，那么红黑树上的 listenfd 就会动，
那么就是走 if 分支，创建客户端的 socket，也就是`int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)`
我们会把这个新的 socket 放到红黑树中，如果与客户端的 socket 有变动，比方说这里只有可能是客户端退出，
那么就会走入 else 分支，然后给客户端发一个消息。但是实际上，我们这里并没有将客户端的 socket 从红黑树上删除

### 不能在 close 了以后 再从红黑树上删除; 因为实际上，close 就会自动将 socket 从红黑树上移出，乐了。

一旦你调用 `close` 关闭了一个文件描述符，这个文件描述符就不能再被使用了，包括不能被传递给 `epoll_ctl`。
如果你尝试在关闭文件描述符后调用 `epoll_ctl`，你可能会遇到错误。

```sh
The server sent a string to the client~~~~~~~~~~!
epoll_ctl: del: Bad file descriptor
```

所以，你应该先使用 `epoll_ctl` 和 `EPOLL_CTL_DEL` 操作将文件描述符从 `epoll` 实例中删除，
然后再调用 `close` 关闭文件描述符。这是正确的顺序。

以下是修改后的代码：

```cpp
for (int n = 0; n < nfds; ++n) {
    if (events[n].data.fd == listenfd) {
        // ... (same as before)
    } else {
        // Handle read/write events here
		// 这里其实相当于是关闭流程
        const char* pcontent = "I sent sth. to client!\n";
        write(events[n].data.fd, pcontent, strlen(pcontent));
        printf("The server sent a string to the client~~~~~~~~~~!\n");

        // Remove the file descriptor from the epoll instance before closing it
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL) == -1) {
            perror("epoll_ctl: del");
            exit(EXIT_FAILURE);
        }

        close(events[n].data.fd);
    }
}
```

在这段修改后的代码中，我在调用 `close` 函数之前添加了一个调用 `epoll_ctl` 函数的语句，用于将文件描述符从 `epoll` 实例中删除。
注意，当你使用 `EPOLL_CTL_DEL` 操作时，你不需要提供一个 `epoll_event` 结构体，所以最后一个参数是 `NULL`。

### accept

`accept` 函数的原型是：

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

其中，`sockfd` 是一个监听套接字，`addr` 是一个指向 `struct sockaddr` 结构体的指针，
这个结构体用于存储新的客户端连接的地址信息，`addrlen` 是一个指向 `socklen_t` 类型的指针，这个指针用于在调用 `accept` 函数前后保存地址的长度。

在你的代码中，你传入了 `NULL` 作为 `accept` 函数的第二个和第三个参数。
这意味着你不关心新的客户端连接的地址信息。
当 `accept` 函数返回后，新的客户端连接的地址信息不会被存储，你也无法获取到这个信息。

如果你想获取新的客户端连接的地址信息，你需要提供非 `NULL` 的第二个和第三个参数。例如：

```c
struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);
int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
```

在这个例子中，新的客户端连接的地址信息会被存储在 `client_addr` 中，
地址的长度会被存储在 `client_addr_len` 中。

### 客户端主动关闭

### 设置地址 serv_addr.sin_addr.s_addr

`serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);`和`inet_pton(AF_INET, ip, &serv_addr.sin_addr);`
这两行代码的作用都是设置网络地址，但是它们设置的地址不同。

`serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);`这行代码将地址设置为`INADDR_ANY`，
这是一个特殊的值，表示任何网络接口的地址。当你在一个多网络接口的机器上运行服务器程序时，
如果你将地址设置为`INADDR_ANY`，那么服务器程序将在所有的网络接口上监听客户端的连接请求。

而`inet_pton(AF_INET, ip, &serv_addr.sin_addr);`这行代码将地址设置为`ip`指定的地址。
`ip`是一个字符串，表示一个 IPv4 地址或者 IPv6 地址。

所以，这两行代码的作用不完全相同。如果你想让服务器程序在所有的网络接口上监听客户端的连接请求，
你应该使用`serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);`。
如果你想让服务器程序只在特定的网络接口上监听客户端的连接请求，
你应该使用`inet_pton(AF_INET, ip, &serv_addr.sin_addr);`。

### 学习代码的方法：搞明白调用顺序

```cpp

```

### struct sockaddr_in

```c
struct sockaddr_in
{
    __SOCKADDR_COMMON (sin_);
    in_port_t sin_port;			/* 端口号 */
    struct in_addr sin_addr;		/* 存储了ipv4的网络地址 */

    /* Pad to size of `struct sockaddr'.  */
    unsigned char sin_zero[sizeof (struct sockaddr)
			   - __SOCKADDR_COMMON_SIZE
			   - sizeof (in_port_t)
			   - sizeof (struct in_addr)];
};
```

`__SOCKADDR_COMMON(sin_)`: 这是一个宏，用于定义与所有套接字地址结构共有的字段。
通常，这包括地址族（AF_INET 对于 IPv4）。在不同的系统中，这部分的细节可能不同。
通常，它会包含一个名为 sin_family 的字段，表示地址族

unsigned char sin_zero[...]: 这是一个填充字段，用于确保 struct sockaddr_in 的总大小与 struct sockaddr 的大小一致。
这种填充是必要的，因为套接字函数（如 bind()、connect()、accept() 等）期望接收 struct sockaddr 类型的参数，
而 struct sockaddr_in 是 struct sockaddr 的一个特定于 IPv4 的版本。这个填充字段没有实际用途，通常在使用前应将其设置为全零。

字节：

```c
struct sockaddr_in {
        sa_family_t                sin_family;           /*     0     2 */
        in_port_t                  sin_port;             /*     2     2 */
        struct in_addr             sin_addr;             /*     4     4 */
        unsigned char              sin_zero[8];          /*     8     8 */

        /* size: 16, cachelines: 1, members: 4 */
        /* last cacheline: 16 bytes */
};

struct in_addr {
        in_addr_t                  s_addr;               /*     0     4 */

        /* size: 4, cachelines: 1, members: 1 */
        /* last cacheline: 4 bytes */
};
```

### struct sockaddr

```c
/* Structure describing a generic socket address.  */
struct sockaddr
{
    __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
    char sa_data[14];		/* Address data.  */
};
```

字节：

```c
struct sockaddr {
        sa_family_t                sa_family;            /*     0     2 */
        char                       sa_data[14];          /*     2    14 */

        /* size: 16, cachelines: 1, members: 2 */
        /* last cacheline: 16 bytes */
};
```

### socklen_t

`socklen_t` 是一个在 socket 编程中用来表示地址结构体长度的数据类型。
它是一个整数类型，通常在各个平台上定义为 32 位的数据类型。
在 socket API 函数中，`socklen_t` 用于指定或接收地址结构体
（如 `struct sockaddr`、`struct sockaddr_in`、`struct sockaddr_storage` 等）的大小。

使用 `socklen_t` 的典型场景包括：

- 在 `bind()`, `connect()`, `accept()`, 和 `getsockname()` 等函数中，`socklen_t` 用来传递地址结构体的实际大小。
- 在调用 `getpeername()`, `getsockname()`, 和 `accept()` 函数时，需要传入一个指向 `socklen_t` 类型变量的指针。
  这个变量在函数调用前表示提供的缓冲区的大小，在函数调用后表示实际填充到缓冲区中的地址的大小。

例如，当使用 `accept()` 函数接受一个新的连接时，`socklen_t` 变量会被用来获取连接的远程地址的实际大小：

```c
struct sockaddr_in client_address;
socklen_t client_address_len = sizeof(client_address);

int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
```

在这个例子中，`client_address_len` 在 `accept()` 调用之前设置为 `struct sockaddr_in` 的大小。
当 `accept()` 返回时，`client_address_len` 可能会被更新为实际用于存储客户端地址的字节数。

同样，当设置或获取 socket 选项时（例如，使用 `getsockopt()` 或 `setsockopt()`），`socklen_t` 用来表示选项值的大小。

简而言之，`socklen_t` 用来处理 socket 地址结构体的长度，确保在不同的平台和编程环境中具有一致的接口。

### switch 中 初始化变量

这个错误通常是由于在`switch`语句中使用了初始化的变量。
在你的代码中，你在`case AF_INET:`中初始化了一个`struct sockaddr_in* sin`。
这在 C++中是不允许的，因为如果`switch`语句跳转到`default`，那么`sin`将不会被初始化，
但是它的析构函数仍然会被调用，这会导致未定义的行为。

你可以通过将`case AF_INET:`的代码块放在花括号中来解决这个问题，这样可以为`sin`创建一个新的作用域。
这样，如果`switch`语句跳转到`default`，`sin`就不会被创建，也就不会调用它的析构函数。

修改后的代码如下：

```cpp
switch (sa->sa_family) {
case AF_INET: {
    struct sockaddr_in* sin = (struct sockaddr_in*)sa;
    u_char* p = (u_char*)&sin->sin_addr; //
    // 日志打印 ip:port
    if (port) {
        p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
    } else {
        p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
    }
    return (p - text);
    break;
}
default:
    return 0;
    break;
}
```
