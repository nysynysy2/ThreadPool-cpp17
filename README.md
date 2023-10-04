# Header Only DynamicAdjust LightWeightl Thread Pool (C++17)
```cpp
ThreadPool(size_t thread_count = std::thread::hardware_concurrency(), bool adjust_enabled = true, size_t max_thread = 100, size_t min_thread = 1, size_t manage_duration_ms = 3000)

std::shared_future<typename std::invoke_result<Fn, Args...>::type> add_task(Fn func, Args&&... args)
std::shared_future<typename std::invoke_result<Fn, Args...>::type> add_task_delay(size_t delay_ms, Fn func, Args&&... args)

void wait()

void stop_and_join()
void stop_and_detach()

void set_adjust_enabled(bool enabled)

void set_max_thread_count(size_t val)
void set_min_thread_count(size_t val)

size_t get_max_thread_count()const
size_t get_min_thread_count()const

size_t get_working_thread_count()const
size_t get_alive_thread_count()const
bool is_adjust_enabled()const

bool is_stopped()const
```
