#include "uno.h"
#include <kernel/graph/graphics.h>
#include <drivers/cmos/cmos.h>
#include <string/string.h>
#include <kernel/exceptions/timer.h>
#include <kernel/graph/theme.h>
static u32 banner_y = 0;
static u32 banner_y_s = BANNER_Y_SPACING;
static u8 last_second = 0;
static u8 needs_update = 1;
static u32 current_banner_height = BANNER_HEIGHT;
static void banner_timer_callback(void);
void banner_init(void)
{
}
void banner_draw(void)
{
}
void banner_update_time(void)
{
}
static void banner_timer_callback(void)
{
}
void banner_tick(void) {
}
void banner_force_update(void) {
}
u32 banner_get_height(void) {
    return 0;
}