/*
Author: Dan Rehberg
Date Modified: 8/4/2021
*/

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <atomic>
#include <condition_variable>//setting threads to sleep
#include <cstdint>
#include <mutex>
#include <thread>

//0 to sleep, 1 to spin - this is just which mechanism to synchronize threads
#define SLEEP_OR_SPIN 1

class ThreadPool final
{
public:
	ThreadPool();//To let the class decide the size of the thread pool
	ThreadPool(unsigned int threadCount);//Manually set size of the thread pool
	~ThreadPool();
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	void dispatch(uint32_t taskCount, void(*task)(std::mutex&, unsigned int));
	void initialized();
private:
	std::condition_variable blockFinish;
	volatile bool blockIsFinished = false;
	volatile bool blockIsMain = false;
	volatile bool blockIsStarted = false;
	std::atomic_bool mainRest = false;
	std::condition_variable blockMain;
	std::condition_variable blockStart;
	volatile bool close = false;//If the thread pool needs to stop running -- set destructor explicitly to verify threads terminated before destroying threads array
	std::atomic_uint32_t completeCounter;//How many threads have finished something...
	void(*f)(std::mutex&, unsigned int) = nullptr;
	void g(const unsigned int i);//The function for the thread(s) to exist in until the program needs to close
	bool init = false;
	std::mutex lockShared;
	std::mutex lockThreads;
	volatile uint32_t n = 0;//This is the number of tasks a thread might work on - maximum
	volatile uint32_t N = 0;//Total number of tasks in a Dispatch call
	std::atomic_uint32_t terminated;
	const unsigned int threadCount;
	std::thread* threads = nullptr;//Thread pool itself
};

#endif