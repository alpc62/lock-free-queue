# Non-Blocking Lock-Free/Wait-Free Circular-Queue
## spsc_queue
- A single-producer/single-consumer FIFO circular queue
- **wait-free**, non-blocking
- **Only use memory fence**. No lock. No CAS(so No ABA problem). No atomic.
- Simple / Lightweight / **High-performance without any dependencies**
- **Support non-trivial** types，such as ``std::string``
- **Support batch** push/pop, use ``memcpy`` for trivial types, use ``std::move`` for non-trivial types
- A great replacement scheme of ``boost/lockfree/spsc_queue.hpp`` on linux platform

## mpmc_queue
- A multi-producers/multi-consumers FIFO circular queue
- **lock-free**, non-blocking
- Use CAS but **No ABA problem**, solving ABA by counting and bit operations
- Simple / Lightweight **without any dependencies**
- **Support non-trivial** types，such as ``std::string``
- Best performance when storing pointer types
- Uncertain Performance when storing non-pointer types
  - Because non-pointer types need call ``new``/``delete`` very frequently
  - It is recommended to use ``-ljemalloc`` to improve performance for non-pointer types

# Tutorial
- used directly by include header file
  - C++ ``include/queue62.hpp`` (Apache License2.0)
  - C ``optional/kfifo.h`` (GPLv2)
- Please make sure that the initialized capacity is power of 2, here is the helper function:
```
static inline unsigned int _round_up_next_power2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}
```

## C++
- Compile required at least ``-std=c++11``
- The interface design imitates the ``boost`` lock-free queue ``boost/lockfree/spsc_queue.hpp``
- If your code is migrated from spsc_queue of ``boost``, then the interface name is exactly the same
```
#include <iostream>
#include <string>
#include "queue62.hpp"

int main()
{
    spsc_queue<std::string, 128> que; // single-producer/single-consumer
    mpmc_queue<std::string, 1024> _q; // multi-producers/multi-consumers

    que.push("abc");
    que.push("kfifo");
    que.push("queue");

    while (que.read_available() > 0)
    {
        std::string res[4];
        int cnt = que.pop(res, 4); // here cnt result is 3

        for (int i = 0; i < cnt; i++)
          std::cout << res[i] << std::endl;
    }
}
```

## C
- Compile required at least ``-std=gnu90``
- The specific usage method is similar to the ``kfifo`` of linux kernel
```
#include "kfifo.h"
```

# Author
- Wu Jiaxu (void00@foxmail.com)
