#include <iostream>
#include <thread>
#include <concepts>
#include <coroutine>
#include <exception>

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

// ********************************* //
// T = data from caller to coroutine
// U = data from coroutine to caller
// ********************************* //
template<typename T, typename U>
struct future 
{
    struct promise_type 
    {
        future<T,U> get_return_object() 
        { 
            return future<T,U>
            {
                std::coroutine_handle<promise_type>::from_promise(*this) 
            };
        }   

        void unhandled_exception(){}
        std::suspend_never  initial_suspend()   { return {}; }
        std::suspend_never  final_suspend()     { return {}; }
        std::suspend_always yield_value(const U& u) 
        {
            data_U = u; 
            return {};
        }

        T data_T; // from caller to coroutine 
        U data_U; // from coroutine to caller 
    };

    // ***************************************** //
    // *** Future construction / destruction *** //
    // ***************************************** //
    explicit future(std::coroutine_handle<promise_type> h_) : h(h_) {} 
   ~future()
    {
        h.destroy();
    }

    // Mimic python : sink.send(data)
    template<typename... ARGS>
    void send_T_to_coroutine(ARGS&&... args) const // universal reference
    {
        h.promise().data_T = T{ std::forward<ARGS>(args)... };
        h();
    }

    // Mimic python : data = next(source)
    const U& next_U_from_coroutine() const 
    {
        h();
        return h.promise().data_U;
    }

    std::coroutine_handle<promise_type> h;
};

template<typename T, typename U>
struct awaitable
{
    awaitable() : data_T_ptr(nullptr) {}
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<typename future<T,U>::promise_type> h) 
    {
        data_T_ptr = &(h.promise().data_T);
//      data_U_ptr = &(h.promise().data_U); 
        return true; 
    }
    const T& await_resume() const noexcept
    { 
        return *data_T_ptr;
    }

    T* data_T_ptr = nullptr;
//  U* data_U_ptr = nullptr; // As U is transmitted by co_yield, no need to keep it here.
}; 

// ************************* //
// *** Example coroutine *** //
// ************************* //
[[nodiscard]] future<pod_T, pod_U> coroutine()
{
    // Two suspensions in each loop
    while(true) 
    {
        const pod_T& t = co_await awaitable<pod_T, pod_U>{};

        pod_U u;
        u.a = t.a;
        u.b = t.a + t.n;
        u.c = t.a + t.n * 2;

        std::cout << "\ncoroutine : input " << t << ", output " << u;
        co_yield u;
    }
}

void test_coroutine_pc()
{
    auto f = coroutine();
    for(std::uint16_t n=0; n!=10; ++n) 
    {
        pod_T t{static_cast<char>('a'+n), n};

        f.send_T_to_coroutine(t);
        pod_U u = f.next_U_from_coroutine();

        std::cout << "\ncaller : input " << t << ", output " << u;
    }
    std::cout << "\n\n";
}

