# nano-coro

`nano-coro` is an experimental c++20 header-only library providing a minimal coroutine system, facilitating cooperative multitasking. `nano-coro` is intended for single threaded use (the provided primitives are not thread-safe) and relies on a "busy" event loop to execute coroutines using a simple work-stealing scheme. Furthermore, `nano-coro` elides all heap allocations by way of allocating memory for coroutine frames from a memory pool using a bump allocator. Therefore, `nano-coro` is effectively syscall free.

## Examples

This is an experimental library: in lieu of proper documentation, a few motivating example usages are provided below.

### Hello world

```c++
#include <nano/buffer.hpp>
#include <nano/continuation.hpp>
#include <nano/executor.hpp>

#include <iostream>

nano::continuation<int> test(nano::coroutine_context) {
    std::cout << "Hello world" << std::endl;
    co_return 0;
}

int main() {
    // Allocate 1-KiB per coroutine stack
    using buffer_type = nano::fixed_size_buffer<1024u>;
    
    // Executor with capacity for one coroutine (1-KiB)
    nano::executor<buffer_type, 1u> executor{};
    [[maybe_unused]] auto continuation = test(executor.find_available_context());

    // Block until execution is complete
    executor.wait();
}

```

Output:
```
Hello world
```

### Lazy and eager (`nano::execution::lazy`, `nano::execution::eager`)

```c++
#include <nano/buffer.hpp>
#include <nano/continuation.hpp>
#include <nano/executor.hpp>

#include <iostream>

// Lazy - this is not the default behavior
nano::continuation<int, nano::execution::lazy> lazy(nano::coroutine_context) {
    std::cout << "lazy" << std::endl;
    co_return 0;
}

// Eager - this is the default behavior
nano::continuation<int, nano::execution::eager> eager(nano::coroutine_context) {
    std::cout << "eager" << std::endl;
    co_return 0;
}


int main() {
    // Allocate 1-KiB per coroutine stack
    using buffer_type = nano::fixed_size_buffer<1024u>;
    
    // Executor with capacity for one coroutine (2-KiB)
    nano::executor<buffer_type, 2u> executor{};

    // Execution will not begin immediately
    [[maybe_unused]] auto continuation_0 = lazy(executor.find_available_context());

    // Execution begins immediately inline with this thread
    [[maybe_unused]] auto continuation_1 = eager(executor.find_available_context());

    // Block until execution is complete
    executor.wait();
}

```

Output:
```
eager
lazy
```

### Cooperatively yield control (`nano::yield`)

```c++
#include <nano/buffer.hpp>
#include <nano/continuation.hpp>
#include <nano/executor.hpp>
#include <nano/yield.hpp>

#include <iostream>

nano::continuation<int> test_0(nano::coroutine_context) {
    std::cout << "Hello ";
    
    // Cooperatively yield control to another coroutine (test_1)
    co_await nano::yield();

    std::cout << " world" << std::endl;
    co_return 0;
}

nano::continuation<int> test_1(nano::coroutine_context) {
    std::cout << "beautiful";
    co_return 0;
}

int main() {
    // Allocate 1-KiB per coroutine stack
    using buffer_type = nano::fixed_size_buffer<1024u>;

    // Executor with capacity for two coroutines (2-KiB)
    nano::executor<buffer_type, 2u> executor{};
    [[maybe_unused]] auto continuation_0 = test_0(executor.find_available_context());
    [[maybe_unused]] auto continuation_1 = test_1(executor.find_available_context());


    // Block until execution is complete
    executor.wait();
}

```

Output:
```
Hello beautiful world
```

### Await asyncronous events (`nano::event`)

```cpp
#include <nano/buffer.hpp>
#include <nano/continuation.hpp>
#include <nano/event.hpp>
#include <nano/executor.hpp>

#include <chrono>
#include <iostream>
#include <thread>


nano::continuation<int> test(nano::coroutine_context, nano::event<int>& event) {
    const int number = co_await event;
    std::cout << number << std::endl;

    co_return 0;
}

int main() {
    // Allocate 1-KiB per coroutine stack
    using buffer_type = nano::fixed_size_buffer<1024u>;

    // Executor with capacity for one coroutine (1-KiB)
    nano::executor<buffer_type, 1u> executor{};

    nano::event<int> test_event{};
    [[maybe_unused]] auto continuation = test(executor.find_available_context(), test_event);

    std::jthread thread([&test_event] {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(5s);
        test_event.send(42);
    });

    // Block until execution is complete
    executor.wait();
}

```

Output:
```
42
```

### Synchronize coroutines (`nano::mutex`)

```cpp
#include <nano/buffer.hpp>
#include <nano/continuation.hpp>
#include <nano/executor.hpp>
#include <nano/mutex.hpp>
#include <nano/yield.hpp>

#include <iostream>

nano::continuation<int> test_0(nano::coroutine_context context, nano::mutex& mutex) {
    // Acquire an exclusive lock on the mutex, released using RAII
    const auto guard = co_await nano::lock(context, mutex);

    std::cout << "Hello ";
    co_await nano::yield();
    std::cout << "beautiful";
    
    co_return 0;
}

nano::continuation<int> test_1(nano::coroutine_context context, nano::mutex& mutex) {
    // Acquire an exclusive lock on the mutex, released using RAII
    const auto guard = co_await nano::lock(context, mutex);

    std::cout << " world" << std::endl;
    co_return 0;
}

int main() {
    // Allocate 1-KiB per coroutine stack
    using buffer_type = nano::fixed_size_buffer<1024u>;

    // Executor with capacity for two coroutines (2-KiB)
    nano::executor<buffer_type, 2u> executor{};

    nano::mutex mutex{};
    [[maybe_unused]] auto continuation_0 = test_0(executor.find_available_context(), mutex);
    [[maybe_unused]] auto continuation_1 = test_1(executor.find_available_context(), mutex);


    // Block until execution is complete
    executor.wait();
}
```

Output:
```
Hello beautiful world
```
