#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
#define LVLS 4
#define ENTRIES 512

/* ---------- ONE contiguous table: row 0=PML4, 1=PDPT, 2=PD, 3=PT */
static u32 PTBL[LVLS][ENTRIES];

/* index table (filled once in main) */
static int idx[LVLS];

/* -------------------------------------------------- */
/* loop kept exactly as written (no unroll, no peel)  */
#if defined(__clang__)
#  define NO_OPT  __attribute__((optnone))
#else                    /* GCC ≥4.4 */
#  define NO_OPT  __attribute__((optimize("O0")))
#endif
/* -------------------------------------------------- */

__attribute__((noinline)) NO_OPT
void page_table_walk(void)
{
    /* loop uses a single base @PTBL: no PHI-of-arrays possible */
    for (int cur_lvl = 3; cur_lvl >= 0; cur_lvl --) {
#ifdef CGRA_COMPILER
        please_map_me();                  /* exactly one token */
#endif
        PTBL[cur_lvl][ idx[cur_lvl] ] += 1;       /* pure 32-bit GEP + load/store */
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