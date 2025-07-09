#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
#define ENTRIES 1024

static u32 hash_table[ENTRIES];
static u32 key;
static u32 va;

u32 hash_function(u32 key)
{
    return ((key << 4) ^ (key >> 4)) % ENTRIES; // 1 SHL, 1 SHR, 1 XOR and 1 MOD
}

__attribute__((noinline))
void page_table_walk(void)
{
    key = -1;

    // Assume 3 colissions, and 1 hit => 4 iterations
    for (int cur_lvl = 3; cur_lvl >= 0; cur_lvl --) {
#ifdef CGRA_COMPILER
        please_map_me();                  /* exactly one token */
#endif
        if (cur_lvl == 3) {
            key = hash_function(va);
        }

        if (cur_lvl > 0) { // PTW miss if: lvl in {1, 2, 3} (to generate 3 misses synthetically)
            int pte = hash_table[key];
            if (pte != -1) { // if we hit in PTW
                break;
            }

        }
        else { // PTW hit
            // Perform a dummy store + index calculation operation
            hash_table[key] = ((hash_table[key] << 4) + 0xab) & 0xffffffff;
        }
    }
}

int main(void)
{
    /* initialise example VA */
    va = 0xCAFEBABE; // Example virtual address

    /* initiliase hash_table so that all entries are != -1, so that we never hit in PTW Hash Table before 4th try*/
    memset(hash_table, 0xffff, sizeof(hash_table));

    page_table_walk();

    printf(" PTW Hash-Table based (chaining) succesfull ...\n");   /* sanity */
    return 0;
}