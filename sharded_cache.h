#include <pthread.h>
#include "cache.h"
#include <stdatomic.h>

typedef struct
{
    LRUCache *shards;
    pthread_mutex_t *locks;
    int num_shards;

    _Atomic uint64_t misses;
    _Atomic uint64_t hits;
} ShardedCache;

int get_shard_index(ShardedCache *cache, const char *key);
void sharded_init(ShardedCache *s_cache, int total_capacity, int num_shards);
void *sharded_get(ShardedCache *s_cache, const char *key);
void sharded_put(ShardedCache *s_cache, const char *key, void *value, int ttl_seconds);
void sharded_destroy(ShardedCache *s_cache);
