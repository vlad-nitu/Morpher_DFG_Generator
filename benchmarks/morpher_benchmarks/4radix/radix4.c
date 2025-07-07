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

u64 pte2, pte;

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
__attribute__((noinline))
void page_table_walk() {
    // Initialize pa to 0. This value will indicate a translation failure
    // if the walk does not successfully find a physical address.
    pa = 0;

    // Extract all necessary indexes from the virtual address first.
    int indexes[LEVELS]; // Array to store indexes for each level
    indexes[3] = get_index(va, 3); // PML4 index
    indexes[2] = get_index(va, 2); // PDPT index
    indexes[1] = get_index(va, 1); // PD index
    indexes[0] = get_index(va, 0); // PT index

#ifdef DEBUG
    // Print the virtual address and extracted indexes for debugging.
    printf("VA: 0x%lx -> Indexes: PML4[%d], PDPT[%d], PD[%d], PT[%d]\n",
           va, indexes[3], indexes[2], indexes[1], indexes[0]);
#endif

    // Force global references so LLVM IR exposes them to the DFG pass
    PML4[1] = PML4[1];
    PDPT[1] = PDPT[1];
    PD[1] = PD[1];
    PT[1] = PT[1];
    physical_memory[1] = physical_memory[1];
    va = va;
    pa = pa;
        


    // --- Start the page table walk loop from PML4 (level 3) down to PT (level 0) ---
    for (int i = 0; i < LEVELS - 1; ++i) {
      int level = LEVELS - i - 1;

      #ifdef CGRA_COMPILER
      please_map_me();
      #endif


        int idx = indexes[level]; // Get the index for the current level

        // Use a switch statement to explicitly access the correct global array
        // based on the current 'level'. This ensures the compiler always knows
        // the static type and size of the array being accessed.
        switch (level) {
            case 3: // PML4 level
                pte = PML4[idx];
                break;
            case 2: // PDPT level
                // The address 'next_table_base_addr' from the previous level (PML4)
                // is now interpreted as the base of the PDPT table.
                // We cast it to u64* *just for this access* to dereference it.
                // The result is then stored back into next_table_base_addr (u64).
                pte = PDPT[idx];
                break;
            case 1: // PD level
                // Similar to PDPT, interpret the address from PDPT as the base of PD.
                pte = PD[idx];
                break;
            case 0: // PT level (final lookup for physical frame)
                // Interpret the address from PD as the base of PT.
                pte = PT[idx];
                break;
            default:
#ifdef DEBUG
            printf("Should not happen with valid 'LEVELS' and loop bounds\n");
#endif
                break;
        }

        pte2 = pte + 1; // Force access to PTE to ensure it is not optimized away.
    }

    // After the loop, 'next_table_base_addr' holds the physical frame base address.
    // Calculate the final physical address.
    u64 page_offset = va & 0xFFF; // Last 12 bits are page offset
    // pa = next_table_base_addr + page_offset;
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

    // Check if the translation was successful (pa is not 0).
    if (pa != 0) {
        printf("Translated PA: 0x%lx\n", pa);
    } else {
        printf("Translation failed.\n");
    }

    return 0;
}
