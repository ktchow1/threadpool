#include<threadpool_cv.h>
#include<threadpool_jcv.h>


void test_threadpool_cv_impl(std::uint32_t throw_in_main,
                             std::uint32_t throw_in_task)
{
    threadpool_cv pool(4);
    for(std::uint32_t n=0; n!=20; ++n)
    {
        auto task = std::bind
        (
            [&](std::uint32_t m)
            {
                 std::cout << "\nHello " << m << " by thread " << std::this_thread::get_id() << std::flush; 
                 std::this_thread::sleep_for(std::chrono::milliseconds(500));

                 // ************************************** //
                 // If one of the tasks does throw :
                 // 1. try-catch in main thread does not help
                 // 2. try-catch is needed in child thread 
                 // ************************************** //
                 if (m==throw_in_task) throw std::runtime_error("ooo ... no");
            }, n
        );
        pool.add_task(task);

        // *************************************************************************** //
        // If main thread does throw :
        // 1. start stack unwind 
        // 2. destruct threadpool
        // 3. destruct thread which is running, invoke std::terminate
        // 
        // Solution : call threadpool_cv::stop() in ~threadpool_cv(), 
        //            need to check joinable in ~threadpool_cv() to avoid double join.
        // *************************************************************************** //
        
        if (n==throw_in_main) 
        {
            throw std::runtime_error("haha ...");
        } 

        // Add sleep to test if all waiting threads can be notified on termination. 
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    pool.stop();
}

void test_threadpool_cv()
{
    std::cout << "\n[Threadpool_cv : no throw]";
    try
    {
        test_threadpool_cv_impl(10000,10000);
    }
    catch(std::exception& e)
    {
        std::cout << "\nexception caught " << e.what() << std::flush;
    } 

    std::cout << "\n\n[Threadpool_cv : throw in main]";
    try
    {
        test_threadpool_cv_impl(12,10000);
    }
    catch(std::exception& e)
    {
        std::cout << "\nexception caught in main, " << e.what() << std::flush;
    } 

    std::cout << "\n\n[Threadpool_cv : throw in task]";
    try
    {
        test_threadpool_cv_impl(10000,12);
    }
    catch(std::exception& e)
    {
        std::cout << "\nexception caught in main, " << e.what() << std::flush;
    } 
    std::cout << "\n\n";
}


