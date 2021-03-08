
#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>

// ****************** // 
// *** Approach 0 *** //
// ****************** // 
struct future0 
{
    struct promise_type // This is promise_type, NOT promise.
    {
        promise_type()                       { std::cout << "\npromise::promise"; }             // step-1
        future0 get_return_object()          { std::cout << "\npromise::ret_obj"; return {}; }  // step-2
        std::suspend_never initial_suspend() { std::cout << "\npromise::initial"; return {}; }  // step-4
        std::suspend_never   final_suspend() { std::cout << "\npromise::final";   return {}; }
        void unhandled_exception()           { }
    };
    
    future0() // step-3
    {
        std::cout << "\nfuture0::future0"; 
    }
};

struct awaitable0
{
    explicit awaitable0(std::coroutine_handle<>* h_ptr_) : h_ptr(h_ptr_) // step-5 
    {
        std::cout << "\nawaitable::awaitable"; 
    }

    // No printing for constexpr fct.
    constexpr bool await_ready()  const noexcept { return false; }
    constexpr void await_resume() const noexcept {               }
    void await_suspend(std::coroutine_handle<> h) // step-6 and for each co_await called
    {
        std::cout << "\nawaitable::suspend(handle)";
        *h_ptr = h;   
    }

    std::coroutine_handle<>* h_ptr;
};

future0 coroutine0(std::coroutine_handle<>* h_ptr)
{
    awaitable0 a{ h_ptr };
    for(std::uint32_t n=0;; ++n) 
    {
        co_await a;
        std::cout << "\ncoroutine0::iteration " << n << " with thread_id = " << std::this_thread::get_id();
    }
}


// ********************************** // 
// *** Approach 1 (No await type) *** //
// ********************************** // 
struct future1 
{
    struct promise_type 
    {
        promise_type()                       { std::cout << "\npromise::promise"; } // step-1
        future1 get_return_object()          { std::cout << "\npromise::ret_obj";   // step-2
                                               return future1
                                               { 
                                                   std::coroutine_handle<promise_type>::from_promise(*this) 
                                               };
                                             }
        std::suspend_never initial_suspend() { std::cout << "\npromise::initial"; return {}; } // step 4
        std::suspend_never   final_suspend() { std::cout << "\npromise::final";   return {}; }
        void unhandled_exception()           { }
    };

    future1(std::coroutine_handle<promise_type> h_) : h(h_) // step-3
    {
        std::cout << "\nfuture1::future1"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const // step-5
    {
        std::cout << "\nfuture1::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

[[nodiscard]] future1 coroutine1()
{
    for(std::uint32_t n=0;; ++n) 
    {
        co_await std::suspend_always{};
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id();
    }
}


// ******************************** // 
// *** Approach 2 (return data) *** //
// ******************************** // 


// ********************* //
// *** Test function *** //
// ********************* //
void test_coroutine()
{
    // *** Approach 0 *** //
    std::coroutine_handle<> h0;
    std::cout << "\n--------------------";
    coroutine0(&h0);
    std::cout << "\n--------------------";
    std::cout << "\ncaller with main thread_id = " << std::this_thread::get_id();
    for(int i=0; i<10; ++i) h0();
    std::cout << "\n\n";


    // *** Approach 1 *** //
    std::cout << "\n--------------------";
    std::coroutine_handle<> h1 = coroutine1();
    std::cout << "\n--------------------";
    std::cout << "\ncaller with main thread_id = " << std::this_thread::get_id();
    for(int i=0; i<10; ++i) h1();
    std::cout << "\n\n";

    // *** Explicit destroy handle in heap *** //
    h0.destroy();
    h1.destroy();
}


