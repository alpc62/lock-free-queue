# lock-free-circular-queue
一个简易、轻便、高性能、无任何依赖的高性能无锁环形队列  
linux平台``boost/lockfree/spsc_queue.hpp``的替换方案

# 使用方法
- 无需安装，仅需要1个头文件
  - C++使用``include/queue62.hpp``(Apache License2.0)
  - C使用``optional/kfifo.h``(GPLv2)
- 使用时请确保初始化的size是2的N次方，附函数按需使用
```
static inline unsigned int _round_up_next_power2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
```

## C++
需要C++11以上，``-std=c++11``即可
```
#include "queue62.hpp"
#include <string>

int main()
{
    spsc_queue<std::string, 1024> que; // single-producer/single-consumer
    //mpmc_queue<std::string, 1024> que; // multi-producers/multi-consumers
    que.push("abc");
    que.push("kfifo");
    que.push("queue");

    while (que.read_available() > 0)
    {
        std::string res;
        int sz = que.pop(&res, 1);
        cout << res << endl;
	  }
}
```
接口设计模仿``boost``的无锁队列``boost/lockfree/spsc_queue.hpp``  
如果你的代码从boost的spsc_queue进行迁移，那么接口名是完全一致的

## 纯C
需要gnu扩展，编译使用``-std=gnu90``即可
```
#include "kfifo.h"
```
具体使用方法可参考操作系统的kfifo，大同小异

# 实现概述
## 单生产单消费
- wait-free, 用内存屏障实现
- 支持non-trivial类型，比如``std::string``等
- 支持批量push/pop，trivial类型使用``memcpy``, non-trivial类型默认使用``std::move``
- 效仿boost spsc_queue接口，接口名和参数列表与boost提供的spsc_queue类似

## 多生产多消费
- lock-free, 用CAS实现
- 支持non-trivial类型，比如``std::string``等
- 存储指针类型时性能最佳，但用户需要自己管理每个元素的申请释放时机
- 非指针类型会有频繁的``new``/``delete``、性能会有所下降，建议使用``-ljemalloc``提高性能
- 不支持批量push/pop
- 效仿std queue接口，接口名和参数列表与std提供的queue类似
- 利用计数和位运算解决ABA问题
