#include<threadpoolJ.h>


void test_threadpoolJ_impl()
{
    std::cout << "\nThreadpool-J test";
    threadpoolJ pool(4);

    for(std::uint32_t n=0; n!=20; ++n)
    {
        auto task = std::bind
        (
            [](std::uint32_t m)
            {
                 std::cout << "\nHello " << m << " by thread " << std::this_thread::get_id() << std::flush; 
                 std::this_thread::sleep_for(std::chrono::milliseconds(500));

            //   if (m==12) throw 12;
            }, n
        );
        pool.add_task(task);

        // Adding this sleep in main thread will result in missing-wakeup on termination.
        if (n<=19-4)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }   
    }
}

void test_threadpoolJ()
{
//  try
    {
        test_threadpoolJ_impl();
    }
/*  catch(...)
    {
        std::cout << "\nexception caught" << std::flush;
        throw; // rethrow is needed 
    }*/
}


