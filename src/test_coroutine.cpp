
#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>

// ****************** // 
// *** Approach 0 *** //
// ****************** // 
struct future_type0 
{
    struct promise_type // This type cannot be changed. 
    {
        future_type0 get_return_object() 
        {
            return {}; 
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never   final_suspend() { return {}; }
        void unhandled_exception()           {            }
    };
};

struct awaitable_type0  
{
    std::coroutine_handle<>* h_ptr;
    constexpr bool await_ready()  const noexcept { return false; }
    constexpr void await_resume() const noexcept {               }
    void await_suspend(std::coroutine_handle<> h) 
    {
        *h_ptr = h;   
        std::cout << " [await_suspend] " << std::flush; 
    }
};

future_type0 counter0(std::coroutine_handle<>* h_ptr)
{
    awaitable_type0 a{ h_ptr };
    for(std::uint32_t n=0;; ++n) 
    {
        co_await a;
        std::cout << "\ncoroutine0 = " << n << " with thread_id = " << std::this_thread::get_id();
    }
}


// ********************************** // 
// *** Approach 1 (No await type) *** //
// ********************************** // 
struct future_type1 
{
    struct promise_type // This type cannot be changed. 
    {
        future_type1 get_return_object() 
        {
            return future_type1{      std::coroutine_handle<promise_type>::from_promise(*this) }; 
        //  return             { .h = std::coroutine_handle<promise_type>::from_promise(*this) }; 
            // C++20 designated initializer syntax, where . is necessary (both the following work)
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never   final_suspend() { return {}; }
        void unhandled_exception()           {            }
    };

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const // invoked in caching 
    {
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

[[nodiscard]] future_type1 counter1()
{
    for(std::uint32_t n=0;; ++n) 
    {
        co_await std::suspend_always{};
        std::cout << "\ncoroutine1 = " << n << " with thread_id = " << std::this_thread::get_id();
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
    counter0(&h0);

    std::cout << "\ncoroutine0 started with main thread_id = " << std::this_thread::get_id();
    for(int i=0; i<15; ++i) h0();
    std::cout << "\n";


    // *** Approach 1 *** //
    std::coroutine_handle<> h1 = counter1();

    std::cout << "\ncoroutine1 started with main thread_id = " << std::this_thread::get_id();
    for(int i=0; i<15; ++i) h1();
    std::cout << "\n";


    h0.destroy();
    h1.destroy();
}


/*  
Components involved : 
1. coroutine
2. coroutine_handle (housekeeping all coroutine states)
3. awaitable_object
4. promise_type + future_type

Requirements :
1. awaitable_object std::suspend_always and std::suspend_never are provided
2. awaitable_object (by some means) updates or returns a coroutine handle to coroutine caller
3. awaitable_object offers 3 functions
4. define future_type::promise_type exists
5. define promise_type::get_return_object() returns future_type
6. coroutine_handle contains an instance of promise_type (with a known offset)
   using function std::coroutine_handle<promise_type>::from_promise(*this)
   future_type is returned from coroutine
   future_type can be converted to std::coroutine_handle<>, please define the conversion
7. std::coroutine_handle<T> can be converted to std:coroutine_handle<void>  
   std::coroutine_handle<T> <---> std::promise_type is possible
   std::coroutine_handle<>  <---> NO ... 
   both handles can be converted into each other  

*/
