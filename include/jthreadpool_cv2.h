#ifndef __EXPERIMENTAL_JTHREADPOOL_CV2_H__
#define __EXPERIMENTAL_JTHREADPOOL_CV2_H__
#include<iostream>
#include<vector>
#include<queue>
#include<functional>
#include<coroutine> // <--- Replace function-task by coroutine-task
#include<thread>
#include<mutex>
#include<condition_variable>

// **************************************** //
// *** This is a version for coroutine. *** //
// **************************************** //
class jthreadpool_cv2
{
public:
    explicit jthreadpool_cv2(std::uint32_t num_threads)
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            jthreads.emplace_back(std::jthread(&jthreadpool_cv2::fct, this, s_source.get_token(), n));
        }
    }

    void stop()
    {
        s_source.request_stop();
        for(auto& x:jthreads) x.join();
    }

public:
    // ********************************************************************* //
    // Ensure that task.promise().final_suspend() return std::suspend_always
    // Otherwise, crash happens when we check-done, as task does not exists.
    //
    //   if (!task.done()) ...
    //
    // ********************************************************************* //
    void add_task(const std::coroutine_handle<>& task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push(task);
        }
        condvar.notify_one();
    }

private:
    void fct(std::stop_token s_token, std::uint32_t id)
    {
        try
        {
            // *** 1st loop *** //
            while(!s_source.get_token().stop_requested())
            {
                std::coroutine_handle<> task;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    if (!condvar.wait(lock, s_token, [this]()
                    {
                        return !tasks.empty();
                    }))
                    {
                        break;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();

                // *** Unfinished coroutine *** //
                if (!task.done()) add_task(task);
            }
            std::cout << "\njthread-coroutine half-done" << id << std::flush;

            // *** 2nd loop *** //
            while(!tasks.empty())
            {
                std::coroutine_handle<> task;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();

                // *** Unfinished coroutine *** //
                if (!task.done()) add_task(task);
            }
            std::cout << "\njthread-coroutine done " << id << std::flush;
        }
        catch(std::exception& e)
        {
            std::cout << "\nexception caugth in worker " << id << ", e = " << e.what() << std::flush;
        }
    }

private:
    std::vector<std::jthread> jthreads;
    std::queue<std::coroutine_handle<>> tasks;
    mutable std::mutex mutex;
    std::condition_variable_any condvar;   // Change : Replace condvar by condvar_any.
    std::stop_source s_source;             // Change : Replace out_of_scope by stop-source.
};

#endif