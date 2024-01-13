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
