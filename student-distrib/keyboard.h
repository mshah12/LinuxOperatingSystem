/* keyboard.h - Defines what is used in interactions with the keyboard
 * vim:ts=4 noexpandtab
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

// Command and data lines for the keyboard
#define KEYBOARD_DATA       0x60
#define KEYBOARD_COMMAND    0x64

// Initializes the keyboard
void keyboard_init(void);
// Maps the keyboard values to letters
unsigned char key_mapping(unsigned char c, uint8_t shift, uint8_t caps);

#endif /* _KEYBOARD_H */
