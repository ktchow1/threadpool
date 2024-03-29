

#include<iostream>
#include<latency_test.h>

void test_epoll();
void test_invocable();
void test_unlock_mutex_in_another_thread();
void test_promise_and_future(); 
void test_jthread0(); 
void test_jthread1(); 

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


// *** Others *** //
void test_threadpool();
void test_threadpool_cv();
void test_lockfree_queue();
void test_lockfree_hashmap_basic();
void test_lockfree_hashmap_multithread();
void test_coroutine();
void test_coroutine_gen();
void test_coroutine_pc();


int main(int argc, char* argv[])
{
//  test_epoll();
//  test_invocable();
//  test_unlock_mutex_in_another_thread();
//  test_promise_and_future();
//  test_jthread0();
//  test_jthread1();
  
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
/*  std::uint32_t NumTest = 10000;
    std::uint32_t us = 10;

    sync_test<sync_futex>("futex", NumTest, us);                              // 1700 ns
    sync_test<sync_mutex>("std mutex", NumTest, us);                          // 1700 ns
    sync_test<sync_pmutex>("posix mutex", NumTest, us);                       // 1700 ns
    sync_test<sync_HansBarz>("posix HansBarz", NumTest, us);                  // 1700 ns
//  sync_test<sync_semaphore>("std semaphore", NumTest, us);                  // ?
    sync_test<sync_psemaphore>("posix semaphore", NumTest, us);               // 1700 ns
    sync_test<sync_condvar>("condition variable", NumTest, us);               // 2200 ns
    sync_test<sync_promfut<NumTest>>("promise and future", NumTest, us);      // 2200 ns
*/
//  test_semaphore();
    test_threadpool(); 
    test_threadpool_cv();  // <--- threadpool with condition variable
    test_lockfree_queue();    
    test_lockfree_hashmap_basic();
    test_lockfree_hashmap_multithread();
    test_coroutine();
//  test_coroutine_gen();
//  test_coroutine_pc();


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

