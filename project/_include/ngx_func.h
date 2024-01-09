#ifndef __NGX_FUNC_H__
#define __NGX_FUNC_H__

#include <cstdarg>
#include <cstdint>
#include <sys/types.h>

void Rtrim(char* string);
void Ltrim(char* string);

// 格式化字符串 ---------
u_char* ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args);
u_char* ngx_slprintf(u_char* buf, u_char* last, const char* fmt, ...);

// 日志 ---------------
void ngx_log_init();
void ngx_log_stderr(int err, const char* fmt, ...);
u_char* ngx_log_errno(u_char* buf, u_char* last, int err);
void ngx_log_error_core(int level, int err, const char* fmt, ...);

// 集中释放资源 ---------
void freeresource();

// 信号 ----------------
int ngx_init_signals();

// 子进程实战
void ngx_master_process_cycle();

#endif //  __NGX_FUNC_H__