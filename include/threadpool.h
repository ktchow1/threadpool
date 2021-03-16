
#ifndef __EXPERIMENTAL_THREADPOOL_H__
#define __EXPERIMENTAL_THREADPOOL_H__
#include<locked_queue.h>
#include<lockfree_queue.h>
#include<sync.h>
#include<thread.h>

// ****************************************************************** //
// T = task (must be default constructible if lockfree_queue is used)
// Q = multi-thread safe queue
// S = synchronization primitive
// ****************************************************************** //
// S = sync_futex        work, 1.7us 
// S = sync_pmutex       deadlock
// S = sync_HansBarz     work, 1.7us 
// S = sync_psemaphore   work, 1.7us
// S = sync_condvar      work, 2.2us
// ****************************************************************** //
template<std::invocable T, 
         template<typename> typename Q = YLib::mutex_locked_queue,  
//       sync_primitive S = sync_futex>       
         sync_primitive S = sync_psemaphore>       
class threadpool
{
public:
    threadpool() = delete;
   ~threadpool()
    { 
        for(std::uint32_t n=0; n!=threads.size(); ++n) 
        {
            threads[n].join();
        }
    }

    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = default;
    threadpool& operator=(const threadpool&) = delete;
    threadpool& operator=(threadpool&&) = default;

    explicit threadpool(std::uint32_t num_threads) : run(true), threads(), task_queue(), sync() 
    {
        for(std::uint32_t n=0; n!=num_threads; ++n)
        {
            threads.emplace_back(std::thread(&threadpool<T,Q>::thread_fct, this));
        }
    }

    threadpool(std::uint32_t num_threads, const std::vector<std::uint32_t>& affinity) : threadpool(num_threads)
    {   
    /*  for(auto& x:threads) // Not good for my home-machine, apply these setting in production only.
        {
            set_thread_affinity(x.native_handle(), affinity);
            set_thread_policy  (x.native_handle(), SCHED_RR);
        } */ 
    }

public: 
    void stop()
    {
        run.store(false);
        for(std::uint32_t n=0; n!=threads.size(); ++n) 
        {
            sync.notify();
        }
    }

public: 
    // ************************* //    
    // *** Producer of tasks *** //
    // ************************* //    
    template<typename... ARGS>
    bool emplace_task(ARGS&&... args)
    {
        if (task_queue.emplace(std::forward<ARGS>(args)...))
        {
            sync.notify(); 
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename... ARGS>
    void emplace_task_until_success(ARGS&&... args)
    {
        while(!task_queue.emplace(std::forward<ARGS>(args)...));
        sync.notify(); 
    }

private:
    // ************************* //    
    // *** Consumer of tasks *** //
    // ************************* //    
    void thread_fct()
    {
        // Decouple two checking :
        // 1. is threadpool running 
        // 2. is task queue empty 
        //
        // Adventages of decoupling :
        // 1. avoid repeated lock & unlock on peeking queue size
        // 2. avoid lost notification on stopping threadpool

        while(run.load())
        {
            sync.wait();
            auto task = task_queue.pop();
            if (task)
            {
                (*task)();
            }
        }

        // All threads are now spinning (no more waiting).
        while(task_queue.peek_size() > 0)
        {
            auto task = task_queue.pop();
            if (task)
            {
                (*task)();
            }
        }
    }

private:
    std::atomic<bool> run;
    std::vector<std::thread> threads;
    Q<T> task_queue;
    S sync;
};

#endif
