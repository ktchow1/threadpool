#ifndef __LOCKFREE_HASHMAP_H__
#define __LOCKFREE_HASHMAP_H__
#include<iostream>
#include<atomic>
#include<array>
#include<limits>
#include<optional>

// ********************************************************************************************* //
// There are drawbacks for the proposed algo in atomic.doc, we can either make it :
// 1. SPMC supporting generic K and generic constant V (need publication flag)  <--- remark 1
// 2. SPMC supporting generic K and integer mutable V                           <--- remark 2
// 3. MPMC supporting integer K and integer mutable V
// ---------------------------------------------------------------------------------------------
// 4. MPMC supporting generic K and generic mutable V can be done only if :
// -  ensuring std::atomic<K>{}.is_lock_free()
// -  ensuring std::atomic<V>{}.is_lock_free()
// -  OR ... replace open-address by separate-chaining (lockfree list, Anothoy William)
// ---------------------------------------------------------------------------------------------
// Remark 1. Consumers need to confirm K is valid by checking publication flag.
// Remark 2. Consumers need to confirm K is valid by checking V != INVALID.
//
// Publication pattern does not support :
// -  multi producers race
// -  multi productions (i.e. repeated writes to the data, even by single producer)
// ********************************************************************************************* //
// THIS VERSION IS TESTED
// - no failure
// - no memory leak in valgrind



template<typename T>
concept LT8BYTES = (sizeof(T) <= 8);

template<typename F, typename T>
concept TO8BYTES = requires (F x)
{
    { x(std::declval<T>()) }             -> std::same_as<std::uint64_t>;
    { x(std::declval<std::uint64_t>()) } -> std::same_as<T>;
};

static constexpr
std::uint64_t EMPTY = std::numeric_limits<std::uint64_t>::max();

struct cell
{
    cell() : hashed_key(EMPTY), hashed_value(EMPTY) {}
    alignas(64) std::atomic<std::uint64_t> hashed_key;
    alignas(64) std::atomic<std::uint64_t> hashed_value;
};

// *********************************************************************** //
// *** MPMC lockfree hashmap with integer key, integer value (mutable) *** //
// *********************************************************************** //
template<LT8BYTES K, LT8BYTES V, TO8BYTES<K> K_HASH, TO8BYTES<V> V_HASH, std::uint32_t SIZE>
class lockfree_hashmap
{
public:
    bool set(const K& key, const V& value)
    {
        // (1) Hash function
        std::uint64_t hashed_key = K_HASH{}(key);
        for(std::uint32_t n=0; n!=SIZE; ++n)
        {
            // (2) Probing
            std::uint32_t index = (hashed_key + n) & mask;   // linear
        //  std::uint32_t index = (hashed_key + n*n) & mask; // quadratic

            // (3) Compare hashed_key
            std::uint64_t tmp = buckets[index].hashed_key.load(); // Optimization : avoid too many CAS
            if (tmp == EMPTY)
            {
                std::uint64_t expected = EMPTY;

                buckets[index].hashed_key.compare_exchange_strong(expected, hashed_key);
                if (expected == EMPTY ||
                    expected == hashed_key)
                {
                    buckets[index].hashed_value.store(V_HASH{}(value));
                    return true;
                }
            }
            else if (tmp == hashed_key)
            {
                buckets[index].hashed_value.store(V_HASH{}(value));
                return true;
            }
        }
        return false;
    }

    std::optional<V> get(const K& key)
    {
        // (1) Hash function
        std::uint64_t hashed_key = K_HASH{}(key);
        for(std::uint32_t n=0; n!=SIZE; ++n)
        {
            // (2) Probing
            std::uint32_t index = (hashed_key + n) & mask;   // linear
        //  std::uint32_t index = (hashed_key + n*n) & mask; // quadratic

            // (3) Compare hashed_key
            std::uint64_t tmp = buckets[index].hashed_key.load();
            if (tmp == EMPTY) return std::nullopt;
            if (tmp == hashed_key)
            {
                auto out = buckets[index].hashed_value.load();
                if (out == EMPTY) return std::nullopt; // BUG : Missing this line
                return std::optional<V>(V_HASH{}(out));
            }
        }
        return std::nullopt;
    }

public:
    void debug() const
    {
        for(std::uint32_t n=0; n!=SIZE; ++n)
        {
            std::cout << "\nlockfree_hashmap " << K_HASH{}(buckets[n].hashed_key)
                                      << " : " << V_HASH{}(buckets[n].hashed_value);
        }
    }

private:
    static const int mask = SIZE-1;
    cell buckets[SIZE];
};

#endif
