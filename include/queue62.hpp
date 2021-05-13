/*
  Copyright (c) 2021 wujiaxu <void00@foxmail.com>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <functional>
#include <new>
#include <utility>

#define __CHECK_POWER_OF_2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

namespace { // not for user
template <typename T, unsigned int capacity>
class __spsc_queue;
template <typename T, unsigned int capacity>
class __mpmc_queue;
}

// replace boost/lockfree/spsc_queue.hpp
// The spsc_queue class provides a single-producer/single-consumer fifo queue
// pushing and popping is wait-free
template <typename T, unsigned int capacity>
class spsc_queue
{
public:
    spsc_queue()  { }
    ~spsc_queue() { }
    spsc_queue(const spsc_queue&) = delete;
    spsc_queue(spsc_queue&&) = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;
    spsc_queue& operator=(spsc_queue&&) = delete;

public:
    int read_available() const;

    bool push(const T& t);
    bool push(T&& t);
    bool pop(T& ret);

    int push(const T *ret, int n);
    int pop(T *ret, int n);

private:
    __spsc_queue<T, capacity> queue_;
};

// thread-safety multi-producer/multi-consumer circular-queue
// The mpmc_queue class provides a multi-producers/multi-consumers fifo queue
// pushing and popping is lock-free (NOT wait-free, implemented using CAS)
template <typename T, unsigned int capacity>
class mpmc_queue
{
public:
    mpmc_queue()  { }
    ~mpmc_queue() { }
    mpmc_queue(const mpmc_queue&) = delete;
    mpmc_queue(mpmc_queue&&) = delete;
    mpmc_queue& operator=(const mpmc_queue&) = delete;
    mpmc_queue& operator=(mpmc_queue&&) = delete;

public:
    bool empty() const;
    size_t size() const;

    bool push(const T& t);
    bool push(T&& t);
    bool pop(T& ret);

private:
    __mpmc_queue<T, capacity> queue_;
};

////
// template inl, not for user
template <typename T, unsigned int capacity>
int spsc_queue<T, capacity>::read_available() const
{
    return queue_.read_available();
}

template <typename T, unsigned int capacity>
bool spsc_queue<T, capacity>::push(const T& t)
{
    return queue_.push(t);
}

template <typename T, unsigned int capacity>
bool spsc_queue<T, capacity>::push(T&& t)
{
    return queue_.push(std::move(t));
}

template <typename T, unsigned int capacity>
bool spsc_queue<T, capacity>::pop(T& t)
{
    return queue_.pop(t);
}

template <typename T, unsigned int capacity>
int spsc_queue<T, capacity>::push(const T *ret, int n)
{
    return queue_.push(ret, n);
}

template <typename T, unsigned int capacity>
int spsc_queue<T, capacity>::pop(T *ret, int n)
{
    return queue_.pop(ret, n);
}

template <typename T, unsigned int capacity>
bool mpmc_queue<T, capacity>::empty() const
{
    return queue_.empty();
}

template <typename T, unsigned int capacity>
size_t mpmc_queue<T, capacity>::size() const
{
    return queue_.size();
}

template <typename T, unsigned int capacity>
bool mpmc_queue<T, capacity>::push(const T& t)
{
    return queue_.push(t);
}

template <typename T, unsigned int capacity>
bool mpmc_queue<T, capacity>::push(T&& t)
{
    return queue_.push(std::move(t));
}

template <typename T, unsigned int capacity>
bool mpmc_queue<T, capacity>::pop(T& t)
{
    return queue_.pop(t);
}

namespace {
struct __fifo
{
    unsigned int in;
    unsigned int out;
    unsigned int mask;
    unsigned int size;
    void *buffer;
};

template <typename T, bool is_trivial = std::is_trivial<T>::value>
class __spsc_worker;

template <typename T, unsigned int capacity>
class __spsc_queue
{
public:
    __spsc_queue();
    ~__spsc_queue() { }
    __spsc_queue(const __spsc_queue&) = delete;
    __spsc_queue(__spsc_queue&&) = delete;
    __spsc_queue& operator=(const __spsc_queue&) = delete;
    __spsc_queue& operator=(__spsc_queue&&) = delete;

public:
    int read_available() const;

    bool push(const T& t);
    bool push(T&& t);
    bool pop(T& ret);

    int push(const T *ret, int n);
    int pop(T *ret, int n);

private:
    __fifo fifo_;
    T arr_[capacity];

    using WORKER = __spsc_worker<T, std::is_trivial<T>::value>;
    static_assert(__CHECK_POWER_OF_2(capacity), "Capacity MUST power of 2");
};

template <typename T, unsigned int capacity>
__spsc_queue<T, capacity>::__spsc_queue()
{
    fifo_.in = 0;
    fifo_.out = 0;
    fifo_.mask = capacity - 1;
    fifo_.size = capacity;
    fifo_.buffer = &arr_;
}

template <typename T, unsigned int capacity>
int __spsc_queue<T, capacity>::read_available() const
{
    return fifo_.in - fifo_.out;
}

template <typename T, unsigned int capacity>
bool __spsc_queue<T, capacity>::push(const T& t)
{
    if (capacity - fifo_.in + fifo_.out == 0)
        return false;

    arr_[fifo_.in & (capacity - 1)] = t;

    asm volatile("sfence" ::: "memory");

    ++fifo_.in;

    return true;
}

template <typename T, unsigned int capacity>
bool __spsc_queue<T, capacity>::push(T&& t)
{
    if (capacity - fifo_.in + fifo_.out == 0)
        return false;

    arr_[fifo_.in & (capacity - 1)] = std::move(t);

    asm volatile("sfence" ::: "memory");

    ++fifo_.in;

    return true;
}

template <typename T, unsigned int capacity>
bool __spsc_queue<T, capacity>::pop(T& t)
{
    if (fifo_.in - fifo_.out == 0)
        return false;

    t = std::move(arr_[fifo_.out & (capacity - 1)]);

    asm volatile("sfence" ::: "memory");

    ++fifo_.out;

    return true;
}

template <typename T, unsigned int capacity>
int __spsc_queue<T, capacity>::push(const T *ret, int n)
{
    return WORKER::push(&fifo_, ret, n);
}

template <typename T, unsigned int capacity>
int __spsc_queue<T, capacity>::pop(T *ret, int n)
{
    return WORKER::pop(&fifo_, ret, n);
}

static inline unsigned int _min(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

template <typename T>
class __spsc_worker<T, true>
{
public:
    static int push(__fifo *fifo, const T *ret, int n)
    {
        unsigned int len = _min(n, fifo->size - fifo->in + fifo->out);
        if (len == 0)
            return 0;

        unsigned int idx_in = fifo->in & fifo->mask;
        unsigned int l = _min(len, fifo->size - idx_in);
        T *arr = (T *)fifo->buffer;

        memcpy(arr + idx_in, ret, l * sizeof (T));
        memcpy(arr, ret + l, (len - l) * sizeof (T));

        asm volatile("sfence" ::: "memory");

        fifo->in += len;

        return len;
    }

    static int pop(__fifo *fifo, T *ret, int n)
    {
        unsigned int len = _min(n, fifo->in - fifo->out);
        if (len == 0)
            return 0;

        unsigned int idx_out = fifo->out & fifo->mask;
        unsigned int l = _min(len, fifo->size - idx_out);
        T *arr = (T *)fifo->buffer;

        memcpy(ret, arr + idx_out, l * sizeof (T));
        memcpy(ret + l, arr, (len - l) * sizeof (T));

        asm volatile("sfence" ::: "memory");

        fifo->out += len;

        return len;
    }
};

template <typename T>
class __spsc_worker<T, false>
{
public:
    static int push(__fifo *fifo, const T *ret, int n)
    {
        unsigned int len = _min(n, fifo->size - fifo->in + fifo->out);
        if (len == 0)
            return 0;

        unsigned int idx_in = fifo->in & fifo->mask;
        unsigned int l = _min(len, fifo->size - idx_in);
        T *arr = (T *)fifo->buffer;

        for (unsigned int i = 0; i < l; i++)
            arr[idx_in + i] = ret[i];

        for (unsigned int i = 0; i < len - l; i++)
            arr[i] = ret[l + i];

        asm volatile("sfence" ::: "memory");

        fifo->in += len;

        return len;
    }

    static int pop(__fifo *fifo, T *ret, int n)
    {
        unsigned int len = _min(n, fifo->in - fifo->out);
        if (len == 0)
            return 0;

        unsigned int idx_out = fifo->out & fifo->mask;
        unsigned int l = _min(len, fifo->size - idx_out);
        T *arr = (T *)fifo->buffer;

        for (unsigned int i = 0; i < l; i++)
            ret[i] = std::move(arr[idx_out + i]);

        for (unsigned int i = 0; i < len - l; i++)
            ret[l + i] = std::move(arr[i]);

        asm volatile("sfence" ::: "memory");

        fifo->out += len;

        return len;
    }
};

struct __atomic_fifo
{
    unsigned int mask;
    unsigned int size;
    uint64_t *buffer;
    std::atomic<unsigned int> in;
    std::atomic<unsigned int> out;
};

template <typename T, bool is_pointer = std::is_pointer<T>::value>
class __mpmc_worker;

template <typename T, unsigned int capacity>
class __mpmc_queue
{
public:
    __mpmc_queue();
    ~__mpmc_queue();
    __mpmc_queue(const __mpmc_queue&) = delete;
    __mpmc_queue(__mpmc_queue&&) = delete;
    __mpmc_queue& operator=(const __mpmc_queue&) = delete;
    __mpmc_queue& operator=(__mpmc_queue&&) = delete;

public:
    bool empty() const;
    size_t size() const;

    bool push(const T& t);
    bool push(T&& t);
    bool pop(T& ret);

private:
    __atomic_fifo fifo_;
    uint64_t arr_[capacity];

    using WORKER = __mpmc_worker<T, std::is_pointer<T>::value>;
    static_assert(__CHECK_POWER_OF_2(capacity), "Capacity MUST power of 2");
    static_assert(capacity > 2, "Capacity MUST larger than 2");
};

static constexpr uint64_t PTR_IN = (uint64_t(1) << 63);
static constexpr uint64_t PTR_OUT = (uint64_t(1) << 62);
static constexpr uint64_t PTR_EMPTY = (uint64_t(1) << 61);

template <typename T, unsigned int capacity>
__mpmc_queue<T, capacity>::__mpmc_queue()
{
    fifo_.mask = capacity - 1;
    fifo_.size = capacity;
    fifo_.buffer = arr_;
    fifo_.in = 1;
    fifo_.out = 0;

    arr_[fifo_.in] = PTR_IN;
    arr_[fifo_.out] = (PTR_OUT | fifo_.out);
    for (unsigned int i = 2; i < capacity; i++)
        arr_[i] = (PTR_EMPTY | i);
}

template <typename T, unsigned int capacity>
__mpmc_queue<T, capacity>::~__mpmc_queue()
{
    WORKER::clear(&fifo_);
}

template <typename T, unsigned int capacity>
bool __mpmc_queue<T, capacity>::empty() const
{
    return fifo_.in - fifo_.out == 1;
}

template <typename T, unsigned int capacity>
size_t __mpmc_queue<T, capacity>::size() const
{
    return fifo_.in - fifo_.out - 1;
}

template <typename T, unsigned int capacity>
bool __mpmc_queue<T, capacity>::push(const T& t)
{
    return WORKER::push(&fifo_, t);
}

template <typename T, unsigned int capacity>
bool __mpmc_queue<T, capacity>::push(T&& t)
{
    return WORKER::push(&fifo_, std::move(t));
}

template <typename T, unsigned int capacity>
bool __mpmc_queue<T, capacity>::pop(T& t)
{
    return WORKER::pop(&fifo_, t);
}

static inline bool __mpmc_push(__atomic_fifo *fifo, void *ptr)
{
    unsigned int cur;
    unsigned int next;
    uint64_t *pNext;

    do
    {
        cur = fifo->in;
        next = cur + 1;
        pNext = fifo->buffer + (next & fifo->mask);
        if ((*pNext) & PTR_OUT)
            return false;

    } while (!__sync_bool_compare_and_swap(pNext, PTR_EMPTY | next, PTR_IN));

    fifo->buffer[cur & fifo->mask] = (uint64_t)ptr;
    ++fifo->in;
    return true;
}

static inline bool __mpmc_pop(__atomic_fifo *fifo, uint64_t& res)
{
    unsigned int cur;
    unsigned int next;
    uint64_t *pNext;
    uint64_t ptr;

    do
    {
        cur = fifo->out;
        next = cur + 1;
        pNext = fifo->buffer + (next & fifo->mask);
        ptr = *pNext;
        if (ptr == PTR_IN)
            return false;

    } while (!__sync_bool_compare_and_swap(fifo->buffer + (cur & fifo->mask),
                                           PTR_OUT | cur,
                                           PTR_EMPTY | (cur + fifo->size)));

    *pNext = (PTR_OUT | next);
    ++fifo->out;
    res = ptr;
    return true;
}

static inline bool __mpmc_try_push(__atomic_fifo *fifo, unsigned int& res)
{
    unsigned int cur;
    unsigned int next;
    uint64_t *pNext;

    do
    {
        cur = fifo->in;
        next = cur + 1;
        pNext = fifo->buffer + (next & fifo->mask);
        if ((*pNext) & PTR_OUT)
            return false;

    } while (!__sync_bool_compare_and_swap(pNext, PTR_EMPTY | next, PTR_IN));

    res = cur;
    return true;
}

static inline void __mpmc_push_rollback(__atomic_fifo *fifo, unsigned int cur)
{
    fifo->buffer[(cur + 1) & fifo->mask] = (PTR_EMPTY | (cur + 1));
}

template <typename T>
class __mpmc_worker<T, true>
{
public:
    static bool push(__atomic_fifo *fifo, const T& t)
    {
        return __mpmc_push(fifo, t);
    }

    static bool push(__atomic_fifo *fifo, T&& t)
    {
        return __mpmc_push(fifo, t);
    }

    static bool pop(__atomic_fifo *fifo, T& t)
    {
        uint64_t ptr;
        bool succ = __mpmc_pop(fifo, ptr);

        if (succ)
            t = (T)ptr;

        return succ;
    }

    static void clear(__atomic_fifo *fifo) { }
};

template <typename T>
class __Holder
{
public:
    __Holder(const T& t) : val(t) { }
    __Holder(T&& t) : val(std::move(t)) { }

public:
    T val;
};

template <typename T>
class __mpmc_worker<T, false>
{
public:
    static bool push(__atomic_fifo *fifo, const T& t)
    {
        unsigned int cur;

        if (!__mpmc_try_push(fifo, cur))
            return false;

        auto *p = new (std::nothrow) __Holder<T>(t);

        if (!p)
        {
            __mpmc_push_rollback(fifo, cur);
            return false;
        }

        fifo->buffer[cur & fifo->mask] = (uint64_t)p;
        ++fifo->in;
        return true;
    }

    static bool push(__atomic_fifo *fifo, T&& t)
    {
        unsigned int cur;

        if (!__mpmc_try_push(fifo, cur))
            return false;

        auto *p = new (std::nothrow) __Holder<T>(std::move(t));

        if (!p)
        {
            __mpmc_push_rollback(fifo, cur);
            return false;
        }

        fifo->buffer[cur & fifo->mask] = (uint64_t)p;
        ++fifo->in;
        return true;
    }

    static bool pop(__atomic_fifo *fifo, T& t)
    {
        uint64_t ptr;

        if (!__mpmc_pop(fifo, ptr))
            return false;

        auto *p = (__Holder<T> *)(ptr);

        t = std::move(p->val);
        delete p;
        return true;
    }

    static void clear(__atomic_fifo *fifo)
    {
        uint64_t ptr;

        while (__mpmc_pop(fifo, ptr))
            delete (__Holder<T> *)(ptr);
    }
};

}
