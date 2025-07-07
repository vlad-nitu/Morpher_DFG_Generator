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

// Define u64 as an alias for uint64_t for convenience
typedef uint64_t u64;

const int LEVELS = 4; // Number of page table levels (PML4, PDPT, PD, PT)

// Simulated page tables as global arrays.
// These represent the base addresses of the page tables at each level.
u64 PML4[ENTRIES];
u64 PDPT[ENTRIES];
u64 PD[ENTRIES];
u64 PT[ENTRIES];

// Simulated physical memory frames for illustration.
u64 physical_memory[1024];

// Global variables for virtual address (va) and physical address (pa).
// These are used globally as per the original code's intent.
u64 va;
u64 pa;
u64 frame = 0;

/**
 * @brief Helper function to extract the index bits for a specific page table level
 * from a given virtual address.
 *
 * @param va_addr The virtual address from which to extract the index.
 * @param level The page table level (0 for PT, 1 for PD, 2 for PDPT, 3 for PML4).
 * @return The 9-bit index for the specified level.
 */
int get_index(u64 va_addr, int level) {
    // Each level uses 9 bits, and PAGE_SHIFT accounts for the page offset.
    // Level 0 (PT): bits 12-20
    // Level 1 (PD): bits 21-29
    // Level 2 (PDPT): bits 30-38
    // Level 3 (PML4): bits 39-47
    return (va_addr >> (PAGE_SHIFT + 9 * level)) & 0x1FF; // 0x1FF is 511 (9 bits all set)
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
    /* ---- “self-copy” keeps every global live ---- */
    PML4[0]=PML4[0]; PDPT[0]=PDPT[0];
    PD[0]  =PD[0];   PT[0]  =PT[0];
    physical_memory[0]=physical_memory[0];
    va=va; pa=pa;
    /* ---------------------------------------------- */

    int idx[LEVELS];
    idx[3]=get_index(va,3);
    idx[2]=get_index(va,2);
    idx[1]=get_index(va,1);
    idx[0]=get_index(va,0);


    /* canonical count-down loop → clean `br` header  */
    for (int level = LEVELS-1; level >= 0; --level) {
#ifdef CGRA_COMPILER
        please_map_me();                 /* anchor for mapper */
#endif
        int i = idx[level];

        switch (level) {                 /* DIRECT accesses */
        case 3: frame = PML4[i]; PML4[i] = frame; break;
        case 2: frame = PDPT[i]; PDPT[i] = frame; break;
        case 1: frame = PD[i];   PD[i]   = frame; break;
        case 0: frame = PT[i];   PT[i]   = frame; break;
        }
    }

    // Assume pa given
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
    va = 0x123456789ABCDEF0;

    // Get the indexes for setting up the simulated page table entries.
    int pml4_idx = get_index(va, 3);
    int pdpt_idx = get_index(va, 2);
    int pd_idx   = get_index(va, 1);
    int pt_idx   = get_index(va, 0);

    // Setup simulated page table entries.
    // In a real system, these would be physical frame numbers, but here
    // we use the addresses of the next level's page table arrays for simulation.
    PML4[pml4_idx] = (u64)&PDPT[0];
    PDPT[pdpt_idx] = (u64)&PD[0];
    PD[pd_idx]     = (u64)&PT[0];
    PT[pt_idx]     = (u64)&physical_memory[0]; // The final entry points to a physical memory frame.

    // Perform the page table walk.
    page_table_walk();


    // printf("Translated PA: 0x%lx\n", frame + (va & 0xFFF)); // Calculate the physical address based on the frame and offset.

    return 0;
}
