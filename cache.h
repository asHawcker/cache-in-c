#include <stdlib.h>

typedef struct Node
{
    char *key;
    void *value;
    size_t value_size;

    // LRU Pointers
    struct Node *prev;
    struct Node *next;

    // Metadata
    long long expiry_at;
} Node;

typedef struct
{
    int capacity;
    int current_size;

    Node **hash_table;
    int table_size;

    Node *head;
    Node *tail;

} LRUCache;
