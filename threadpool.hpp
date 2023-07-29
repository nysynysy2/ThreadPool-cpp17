#ifndef _THREAD_POOL_
#define _THREAD_POOL_
#include <vector>
#include <queue>
#include <thread>
#include <iostream>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <list>
#include <type_traits>
#include <functional>
#include <future>
#include <cassert>
#include <algorithm>
enum class PoolStatus {shutdown = 0, working};
enum class WorkerStatus { terminated = 0, working, sleeping };
class ThreadPool
{
	friend class _Worker;
	std::queue<std::packaged_task<void(void)>> taskQueue;
	size_t queueCapacity;
	std::atomic<size_t> queueSize;
	
	std::thread::id managerId;
	std::vector<std::thread::id> workerIds;
	
	std::vector<_Worker> workers;
	std::vector<std::thread> threadPool;

	size_t minThreadAmount;
	size_t maxThreadAmount;

	std::atomic<size_t> workingAmount;
	std::atomic<size_t> existAmount;
	std::atomic<size_t> adjustAmount;

	std::atomic<bool> dynamicAdjustEnable;

	std::mutex poolMutex;
	std::condition_variable addTaskCV;
	std::condition_variable endTaskCV;

	std::atomic<PoolStatus> m_status;

public: 
	explicit ThreadPool(size_t min = 3, size_t max = 100, size_t queueCap = 20);
	~ThreadPool();
	
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	
	template<class Fn, class... Args>
	std::shared_future<typename std::invoke_result<Fn, Args...>::type> addTask(Fn func, Args&&... args);

	template<class Fn, class... Args, class Dura>
	std::shared_future<typename std::invoke_result<Fn, Args...>::type> addTask(Dura dura, Fn func, Args&&... args);

	size_t getWorkingAmount()const;
	size_t getExistAmount()const;

	void setDynamicAdjust(bool DAE);

	void wait();
	void close();
private:
	void _adjustThreadAmount();
};


class _Worker
{
public:
	ThreadPool* m_parentPool;
	std::atomic<WorkerStatus> m_status;
	_Worker(ThreadPool* parentPool);
	_Worker(const _Worker& other);
	void execThread();
};

_Worker::_Worker(ThreadPool* parentPool)
{
	this->m_parentPool = parentPool;
	m_status.store(WorkerStatus::sleeping);
}

_Worker::_Worker(const _Worker& other)
{
	m_parentPool = other.m_parentPool;
	m_status.store(other.m_status);
}

void _Worker::execThread()
{
	while (m_status.load() != WorkerStatus::terminated)
	{
		std::unique_lock<std::mutex> locker(m_parentPool->poolMutex);
		m_parentPool->addTaskCV.wait(locker, [=] {return !(this->m_parentPool->taskQueue.empty()) 
			|| this->m_status.load() == WorkerStatus::terminated; 
		});
		if (this->m_status.load() == WorkerStatus::terminated) return;
		this->m_status.store(WorkerStatus::working);
		std::packaged_task<void(void)> task = std::move(this->m_parentPool->taskQueue.front());
		this->m_parentPool->taskQueue.pop();
		locker.unlock();
		++(this->m_parentPool->workingAmount);
		task();
		--(this->m_parentPool->workingAmount);
		this->m_status.store(WorkerStatus::sleeping);
		this->m_parentPool->endTaskCV.notify_all();
	}
}


ThreadPool::ThreadPool(size_t min, size_t max, size_t queueCap)
{
	assert(min <= max);
	this->minThreadAmount = min;
	this->maxThreadAmount = max;
	this->existAmount.store(min);
	this->queueCapacity = queueCap;
	this->queueSize.store(0);
	this->m_status.store(PoolStatus::working);
	while (min--) 
	{
		this->workers.push_back(_Worker(this));
		this->threadPool.push_back(std::thread(&_Worker::execThread, &(this->workers.back())));
	}
}

