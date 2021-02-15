
#include<iostream>
#include<thread>
#include<mutex>

// **************************************************************************************** //
// [Stackoverflow question 57342160]
// Always to lock and unlock mutexin the same thread, yet the following result contradicts.
// **************************************************************************************** //
auto unlock_mutex_in_another_thread()
{
    std::mutex mutex;
    std::atomic<std::int16_t> flag = -1;
    std::int16_t expected = -1;

    // Race condition, either thread 0 or thread 1 may claim the flag
    std::thread t0
    (
        [&]()
        {
            mutex.lock();
            flag.compare_exchange_strong(expected, 0);
        }
    );

    std::thread t1
    (
        [&]()
        {
            mutex.lock();
            flag.compare_exchange_strong(expected, 1);
        }
    );

    std::this_thread::sleep_for(std::chrono::microseconds(100));
    mutex.unlock();
    t0.join();
    t1.join();
    return flag.load();
}

void test_unlock_mutex_in_another_thread()
{
    std::cout << "\nUnlock mutex in another thread";

    std::uint32_t num_x = 0;
    std::uint32_t num_0 = 0;
    std::uint32_t num_1 = 0;
    std::uint32_t N = 1000;

    for(std::uint32_t n=0; n!=N; ++n)
    {
        auto result = unlock_mutex_in_another_thread();
        if      (result == -1) ++num_x;
        else if (result ==  0) ++num_0;
        else if (result ==  1) ++num_1; 

        if ((n+1)%100 == 0)
        {
            std::cout << "\nTest_" << n 
                      << " thread_" << result << " won the race."
                      << " Statistic = " << num_x << "/" << num_0 << "/" << num_1 << "/" << num_x + num_0 + num_1
                      << std::flush;
        }
    }
    std::cout << "\n";
}

