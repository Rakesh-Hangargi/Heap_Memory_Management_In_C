#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void print_block(block_meta_data_t *block) {
    printf("    Block [%p] - size: %u, is_free: %s, prev: %p, next: %p\n",
           (void *)block,
           block->block_size,
           block->is_free ? "TRUE" : "FALSE",
           (void *)block->prev_block,
           (void *)NEXT_META_BLOCK(block));
}

void print_vm_page(vm_page_t *page) {
    printf("  VM Page [%p] - Family: %s\n", (void *)page, page->pg_family->struct_name);
    block_meta_data_t *curr = &page->block_meta_data;

    while (curr) {
        print_block(curr);
        curr = NEXT_META_BLOCK(curr);
    }
}

void dump_lmm_state() {
    vm_page_for_families_t *curr_vm_page_for_families = first_vm_page_for_families;
    printf("\n=== LMM STATE DUMP ===\n");

    while (curr_vm_page_for_families) {
        vm_page_family_t *family;

        ITERATE_PAGE_FAMILIES_BEGIN(curr_vm_page_for_families, family) {
            if (!family->struct_size) continue;

            printf("\nPage Family: %s, struct_size: %u, First VM Page: %p\n",
                   family->struct_name,
                   family->struct_size,
                   (void *)family->first_page);

            vm_page_t *page = family->first_page;
            int page_idx = 0;
            while (page) {
                printf("  VM Page %d:\n", page_idx++);
                print_vm_page(page);
                page = page->next;
            }

            if (family->heap_size > 0) {
                printf("  Free Block Heap: ");
                for (uint32_t i = 0; i < family->heap_size; i++) {
                    printf("[%u bytes] ", family->free_block_heap[i]->block_size);
                }
                printf("\n");
            }

        } ITERATE_PAGE_FAMILIES_END(curr_vm_page_for_families, family);

        curr_vm_page_for_families = curr_vm_page_for_families->next;
    }

    printf("\n=== END OF LMM DUMP ===\n");
}

