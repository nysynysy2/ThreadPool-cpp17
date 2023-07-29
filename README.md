# Simple but Practical Thread Pool
Get rid of those annoying enhanced functionalities that you don't need!

functions:

```cpp
ThreadPool(size_t thread_count, size_t cap); //cap:maximum capacity of the cache.

std::future addTask(yourFunction,yourArguments...);
std::future addTask_delay(delay, yourFunction,yourArguments...);//you must use std::chrono for the delay.

void wait(); //wait for all the working threads to finish their works.
```
