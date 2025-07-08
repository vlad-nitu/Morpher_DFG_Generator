#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
#define LVLS 4
#define ENTRIES 512

/* ---------- ONE contiguous table: row 0=PML4, 1=PDPT, 2=PD, 3=PT */
static u32 PTBL[LVLS][ENTRIES];

/* index table (filled once in main) */
static int idx[LVLS];

__attribute__((noinline,optnone))          /* keep loop + header intact */
void page_table_walk(void)
{
    /* loop uses a single base @PTBL: no PHI-of-arrays possible */
    for (volatile int lvl = 3; lvl >= 0; --lvl) {
#ifdef CGRA_COMPILER
        please_map_me();                  /* exactly one token */
#endif
        PTBL[lvl][ idx[lvl] ] += 1;       /* pure 32-bit GEP + load/store */
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