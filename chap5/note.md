# chap5 - 网络通讯实战

## chap5 - 01 - TCP/IP 协议妙趣横生

### 客户端 与 服务器

客户端，服务端 都是 一个程序

#### 解析一个浏览器访问网页的过程

#### 客户端 服务器 角色 规律总结

1. 数据通讯总在两端进行，其中一端叫做客户端，另一端叫做服务器端

2. 总有一方要先发起数据，这发起第一个数据包的一段，就叫客户端; 被动接受第一个数据包这端，就叫服务器端

3. 连接建立起来，数据双向流动，这叫双工【你可以发数据包给我，我也可以发数据包给你】

4. 既然服务器端是被动接收连接，那么客户端必须能够找到服务器在哪里

比方说，我的浏览器要访问 taobao，我需要知道 taobao 服务器的地址【ip 地址，192.168.1.1】，
以及 taobao 服务器的姓名【端口号，这是一个 无符号数字，范围 0 ～ 65535】，浏览器默认是访问 80 端口（http）

taobao 服务器【nginx 服务器】会调用 listen() 函数监听 80 端口

在编写网络通讯程序时，只需要指定 taobao 服务器的 ip 地址 和 淘宝服务器的端口号，就能够跟 taobao 服务器 进行 通讯

5. epoll 技术可以支持 数十万 的连接

### 网络模型

#### OSI 七层网络模型

哪 7 层？顺口溜：物【物理层】 链【数据链路层】 网【网络层】 传【传输层】 会【会话层】 表【表示层】 应【应用层】

open system interconnect（osi）：开放式系统互联：ISO 国际标准化组织，在 1985 年研究出来的一个模型

物理层：网卡、硬件等

应用层：为应用程序提供服务

把一个要发送出去的数据包，从里到外裹了 7 层; 最终把包裹了 7 层的数据包发送了出去

（这个会在《计网》中应该会 详细学习）

#### TCP/IP 协议 四层模型

transfer control protocol（传输控制协议） / internet protocol （网际协议） (TCP/IP)

tcp/ip 实际上是 一组 协议 的代名词，而不仅仅是一个协议

- 链路层（物、链） --> ARP、以太网帧、RARP
- 网络层（网） --> ICMP、IP、IGMP
- 传输层（传）--> TCP、UDP
- 应用层（会、表、应） --> 用户进程

把数据包，裹了 4 层

TCP/IP，其实每一层都对应着一些协议

重点理解：

以太网帧 <--> IP <--> TCP <--> 用户进程

#### TCP/IP 协议的解释和比喻

要发送 abc --> 加 tcp 头 --> 加 ip 头 --> 加 以太网帧 头和尾

加上 3 头 1 尾

![tcpip](image/tcpip.png)

### 最简单的用户端 和 服务器程序实现代码

下面这两个 服务器程序、客户端程序，只具备演示价值

推荐书籍：《unix 网络编程》

#### 套接字 socket 概念

socket（套接字）：就是一个数字，通过调用 socket()函数生成的，这个数字具有唯一性;
当调用 socket()，操作系统给你返回的话，这个数字会一直给你用，直到你调用 close()，
把这个数字关闭（有点 fd 的意思），才会把这个数字回收。

unix 哲学：一切皆文件，所以我们也把这个 socket 也堪称是文件描述符，
我们可以用 socket 收发数据：send()，recv()

#### 一个简单的服务端 通讯程序 范例

注意：服务器端 与 客户端 程序是不一样的。

listenfd = socket() --> bind() --> listen() --> connfd = accept()【等待客户端连我，阻塞】
--> write(connfd) --> close(connfd) --> close(listenfd)

#### IP 地址 简单谈

ipv4 地址：理解成现实社会中的 居住地址

#### 一个简单的客户端 通讯程序 范例

建立连接时，双方彼此都要有 ip 地址、端口号。

连接一旦建立起来，那么双方的通讯【双工收发】，
就只需要用双方彼此对应的 socket 即可。

#### 客户端 服务器程序 综合演示 和 调用流程图

### TCP 和 UDP 的区别

我们可以看到：UDP、TCP 都是 “传输层”

UDP：不可靠，无连接的（有种无线网络的感觉），
发送速度特别快，但无法确保数据的可靠性。
可能后发出的数据先到达。

TCP：可靠，有连接的（有种有线网络的感觉），
TCP 可靠，必要要更多的系统资源确保数据传输的可靠。
得到的呃好处就是：只要不断线，传输给对象的数据，一定正确，不丢失，不重复，按顺序到达对端

应用场景：

