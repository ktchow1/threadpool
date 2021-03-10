#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>


struct pod
{
    char a;
    char b;
    char c;

    inline void set(std::uint32_t n)
    {
        a = 'A'+n;
        b = 'B'+n;
        c = 'C'+n;
    }
};

inline std::ostream& operator<<(std::ostream& os, const pod& x)
{
    os << x.a << x.b << x.c;
    return os;
}

// Generator pattern
// 1. caller housekeeps future (generator) instead of handle
// 2. caller invokes the future (generator) instead of handle
// 3. generator can be checked with IF condition


template<typename T>
struct generator // which is the future 
{
    struct promise_type 
    {
        promise_type() : n(0)
        { 
            std::cout << "\npromise::promise";   
        } 
        
        generator<T> get_return_object() 
        { 
            std::cout << "\npromise::ret_obj"; 
            return generator<T>
            {
                std::coroutine_handle<promise_type>::from_promise(*this) 
            };
        }   

        void unhandled_exception(){}

        std::suspend_always initial_suspend() // can be suspend_always or suspend_never
        { 
            std::cout << "\npromise::initial"; 
            return {};   
        }

        std::suspend_always final_suspend() // segmentation fault if return suspend_never
        {
            std::cout << "\npromise::final";   
            return {};  
        }

        std::suspend_always yield_value(const T& x)
        {
            data = x; 
            ++n;
            return {};
        }

        T data; 
        std::uint32_t n;
    };

    explicit generator(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\ngenerator::generator"; 
    } 

   ~generator()
    {
        h.destroy();
    }

    // *** Conversion operator *** //
    operator bool() const 
    {
        return !h.done();
    } 

    const T& operator()() 
    {
        h();
        return h.promise().data;
    }

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] generator<T> coroutine(std::uint32_t n0)
{
    for(std::uint32_t n=n0; n!=n0+8; ++n) 
    {
        pod data;
        data.set(n);
        co_yield data;
        std::cout << "\ncoroutine::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
    }
}


// ********************************************************** //
// *** Concurrency of multi-generators (not parallelism)  *** //
// ********************************************************** //
void test_coroutine_gen()
{
    std::cout << "\n--------------------";
    generator<pod> g0 = coroutine<pod>(0);
    generator<pod> g1 = coroutine<pod>(10);
    generator<pod> g2 = coroutine<pod>(20);
    std::cout << "\n--------------------";
    while(g0 && g1 && g2) 
    {
        auto t0 = g0();
        auto t1 = g1();
        auto t2 = g2();
        std::cout << "\ncaller invokes handle, data = " << t0 << " " << t1 << " " << t2;
    }
    std::cout << "\n\n";
}

