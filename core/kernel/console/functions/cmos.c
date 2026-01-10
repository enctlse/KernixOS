#include <kernel/console/console.h>
#include <drivers/cmos/cmos.h>
#include <kernel/exceptions/timer.h>
FHDR(cmd_cal) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
}
    GetCMOSDate();
    print(" ", GFX_GRAY_70);
    GetCMOSTime();
}
FHDR(cmd_date) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
}
    GetCMOSDate();
    print("\n", GFX_WHITE);
}
FHDR(cmd_time) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
}
    GetCMOSTime();
}
FHDR(cmd_uptime) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
}
    print("Uptime: ", GFX_GRAY_70);
    timer_print_uptime();
}