#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "sharded_cache.h"

#define NUM_THREADS 8
#define OPS_PER_THREAD 100000

ShardedCache global_cache;

// High-resolution timer helper
double get_time_seconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1000000000.0);
}

void *benchmark_worker(void *arg)
{
    long thread_id = (long)arg;
    char key[32];
    char value[32] = "benchmark_payload";

    for (int i = 0; i < OPS_PER_THREAD; i++)
    {
        // 20% of keys will get 80% of the traffic to simulate a realistic cache hit rate.
        int key_id;
        if (rand() % 100 < 80)
        {
            key_id = rand() % 200; // "Hot" keys
        }
        else
        {
            key_id = rand() % 10000; // "Cold" keys
        }

        sprintf(key, "key_%d", key_id);

        if (rand() % 100 < 10)
        {
            sharded_put(&global_cache, key, value, 0);
        }
        else
        {
            sharded_get(&global_cache, key);
        }
    }
    return NULL;
}

int main()
{
    // Initialize cache with 1024 total capacity across 16 shards
    sharded_init(&global_cache, 1024, 16);
    pthread_t threads[NUM_THREADS];
    srand(time(NULL));

    printf("Starting benchmark with %d threads (%d ops total)...\n",
           NUM_THREADS, NUM_THREADS * OPS_PER_THREAD);

    double start_time = get_time_seconds();

    for (long i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, benchmark_worker, (void *)i);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    double end_time = get_time_seconds();
    double duration = end_time - start_time;

    // Fetch atomic results
    uint64_t total_hits = atomic_load(&global_cache.hits);
    uint64_t total_misses = atomic_load(&global_cache.misses);
    uint64_t total_ops = NUM_THREADS * OPS_PER_THREAD;

    double hit_rate = (double)total_hits / (total_hits + total_misses) * 100.0;
    double ops_per_sec = total_ops / duration;

    printf("\n--- Benchmark Results ---\n");
    printf("Execution Time: %.4f seconds\n", duration);
    printf("Throughput:     %.2f Ops/sec\n", ops_per_sec);
    printf("Cache Hits:     %lu\n", total_hits);
    printf("Cache Misses:   %lu\n", total_misses);
    printf("Hit Rate:       %.2f%%\n", hit_rate);

    sharded_destroy(&global_cache);
    return 0;
}