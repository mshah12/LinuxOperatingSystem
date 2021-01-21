/* i8259.c - Functions to interact with the keyboard
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "keyboard.h"
#include "lib.h"

// Initializes the keyboard
// Function to initalize the keyboard
// Input: none
// Output: none
void keyboard_init(void) {
    // Initializes variables
    uint8_t ps2_byte;

    // Sets the IRQ for the PIC
    enable_irq(1);

    // Reads(0x20) the PS/2 config byte
    outb(0x20, KEYBOARD_COMMAND);
    ps2_byte = inb(KEYBOARD_COMMAND);

    // Updates the config byte to turn off scan code translation
    ps2_byte &= 0xBF;

    // Writes(0x60) config byte back to PS/2 controller
    outb(0x60, KEYBOARD_COMMAND);
    outb(ps2_byte, KEYBOARD_COMMAND);

    // // Change keyboard scan code to scan code 3
    // // inb(KEYBOARD_DATA);
    // outb(0xF0, KEYBOARD_DATA);
    // //inb(KEYBOARD_DATA);
    // outb(0x61, KEYBOARD_DATA);
    // //inb(KEYBOARD_DATA);
}

// Maps the keyboard values to letters for scan code 1
// Function to map keboard values to letter
// Input: keyboard value
// Output: character to display
unsigned char key_mapping(unsigned char c, uint8_t shift, uint8_t caps) {
    switch (c) {
        // Returns the capitalized or lower case alahabet based on the arguments
        case 0x1E: return (shift ^ caps) ? 'A' : 'a';
        case 0x30: return (shift ^ caps) ? 'B' : 'b';
        case 0x2E: return (shift ^ caps) ? 'C' : 'c';
        case 0x20: return (shift ^ caps) ? 'D' : 'd';
        case 0x12: return (shift ^ caps) ? 'E' : 'e';
        case 0x21: return (shift ^ caps) ? 'F' : 'f';
        case 0x22: return (shift ^ caps) ? 'G' : 'g';
        case 0x23: return (shift ^ caps) ? 'H' : 'h';
        case 0x17: return (shift ^ caps) ? 'I' : 'i';
        case 0x24: return (shift ^ caps) ? 'J' : 'j';
        case 0x25: return (shift ^ caps) ? 'K' : 'k';
        case 0x26: return (shift ^ caps) ? 'L' : 'l';
        case 0x32: return (shift ^ caps) ? 'M' : 'm';
        case 0x31: return (shift ^ caps) ? 'N' : 'n';
        case 0x18: return (shift ^ caps) ? 'O' : 'o';
        case 0x19: return (shift ^ caps) ? 'P' : 'p';
        case 0x10: return (shift ^ caps) ? 'Q' : 'q';
        case 0x13: return (shift ^ caps) ? 'R' : 'r';
        case 0x1F: return (shift ^ caps) ? 'S' : 's';
        case 0x14: return (shift ^ caps) ? 'T' : 't';
        case 0x16: return (shift ^ caps) ? 'U' : 'u';
        case 0x2F: return (shift ^ caps) ? 'V' : 'v';
        case 0x11: return (shift ^ caps) ? 'W' : 'w';
        case 0x2D: return (shift ^ caps) ? 'X' : 'x';
        case 0x15: return (shift ^ caps) ? 'Y' : 'y';
        case 0x2C: return (shift ^ caps) ? 'Z' : 'z';
        // The following values only change if shift is held
        case 0x02: return (shift) ? '!' : '1';
        case 0x03: return (shift) ? '@' : '2';
        case 0x04: return (shift) ? '#' : '3';
        case 0x05: return (shift) ? '$' : '4';
        case 0x06: return (shift) ? '%' : '5';
        case 0x07: return (shift) ? '^' : '6';
        case 0x08: return (shift) ? '&' : '7';
        case 0x09: return (shift) ? '*' : '8';
        case 0x0A: return (shift) ? '(' : '9';
        case 0x0B: return (shift) ? ')' : '0';
        case 0x0C: return (shift) ? '_' : '-';
        case 0x0D: return (shift) ? '+' : '=';
        case 0x1A: return (shift) ? '{' : '[';
        case 0x1B: return (shift) ? '}' : ']';
        case 0x27: return (shift) ? ':' : ';';
        case 0x28: return (shift) ? '"' : '\'';
        case 0x29: return (shift) ? '~' : '`';
        case 0x2B: return (shift) ? '|' : '\\';
        case 0x33: return (shift) ? '<' : ',';
        case 0x34: return (shift) ? '>' : '.';
        case 0x35: return (shift) ? '?' : '/';
        // Case for space
        case 0x39: return ' ';
        // If not any of the characters above return NULL
        default: return NULL;
    }
}
