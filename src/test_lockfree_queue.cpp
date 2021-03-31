
#include<iostream>
#include<boost/lockfree/spsc_queue.hpp>
#include<boost/lockfree/queue.hpp>
#include<locked_queue.h>
#include<lockfree_queue.h>
#include<latency_test.h>


void test_lockfree_queue()
{
    boost::lockfree::spsc_queue<task, boost::lockfree::capacity<1024>> queue0;
    boost::lockfree::queue<task> queue1(1024);
    YLib::locked_queue<task,spinlock> queue2;
    YLib::lockfree_queue<task,1024> queue3;

    std::uint32_t num_producers = 3;
    std::uint32_t num_consumers = 3;
    std::uint32_t num_tasks = 50000;

    std::cout << "\ntesting boost lockfree spscq" << std::flush;
    queue_test(queue0, 1, 1, num_tasks);
    std::cout << "\n";

    std::cout << "\ntesting boost lockfree queue" << std::flush;
    queue_test(queue1, num_producers, num_consumers, num_tasks);
    std::cout << "\n";
  
    std::cout << "\ntesting YLib locked queue" << std::flush;
    queue_test(queue2, num_producers, num_consumers, num_tasks);
    std::cout << "\n"; 

    std::cout << "\ntesting YLib lockfree queue" << std::flush;
    queue_test(queue3, num_producers, num_consumers, num_tasks);
    std::cout << "\n"; 
}

/*********************************************************************************************
Affinity is a must
Priority (using nice value)  is a must
Priority (using FIFO policy) should be avoided, it kills mpmcq
**********************************************************************************************
Here are the results for 3 producers, 3 consumers, 50000 tasks, 
with 300ns waiting time between any two productions in producer (unit = nanosec) :

percentile           | 0.1 |   1 |   5 |  10 |  25 |  50 |  75 |   90 |   95 |   99 | 99.9   |  
---------------------+-----+-----+-----+-----+-----+-----+-----+------+------+------+--------+
boost lockfree spscq |  86 |  97 | 106 | 108 | 112 | 116 | 122 |  127 |  130 |  138 |  157   | 
boost lockfree queue | 166 | 172 | 178 | 181 | 188 | 195 | 210 |  228 |  252 |  282 |  318   | 
YLib  locked   queue | 141 | 142 | 146 | 151 | 176 | 197 | 546 | 1527 | 2481 | 5798 |   12ms | 
YLib  lockfree queue |  83 |  88 | 116 | 117 | 119 | 122 | 126 |  128 |  130 |  135 |  151   | 
**********************************************************************************************/


