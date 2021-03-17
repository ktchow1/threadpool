#ifndef __EXPERIMENTAL_THREADPOOL_JCV_H__
#define __EXPERIMENTAL_THREADPOOL_JCV_H__
#include<iostream>
#include<vector>
#include<queue>
#include<functional>
#include<thread>
#include<mutex>
#include<condition_variable>

class threadpool_jcv
{
public:
    explicit threadpool_jcv(std::uint32_t num_threads) : out_of_scope(false)
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            threads.emplace_back(std::thread(&threadpool_jcv::fct, this, n));
        }
    }

    ~threadpool_jcv()
    {
        std::cout << "\njthreadpool destructor" << std::flush;
        stop();
    }

    void stop()
    {
        out_of_scope.store(true);
        condvar.notify_all();
        for(auto& x:threads)
        {
            if (x.joinable()) 
            {
                x.join();
            }
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
        try
        {
            // *** 1st loop *** //
            while(!out_of_scope.load())
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condvar.wait(lock, [this]()
                    { 
                        return !tasks.empty() || 
                               out_of_scope.load();
                    }); 

                    if (out_of_scope.load()) break;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
            std::cout << "\njthread half-done" << id << std::flush;

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
            std::cout << "\njthread done " << id << std::flush;
        }
        catch(std::exception& e)
        {
            std::cout << "\nexception caugth in worker " << id << ", e = " << e.what() << std::flush;
        }
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex mutex;
    std::condition_variable condvar;
    std::atomic<bool> out_of_scope;
};

#endif
