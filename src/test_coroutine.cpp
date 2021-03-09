#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>

// Big 6 in C++ coroutine
// 1. caller
// 2. coroutine
// 3. coroutine_handle  = resumption by caller (no customization, fixed by C++)
// 4. future            = customization of coroutine return value
// 5. promise           = customization of future construction (promise is the data-link between caller and coroutine)
// 6. awaitable         = customization of co_await return value (and other properties)
// 6a awaitable.ready   =  true : no allocation, no coroutine_handle created
//                      = false :    allocation of coroutine_states in heap, construction of coroutine_handle
// 6b awaitable.suspend =  true :    suspension, return control to caller
//                      = false : no suspension, yet time is wasted in construction of coroutine_handle


// ******************** // 
// *** Experiment 0 *** //
// ******************** // 
// How does caller get a coroutine_handle? 
// 1. pass handle pointer from caller to coroutine
// 2. pass handle pointer from coroutine to awaitable
// 3. init handle pointer inside awaitable::suspend
// 
// How does coroutine get a coroutine_handle?
// 1. own  handle pointer inside coroutine
// 2. pass handle pointer from coroutine to awaitable (same as above)
// 3. init handle pointer inside awaitable::suspend   (same as above)
//
// step-1 -> step 7 : steps before 1st resumption 
// step-A -> step E : steps after each resumption
//
// How to expand the flow in promise?
// How to expand the flow in awaitable?

struct future0 
{
    struct promise_type // This is promise_type, NOT promise.
    {
        promise_type()                       {  std::cout << "\npromise::promise";             } // step-1
        future0 get_return_object()          {  std::cout << "\npromise::ret_obj"; return {};  } // step-2
        void unhandled_exception()           {  }
        std::suspend_never initial_suspend() {  std::cout << "\npromise::initial"; return {};  } // step-4
        std::suspend_never   final_suspend() {  std::cout << "\npromise::final";   return {};  }
    };
    
    future0()
    {
        std::cout << "\nfuture0::future0"; // step-3
    }
};

struct awaitable0
{
    explicit awaitable0(std::coroutine_handle<>* h_ptr_) : h_ptr(h_ptr_), n(0) 
    {
        std::cout << "\nawait::await"; // step-5 
    }

    // No printing for constexpr fct.
    bool await_ready()  const noexcept            { std::cout << "\nawait::ready";  return ++n%2==0; } // step-6, step-D
    void await_suspend(std::coroutine_handle<> h) { std::cout << "\nawait::suspend(h)";  *h_ptr = h; } // step-7, step-E
    // -----------------------------------------------------------------------------------------------------------------
    void await_resume() const noexcept            { std::cout << "\nawait::resume";                  } //         step-B

    std::coroutine_handle<>* h_ptr;
    mutable std::uint32_t n;
};

future0 coroutine0(std::coroutine_handle<>* h_ptr)
{
    awaitable0 a{ h_ptr };
    for(std::uint32_t n=0;; ++n) 
    {
        co_await a;
        std::cout << "\ncoroutine0::iteration " << n << " with thread_id = " << std::this_thread::get_id(); // step-C
    }
}


// ******************** // 
// *** Experiment 1 *** //
// ******************** // 
// How does caller get a coroutine pointer (with another method)?
// 1. promise constructs a future object with coroutine_handle inside
// 2. caller obtains handle from future::get() or 
//    caller obtains handle from future::conversion_to_handle

