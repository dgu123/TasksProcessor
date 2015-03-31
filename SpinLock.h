#pragma once
//-------------------------------------------------------------------------------------------------
#include <atomic>
//-------------------------------------------------------------------------------------------------
class Spinlock
{
public:
	Spinlock()		
	{
		m_locked.clear(); //VS workaround
	}

    void lock()
    {
		int repeatCount = 100;

		while (true)
		{
			if (m_locked.test_and_set(std::memory_order::memory_order_acquire))
				--repeatCount;
			else
				break;
			
			if (repeatCount == 0)
			{
				//usleep(100); //ACHTUNG!!! place delay here
				repeatCount = 100;
			}
		}
    }
    void unlock(){m_locked.clear(std::memory_order::memory_order_release);}
    
    bool tryLock()
	{
		return !m_locked.test_and_set(std::memory_order::memory_order_acquire);
	}

private:
	std::atomic_flag m_locked;
};
//-------------------------------------------------------------------------------------------------
