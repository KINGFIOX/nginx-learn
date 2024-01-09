#ifndef __NGX_MACRO_H__
#define __NGX_MACRO_H__

#include <cstring>

#define NGX_MAX_ERROR_STR 2048

// 数字相关 ----------------
#define NGX_MAX_UINT32_VALUE (uint32_t)0xffffffff /* 最大的32位无符号数：十进制是‭4294967295‬ */
#define NGX_INT64_LEN (sizeof("-9223372036854775808") - 1) /* 20个字节 */

// 宏函数 ------------------
/* 拷贝内存，并将返回指针，指针的位置是：dst结束的位置 */
#define ngx_cpymem(dst, src, n) (((u_char*)memcpy(dst, src, n)) + (n))
#define ngx_min(val1, val2) ((val1 < val2) ? (val1) : (val2))

// 日志等级 ----------------

#endif