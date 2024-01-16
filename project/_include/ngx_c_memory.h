#ifndef __NGX_C_MEMORY_H__
#define __NGX_C_MEMORY_H__

#include <cstddef>
class CMemory {
private:
    CMemory() {}

public:
    ~CMemory() {}

private:
    static CMemory* m_instance;

public:
    static CMemory* GetInstance()
    {
        // 上锁
        if (m_instance == NULL) {
            m_instance = new CMemory();
            static CGarhuishou cl;
        }
        return m_instance;
    }
    class CGarhuishou {
    public:
        ~CGarhuishou()
        {
            if (CMemory::m_instance != NULL) {
                delete CMemory::m_instance;
                CMemory::m_instance = NULL;
            }
        }
    }; // end class CGarhuishou
public:
    void* AllocMemory(int memCount, bool ifmemset);
    void FreeMemory(void* point);
}; // end class CMemory

#endif // __NGX_C_MEMORY_H__