#include "sharded_cache.h"

int get_shard_index(ShardedCache *cache, const char *key)
{
    uint32_t hash = djb2_hash(key);
    return hash % cache->num_shards;
}

void sharded_init(ShardedCache *s_cache, int total_capacity, int num_shards)
{
    s_cache->num_shards = num_shards;

    int capacity_per_shard = total_capacity / num_shards;

    s_cache->shards = (LRUCache *)calloc(num_shards, sizeof(LRUCache));
    s_cache->locks = (pthread_mutex_t *)calloc(num_shards, sizeof(pthread_mutex_t));

    for (int i = 0; i < num_shards; i++)
    {
        pthread_mutex_init(s_cache->locks + i, NULL);
        lru_init(s_cache->shards + i, capacity_per_shard);
    }
}

void *sharded_get(ShardedCache *s_cache, const char *key)
{
    int shard_idx = get_shard_index(s_cache, key);
    void *result = NULL;

    pthread_mutex_lock(s_cache->locks + shard_idx);
    result = lru_cache_get(s_cache->shards + shard_idx, key);
    pthread_mutex_unlock(s_cache->locks + shard_idx);
    return result;
}

void sharded_put(ShardedCache *s_cache, const char *key, void *value, int ttl_seconds)
{
    uint32_t shard_idx = get_shard_index(s_cache, key);
    pthread_mutex_lock(s_cache->locks + shard_idx);
    lru_cache_put(s_cache->shards + shard_idx, key, value, ttl_seconds);
    pthread_mutex_unlock(s_cache->locks + shard_idx);
}

void sharded_destroy(ShardedCache *s_cache)
{
    for (int i = 0; i < s_cache->num_shards; i++)
    {
        lru_destroy(s_cache->shards + i);
        pthread_mutex_destroy(s_cache->locks + i);
    }
    free(s_cache->shards);
    free(s_cache->locks);
    s_cache->shards = NULL;
    s_cache->locks = NULL;
}
