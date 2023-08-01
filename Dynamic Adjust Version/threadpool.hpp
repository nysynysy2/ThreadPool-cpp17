#ifndef _DYNAMIC_THREAD_POOL_
#define _DYNAMIC_THREAD_POOL_
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
#include <cassert>
#include <algorithm>
class ThreadPool {
public:
	ThreadPool(size_t min = std::thread::hardware_concurrency(), size_t max = 100, bool dynamicAdjustEnable = true, size_t dynamic_dura_ms = 5000, size_t cap = ULLONG_MAX);
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool(const ThreadPool&) = delete;
	~ThreadPool();
	template<class Fn, class... Args> auto addTask(Fn&& func, Args&&... args);
	template<class Fn, class... Args> auto addTask_delay(size_t dura_ms, Fn&& func, Args&&... args);
	size_t getExistThreadCount()const { return exist_count.load(); }
	size_t getWorkingThreadCount()const { return working_count.load(); }
	void setMinThreadCount(size_t min) { assert(min <= max_threads); min_threads = min; }
	void setMaxThreadCount(size_t max) { assert(max >= min_threads); max_threads = max; }
	void setDynamicAdjustEnable(bool enable) { dynamic_adjust_enable = enable; }
	void wait();
	void exit();
private:
	size_t cacheCap;
	std::atomic<size_t> working_count;
	std::atomic<size_t> exist_count;
	std::atomic<size_t> kill_count;
	std::vector<std::thread> threads;
	std::queue< std::packaged_task<void()> > cache;
	std::mutex pool_lock;
	std::condition_variable add_cv, end_cv;
	void _dynamicAdjust();
	void _exec();
	bool stop;
	bool dynamic_adjust_enable;
	size_t min_threads;
	size_t max_threads;
	std::chrono::microseconds adjust_dura;
};
ThreadPool::ThreadPool(size_t min, size_t max, bool dynamicAdjustEnable, size_t dynamic_dura_ms, size_t cap)
	:stop(false), cacheCap(cap), adjust_dura(std::chrono::milliseconds(dynamic_dura_ms)), min_threads(min), max_threads(max), dynamic_adjust_enable(dynamicAdjustEnable) {
	assert(min <= max);
	exist_count.store(0);
	working_count.store(0);
	kill_count.store(0);
	while (min--) {
		threads.emplace_back(&ThreadPool::_exec, this);
		++exist_count;
	}
	if (dynamic_adjust_enable) threads.emplace_back(&ThreadPool::_dynamicAdjust, this);
}
template<class Fn, class... Args> auto ThreadPool::addTask(Fn&& func, Args&&... args) {
	auto ptr = std::make_shared< std::packaged_task<typename std::invoke_result<Fn, Args...>::type()> >(
		std::bind(std::forward<Fn>(func), std::forward<Args>(args)...)
	);
	auto ret = ptr->get_future();
	std::unique_lock<std::mutex> locker(pool_lock);
	end_cv.wait(locker, [=] {return cache.size() <= cacheCap; });
	cache.emplace([=]()mutable {(*ptr)(); });
	locker.unlock();
	add_cv.notify_one();
	return ret;
}
template<class Fn, class... Args> auto ThreadPool::addTask_delay(size_t delay_ms, Fn&& func, Args&&... args) {
	return addTask([&, delay_ms] {std::this_thread::sleep_for(std::chrono::microseconds(delay_ms)); return std::forward<Fn>(func)(std::forward<Args>(args)...); });
}
void ThreadPool::_exec() {
	while ((!stop) && dynamic_adjust_enable) {
		std::unique_lock<std::mutex> locker(pool_lock);
		add_cv.wait(locker, [=] {return !(cache.empty()) || stop || kill_count.load() != 0; });
		if (stop) return;
		if (kill_count.load() != 0) {
			--kill_count;
			return;
		}
		auto task = std::move(cache.front());
		cache.pop();
		locker.unlock();
		++working_count;
		task();
		--working_count;
		end_cv.notify_all();
	}
}
void ThreadPool::wait() {
	if (stop) return;
	std::unique_lock<std::mutex> locker(pool_lock);
	end_cv.wait(locker, [=] {return stop || (working_count == 0 && cache.empty()); });
}
void ThreadPool::_dynamicAdjust() {
	while (!stop) {
		std::unique_lock<std::mutex> locker(pool_lock);
		if (cache.size() > exist_count && exist_count < max_threads) {
			size_t adjust_count = std::min(cache.size(), max_threads - exist_count.load());
			for (size_t count = 0; count < adjust_count; ++count) {
				threads.emplace_back(&ThreadPool::_exec, this);
				++exist_count;
			}
			locker.unlock();
		}
		else if (cache.empty() && exist_count.load() > working_count.load() * 2 && exist_count > min_threads) {
			size_t adjust_count = std::min(exist_count.load() - working_count.load(), exist_count.load() - min_threads);
			kill_count += adjust_count;
			locker.unlock();
			add_cv.notify_all();
			exist_count -= adjust_count;
		}
		std::this_thread::sleep_for(adjust_dura);
	}
}
void ThreadPool::exit() {
	if (!stop) {
		stop = true;
		add_cv.notify_all();
		for (auto& t : threads) t.join();
	}
}
ThreadPool::~ThreadPool() {
	if (!stop) {
		stop = true;
		add_cv.notify_all();
		for (auto& t : threads) t.join();
	}
}
#endif
