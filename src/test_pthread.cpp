
#include<iostream>
#include<thread.h>
#include<stat.h>
#include<unordered_set>

// ***************************** //
// *** Posix thread affinity *** //
// ***************************** //
struct thread_data
{
    std::uint32_t id;                         //  input
    std::uint64_t aff_ns;                     // output (time for changing affinity) 
    std::uint64_t pri_ns;                     // output (time for changing priority)
    std::unordered_set<std::uint32_t> cores0; // output
    std::unordered_set<std::uint32_t> cores1; // output

    inline void reset(std::uint32_t n)
    {
        id = n;
        aff_ns = 0;
        pri_ns = 0;
        cores0.clear();
        cores1.clear();
    }
};

void* thread_fct(void* arg)
{
    thread_data* data_ptr = reinterpret_cast<thread_data*>(arg);

    // *** Sample current core *** //
    for(std::uint32_t n=0; n!=10; ++n)
    {
        std::this_thread::yield();
        data_ptr->cores0.insert(sched_getcpu());
    }

    // *** Chanage affinity *** //
    timespec ts0;
    timespec ts1;
    timespec ts2;

    clock_gettime(CLOCK_MONOTONIC, &ts0);
    set_this_thread_affinity(data_ptr->id);
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    set_this_thread_priority(SCHED_RR);
    clock_gettime(CLOCK_MONOTONIC, &ts2);

    data_ptr->aff_ns = to_nanosec(ts1) - to_nanosec(ts0);
    data_ptr->pri_ns = to_nanosec(ts2) - to_nanosec(ts1);

    // *** Sample current core *** //
    for(std::uint32_t n=0; n!=10; ++n)
    {
        std::this_thread::yield();
        data_ptr->cores1.insert(sched_getcpu());
    }
    return NULL;
}

void test_pthread_affinity_and_priority()
{
    constexpr std::uint32_t num_threads = 3;
    statistics<std::uint64_t> aff_stat;                    // time for changing affinity 
    statistics<std::uint64_t> pri_stat;                    // time for changing priority      
    std::unordered_set<std::uint32_t> cores0[num_threads]; // core-id before setting affinity
    std::unordered_set<std::uint32_t> cores1[num_threads]; // core-id after  setting affinity

    std::cout << "\nChange pthread affinity ";
    for(std::uint32_t n=0; n!=1000; ++n)
    {
        if(n%20==0) std::cout << "." << std::flush;        
        pthread_t threads[num_threads];
        thread_data data[num_threads];

        // *** Create *** //
        for(std::uint32_t n=0; n!=num_threads; ++n) 
        {
            data[n].reset(n);
            pthread_create(threads+n, NULL, thread_fct, data+n);
            // arg[1] = pthread_attr_t which is NULL for default
            // arg[2] = function taking void* returning void*
            // arg[3] = function argument
        }

        // *** Join *** //
        for(std::uint32_t n=0; n!=num_threads; ++n) 
        {
            pthread_join(threads[n], NULL);        
        }

        // *** Measurement *** //
        for(std::uint32_t n=0; n!=num_threads; ++n)            
        {
            aff_stat.add(data[n].aff_ns);
            pri_stat.add(data[n].pri_ns);
            cores0[n].insert(data[n].cores0.begin(), data[n].cores0.end());
            cores1[n].insert(data[n].cores1.begin(), data[n].cores1.end());
        }
    }

    for(std::uint32_t n=0; n!=num_threads; ++n)            
    {
        std::cout << "\nthread " << n << " before affinity : "; for(const auto&x : cores0[n]) std::cout << x << " ";
        std::cout << "\nthread " << n << " after  affinity : "; for(const auto&x : cores1[n]) std::cout << x << " ";
    }

    std::cout << "\n";
    std::cout << "\nChange affinity" << aff_stat.get_string() << "\n";
    std::cout << "\nChange priority" << pri_stat.get_string() << "\n";
}

