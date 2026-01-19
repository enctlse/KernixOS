#ifndef COMPONENT_HANDLER_H
#define COMPONENT_HANDLER_H
#include <outputs/types.h>
#define COMPONENT_LIMIT 256
#define BUILD_VERSION(maj, min, pat, bld) \
    (((maj) << 24) | ((min) << 16) | ((pat) << 8) | (bld))
struct component_handler {
    const char *identifier;
    const char *attachment_point;
    u32 build_number;
    int (*startup)(void);
    void (*shutdown)(void);
    void *(*access)(const char *location);
    int (*retrieve)(void *resource, void *destination, size_t amount);
    int (*store)(void *resource, const void *source, size_t amount);
};
void initialize_component_system(void);
int enroll_component(struct component_handler *handler);
void remove_component(const char *identifier);
struct component_handler* locate_component(const char *identifier);
int count_components(void);
struct component_handler* component_at_position(int position);
#endif