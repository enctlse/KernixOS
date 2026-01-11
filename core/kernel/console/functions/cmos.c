#include <kernel/console/console.h>
#include <drivers/cmos/cmos.h>
#include <kernel/exceptions/timer.h>
FHDR(cmd_cal) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
}
    GetCMOSDate();
    print(" ", gray_70);
    GetCMOSTime();
}
FHDR(cmd_date) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
}
    GetCMOSDate();
    print("\n", white);
}
FHDR(cmd_time) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
}
    GetCMOSTime();
}
FHDR(cmd_uptime) {
    (void)s;
    if (*s != '\0') {
    print("Error: invalid option: ", red);
    print(s, red);
    print("\n", red);
    return;
}
    print("Uptime: ", gray_70);
    timer_print_uptime();
}