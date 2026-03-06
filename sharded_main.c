#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "sharded_cache.h"

// Shared global cache for threads to hammer
ShardedCache global_cache;

void *worker_thread(void *arg)
{
    long thread_id = (long)arg;
    char key[32];
    char value[32];

    printf("Thread %ld starting...\n", thread_id);

    // Each thread does 10,000 operations
    for (int i = 0; i < 10000; i++)
    {
        // Generate a pseudo-random key to ensure we hit different shards
        sprintf(key, "key_%d", (i * (int)thread_id) % 100);
        sprintf(value, "val_%ld_%d", thread_id, i);

        // Mix of Reads and Writes
        if (i % 3 == 0)
        {
            sharded_put(&global_cache, key, value, 0); // 0 TTL
        }
        else
        {
            sharded_get(&global_cache, key);
        }
    }

    printf("Thread %ld finished.\n", thread_id);
    return NULL;
}

int main()
{
    // 1024 total capacity, divided into 16 shards (64 items per shard)
    sharded_init(&global_cache, 1024, 16);

    pthread_t threads[4];

    // 1. Spawn threads to bombard the cache
    for (long i = 0; i < 4; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, (void *)i);
    }

    // 2. Wait for all threads to finish (Strict Teardown Rule)
    for (int i = 0; i < 4; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("All threads joined successfully. No deadlocks!\n");

    // 3. Clean up
    sharded_destroy(&global_cache);
    printf("Cache destroyed safely.\n");

    return 0;
}