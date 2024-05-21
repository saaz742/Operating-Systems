
#include "mm_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

s_block_ptr first_heap = NULL;

void split_block(s_block_ptr block, size_t new_size)
{
    s_block_ptr new = NULL;
    if (block->size >= BLOCK_SIZE + new_size)
    {
        new = (s_block_ptr)((void *)block + BLOCK_SIZE + new_size);
        new->size = block->size - new_size - BLOCK_SIZE;
        new->prev = block;
        new->next = block->next;
        if (block->next)
            block->next->prev = new;
        block->size = new_size;
        new->ptr = BLOCK_SIZE + (void *)new;
        new->isfree = 1;
        block->next = new;
    }
    return new;
}

s_block_ptr fusion(s_block_ptr block)
{
    if (block)
    {
        if (block->next && block->next->isfree)
        {
            block->size += BLOCK_SIZE + block->next->size;
            block->next = block->next->next;
            if (block->next)
                block->next->prev = block;
        }
        if (block->prev && block->prev->isfree)
        {
            block->prev->size += BLOCK_SIZE + block->size;
            block = block->prev;
            if (block->next)
                block->next->prev = block->prev;
            block->prev->next = block->next;
        }
        return block;
    }
    return NULL;
}

s_block_ptr get_block(void *p)
{
    char *s;
    s = (char *)p;
    s_block_ptr block = (s_block_ptr)(s - BLOCK_SIZE);
    return block;
}

s_block_ptr extend_heap(s_block_ptr block, size_t size)
{
    void *newp = sbrk(BLOCK_SIZE + size);
    if (newp != (void *)-1)
    {
        s_block_ptr new = newp;
        if (first_heap)
            block->next = new;
        else
            first_heap = new;
        new->size = size;
        new->ptr = new + 1;
        new->prev = block;
        new->next = NULL;
        new->isfree = 0;
        return new;
    }
    return NULL;
}

void *mm_malloc(size_t size)
{
    s_block_ptr last, cur, new;
    cur = first_heap;
    last = NULL;
    if (size == 0)
        return NULL;
    while (cur)
    {
        if (cur->isfree && cur->size >= size)
        {
            split_block(cur, size);
            cur->isfree = 0;
            return cur->ptr;
        }
        else
        {
            last = cur;
            cur = cur->next;
        }
    }
    if ((new = extend_heap(last, size)) == NULL)
        return NULL;
    memset(new->ptr, 0, size);
    new->isfree = 0;
    return new->ptr;
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t s;
    s_block_ptr block;
    void *newp;
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    if (ptr == NULL)
        return mm_malloc(size);
    if (newp = mm_malloc(size) == NULL)
        return NULL;
    if (block = get_block(ptr) != NULL)
    {
        if (block->size > size)
            s = size;
        else
            s = block->size;
        memcpy(newp, ptr, s);
        mm_free(ptr);
        return newp;
    }
    return NULL;
}

void mm_free(void *ptr)
{
    s_block_ptr block;
    if (ptr == NULL)
        return;
    if ((block = get_block(ptr)) != NULL)
    {
        block->isfree = 1;
        memset(block->ptr, 0, block->size);
        fusion(block);
    }
}