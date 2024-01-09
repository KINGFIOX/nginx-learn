#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "ngx_global.h"

#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__

class CConfig {
private:
    CConfig();

public:
    ~CConfig();

private:
    static CConfig* m_instance;

public:
    static CConfig* GetInstance()
    {
        if (m_instance == NULL) {
            // 锁
            if (m_instance == NULL) {
                m_instance = new CConfig();
                static CGarhuishou cl; // 生命周期和程序一样长，当程序结束的时候，就会释放资源
            }
            // 放锁
        }
        return m_instance;
    }

    // 这是一种设计模式：垃圾回收类，用来释放单例类
    class CGarhuishou {
    public:
        ~CGarhuishou()
        {
            if (CConfig::m_instance) {
                delete CConfig::m_instance;
                CConfig::m_instance = NULL;
            }
        }
    };

public:
    bool Load(const char* pconfName);
    char* GetString(const char* p_itemname);
    int GetIntDefault(const char* p_itemname, const int def);

public:
    std::vector<LPCConfItem> m_ConfigItemList; // 存储配置信息的列表，这里面存放的是指针
};

#endif // __NGX_C_CONF_H__