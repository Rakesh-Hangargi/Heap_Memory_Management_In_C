#include "mm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static inline int parent(int i) { return (i - 1) / 2; }
static inline int left(int i) { return 2 * i + 1; }
static inline int right(int i) { return 2 * i + 2; }

/*void mm_heapify_up(vm_page_family_t *family, int index) {
    while (index > 0 && family->free_block_heap[parent(index)]->block_size < family->free_block_heap[index]->block_size) {
        block_meta_data_t *tmp = family->free_block_heap[parent(index)];
        family->free_block_heap[parent(index)] = family->free_block_heap[index];
        family->free_block_heap[index] = tmp;
        index = parent(index);
    }
}

void mm_heapify_down(vm_page_family_t *family, int index) {
    int largest = index;
    int l = left(index), r = right(index);
    if (l < family->heap_size && family->free_block_heap[l]->block_size > family->free_block_heap[largest]->block_size)
        largest = l;
    if (r < family->heap_size && family->free_block_heap[r]->block_size > family->free_block_heap[largest]->block_size)
        largest = r;
    if (largest != index) {
        block_meta_data_t *tmp = family->free_block_heap[index];
        family->free_block_heap[index] = family->free_block_heap[largest];
        family->free_block_heap[largest] = tmp;
        mm_heapify_down(family, largest);
    }
}

void mm_insert_free_block(vm_page_family_t *family, block_meta_data_t *block) {
    if (family->heap_size == family->heap_capacity) {
        family->heap_capacity = family->heap_capacity ? family->heap_capacity * 2 : 8;
        family->free_block_heap = realloc(family->free_block_heap, family->heap_capacity * sizeof(block_meta_data_t *));
    }
    family->free_block_heap[family->heap_size] = block;
    family->heap_size++;
    mm_heapify_up(family, family->heap_size - 1);
}*/

block_meta_data_t *mm_extract_largest_block(vm_page_family_t *family) {
    if (!family->heap_size) return NULL;
    block_meta_data_t *max = family->free_block_heap[0];
    family->free_block_heap[0] = family->free_block_heap[family->heap_size - 1];
    family->heap_size--;
    mm_heapify_down(family, 0);
    return max;
}

void mm_split_free_data_blocks_for_allocation(vm_page_family_t *family, block_meta_data_t *block, uint32_t req_size) {
    if (block->block_size > req_size + sizeof(block_meta_data_t)) {
        block_meta_data_t *new_free = (block_meta_data_t *)((char *)(block + 1) + req_size);
        new_free->block_size = block->block_size - req_size - sizeof(block_meta_data_t);
        new_free->is_free = MM_TRUE;
        new_free->prev_block = block;
        new_free->next_block = block->next_block;
        if (block->next_block) block->next_block->prev_block = new_free;
        block->next_block = new_free;
        block->block_size = req_size;
        mm_insert_free_block(family, new_free);
    }
}

block_meta_data_t *mm_allocate_free_data_block(vm_page_family_t *family, uint32_t req_size) {
    // If no page exists yet, allocate one
    if (!family->first_page) {
        allocate_vm_page(family);
        // Insert the first block into free block heap
        mm_insert_free_block(family, &family->first_page->block_meta_data);
    }

    // Step 1: Extract the largest free block from heap
    block_meta_data_t *largest = mm_extract_largest_block(family);
    if (!largest || largest->block_size < req_size)
        return NULL; // Not enough memory

    // Step 2: Split block if needed
    mm_split_free_data_blocks_for_allocation(family, largest, req_size);

    // Step 3: Mark as allocated
    largest->is_free = MM_FALSE;

    return largest;
}


void *xcalloc(const char *struct_name, uint32_t units) {
    vm_page_family_t *family = lookup_page_family_by_name(struct_name);
    if (!family) {
        printf("ERROR: Page family '%s' is not registered\n", struct_name);
        return NULL;
    }

    block_meta_data_t *block = mm_allocate_free_data_block(family, units);
    if (!block) {
        printf("ERROR: Not enough memory in page family '%s'\n", struct_name);
        return NULL;
    }

    void *user_ptr = (void *)(block + 1);
    memset(user_ptr, 0, units);
    return user_ptr;
}
/* xfree — safe free wrapper used by user code */
void xfree(void *ptr) {
    if (!ptr) return;

    /* compute block meta pointer */
    block_meta_data_t *block = (block_meta_data_t *)ptr - 1;

    /* find vm_page from meta using macro in mm.h */
    vm_page_t *vm_page = (vm_page_t *)MM_GET_PAGE_FROM_META_BLOCK(block);
    if (!vm_page) {
        fprintf(stderr, "xfree: invalid pointer (no vm_page) %p\n", ptr);
        return;
    }

    vm_page_family_t *family = vm_page->pg_family;
    if (!family) {
        fprintf(stderr, "xfree: pointer does not belong to a family %p\n", ptr);
        return;
    }

    /* Safety: ensure block appears to belong to this page (offset check) */
    if ((uint32_t)((char *)block - (char *)vm_page) != block->offset) {
        // suspicious: metadata offset mismatches
        fprintf(stderr, "xfree: metadata offset mismatch for %p (expected %u got %zu)\n",
                ptr, block->offset, (size_t)((char*)block - (char*)vm_page));
        // still attempt free but safer to return
        return;
    }

    /* Use mm_free_block to handle merging/heap reinsertion */
    mm_free_block(family, block);

    /* If page became empty, free it back to kernel */
    if (mm_is_vm_page_empty(vm_page)) {
        /* before freeing page, remove its initial block from heap if present */
        mm_remove_block_from_heap(family, &vm_page->block_meta_data);
        mm_vm_page_delete_and_free(vm_page);
    }
}

