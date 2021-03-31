#include<iostream>
#include<cstring>
#include<vector>
#include<thread>
#include<lockfree_hashmap.h>

struct A
{
    std::uint16_t x;
    std::uint16_t y;
};

struct B
{
    std::uint16_t x;
    std::uint16_t y;
    std::uint16_t z;
    std::uint16_t w;
};

struct C
{
    char c[9];
};

struct F0
{
    std::uint64_t operator()(const A& a) const { return 1; }
    A operator()(std::uint64_t n) const { return A{}; }
};

struct F1
{
    std::uint64_t operator()(const A& a) const { return 1; }
//  A operator()(std::uint64_t n) const { return A{}; }
};

struct F2
{
    std::int64_t  operator()(const A& a) const { return 1; }
    A operator()(std::uint64_t n) const { return A{}; }
};

struct HA
{
    // With the following implementation,
    // more significant word of output = A::y
    // less significant word of output = A::x
    std::uint64_t operator()(const A& a) const
    {
        std::uint64_t n = 0;
        memcpy(reinterpret_cast<void*>(&n),
               reinterpret_cast<const void*>(&a), sizeof(A));
        return n;
    }

    A operator()(std::uint64_t n) const
    {
        A a;
        memcpy(reinterpret_cast<void*>(&a),
               reinterpret_cast<const void*>(&n), sizeof(A));
        return a;
    }
};

struct HB
{
    std::uint64_t operator()(const B& b) const
    {
        std::uint64_t n = *reinterpret_cast<const std::uint64_t*>(&b);
        return n;
    }

    B operator()(std::uint64_t n) const
    {
        B b;
        *reinterpret_cast<std::uint64_t*>(&b) = n;
        return b;
    }
};

std::ostream& operator<<(std::ostream& os, const A& a)
{
    os << "A[" << a.x << ", " << a.y << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const B& b)
{
    os << "B[" << b.x << ", " << b.y << ", " << b.z << ", " << b.w << "]";
    return os;
}

// ****************** //
// *** Basic test *** //
// ****************** //
void test_lockfree_hashmap_basic()
{
    std::cout << "\nTest concepts for lockfree_hashmap";
    std::cout << "\nconcept0 : " << LT8BYTES<std::uint32_t>;
    std::cout << "\nconcept0 : " << LT8BYTES<std::uint64_t>;
    std::cout << "\nconcept0 : " << LT8BYTES<A>;
    std::cout << "\nconcept0 : " << LT8BYTES<B>;
    std::cout << "\nconcept0 : " << LT8BYTES<C>;
    std::cout << "\nconcept1 : " << TO8BYTES<F0,A>;
    std::cout << "\nconcept1 : " << TO8BYTES<F1,A>;
    std::cout << "\nconcept1 : " << TO8BYTES<F2,A>;
    std::cout << "\n";

    // *************************************************** //
    std::cout << "\nTest POD to std::uint64_t conversion";
    A a0{10001, 20002};
    std::uint64_t na = HA{}(a0);
    A a1 = HA{}(na);

    B b0{10011, 20022, 30033, 40044};
    std::uint64_t nb = HB{}(b0);
    B b1 = HB{}(nb);

    std::cout << "\na0 = " << a0;
    std::cout << "\na1 = " << a1;
    std::cout << "\nna = " << na;
    std::cout << "\nb0 = " << b0;
    std::cout << "\nb1 = " << b1;
    std::cout << "\nnb = " << nb;
    std::cout << "\n";

    // *************************************************** //
    std::cout << "\nTest lockfree_hashmap";
    lockfree_hashmap<A,B,HA,HB,16> map;

    for(std::uint16_t n=0; n!=24; ++n)
    {
        A a{(std::uint16_t)(10000+n),
            (std::uint16_t)(20000+n)};
        B b{(std::uint16_t)(10000+n),
            (std::uint16_t)(20000+n),
            (std::uint16_t)(30000+n),
            (std::uint16_t)(40000+n)};
        if (!map.set(a,b)) { std::cout << "\nSet " << a << " : failed"; }
    }

    for(std::uint16_t n=0; n!=24; ++n)
    {
        A a{(std::uint16_t)(10000+n),
            (std::uint16_t)(20000+n)};
        auto b = map.get(a);
        if (b)
        {
            std::cout << "\nGet " << a << " : " << *b;
        }
        else
        {
            std::cout << "\nGet " << a << " : failed";
        }
    }
    std::cout << "\n";
}

// ************************ //
// *** Multithread test *** //
// ************************ //
void randomise(A& a, std::uint32_t N)
{
    // Intentionally choose multiple of 256 to create collision ...
    a.x = (rand()%N + 1) * 256;
    a.y = a.x + 16;
}

void calculate(const A& a, B& b)
{
    b.x = a.x;
    b.y = a.y;
    b.z = a.x+1;
    b.w = a.x+2;
}

bool verify(const A& a, const B& b)
{
    if (b.x != a.x)   return false;
    if (b.y != a.y)   return false;
    if (b.z != a.x+1) return false;
    if (b.w != a.x+2) return false;
    return true;
}

template<std::uint32_t NUM_CELLS> // choose 2,4,8,16,32 ...
auto test_lockfree_hashmap_multithread_impl()
{
    lockfree_hashmap<A,B,HA,HB,NUM_CELLS> map;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<std::uint16_t> completed_producers = 0;
    std::atomic<std::uint64_t> num_test = 0;
    std::atomic<std::uint64_t> num_error = 0;

    for(std::uint16_t num_threads=0; num_threads!=4; ++num_threads)
    {
        consumers.push_back(std::thread([&]()
        {
            while(completed_producers.load() < 4)
            {
                A a;
                randomise(a,NUM_CELLS);
                auto b = map.get(a);
                if (b)
                {
                    if (!verify(a,*b))
                    {
                        num_error.fetch_add(1);

                        // ************* //
                        // *** Debug *** //
                        // ************* //
                        std::cout << "\n\n[Failure] Getting " << a << " : " << *b;
                        map.debug();
                    }
                }
                num_test.fetch_add(1);
            }
        }));
    }

    for(std::uint16_t num_threads=0; num_threads!=4; ++num_threads)
    {
        producers.push_back(std::thread([&]()
        {
            for(std::uint16_t n=0; n!=100; ++n)
            {
                A a;
                B b;
                randomise(a,NUM_CELLS);
                calculate(a,b);
                map.set(a,b);
            }
            completed_producers.fetch_add(1);
        }));
    }

    for(auto& x:producers) x.join();
    for(auto& x:consumers) x.join();
    return std::make_pair(num_test.load(), num_error.load());
}

void test_lockfree_hashmap_multithread()
{
    static constexpr std::uint32_t NUM_CELLS = 8;
    for(std::uint32_t n=0; n!=300; ++n)
    {
        auto [num_test, num_error] = test_lockfree_hashmap_multithread_impl<NUM_CELLS>();

        std::cout << "\nlockfree_hashmap multithread test " << n << " : ";
        std::cout << num_test << ", " << num_error;
    }
}
