#include "paging.h"
#include "lib.h"

/*
0MB
    Video Memory(4kb)
4MB
    Kernel(MB)
8MB
    Not Present
4GB
*/

pde page_directory[1024] __attribute__((aligned(BYTES_TO_ALIGN_TO)));
pte page_table[1024]__attribute__((aligned(BYTES_TO_ALIGN_TO)));
pte vidMapPageTable[1024]__attribute__((aligned(BYTES_TO_ALIGN_TO)));

// Function to initalize paging
// Input: none
// Output: none
void init_paging()
{
    //Set The Kernel in Page directory
    //present = 1 because kernel is loaded in physical memory
    //read_write = 1 because kernel data is read and written
    //page_size = 1 because page size is 4MB
    //global_page = 1 to prevent flushing
    //page_table_address = 1 << 10 because we are referencing the second 4MB page
    //it is left shifted by 10 due to reserved bits
    page_directory[1].present = 1;
    page_directory[1].read_write = 1;
    page_directory[1].user_supervisor = 0;
    page_directory[1].write_through = 0;
    page_directory[1].cache_disabled = 0;
    page_directory[1].accessed = 0;
    page_directory[1].dirty = 0;
    page_directory[1].page_size = 1;
    page_directory[1].global_page = 0;
    page_directory[1].available = 0;
    page_directory[1].page_table_address = (1<<10);

    //Set The video memory page directory
    //Video memory location 0xB8000
    //present = 1 because video memory is loaded in physical memory
    //read_write = 1 because video data is read and written
    //page_table_address = address of first page table
    //left shifted by 12 because we want to avoid padding
    page_directory[0].present = 1;
    page_directory[0].read_write = 1;
    page_directory[0].user_supervisor = 0;
    page_directory[0].write_through = 0;
    page_directory[0].cache_disabled = 0;
    page_directory[0].accessed = 0;
    page_directory[0].dirty = 0;
    page_directory[0].page_size = 0;
    page_directory[0].global_page = 0;
    page_directory[0].available = 0;
    page_directory[0].page_table_address = (((uint32_t)&page_table[0])>>12);

    //Set the video memory page table
    //page table entry 184 because (0xB8000 / 4096)
    //present = 1 due to video memory being loaded into physical memory
    //global_page = 1 to prevent flushing
    //page_base_address = 0xB8000 as that is where video memory starts
    page_table[184].present = 1;
    page_table[184].read_write = 1; 
    page_table[184].user_supervisor = 0;
    page_table[184].write_through = 0;
    page_table[184].cache_disabled = 0;
    page_table[184].accessed = 0;
    page_table[184].dirty = 0;
    page_table[184].page_table_attribute_index = 0;
    page_table[184].global_page = 0;
    page_table[184].available = 0;
    page_table[184].page_base_address = (0xB8000)>>12; 
	
    //CR3 register to hold page_directory address
    //CR4 register to hold PSE bit
    //CR0 register to hold PE bit and PG bit
	asm (
    "movl $page_directory, %%eax    ;"
    "andl $0xFFFFF000, %%eax          ;"
    "movl %%eax, %%cr3                ;"
    "movl %%cr4, %%eax                ;"
    "orl $0x00000010, %%eax           ;"
    "movl %%eax, %%cr4                ;"
    "movl %%cr0, %%eax                ;"
    "orl $0x80000001, %%eax           ;"
    "movl %%eax, %%cr0                 "
    : : : "eax", "cc" );
}

