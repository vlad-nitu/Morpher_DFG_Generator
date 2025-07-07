#include <stdio.h>
#include <stdint.h>

// Define constants for page table entries, shift, and levels
#define ENTRIES 512
#define PAGE_SHIFT 12
#define LEVELS 4 // Number of page table levels (PML4, PDPT, PD, PT)

// Undefine DEBUG macro to prevent printf calls during CGRA compilation,
// as Morpher v2 might not support mapping 'call' assembly instructions.
#undef DEBUG

// Define u64 as an alias for uint64_t for convenience
typedef uint64_t u64;

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
void page_table_walk() {
    // Initialize pa to 0. This value will indicate a translation failure
    // if the walk does not successfully find a physical address.
    pa = 0;

    // Extract all necessary indexes from the virtual address first.
    // This makes the subsequent walk logic cleaner and less error-prone.
    int pml4_idx = get_index(va, 3); // Index for the PML4 table
    int pdpt_idx = get_index(va, 2); // Index for the PDPT table
    int pd_idx   = get_index(va, 1); // Index for the PD table
    int pt_idx   = get_index(va, 0); // Index for the PT table

#ifdef DEBUG
    // Print the virtual address and extracted indexes for debugging.
    printf("VA: 0x%lx -> Indexes: PML4[%d], PDPT[%d], PD[%d], PT[%d]\n",
           va, pml4_idx, pdpt_idx, pd_idx, pt_idx);
#endif

    u64 current_entry;    // Holds the entry read from the current page table.
    u64* next_table_ptr; // Pointer to the next level's page table.

    // --- Level 4: PML4 (Page Map Level 4) ---
    // Access the PML4 table using the extracted PML4 index.
    current_entry = PML4[pml4_idx];
    if (current_entry == 0) {
#ifdef DEBUG
        printf("PML4 miss: Entry is 0x0 at index %d\n", pml4_idx);
#endif
        return; // Translation failed, pa remains 0.
    }
    // The entry contains the base address of the next level table (PDPT).
    next_table_ptr = (u64*)current_entry;

#ifdef CGRA_COMPILER
    // This macro is specific to your CGRA compiler to indicate a mappable region.
    // It's placed here to signify that this stage of the walk should be mapped.
    please_map_me();
#endif

    // --- Level 3: PDPT (Page Directory Pointer Table) ---
    // Access the PDPT table using the extracted PDPT index.
    current_entry = next_table_ptr[pdpt_idx];
    if (current_entry == 0) {
#ifdef DEBUG
        printf("PDPT miss: Entry is 0x0 at index %d\n", pdpt_idx);
#endif
        return;
    }
    // The entry contains the base address of the next level table (PD).
    next_table_ptr = (u64*)current_entry;

#ifdef CGRA_COMPILER
    please_map_me();
#endif

    // --- Level 2: PD (Page Directory) ---
    // Access the PD table using the extracted PD index.
    current_entry = next_table_ptr[pd_idx];
    if (current_entry == 0) {
#ifdef DEBUG
        printf("PD miss: Entry is 0x0 at index %d\n", pd_idx);
#endif
        return;
    }
    // The entry contains the base address of the next level table (PT).
    next_table_ptr = (u64*)current_entry;

#ifdef CGRA_COMPILER
    please_map_me();
#endif

    // --- Level 1: PT (Page Table) ---
    // Access the PT table using the extracted PT index.
    current_entry = next_table_ptr[pt_idx];
    if (current_entry == 0) {
#ifdef DEBUG
        printf("PT miss: Entry is 0x0 at index %d\n", pt_idx);
#endif
        return;
    }

    // --- Final Physical Address Calculation ---
    // The last 12 bits of the virtual address form the page offset.
    u64 page_offset = va & 0xFFF;
    // The 'current_entry' at this point holds the base physical frame address.
    u64 frame_base = current_entry;
    // Calculate the final physical address by adding the page offset to the frame base.
    pa = frame_base + page_offset;
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
