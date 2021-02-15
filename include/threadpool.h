
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
// ****************************************************************************************** //
// S = sync_futex         may deadlock, fastest  (Todo : make lock and counter update atomic)  
// S = sync_pmutex       100% deadlock           (Todo : support multi consumers)
// S = sync_pmutex2      work, medium speed
// S = sync_psemaphore   work, medium speed
// S = sync_condvar      work, slower speed 
// ****************************************************************************************** //
template<std::invocable T, 
         template<typename> typename Q = YLib::mutex_locked_queue,  
         typename S = sync_psemaphore>       
class threadpool
{
public:
    threadpool() = delete;
   ~threadpool()
    { 
        run.store(false);
        for(std::uint32_t n=0; n!=threads.size(); ++n) sync.notify();
        for(std::uint32_t n=0; n!=threads.size(); ++n) threads[n].join();
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
        for(auto& x:threads)
        {
            set_thread_affinity(x.native_handle(), affinity);
            set_thread_priority(x.native_handle(), SCHED_RR);
        }        
    }

public: 
    // ************************* //    
    // *** Producer of tasks *** //
    // ************************* //    
    template<typename... ARGS>
    void emplace_task(ARGS&&... args)
    {
        bool done = false;
        while(!done) 
        {
            done = task_queue.emplace(std::forward<ARGS>(args)...);
        }
        sync.notify(); 

        // Ideally, semaphore::count == queue::size
    }

private:
    // ************************* //    
    // *** Consumer of tasks *** //
    // ************************* //    
    void thread_fct()
    {
        while(run.load() || task_queue.peek_size() > 0)
        {
            sync.wait();
            auto task = task_queue.pop();
            if (task)
            {
                (*task)();
            }

            // Todo : can we invoke task before popping? Feasible only for mpsc?
        }
    }

private:
    std::atomic<bool> run;
    std::vector<std::thread> threads;
    Q<T> task_queue;
    S sync;
};

#endif
