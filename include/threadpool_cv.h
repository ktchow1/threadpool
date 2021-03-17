#ifndef __EXPERIMENTAL_THREADPOOL_CV_H__
#define __EXPERIMENTAL_THREADPOOL_CV_H__
#include<iostream>
#include<vector>
#include<queue>
#include<functional>
#include<thread>
#include<mutex>
#include<condition_variable>

// **************************************************** //
// Common problems in threadpool :
// 1. destruct a running thread (without stop / join)
// 2. join a already joined thread 
// 3. pop a destructed task-queue
// **************************************************** //
class threadpool_cv
{
public:
    explicit threadpool_cv(std::uint32_t num_threads) : out_of_scope(false)
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            threads.emplace_back(std::thread(&threadpool_cv::fct, this, n));
        }
    }

    ~threadpool_cv()
    {
        std::cout << "\nthreadpool destructor" << std::flush;
        stop();
    }

    // Stop may be called 2 times : 
    // 1. once explicitly
    // 2. once inside destructor
    void stop()
    {
        out_of_scope.store(true);
        condvar.notify_all();
        for(auto& x:threads)
        {
            // BUG3 : Need to check joinable to avoid multi-join, otherwise it throws
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
    // Two-loop approaches to decouple :
    // 1. checking of out-of-scope and
    // 2. checking of queue emptyness 
    void fct(std::uint32_t id)
    {
        // set affinity here (skipped for simplicity)
        // set priority here (skipped for simplicity)
        
        try // BUG4 : Need to handle exception thrown from task
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
                        return !tasks.empty() 
                              || out_of_scope.load(); // BUG1 : missing this results in wakeup-miss on termination
                    }); 

                    if (out_of_scope.load()) break; // BUG2 : missing this results in popping empty queue on termination
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
            std::cout << "\nthread half-done" << id << std::flush;

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
        catch(std::exception& e)
        {
            std::cout << "\nexception caugth in worker " << id << ", e = " << e.what() << std::flush;
        //  stop(); // BUG5 : No need, as threadpool destructor is called in stack unwinding
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
