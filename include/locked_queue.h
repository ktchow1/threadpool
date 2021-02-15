
#ifndef __EXPERIMENTAL_LOCKED_QUEUE_H__
#define __EXPERIMENTAL_LOCKED_QUEUE_H__
#include<iostream>
#include<queue>
#include<mutex>
#include<optional>

class spinlock final
{
public:
    spinlock()
    {
        pthread_spin_init(&impl, PTHREAD_PROCESS_PRIVATE); // for threads inside same process only
    }

    ~spinlock()
    {
        pthread_spin_destroy(&impl);
    }

    spinlock(const spinlock&) = delete;
    spinlock(spinlock&&) = delete;
    spinlock& operator=(const spinlock&) = delete;
    spinlock& operator=(spinlock&&) = delete;

public:
    inline void lock() noexcept
    {
        pthread_spin_lock(&impl);
    }

    inline void unlock() noexcept
    {
        pthread_spin_unlock(&impl);
    }

private:
    pthread_spinlock_t impl;
};


namespace YLib {
template<typename T, typename LOCK> class locked_queue final
{
public:
    using value_type = T;
    using this_type = locked_queue<T, LOCK>;

public:
    locked_queue() = default;
   ~locked_queue() = default;
    locked_queue(const this_type&) = delete;
    locked_queue(this_type&&) = default;
    this_type& operator=(const this_type&) = delete;
    this_type& operator=(this_type&&) = default;

public:
    template<typename... ARGS> 
    bool emplace(ARGS&&... args) noexcept
    {
        std::lock_guard<LOCK> lock(impl);  
        queue.emplace(std::forward<ARGS>(args)...);
        return true;
    }

    std::optional<T> pop() noexcept
    {
        std::lock_guard<LOCK> lock(impl); 
        if (queue.size() == 0) return std::nullopt;
        
        T output = std::move(queue.front());
        queue.pop();
        return std::make_optional(output);
    }

public:
    std::uint32_t peek_size() const noexcept
    {
        std::lock_guard<LOCK> lock(impl);
        return queue.size();
    }
    
private:
    std::queue<T> queue;
    mutable LOCK impl; // mutex or spinlock
};

template<typename T>
using mutex_locked_queue = locked_queue<T, std::mutex>;
template<typename T>
using spin_locked_queue = locked_queue<T, spinlock>;
}

#endif
