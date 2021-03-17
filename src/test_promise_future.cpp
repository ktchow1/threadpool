
#include<iostream>
#include<thread>
#include<atomic>

// ****************************************************** //
// *** Promise and future is just publication pattern *** //
// ****************************************************** //
template<typename T>
struct future
{
    future(const T& x, const std::atomic<bool>& f) : publication(x), flag(f) {}

    const T& get() const
    {
        while(!flag.load(std::memory_order_acquire));
        return publication;
    }

    const T& publication;
    const std::atomic<bool>& flag;
};

template<typename T>
struct promise
{
    promise() : flag(false){}

    future<T> get_future() const
    {
        return { publication, flag };
    }

    void set(const T& x)
    {
        publication = x;
        flag.store(true, std::memory_order_release);
    }

    T publication;
    std::atomic<bool> flag;
};

void test_promise_and_future()
{
    std::cout << "\nPromise and future is just publication pattern.";

    promise<std::uint32_t> p;
    auto f = p.get_future();

    std::thread t0([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        p.set(123);
    });

    std::thread t1([&]()
    {
        std::cout << "\nwait .... " << std::flush;
        std::cout << "I got " << f.get() << std::flush;
    });

    t0.join(); 
    t1.join(); 
    std::cout << "\n\n";
}

