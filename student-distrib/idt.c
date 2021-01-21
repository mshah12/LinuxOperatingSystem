/* idt.c - Functions to setup the idt
 * vim:ts=4 noexpandtab
 */

#include "idt.h"
#include "x86_desc.h"
#include "interrupt.h"

// Sets up the IDT table
// Function to set up the IDT table
// Input: none
// Output: none
void idt_entry_setup() {
     // Initializes IDT entries
    {
        // Initializes variables
        uint32_t i;

        // Fills out Intel defined entries in the IDT
        for (i = 0; i < 256; i++) {
            // Fills out the IDT entry values
            idt[i].seg_selector = KERNEL_CS;
            idt[i].reserved4 = 0;
            idt[i].reserved3 = 1;
            idt[i].reserved2 = 1;
            idt[i].reserved1 = 1;
            idt[i].size = 1;
            idt[i].reserved0 = 0;
            idt[i].dpl = 0;
            idt[i].present = 1;

            // If the IDT entry is not the Intel ones change the gate type
            if (i > 31) idt[i].reserved3 = 0;

            // If the IDT entry is the system call entry (0x80) change the dpl and gate
            if (i == 0x80) {idt[i].dpl = 3; idt[i].reserved3 = 1;}

            // Marks unused interrupts
            if (i == 15) idt[i].present = 0;
            if (i > 0x13 && i != 0x80 && i!= 0x20 && i != 0x21 && i != 0x28) idt[i].present = 0;

            // Sets IDT entry handler address for each entry
            // Entries 0x00 - 0x14, 0x21, 0x28, and 0x80 have special handlers 
            if (i < 0x14 && i != 15) SET_IDT_ENTRY(idt[i], &int_num_0 + (4 * i));
            else if (i == 0x28) SET_IDT_ENTRY(idt[i], &rtc_interrupt_handler_prep);
            else if (i == 0x21) SET_IDT_ENTRY(idt[i], &kb_interrupt_handler_prep);
            else if (i == 0x20) SET_IDT_ENTRY(idt[i], &pit_handler_prep);
            else if (i == 0x80) SET_IDT_ENTRY(idt[i], &system_call_prep);
            else SET_IDT_ENTRY(idt[i], &other_interrupt_handler_prep);
        }
    }
}
