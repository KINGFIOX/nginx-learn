// 用来存放：与日志相关的函数

#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h> // open
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_macro.h"

static u_char err_levels[][20] = {
    { "stderr" }, // 0：控制台错误
    { "emerg" }, // 1：紧急
    { "alert" }, // 2：警戒
    { "crit" }, // 3：严重
    { "error" }, // 4：错误
    { "warn" }, // 5：警告
    { "notice" }, // 6：注意
    { "info" }, // 7：信息
    { "debug" } // 8：调试
};

ngx_log_t ngx_log;

// 用来打印错误log （往终端上显示）
// （之前提到过，守护进程应该重定向到/dev/null，但是如果是紧急错误就需要打印到终端了）
void ngx_log_stderr(int err, const char* fmt, ...)
{
    // u_char 就是 unsigned char
    u_char errstr[NGX_MAX_ERROR_STR + 1];
    memset(errstr, 0, sizeof(errstr));

    // 标记数组末尾
    u_char* last = errstr + NGX_MAX_ERROR_STR;

    // 返回的 p 是指向 "nginx: "之后的位置
    u_char* p = ngx_cpymem(errstr, "nginx: ", 7);

    va_list args; // 可变参数，现代C++推荐使用 可变参数模板（是一种类型安全的方式）
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args); // 得到格式化以后的信息
    va_end(args);

    if (err) { // 如果err!=0，说明有错误发生
        p = ngx_log_errno(p, last, err);
    }

    // 位置不够，那么就会覆盖
    if (p >= (last - 1)) {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    write(STDERR_FILENO, errstr, p - errstr);

    return;
}

u_char* ngx_log_errno(u_char* buf, u_char* last, int err)
{
    // 根据err的编号，返回err的描述信息perrorinfo
    char* perrorinfo = strerror(err);
    size_t len = strlen(perrorinfo);

    char leftstr[10] = { 0 };
    sprintf(leftstr, " (%d: ", err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") ";
    size_t rightlen = strlen(rightstr);

    size_t extralen = leftlen + rightlen;
    if ((buf + len + extralen) < last) {
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, perrorinfo, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void ngx_log_error_core(int level, int err, const char* fmt, ...)
{
    u_char* last;
    u_char errstr[NGX_MAX_ERROR_STR + 1];
    memset(errstr, 0, sizeof(errstr));
    last = errstr + NGX_MAX_ERROR_STR;

    // 时间相关 -----------------------------------
    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    gettimeofday(&tv, NULL);
    time_t sec = tv.tv_sec;

    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    localtime_r(&sec, &tm);
    tm.tm_mon++;
    tm.tm_year += 1900;

    u_char strcurrtime[40] = { 0 };
    // last = (u_char *)-1就是0xffffffffffffffff（16个f），意思就是随便写
    ngx_slprintf(strcurrtime, (u_char*)-1, "%4d%02d%02d %02d:%02d:%02d",
        tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    u_char* p;
    p = ngx_cpymem(errstr, strcurrtime, strlen((const char*)strcurrtime));
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);
    p = ngx_slprintf(p, last, "%P: ", ngx_pid);

    va_list args;
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    // 位置不够，抛弃多出来的部分
    // 但是个人感觉，可以做一个循环，分批次flush
    if (p >= (last - 1)) {
        p = (last - 1) - 1;
    }

    *p++ = '\n';

    ssize_t n;
    while (1) {
        if (level > ngx_log.log_level) {
            break;
        }

        // 写入配置文件
        n = write(ngx_log.fd, errstr, p - errstr);

        if (n == -1) {
            // 这里说明，写文件出现一些问题，这个要看系统管理员怎么处理了
        } else {
            // 既写入配置文件，又stderr
            if (ngx_log.fd != STDERR_FILENO) {
                n = write(STDERR_FILENO, errstr, p - errstr);
            }
        }
        break;
    } // end while
    return;
}

void ngx_log_init()
{
    u_char* plogname = NULL;
    size_t nlen;

    CConfig* p_config = CConfig::GetInstance();
    plogname = (u_char*)p_config->GetString("Log");
    if (plogname == NULL) {
        // 使用缺省路径
        plogname = (u_char*)NGX_ERROR_LOG_PATH;
    }
    ngx_log.log_level = p_config->GetIntDefault("LogLevel", NGX_LOG_NOTICE); // 默认为NOTICE以上的等级

    ngx_log.fd = open((const char*)plogname, O_WRONLY | O_APPEND | O_CREAT, 644);

    // 如果在打开文件有，那么就定位到标准错误中
    if (ngx_log.fd == -1) {
        ngx_log_stderr(errno, "[alert] could not open err log file: open() \"%s\" failed", plogname);
        ngx_log.fd = STDERR_FILENO;
    }
    return;
}
