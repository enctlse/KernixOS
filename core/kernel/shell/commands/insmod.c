#include <kernel/shell/acsh.h>
#include <kernel/module/lkm.h>
#include <string/string.h>

FHDR(cmd_insmod)
{
    if (*s == '\0') {
        print("Usage: insmod <module_path>\n", yellow);
        return;
    }
    while (*s == ' ') s++;
    const char *path = s;
    if (lkm_load(path) == 0) {
        print("Module loaded successfully\n", green);
    } else {
        print("Failed to load module\n", red);
    }
}