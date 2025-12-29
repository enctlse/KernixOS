#include "console.h"
#include "graph/uno.h"
#include "graph/dos.h"
extern char cwd[];
#define MAX_PATH_LEN 256
void shell_clear_screen(u32 color)
{
    console_window_clear(color);
}
void shell_print_prompt(void)
{
    string("\n", GFX_WHITE);
    string(USER_NAME, GFX_GREEN);
    string("@", GFX_GREEN);
    string(PC_NAME, GFX_GREEN);
    string(":", GFX_WHITE);
    if (str_len(cwd) > 1 && cwd[str_len(cwd) - 1] == '/') {
        char prompt_cwd[MAX_PATH_LEN];
        str_copy(prompt_cwd, cwd);
        prompt_cwd[str_len(cwd) - 1] = '\0';
        string(prompt_cwd, GFX_WHITE);
    }
    string("$ ", GFX_BLUE);
}
void shell_redraw_input(void)
{
}