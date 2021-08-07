/*
Author: Dan Rehberg
Date Modified: 8/5/2021

Notes: While from a single use case, creating the unique locks in the sleep sync method
		can be seen as taking an exorbitant amount of time, empirically testing the 
		single creation of unique locks (for the main and Daemon threads) proved to average
		slightly worse performance.
	It seems reasonably to conclude good caching over repeated use of the dispatch system
		might be keeping the seeming cost of creation per usage low.
	I.E. do not pre-optimize this - it is stable and fast as is for the sleep sync method.
	Also, more effort is required to ensure the correct ownership of the shared mutex if
		creating the unique locks only once.
	This also includes making sure the main thread unlocks its unique lock in the
		destructor to successfully allow the Daemon threads to close.
	Furthermore, compiler warnings will likely arise because it will be trivial to detect
		that a unique lock for the main thread being created once - thereby requiring it
		to exist for the duration of this classes existence - will not be released at the
		end of a local scope!
*/

#include "ThreadPool.hpp"
#include <iostream>

ThreadPool::ThreadPool() : threadCount(std::thread::hardware_concurrency())
{
	threads = new std::thread[threadCount];
	completeCounter.store(0, std::memory_order_relaxed);
	terminated.store(0, std::memory_order_relaxed);
	for (unsigned int i = 0; i < threadCount; ++i)
	{
		threads[i] = std::move(std::thread(&ThreadPool::g, this, i));
	}
	for (unsigned int i = 0; i < threadCount; ++i)
	{
		threads[i].detach();//daemon threads
	}
}

ThreadPool::ThreadPool(unsigned int threadCount) : threadCount(threadCount)
{
	threads = new std::thread[this->threadCount];
	completeCounter.store(0, std::memory_order_relaxed);
	terminated.store(0, std::memory_order_relaxed);
	for (unsigned int i = 0; i < this->threadCount; ++i)
	{
		threads[i] = std::move(std::thread(&ThreadPool::g, this, i));
	}
	for (unsigned int i = 0; i < this->threadCount; ++i)
	{
		threads[i].detach();//daemon threads
	}
}

ThreadPool::~ThreadPool()
{
	if (terminated.load(std::memory_order_relaxed) != (threadCount - 1))
	{
		close = true;
		N = 0;
		n = 0;
		blockIsFinished = true;
		blockIsStarted = true;
		blockStart.notify_all();
		blockFinish.notify_all();
	}
	while (terminated.load(std::memory_order_relaxed) != threadCount)
	{
	}
	f = nullptr;
	delete[] threads;
}

void ThreadPool::dispatch(uint32_t taskCount, void(*task)(std::mutex&, unsigned int))
{
	if (!init)initialized();
	N = taskCount;
	n = (N + (threadCount - 1)) / threadCount;
	if (task == nullptr)
	{
		std::cerr << "Invalid function given to dispatch call\n";
		return;
	}
	f = task;
	blockIsFinished = false;
	blockIsMain = false;// true;
	completeCounter.store(0, std::memory_order_relaxed);
	//Threads GO from Starting Line
	blockIsStarted = true;
#if SLEEP_OR_SPIN
	while (!blockIsMain)
	{
	}//Spin until thread pool at finish line
#else
	std::unique_lock<std::mutex> lock(lockThreads);
	blockStart.notify_all();//Always create the lock before notifying the threads; it seems either like lock creation takes extremely long, or it is important to the shared mutex
	blockMain.wait(lock, [&]() { return blockIsMain; });
#endif
	//Threads are waiting at Finish Line
	blockIsStarted = false;
	blockIsMain = false;// true;
	completeCounter.store(0, std::memory_order_relaxed);
	blockIsFinished = true;
#if SLEEP_OR_SPIN
	while (!blockIsMain)
	{
	}//Spin until thread pool at finish line
#else
	blockFinish.notify_all();
	blockMain.wait(lock, [&]() { return blockIsMain; });
#endif
	return;
}

void ThreadPool::g(const unsigned int i)//the ith thread in the argument
{
	while (!close)
	{
		//Starting line
		if (completeCounter.fetch_add(1, std::memory_order_relaxed) == (threadCount - 1))
		{
			blockIsMain = true;// false;
			blockMain.notify_one();
		}
#if SLEEP_OR_SPIN
		while (!blockIsStarted)
		{
		}//Spin in here
#else
		{
			std::unique_lock<std::mutex> lock(lockThreads);
			blockStart.wait(lock, [&]() {return blockIsStarted; });
		}
#endif
		//distribute tasks
		uint32_t t0 = static_cast<uint32_t>(i)* n;
		uint32_t t1 = t0 + n;
		if (t0 >= N)
		{
			t0 = 0;
			t1 = 0;
		}
		else if (t1 > N)
		{
			t1 = N;
		}
		//Execute on tasks
		for (uint32_t j = t0; j < t1; ++j)
		{
			f(lockShared, j);
		}

		//Finish line
		if (completeCounter.fetch_add(1, std::memory_order_relaxed) == (threadCount - 1))
		{
			blockIsMain = true;// false;
			blockMain.notify_one();
		}
#if SLEEP_OR_SPIN
		while (!blockIsFinished)
		{
		}//Spin in here
#else
		{
			std::unique_lock<std::mutex> lock(lockThreads);
			blockFinish.wait(lock, [&]() {return blockIsFinished; });
		}
#endif
	}
	terminated.fetch_add(1, std::memory_order_relaxed);
	return;
}

void ThreadPool::initialized()
{
	if (!init)while (!blockIsMain) {}
	init = true;
	return;
}