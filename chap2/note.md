# chap2 - 初步

## ps -ef | grep nginx 解析

- nobody 是一个权限比较低的用户，防止被强奸

## nginx 进程模型

一个 master 进程，多个 worker 进程

master 进策划那个职责：监控进程，不处理具体业务

worker 进程，主要用来干活的

用户来了，worker 会竞争

worker 进程 与 master 进程之间通信，可以通过共享内存，可以通过信号

worker 进程多少个合适呢？多核 CPU，让每一个 worker 运行在一个单独的核心上
