#include<iostream>
#include<vector>
#include<thread>
#include<functional> // for std::placeholders

// ***************************** //
// Jthread is for RAII, as it :
// 1. stop on destructor
// 2. join on destructor
// ***************************** //
void test_jthread0()
{
    std::cout << "\n[TEST] stop source and stop token for std::thread";

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

    // *** Stop and join are auto for jthread *** //
    s_source.request_stop();
    for(auto& x:threads) x.join();

    std::cout << "\n\n";
}
  
void test_jthread1()
{
    std::cout << "\n[TEST] stop source and stop token for std::jthread";

//  std::stop_source s_source; // Change 1
    std::vector<std::jthread> threads;
    for(std::uint32_t n=0; n!=5; ++n)
    {
        // Change 2 : captured token becomes arg
        threads.push_back(std::jthread([](std::stop_token s_token, std::uint32_t thread_id)
        {
            std::uint32_t n = 0;
            while(!s_token.stop_requested())
            {
                std::cout << "\njthread " << thread_id << ", loop " << n << std::flush; 
                std::this_thread::sleep_for(std::chrono::milliseconds(400 + 40 * thread_id));
                ++n;
            }
            std::cout << "\njthread " << thread_id << ", done" << std::flush; 
        }, n)); 
    //  }, std::placeholders::_1, n)); // Why no placeholders needed?
    }
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Change 3 : Stop and join are auto
//  s_source.request_stop();
//  for(auto& x:threads) x.join(); 

    std::cout << "\n\n";
} 
