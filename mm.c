#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include<stdbool.h>
/* Globals */
vm_page_for_families_t *first_vm_page_for_families = NULL;
size_t SYSTEM_PAGE_SIZE = 0;

/* Initialize memory manager */
void mm_init(void) {
    SYSTEM_PAGE_SIZE = (size_t)getpagesize();
}

/* Allocate a new VM page from kernel */
static void *mm_get_new_vm_page_from_kernel(int units) {
    size_t bytes = units * SYSTEM_PAGE_SIZE;
    void *vm_page = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                         MAP_ANON | MAP_PRIVATE, -1, 0);
    if (vm_page == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }
    memset(vm_page, 0, bytes);
    return vm_page;
}

/* Return VM page to kernel */
static void mm_return_vm_page_to_kernel(void *vm_page, int units) {
    size_t bytes = units * SYSTEM_PAGE_SIZE;
    if (munmap(vm_page, bytes) != 0) {
        perror("munmap failed");
    }
}

/* Lookup page family by name */
vm_page_family_t *lookup_page_family_by_name(const char *struct_name) {
    vm_page_for_families_t *page = first_vm_page_for_families;
    while (page) {
        vm_page_family_t *curr = &page->vm_page_family[0];
        for (uint32_t i = 0; i < ((SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(vm_page_family_t)); i++, curr++) {
            if (curr->struct_size == 0) continue;
            if (strncmp(curr->struct_name, struct_name, MM_MAX_STRUCT_NAME) == 0)
                return curr;
        }
        page = page->next;
    }
    return NULL;
}

/* Instantiate a new page family */
void mm_instantiate_new_page_family(const char *struct_name, uint32_t struct_size) {
    if (struct_size > SYSTEM_PAGE_SIZE) {
        fprintf(stderr, "SIZE Exceeded\n");
        return;
    }

    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *page_ptr = first_vm_page_for_families;

    while (page_ptr) {
        vm_page_family_t *iter = &page_ptr->vm_page_family[0];
        for (uint32_t i = 0; i < ((SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(vm_page_family_t)); i++, iter++) {
            if (strncmp(iter->struct_name, struct_name, MM_MAX_STRUCT_NAME) == 0) {
                fprintf(stderr, "Page family already exists\n");
                return;
            }
            if (iter->struct_size == 0 && !vm_page_family_curr)
                vm_page_family_curr = iter;
        }
        page_ptr = page_ptr->next;
    }

    if (!vm_page_family_curr) {
        vm_page_for_families_t *new_page = (vm_page_for_families_t *)mm_get_new_vm_page_from_kernel(1);
        if (!new_page) return;
        new_page->next = first_vm_page_for_families;
        first_vm_page_for_families = new_page;
        vm_page_family_curr = &new_page->vm_page_family[0];
    }

    strncpy(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME - 1);
    vm_page_family_curr->struct_name[MM_MAX_STRUCT_NAME - 1] = '\0';
    vm_page_family_curr->struct_size = struct_size;
    vm_page_family_curr->first_page = NULL;
}

/* Allocate a VM page for a family */
vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family) {
    vm_page_t *vm_page = (vm_page_t *)mm_get_new_vm_page_from_kernel(1);
    if (!vm_page) return NULL;

    MARK_VM_PAGE_EMPTY(vm_page);
    vm_page->block_meta_data.block_size = SYSTEM_PAGE_SIZE - sizeof(vm_page_t);
    vm_page->block_meta_data.offset = (uint32_t)offset_of(vm_page_t, block_meta_data);

    vm_page->next = NULL;
    vm_page->prev = NULL;
    vm_page->pg_family = vm_page_family;

    if (!vm_page_family->first_page) {
        vm_page_family->first_page = vm_page;
        return vm_page;
    }

    vm_page->next = vm_page_family->first_page;
    vm_page_family->first_page->prev = vm_page;
    vm_page_family->first_page = vm_page;
    return vm_page;
}

/* Free block union */
void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second) {
    assert(first->is_free && second->is_free);
    first->block_size += sizeof(block_meta_data_t) + second->block_size;
    first->next_block = second->next_block;
    if (second->next_block)
        second->next_block->prev_block = first;
}

