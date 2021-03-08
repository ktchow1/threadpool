
#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>

// ****************** // 
// *** Approach 0 *** //
// ****************** // 
// How does caller get a coroutine_handle? 
// 1. pass handle pointer from caller to coroutine
// 2. pass handle pointer from coroutine to awaitable
// 3. init handle pointer inside awaitable::suspend
// 
// How does coroutine get a coroutine_handle?
// 1. own  handle pointer inside coroutine
// 2. pass handle pointer from coroutine to awaitable (same as above)
// 3. init handle pointer inside awaitable::suspend   (same as above)

struct future0 
{
    struct promise_type // This is promise_type, NOT promise.
    {
        promise_type()                       {  std::cout << "\npromise::promise";             } // step-1
        future0 get_return_object()          {  std::cout << "\npromise::ret_obj"; return {};  } // step-2
        std::suspend_never initial_suspend() {  std::cout << "\npromise::initial"; return {};  } // step-4
        std::suspend_never   final_suspend() {  std::cout << "\npromise::final";   return {};  }
        void unhandled_exception()           {  }
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
    bool await_ready()  const noexcept            { std::cout << "\nawait::ready";    return false; } // step-6 before co_await called
    void await_suspend(std::coroutine_handle<> h) { std::cout << "\nawait::suspend(h)"; *h_ptr = h; } // step-7 before co_await called
    // ----------------------------------------------------------------------------------------------------------------------------------
    void await_resume() const noexcept            { std::cout << "\nawait::resume";                 } // step-8 after  co_await resumed 

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


// ****************** // 
// *** Approach 1 *** //
// ****************** // 
// How does caller get a coroutine pointer (with another method)?
// 1. promise constructs a future object with coroutine_handle inside
// 2. caller obtains handle from future::get() or 
//    caller obtains handle from future::conversion_to_handle

struct future1 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } // step-1
        future1 get_return_object()          {   std::cout << "\npromise::ret_obj";     // step-2
                                                 return future1
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   } // step 4
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }
        void unhandled_exception()           {   }
    };

    explicit future1(std::coroutine_handle<promise_type> h_) : h(h_) // step-3 
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


// ****************** // 
// *** Approach 2 *** //
// ****************** // 
// How does coroutine get a coroutine handle (with another method)?
// 1. coroutine invokes co_await operator and capture the return value
// 2. define the return value in awaitable::resume
//
// Besides ...
// coroutine_handle can be converted into promise by : p = coroutine_handle::promise()
// promise can be converted into coroutine_handle by : h = coroutine_handle::from_promise(*this)
// 
// Now caller can access coroutine_handle,
// coroutine can access coroutine_handle,
// we form data connection between caller and coroutine, 
// we can put data inside promise for communication.
//
// Here, in this example, we have 2 awaitable objects in coroutine.
// 1. awaitable for getting promise in coroutine, called once only
// 2. awaitable std::suspend_always for looping in coroutine

template<typename T>
struct future2 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } // step-1
        future2 get_return_object()          {   std::cout << "\npromise::ret_obj";     // step-2
                                                 return future2
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   } // step 4
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }
        void unhandled_exception()           {   }

        T data; // <--- new stuff (as compared to previous)
    };

    explicit future2(std::coroutine_handle<promise_type> h_) : h(h_) // step-3
    {
        std::cout << "\nfuture2::future2"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const // step-5
    {
        std::cout << "\nfuture2::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
struct awaitable2
{
    awaitable2() : p_ptr(nullptr) // step-5 
    {
        std::cout << "\nawaitable::awaitable"; 
    }

    bool await_ready() const noexcept { return false; }
//  void await_suspend(std::coroutine_handle<> h) 
    bool await_suspend(std::coroutine_handle<typename future2<T>::promise_type> h) // <--- need to add promise_type inside handle
    {                                                                              //      dont forget typename ...
        std::cout << "\nawaitable::suspend(handle)";
        p_ptr = &h.promise();   

        // We don't want suspension from this awaitable
        // it offers an access to handle only.
        return false; 
    }
    typename future2<T>::promise_type* await_resume() const noexcept
    { 
        return p_ptr;
    }

//  std::coroutine_handle<>* h_ptr; // use pointer to promise instead of pointer to handle 
    future2<T>::promise_type* p_ptr;
}; 

template<typename T>
[[nodiscard]] future2<T> coroutine2()
{
    for(std::uint32_t n=0;; ++n) 
    {
        co_await std::suspend_always{};
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id();
    }
}

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


    // *** Approach 2 *** //
    std::cout << "\n--------------------";
    std::coroutine_handle<> h2 = coroutine2<std::uint32_t>();
    std::cout << "\n--------------------";
    std::cout << "\ncaller with main thread_id = " << std::this_thread::get_id();
    for(int i=0; i<10; ++i) h2();
    std::cout << "\n\n";


    // *** Explicit destroy handle in heap *** //
    h0.destroy();
    h1.destroy();
    h2.destroy();
}


