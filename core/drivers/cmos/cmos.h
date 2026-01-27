#ifndef CMOS_H
#define CMOS_H
#include <outputs/types.h>
typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u8 year;     
} cmos_time_t;
void cmos_read_time(cmos_time_t *time);
u64 cmos_get_unix_timestamp(void);
void GetCMOSTime(void);
USHORT GetCMOSMem(void);
void GetCMOSDate(void);
#endif