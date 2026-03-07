# High-Performance Sharded LRU Cache implemented in C

A thread-safe, high-throughput, in-memory cache built from scratch in C.
This project was developed focusing on lock contention mitigation, memory locality, and the architectural separation of mechanism and policy.

## Architecture

This cache implements a strict **Least Recently Used (LRU)** eviction policy combined with **Lazy Time-To-Live (TTL)** expiration. To achieve high throughput in multi-threaded environments, it completely avoids global locks in favor of **Lock Striping (Sharding)**.

### Mechanism vs. Policy

The internal design separates the storage mechanism from the eviction policy to maintain _O(1)_ operations:

- The Mechanism (Speed): An Open-Addressed Hash Table using linear probing and the djb2 hash function.
- The Policy (Order): A Doubly Linked List.
- Hash table buckets store direct pointers to the Linked List nodes. On a cache hit, the node is detached and pushed to the LRU head in _O(1)_ time.

## Core Features & Systems Tradeoffs

1. Concurrency via Lock Striping (Sharding)
   A single pthread*mutex_t creates a massive bottleneck on multi-core systems. This cache divides the storage into \_N* independent shards, each protected by its own mutex.

   Tradeoff: Slightly higher memory usage for multiple cache controllers, but provides near-linear scaling for concurrent reads/writes as long as keys are uniformly hashed across shards.

2. Open Addressing with Tombstones
   Instead of chaining (which ruins spatial locality), the hash table utilizes contiguous memory arrays.

   Tradeoff: Deletions require "TOMBSTONE" markers to prevent breaking the linear probing chain. This maintains CPU cache-line efficiency at the cost of requiring more complex eviction logic.

3. Pointer-Based Data Ownership
   The cache stores `void*` pointers rather than copying data into its own memory space via memcpy.

   Tradeoff: Very fast insertions, but it transfers the lifetime management burden to the caller. The cache cleans up its internal metadata nodes, but the user is responsible for freeing the actual payload.

4. Lazy TTL Expiration
   Instead of a background thread periodically locking the cache to scrub dead items, expiration is "lazy".

   Tradeoff: Zero background CPU overhead. Items are only checked against the system clock upon a get() request. If an item expires, it is evicted immediately.

5. Lock-Free Telemetry
   Tracking cache hits and misses with a mutex would destroy the benefits of sharding. This project uses C11 Atomics (`stdatomic.h`) with `memory_order_relaxed` to track hit rates without blocking thread execution.

## Project Structure

- `cache.c / cache.h`: The core, single-threaded LRU logic, Open-Addressed Hash Table, and pointer stitching.

- `sharded_cache.c / sharded_cache.h`: The concurrency wrapper that turns the basic LRU Cache into a Sharded LRU cache.

- `main.c`: Single-threaded correctness tests (Basic put/get, Eviction, TTL).

- `sharded_main.c`: Multi-threaded stress test ensuring thread safety and zero deadlocks.

- `sharded_main_atomic.c`: High-performance benchmarking code to measure Ops/sec and Cache Hit Rate.

## Building and Running

This project relies on standard POSIX libraries (`pthread`, `unistd.h`) and **C11 atomics**.

### Compile the basic tests:

```bash
gcc -Wall -Wextra -O2 main.c cache.c -o cache_test
./cache_test
```

### Compile and run the multithreaded benchmark:

```bash
gcc -Wall -Wextra -O2 sharded_main_atomic.c sharded_cache.c cache.c -o benchmark -pthread
./benchmark
```

### Example Benchmark Output

Running 8 concurrent threads executing 100,000 operations each (80/20 skewed read-heavy workload):

```plaintext
Starting benchmark with 8 threads (800000 ops total)...
--- Benchmark Results ---
Execution Time: 0.1245 seconds
Throughput:     6425702.81 Ops/sec
Cache Hits:     621043
Cache Misses:   98957
Hit Rate:       86.25%
```
