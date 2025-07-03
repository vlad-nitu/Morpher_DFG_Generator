#include <stdio.h>
#include <stdint.h>

#define ENTRIES 512
#define PAGE_SHIFT 12
#define LEVELS 4

typedef uint64_t u64;

// Simulated page tables
u64 PML4[ENTRIES];
u64 PDPT[ENTRIES];
u64 PD[ENTRIES];
u64 PT[ENTRIES];

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
#ifdef CGRA_COMPILER
    // This function is expected to be mapped by the CGRA compiler 
    please_map_me();
#endif

    int pml4_idx = get_index(va, 3);
    int pdpt_idx = get_index(va, 2);
    int pd_idx   = get_index(va, 1);
    int pt_idx   = get_index(va, 0);

    printf("VA: 0x%lx -> Indexes: [%d, %d, %d, %d]\n", va, pml4_idx, pdpt_idx, pd_idx, pt_idx);

    if (PML4[pml4_idx] == 0) {
        printf("PML4 miss\n");
        return;
    }

    if (PDPT[pdpt_idx] == 0) {
        printf("PDPT miss\n");
        return;
    }

    if (PD[pd_idx] == 0) {
        printf("PD miss\n");
        return;
    }

    if (PT[pt_idx] == 0) {
        printf("PT miss\n");
        return;
    }

    // Final physical address = frame base + page offset
    u64 page_offset = va & 0xFFF;
    u64 frame_base = PT[pt_idx];
    pa = frame_base + page_offset;
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
