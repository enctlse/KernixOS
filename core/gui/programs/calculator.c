#include <kernel/console/console.h>
#include <drivers/ps2/mouse/mouse.h>
FHDR(cmd_calc) {
    (void)s;
    print("Calculator GUI - Not implemented yet\n", yellow);
    print("Use: calc\n", white);
}