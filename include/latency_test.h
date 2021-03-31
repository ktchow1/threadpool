
#ifndef __EXPERIMENTAL_SYNC_TEST_H__
#define __EXPERIMENTAL_SYNC_TEST_H__
#include<iostream>
#include<numeric>
#include<thread.h>
#include<sync.h>
#include<stat.h>
#include<atomic>

// ********************************************************************* //
// *** Generic test for synchonization between producer and consumer *** //
// ********************************************************************* //
template<typename SYNC>
void sync_test(const std::string& label, std::uint32_t N, std::uint32_t us)
{
    // *** Sync mechanism *** //
    SYNC sync;
    std::atomic<std::uint32_t> ready = 0;

    // *** Measurement *** //
    timespec ts0;
    timespec ts1;
    statistics<std::uint64_t> stat{};

    std::thread consumer
    (
        [&]()
        {
            set_this_thread_affinity(2);
            set_this_thread_policy(SCHED_FIFO);
            for(std::uint32_t n=0; n!=N; ++n)
            {
                ready.fetch_add(1); // *** sync point
                sync.wait();        // *** consumer is blocked

                // *** stop timer *** //
                clock_gettime(CLOCK_MONOTONIC, &ts1);
                stat.add(to_nanosec(ts1)-to_nanosec(ts0));
            }
        }
    );

    // *** Producer *** //
    {
        set_this_thread_affinity(4);
        set_this_thread_policy(SCHED_FIFO);
        for(std::uint32_t n=0; n!=N; ++n)
        {
            // Yield inside while loop is necessary in real-time mode, or consumer fails to fetch-add.
            while(ready.load()!=n+1) 
            {
                std::this_thread::yield(); // *** sync point
            }
            std::this_thread::sleep_for(std::chrono::microseconds(us)); // *** wait until consumer is blocked

            clock_gettime(CLOCK_MONOTONIC, &ts0);
            sync.notify();
        }
    }

    consumer.join();
    std::cout << "\n[" << label << "] N=" << N << " us=" << us;
    std::cout << stat.get_string();
    std::cout << "\n";
}


// ********************************************************************* //
// *** Generic test for lockfree queue between producer and consumer *** //
// ********************************************************************* //
struct task_output
{
    task_output() : value(0) {}

    std::uint32_t value;
    timespec ts0;
    timespec ts1;
};

struct task
{
public:
    task() = default;
   ~task() = default;
    task(const task&) = default;
    task(task&&) = default;
    task& operator=(const task&) = default;
    task& operator=(task&&) = default;

public:
    task(std::uint32_t task_id_, std::vector<task_output>& work_done_) : task_id(task_id_), work_done(&work_done_)
    {
        clock_gettime(CLOCK_MONOTONIC, &(*work_done)[task_id].ts0); 
    }

    inline void operator()()
    {
        clock_gettime(CLOCK_MONOTONIC, &(*work_done)[task_id].ts1); 
        ++(*work_done)[task_id].value; 
    }

public:
    std::uint32_t task_id;
    std::vector<task_output>* work_done;
};

// Remarks :
// 1. Q is constructed outside test, as constructor for different Qs take different parameters.
// 2. T is movable only         for     my lockfree_queue
//    T is movable and copyable for boost::lockfree::queue
template<typename Q> 
void queue_test(Q& queue, std::uint32_t num_producers, std::uint32_t num_consumers, std::uint32_t num_tasks)
{
    std::atomic<bool>          running(true);
    std::atomic<std::uint32_t> ready(0);
    std::vector<std::thread>   producers;
    std::vector<std::thread>   consumers;
    std::vector<task_output>   work_done;  // Each entry is accessed by one consumer, hence no atomic needed.
    std::vector<std::uint32_t> work_count; // Each entry is accessed by one consumer, hence no atomic needed.

    set_this_thread_affinity(num_consumers+num_producers); // for main thread
    for(std::uint32_t n=0; n!=num_producers*num_tasks; ++n)
    {
        work_done.push_back(task_output{});
    }
    for(std::uint32_t n=0; n!=num_consumers; ++n)
    {
        work_count.push_back(0);
    }

    // ***************** //
    // *** Consumers *** //
    // ***************** //
    for(std::uint32_t n=0; n!=num_consumers; ++n)
    {
        consumers.push_back(std::thread([&](std::uint32_t consumer_id)
        {
            set_this_thread_affinity(n); 
            set_this_thread_priority();         // This is necessary, otherwise latency can be as large as 50us.
        //  set_this_thread_policy(SCHED_FIFO); // This policy in consumer is fine. But NOT necessary.
            task item;

            // Step 1 : Loop until running is false
            while(running.load())
            {
                ready.fetch_add(1);
                if (queue.pop(item))
                {
                    item();
                    ++work_count[consumer_id];
                } 
            }

            // Step 2 : Loop until queue is clear
            while(queue.pop(item))
            {
                item();
                ++work_count[consumer_id];
            } 
        }
        ,n));
    }
    
    // ***************** //
    // *** Producers *** //
    // ***************** //
    for(std::uint32_t n=0; n!=num_producers; ++n)
    {
        producers.push_back(std::thread([&](std::uint32_t producer_id)
        {
            set_this_thread_affinity(n+num_consumers);
            set_this_thread_priority();         // This is necessary, otherwise latency can be as large as 50us.
        //  set_this_thread_policy(SCHED_FIFO); // This policy kills both boost::lockfree::queue & my lockfree_queue.

            while(ready.load() < num_consumers);
            for(std::uint32_t m=0; m!=num_tasks; ++m)
            {
                task t(num_tasks*producer_id+m, work_done);
                while(!queue.push(t))
                {
                    t = task{num_tasks*producer_id+m, work_done}; // clock_gettime again ...
                }
                // This is necessary, otherwise latency can be as large as 500ns. 
                std::this_thread::sleep_for(std::chrono::nanoseconds(300)); 
            }
        }
        ,n));
    }

    // ******************** //
    // *** Check answer *** //
    // ******************** //
    for(auto& x:producers) x.join();
    running.store(false);
    for(auto& x:consumers) x.join();    

    std::uint32_t total = std::accumulate(work_count.begin(), work_count.end(), 0); 
    std::uint32_t error = 0;
    statistics<std::uint64_t> stat{};
    for(const auto& x:work_done) 
    {
        if (x.value!=1) ++error;
        stat.add(to_nanosec(x.ts1)-to_nanosec(x.ts0));
    }
   
    for(std::uint32_t n=0; n!=num_consumers; ++n) 
    {
        std::cout << "\nconsumer_" << n << " = " << work_count[n];
    }
    std::cout << "\ntotal = " << total;
    std::cout << "\nerror = " << error;
    std::cout << "\nstat  = " << stat.get_string();
}

#endif
