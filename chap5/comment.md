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

### struct epoll_event

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

`struct epoll_event` 是 Linux 系统中 `epoll` API 所定义的一个结构体，
它用于表示对 `epoll` 实例感兴趣的事件类型以及这些事件相关联的数据。

`epoll` 是 Linux 提供的多路复用输入/输出（I/O）系统调用接口，
它允许程序监视多个文件描述符以等待某个文件描述符上的 I/O 事件发生，如可读、可写或错误等。
`epoll` 比传统的 `select` 或 `poll` 系统调用更加高效，因为它能够更好地扩展到大量文件描述符。

- `events` 成员用于指定感兴趣的事件类型和一些额外的条件。它可以是以下一种或多种的位集合：

  - `EPOLLIN`：表示对应的文件描述符可读（包括对等方关闭连接的情况）。
  - `EPOLLOUT`：表示对应的文件描述符可写。
  - `EPOLLRDHUP`：表示对等方关闭了连接，或者关闭了写操作。
  - `EPOLLPRI`：表示对应的文件描述符有紧急的数据可读（如 TCP socket 的带外数据）。
  - `EPOLLERR`：表示对应的文件描述符发生了错误。
  - `EPOLLHUP`：表示对应的文件描述符被挂断。
  - `EPOLLET`：将 EPOLL 设为边缘触发（Edge Triggered）模式，这意味着它只会通知有状态改变的事件。
  - `EPOLLONESHOT`：表示对应的文件描述符只会报告一次事件，之后需要重新设置。

- `data` 成员是一个用户自定义的数据，用于存储某些与事件相关联的额外信息。
  这可以是一个指针、一个文件描述符、一个 32 位整数或一个 64 位整数。
  在事件发生时，可以从 `epoll_wait` 调用返回的事件数组中取回这个数据，
  这常用于回调函数或者是与文件描述符相关联的对象。

当你使用 `epoll_ctl` 函数将文件描述符添加到 `epoll` 实例时，
你会提供一个指向 `struct epoll_event` 的指针，
以告诉 `epoll` 你对哪些事件感兴趣，以及与这些事件相关联的用户数据。
随后，当你调用 `epoll_wait` 函数等待事件发生时，
`epoll` 会返回一个 `struct epoll_event` 类型的数组，
其中包含了实际发生的事件以及你之前提供的用户数据。

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

### accept4

`accept4` 是一个在某些类 Unix 系统（如 Linux）中提供的系统调用，它是 `accept` 系统调用的扩展版本。
`accept` 函数用于在一个监听的套接字上接受一个新的连接请求，而 `accept4` 除了具有 `accept` 的功能外，
还允许在接受连接时指定额外的标志。

