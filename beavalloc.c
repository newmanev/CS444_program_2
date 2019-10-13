#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "beavalloc.h"

#define REQ_SIZE 1024
void *lower_mem_bound = NULL;
void *upper_mem_bound = NULL;
struct block_list *head_block = NULL;
struct block_list *tail_block = NULL;

typedef struct block_list {
    struct block_list *prev;
    struct block_list *next;
    void *data;
    int capacity;
    int size;
    int excess;
    bool free;
} header;

void all_info(struct block_list *);
void true_stuff();


void all_info(struct block_list *this) {
    printf("WHERE DOES THIS BLOCK POINT TO?\t%p\n", (void *)this);
    printf("Let's do this:\n prev: %p\n next; %p\n data: %p\n capacity: %d\n size: %d\n excess: %d\n free %d\n", 
    this->prev, this->next, this->data, this->capacity, this->size, this->excess, this->free);
    printf("Global stuff:\n lower_mem_bound: %p\n upper_mem_bound: %p\n head_block: %p\n tail_block: %p\n",
                            lower_mem_bound, upper_mem_bound, head_block, tail_block);
    printf("SIZE OF BLOCK: %ld\n", sizeof(header));
}

void *beavalloc(size_t size) {

    uint i = 0;
    void *addr = NULL;
    void *beg_addr = NULL;
    void *first_addr = NULL;
    struct block_list *start = NULL;
    struct block_list *curr = NULL;
    int prev_total_mem_count = 0;
    int specific_capacity = 0;
    int total_mem_count = 0;

    // error checking
    if (size <= 0) {
        return NULL;
    }

    first_addr = sbrk(0);

    // make sure there is not enough mem in current mem before allocating more
    // make sure you are taking into account the header as well

    for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
        total_mem_count += curr->capacity + sizeof(header);

        if (curr->free == TRUE && curr->capacity >= size + sizeof(header)){
            // no need to split just assign free block with users req

            curr->size = size;
            curr->free = FALSE;
            curr->excess = curr->capacity - size;
            // printf("NO SPLIT FREE BLOCK!\n");
            // all_info(curr);
            return curr->data;

        } else if (curr->excess > (size + sizeof(header))) {
            // if significanlty enough space in found block split the block

            start = curr->data + curr->size;

            start->prev = curr;
            start->next = curr->next;
            start->data = curr->data + curr->size + sizeof(header);
            start->capacity = curr->excess - sizeof(header);
            start->size = size;
            start->excess = start->capacity - start->size;
            start->free = FALSE;

            curr->next = start;
            curr->capacity = curr->size;
            curr->excess = 0;

            // make sure tail is not pointing to garbage
            if (curr == tail_block) {
                tail_block = start;
            }
            // printf("SPLIT\n");
            // all_info(start);
            return start->data;

        } 
    }

    prev_total_mem_count = total_mem_count;
    while (total_mem_count < (size + prev_total_mem_count + sizeof(header))) {
        addr = sbrk(REQ_SIZE);
        if (beg_addr == NULL) {
            beg_addr = addr;
        }
        total_mem_count += REQ_SIZE;
        specific_capacity += REQ_SIZE;
    }

    if (addr == (void *)-1) {
        errno = ENOMEM;
        return NULL;
    }

    if (head_block == NULL) {
        start = (struct block_list *)first_addr;

        start->prev = NULL;
        start->next = NULL;
        start->data = first_addr + sizeof(header);

        head_block = start;
        tail_block = start;
        lower_mem_bound = (void *)start;
        upper_mem_bound = (void *)start;

    } else {
        start = (struct block_list *)upper_mem_bound;

        tail_block->next = start;

        start->next = NULL;
        start->data = upper_mem_bound + sizeof(header);

        start->prev = tail_block;
        tail_block = tail_block->next;
    }
    start->capacity = specific_capacity - sizeof(header);
    start->size = size;
    start->excess = start->capacity - start->size;
    start->free = FALSE;

    upper_mem_bound += specific_capacity;

    // printf("NEW BLOCK FOR STUFF\n");
    // all_info(start);
    return start->data;
}

void beavfree(void *ptr) {

    uint i = 0;
    struct block_list *curr = NULL;
    struct block_list *next_block = NULL;

    curr = ptr - sizeof(header);
    // error checking
    if (ptr == NULL || curr->free == TRUE) {
        return;
    }

    curr->free = TRUE;

    for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
        if (curr->free == TRUE) {
            while (curr->next != NULL && curr->next->free == TRUE) {
                // coalesce blocks
                // make sure you are taking into account the header as well
                next_block = curr->next->next;
                // make sure tail is not pointing to garbage
                if (curr->next == tail_block) {
                    tail_block = curr;
                }

                curr->capacity = curr->capacity + curr->next->capacity + sizeof(header);
                curr->next = next_block; 
                curr->size = 0;
                curr->excess = curr->capacity;

                if (next_block != NULL) {
                    next_block->prev = curr;
                }
            }
        }
    }
}

