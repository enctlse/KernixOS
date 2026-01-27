#include "acsh.h"
#include "functions.h"
extern char cwd[];
#define MAX_PATH_LEN 256
void shell_clear_screen(u32 color)
{
    console_window_clear(color);
}
void shell_print_prompt(void)
{
    string("\n", white);
    string(User, green);
    string("@", green);
    string(Host, green);
    string(":", white);
    if (str_len(cwd) > 1 && cwd[str_len(cwd) - 1] == '/') {
        char prompt_cwd[MAX_PATH_LEN];
        str_copy(prompt_cwd, cwd);
        prompt_cwd[str_len(cwd) - 1] = '\0';
        string(prompt_cwd, gray_70);
    }
    string("# ", gray_70);
}
void shell_redraw_input(void)
{
}