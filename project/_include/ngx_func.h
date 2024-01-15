#ifndef __NGX_FUNC_H__
#define __NGX_FUNC_H__

#include <cstdarg>
#include <cstdint>
#include <sys/types.h>

// 字符串 左右裁剪空白
void Rtrim(char* string);
void Ltrim(char* string);

// 格式化字符串、日志打印
u_char* ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args);
u_char* ngx_slprintf(u_char* buf, u_char* last, const char* fmt, ...);
u_char* ngx_snprintf(u_char* buf, size_t max, const char* fmt, ...);
void ngx_log_init();
void ngx_log_stderr(int err, const char* fmt, ...);
u_char* ngx_log_errno(u_char* buf, u_char* last, int err);
void ngx_log_error_core(int level, int err, const char* fmt, ...);

// 子进程、守护进程、信号
int ngx_init_signals();
void ngx_master_process_cycle();
int ngx_daemon();
void ngx_process_events_and_timers();

#endif //  __NGX_FUNC_H__