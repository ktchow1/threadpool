
#include<iostream>
#include<sync_test.h>

void test_invocable();
void test_unlock_mutex_in_another_thread();

// *** Time thread *** //
void timer_resolution();
void time_thread_create();
void time_thread_created_by_async_call();    
void time_mutex_lock_and_unlock();           
void time_synchronization_with_atomic();     
void test_pthread_affinity_and_priority();

// *** Time wakeup *** //
void time_wakeup_mutex(std::uint32_t waiting_in_us);  
void time_wakeup_pmutex(std::uint32_t waiting_in_us);  
void time_wakeup_semaphore(std::uint32_t waiting_in_us);  
void time_wakeup_condvar(std::uint32_t waiting_in_us);
void time_wakeup_promfut(std::uint32_t waiting_in_us);
void test_semaphore();

// *** Threadpool *** //
void test_threadpool();




int main(int argc, char* argv[])
{
//  test_invocable();
//  test_unlock_mutex_in_another_thread();
  
    // ******************* //
    // *** Time thread *** //
    // ******************* //
/*  timer_resolution();                        //      14-15 ns
    time_thread_create();                      //  5000-8000 ns
    time_thread_created_by_async_call();       //  5000-8000 ns
    time_mutex_lock_and_unlock();              //         20 ns
    time_synchronization_with_atomic();        //     90-160 ns (for __builtin_ia32_pause) 
                                               //    500-600 ns (for std::this_thread::yield) 
    test_pthread_affinity_and_priority();      // 2000-20000 ns (for affinity)
                                               // 1200- 1800 ns (for priority-RR)
                                               // 1700- 2500 ns (for priority-IDLE)
*/  

    // ******************* //
    // *** Time wakeup *** //
    // ******************* //
    constexpr std::uint32_t NumTest = 10000;
//  for(std::uint32_t us : {0,10,100,1000,10000}) 
    for(std::uint32_t us : {10})
    {
        sync_test<sync_futex>("futex", NumTest, us);                              // 1700 ns
        sync_test<sync_mutex>("std mutex", NumTest, us);                          // 1700 ns
        sync_test<sync_pmutex>("posix mutex", NumTest, us);                       // 1700 ns
        sync_test<sync_pmutex2>("posix mutex2", NumTest, us);                     // ?
    //  sync_test<sync_semaphore>("std semaphore", NumTest, us);                  // ?
        sync_test<sync_psemaphore>("posix semaphore", NumTest, us);               // 1700 ns
        sync_test<sync_condvar>("condition variable", NumTest, us);               // 2200 ns
        sync_test<sync_promfut<NumTest>>("promise and future", NumTest, us);      // 2200 ns
    }   
//  test_semaphore();
    test_threadpool(); 


    // ********************************************************************************************************* //
    // Producer should wait until consumer is blocked in next iteration.
    // otherwise the wakeup latency is underestimated, it becomes < 0.5us.
    //
    // Provided that if the consumer is blocked.
    // The results form 2 clusters in my machine : 2.0us (4.5GHz cpu) and 20-30us (context switch of consumer)
    // The results form 1 cluster in dev machine : 3.5us (3.xGHz cpu)
    // Context switch happens in my machine as cpu are not isolated.
    //
    // The above test is for spsc scenario only.
    // For spmc scenario, we need 1 hitter + 1 oms_threadpool.
    // ********************************************************************************************************* //
    std::cout << "\n\n\n";
    return 0;
} 

