#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
#define LVLS 4
#define ENTRIES 512

/* ---------- ONE contiguous table: row 0=PML4, 1=PDPT, 2=PD, 3=PT */
static u32 PTBL[LVLS][ENTRIES];

/* index table (filled once in main) */
static int idx[LVLS];
static u32 x; // Global variable to hold the value loaded from the page table

__attribute__((noinline))
void page_table_walk(void)
{
    /* loop uses a single base @PTBL: no PHI-of-arrays possible */
    for (int cur_lvl = 3; cur_lvl >= 0; cur_lvl --) {
#ifdef CGRA_COMPILER
        please_map_me();                  /* exactly one token */
#endif

        // PTBL[cur_lvl][ idx[cur_lvl] ] += 1;
        x = PTBL[cur_lvl][ idx[cur_lvl] ];       /* pure 32-bit GEP + load/store */
        PTBL[cur_lvl][ idx[cur_lvl] ] = x;       /* pure 32-bit GEP + load/store */
        // PTBL[cur_lvl][ idx[cur_lvl] ] = ((x << 4) + 0xab) & 0xffffffff;  // Perform a dummy store + index calculation operation
    }
}

int main(void)
{
    /* dummy virtual address → dummy indices */
    idx[3] = 0;  idx[2] = 1;  idx[1] = 2;  idx[0] = 3;

    /* dummy data */
    PTBL[3][0] = 0;   PTBL[2][1] = 1;   PTBL[1][2] = 2;   PTBL[0][3] = 3;

    page_table_walk();
    printf("PTBL[0][3] = %u\n", PTBL[0][3]);   /* sanity */
    return 0;
}