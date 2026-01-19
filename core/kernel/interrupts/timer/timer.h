#ifndef TIMER_H
#define TIMER_H
#include <outputs/types.h>

#define TIMER_BASE_FREQUENCY 1193182
#define MAX_EVENT_SUBSCRIBERS 16

typedef struct {
    u64 id;
    void (*handler)(void* data);
    void* user_data;
    u64 interval_ticks;
    u64 next_trigger;
    int recurring;
} timer_event_t;

typedef struct {
    u64 total_ticks;
    u64 boot_time_ticks;
    timer_event_t subscribers[MAX_EVENT_SUBSCRIBERS];
    int subscriber_count;
} timer_system_t;

void timer_system_initialize(u32 target_frequency);
void timer_delay_ticks(u32 tick_count);
u64 timer_retrieve_current_ticks(void);
u64 timer_calculate_elapsed_seconds(void);
u64 timer_calculate_elapsed_milliseconds(void);
int timer_subscribe_event(void (*callback)(void*), void* data, u64 interval_ms, int repeat);
void timer_unsubscribe_event(int subscription_id);
void timer_record_boot_timestamp(void);
u64 timer_get_system_uptime_seconds(void);
void timer_display_uptime_information(void);
void timer_handle_interrupt(void);
#endif