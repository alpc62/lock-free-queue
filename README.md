# lock-free-queue
一个简易、轻便、高性能、无任何依赖，基于``linux kernel kfifo``改编实现的高性能无锁队列  
linux平台``boost/lockfree/spsc_queue.hpp``的替换方案

# 使用方法
- 无需安装，仅kfifo.h和queue62.hpp两个头文件
- 使用时务必自己确保初始化的size是2的N次方，暂时没有做额外的判断、提示或容错
- 如果大小不符合2的N次方的要求，任何行为都无法预计

## C++
需要C++11以上，``-std=c++11``即可  
由于模板中使用的std::is_pod特性在c++20被废弃、固暂不支持c++20，待修复[TODO]
```
#include "queue62.hpp"
#include <string>

int main()
{
    spsc_queue<std::string, 1024> que;
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
- 以``linux kernel first-input-first-output queue``的源码``include/linux/kfifo.h``和``kernel/kfifo.c``改编
- 保持原作的无锁特性
- POD支持变长
- 扩充非POD类型的支持、比如``std::string``等
- 非POD仅支持push(1)，pop(N)，且默认使用``std::move``进行pop操作以尽可能减少不必要的内存拷贝

## 多生产多消费
- 非无锁，目前版本使用自旋锁
- 支持非POD
- 效仿``include/linux/kfifo.h``使用``pthread_spinlock_t``上锁
- 后续考虑使用CAS进一步提高性能，以实现真正的无锁队列 -- [TODO]
