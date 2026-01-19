#ifndef CONSOLE_H
#define CONSOLE_H
#include <kernel/display/visual.h>
#include <kernel/display/fonts/typeface.h>
#include <config/boot.h>
#include <config/user.h>
#include <kernel/module/module.h>
extern struct component_handler console_handler;
extern int boot_completed;
#define MAX_INPUT_LEN 256
#define MAX_CMDS 32
#define MAX_CHAINED_CMDS 8
#define FHDR(name) void name(const char* s)
typedef struct {
    void (*func)(const char*);
    const char *name;
    const char *description;
    const char *usage;
} console_cmd_t;
#define CMDENTRY(func, name, desc, usage) { func, name, desc, usage }
void console_init(void);
void console_run(void);
void console_handle_key(int c);
void console_execute(const char *input);
void console_set_input_start_x(void);
void shell_clear_screen(u32 color);
void shell_print_prompt(void);
void shell_redraw_input(void);
void cursor_(void);
void cursor_draw(void);
void cursor_c(void);
void cursor_redraw(void);
void cursor_enable(void);
void cursor_disable(void);
void cursor_reset_blink(void);
console_cmd_t* console_find_cmd(const char *name);
int parse_color(const char *color_str, u32 *out_color);
void parse_and_execute_chained(const char *input);
#endif