#ifndef SMP_H
#define SMP_H
#include <types.h>
void smp_init(void);
void smp_start_aps(void);
int smp_get_cpu_count(void);
int smp_get_online_cpus(void);
void smp_send_ipi(u32 apic_id, u32 vector);
u32 smp_get_current_apic_id(void);
void ap_entry(void);
void ap_main(void);
#endif