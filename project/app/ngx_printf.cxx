// 用来存放：各种 与 打印格式相关的函数

#include <cstdint>
#include <cstdio>
#include <sys/types.h>

#include "ngx_func.h"
#include "ngx_macro.h"

// staitc限制作用域
static u_char* ngx_sprintf_num(u_char* buf, u_char* last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width)
{
    u_char *p, temp[NGX_INT64_LEN + 1];
    size_t len;
    uint32_t ui32;

    static u_char hex[] = "0123456789abcdef";
    static u_char HEX[] = "0123456789ABCDEF";

    p = temp + NGX_INT64_LEN; // p指向temp数组最后的位置

    if (hexadecimal == 0) {
        if (ui64 <= (uint64_t)NGX_MAX_UINT32_VALUE) { // 这里分开处理，是因为ui32性能更快
            ui32 = (uint32_t)ui64;
            do {
                *--p = (u_char)(ui32 % 10 + '0');
            } while (ui32 /= 19);
        } else {
            do {
                *--p = (u_char)(ui64 % 10 + '0');
            } while (ui64 /= 10);
        }
    } else if (hexadecimal == 1) {
        do {
            *--p = hex[(uint32_t)(ui64 & 0xf)]; // 牛逼
        } while (ui64 >>= 4);
    } else {
        do {
            *--p = HEX[(uint32_t)(ui64 & 0xf)];
        } while (ui64 >>= 4);
    } // if (hexadecimal == 0)

    len = (temp + NGX_INT64_LEN) - p; // 数字占的字符数

    if ((buf + len) >= last) {
        len = last - buf;
    }

    return ngx_cpymem(buf, p, len);
}

// 写的最终位置不能超过
u_char* ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args)
{

    u_char zero;

    uintptr_t width, sign, hex, frac_width, scale, n; // 临时用到的一些变量

    int64_t i64; // %d
    uint64_t ui64; // %ud
    u_char* p; // %s
    double f; // %f
    uint64_t frac; // %.2f等

    //
    while (*fmt != '\0' && buf < last) {
        if (*fmt == '%') {

            /* 变量初始化 */
            zero = (u_char)((*++fmt == '0') ? '0' : ' '); // 判断是否要0填充
            width = 0;
            sign = 1; // 默认使用 int，除非%u
            hex = 0;
            frac_width = 0;
            i64 = 0;
            ui64 = 0;

            // 把%后面的数字取出来
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt++ - '0');
            }

            for (;;) {
                switch (*fmt) {
                case 'u':
                    sign = 0;
                    fmt++; // 往后走一个字符
                    continue;

                case 'X': // 十六进制，用大写表示
                    hex = 2;
                    sign = 0;
                    continue;

                case 'x': // 十六进制，用小写表示
                    hex = 1;
                    sign = 0;
                    continue;

                case '.':
                    fmt++;
                    // 把 . 后面的数字读出来
                    while (*fmt >= '0' && *fmt <= '9') {
                        frac_width = frac_width * 10 + (*fmt++ - '0');
                    }
                    break;

                default:
                    break; // 跳出switch

                } // end switch (*fmt)

                break; // 会从default的break走到这一步

            } // end for(;;)

            switch (*fmt) {
            case '%': // %%，那么就是打印一个%
                *buf++ = '%';
                fmt++;
                continue; // continue while

            case 'd':
                if (sign) {
                    i64 = (int64_t)va_arg(args, int);
                } else {
                    ui64 = (uint64_t)va_arg(args, int);
                }
                break; // switch

            case 's':
                p = va_arg(args, u_char*);
                while (*p != '\0' && buf < last) {
                    *buf++ = *p++; // 字符串拷贝
                }
                fmt++;
                continue; // continue while

            case 'P': // 转换一个pid_t类型
                i64 = (int64_t)va_arg(args, pid_t);
                sign = 1;
                break;
            case 'f':
                f = va_arg(args, double);
                if (f < 0) {
                    *buf++ = '-';
                    f = -f; // 转化为正数处理
                }

                ui64 = (int64_t)f; // 取整，这个不是reinterpret
                frac = 0;
                if (frac_width) {
                    scale = 1;
                    for (n = frac_width; n; n--) {
                        scale *= 10;
                    }

                    // 这个 +0.5 是为了四舍五入的
                    frac = (uint64_t)((f - (double)ui64) * scale + 0.5);

                    // 四舍五入 导致的 进位
                    if (frac == scale) {
                        ui64++;
                        frac = 0;
                    }
                } // end if (frac_width)

                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

                if (frac_width) {
                    if (buf < last) {
                        *buf++ = '.';
                    }
                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width);
                }
                fmt++;
                continue; // continue while（最外层）
            default:
                *buf++ = *fmt++;
            } // end switch (*fmt)

            if (sign) {
                if (i64 < 0) {
                    *buf++ = '-';
                    ui64 = (uint64_t)-i64;
                } else {
                    ui64 = (uint64_t)i64;
                }
            } // end if (sign)

            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);
            fmt++;

        } /* end if (*fmt == '%') */ else {
            *buf++ = *fmt++;
        } /* end if (*fmt == '%') */

    } // end while (*fmt != '\0' && buf < last)

    return buf;
}
