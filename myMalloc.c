#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct block {
    size_t size;
    struct block *next;
    struct block *prev;
} block;

#ifndef ALLOC_UNIT
#define ALLOC_UNIT 3 * sysconf(_SC_PAGESIZE)
#endif

#ifndef MIN_DEALLOC
#define MIN_DEALLOC 1 * sysconf(_SC_PAGESIZE)
#endif

#define BLOCK_MEM(ptr) ((void *)((unsigned long)ptr + sizeof(block)))
#define BLOCK_HEADER(ptr) ((void *)((unsigned long)ptr - sizeof(block)))

static block *mHead = NULL;

void remove_block(block *ptr) {
    if (!ptr->prev) {
        if (ptr->next) {
            mHead = ptr->next;
        } else {
            mHead = NULL;
        }
    } else {
        ptr->prev->next = ptr->next;
    }
    if (ptr->next) {
        ptr->next->prev = ptr->prev;
    }
}

block *split(block *ptr, size_t size) {
    void *mem_block = BLOCK_MEM(ptr);
    block *newptr = (block *) ((unsigned long) mem_block + size);
    newptr->size = ptr->size - (size + sizeof(block));
    ptr->size = size;
    return newptr;
}

void add_block(block *b) {
    b->prev = NULL;
    b->next = NULL;
    if (!mHead || (unsigned long) mHead > (unsigned long) b) {
        if (mHead) {
            mHead->prev = b;
        }
        b->next = mHead;
        mHead = b;
    } else {
        block *curr = mHead;
        while (curr->next
               && (unsigned long) curr->next < (unsigned long) b) {
            curr = curr->next;
        }
        b->next = curr->next;
        curr->next = b;
    }
}

void *_malloc(size_t size) {
    void *block_mem;
    block *ptr, *newptr;
    size_t alloc_size = size >= ALLOC_UNIT ? size + sizeof(block)
                                           : ALLOC_UNIT;
    ptr = mHead;
    while (ptr) {
        if (ptr->size >= size + sizeof(block)) {
            block_mem = BLOCK_MEM(ptr);
            remove_block(ptr);
            if (ptr->size == size) { // found a perfect sized block
                return block_mem;
            }
            // our block is bigger then requested, split it and add
            newptr = split(ptr, size);
            add_block(newptr);
            return block_mem;
        } else {
            ptr = ptr->next;
        }
    }
    ptr = sbrk(alloc_size);
    if (!ptr) {
        printf("failed to alloc %ld\n", alloc_size);
        return NULL;
    }
    ptr->next = NULL;
    ptr->prev = NULL;
    ptr->size = alloc_size - sizeof(block);
    if (alloc_size > size + sizeof(block)) {
        newptr = split(ptr, size);
        add_block(newptr);
    }
    return BLOCK_MEM(ptr);
}


void _free(void *ptr) {
    add_block(BLOCK_HEADER(ptr));
    block *curr = mHead;
    unsigned long header_curr, header_next;
    unsigned long program_break = (unsigned long) sbrk(0);
    if (program_break == 0) {
        printf("failed to retrieve program break\n");
        return;
    }
    while (curr->next) {
        header_curr = (unsigned long) curr;
        header_next = (unsigned long) curr->next;
        if (header_curr + curr->size + sizeof(block) == header_next) {
            curr->size += curr->next->size + sizeof(block);
            curr->next = curr->next->next;
            if (curr->next) {
                curr->next->prev = curr;
            } else {
                break;
            }
        }
        curr = curr->next;
    }
    header_curr = (unsigned long) curr;
    if (header_curr + curr->size + sizeof(block) == program_break
        && curr->size >= MIN_DEALLOC) {
        remove_block(curr);
        if (brk(curr) != 0) {
            printf("error freeing memory\n");
        }
    }
}

void *_calloc(size_t nsize, size_t size) {
    size_t newsize = size*nsize;
    void *ptr;
    ptr = _malloc(newsize);
    if (!ptr) {
        return NULL;
    }
    memset(ptr, 0, newsize);
    return ptr;
}