void beavalloc_reset(void) {
    brk(head_block);
    head_block = NULL;
    tail_block = NULL;
    lower_mem_bound = NULL;
    upper_mem_bound = NULL;
    return;
}

void beavalloc_set_verbose(uint8_t display_diagnostics) {

    if (display_diagnostics == TRUE) {
        fprintf(stderr, "The woas of life\n");
    }
}

void *beavcalloc(size_t nmemb, size_t size) {

    uint i = 0;
    void *mem = NULL;

    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    mem = beavalloc(size * nmemb);

    for (i = 0; i < nmemb; i++) {
        memset(mem, 0, size);
        mem += size;
    }

    return mem;
}

void *beavrealloc(void *ptr, size_t size) {

    void *mem = NULL;
    struct block_list *header_info = ptr - sizeof(header);

    if (size == 0) {
        return NULL;
    }
    if (ptr == NULL) {
        mem = beavalloc(size*2);
    } else {

        if(header_info->size >= size) {
            return ptr;
        }

        mem = beavalloc(size);
        memcpy(mem, ptr, header_info->size);
        beavfree(ptr);
    }
    return mem;
}

void beavalloc_dump(uint leaks_only)
{
    struct block_list *curr = NULL;
    uint i = 0;
    uint leak_count = 0;
    uint user_bytes = 0;
    uint capacity_bytes = 0;
    uint block_bytes = 0;
    uint used_blocks = 0;
    uint free_blocks = 0;

    if (leaks_only) {
        fprintf(stderr, "heap lost blocks\n");
    }
    else {
        fprintf(stderr, "heap map\n");
    }
    fprintf(stderr
            , "  %s\t%s\t%s\t%s\t%s" 
            "\t%s\t%s\t%s\t%s\t%s\t%s"
            "\n"
            , "blk no  "
            , "block add "
            , "next add  "
            , "prev add  "
            , "data add  "
            
            , "blk off  "
            , "dat off  "
            , "capacity "
            , "size     "
            , "blk size "
            , "status   "
        );
    for (curr = head_block, i = 0; curr != NULL; curr = curr->next, i++) {
        if (leaks_only == FALSE || (leaks_only == TRUE && curr->free == FALSE)) {
            fprintf(stderr
                    , "  %u\t\t%9p\t%9p\t%9p\t%9p\t%u\t\t%u\t\t"
                      "%u\t\t%u\t\t%u\t\t%s\t%c\n"
                    , i
                    , curr
                    , curr->next
                    , curr->prev
                    , curr->data
                    , (unsigned) ((void *) curr - lower_mem_bound)
                    , (unsigned) ((void *) curr->data - lower_mem_bound)
                    , (unsigned) curr->capacity
                    , (unsigned) curr->size
                    , (unsigned) (curr->capacity + sizeof(struct block_list))
                    , curr->free ? "free  " : "in use"
                    , curr->free ? '*' : ' '
                );
            user_bytes += curr->size;
            capacity_bytes += curr->capacity;
            block_bytes += curr->capacity + sizeof(struct block_list);
            if (curr->free == FALSE && leaks_only == TRUE) {
                leak_count++;
            }
            if (curr->free == TRUE) {
                free_blocks++;
            }
            else {
                used_blocks++;
            }
        }
    }
    if (leaks_only) {
        if (leak_count == 0) {
            fprintf(stderr, "  *** No leaks found!!! That does NOT mean no leaks are possible. ***\n");
        }
        else {
            fprintf(stderr
                    , "  %s\t\t\t\t\t\t\t\t\t\t\t\t"
                      "%u\t\t%u\t\t%u\n"
                    , "Total bytes lost"
                    , capacity_bytes
                    , user_bytes
                    , block_bytes
                );
        }
    }
    else {
        fprintf(stderr
                , "  %s\t\t\t\t\t\t\t\t\t\t\t\t"
                "%u\t\t%u\t\t%u\n"
                , "Total bytes used"
                , capacity_bytes
                , user_bytes
                , block_bytes
            );
        fprintf(stderr, "  Used blocks: %u  Free blocks: %u  "
             "Min heap: %p    Max heap: %p\n"
               , used_blocks, free_blocks
               , lower_mem_bound, upper_mem_bound
            );
    }
}
