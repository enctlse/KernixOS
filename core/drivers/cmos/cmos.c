#include "cmos.h"
#include <kernel/include/ports.h>
#include <kernel/graph/graphics.h>
static u8 cmos_read_register(u8 reg) {
    outb(0x70, reg);
    return inb(0x71);
}
static u8 bcd_to_binary(u8 bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}
USHORT GetCMOSMem()
{
    outb(0x70, 0x30);
    u8 low = inb(0x71);
    outb(0x70, 0x31);
    u8 high = inb(0x71);
    USHORT total = low | high << 8;
    return total;
}
void GetCMOSDate()
{
    outb(0x70, 0x0A);
    while(inb(0x71) & 0x80);
    outb(0x70, 0x07);
    u8 day = inb(0x71);
    outb(0x70, 0x08);
    u8 month = inb(0x71);
    outb(0x70, 0x09);
    u8 year = inb(0x71);
    day   = ((day   / 16) * 10) + (day   & 0x0F);
    month = ((month / 16) * 10) + (month & 0x0F);
    year  = ((year  / 16) * 10) + (year  & 0x0F);
    print_int(month, white);
    print("/", white);
    print_int(day, white);
    print("/", white);
    print_int(year, white);
}
void cmos_read_time(cmos_time_t *time) {
    if (!time) return;
    outb(0x70, 0x0A);
    while(inb(0x71) & 0x80);
    time->second = bcd_to_binary(cmos_read_register(0x00));
    time->minute = bcd_to_binary(cmos_read_register(0x02));
    time->hour   = bcd_to_binary(cmos_read_register(0x04));
    time->day    = bcd_to_binary(cmos_read_register(0x07));
    time->month  = bcd_to_binary(cmos_read_register(0x08));
    time->year   = bcd_to_binary(cmos_read_register(0x09));
}
u64 cmos_get_unix_timestamp(void) {
    cmos_time_t time;
    cmos_read_time(&time);
    u64 timestamp = 946684800;
    timestamp += (u64)(time.year) * 365 * 24 * 3600;
    timestamp += (u64)(time.month - 1) * 30 * 24 * 3600;
    timestamp += (u64)(time.day - 1) * 24 * 3600;
    timestamp += (u64)time.hour * 3600;
    timestamp += (u64)time.minute * 60;
    timestamp += (u64)time.second;
    return timestamp;
}
void GetCMOSTime(void) {
    cmos_time_t time;
    cmos_read_time(&time);
    if (time.hour < 10) print("0", white);
    print_int(time.hour, white);
    print(":", white);
    if (time.minute < 10) print("0", white);
    print_int(time.minute, white);
    print(":", white);
    if (time.second < 10) print("0", white);
    print_int(time.second, white);
}