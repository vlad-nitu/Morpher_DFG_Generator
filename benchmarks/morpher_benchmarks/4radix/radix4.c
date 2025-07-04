#include <stdio.h>
#include <stdint.h>

#define ENTRIES     512
#define PAGE_SHIFT  12           /* 4 KB pages → 12-bit offset */

typedef uint64_t u64;

/* ------------------------------------------------------------------ */
/* simulated page tables and memory                                   */
u64 PML4[ENTRIES], PDPT[ENTRIES], PD[ENTRIES], PT[ENTRIES];
u64 physical_memory[1024];

/* virtual address for the demo */
u64 va = 0x123456789;

/* ------------------------------------------------------------------ */
static inline int get_index(u64 va, int level)
/* level = 0 → PML4, …, 3 → PT                                    */
{
    return (va >> (PAGE_SHIFT + 9 * level)) & 0x1FF;
}

/* ------------------------------------------------------------------ */
u64 page_table_walk(u64 va)
/* returns 0 on miss, PA on hit                                     */
{
    int idx[4];
    idx[3] = get_index(va, 0);   /* PML4 */
    idx[2] = get_index(va, 1);   /* PDPT */
    idx[1] = get_index(va, 2);   /* PD   */
    idx[0] = get_index(va, 3);   /* PT   */

#ifdef DEBUG
    printf("VA 0x%lx → idx [%d %d %d %d]\n",
           va, idx[3], idx[2], idx[1], idx[0]);
#endif

    u64 *cur = &PML4[0];         /* named base pointer */

    for (int level = 3; level > 0; --level) {
#ifdef CGRA_COMPILER
        please_map_me();
#endif
        int i = idx[level];
        if (cur[i] == 0) {       /* entry not present → miss */
#ifdef DEBUG
            printf("miss at level %d\n", level);
#endif
            return 0;
        }
        cur = (u64*)cur[i];      /* descend to next level   */
    }

    /* PT level */
    if (cur[idx[0]] == 0) {
#ifdef DEBUG
        puts("PT miss");
#endif
        return 0;
    }

    const u64 frame_base  = cur[idx[0]];
    const u64 page_offset = va & ((1ULL << PAGE_SHIFT) - 1);
    // pa = frame_base + page_offset;
}

/* ------------------------------------------------------------------ */
int main(void)
{
    /* build a simple mapping for the demo */
    PML4[get_index(va, 3)] = (u64)&PDPT;
    PDPT[get_index(va, 2)] = (u64)&PD;
    PD[get_index(va, 1)]   = (u64)&PT;
    PT[get_index(va, 0)]   = (u64)&physical_memory[0];

    u64 pa = page_table_walk(va);

    if (pa)
        printf("PA 0x%lx\n", pa);
    else
        puts("translation failed");

    return 0;
}
