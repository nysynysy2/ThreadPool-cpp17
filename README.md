# Header Only DynamicAdjust LightWeightl Thread Pool (C++17)
```cpp
ThreadPool(size_t thread_count = std::thread::hardware_concurrency(), bool adjust_enabled = true, size_t max_thread = 100, size_t min_thread = 1, size_t manage_duration_ms = 3000)

add_task(Fn func, Args&&... args)
add_task_delay(size_t delay_ms, Fn func, Args&&... args)

wait()

stop_and_join()
stop_and_detach()
```
