#ifndef __NGX_MACRO_H__
#define __NGX_MACRO_H__

#include <cstring>

// 宏函数 ------------------
/* 拷贝内存，并将返回指针，指针的位置是：dst结束的位置 */
#define ngx_cpymem(dst, src, n) (((u_char*)memcpy(dst, src, n)) + (n))
#define ngx_min(val1, val2) ((val1 < val2) ? (val1) : (val2))

// 日志相关 ----------------
#define NGX_LOG_STDERR 0 // 控制台错误【stderr】：最高级别日志，日志的内容不再写入log参数指定的文件，而是会直接将日志输出到标准错误设备比如控制台屏幕
#define NGX_LOG_EMERG 1 // 紧急 【emerg】
#define NGX_LOG_ALERT 2 // 警戒 【alert】
#define NGX_LOG_CRIT 3 // 严重 【critical】
#define NGX_LOG_ERR 4 // 错误 【error】：属于常用级别
#define NGX_LOG_WARN 5 // 警告 【warn】：属于常用级别
#define NGX_LOG_NOTICE 6 // 注意 【notice】
#define NGX_LOG_INFO 7 // 信息 【info】
#define NGX_LOG_DEBUG 8 // 调试 【debug】：最低级别

#define NGX_ERROR_LOG_PATH "logs/error1.log" // 定义日志存放的路径和文件名

#define NGX_MAX_ERROR_STR 2048 /* 最长错误信息是2048 */

// 数字上下界 ----------------
#define NGX_MAX_UINT32_VALUE (uint32_t)0xffffffff /* 最大的32位无符号数：十进制是‭4294967295‬ */
#define NGX_INT64_LEN (sizeof("-9223372036854775808") - 1) /* 20个字节 */

// 进程相关 ------------
#define NGX_PROCESS_MASTER 0 /* master进程，管理进程 */
#define NGX_PROCESS_WORKER 1 /* worker进程，工作进程 */

#endif