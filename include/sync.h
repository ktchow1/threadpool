
#ifndef __EXPERIMENTAL_SYNC_H__
#define __EXPERIMENTAL_SYNC_H__
#include<mutex>
#include<condition_variable>
#include<future>
#include<array>

// For posix-thread lib
#include<stdint.h>    
#include<unistd.h>    
#include<semaphore.h> 
#include<linux/futex.h> 
#include<sys/syscall.h> // for SYS_futex
#include<sys/time.h> 


template<typename T> 
concept sync_primitive = requires(T x)
{
    x.wait();
    x.notify();
};

// ********************************* //
// *** Implementation with futex *** //
// ********************************* //
class sync_futex
{
public:
    // This thread is blocked when futex == blocking_value.
    sync_futex() : blocking_value(0)
    {
    }

   ~sync_futex() = default;
    sync_futex(const sync_futex&) = delete;
    sync_futex(sync_futex&&) = default;
    sync_futex& operator=(const sync_futex&) = delete;
    sync_futex& operator=(sync_futex&&) = default;

public: 
    void wait()
    {
        syscall(SYS_futex, &futex, FUTEX_WAIT, blocking_value, NULL, NULL, 0); 
        ++blocking_value;
    }

    void notify()
    {
        futex.fetch_add(1);
        syscall(SYS_futex, &futex, FUTEX_WAKE, 1, NULL, NULL, 0); 
    }

private:
    std::atomic<std::int32_t> futex;
    std::int32_t blocking_value;
};

// ******************************************************** //
// *** Implementation with std::mutex (single consumer) *** //
// ******************************************************** //
class sync_mutex
{
public:
    sync_mutex() : mutex()
    {
        mutex.lock();
    }

   ~sync_mutex() 
    {
        mutex.unlock();
    }

    sync_mutex(const sync_mutex&) = delete;
    sync_mutex(sync_mutex&&) = default;
    sync_mutex& operator=(const sync_mutex&) = delete;
    sync_mutex& operator=(sync_mutex&&) = default;

public:
    inline void wait()
    {
        mutex.lock(); 
    }

    inline void notify()
    {
        mutex.unlock(); 
    }

private:
    std::mutex mutex;
};

// *********************************************************** //
// *** Implementation with pthread mutex (single consumer) *** //
// *********************************************************** //
class sync_pmutex 
{
public:
    sync_pmutex()
    {
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_lock(&mutex);
    }

   ~sync_pmutex() 
    {
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
    }

    sync_pmutex(const sync_pmutex&) = delete;
    sync_pmutex(sync_pmutex&&) = default;
    sync_pmutex& operator=(const sync_pmutex&) = delete;
    sync_pmutex& operator=(sync_pmutex&&) = default;

public:
    inline void wait()
    {
        pthread_mutex_lock(&mutex);
    }

    inline void notify()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex;
};

// ******************************************************** //
// *** Implementation with pthread mutex (Hans W. Barz) *** //
// ******************************************************* //
// 1. count is protected by cs_mutex, hence its inc/dec are atomic
// 2. waiting threads on P are waked either by :
// -- other threads completing P and count N->N-1, where N>1
// -- other threads completing V and count 0->1
// ******************************************************* //
class sync_HansBarz  
{
public:
    sync_HansBarz() : count(0)
    {
        pthread_mutex_init(&cs_mutex, NULL);
        pthread_mutex_init(&pv_mutex, NULL);
        pthread_mutex_lock(&pv_mutex);
    }

   ~sync_HansBarz() 
    {
        pthread_mutex_unlock (&pv_mutex);
        pthread_mutex_destroy(&pv_mutex);
        pthread_mutex_destroy(&cs_mutex);
    }

    sync_HansBarz(const sync_HansBarz&) = delete;
    sync_HansBarz(sync_HansBarz&&) = default;
    sync_HansBarz& operator=(const sync_HansBarz&) = delete;
    sync_HansBarz& operator=(sync_HansBarz&&) = default;

public:
    inline void wait()
    {
        pthread_mutex_lock(&pv_mutex); // wait ... 
        pthread_mutex_lock(&cs_mutex);
        --count;
        if (count > 0)
        {
            pthread_mutex_unlock(&pv_mutex);
        }
        pthread_mutex_unlock(&cs_mutex);
    }

