#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngx_c_memory.h"

CMemory* CMemory::m_instance = NULL;

/**
 * @brief 分配内存
 * 
 * @param memCount 分配的字节大小
 * @param ifmemset 是否要初始化为0
 * @return void* 
 */
void* CMemory::AllocMemory(int memCount, bool ifmemset)
{
    void* tmpData = (void*)new char[memCount];
    if (ifmemset) {
        memset(tmpData, 0, memCount);
    }
    return tmpData;
}

/**
 * @brief 内存释放
 * 
 * @param point 
 */
void CMemory::FreeMemory(void* point)
{
    delete[]((char*)point);
}