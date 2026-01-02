#include <kernel/console/console.h>
#include <drivers/ps2/mouse/mouse.h>
FHDR(cmd_calc) {
    (void)s;
    print("Calculator GUI - Not implemented yet\n", GFX_YELLOW);
    print("Use: calc\n", GFX_WHITE);
}