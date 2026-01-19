#include <kernel/shell/acsh.h>
#include <kernel/module/lkm.h>
#include <string/string.h>

FHDR(cmd_rmmod)
{
    if (*s == '\0') {
        print("Usage: rmmod <module_name>\n", yellow);
        return;
    }
    while (*s == ' ') s++;
    const char *name = s;
    if (lkm_unload(name) == 0) {
        print("Module unloaded successfully\n", green);
    } else {
        print("Failed to unload module or module not found\n", red);
    }
}