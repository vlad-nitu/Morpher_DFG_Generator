/*  elastic_cuckoo_hash.c   –  minimal software kernel of Elastic Cuckoo Hashing
 *
 *  Build & run (GCC or Clang):
 *      gcc -O2 -std=c99 -Wall elastic_cuckoo_hash.c -o ech && ./ech
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BUCKETS            1024          /* number of primary buckets   (must be power-of-2) */
#define SLOTS_PER_BUCKET   4             /* entries per bucket (way)                    */
#define ELASTIC_STASH      32            /* extra “elastic” slots                       */
#define MAX_KICKOUTS       16            /* max displacements before stashing           */
#define EMPTY_KEY          0xffffffffu   /* sentinel for an unused slot                 */

typedef struct { uint32_t key, val; } entry_t;

/* ------------------- global hash table ------------------- */
static entry_t table[BUCKETS][SLOTS_PER_BUCKET];
static entry_t stash[ELASTIC_STASH];

/* ------------------- helpers ------------------- */
static inline uint32_t hash32(uint32_t x)
{
    /* 32-bit mix (Thomas Wang) – fast and good enough */
    x ^= x >> 16;  x *= 0x7feb352d;  x ^= x >> 15;
    x *= 0x846ca68b;  x ^= x >> 16;
    return x;
}

static inline uint32_t h1(uint32_t k) { return hash32(k) & (BUCKETS-1); }
static inline uint32_t h2(uint32_t k) { return hash32(k ^ 0x5bd1e995) & (BUCKETS-1); }

/* ------------------- API ------------------- */
void ech_init(void)
{
    for (size_t b = 0; b < BUCKETS; ++b)
        for (size_t s = 0; s < SLOTS_PER_BUCKET; ++s)
            table[b][s].key = EMPTY_KEY;

    for (size_t i = 0; i < ELASTIC_STASH; ++i)
        stash[i].key = EMPTY_KEY;
}

static bool bucket_insert(uint32_t b, uint32_t key, uint32_t val)
{
    for (size_t s = 0; s < SLOTS_PER_BUCKET; ++s)
        if (table[b][s].key == EMPTY_KEY) {
            table[b][s] = (entry_t){key, val};
            return true;
        }
    return false;        /* bucket full */
}

static bool stash_insert(uint32_t key, uint32_t val)
{
    for (size_t i = 0; i < ELASTIC_STASH; ++i)
        if (stash[i].key == EMPTY_KEY) {
            stash[i] = (entry_t){key, val};
            return true;
        }
    return false;        /* stash full → table genuinely full */
}

/* Return true on success, false if the table + stash are saturated */
bool ech_insert(uint32_t key, uint32_t val)
{
    uint32_t b1 = h1(key), b2 = h2(key);

    if (bucket_insert(b1, key, val) || bucket_insert(b2, key, val))
        return true;

    /* Cuckoo kick-out loop */
    uint32_t cur_key = key, cur_val = val, cur_bucket = (rand() & 1) ? b1 : b2;
    for (int kick = 0; kick < MAX_KICKOUTS; ++kick) {
        size_t victim_slot = rand() % SLOTS_PER_BUCKET;
        entry_t victim = table[cur_bucket][victim_slot];
        table[cur_bucket][victim_slot] = (entry_t){cur_key, cur_val};

        cur_key = victim.key;  cur_val = victim.val;
        if (cur_key == EMPTY_KEY) return true;        /* replaced an empty slot */

        cur_bucket = (cur_bucket == h1(cur_key)) ? h2(cur_key) : h1(cur_key);
        if (bucket_insert(cur_bucket, cur_key, cur_val))
            return true;                              /* placed successfully */
    }
    /* Too many displacements → fall back to elastic stash */
    return stash_insert(cur_key, cur_val);
}

bool ech_lookup(uint32_t key, uint32_t *out_val)
{
    uint32_t b1 = h1(key), b2 = h2(key);
    for (size_t s = 0; s < SLOTS_PER_BUCKET; ++s) {
        if (table[b1][s].key == key) { *out_val = table[b1][s].val; return true; }
        if (table[b2][s].key == key) { *out_val = table[b2][s].val; return true; }
    }
    for (size_t i = 0; i < ELASTIC_STASH; ++i)
        if (stash[i].key == key) { *out_val = stash[i].val; return true; }
    return false;
}

/* --------------- demo / “kernel” main --------------- */
int main(void)
{
    ech_init();

    /* simple demonstration: insert N keys then probe them back */
    const int N = 10'000;
    int inserted = 0, stash_used = 0;

    for (uint32_t k = 1; k <= (uint32_t)N; ++k)
        if (ech_insert(k, k * 10))
            ++inserted;

    /* count stash occupancy */
    for (size_t i = 0; i < ELASTIC_STASH; ++i)
        if (stash[i].key != EMPTY_KEY) ++stash_used;

    printf("Inserted %d/%d keys (stash used: %d of %d)\n",
           inserted, N, stash_used, ELASTIC_STASH);

    /* verify look-ups */
    int missed = 0;
    for (uint32_t k = 1; k <= (uint32_t)N; ++k) {
        uint32_t v;
        if (!ech_lookup(k, &v) || v != k * 10)
            ++missed;
    }
    printf("Lookup check: %d failures\n", missed);
    return 0;
}
