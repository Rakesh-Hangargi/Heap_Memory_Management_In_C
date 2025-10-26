#include "mm.h"
#include <stdio.h>
#include <stdint.h>
#include<stdbool.h>

/* Forward declarations for helper debug functions (optional) */
void dump_lmm_state(void);
vm_page_family_t *lookup_page_family_by_name(const char *struct_name);
void xfree(void *ptr);  


int main() {
    printf("=== Custom Heap Manager Test ===\n");

    // 1️⃣ Initialize memory manager
    mm_init();

    // 2️⃣ Register page families
    MM_REG_STRUCT(my_struct, 64);
    MM_REG_STRUCT(another_struct, 128);

    // 3️⃣ Allocate memory blocks
    void *p1 = xcalloc("my_struct", 100);   // Allocate and split
    void *p2 = xcalloc("my_struct", 50);
    void *p3 = xcalloc("another_struct", 200);

    printf("\n=== After initial allocation ===\n");
    dump_lmm_state();

    // 4️⃣ Free using xfree (instead of mm_free_block)
    printf("\nFreeing p1 using xfree...\n");
    xfree(p1);

    printf("\n=== After freeing p1 ===\n");
    dump_lmm_state();

    // 5️⃣ Allocate another block to test reusing freed space
    void *p4 = xcalloc("my_struct", 80);
    printf("\n=== After allocating p4 (80 bytes) ===\n");
    dump_lmm_state();

    // 6️⃣ Free second block
    printf("\nFreeing p2 using xfree...\n");
    xfree(p2);

    printf("\n=== After freeing p2 ===\n");
    dump_lmm_state();

    // 7️⃣ Free p4 to test merging of free blocks
    printf("\nFreeing p4 using xfree (should merge with neighbors)...\n");
    xfree(p4);

    printf("\n=== After freeing p4 (testing merging) ===\n");
    dump_lmm_state();

    // 8️⃣ Free everything related to 'another_struct'
    printf("\nFreeing p3 (another_struct) using xfree...\n");
    xfree(p3);

    printf("\n=== After freeing p3 ===\n");
    dump_lmm_state();

    // 9️⃣ Allocate again to test if pages are reused or newly allocated
    void *p5 = xcalloc("another_struct", 100);
    printf("\n=== After allocating p5 (another_struct, 100 bytes) ===\n");
    dump_lmm_state();

    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}

