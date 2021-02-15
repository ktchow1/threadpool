
#include<iostream>
#include<thread.h>
#include<stat.h> 
#include<future>

void timer_resolution()
{
    constexpr std::uint32_t N = 1000;
    statistics<std::uint64_t> stat;
    for(std::uint32_t n=0; n!=N; ++n)
    {
        timespec ts0;
        timespec ts1;
        clock_gettime(CLOCK_MONOTONIC, &ts0);
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        stat.add(to_nanosec(ts1)-to_nanosec(ts0));
    }

    std::cout << "\nTimer resolution : ";
    std::cout << stat.get_string();
    std::cout << "\n";
}

void time_thread_create()
{
    constexpr std::uint32_t N = 1000;
    statistics<std::uint64_t> stat;
    for(std::uint32_t n=0; n!=N; ++n)
    {
        timespec ts0;
        timespec ts1;
        clock_gettime(CLOCK_MONOTONIC, &ts0);

        std::thread t([&]()
        {
            clock_gettime(CLOCK_MONOTONIC, &ts1);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        });

        t.join();
        stat.add(to_nanosec(ts1)-to_nanosec(ts0));
    }

    std::cout << "\nThread creation time : ";
    std::cout << stat.get_string();
    std::cout << "\n";
}

void time_thread_created_by_async_call()
{
    constexpr std::uint32_t N = 1000;
    statistics<std::uint64_t> stat;
    for(std::uint32_t n=0; n!=N; ++n)
    {
        timespec ts0;
        clock_gettime(CLOCK_MONOTONIC, &ts0);

        std::future<timespec> future = std::async([]()
        {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            return ts;
        });

        timespec ts1 = future.get(); // thread join inside
        stat.add(to_nanosec(ts1)-to_nanosec(ts0));
    }

    std::cout << "\nAsync call time : ";
    std::cout << stat.get_string();
    std::cout << "\n";
}

void time_mutex_lock_and_unlock()
{
    std::mutex mutex;

    constexpr std::uint32_t N = 1000;
    statistics<std::uint64_t> stat0;
    statistics<std::uint64_t> stat1;
    for(std::uint32_t n=0; n!=N; ++n)
    {
        timespec ts0, ts1, ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts0);
        mutex.lock();
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        mutex.unlock();
        clock_gettime(CLOCK_MONOTONIC, &ts2);

        stat0.add(to_nanosec(ts1)-to_nanosec(ts0));
        stat1.add(to_nanosec(ts2)-to_nanosec(ts1));
    }

    std::cout << "\nMutex lock time : ";
    std::cout << stat0.get_string();
    std::cout << "\n";
    std::cout << "\nMutex unlock time : ";
    std::cout << stat1.get_string();
    std::cout << "\n";
}

auto synchronization_with_atomic(std::uint32_t cpu0, std::uint32_t cpu1)
{
    // *** Sync mechanism *** //
    std::atomic<std::uint32_t> trigger = 0;
    std::atomic<std::uint32_t> response = 0;
    constexpr std::uint32_t N = 1000;

    // *** Measurement *** //
    timespec ts0;
    timespec ts1;
    timespec tsM;
    statistics<std::uint64_t> stat01;
    statistics<std::uint64_t> stat0M;
    statistics<std::uint64_t> statM1;

    // *************** //
    // *** Reactor *** //
    // *************** //
    std::thread reactor([&]()
    {
        set_this_thread_affinity(cpu0);        
        set_this_thread_priority(SCHED_FIFO);        
        for(std::uint32_t n=0; n!=N; ++n)
        {
            while(trigger.load()!=n+1) 
            {
                __builtin_ia32_pause();
            //  std::this_thread::yield(); // very slow response time
            }
            clock_gettime(CLOCK_MONOTONIC, &tsM);
            response.fetch_add(1);
        }
    });

    // ***************** //
    // *** Initiator *** //
    // ***************** //
    set_this_thread_affinity(cpu1);        
    set_this_thread_priority(SCHED_FIFO);        
    for(std::uint32_t n=0; n!=N; ++n)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts0);
        trigger.fetch_add(1);
        while(response.load()!=n+1) 
        {
            __builtin_ia32_pause();
        //  std::this_thread::yield(); // very slow response time 
        }
        clock_gettime(CLOCK_MONOTONIC, &ts1);


        // *** Print measurement *** //
        std::uint64_t ns0 = to_nanosec(ts0);
        std::uint64_t ns1 = to_nanosec(ts1);
        std::uint64_t nsM = to_nanosec(tsM);
        stat01.add(ns1 - ns0);
        stat0M.add(nsM - ns0);
        statM1.add(ns1 - nsM);

        // *************************************************************************** //
        // There is no way to block initiator before each round, thus we have to wait. //
        // *************************************************************************** //
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    reactor.join();
    return std::make_tuple(stat01, stat0M, statM1);
}

void time_synchronization_with_atomic()
{
    statistics<std::uint64_t> stat01;
    statistics<std::uint64_t> stat0M;
    statistics<std::uint64_t> statM1;

    for(std::uint32_t n=0; n!=7; ++n)
    {
        for(std::uint32_t m=n+1; m!=8; ++m)
        {
            auto [a,b,c] = synchronization_with_atomic(n,m);     
            auto [d,e,f] = synchronization_with_atomic(m,n);     
            stat01 += a += d; 
            stat0M += b += e; 
            statM1 += c += f; 
        }
    }
    std::cout << "\nSynchonization with atomic : all-in-one";
    std::cout << "\n[stat01]" << stat01.get_string() << "\n";
    std::cout << "\n[stat0M]" << stat0M.get_string() << "\n";
    std::cout << "\n[statM1]" << statM1.get_string() << "\n";
}

