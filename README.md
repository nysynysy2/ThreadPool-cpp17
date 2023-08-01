# Simple Dynamic-Adjust Super-Light-Weightl Thread Pool (C++17)
Easy to use, simple, light-weight (Only One header File!)


Functionalities:

1, Dynamic Adjust Thread Count

2, Create Delay Task

3, Automatic Manage Threads' Resources


Usage:

Dynamic Adjustion Version:
```cpp
ThreadPool(size_t min, size_t max, bool dynamicAdjustEnable, size_t dynamic_dura_ms, size_t cap);
//cap:maximum capacity of the cache.
//dynamic_dura_ms: duration for the dynamic adjustion cycle(milliseconds).

std::future addTask(yourFunction,yourArguments...);
std::future addTask_delay(size_t delay_ms, yourFunction,yourArguments...);//milliseconds for the delay.

void wait(); //wait for all the working threads to finish their works.
void close(); //close the thread pool;

size_t getExistThreadCount();
size_t getWorkingThreadCount();
void setMinThreadCount(size_t min);
void setMaxThreadCount(size_t max);

void setDynamicAdjustEnable(bool enable);
bool isDynamicAdjustEnable();
```

Simple Version:
```cpp
ThreadPool(size_t threadCount, size_t cap);
//cap:maximum capacity of the cache.

std::future addTask(yourFunction,yourArguments...);
std::future addTask_delay(size_t delay_ms, yourFunction,yourArguments...);//milliseconds for the delay.

void wait(); //wait for all the working threads to finish their works.
```
