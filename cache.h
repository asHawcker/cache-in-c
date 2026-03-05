#include <stdint.h>
#include <stdlib.h>

#define EMPTY NULL
#define TOMBSTONE ((Node *)-1)

typedef struct Node
{
    char *key;
    void *value;
    size_t value_size;

    // LRU Pointers
    struct Node *prev;
    struct Node *next;

    // Metadata
    uint64_t expiry_time;
} Node;

typedef struct
{
    Node **buckets;
    int capacity;
    int count;
} HashTable;

typedef struct
{
    int capacity;
    int current_size;

    HashTable *hash_table;

    Node *head;
    Node *tail;
} LRUCache;

Node *hash_get(HashTable *table, const char *key);
void hash_table_remove(HashTable *table, const char *key);

void detach_node(Node *node);
void push_front(LRUCache *cache, Node *node);

void lru_init(LRUCache *cache, int capacity);
void *lru_cache_get(LRUCache *cache, const char *key);
void lru_cache_put(LRUCache *cache, const char *key, void *value, uint64_t ttl_seconds);
