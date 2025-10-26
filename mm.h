#ifndef __MM__
#define __MM__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdbool.h>

/* boolean type */
typedef enum {
    MM_FALSE,
    MM_TRUE
} vm_bool_t;

#define MM_MAX_STRUCT_NAME 32

/* forward declarations */
typedef struct vm_page_ vm_page_t;
typedef struct block_meta_data_ block_meta_data_t;

/* block metadata */
typedef struct block_meta_data_ {
    vm_bool_t is_free;
    uint32_t block_size; /* size of user-data area */
    uint32_t offset;     /* offset from start of page */
    struct block_meta_data_ *prev_block;
    struct block_meta_data_ *next_block;
} block_meta_data_t;

/* page family */
typedef struct vm_page_family_ {
    char struct_name[MM_MAX_STRUCT_NAME];
    uint32_t struct_size;
    vm_page_t *first_page;
    block_meta_data_t **free_block_heap; // max-heap of free blocks
    uint32_t heap_size;                  // number of blocks
    uint32_t heap_capacity;
} vm_page_family_t;

/* VM page structure */
typedef struct vm_page_ {
    struct vm_page_ *next;
    struct vm_page_ *prev;
    vm_page_family_t *pg_family;
    block_meta_data_t block_meta_data; /* first block metadata */
    char page_memory[0]; /* flexible array */
} vm_page_t;

/* page storing families */
typedef struct vm_page_for_families {
    struct vm_page_for_families *next;
    vm_page_family_t vm_page_family[0];
} vm_page_for_families_t;

vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family);

/* global state */
extern vm_page_for_families_t *first_vm_page_for_families;
extern size_t SYSTEM_PAGE_SIZE;

/* macros */
#define MM_REG_STRUCT(name, size) mm_instantiate_new_page_family(#name, size)
#define MARK_VM_PAGE_EMPTY(vm_page_ptr)      \
    do {                                     \
        (vm_page_ptr)->block_meta_data.next_block = NULL; \
        (vm_page_ptr)->block_meta_data.prev_block = NULL; \
        (vm_page_ptr)->block_meta_data.is_free = MM_TRUE; \
    } while (0)

#define OFFSET_OF(struct_type, field_name) ((size_t)&(((struct_type *)0)->field_name))
#define MM_GET_PAGE_FROM_BLOCK(block_ptr)  \
    ((vm_page_t *)((char *)(block_ptr) - OFFSET_OF(vm_page_t, block_meta_data)))



#define offset_of(container_structure, field_name) \
    ((size_t)&(((container_structure *)0)->field_name))

#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr) \
    ((void *)((char *)block_meta_data_ptr - (block_meta_data_ptr)->offset))

#define NEXT_META_BLOCK(block_meta_data_ptr) \
    ((block_meta_data_ptr)->next_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr) \
    ((block_meta_data_t *)((char *)((block_meta_data_ptr) + 1) + (block_meta_data_ptr)->block_size))

#define PREV_META_BLOCK(block_meta_data_ptr) \
    ((block_meta_data_ptr)->prev_block)

#define MAX_FAMILIES_PER_VM_PAGE \
    ((SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(vm_page_family_t))

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr) \
    do { \
        uint32_t __iter_count = 0; \
        for ((curr) = (vm_page_family_t *)&(vm_page_for_families_ptr)->vm_page_family[0]; \
             (curr)->struct_size && __iter_count < MAX_FAMILIES_PER_VM_PAGE; \
             (curr)++, __iter_count++) {

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) \
        } \
    } while (0)

/* bind a free block after an allocated block */
#define mm_bind_block_for_allocation(allocated_meta_block, free_meta_block) \
    do { \
        (free_meta_block)->prev_block = (allocated_meta_block); \
        (free_meta_block)->next_block = (allocated_meta_block)->next_block; \
        (allocated_meta_block)->next_block = (free_meta_block); \
        if ((free_meta_block)->next_block) { \
            (free_meta_block)->next_block->prev_block = (free_meta_block); \
        } \
    } while (0)

/* function prototypes */
void mm_init(void);
void mm_instantiate_new_page_family(const char *struct_name, uint32_t struct_size);
vm_page_family_t *lookup_page_family_by_name(const char *struct_name);
void *xcalloc(const char *struct_name, uint32_t units);
void mm_free_block(vm_page_family_t *family, block_meta_data_t *block);
void dump_lmm_state(void);
block_meta_data_t *mm_allocate_free_data_block(vm_page_family_t *family, uint32_t req_size);
void mm_split_free_data_blocks_for_allocation(vm_page_family_t *family, block_meta_data_t *block, uint32_t req_size);
void mm_insert_free_block(vm_page_family_t *family, block_meta_data_t *block);
/* heap helper functions */
void mm_heapify_up(vm_page_family_t *family, int index);
void mm_heapify_down(vm_page_family_t *family, int index);
void mm_insert_free_block(vm_page_family_t *family, block_meta_data_t *block);
block_meta_data_t *mm_extract_largest_block(vm_page_family_t *family);
block_meta_data_t *mm_allocate_free_data_block(vm_page_family_t *family, uint32_t req_size);
void mm_split_free_data_blocks_for_allocation(vm_page_family_t *family, block_meta_data_t *block, uint32_t req_size);

bool mm_is_vm_page_empty(vm_page_t *vm_page);
void mm_vm_page_delete_and_free(vm_page_t *vm_page);
int mm_remove_block_from_heap(vm_page_family_t *family, block_meta_data_t *block);

void xfree(void *ptr);

#endif /* __MM__ */
