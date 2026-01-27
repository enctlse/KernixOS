#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <outputs/types.h>
#include <kernel/module/module.h>
#include <kernel/interrupts/cpu/cpu_faults.h>
#define KEY_BUFFER_SIZE 128
typedef struct {
    char buffer[KEY_BUFFER_SIZE];
    int read_pos;
    int write_pos;
} key_buffer_t;
void keyboard_init(void);
void keyboard_poll(void);
int keyboard_has_key(void);
char keyboard_get_key(void);
void keyboard_process_input(void);
int keyboard_get_interrupt_count(void);
void keyboard_interrupt_handler(cpu_state_t* state);
extern struct component_handler keyboard_handler;
#endif