#include <stdio.h>
#include <stdbool.h>
#include "beavalloc.h"
#include "main.c"

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
    bool free;
} header;

void *beavalloc(size_t size) {

    uint i = 0;
    void *addr;
    struct block_list start;
    struct block_list *curr = NULL;
    void *find_mem;

    // error checking
    if (size <= 0) {
        return NULL;
    }

    if (head_block == NULL) {
        // first time allocating memory

        addr = sbrk(REQ_SIZE);

        if (addr == (void *)-1) {
            errno = ENOMEM;
            return NULL;
        }

        start.prev = NULL;
        start.next = NULL;
        start.data = addr;
        // TODO: WHAT EXACTLY IS THE CAPACITY??
        start.capacity = REQ_SIZE - sizeof(header);
        start.size = size;
        start.free = FALSE;

        head_block = &start;
        tail_block = &start;
        lower_mem_bound = addr;
        upper_mem_bound = addr + REQ_SIZE;

        find_mem = &start;

        return find_mem;
    } else {
        // memeory has already been allocated
        // make sure there is not enough mem in current mem before allocating more
        // make sure you are taking into account the header as well

        for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
            if (curr->free == TRUE && curr->capacity >= size) {
                if (curr->capacity > ((curr->size + sizeof(header)) * 2)) {
                    // if significanlty enough space in found block split the block

                    start.prev = curr;
                    start.next = curr->next;
                    start.data = NULL;
                    start.capacity = curr->capacity - sizeof(header) - size;
                    start.size = 0;
                    start.free = TRUE;


                    curr->next = &start;
                    // TODO: curr->data = ??
                    curr->size = size;
                    curr->free = FALSE;

                    // make sure tail is not pointing to garbage
                    if (curr == tail_block) {
                        tail_block = curr->next;
                    }
                    // TODO: return ??

                } else {
                    // no need to split just assign free block with users req

                    // TODO: curr->data = ??
                    curr->size = size;
                    curr->free = FALSE;
                    // TODO: return ??
                }
            }
        }

        // entering here means we don't have enough space for the req need to make more mem
        // TODO: How do you allocate more memory??
        addr = sbrk(REQ_SIZE);

        if (addr == (void *)-1) {
            errno = ENOMEM;
            return NULL;
        }

        start.prev = tail_block;
        start.next = NULL;
        start.data = addr;
        // TODO: WHAT EXACTLY IS THE CAPACITY??
        start.capacity = REQ_SIZE - sizeof(header);
        start.size = size;
        start.free = FALSE;

        tail_block->next = &start;
        tail_block = &start;
        lower_mem_bound = addr;
        upper_mem_bound = addr + REQ_SIZE;

        find_mem = &start;

        return find_mem;
    }
    return NULL;
}

void beavfree(void *ptr) {

    uint i = 0;
    struct block_list *curr = NULL;

    for (curr = head_block, i=0; curr != NULL; curr = curr->next, i++) {
        if (curr->free == TRUE && curr->next != NULL && curr->next->free == TRUE) {
            // coalesce blocks
            // make sure you are taking into account the header as well

            // make sure tail is not pointing to garbage
            if (curr->next == tail_block) {
                tail_block = curr;
            }

            // HOW TO FREE UP OTHER BLOCK HEADER MEMORY??
            curr->capacity = curr->capacity + curr->next->capacity;
            curr->next = curr->next->next;
            curr->size = 0;
        }
    }
}

void beavalloc_reset(void) {


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
