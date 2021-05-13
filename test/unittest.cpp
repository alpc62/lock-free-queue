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
#include "queue62.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <gtest/gtest.h>

template<class Q>
void push_int(Q& que, std::atomic<int>& pusher)
{
    int i = 0;
    while (i < 2048)
    {
        int sz = que.push(&i, 1);
        if (sz == 0)
        {
            // full
            std::this_thread::yield();
            continue;
        }
        i++;
    }
    --pusher;
}

template<class Q>
void pop_int(Q& que, std::atomic<int>& pusher, std::map<int, int>& counter)
{
    int res;
    while (que.read_available() > 0 || pusher > 0)
    {
        bool succ = que.pop(res);
        if (!succ)
        {
            // empty
            std::this_thread::yield();
            continue;
        }

        EXPECT_LE(0, res);
        EXPECT_LT(res, 2048);
        counter[res]++;
    }
}

template<class Q>
void push_string(Q& que, std::atomic<int>& pusher)
{
    int i = 0;
    while (i < 2048)
    {
        bool succ = que.push(std::to_string(i));
        if (!succ)
        {
            // full
            std::this_thread::yield();
            continue;
        }
        i++;
    }
    --pusher;
}

template<class Q>
void pop_string(Q& que, std::atomic<int>& pusher, std::map<int, int>& counter)
{
    std::string str;
    while (que.read_available() > 0 || pusher > 0)
    {
        int sz = que.pop(&str, 1);
        if (sz == 0)
        {
            // empty
            std::this_thread::yield();
            continue;
        }

        int res = atoi(str.c_str());
        EXPECT_LE(0, res);
        EXPECT_LT(res, 2048);
        counter[res]++;
    }
}

template<class Q>
void push_int2(Q& que, std::atomic<int>& pusher)
{
    int i = 0;
    while (i < 2048)
    {
        bool succ = que.push(i);
        if (!succ)
        {
            // full
            std::this_thread::yield();
            continue;
        }
        i++;
    }
    --pusher;
}

template<class Q>
void pop_int2(Q& que, std::atomic<int>& pusher, std::map<int, int>& counter)
{
    int res;
    while (!que.empty() || pusher > 0)
    {
        bool succ = que.pop(res);
        if (!succ)
        {
            // empty
            std::this_thread::yield();
            continue;
        }

        EXPECT_LE(0, res);
        EXPECT_LT(res, 2048);
        counter[res]++;
    }
}

template<class Q>
void push_string2(Q& que, std::atomic<int>& pusher)
{
    int i = 0;
    while (i < 2048)
    {
        bool succ = que.push(std::to_string(i));
        if (!succ)
        {
            // full
            std::this_thread::yield();
            continue;
        }
        i++;
    }
    --pusher;
}

template<class Q>
void pop_string2(Q& que, std::atomic<int>& pusher, std::map<int, int>& counter)
{
    std::string str;
    while (!que.empty() || pusher > 0)
    {
        bool succ = que.pop(str);
        if (!succ)
        {
            // empty
            std::this_thread::yield();
            continue;
        }

        int res = atoi(str.c_str());
        EXPECT_LE(0, res);
        EXPECT_LT(res, 2048);
        counter[res]++;
    }
}

template<class Q>
void push_ptr(Q& que, std::atomic<int>& pusher)
{
    int i = 0;
    while (i < 2048)
    {
        std::string *p = new std::string(std::to_string(i));
        bool succ = que.push(p);
        if (!succ)
        {
            // full
            delete p;
            std::this_thread::yield();
            continue;
        }
        i++;
    }
    --pusher;
}

template<class Q>
void pop_ptr(Q& que, std::atomic<int>& pusher, std::map<int, int>& counter)
{
    std::string *p;
    while (!que.empty() || pusher > 0)
    {
        bool succ = que.pop(p);
        if (!succ)
        {
            // empty
            std::this_thread::yield();
            continue;
        }

        int res = atoi(p->c_str());
        EXPECT_LE(0, res);
        EXPECT_LT(res, 2048);
        counter[res]++;
        delete p;
    }
}

void check1(int range, int n, std::map<int, int>& counter)
{
    for (int i = 0; i < range; i++)
        EXPECT_EQ(counter[i], n);
}

