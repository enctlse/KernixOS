#include "timer.h"
#include "irq.h"
#include <kernel/include/ports.h>
#include <ui/theme/colors.h>
#include <string/string.h>
#include <drivers/cmos/cmos.h>
#include <kernel/graph/theme.h>
#include <config/boot.h>
static volatile u64 timer_ticks = 0;
static volatile int timer_initialized = 0;
static u64 boot_timestamp = 0;
static timer_callback_t timer_callbacks[MAX_TIMER_CALLBACKS];
static int callback_count = 0;
void timer_handler(cpu_state_t* state)
{
    (void)state;
    if (!timer_initialized) return;
    timer_ticks++;
    for (int i = 0; i < callback_count; i++) {
        if (timer_callbacks[i]) timer_callbacks[i]();
    }
}
void timer_init(u32 frequency)
{
    if (frequency == 0 || frequency > 1193182) frequency = TIMER_FREQUENCY;
    for (int i = 0; i < MAX_TIMER_CALLBACKS; i++) timer_callbacks[i] = NULL;
    callback_count = 0;
    irq_register_handler(0, timer_handler);
    u32 divisor = 1193182 / frequency;
    if (divisor < 1) divisor = 1;
    if (divisor > 65535) divisor = 65535;
    outb(0x43, 0x36);
    io_wait();
    outb(0x40, divisor & 0xFF);
    io_wait();
    outb(0x40, (divisor >> 8) & 0xFF);
    io_wait();
    timer_ticks = 0;
    timer_initialized = 1;
    __asm__ volatile("sti");
    BOOTUP_PRINT("[TIMER] Initialized at ", cyan);
    BOOTUP_PRINT_INT(frequency, yellow);
    BOOTUP_PRINT(" Hz\n", cyan);
}
int timer_register_callback(timer_callback_t callback)
{
    if (!callback || callback_count >= MAX_TIMER_CALLBACKS) {
        return -1;
    }
    for (int i = 0; i < callback_count; i++) {
        if (timer_callbacks[i] == callback) {
            return -1;
        }
    }
    timer_callbacks[callback_count++] = callback;
    return 0;
}
void timer_unregister_callback(timer_callback_t callback)
{
    if (!callback) return;
    for (int i = 0; i < callback_count; i++) {
        if (timer_callbacks[i] == callback) {
            for (int j = i; j < callback_count - 1; j++) {
                timer_callbacks[j] = timer_callbacks[j + 1];
            }
            timer_callbacks[callback_count - 1] = NULL;
            callback_count--;
            return;
        }
    }
}
void timer_wait(u32 ticks)
{
    if (!timer_initialized || ticks == 0) {
        return;
    }
    u64 start = timer_ticks;
    while ((timer_ticks - start) < ticks) {
        __asm__ volatile("hlt");
    }
}
u64 timer_get_ticks(void)
{
    return timer_ticks;
}
u64 timer_get_seconds(void)
{
    if (!timer_initialized) {
        return 0;
    }
    return timer_ticks / TIMER_FREQUENCY;
}
u64 timer_get_milliseconds(void)
{
    if (!timer_initialized) {
        return 0;
    }
    return timer_ticks;
}
void timer_print_uptime(void)
{
    u64 uptime = timer_get_uptime_seconds();
    u64 hours = uptime / 3600;
    u64 minutes = (uptime % 3600) / 60;
    u64 seconds = uptime % 60;
    char buf[16];
    str_copy(buf, "");
    if (hours < 10) str_append(buf, "0");
    str_append_uint(buf, (u32)hours);
    str_append(buf, ":");
    if (minutes < 10) str_append(buf, "0");
    str_append_uint(buf, (u32)minutes);
    str_append(buf, ":");
    if (seconds < 10) str_append(buf, "0");
    str_append_uint(buf, (u32)seconds);
    BOOTUP_PRINT(buf, yellow);
}
u64 timer_get_uptime_seconds(void)
{
    if (!timer_initialized) return 0;
    u64 current_ticks = timer_ticks - boot_timestamp;
    return current_ticks / TIMER_FREQUENCY;
}
void timer_set_boot_time(void) {
    boot_timestamp = timer_ticks;
    BOOTUP_PRINT("[UPTIME] Tracking started\n", green);
}