    inline void notify()
    {
        pthread_mutex_lock(&cs_mutex);
        ++count;
        if (count == 1)
        {
            pthread_mutex_unlock(&pv_mutex); // notify ... 
        }
        pthread_mutex_unlock(&cs_mutex);
    }

private:
    std::int32_t count;
    pthread_mutex_t cs_mutex; // for critical session protection
    pthread_mutex_t pv_mutex; // for signaling, i.e. p and v
}; 
  
// ***************************************** //
// *** Implementation with std semaphore *** //
// ***************************************** //
class sync_semaphore
{
public:
    sync_semaphore() // : semaphore(0) // initial count is zero, i.e. consumer must wait 
    {
    }

   ~sync_semaphore() = default;
    sync_semaphore(const sync_semaphore&) = delete;
    sync_semaphore(sync_semaphore&&) = default;
    sync_semaphore& operator=(const sync_semaphore&) = delete;
    sync_semaphore& operator=(sync_semaphore&&) = default;

public:
    inline void wait()
    {
    //  semaphore.acquire();
    }

    inline void notify()
    {
    //  semaphore.release();
    }

private:
//  std::counting_semaphore<1> semaphore;
};   

// ********************************************* //
// *** Implementation with pthread semaphore *** //
// ********************************************* //
class sync_psemaphore
{
public:
    sync_psemaphore()
    {
        sem_init(&semaphore, 0, 0); 
        // arg[1] : 0 for multi-thread, 1 for multi-process
        // arg[2] : initial value
    }

   ~sync_psemaphore() 
    {
        sem_destroy(&semaphore);
    }

    sync_psemaphore(const sync_psemaphore&) = delete;
    sync_psemaphore(sync_psemaphore&&) = default;
    sync_psemaphore& operator=(const sync_psemaphore&) = delete;
    sync_psemaphore& operator=(sync_psemaphore&&) = default;

public:
    inline void wait()
    {
        sem_wait(&semaphore);
    }

    inline void notify()
    {
       sem_post(&semaphore);
    }

    inline auto peek_value() 
    {
        std::int32_t x;
        sem_getvalue(&semaphore, &x);
        return x;
    }

private:
    sem_t semaphore;
};

// ********************************************** //
// *** Implementation with condition variable *** //
// ********************************************** //
class sync_condvar
{
public:
    sync_condvar() : count(0)
    {
    }

   ~sync_condvar() = default;
    sync_condvar(const sync_condvar&) = delete;
    sync_condvar(sync_condvar&&) = default;
    sync_condvar& operator=(const sync_condvar&) = delete;
    sync_condvar& operator=(sync_condvar&&) = default;

public: 
    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(count == 0) cv.wait(lock); 
        --count;
    }

    void notify()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++count;
        }
        cv.notify_one();
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::uint32_t count; // emulate a semaphore
};

// ********************************************** //
// *** Implementation with promise and future *** //
// ********************************************** //
template<std::uint32_t N>
class sync_promfut
{
public:
    sync_promfut() : p_index(0), f_index(0), promises{}, futures{}
    {
        for(std::uint32_t n=0; n!=N ; ++n)
        {
            futures[n] = promises[n].get_future();
        }
    }

   ~sync_promfut() = default;
    sync_promfut(const sync_promfut&) = delete; 
    sync_promfut(sync_promfut&&) = default; 
    sync_promfut& operator=(const sync_promfut&) = delete; 
    sync_promfut& operator=(sync_promfut&&) = default; 

public:
    void wait()
    {
        auto n = f_index.fetch_add(1);
        if (n < N) 
        {
            futures[n].get();
        }
    }

    void notify()
    {
        auto n = p_index.fetch_add(1);
        if (n < N)
        {
            promises[n].set_value();
        }
    }

private:
    std::atomic<std::uint32_t> p_index;
    std::atomic<std::uint32_t> f_index;
    std::array<std::promise<void>, N> promises;
    std::array<std::future<void>, N> futures;
};

#endif