```c
// 这个是accept的原型，可以看到，accept4比accept多了一个flags
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

`accept4` 的函数原型如下（在 Linux 中定义）：

```c
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
```

这个函数的参数包括：

- `sockfd` 是一个 listen fd
- `addr` 是一个指向 `sockaddr` 结构体的指针，用于返回接受的连接的远程端地址信息。
- `addrlen` 是一个指向 `socklen_t` 类型的指针，在调用之前，它应该被初始化为指向的缓冲区的大小。调用完成后，这个值被设置为实际地址结构的大小。也就是`sizeof(struct sockaddr)`
- `flags` 是一个整数，用于指定一些特殊操作。这些标志可以是以下值的位或（bitwise OR）：
  - `SOCK_NONBLOCK` 设置返回的文件描述符为非阻塞模式。
  - `SOCK_CLOEXEC` 设置返回的文件描述符在执行 exec 系列函数时关闭（即设置 close-on-exec 标志）。

在没有 `accept4` 函数的系统中，如果你想要设置返回的套接字为非阻塞或者设置 close-on-exec 标志，
你需要在 `accept` 调用之后分别调用 `fcntl` 函数来设置这些属性。`accept4` 允许你直接在接受新连接的同时设置这些属性，
这样可以减少系统调用的次数，并且避免了多线程环境下的潜在竞争条件。

举个简单的例子：

```c
int new_socket = accept4(listen_socket, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
```

在这个例子中，`accept4` 创建了一个新的非阻塞套接字，并且设置了 close-on-exec 标志，
这意味着这个新的套接字在程序调用 exec 函数替换当前进程映像时会被关闭。

### 心得

vscode 中，可以通过`shift + f12`，查看一个函数在哪里被引用

### 内存对齐模数

实际上，"内存魔术"这个词可能是一个误解或者是一个打字错误。在内存对齐的上下文中，我们通常讨论的是"内存对齐模数"（alignment modulus），它是指数据类型在内存中存储时地址对齐的基数。默认的内存对齐模数通常取决于你的编译器和目标平台的架构。

要确定你的系统或某个特定数据类型的默认内存对齐模数，你可以使用编程语言提供的工具或函数。例如，在 C 或 C++中，你可以使用`alignof`或`_Alignof`运算符来获取一个类型的对齐要求：

```c
#include <stdio.h>

int main() {
    printf("The alignment of 'int' is: %zu\n", alignof(int));
    printf("The alignment of 'double' is: %zu\n", alignof(double));
    printf("The alignment of a 'struct' is: %zu\n", alignof(struct { char c; int i; }));

    return 0;
}
```

在 C++中，`alignof`是关键字，而在 C11 中，`_Alignof`是运算符。

如果你想要获取更一般的系统级的内存对齐模数信息，这通常是由 CPU 的架构决定的。
大多数现代的 x86 和 x64 架构处理器在硬件层面上支持不同的对齐模数，
但在软件层面，编译器通常会选择一个默认的对齐模数，这个值往往是 8 或 16。
这个值是一个平衡内存使用和性能的结果。

如果你使用的是 GCC 或 Clang，你可以通过编译器的文档或者使用特定的编译器标志来获取对齐信息。
例如，GCC 有一个`-Wpadded`标志，它会警告你关于结构体的填充和可能的对齐问题。

还有，如果你使用的是特定的 IDE 或者开发工具，
它们可能会有自己的方法来显示或设置默认的内存对齐模数。

需要注意的是，操作系统和编译器通常允许开发者通过特定的指令和属性来覆盖默认的内存对齐模数。
这在处理特定硬件接口或者优化性能时可能很有用。

### epoll 如何知道 事件被处理了 ？

在使用 `epoll` 的默认行为，即水平触发（Level Triggered, LT）模式时，
`epoll_wait` 会不停地通知你有事件发生，直到你处理了这些事件。
实际上，`epoll` 并不知道你是否调用了处理函数；它只关心文件描述符的状态。

在水平触发模式下，如果某个文件描述符上有可读写事件发生，
并且你没有读取数据或者没有写入足够的数据到缓冲区（因此缓冲区仍然是满的或空的），
`epoll_wait` 在下次被调用时还会报告这个文件描述符上有相同的事件。
这意味着如果你不处理这些事件（例如，对于 `EPOLLIN`，你不读取数据；对于 `EPOLLOUT`，你不写入数据），
`epoll_wait` 将会不断地返回这个文件描述符，因为从 `epoll` 的角度来看，这个事件一直处于触发状态。

这里是处理 `epoll` 事件的一个简化的例子：

```c
struct epoll_event events[MAX_EVENTS];

// Assume epoll_fd is the file descriptor for the epoll instance
while (true) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    if (nfds == -1) {
        if (errno == EINTR) {
            // Interrupted by a signal, continue the loop
            continue;
        } else {
            // Handle other errors
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
    }

    for (int n = 0; n < nfds; ++n) {
        int fd = events[n].data.fd;

        // Check if the file descriptor is ready for reading
        if (events[n].events & EPOLLIN) {
            // Handle the read event
            ssize_t count;
            char buf[BUF_SIZE];

            // It's important to actually perform the read operation here
            count = read(fd, buf, sizeof buf);

            if (count == -1) {
                // If errno == EAGAIN, that means we have read all data.
                if (errno != EAGAIN) {
                    perror("read");
                    // Handle read error
                }
                continue;
            } else if (count == 0) {
                // End of file. The remote has closed the connection.
                close(fd);
            }

            // Process the read data contained in 'buf'
        }

        // Similarly, handle other event types like EPOLLOUT, EPOLLERR, etc.
    }
}
```

在上面的代码中，一旦 `epoll_wait` 返回，并且报告 `EPOLLIN`（可读事件），
代码就会尝试从文件描述符读取数据。你必须确保实际从文件描述符读取数据，
否则 `epoll_wait` 会不断地报告该文件描述符上有可读事件。

同样的逻辑适用于写事件（`EPOLLOUT`）。
如果你不实际写入数据到输出缓冲区，
或者没有写入足够的数据使得缓冲区不再是满的，
`epoll_wait` 会继续报告可写事件。

总之，`epoll` 不知道你是否调用了处理函数，
但它会基于文件描述符的状态不断通知你。
因此，你需要确保在接收到事件通知后，
正确地处理这些事件（如读取或写入数据），
以避免 `epoll_wait` 持续不断地返回相同的事件。

### epoll_wait 返回后，过滤过期事件（时间戳）

`epoll_wait` 返回后，它提供的事件是根据最新的文件描述符状态来报告的。由于 `epoll` 是基于事件的机制，理论上它不会返回过期的事件——即它返回的事件应该反映了文件描述符在 `epoll_wait` 被调用时刻的实时状态。

然而，实际编程中可能出现以下情况，你可能认为某些事件是 "过期" 的：

1. **事件已经被处理**

如果你在多个线程中处理 `epoll` 事件，
可能一个线程已经处理了某个文件描述符上的事件，
而 `epoll_wait` 在另一个线程中又返回了相同的文件描述符。
这种情况可以通过设计合理的同步机制来避免，
比如使用线程安全的队列存储待处理的文件描述符。

2. **文件描述符状态改变**

如果事件通知之后，文件描述符的状态改变了（例如，通过另一个线程），
你可能会认为 `epoll_wait` 返回的事件已经不再相关。
处理这种情况的一种方法是在处理事件之前再次检查文件描述符的状态。

3. **边缘触发（ET）模式下的事件遗漏**

在边缘触发模式下，如果你没有读取或写入所有数据，`epoll_wait` 可能不会再次通知你。
这不是事件 "过期"，而是事件被遗漏了。
为了避免这种情况，你需要确保在边缘触发模式下读取或写入直到返回 `EAGAIN` 错误。

如果你确实需要过滤掉某些事件，你需要在你的应用程序逻辑中实现这一机制。
这通常涉及到跟踪你的应用程序的状态，并在处理事件时检查这些状态。以下是一种可能的策略：

- **时间戳检查**

为每个文件描述符关联一个时间戳，当你处理一个事件时，更新该时间戳。当 `epoll_wait` 返回相同的文件描述符时，检查时间戳，如果事件的时间戳早于你期望的时间，你可以认为这个事件是 "过期" 的，跳过处理。

这是一个示例伪代码，展示如何实现时间戳检查：

```c
// 假设每个文件描述符都关联了一个时间戳结构体
typedef struct {
    int fd;
    uint64_t last_processed_time;
} fd_state;

fd_state fds[MAX_FDS];

// 初始化 fds ...

while (running) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        uint64_t current_time = get_current_time(); // 获取当前时间的函数

        if (fds[fd].last_processed_time < current_time) {
            // 处理事件
            process_event(events[i]);

            // 更新该文件描述符的时间戳
            fds[fd].last_processed_time = current_time;
        } else {
            // 事件可能是"过期"的，跳过处理
        }
    }
}
```

请注意，上述示例是一种简化的表示，并不适用于所有场景。在多线程环境中，你需要确保对文件描述符状态的访问是线程安全的。
此外，确保时间戳的精度和更新机制与你的应用程序的需求相匹配是非常重要的。