- tcp: 文件传输，一般 TCP 对 UDP 用的范围 和 场景更广
- udp: qq 聊天信息，聊天信息巨大，服务器懒得整，但是 qq 内部有一些算法导致丢包率下降

《TCP/IP 协议详解》

## chap5 - 02 - TCP 三次握手详解、telnet、wireshark 示范

### TCP 连接的三次握手

只有 tcp 有三次握手【连接的时候】

#### 最大传输单元 MTU

mtu (maximum transfer unit)，最大传输单元

每个数据包 包含的数据最多可以有多少个字节？1.5k 左右（真正传输的数据，大概是 1.4k 左右，加上一 3 头 1 尾之类的）

你要发送 100k，操作系统内部会把你这 100k 数据拆分成若干个数据包，
每个数据包大概 1.5k 之内。

我们只需要有：拆包、组包 这个过程就行了，细节不用掌握

我们拆开的包，各自传输的路径可能不同，每个包可能因为路由器、交换机等原因被再次分片。

最终 tcp/ip 协议，保证了我们收发数据的顺序性和可靠性

#### TCP 包 头结构

就是一个 struct，

0. 前面的头
1. 源端口，目标端口（这里就没有 ip 了，ip 有 IP 头）
2. 32 位序列号
3. 32 位确认号
4. tcp 头部大小（4 位） + 预留 4 位 + 一些开关（关注 syn 位和 ack 位 ）+ 16 位窗口大小
5. 16 位校验值 + 16 位紧急指针
6. tcp 选项
7. data（可选）

每层是 32 位的。

有的数据包里可能是没有 数据（data） 的

#### TCP 数据包收发之前的准备工作

回忆日志操作的步骤：

1. 打开日志文件
2. 多次，反复的往日志文件中写信息
3. 关闭日志文件

tcp 数据的收发是双工的：每端既可以收数据，也可以发数据。

tcp 数据包的发送也分三大部分：

1. 建立 tcp 连接，三次握手(connect)
2. 多次反复的数据收发(read/write)
3. 关闭 tcp 连接(close)

udp 不存在三次握手来建立连接，因此 udp 实际上不存在建立连接的步骤，
udp 数据包是直接发送出去的

#### TCP 三次握手建立连接的过程

客户端理解成一个人，服务器理解为一个人，两个人要用电话通话：

1. 张三（客户端）：你好，李四
2. 李四（服务端）：你好，张三，我是李四
3. 张三：你好，李四

![hand](image/hand.png)

1. 客户端 ---（SYN=1，无 data）---> 服务器
2. 服务器 ---（syn=1，ACK=1，ack=xxx，无 data）---> 客户端
3. 客户端 ---（ACK=1，ack=xxx，无 data）---> 服务器

三次握手成功 建立连接后，才会开始传送 应用层 的数据

#### 为什么 TCP 握手是 三次 而不是 两次

tcp 握手是三次，而不是二次。

比方说诈骗：

1. 假冒公安局（客户端）：你的身份证号是 xxxx，你的名字是 xxxx
2. 你是妖妖灵？那我给你打回去，啪，挂断电话，打回去（笑了）

TCP 三次握手的最主要原因之一： 1. 尽量减少伪造数据包对服务器的攻击

上面的三次中

服务器给客户端发送 ack 包，只有客户端再次返回 ack 相关的包，那么才能建立连接。

因为真正的收发数据，是有成本的。如果伪造了许多假的客户端连接服务器，那么服务区吃不消的。
阻碍了正常的业务。

### telnet 工具使用介绍

是一款命令行方式运行的 客户端 tcp 通讯工具，可以连接到服务器端，
往服务器端发送数据，也可以接收从服务器端发送过来的数据。
有点像上一节课写的客户端代码

```sh
telnet <ip地址> <端口号>
```

可以用来测试服务器端的端口开了没有

### wireshark 监控数据包

在 capture 中，先关闭抓包，然后选择 option，然后设置 filter

```wireshark
host 10.211.55.3 and port 9000
```

![wireshark](image/wireshark.png)

#### TCP 断开的 4 次挥手

这个谁先发送不一定的，看是谁先断开

1. FIN ACK 服务端 --> 客户端
2. ACK 服务端 <-- 客户端
3. FIN ACK 服务端 --> 客户端
4. ACK 服务端 <-- 客户端

![wave](image/wave.png)

## chap5 - 03

### TCP 状态转换

### TIME_WAIT 详解

#### RST 标志

### SO_REUSEADDR

#### 两个进程，绑定同一个 ip 和端口

#### time_wait 状态时的 bind 绑定