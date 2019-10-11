#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include "beavalloc.h"

#define REQ_SIZE 1024
void *lower_mem_bound = NULL;
void *upper_mem_bound = NULL;
struct block_list *head_block = NULL;
struct block_list *tail_block = NULL;
int total_capacity = 0;

typedef struct block_list {
    struct block_list *prev;
    struct block_list *next;
    void *data;
    int capacity;
    int size;
    int excess;
    bool free;
} header;

void *beavalloc(size_t size) {

    uint i = 0;
    void *addr;
    void *beg_addr;
    struct block_list *start = NULL;
    struct block_list *curr = NULL;
    int previous_capacity = 0;
    int specific_capacity = 0;

    // error checking
    if (size <= 0) {
        return NULL;
    }

    beg_addr = sbrk(0);

    // memeory has already been allocated
    // make sure there is not enough mem in current mem before allocating more
    // make sure you are taking into account the header as well

    for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
        if (curr->excess > (size + sizeof(header) + 100)) {
            // if significanlty enough space in found block split the block

            start = curr->data + size;

            start->prev = curr;
            start->next = curr->next;
            start->data = curr->data + size + sizeof(header);
            start->capacity = curr->capacity - sizeof(header) - size;
            start->size = 0;
            start->excess = start->capacity - start->size;
            start->free = FALSE;

            curr->next = start;

            // make sure tail is not pointing to garbage
            if (curr == tail_block) {
                tail_block = curr->next;
            }
            return start->data;

        } else if (curr->free == TRUE && curr->capacity <= size){
            // no need to split just assign free block with users req

            curr->size = size;
            curr->free = FALSE;
            return curr->data;
        }
    }

    previous_capacity = total_capacity;
    while (total_capacity < (size + previous_capacity)) {
        addr = sbrk(REQ_SIZE);
        total_capacity += REQ_SIZE;
        specific_capacity += REQ_SIZE;
    }

    if (addr == (void *)-1) {
        errno = ENOMEM;
        return NULL;
    }

    if (head_block == NULL) {
        start = beg_addr;

        start->prev = NULL;
        start->next = NULL;
        start->data = beg_addr + sizeof(header);

        head_block = beg_addr;
        tail_block = beg_addr;
        lower_mem_bound = beg_addr;
        upper_mem_bound = addr;

    } else {
        start = addr;

        start->prev = tail_block;
        start->next = NULL;
        start->data = addr + sizeof(header);

        tail_block->next = start;
        tail_block = start;
        upper_mem_bound = addr;
    }
    start->capacity = specific_capacity - sizeof(header);
    start->size = size;
    start->excess = start->capacity - start->size;
    start->free = FALSE;

    return start->data;
}

void beavfree(void *ptr) {

    // uint i = 0;
    struct block_list *curr = NULL;
    struct block_list *next_block = NULL;
    struct block_list *prev_block = NULL;

    curr = ptr - sizeof(header);
    // error checking
    if (ptr == NULL || curr->free == TRUE) {
        return;
    }

    curr->free = TRUE;

    if (curr->next != NULL && curr->next->free == TRUE) {

        next_block = curr->next->next;

        if (curr->next == tail_block) {
            tail_block = curr;
        }
        curr->capacity += curr->next->capacity;
        curr->next = next_block; 
        curr->size = 0;
        curr->excess = 0;

        if (next_block != NULL) {
            next_block->prev = curr;
        }
    }
    if (curr->prev != NULL && curr->prev->free == TRUE) {

        prev_block = curr->prev;

        prev_block->capacity += curr->capacity;
        prev_block->next = curr->next;
        prev_block->size = 0;
        prev_block->excess = 0;
    }

    // for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
    //     if (curr->free == TRUE) {
    //         while (curr->next != NULL && curr->next->free == TRUE) {
    //             // coalesce blocks
    //             // make sure you are taking into account the header as well
    //             next_block = curr->next->next;
    //             // make sure tail is not pointing to garbage
    //             if (curr->next == tail_block) {
    //                 tail_block = curr;
    //             }

    //             // HOW TO FREE UP OTHER BLOCK HEADER MEMORY??
    //             curr->capacity = curr->capacity + curr->next->capacity;
    //             curr->next = next_block; 
    //             curr->size = 0;
    //             curr->excess = 0;

    //             if (next_block != NULL) {
    //                 next_block->prev = curr;
    //             }
    //         }
    //     }
    // }
}

void beavalloc_reset(void) {
    brk(head_block);
    return;
}

void beavalloc_set_verbose(uint8_t typo) {


}

void *beavcalloc(size_t nmemb, size_t size) {

    return NULL;
}

void *beavrealloc(void *ptr, size_t size) {

    return NULL;
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
