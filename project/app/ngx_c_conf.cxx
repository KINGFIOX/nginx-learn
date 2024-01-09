#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <vector>

#include "ngx_c_conf.h"
#include "ngx_func.h"
#include "ngx_global.h"

CConfig* CConfig::m_instance = NULL;

// 构造函数
CConfig::CConfig()
{
}

CConfig::~CConfig()
{
    std::vector<LPCConfItem>::iterator pos;
    for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos) {
        delete (*pos);
    }
    m_ConfigItemList.clear();
}

bool CConfig::Load(const char* pconfName)
{
    FILE* fp = fopen(pconfName, "r");
    if (fp == NULL) {
        return false;
    }

    // 每一行配置文件读出来都放到这里
    // 每行不错过500字符
    char linebuf[500];

    while (!feof(fp)) { // 没有读到文件尾
        if (fgets(linebuf, 500, fp) == NULL) {
            continue;
        }
        if (linebuf[0] == 0) {
            continue;
        }
        // 处理注释行、空行
        if (*linebuf == ';' || *linebuf == ' ' || *linebuf == '#' || *linebuf == 't' || *linebuf == '\n') {
            continue;
        }
    // 标号
    lblprocstring:
        // 截掉换行、空格、回车
        if (strlen(linebuf) > 0) {
            if (linebuf[strlen(linebuf) - 1] == 10 || linebuf[strlen(linebuf) - 1] == 13 || linebuf[strlen(linebuf) - 1] == 32) {
                linebuf[strlen(linebuf) - 1] = 0; // 相当于\0提前出现
                goto lblprocstring;
            }
        }
        if (linebuf[0] == 0) {
            continue;
        }
        // [组] 行不处理
        if (*linebuf == '[') {
            continue;
        }
        // 找等号，返回的是第一个位置的指针
        char* ptmp = strchr(linebuf, '=');
        if (ptmp != NULL) {
            LPCConfItem p_confitem = new CConfItem;
            memset(p_confitem, 0, sizeof(CConfItem));
            // 把等号之前的元素，拷贝到itemname中
            strncpy(p_confitem->ItemName, linebuf, (int)(ptmp - linebuf));
            strcpy(p_confitem->ItemName, ptmp + 1);

            // 去掉空白字符
            Rtrim(p_confitem->ItemName);
            Ltrim(p_confitem->ItemName);
            Rtrim(p_confitem->ItemContent);
            Ltrim(p_confitem->ItemContent);
            m_ConfigItemList.push_back(p_confitem);
        }
    } // while (!feof(fp))
    fclose(fp); // 有fopen，就要有fclose
    return true;
}

// 根据itemName获取配置信息字符串，因为不修改，所以不用上锁
char* CConfig::GetString(const char* p_itemname)
{
    std::vector<LPCConfItem>::iterator pos;
    for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, p_itemname) == 0) {
            return (*pos)->ItemContent;
        }
    }
    return NULL;
}

// 如果是取出来的是数字
int CConfig::GetIntDefault(const char* p_itemname, const int def)
{
    std::vector<LPCConfItem>::iterator pos;
    for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos) {
        if (strcasecmp((*pos)->ItemName, p_itemname) == 0) {
            return atoi((*pos)->ItemContent);
        }
    }
    return def;
}
