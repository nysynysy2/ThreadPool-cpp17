# Header Only DynamicAdjust LightWeightl Thread Pool (C++17)
Easy to use, simple, light-weight header only


Functionalities:

1, Dynamic Adjust Thread Count

2, Create Delay Task

3, Automatic Manage Threads' Resources


Usage:

Dynamic Adjustion Version:
```cpp
ThreadPool(size_t thread_count, size_t min, size_t max, bool ddjust_enable, size_t adjust_duration_ms);
//adjust_duration: duration for the dynamic adjustion cycle(milliseconds).

std::shared_future addTask(yourFunction,yourArguments...);
std::shared_future addTask(size_t delay_ms, yourFunction,yourArguments...);//milliseconds for the delay.

void wait(); //wait for all the working threads to finish their works.
void stop(); //stop the thread pool;

size_t get_alive_thread_count();
size_t get_working_thread_count();
void set_min_thread_count(size_t min);
void set_max_thread_count(size_t max);

void set_adjust_enabled(bool enabled);
bool is_adjust_enabled();
```

Simple Version:
```cpp
ThreadPool(size_t threadCount, size_t cap);
//cap:maximum capacity of the cache.

std::future addTask(yourFunction,yourArguments...);
std::future addTask_delay(size_t delay_ms, yourFunction,yourArguments...);//milliseconds for the delay.

void wait(); //wait for all the working threads to finish their works.
```
