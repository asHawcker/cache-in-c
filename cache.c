#include <stdint.h>
#include "cache.h"
#include <time.h>

uint32_t djb2_hash(const char *str)
{
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

Node *create_node(const char *key, void *value)
{
    Node *node = malloc(sizeof(Node));

    node->key = malloc(strlen(key) + 1);
    strcpy(node->key, key);

    node->value = value;
    node->prev = NULL;
    node->next = NULL;
    node->expiry_time = 0;

    return node;
}

Node *hash_get(HashTable *table, const char *key)
{
    uint32_t hash = djb2_hash(key);
    int index = hash % table->capacity;

    while (table->buckets[index] != EMPTY)
    {
        if (table->buckets[index] != TOMBSTONE)
        {
            if (strcmp(table->buckets[index]->key, key) == 0)
            {
                return table->buckets[index];
            }
        }
        index = (index + 1) % table->capacity;
    }
    return NULL;
}

void hash_table_insert(HashTable *table, Node *node)
{
    uint32_t hash = djb2_hash(node->key);
    int index = hash % table->capacity;

    while (table->buckets[index] != EMPTY && table->buckets[index] != TOMBSTONE)
    {
        if (strcmp(table->buckets[index]->key, node->key) == 0)
        {
            break;
        }
        index = (index + 1) % table->capacity;
    }

    table->buckets[index] = node;
    table->count++;
}

void hash_table_remove(HashTable *table, const char *key)
{
    uint32_t hash = djb2_hash(key);
    int index = hash % table->capacity;

    while (table->buckets[index] != NULL)
    {
        if (table->buckets[index] != TOMBSTONE)
        {
            if (strcmp(table->buckets[index]->key, key) == 0)
            {
                table->buckets[index] = TOMBSTONE;
                table->count--;
                return;
            }
        }
        index = (index + 1) % table->capacity;
    }
}

void detach_node(Node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void push_front(LRUCache *cache, Node *node)
{
    node->next = cache->head->next;
    node->prev = cache->head;
    cache->head->next->prev = node;
    cache->head->next = node;
}

uint64_t current_time_sec()
{
    return (uint64_t)time(NULL);
}

void lru_cache_delete_node(LRUCache *cache, Node *node)
{
    hash_table_remove(cache->hash_table, node->key);
    detach_node(node);

    free(node->key);
    free(node);

    cache->current_size--;
}

void lru_init(LRUCache *cache, int lru_capacity)
{
    cache->head = calloc(1, sizeof(Node));
    cache->tail = calloc(1, sizeof(Node));

    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;

    cache->capacity = lru_capacity;
    cache->current_size = 0;

    cache->hash_table = malloc(sizeof(HashTable));
    cache->hash_table->count = 0;

    cache->hash_table->capacity = lru_capacity * 2;

    cache->hash_table->buckets = calloc(cache->hash_table->capacity, sizeof(Node *));
}
void *lru_cache_get(LRUCache *cache, const char *key)
{
    Node *node = hash_get(cache->hash_table, key);

    if (node == NULL || node == TOMBSTONE)
    {
        return NULL;
    }

    if (node->expiry_time > 0 && current_time_sec() >= node->expiry_time)
    {
        lru_cache_delete_node(cache, node);
        return NULL;
    }

    // LRU
    detach_node(node);
    push_front(cache, node);

    return node->value;
}

void lru_cache_put(LRUCache *cache, const char *key, void *value, uint64_t ttl_seconds)
{
    uint64_t death_time = (ttl_seconds == 0) ? 0 : current_time_sec() + ttl_seconds;
    Node *existing_node = hash_get(cache->hash_table, key);

    if (existing_node != NULL && existing_node != TOMBSTONE)
    {

        existing_node->value = value;
        existing_node->expiry_time = death_time;
        detach_node(existing_node);
        push_front(cache, existing_node);
        return;
    }

    if (cache->current_size >= cache->capacity)
    {

        Node *victim = cache->tail->prev;

        lru_cache_delete_node(cache, victim);
    }

    Node *new_node = create_node(key, value);
    new_node->expiry_time = death_time;
    push_front(cache, new_node);
    hash_table_insert(cache->hash_table, new_node);
    cache->current_size++;
}