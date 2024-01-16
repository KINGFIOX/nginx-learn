#ifndef __NGX_COMM_H__
#define __NGX_COMM_H__

#define _PKG_MAX_LENGTH 30000 /* 每个包的最大长度 */

#define _PKG_HD_INIT 0 /* 初始状态，准备接收数据包头 */
#define _PKG_HD_RECVING 1 /* 接收包头中，包头不完整，继续接收中 */
#define _PKG_BD_INIT 2 /* 包头刚好接收完，准备接收包体 */
#define _PKG_BD_RECVING 3 /* 接收包体中，包体不完整，继续接收中，接收完成以后，因为状态化简，下个状态是 INIT 即可 */
// #define _PKG_RV_FINISHED 4 /* 接收完成，可以不需要这个状态，但是这个状态可以化简 */

#define _DATA_BUFSIZE_ 20 /* 定义一个固定长度的数组，用来接收包头，尽管包头是10个字节，但是我这里大一些 */

#pragma pack(1) /* 对齐方式，1个字节对齐，也就是不对其 */

typedef struct _COMM_PKG_HEADER {
    unsigned short pkgLen; /* 报文总长度 包头 + 包体，2字节，2字节最大数字65535，我们规定包长度上限是3w */
    unsigned short msgCode; /* 消息类型，2字节，用来区别每个不同的命令 */
    int crc32; /* crc32校验，4 字节 */
} COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

#pragma pack() /* 取消1字节对齐 */

#endif /*  __NGX_COMM_H__ */