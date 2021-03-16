#ifndef __EXPERIMENTAL_THREADPOOL_H__
#define __EXPERIMENTAL_THREADPOOL_H__
#include<iostream>
#include<vector>
#include<queue>
#include<functional>
#include<thread>
#include<mutex>
#include<condition_variable>

class threadpoolJ
{
public:
    explicit threadpoolJ(std::uint32_t num_threads) : out_of_scope(false)
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            threads.emplace_back(std::thread(&threadpoolJ::fct, this, n));
        }
    }

    ~threadpoolJ()
    {
        out_of_scope.store(true);
        condvar.notify_all();
        for(auto& x:threads)
        {
            x.join();
            std::cout << "\nthread joined" << std::flush;
        }
    }

public: 
    void add_task(const std::function<void()>& task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push(task);
        }
        condvar.notify_one();
    }

private:
    // Two-loop approaches to decouple :
    // 1. checking of out-of-scope and
    // 2. checking of queue emptyness 
    void fct(std::uint32_t id)
    {
        // set affinity here (skipped for simplicity)
        // set priority here (skipped for simplicity)
        
        // *** 1st loop *** //
        while(!out_of_scope.load())
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condvar.wait(lock, [this]()
                { 
                    // Predicate returns true to continue
                    return !tasks.empty(); 
                }); 
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
        std::cout << "\nthread half-done" << std::flush;

        // *** 2nd loop *** //
        while(!tasks.empty())
        {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(mutex);
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
        std::cout << "\nthread done " << id << std::flush;
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex mutex;
    std::condition_variable condvar;
    std::atomic<bool> out_of_scope;
};

/*
class threadpoolJ2
{
public:
    explicit threadpoolJ2(std::uint32_t num_threads) : out_of_scope(false)
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            threads.emplace_back(std::thread(&threadpoolJ2::fct, this, n));
        }
    }

    ~threadpoolJ2()
    {
        out_of_scope.store(true);
        condvar.notify_all();
        for(auto& x:threads)
        {
            x.join();
            std::cout << "\nthread joined" << std::flush;
        }
    }

public: 
    void add_task(const std::function<void()>& task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push(task);
        }
        condvar.notify_one();
    }

private:
    void fct(std::uint32_t id)
    {
        // *** 1st loop *** //
        while(!out_of_scope.load())
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condvar.wait(lock, [this]()
                { 
                    // Predicate returns true to continue
                    return !tasks.empty(); 
                }); 
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
        std::cout << "\nthread half-done" << std::flush;

        // *** 2nd loop *** //
        while(!tasks.empty())
        {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(mutex);
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
        std::cout << "\nthread done " << id << std::flush;
    }

private:
    std::vector<std::jthread> threads;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex mutex;
    std::condition_variable condvar;
    std::atomic<bool> out_of_scope;
}; */

#endif
