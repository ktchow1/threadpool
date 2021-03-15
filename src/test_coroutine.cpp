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
        std::cout << "\nfuture::future"; 
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
        std::cout << "\ncoroutine::iteration " << n << " with thread_id = " << std::this_thread::get_id(); 
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
        std::cout << "\nfuture::future"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

[[nodiscard]] future1 coroutine1()
{
    for(std::uint32_t n=0;; ++n) 
    {
        co_await std::suspend_always{};
        std::cout << "\ncoroutine::iteration " << n << " with thread_id = " << std::this_thread::get_id();
    }
}


// ******************** // 
// *** Experiment 2 *** //
// ******************** // 
// T = data from caller to coroutine
// U = data from coroutine to caller
template<typename T, typename U>
struct future2 
{
    struct promise_type 
    {
        promise_type()                       {   std::cout << "\npromise::promise";   } 
        future2<T,U> get_return_object()     {   std::cout << "\npromise::ret_obj"; 
                                                 return future2<T,U>
                                                 { 
                                                     std::coroutine_handle<promise_type>::from_promise(*this) 
                                                 };
                                             }   
        void unhandled_exception()           {   }
        std::suspend_never initial_suspend() {   std::cout << "\npromise::initial"; return {};   }
        std::suspend_never   final_suspend() {   std::cout << "\npromise::final";   return {};   }

        T data_T; // <--- new stuff (as compared to previous)
        U data_U; // <--- new stuff (as compared to previous)
    };

    explicit future2(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture::future"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T, typename U>
struct awaitable2
{
    awaitable2() : data_T_ptr(nullptr), data_U_ptr(nullptr)
    {
        std::cout << "\nawait::await"; 
    }

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<typename future2<T,U>::promise_type> h) // <--- need to add promise_type inside handle
    {
        std::cout << "\nawait::suspend(handle)";
        data_T_ptr = &(h.promise().data_T);   
        data_U_ptr = &(h.promise().data_U);   
        return true; 
    }
    std::pair<T*,U*> await_resume() const noexcept
    { 
        return std::make_pair(data_T_ptr, data_U_ptr);
    }

    // Keep data-pointer instead of handle-pointer
    T* data_T_ptr;
    U* data_U_ptr;
}; 

template<typename T, typename U>
[[nodiscard]] future2<T,U> coroutine2()
{

    for(std::uint32_t n=0;; ++n) 
    {
        // Read t from caller to coroutine
        // Send u from coroutine to caller
        auto [T_ptr,U_ptr] = co_await awaitable2<T,U>{}; // wait for caller until t is ready
        U_ptr->a = T_ptr->a;
        U_ptr->b = T_ptr->a + T_ptr->n;
        U_ptr->c = T_ptr->a + T_ptr->n * 2;
        co_await std::suspend_always{}; // u is ready and wait for caller

        std::cout << "\ncoroutine::iteration " << n << ", " << *T_ptr << ", " << *U_ptr;
    }
}

// Sample POD
struct pod_T 
{
    char a;
    std::uint16_t n;
};

struct pod_U 
{
    char a;
    char b;
    char c;
};

inline std::ostream& operator<<(std::ostream& os, const pod_T& t)
{
    os << "T = " << t.a << "_" << t.n;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const pod_U& u)
{
    os << "U = " << u.a << u.b << u.c;
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
        std::cout << "\nfuture::future"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] future3<T> coroutine3()
{
    for(std::uint16_t n=0;; ++n) 
    {
        T t{ static_cast<char>('A'+n), n };

        co_yield t;
        std::cout << "\ncoroutine::iteration " << n << ", " << t;
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
        std::suspend_always  yield_value(const T& x)
        {
            data = x; 
            return {};
        }

        T data; 
    };

    explicit future4(std::coroutine_handle<promise_type> h_) : h(h_) 
    {
        std::cout << "\nfuture::future"; 
    } 

    // *** Conversion operator *** //
    operator std::coroutine_handle<promise_type>() const 
    {
        std::cout << "\nfuture::convert_to_handle"; 
        return h;
    } 

    std::coroutine_handle<promise_type> h;
};

template<typename T>
[[nodiscard]] future4<T> coroutine4()
{
    for(std::uint16_t n=0; n!=8; ++n) // <--- new stuff (as compared to previous), limited number of iterations
    {
        T t{ static_cast<char>('A'+n), n };
        co_yield t;
        std::cout << "\ncoroutine4::iteration " << n << ", " << t;
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
    for(int i=0; i<8; ++i)
    {
        std::cout << "\ncaller invokes handle"; 
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
    std::coroutine_handle<future2<pod_T,pod_U>::promise_type> h2 = coroutine2<pod_T,pod_U>();
    auto& p2 = h2.promise();
    std::cout << "\n--------------------";
    for(int i=0; i<8; ++i) 
    {
        p2.data_T.a = static_cast<char>('A' + i);
        p2.data_T.n = i;

        std::cout << "\ncaller inputs " << p2.data_T;
        h2();
        std::cout << "\ncaller outputs " << p2.data_U;
        h2();
    }
    std::cout << "\n\n";


    // *** Experiment 3 *** // 
    std::cout << "\nExperiment 3";
    std::cout << "\n--------------------"; 
    std::coroutine_handle<future3<pod_T>::promise_type> h3 = coroutine3<pod_T>();
    auto& p3 = h3.promise();
    std::cout << "\n--------------------";
    for(int i=0; i<8; ++i) 
    {
        std::cout << "\ncaller outputs " << p3.data;
        h3();
    }
    std::cout << "\n\n";
    
  
    // *** Experiment 4 *** // 
    std::cout << "\nExperiment 4";
    std::cout << "\n--------------------";
    std::coroutine_handle<future4<pod_T>::promise_type> h4 = coroutine4<pod_T>();
    auto& p4 = h4.promise();
    std::cout << "\n--------------------";
    while(!h4.done()) 
    {
        std::cout << "\ncaller outputs " << p4.data;
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

