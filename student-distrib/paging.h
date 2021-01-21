#ifndef PAGING_H
#define PAGING_H

#include "x86_desc.h"
#include "types.h"

#define BYTES_TO_ALIGN_TO 4096

/* This is a page directory entry for a 4KB page table. */
typedef struct pde {
    union {
        uint32_t val;
        struct {
            uint32_t present : 1;
            uint32_t read_write : 1;
            uint32_t user_supervisor : 1;
            uint32_t write_through : 1;
            uint32_t cache_disabled : 1;
            uint32_t accessed : 1;
            uint32_t dirty : 1;
            uint32_t page_size : 1;
            uint32_t global_page : 1;
            uint32_t available : 3;
            uint32_t page_table_address : 20;
        } __attribute__ ((packed));
    };
} pde;

/* This is a page table entry for a 4KB page table. */
typedef struct pte {
    union {
        uint32_t val;
        struct {
            uint32_t present : 1;
            uint32_t read_write : 1;
            uint32_t user_supervisor : 1;
            uint32_t write_through : 1;
            uint32_t cache_disabled : 1;
            uint32_t accessed : 1;
            uint32_t dirty : 1;
            uint32_t page_table_attribute_index : 1;
            uint32_t global_page : 1;
            uint32_t available : 3;
            uint32_t page_base_address : 20;
        } __attribute__ ((packed));
    };
} pte;

//creates a page directory with 1024 page directory entries
extern pde page_directory[1024] __attribute__((aligned(BYTES_TO_ALIGN_TO)));

/*(1024 page tables is 4MB which will push our file system outside of the kernel space)*/
//creates a page table with 1024 page table entries 
// typedef pte page_table[1024] __attribute__((aligned(BYTES_TO_ALIGN_TO)));
//creates 1024 page tables
//page_table total_tables[1024] __attribute__((aligned(BYTES_TO_ALIGN_TO)));
/********************************************************************/

//Create a single page table for 0MB-4MB
extern pte page_table[1024]__attribute__((aligned(BYTES_TO_ALIGN_TO)));
//Vid map page_table
extern pte vidMapPageTable[1024]__attribute__((aligned(BYTES_TO_ALIGN_TO)));
extern void init_paging();
#endif /* paging*/
