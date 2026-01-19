#include "timer.h"
#include <kernel/include/io.h>
#include "../handlers/irq.h"
#include <ui/theme/colors.h>
#include <string/string.h>
#include <drivers/cmos/cmos.h>
#include <config/boot.h>

static timer_system_t global_timer_system = {0};

void timer_handle_interrupt(void) {
    if (!global_timer_system.total_ticks) return; // Not initialized
    global_timer_system.total_ticks++;

    // Process event subscribers
    for (int i = 0; i < global_timer_system.subscriber_count; i++) {
        timer_event_t* event = &global_timer_system.subscribers[i];
        if (global_timer_system.total_ticks >= event->next_trigger) {
            if (event->handler) {
                event->handler(event->user_data);
            }
            if (event->recurring) {
                event->next_trigger = global_timer_system.total_ticks + event->interval_ticks;
            } else {
                // Remove one-time event
                for (int j = i; j < global_timer_system.subscriber_count - 1; j++) {
                    global_timer_system.subscribers[j] = global_timer_system.subscribers[j + 1];
                }
                global_timer_system.subscriber_count--;
                i--; // Adjust index
            }
        }
    }

    // Send EOI
    outb(0x20, 0x20);
}

void timer_system_initialize(u32 target_frequency) {
    if (target_frequency == 0 || target_frequency > TIMER_BASE_FREQUENCY) {
        target_frequency = 300; // Default
    }

    // Clear subscribers
    global_timer_system.subscriber_count = 0;
    global_timer_system.total_ticks = 0;
    global_timer_system.boot_time_ticks = 0;

    // No need to register here, handled in exception_handler.c

    // Configure PIT
    u32 divisor = TIMER_BASE_FREQUENCY / target_frequency;
    if (divisor < 1) divisor = 1;
    if (divisor > 65535) divisor = 65535;

    outb(0x43, 0x36);
    io_wait();
    outb(0x40, divisor & 0xFF);
    io_wait();
    outb(0x40, (divisor >> 8) & 0xFF);
    io_wait();

    global_timer_system.total_ticks = 1; // Mark as initialized
    __asm__ volatile("sti");

    SYSTEM_PRINT("[TIMER] System initialized at ", cyan);
    SYSTEM_PRINT_INT(target_frequency, yellow);
    SYSTEM_PRINT(" Hz\n", cyan);
}

void timer_delay_ticks(u32 tick_count) {
    if (!global_timer_system.total_ticks || tick_count == 0) return;
    u64 start = global_timer_system.total_ticks;
    while ((global_timer_system.total_ticks - start) < tick_count) {
        __asm__ volatile("hlt");
    }
}

u64 timer_retrieve_current_ticks(void) {
    return global_timer_system.total_ticks;
}

u64 timer_calculate_elapsed_seconds(void) {
    if (!global_timer_system.total_ticks) return 0;
    return global_timer_system.total_ticks / 300; // Assuming 300 Hz
}

u64 timer_calculate_elapsed_milliseconds(void) {
    if (!global_timer_system.total_ticks) return 0;
    return global_timer_system.total_ticks * 1000 / 300;
}

int timer_subscribe_event(void (*callback)(void*), void* data, u64 interval_ms, int repeat) {
    if (!callback || global_timer_system.subscriber_count >= MAX_EVENT_SUBSCRIBERS) {
        return -1;
    }

    timer_event_t* event = &global_timer_system.subscribers[global_timer_system.subscriber_count++];
    event->id = global_timer_system.subscriber_count; // Simple ID
    event->handler = callback;
    event->user_data = data;
    event->interval_ticks = (interval_ms * 300) / 1000; // Convert ms to ticks
    event->next_trigger = global_timer_system.total_ticks + event->interval_ticks;
    event->recurring = repeat;

    return (int)event->id;
}

void timer_unsubscribe_event(int subscription_id) {
    for (int i = 0; i < global_timer_system.subscriber_count; i++) {
        if ((int)global_timer_system.subscribers[i].id == subscription_id) {
            for (int j = i; j < global_timer_system.subscriber_count - 1; j++) {
                global_timer_system.subscribers[j] = global_timer_system.subscribers[j + 1];
            }
            global_timer_system.subscriber_count--;
            return;
        }
    }
}

void timer_record_boot_timestamp(void) {
    global_timer_system.boot_time_ticks = global_timer_system.total_ticks;
    SYSTEM_PRINT("[UPTIME] System tracking activated\n", green);
}

u64 timer_get_system_uptime_seconds(void) {
    if (!global_timer_system.total_ticks) return 0;
    u64 elapsed_ticks = global_timer_system.total_ticks - global_timer_system.boot_time_ticks;
    return elapsed_ticks / 300;
}

void timer_display_uptime_information(void) {
    u64 uptime = timer_get_system_uptime_seconds();
    u64 hours = uptime / 3600;
    u64 minutes = (uptime % 3600) / 60;
    u64 seconds = uptime % 60;
    char buffer[32];
    str_copy(buffer, "");
    if (hours < 10) str_append(buffer, "0");
    str_append_uint(buffer, (u32)hours);
    str_append(buffer, ":");
    if (minutes < 10) str_append(buffer, "0");
    str_append_uint(buffer, (u32)minutes);
    str_append(buffer, ":");
    if (seconds < 10) str_append(buffer, "0");
    str_append_uint(buffer, (u32)seconds);
    SYSTEM_PRINT(buffer, yellow);
}