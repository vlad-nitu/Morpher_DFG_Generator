#include <stdint.h>
#include <stdio.h>

#define PAGE_SHIFT 12
#define ENTRIES    512

typedef uint64_t u64;

/* helper --------------------------------------------------------------- */
static inline int get_index(u64 va, int level)
{
    return (va >> (PAGE_SHIFT + 9 * level)) & 0x1FF;
}

/* ---------------------------------------------------------------------- */
/*  new signature: all tables come in as arguments                        */
void page_table_walk(u64              va,
                    u64 *restrict    PML4,
                    u64 *restrict    PDPT,
                    u64 *restrict    PD,
                    u64 *restrict    PT)
{
    /* compute indexes once */
    int idx3 = get_index(va, 0);   /* PML4  */
    int idx2 = get_index(va, 1);   /* PDPT  */
    int idx1 = get_index(va, 2);   /* PD    */
    int idx0 = get_index(va, 3);   /* PT    */

    /* walk */
    u64 *cur = PML4;
    if ((cur = (u64*)cur[idx3]) == 0) return 0;
    if ((cur = (u64*)cur[idx2]) == 0) return 0;
    if ((cur = (u64*)cur[idx1]) == 0) return 0;
    if (cur[idx0] == 0)               return 0;

    u64 frame_base  = cur[idx0];
    u64 page_offset = va & ((1ULL<<PAGE_SHIFT)-1);
    // pa = frame_base + page_offset;
}

/* ---------------------------------------------------------------------- */
/*  driver: allocate tables as globals but pass their base address in     */
u64 PML4[ENTRIES], PDPT[ENTRIES], PD[ENTRIES], PT[ENTRIES];
u64 physical_memory[1024];

int main(void)
{
    u64 va = 0x123456789;

    /* minimal mapping for demo */
    PML4[get_index(va,3)] = (u64)&PDPT;
    PDPT[get_index(va,2)] = (u64)&PD;
    PD[get_index(va,1)]   = (u64)&PT;
    PT[get_index(va,0)]   = (u64)&physical_memory[0];

    u64 pa = page_table_walk(va, PML4, PDPT, PD, PT);

    if (pa) printf("PA = 0x%lx\n", pa);
    else    puts("translation failed");
}
