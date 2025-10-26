# Linux Heap Memory Management

## Overview
This project explores the **internal workings of dynamic memory allocation in Linux** by implementing custom versions of `xcalloc` and `xfree`.  
It demonstrates how memory pages are requested from the kernel, how memory blocks are linked and tracked via pointers, and how freed memory is recycled efficiently.

This hands-on project goes beyond standard C library functions to provide a **deep understanding of heap management at the system level**.

---

## Features
- Custom `xcalloc` function for dynamic memory allocation
- Custom `xfree` function for memory deallocation
- Internal memory tracking using pointers
- Priority-based page allocation using Max Heap
- Visualization of memory blocks and page connections
- Sample outputs to demonstrate memory allocation and freeing behavior

---

## Data Structures Used
| Operation | Data Structure | Purpose |
|-----------|----------------|---------|
| Tracking memory pages | Linked List | Each page allocated from the kernel is represented as a node, allowing easy traversal and management. |
| Tracking allocated/free blocks within a page | Doubly Linked List | Blocks inside each page are linked for quick insertion, removal, and coalescing of free memory. |
| Priority-based page allocation | Max Heap | Maintains pages based on availability or usage priority, enabling efficient allocation of the most suitable page. |
| Fast access to memory blocks | Pointers | Pointers connect memory blocks and pages, enabling allocation (`xcalloc`) and deallocation (`xfree`). |

These structures allow efficient allocation, freeing, and memory recycling while keeping track of memory usage and prioritizing page selection.

---

## Learning Outcomes
- Understanding low-level memory operations in C
- How heap memory is managed internally
- Memory page allocation and recycling
- Pointer-based linking of memory blocks
- Use of linked lists and Max Heap for efficient memory management

---
