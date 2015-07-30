
//
// Synchronization Primitives
//
// Copyright (c) 2015  Dmitry Popov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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