#ifndef __NGX_FUNC_H__
#define __NGX_FUNC_H__

#include <cstdarg>
#include <sys/types.h>

void Rtrim(char* string);
void Ltrim(char* string);

u_char* ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args);

#endif //  __NGX_FUNC_H__