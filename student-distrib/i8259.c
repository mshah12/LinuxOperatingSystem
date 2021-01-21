/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
// Based of the initialization from OSDev
// Initializes 8259 PIC
// Input: none
// Output: none
void i8259_init(void) {
    // Initializes the PIC by feeding each PIC the control words
    outb(ICW1, MASTER_COMMAND);
    outb(ICW1, SLAVE_COMMAND);
    outb(ICW2_MASTER, MASTER_DATA);
    outb(ICW2_SLAVE, SLAVE_DATA);
    outb(ICW3_MASTER, MASTER_DATA);
    outb(ICW3_SLAVE, SLAVE_DATA);
    outb(ICW4, MASTER_DATA);
    outb(ICW4, SLAVE_DATA);

    // Masks out all IRQs except the slave IRQ and saves it
    master_mask = 0xFF;
    slave_mask = 0xFF;
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);
}

/* Enable (unmask) the specified IRQ */
// Enables specific IRQ
// Input: IRQ to unmask
// Output: none
void enable_irq(uint32_t irq_num) {
    // Updates the local mask variables with the IRQ number
    switch (irq_num) {
        case 0:
            // Updates the master mask
            master_mask = master_mask & 0xFE;
            break;
        case 1:
            // Updates the master mask
            master_mask = master_mask & 0xFD;
            break;
        case 2:
            // Updates the master mask
            master_mask = master_mask & 0xFB;
            break;
        case 3:
            // Updates the master mask
            master_mask = master_mask & 0xF7;
            break;
        case 4:
            // Updates the master mask
            master_mask = master_mask & 0xEF;
            break;
        case 5:
            // Updates the master mask
            master_mask = master_mask & 0xDF;
            break;
        case 6:
            // Updates the master mask
            master_mask = master_mask & 0xBF;
            break;
        case 7:
            // Updates the master mask
            master_mask = master_mask & 0x7F;
            break;
        case 8:
            // Updates the slave mask
            slave_mask = slave_mask & 0xFE;
            break;
        case 9:
            // Updates the slave mask
            slave_mask = slave_mask & 0xFD;
            break;
        case 10:
            // Updates the slave mask
            slave_mask = slave_mask & 0xFB;
            break;
        case 11:
            // Updates the slave mask
            slave_mask = slave_mask & 0xF7;
            break;
        case 12:
            // Updates the slave mask
            slave_mask = slave_mask & 0xEF;
            break;
        case 13:
            // Updates the slave mask
            slave_mask = slave_mask & 0xDF;
            break;
        case 14:
            // Updates the slave mask
            slave_mask = slave_mask & 0xBF;
            break;
        case 15:
            // Updates the slave mask
            slave_mask = slave_mask & 0x7F;
            break;
        default: break;
    }

    // Sends the masks to the PICs
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);
}

/* Disable (mask) the specified IRQ */
// Disables specific IRQ
// Input: irq to be masked
// Output: none
void disable_irq(uint32_t irq_num) {
    // Updates the local mask variables with the IRQ number
    switch (irq_num) {
        case 0:
            // Updates the master mask
            master_mask = master_mask | 0x01;
            break;
        case 1:
            // Updates the master mask
            master_mask = master_mask | 0x02;
        case 2:
            // Updates the master mask
            master_mask = master_mask | 0x04;
            break;
        case 3:
            // Updates the master mask
            master_mask = master_mask | 0x08;
            break;
        case 4:
            // Updates the master mask
            master_mask = master_mask | 0x10;
            break;
        case 5:
            // Updates the master mask
            master_mask = master_mask | 0x20;
            break;
        case 6:
            // Updates the master mask
            master_mask = master_mask | 0x40;
            break;
        case 7:
            // Updates the master mask
            master_mask = master_mask | 0x80;
            break;
        case 8:
            // Updates the slave mask
            slave_mask = slave_mask | 0x01;
            break;
        case 9:
            // Updates the slave mask
            slave_mask = slave_mask | 0x02;
            break;
        case 10:
            // Updates the slave mask
            slave_mask = slave_mask | 0x04;
            break;
        case 11:
            // Updates the slave mask
            slave_mask = slave_mask | 0x08;
            break;
        case 12:
            // Updates the slave mask
            slave_mask = slave_mask | 0x10;
            break;
        case 13:
            // Updates the slave mask
            slave_mask = slave_mask | 0x20;
            break;
        case 14:
            // Updates the slave mask
            slave_mask = slave_mask | 0x40;
            break;
        case 15:
            // Updates the slave mask
            slave_mask = slave_mask | 0x80;
            break;
        default: break;
    }

    // Sends the masks to the PICs
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);
}

/* Send end-of-interrupt signal for the specified IRQ */
// Function to send EOI signal to specific IRQ
// Input: irq to send EOI to
// Output: none
void send_eoi(uint32_t irq_num) {
    // Sends the EOI signal to the PIC based on the IRQ number
    if (irq_num > 7) {
        outb(((irq_num - 8) | EOI), SLAVE_COMMAND);
        outb((2 | EOI), MASTER_COMMAND);
    }
    else outb((irq_num | EOI), MASTER_COMMAND);
}
