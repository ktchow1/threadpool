#ifndef __EXPERIMENTAL_JTHREADPOOL_CV_H__
#define __EXPERIMENTAL_JTHREADPOOL_CV_H__
#include<iostream>
#include<vector>
#include<queue>
#include<functional>
#include<thread>
#include<mutex>
#include<condition_variable>

class jthreadpool_cv
{
public:
    explicit jthreadpool_cv(std::uint32_t num_threads) 
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            jthreads.emplace_back(std::jthread(&jthreadpool_cv::fct, this, s_source.get_token(), n));
        }
    }

    ~jthreadpool_cv()
    {
        std::cout << "\njthreadpool destructor" << std::flush;
        stop();
    }

    void stop()
    {
        s_source.request_stop();
    //  condvar.notify_all(); // Change : Stop-callback auto invokes condvar.notify_all().
    //  for(auto& x:jthreads) // Change : auto-join in destructor. 
    //  {
    //      if (x.joinable()) 
    //      {
    //          x.join();
    //      }
    //      std::cout << "\njthread joined" << std::flush;
    //  }
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
    void fct(std::stop_token s_token, std::uint32_t id)
    {
        try
        {
            // *** 1st loop *** //
            while(!s_source.get_token().stop_requested())
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    if (!condvar.wait(lock, s_token, [this]()
                    { 
                        return !tasks.empty();
                    //        || out_of_scope.load(); // Change : Move stop-token into predicate.
                    }))
                    {
                        break; // Change : For stop_requested, break ...
                    } 

                //  if (out_of_scope.load()) break;
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
    std::vector<std::jthread> jthreads;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex mutex;
    std::condition_variable_any condvar;   // Change : Replace condvar by condvar_any.
    std::stop_source s_source;             // Change : Replace out_of_scope by stop-source.
};

#endif