void check2(int range, int n, std::map<int, int>& counter1, std::map<int, int>& counter2)
{
    for (int i = 0; i < range; i++)
        EXPECT_EQ(counter1[i] + counter2[i], n);
}

TEST(unittest, case1)
{
    using QueInt = spsc_queue<int, 1024>;
    QueInt que;
    std::atomic<int> pusher(1);
    std::map<int, int> counter1;
    std::thread in1(push_int<QueInt>, std::ref(que), std::ref(pusher));
    std::thread out1(pop_int<QueInt>, std::ref(que), std::ref(pusher), std::ref(counter1));
    in1.join();
    out1.join();
    EXPECT_EQ(que.read_available(), 0);
    check1(2048, 1, counter1);
}

TEST(unittest, case2)
{
    using QueStr = spsc_queue<std::string, 1024>;
    QueStr que;
    std::atomic<int> pusher(1);
    std::map<int, int> counter1;
    std::thread in1(push_string<QueStr>, std::ref(que), std::ref(pusher));
    std::thread out1(pop_string<QueStr>, std::ref(que), std::ref(pusher), std::ref(counter1));
    in1.join();
    out1.join();
    EXPECT_EQ(que.read_available(), 0);
    check1(2048, 1, counter1);
}

TEST(unittest, case3)
{
    using QueInt = mpmc_queue<int, 1024>;
    QueInt que;
    std::atomic<int> pusher(4);
    std::map<int, int> counter1;
    std::map<int, int> counter2;
    std::thread in1(push_int2<QueInt>, std::ref(que), std::ref(pusher));
    std::thread in2(push_int2<QueInt>, std::ref(que), std::ref(pusher));
    std::thread in3(push_int2<QueInt>, std::ref(que), std::ref(pusher));
    std::thread in4(push_int2<QueInt>, std::ref(que), std::ref(pusher));
    std::thread out1(pop_int2<QueInt>, std::ref(que), std::ref(pusher), std::ref(counter1));
    std::thread out2(pop_int2<QueInt>, std::ref(que), std::ref(pusher), std::ref(counter2));
    in1.join();
    in2.join();
    in3.join();
    in4.join();
    out1.join();
    out2.join();
    EXPECT_EQ(que.size(), 0);
    check2(2048, 4, counter1, counter2);
}

TEST(unittest, case4)
{
    using QueStr = mpmc_queue<std::string, 1024>;
    QueStr que;
    std::atomic<int> pusher(4);
    std::map<int, int> counter1;
    std::map<int, int> counter2;
    std::thread in1(push_string2<QueStr>, std::ref(que), std::ref(pusher));
    std::thread in2(push_string2<QueStr>, std::ref(que), std::ref(pusher));
    std::thread in3(push_string2<QueStr>, std::ref(que), std::ref(pusher));
    std::thread in4(push_string2<QueStr>, std::ref(que), std::ref(pusher));
    std::thread out1(pop_string2<QueStr>, std::ref(que), std::ref(pusher), std::ref(counter1));
    std::thread out2(pop_string2<QueStr>, std::ref(que), std::ref(pusher), std::ref(counter2));
    in1.join();
    in2.join();
    in3.join();
    in4.join();
    out1.join();
    out2.join();
    EXPECT_EQ(que.size(), 0);
    check2(2048, 4, counter1, counter2);
}

TEST(unittest, case5)
{
    using QuePtr = mpmc_queue<std::string *, 4>;
    QuePtr que;
    std::atomic<int> pusher(8);
    std::map<int, int> counter1;
    std::map<int, int> counter2;
    std::thread in1(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in2(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in3(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in4(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in5(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in6(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in7(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread in8(push_ptr<QuePtr>, std::ref(que), std::ref(pusher));
    std::thread out1(pop_ptr<QuePtr>, std::ref(que), std::ref(pusher), std::ref(counter1));
    std::thread out2(pop_ptr<QuePtr>, std::ref(que), std::ref(pusher), std::ref(counter2));
    in1.join();
    in2.join();
    in3.join();
    in4.join();
    in5.join();
    in6.join();
    in7.join();
    in8.join();
    out1.join();
    out2.join();
    EXPECT_EQ(que.size(), 0);
    check2(2048, 8, counter1, counter2);
}
