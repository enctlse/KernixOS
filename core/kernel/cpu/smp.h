#ifndef SMP_H
#define SMP_H
#include <outputs/types.h>
#include <kernel/include/defs.h>
#define MAX_CPUS 256
typedef struct {
    u32 apic_id;
    u32 processor_id;
    u8 enabled;
    u8 online;
} cpu_t;
extern cpu_t cpu_list[MAX_CPUS];
extern int cpu_count;
void smp_init(void);
void smp_start_aps(void);
int smp_get_cpu_count(void);
int smp_get_online_cpus(void);
void smp_send_ipi(u32 apic_id, u32 vector);
u32 smp_get_current_apic_id(void);
void ap_entry(struct limine_mp_info* info);
void ap_main(void);
#endif