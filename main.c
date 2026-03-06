#include "cache.h"
#include <unistd.h>
#include <stdio.h>

int main()
{
    LRUCache cache;
    // Initialize a tiny cache of size 3 so we can easily force evictions
    lru_init(&cache, 3);

    printf("--- TEST 1: Basic Put & Get ---\n");
    lru_cache_put(&cache, "A", "Data_A", 0); // 0 TTL = lives forever
    lru_cache_put(&cache, "B", "Data_B", 0);
    lru_cache_put(&cache, "C", "Data_C", 0);

    printf("Get A: %s\n", (char *)lru_cache_get(&cache, "A"));
    printf("Get B: %s\n", (char *)lru_cache_get(&cache, "B"));

    // Current LRU order from MRU to LRU: B -> A -> C

    printf("\n--- TEST 2: LRU Eviction ---\n");
    // Inserting 'D' should exceed capacity (3).
    // 'C' is the Least Recently Used (at the tail), so 'C' must die.
    lru_cache_put(&cache, "D", "Data_D", 0);

    printf("Get C (Evicted): %s\n", (char *)lru_cache_get(&cache, "C")); // Should be (null)
    printf("Get D (New MRU): %s\n", (char *)lru_cache_get(&cache, "D")); // Should print Data_D

    printf("\n--- TEST 3: TTL Expiration ---\n");
    // Put 'E' with a 2-second Time-To-Live
    lru_cache_put(&cache, "E", "Data_E", 2);
    printf("Get E (Immediate): %s\n", (char *)lru_cache_get(&cache, "E")); // Should print Data_E

    printf("Sleeping for 3 seconds...\n");
    sleep(3);

    // The lazy expiration should catch it here
    printf("Get E (After 3s): %s\n", (char *)lru_cache_get(&cache, "E")); // Should be (null)
    lru_destroy(&cache);
    return 0;
}