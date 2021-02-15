
#include<iostream>
#include<thread.h>
#include<sync.h>
#include<stat.h>
#include<atomic>
#include<vector>


// ******************************************************* //
// Main differences between this test and sync_test are :
// * this test is for semaphore
// * this test is spmc
// * supports customized notification schedule
// ******************************************************* //
void test_semaphore()
{
    // *** Sync mechanism *** //
    std::mutex mcout;  
    sync_psemaphore s;
    std::atomic<std::uint32_t> ready = 0;


    std::vector<std::thread> threads;
    for(std::uint32_t m=0; m!=3; ++m)
    {
        threads.push_back(std::thread([&](std::uint32_t tid)
        {
            // *** Measurement *** //
            timespec ts0;
            timespec ts1;

            for(std::uint32_t n=0; n!=10; ++n)
            {
                // *** Print *** //
                {
                    std::lock_guard<std::mutex> lock(mcout);
                    std::cout << "\nconsumer " << tid << " waiting" << std::flush;
                }

                ready.fetch_add(1); // *** sync point 
                clock_gettime(CLOCK_MONOTONIC, &ts0);
                s.wait();           // *** consumer is blocked
                clock_gettime(CLOCK_MONOTONIC, &ts1);

                // *** Print *** //
                {
                    std::lock_guard<std::mutex> lock(mcout);
                    std::cout << "\nconsumer " << tid 
                              << " notified " << n 
                              << " sema = " << s.peek_value() 
                              << " time = " << to_nanosec(ts1)-to_nanosec(ts0) << std::flush;
                }

                // *** Do some work *** //
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            // *** Print *** //
            {
                std::lock_guard<std::mutex> lock(mcout);
                std::cout << "\nconsumer " << tid << " done";
            }
        }, m));
    }

    // *** Producer (any testing schedule) *** //
    while(ready.load()!=3) std::this_thread::yield();

    for(std::uint32_t n=0; n!=3; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=2; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=2; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=2; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=2; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=2; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=4; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for(std::uint32_t n=0; n!=10; ++n) s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    s.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    s.notify();


    // *** Test maximum number of notifications *** //
    for(std::uint32_t n=0; n!=100000; ++n) s.notify();
    std::cout << "\nsema = " << s.peek_value() << std::flush;
    for(std::uint32_t n=0; n!=100000; ++n) s.wait();
    std::cout << "\nsema = " << s.peek_value() << std::flush;

    
    for(auto& x:threads) x.join(); 
    std::cout << "\n";
}

