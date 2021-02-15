
#ifndef __EXPERIMENTAL_THREAD_H__
#define __EXPERIMENTAL_THREAD_H__
#include<iostream>
#include<vector>
#include<thread>         //  std::thread
#include<pthread.h>      // posix thread
#include<sched.h>        // affinity and realtime
#include<unistd.h>       // priority
#include<sys/resource.h> // priority
#include<sys/syscall.h>  // tid

/*  Recall that :
 *
 *  pthread_self() == std::this_thread::native_handle()
 *  policy         =  SCHED_RR / SCHED_FIFO 
 */

inline void set_thread_affinity(auto thread_handle, const std::vector<std::uint32_t>& affinity)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for(const auto& x:affinity)
    {
        CPU_SET(x, &cpuset);  
    }
    auto rc = pthread_setaffinity_np(thread_handle, sizeof(cpuset), &cpuset); 
    if (rc != 0)
    {
        std::cout << "*** cannot set affinity ***\n" << std::flush;
    }
}

inline void set_this_thread_affinity(const std::vector<std::uint32_t>& affinity)
{
    set_thread_affinity(pthread_self(), affinity);
}

inline void set_thread_affinity(auto thread_handle, std::uint32_t affinity)
{
    set_thread_affinity(thread_handle, std::vector<std::uint32_t>{affinity});
}

inline void set_this_thread_affinity(std::uint32_t affinity)
{
    set_thread_affinity(pthread_self(), affinity);
}

inline void set_thread_priority(auto thread_handle, auto policy) 
{
    struct sched_param sp;
    sp.sched_priority = sched_get_priority_max(policy);

    auto rc = pthread_setschedparam(thread_handle, policy, &sp);
    if (rc != 0)
    {
        std::cout << "*** cannot set priority ***\n" << std::flush;
    }   
}

inline void set_this_thread_priority(auto policy) 
{
    set_thread_priority(pthread_self(), policy);
}

#endif
