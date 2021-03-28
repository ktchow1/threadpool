struct cell
{
    std::atomic<int> hashed_key = EMPTY; // Use hashed_key (NOT key) as we can’t declare atomic<K>.
    std::atomic<int> value;
};

int probing(int n) { return n;     }
int probing(int n) { return n * n; }

template<typename K> struct lockfree_hash
{
	STATUS set(const K& key, int value) 
    {
        // (1) hash-function
        int hashed_key = murmurhash3(key); 

        // (2) probing 
        for(int n=0; n!=size; ++n) 
        {
            int index = (hashed_key + probing(n)) & mask; 

            // (3) compare-key
                int tmp = buckets[index].hashed_key.load(); // A “DCLP-liked” trick to reduce chance of slow CAS.
            if (tmp == EMPTY)
            {
                int expected = EMPTY;

                buckets[index].hashed_key.compare_exchange_strong(expected, hashed_key); 
                if (expected == EMPTY || 
                    expected == hashed_key)
                {
                    buckets[index].value.store(value);
                    return OK; 	
                }
            }
            else if (tmp == hashed_key)
            {
                buckets[index].value.store(value);
                return OK;
            }
        }
        return TABLE_IS_FULL;
    } 

    std::optional<V> get(const K& key)
    {
        // (1) hash-function
        int hashed_key = murmurhash3(key);

        // (2) probing 
        for(int n=0; n!=size; ++n) 
        {
            int index = (hashed_key + probing(n)) & mask;  

            // (3) compare-key
            int tmp = buckets[index].hashed_key.load();
            if (tmp == EMPTY)      return std::nullopt;
            if (tmp == hashed_key) return std::optional<V>(bucket[index].value.load());
        }
        return std::nullopt;
    } 

	static const int size = 1024;
	static const int mask = size-1;
    cell<V> buckets[size];
};
