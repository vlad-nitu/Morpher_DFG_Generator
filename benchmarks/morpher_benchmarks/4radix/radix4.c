#include <stdio.h>
#include <stdint.h>

// Define constants for page table entries, shift, and levels
#define ENTRIES 512
#define PAGE_SHIFT 12

// Undefine DEBUG macro to prevent printf calls during CGRA compilation,
// as Morpher v2 might not support mapping 'call' assembly instructions.
#undef DEBUG

#ifdef CGRA_COMPILER
extern void please_map_me();
#endif

typedef uint32_t u32; // Define u32 as an alias for uint64_t for clarity

const int LEVELS = 4; // Number of page table levels (PML4, PDPT, PD, PT)

// Simulated page tables as global arrays.
// These represent the base addresses of the page tables at each level.
u32 PML4[ENTRIES];
u32 PDPT[ENTRIES];
u32 PD[ENTRIES];
u32 PT[ENTRIES];

// Simulated physical memory frames for illustration.
u32 physical_memory[1024];

// Global variables for virtual address (va) and physical address (pa).
// These are used globally as per the original code's intent.
u32 va;
u32 pa;
u32 frame;


#define PAGE_SHIFT 12      /* 4 KiB pages */
#define LVL_BITS    4      /* 4 bits per level */
#define LVL_MASK    0xF    /* 0b1111 */

static inline int get_index(uint32_t va_addr, int level)
/* level: 0 = PT, 1 = PD, 2 = PDPT, 3 = PML4 */
{
    /* shift = 16 + 4*level
       level 0 → 16  (bits 19–16)
       level 1 → 20  (bits 23–20)
       level 2 → 24  (bits 27–24)
       level 3 → 28  (bits 31–28) */
    return (va_addr >> (16 + LVL_BITS * level)) & LVL_MASK;
}

/**
 * @brief Performs a simulated page table walk for the global virtual address (va).
 * Updates the global physical address (pa) upon successful translation,
 * or sets pa to 0 if a page table miss occurs.
 */
/* --------------------------------------------------- */
__attribute__((noinline))
void page_table_walk(void)
{

    for (volatile int level = LEVELS - 1; level >= 0; --level) {
#ifdef CGRA_COMPILER
        please_map_me();
#endif
        if (level == 3) { frame = PML4[get_index(va, level)];  }
        else if (level == 2){ frame = PDPT[get_index(va, level)]; }
        else if (level == 1){ frame = PD[get_index(va, level)]; }
        else              { frame = PT[get_index(va, level)]; }
    }

    // Assume PA given; 'frame' is global variable
    // pa = frame + (va & 0xFFF);
}

/**
 * @brief Main function to set up a simulated page table mapping and perform a walk.
 * @return 0 on successful execution.
 */
int main() {
    // Initialize pa to 0 at the start of main. This ensures a clean state
    // before the page table walk is attempted.
    pa = 0;

    // Set an example virtual address.
    // Using a longer address (64-bit) to ensure all page table levels are exercised.
    va = 0xCAFEBABE;

    // Get the indexes for setting up the simulated page table entries.
    int pml4_idx = get_index(va, 3);
    int pdpt_idx = get_index(va, 2);
    int pd_idx   = get_index(va, 1);
    int pt_idx   = get_index(va, 0);

    // Setup simulated page table entries. -> Dummy data
    PML4[pml4_idx] = (u32)0;
    PDPT[pdpt_idx] = (u32)1;
    PD[pd_idx]     = (u32)2;
    PT[pt_idx]     = (u32)3; // The final entry points to a physical memory frame.

    // Perform the page table walk.
    page_table_walk();

    u32 pa = frame + (va & 0xFFF); // Calculate the physical address based on the frame and offset.
    if (pa == 0) {
        printf("Page table walk failed, physical address is 0.\n");
        return -1; // Indicate failure if pa is still 0.
    }
    else {
        printf("Translated PA: 0x%lx\n", pa); // Calculate the physical address based on the frame and offset.
    }

    return 0;
}
