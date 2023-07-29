#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
class ThreadPool {
public:
	ThreadPool(size_t thread_count = std::thread::hardware_concurrency(), size_t cap = ULLONG_MAX);
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool(const ThreadPool&) = delete;
	~ThreadPool();
	template<class Fn, class... Args> auto addTask(Fn&& func, Args&&... args);
	template<class Fn, class... Args, class Dura> auto addTask_delay(Dura dura, Fn&& func, Args&&... args);
	void wait();
private:
	size_t cacheCap;
	std::atomic<size_t> working_count;
	std::vector<std::thread> threads;
	std::queue< std::packaged_task<void()> > cache;
	std::mutex pool_lock;
	std::condition_variable add_cv;
	std::condition_variable end_cv;
	void _exec();
	bool stop;
};
ThreadPool::ThreadPool(size_t thread_count, size_t cap) :stop(false), cacheCap(cap) {
	while (thread_count--) threads.emplace_back(&ThreadPool::_exec,this);
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
template<class Fn, class... Args, class Dura> auto ThreadPool::addTask_delay(Dura dura, Fn&& func, Args&&... args) {
	return addTask([&] {std::this_thread::sleep_for(dura); return std::forward<Fn>(func)(std::forward<Args>(args)...); });
}
void ThreadPool::_exec() {
	while (!stop) {
		std::unique_lock<std::mutex> locker(pool_lock);
		add_cv.wait(locker, [=] {return !(cache.empty()) || stop; });
		if (stop) return;
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
	std::unique_lock<std::mutex> locker(pool_lock);
	end_cv.wait(locker, [=] {return working_count == 0 && cache.empty(); });
}
ThreadPool::~ThreadPool() {
	stop = true;
	add_cv.notify_all();
	for (auto& t : threads) t.join();
}
