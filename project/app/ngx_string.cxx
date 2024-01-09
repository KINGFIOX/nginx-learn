#include "ngx_func.h"
#include <cstdio>
#include <cstring>

void Rtrim(char* string)
{
    size_t len = 0;
    if (string == NULL) {
        return;
    }
    len = strlen(string);
    while (len > 0 && (string[len - 1] == ' ' || string[len - 1] == '\t')) {
        string[--len] = 0;
    }
    return;
}

void Ltrim(char* string)
{
    size_t len = 0;
    len = strlen(string);
    char* p_tmp = string;
    if ((*p_tmp) != ' ' && (*p_tmp) != '\t') {
        return;
    }
    while ((*p_tmp) == '\0') {
        if ((*p_tmp) == ' ' || (*p_tmp) == '\t') {
            ++p_tmp;
        } else {
            break;
        }
    }
    // 这一行全是空白
    if ((*p_tmp) == '\0') {
        *string = '\0';
        return;
    }
    char* p_tmp2 = string;
    // 左移
    while ((*p_tmp) != '\0') {
        (*p_tmp2) = (*p_tmp);
        p_tmp++;
        p_tmp2++;
    }
    (*p_tmp2) = '\0';
    return;
}