template<class Fn, class... Args>
std::shared_future<typename std::invoke_result<Fn, Args...>::type> ThreadPool::addTask(Fn func, Args&&... args)
{
	std::future<typename std::invoke_result<Fn, Args...>::type> m_future = std::async(std::launch::deferred, func, std::forward<Args>(args)...);
	std::shared_future<typename std::invoke_result<Fn, Args...>::type> s_future = m_future.share();
	auto f = [s_future]()mutable {s_future.get(); };
	std::packaged_task<void(void)> task(f);
	std::unique_lock<std::mutex> locker(this->poolMutex);
	this->endTaskCV.wait(locker, [=] {return this->taskQueue.size() <= this->queueCapacity; });
	this->taskQueue.push(std::move(task));
	locker.unlock();
	this->addTaskCV.notify_one();
	return s_future;
}

template<class Fn, class... Args, class Dura>
std::shared_future<typename std::invoke_result<Fn, Args...>::type> ThreadPool::addTask(Dura dura, Fn func, Args&&... args)
{
	return this->addTask([&] {std::this_thread::sleep_for(dura);
		return func(std::forward<Args>(args)...); 
	});
}

size_t ThreadPool::getWorkingAmount()const
{
	return this->workingAmount.load();
}
size_t ThreadPool::getExistAmount()const
{
	return this->workers.size();
}
void ThreadPool::setDynamicAdjust(bool DAE)
{
	//TODO: Enable and Disable dynamic adjustion of thread amount.
	this->dynamicAdjustEnable.store(DAE);
}
void ThreadPool::wait()
{
	if (this->m_status.load() == PoolStatus::shutdown) return;
	std::unique_lock<std::mutex> locker(this->poolMutex);
	this->endTaskCV.wait(locker, [=] {return this->taskQueue.empty() && (this->workingAmount.load() == 0); });
}
void ThreadPool::close()
{
	this->m_status.store(PoolStatus::shutdown);
	for (auto& e : this->workers)
	{
		e.m_status.store(WorkerStatus::terminated);
	}
	this->addTaskCV.notify_all();
	for (auto& e : this->threadPool)
	{
		e.join();
	}
}
void ThreadPool::_adjustThreadAmount()
{
	while (this->m_status.load() != PoolStatus::shutdown)
	{
		std::unique_lock<std::mutex> locker(this->poolMutex);
		this->addTaskCV.wait(locker, [=] {return (this->taskQueue.size() >= this->existAmount.load() * 2 && this->existAmount.load() != this->maxThreadAmount) 
			|| (this->workingAmount.load() * 2 <= this->existAmount.load() && this->workingAmount.load() != this->minThreadAmount ); });
		if (this->taskQueue.size() >= this->existAmount.load() * 2)
		{
			this->adjustAmount.store(this->existAmount.load());
			size_t adj = this->maxThreadAmount - this->existAmount.load() > this->adjustAmount.load() ? this->adjustAmount.load() : this->maxThreadAmount - this->existAmount.load();
			for (int i = 1; i <= adj; ++i)
			{
				this->workers.emplace_back(this);
				this->threadPool.push_back(std::thread(&_Worker::execThread, &(this->workers.back())));
				++(this->existAmount);
				if (this->existAmount.load() == maxThreadAmount) return;
			}
		}
		else
		{
			this->adjustAmount.store(this->existAmount.load() - this->workingAmount.load());
			size_t adj = this->existAmount.load() - this->minThreadAmount > this->adjustAmount.load() ? this->adjustAmount.load() : this->existAmount.load() - this->minThreadAmount;
			for (int i = 1; i <= adj; ++i)
			{
				this->workers[this->existAmount.load() - i].m_status.store(WorkerStatus::terminated);
			}
			locker.unlock();
			this->addTaskCV.notify_all();
			for (int i = 1; i <= adj; ++i)
			{
				this->threadPool[this->existAmount.load() - i].join();
				--(this->existAmount);
			}
		}
	}
}
ThreadPool::~ThreadPool()
{
	if (this->m_status.load() != PoolStatus::shutdown)
	{
		this->m_status.store(PoolStatus::shutdown);
		for (auto& e : this->workers)
		{
			e.m_status.store(WorkerStatus::terminated);
		}
		this->addTaskCV.notify_all();
		for (auto& e : this->threadPool)
		{
			e.join();
		}
	}
}
#endif
