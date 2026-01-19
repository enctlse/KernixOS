#include <config/boot.h>
#include <ui/theme/colors.h>
#include <string/string.h>
#include "module.h"

static struct component_handler *registered_handlers[COMPONENT_LIMIT];
static int handler_total = 0;

void initialize_component_system(void) {
    SYSTEM_PRINT("[SYS] ", gray_70);
    SYSTEM_PRINT("component management initialized\n", theme_white);
    for (int idx = 0; idx < COMPONENT_LIMIT; idx++) {
        registered_handlers[idx] = NULL;
    }
    handler_total = 0;
}

int enroll_component(struct component_handler *handler) {
    if (!handler || handler_total >= COMPONENT_LIMIT) {
        return -1;
    }
    for (int idx = 0; idx < handler_total; idx++) {
        if (registered_handlers[idx] == handler) {
            return -1;
        }
    }
    if (handler->startup) {
        int result = handler->startup();
        if (result != 0) {
            return result;
        }
    }
    registered_handlers[handler_total++] = handler;
    return 0;
}

void remove_component(const char *identifier) {
    if (!identifier) return;
    for (int idx = 0; idx < handler_total; idx++) {
        if (registered_handlers[idx] && registered_handlers[idx]->identifier) {
            const char *first = identifier;
            const char *second = registered_handlers[idx]->identifier;
            int identical = 1;
            while (*first && *second) {
                if (*first != *second) {
                    identical = 0;
                    break;
                }
                first++;
                second++;
            }
            if (identical && *first == '\0' && *second == '\0') {
                if (registered_handlers[idx]->shutdown) {
                    registered_handlers[idx]->shutdown();
                }
                for (int shift = idx; shift < handler_total - 1; shift++) {
                    registered_handlers[shift] = registered_handlers[shift + 1];
                }
                registered_handlers[handler_total - 1] = NULL;
                handler_total--;
                return;
            }
        }
    }
}

struct component_handler* locate_component(const char *identifier) {
    if (!identifier) return NULL;
    for (int idx = 0; idx < handler_total; idx++) {
        if (registered_handlers[idx] && registered_handlers[idx]->identifier) {
            const char *first = identifier;
            const char *second = registered_handlers[idx]->identifier;
            int identical = 1;
            while (*first && *second) {
                if (*first != *second) {
                    identical = 0;
                    break;
                }
                first++;
                second++;
            }
            if (identical && *first == '\0' && *second == '\0') {
                return registered_handlers[idx];
            }
        }
    }
    return NULL;
}

int count_components(void) {
    return handler_total;
}

struct component_handler* component_at_position(int position) {
    if (position < 0 || position >= handler_total) {
        return NULL;
    }
    return registered_handlers[position];
}