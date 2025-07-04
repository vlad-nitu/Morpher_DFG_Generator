#include <stdio.h>
#include <stdint.h>

#define ENTRIES 512
#define PAGE_SHIFT 12
#define LEVELS 4

// !!!!!!!!!!!! If we define DEBUG and then use CGRA compiler to map, it will see the printf and will translate it into the 'call' assembly instruction
// which Morpher does not support (i.e, doesn't know how to map it).
// So, when compiling with CGRA_COMPILER, MAKE SURE THIS MACRO IS UNDEFINED: DEBUG.
#undef DEBUG

typedef uint64_t u64;

// Simulated page tables
u64 PML4[ENTRIES] __attribute__((morpher_spm_array(4096)));
u64 PDPT[ENTRIES] __attribute__((morpher_spm_array(4096)));
u64 PD[ENTRIES]   __attribute__((morpher_spm_array(4096)));
u64 PT[ENTRIES]   __attribute__((morpher_spm_array(4096)));

// Simulated physical frames (for illustration)
u64 physical_memory[1024];

// VA global variabel, the LLVM optimiser (opt) expects a function w/o args?
u64 va = -1;
u64 pa = -1;

// Helper to extract index bits from a virtual address
int get_index(u64 va, int level) {
    return (va >> (PAGE_SHIFT + 9 * level)) & 0x1FF;  // 9 bits per level
}

// Perform a simulated page table walk
void page_table_walk() {

    u64* tables[4] = {PT, PD, PDPT, PML4};  // Bottom-up order
    int indexes[4];
    
    indexes[3] = get_index(va, 0);  // PML4 index
    indexes[2] = get_index(va, 1);  // PDPT index
    indexes[1] = get_index(va, 2);  // PD index
    indexes[0] = get_index(va, 3);  // PT index

#ifdef DEBUG
    printf("VA: 0x%lx -> Indexes: [%d, %d, %d, %d]\n",
           va, indexes[3], indexes[2], indexes[1], indexes[0]);
#endif

    // Page table walk using while loop
    u64* current_table = PML4;
    for (int level = 3; level > 0; level--) {
#ifdef CGRA_COMPILER
    please_map_me();
#endif
        int idx = indexes[level];
        if (current_table[idx] == 0) {
#ifdef DEBUG
            printf("Level %d miss\n", level);
#endif
            return;
        }
        current_table = (u64*)current_table[idx];
        level--;
    }

    // PT level
    int pt_idx = indexes[0];
    if (current_table[pt_idx] == 0) {
#ifdef DEBUG
        printf("PT miss\n");
#endif
        return;
    }

    u64 page_offset = va & 0xFFF;
    u64 frame_base = current_table[pt_idx];
    // pa = frame_base + page_offset;
}

int main() {
    // Setup a virtual-to-physical mapping
    va = 0x123456789;  // Example virtual address

    // Setup simulated page table entries
    int pml4_idx = get_index(va, 3);
    int pdpt_idx = get_index(va, 2);
    int pd_idx   = get_index(va, 1);
    int pt_idx   = get_index(va, 0);

    PML4[pml4_idx] = (u64)&PDPT;
    PDPT[pdpt_idx] = (u64)&PD;
    PD[pd_idx] = (u64)&PT;
    PT[pt_idx] = (u64)&physical_memory[0];  // Points to physical frame

    page_table_walk();

    if (pa)
        printf("Translated PA: 0x%lx\n", pa);
    else
        printf("Translation failed.\n");

    return 0;
}