struct future1 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   }
        future1 get_return_object()          {   std::cout << "\npromise::ret_obj";  
                                                 return future1
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }
    };

    explicit future1(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture1::future1"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
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


// ******************** // 
// *** Experiment 2 *** //
// ******************** // 
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
// Here, in this example, we use 2 awaitable objects in coroutine.
// 1. one awaitable for getting promise in coroutine, called once only
// 2. one awaitable returns std::suspend_always for suspension in coroutine

template<typename T>
struct future2 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } 
        future2 get_return_object()          {   std::cout << "\npromise::ret_obj"; 
                                                 return future2
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }

        T data; // <--- new stuff (as compared to previous)
    };

    explicit future2(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture2::future2"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture2::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
struct awaitable2
{
    awaitable2() : p_ptr(nullptr) 
    {
        std::cout << "\nawaitable::awaitable"; 
    }

    bool await_ready() const noexcept { return false; }
//  void await_suspend(std::coroutine_handle<> h) 
    bool await_suspend(std::coroutine_handle<typename future2<T>::promise_type> h) // <--- need to add promise_type inside handle
    {                                                                              //      don't forget keyword typename ...
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

//  std::coroutine_handle<>* h_ptr; // use promise-pointer instead of handle-pointer
    future2<T>::promise_type* p_ptr;
}; 

template<typename T>
[[nodiscard]] future2<T> coroutine2()
{
    // 1st awaitable is invoked once ...
    auto p_ptr = co_await awaitable2<T>{};

    for(std::uint32_t n=0;; ++n) 
    {
        p_ptr->data.set(n);

        // 2nd awaitable is invoked multiple times ...
        co_await std::suspend_always{};
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << p_ptr->data;
    }
}

struct pod
{
    char a;
    char b;
    char c;

    inline void set(std::uint32_t n)
    {
        a = 'a'+n;
        b = 'b'+n;
        c = 'c'+n;
    }
};

std::ostream& operator<<(std::ostream& os, const pod& x)
{
    os << x.a << x.b << x.c;
    return os;
}


// ******************** // 
// *** Experiment 3 *** //
// ******************** // 
// There are two approaches to transfer data between caller and coroutine :
//
// 1. The method we saw in experiment 2 ... 
// -  shared data in promise
// -  caller gets access to promise
// -  coroutine gets access to promise and update it 
// 2. The method we are going to test in experiment 3 ...
// -  shared data in promise
// -  caller gets access to promise 
// -  coroutine sends data to promise directly by co_yield or co_return
// -  hence in the following experiment, no customization of awaitable needed

template<typename T>
struct future3 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } 
        future3 get_return_object()          {   std::cout << "\npromise::ret_obj"; 
                                                 return future3
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }
        std::suspend_always  yield_value(const T& x) // <--- new stuff (as compared to previous)
        {
            data = x; // deep copy
            return {};
        }

        T data; 
    };

    explicit future3(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture3::future3"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture3::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] future3<T> coroutine3()
{
    for(std::uint32_t n=0;; ++n) 
    {
        pod data;
        data.set(n);
        co_yield data;
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
    }
}

// ******************** // 
// *** Experiment 4 *** //
// ******************** // 
// Return std::suspend_always from initial_suspend if we need to delay the first production. 
// Return std::suspend_always from   final_suspend if we need to use handle when coroutine is done.

template<typename T>
struct future4 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } 
        future4 get_return_object()          {   std::cout << "\npromise::ret_obj"; 
                                                 return future4
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_always  final_suspend() {   std::cout << "\npromise::final";   return {};   }  // <--- new stuff (as compared to previous)
        std::suspend_always  yield_value(const T& x) // <--- new stuff (as compared to previous)
        {
            data = x; 
            return {};
        }

        T data; 
    };

    explicit future4(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture4::future4"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture4::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] future4<T> coroutine4()
{
    for(std::uint32_t n=0; n!=8; ++n) // <--- new stuff (as compared to previous), limited number of iterations
    {
        pod data;
        data.set(n);
        co_yield data;
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
    }
}


// ******************** // 
// *** Experiment 5 *** //
// ******************** // 
// Generator pattern
// 1. caller housekeeps the future returned from coroutine (instead of casting to handle)
// 2. caller invokes the future directly (instead of getting handle and invoke) 

template<typename T>
struct future5 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } 
        future5 get_return_object()          {   std::cout << "\npromise::ret_obj"; 
                                                 return future5
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_always  final_suspend() {   std::cout << "\npromise::final";   return {};   }
        std::suspend_always  yield_value(const T& x)
        {
            data = x; 
            return {};
        }

        T data; 
    };

    explicit future5(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture5::future5"; 
    } 

    // *** Conversion operator *** //
/*  operator bool() const 
    {
        return ;
    } */

    void operator()() 
    {
    }

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] future5<T> coroutine5()
{
    for(std::uint32_t n=0; n!=8; ++n) 
    {
        pod data;
        data.set(n);
        co_yield data;
        std::cout << "\ncoroutine1::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
    }
}


// ********************* //
// *** Test function *** //
// ********************* //
void test_coroutine()
{
    std::cout << "\ncaller with main thread_id = " << std::this_thread::get_id();
    std::cout << "\n";


    // *** Experiment 0 *** //
    std::coroutine_handle<> h0;
    std::cout << "\n--------------------";
    coroutine0(&h0);
    std::cout << "\n--------------------";
    for(int i=0; i<16; ++i)
    {
        std::cout << "\ncaller invokes handle"; // step-A
        h0();
    }
    std::cout << "\n\n";


    // *** Experiment 1 *** //
    std::cout << "\n--------------------";
    std::coroutine_handle<> h1 = coroutine1();
    std::cout << "\n--------------------";
    for(int i=0; i<8; ++i) 
    {
        std::cout << "\ncaller invokes handle";
        h1();
    }
    std::cout << "\n\n";


    // *** Experiment 2 *** //
    std::cout << "\n--------------------";
    std::coroutine_handle<future2<pod>::promise_type> h2 = coroutine2<pod>();
    future2<pod>::promise_type& p2 = h2.promise();
    std::cout << "\n--------------------";
    for(int i=0; i<8; ++i) 
    {
        std::cout << "\ncaller invokes handle, data = " << p2.data;
        h2();
    }
    std::cout << "\n\n";


    // *** Experiment 3 *** // 
    std::cout << "\n-------------------- [The caller part is the same as in expt 2&3.]";
    std::coroutine_handle<future3<pod>::promise_type> h3 = coroutine3<pod>();
    future3<pod>::promise_type& p3 = h3.promise();
    std::cout << "\n--------------------";
    for(int i=0; i<8; ++i) 
    {
        std::cout << "\ncaller invokes handle, data = " << p3.data;
        h3();
    }
    std::cout << "\n\n";
    

    // *** Experiment 4 *** // 
    std::cout << "\n--------------------";
    std::coroutine_handle<future4<pod>::promise_type> h4 = coroutine4<pod>();
    future4<pod>::promise_type& p4 = h4.promise();
    std::cout << "\n--------------------";
    while(!h4.done()) 
    {
        std::cout << "\ncaller invokes handle, data = " << p4.data;
        h4();
    }
    std::cout << "\n\n";

    
    // *** Explicit destroy handle in heap *** //
    h0.destroy();
    h1.destroy();
    h2.destroy();
    h3.destroy();
    h4.destroy();
}

