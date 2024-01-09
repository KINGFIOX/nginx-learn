// 用来存放：与日志相关的函数

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ngx_func.h"
#include "ngx_macro.h"

// 用来打印错误log （往终端上显示）
// （之前提到过，守护进程应该重定向到/dev/null，但是如果是紧急错误就需要打印到终端了）
void ngx_log_stderr(int err, const char* fmt, ...)
{
    // u_char 就是 unsigned char
    u_char errstr[NGX_MAX_ERROR_STR + 1];
    u_char *p, *last;

    memset(errstr, 0, sizeof(errstr));

    // 标记数组末尾
    last = errstr + NGX_MAX_ERROR_STR;

    // 返回的 p 是指向 "nginx: "之后的位置
    p = ngx_cpymem(errstr, "nginx: ", 7);

    va_list args; // 可变参数，现代C++推荐使用 可变参数模板（是一种类型安全的方式）
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);
}