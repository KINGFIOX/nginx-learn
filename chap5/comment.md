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

###
