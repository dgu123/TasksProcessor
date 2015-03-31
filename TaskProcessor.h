#pragma once
//-------------------------------------------------------------------------------------------------
#include <vector>
#include <list>
#include <thread>
#include <condition_variable>
#include <future>
#include <iostream>

#include "SpinLock.h"
//-------------------------------------------------------------------------------------------------
class TaskProcessor
{
public:
	TaskProcessor() :
		m_Running(true),
		m_Threads(std::thread::hardware_concurrency())
	{
		for (auto &t : m_Threads)
			t = std::thread([this]{this->ExecuteLoop();});
	}

	~TaskProcessor()
	{
		{
			std::lock_guard<Spinlock> lock(m_TasksLock);
			m_Running = false;
		}
		m_Notify.notify_all();

		for (auto &t : m_Threads)
			t.join();
	}

	template<class T, class... Args>
	auto Add(T&& t, Args&&... args)
		-> std::future<typename std::result_of<T(Args...)>::type>
	{
		using return_type = typename std::result_of<T(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>
			(
			std::bind(std::forward<T>(t), std::forward<Args>(args)...)
			);

		auto res = task->get_future();
		{
			std::lock_guard<Spinlock> lock(m_TasksLock);
			m_AllTasks.emplace(m_AllTasks.end(), [task](){ (*task)(); });
		}
		m_Notify.notify_one();

		return res;
	}

private:
	void ExecuteLoop()
	{
		while (true)
		{
			std::unique_lock<Spinlock> lock(m_TasksLock);
			m_Notify.wait(lock, [&](){ return !m_AllTasks.empty() || !m_Running; });

			if (!m_Running && m_AllTasks.empty())
				return;

			auto task = std::move(m_AllTasks.front());
			m_AllTasks.pop_front();
			lock.unlock();

			task();
		}
	}

	std::list<std::function<void()>> m_AllTasks;
	Spinlock m_TasksLock;

	bool 							m_Running;
	std::condition_variable_any		m_Notify;
	std::vector<std::thread> 		m_Threads;
};
//-------------------------------------------------------------------------------------------------