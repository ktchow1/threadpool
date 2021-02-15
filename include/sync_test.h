
#ifndef __EXPERIMENTAL_SYNC_TEST_H__
#define __EXPERIMENTAL_SYNC_TEST_H__
#include<iostream>
#include<thread.h>
#include<sync.h>
#include<stat.h>
#include<atomic>


template<typename SYNC>
void sync_test(const std::string& label, std::uint32_t N, std::uint32_t us)
{
    // *** Sync mechanism *** //
    SYNC sync;
    std::atomic<std::uint32_t> ready = 0;

    // *** Measurement *** //
    timespec ts0;
    timespec ts1;
    statistics<std::uint64_t> stat{};

    std::thread consumer
    (
        [&]()
        {
            set_this_thread_affinity(2);
            set_this_thread_priority(SCHED_FIFO);
            for(std::uint32_t n=0; n!=N; ++n)
            {
                ready.fetch_add(1); // *** sync point
                sync.wait();        // *** consumer is blocked

                // *** stop timer *** //
                clock_gettime(CLOCK_MONOTONIC, &ts1);
                stat.add(to_nanosec(ts1)-to_nanosec(ts0));
            }
        }
    );

    // *** Producer *** //
    {
        set_this_thread_affinity(4);
        set_this_thread_priority(SCHED_FIFO);
        for(std::uint32_t n=0; n!=N; ++n)
        {
            // Yield inside while loop is necessary in real-time mode, or consumer fails to fetch-add.
            while(ready.load()!=n+1) 
            {
                std::this_thread::yield(); // *** sync point
            }
            std::this_thread::sleep_for(std::chrono::microseconds(us)); // *** wait until consumer is blocked

            clock_gettime(CLOCK_MONOTONIC, &ts0);
            sync.notify();
        }
    }

    consumer.join();
    std::cout << "\n[" << label << "] N=" << N << " us=" << us;
    std::cout << stat.get_string();
    std::cout << "\n";
}

#endif
