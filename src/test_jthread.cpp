
#include<iostream>
#include<vector>
#include<thread>

// ***************************** //
// Jthread is for RAII, as it :
// 1. stop on destructor
// 2. join on destructor
// ***************************** //
void test_jthread0()
{
    std::cout << "\n[TEST] stop source and stop token";

    std::stop_source s_source;
    std::vector<std::thread> threads;
    for(std::uint32_t n=0; n!=5; ++n)
    {
        threads.push_back(std::thread([s_token = s_source.get_token()](std::uint32_t thread_id)
        {
            std::uint32_t n = 0;
            while(!s_token.stop_requested())
            {
                std::cout << "\nthread " << thread_id << ", loop " << n << std::flush; 
                std::this_thread::sleep_for(std::chrono::milliseconds(400 + 40 * thread_id));
                ++n;
            }
            std::cout << "\nthread " << thread_id << ", done" << std::flush; 
        }, n));
    }

    std::this_thread::sleep_for(std::chrono::seconds(4));
    s_source.request_stop();

    for(auto& x:threads) x.join();
    std::cout << "\n\n";
}

// How can I repeat the above with jthread?


void test_jthread1()
{
    std::jthread jthread([](std::stop_token s_token)
    {
        std::uint32_t n=0;
        while(s_token.stop_requested())
        {
            std::cout << "\nhello " << n << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            ++n;
        }
        std::cout << "\nbyebye" << std::flush;
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
}
