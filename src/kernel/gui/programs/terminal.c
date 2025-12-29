#include <kernel/gui/gui.h>
#include <string/string.h>
#include <kernel/graph/graphics.h>
#include <drivers/ps2/mouse/mouse.h>
#include <kernel/console/graph/dos.h>
#include <kernel/console/console.h>
FHDR(cmd_gui) {
    (void)s;
    if (gui_running) {
        print("GUI already running!\n", GFX_YELLOW);
        return;
    }
    print("Starting graphical interface...\n", GFX_GREEN);
    gui_init();
    print("GUI started successfully!\n", GFX_GREEN);
    print("Use mouse/keyboard inside GUI. Type 'exit' in the terminal window to return.\n", GFX_CYAN);
}
FHDR(cmd_loadcursor) {
    if (!s || s[0] == '\0') {
        print("Usage: loadcursor <filename>\n", GFX_RED);
        print("File should be in /system/cursor/ directory\n", GFX_CYAN);
        return;
    }
    print("Loading cursor: ", GFX_CYAN);
    print(s, GFX_YELLOW);
    print("\n", GFX_CYAN);
    print("Cursor loading not implemented yet (needs file system)\n", GFX_RED);
    print("Using built-in cursor for now\n", GFX_YELLOW);
}