/* Free a block */
/*void mm_free_block(vm_page_family_t *family, block_meta_data_t *block) {
    if (!block || !family) return;
    block->is_free = MM_TRUE;
    mm_insert_free_block(family, block);

    if (block->next_block && block->next_block->is_free)
        mm_union_free_blocks(block, block->next_block);
    if (block->prev_block && block->prev_block->is_free)
        mm_union_free_blocks(block->prev_block, block);
}*/



/* Safe mm_free_block: removes stale heap entries, merges neighbors, reinserts merged block */
void mm_free_block(vm_page_family_t *family, block_meta_data_t *block) {
    if (!family || !block) return;

    /* Prevent double-free */
    if (block->is_free == MM_TRUE) {
        // already free — ignore or warn
        // fprintf(stderr, "mm_free_block: double free detected %p\n", (void*)block);
        return;
    }

    /* Mark as free */
    block->is_free = MM_TRUE;

    /* Remove block from heap if somehow present (should normally not be) */
    mm_remove_block_from_heap(family, block);

    /* Try merge with next: if next exists and free, remove next from heap and union */
    if (block->next_block && block->next_block->is_free) {
        mm_remove_block_from_heap(family, block->next_block);
        mm_union_free_blocks(block, block->next_block);
    }

    /* Try merge with prev: if prev exists and free, remove prev from heap and union into prev */
    if (block->prev_block && block->prev_block->is_free) {
        mm_remove_block_from_heap(family, block->prev_block);
        mm_union_free_blocks(block->prev_block, block);
        block = block->prev_block; /* merged block is prev_block now */
    }

    /* Now insert the (possibly merged) free block back into heap */
    mm_insert_free_block(family, block);
}






/* Heap functions */
static inline int parent(int i) { return (i - 1) / 2; }
static inline int left(int i) { return 2 * i + 1; }
static inline int right(int i) { return 2 * i + 2; }

void mm_heapify_up(vm_page_family_t *family, int index) {
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

    if (l < (int)family->heap_size &&
        family->free_block_heap[l]->block_size > family->free_block_heap[largest]->block_size)
        largest = l;

    if (r < (int)family->heap_size &&
        family->free_block_heap[r]->block_size > family->free_block_heap[largest]->block_size)
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
        family->free_block_heap = realloc(family->free_block_heap,
                                          family->heap_capacity * sizeof(block_meta_data_t *));
    }
    family->free_block_heap[family->heap_size] = block;
    family->heap_size++;
    mm_heapify_up(family, family->heap_size - 1);
}
/* Check if all blocks in the VM page are free */
bool mm_is_vm_page_empty(vm_page_t *vm_page) {
    block_meta_data_t *block = &vm_page->block_meta_data;

    while (block) {
        if (!block->is_free)
            return false;
        block = block->next_block;
    }
    return true;
}
void mm_vm_page_delete_and_free(vm_page_t *vm_page) {
    vm_page_family_t *vm_page_family = vm_page->pg_family;

    // Unlink from family's list
    if (vm_page->prev)
        vm_page->prev->next = vm_page->next;
    if (vm_page->next)
        vm_page->next->prev = vm_page->prev;

    if (vm_page_family->first_page == vm_page)
        vm_page_family->first_page = vm_page->next;

    // Finally free the page to kernel
    munmap(vm_page, SYSTEM_PAGE_SIZE);
}
int mm_remove_block_from_heap(vm_page_family_t *family, block_meta_data_t *block) {
    if (!family || !family->free_block_heap || family->heap_size == 0) return 0;

    uint32_t idx = UINT32_MAX;
    for (uint32_t i = 0; i < family->heap_size; ++i) {
        if (family->free_block_heap[i] == block) { idx = i; break; }
    }
    if (idx == UINT32_MAX) return 0;

    /* Replace with last element and shrink heap, then restore heap property */
    family->free_block_heap[idx] = family->free_block_heap[family->heap_size - 1];
    family->heap_size--;
    if (idx < family->heap_size) {
        /* Try heapify down then up to restore order */
        mm_heapify_down(family, (int)idx);
        mm_heapify_up(family, (int)idx);
    }
    return 1;
}
