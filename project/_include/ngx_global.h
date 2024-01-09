#ifndef __NGX_GLOBAL_H__
#define __NGX_GLOBAL_H__

typedef struct {
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

extern char** g_os_argv;
extern char* gp_envmem;
extern int g_environlen;

#endif