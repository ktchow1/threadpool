#include<iostream>

void test_epoll();
void test_invocable();
void test_unlock_mutex_in_another_thread();
void test_promise_and_future(); 
void test_jthread0(); 
void test_jthread1(); 


void test_coroutine();
void test_coroutine_gen();
void test_coroutine_pc();


int main(int argc, char* argv[])
{
//  test_epoll();
//  test_invocable();
//  test_unlock_mutex_in_another_thread();
//  test_jthread0();
//  test_jthread1();
  
    // ******************* //
    // *** Time thread *** //
    // ******************* //

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

