#include "stdafx.h"
#include <string>

#include "TaskProcessor.h"
//-------------------------------------------------------------------------------------------------
class SomeIO
{
public:
	void LongWork()
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));		
		std::cout << std::this_thread::get_id() << std::endl;

		return;
	}

	int LongWorkWithResult(int param)
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return param;
	}

	std::string FuncWithExc(int param)
	{
		if (param % 2 == 0)
			throw std::exception("wow");

		return std::to_string(param);
	}
};
//-------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	TaskProcessor processor;
	SomeIO ioObj;

	//Add and forget
	processor.Add(std::bind(&SomeIO::LongWork, ioObj));
	processor.Add([](){std::cout << "Hello world" << std::endl; });

	//Add and wait for result
	auto fr = processor.Add(std::bind(&SomeIO::LongWorkWithResult, ioObj, 42));
	std::cout << fr.get() << std::endl;

	//Charge tasks
	for (int i = 0; i < 10; ++i)
	{
		auto p = processor.Add(std::bind(&SomeIO::FuncWithExc, ioObj, i));

		try
		{
			std::cout << p.get() << std::endl;
		}
		catch(const std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------
