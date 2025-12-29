#include "smp.h"
#include "apic.h"
void smp_init(void) {
    apic_init();
}
void smp_start_aps(void) {
}
u32 smp_get_current_apic_id(void) {
    return apic_get_id();
}