#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>


// ******************** // 
// *** Experiment 0 *** //
// ******************** // 
struct future0 
{
    struct promise_type // This is promise_type, NOT promise.
    {
        promise_type()                       {  std::cout << "\npromise::promise";             }
        future0 get_return_object()          {  std::cout << "\npromise::ret_obj"; return {};  }
        void unhandled_exception()           {  }
        std::suspend_never initial_suspend() {  std::cout << "\npromise::initial"; return {};  } 
        std::suspend_never   final_suspend() {  std::cout << "\npromise::final";   return {};  }
    };
    
    future0()
    {
        std::cout << "\nfuture0::future0"; 
    }
};

struct awaitable0
{
    explicit awaitable0(std::coroutine_handle<>* h_ptr_) : h_ptr(h_ptr_)
    {
        std::cout << "\nawait::await"; 
    }

    // No printing for constexpr fct.
    bool await_ready()  const noexcept            { std::cout << "\nawait::ready";     return false; } 
    void await_suspend(std::coroutine_handle<> h) { std::cout << "\nawait::suspend(h)";  *h_ptr = h; }
    void await_resume() const noexcept            { std::cout << "\nawait::resume";                  }

    std::coroutine_handle<>* h_ptr;
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
        std::cout << "\ncoroutine2::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << p_ptr->data;
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

inline std::ostream& operator<<(std::ostream& os, const pod& x)
{
    os << x.a << x.b << x.c;
    return os;
}


// ******************** // 
// *** Experiment 3 *** //
// ******************** // 
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
        std::cout << "\ncoroutine3::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
    }
}

// ******************** // 
// *** Experiment 4 *** //
// ******************** // 
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
        std::cout << "\ncoroutine4::iteration " << n << " with thread_id = " << std::this_thread::get_id() << ", data = " << data;
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
    std::cout << "\nExperiment 0";
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
    std::cout << "\nExperiment 1";
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
    std::cout << "\nExperiment 2";
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
    std::cout << "\nExperiment 3";
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
    std::cout << "\nExperiment 4";
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

