# ThreadPool
Thread Pool Implementation

functionalities:

add task to thread pool.

dynamic adjust the amount of threads.

automaticly join the threads and free the resources when leave the scope.

functions:

```cpp
ThreadPool(size_t min = 3, size_t max = 100, size_t queueCap = 20);
//min:minimum thread count.  max:maximum thread count. queueSize: maximum task queue size.

std::shared_future addTask(yourFunction,yourArguments...);
std::shared_future addTask(delay, yourFunction,yourArguments...);//you must use std::chrono for the delay.

size_t getWorkingAmount()const;//return amount of working threads.

size_t getExistAmount()const; //return amount of existed threads.

void wait(); //wait for all the working threads to finish their works.

void close(); //close the thread pool, join all the threads.
```
