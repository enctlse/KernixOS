#include "graphics_manager.h"
int gworkspace_init(graphics_manager_t *graphics_manager, u8 *name, u64 pos) {
    if (!graphics_manager) return 1;
    if (graphics_manager->workspaces_len > graphics_manager->workspaces_total) return 1;
    gworkspace_t *workspace = (gworkspace_t *) graphics_manager_create(graphics_manager, sizeof(gworkspace_t));
    if (!workspace) return 1;
    int seek = 0;
    while (name[seek] != '\0' && seek < GWORKSPACE_NAME_MAX - 1) {
        workspace->name[seek] = name[seek];
        seek++;
    }
    workspace->name[seek] = '\0';
    workspace->pos_x = 0;
    workspace->pos_y = pos;
    workspace->gap_x = 10;
    workspace->sessions_total = GSESSION_LEN_MAX;
    workspace->session_active = 0;
    workspace->sessions = (gsession_t **) graphics_manager_alloc(graphics_manager, sizeof(gsession_t *), GSESSION_LEN_MAX);
    if (!workspace->sessions) {
        return 1;
    }
    workspace->graphics_manager = graphics_manager;
    graphics_manager->workspaces[graphics_manager->workspaces_len] = workspace;
    graphics_manager->workspace_active = graphics_manager->workspaces_len;
    graphics_manager->workspaces_len++;
    return 0;
}
gworkspace_t *gworkspace_get_name(graphics_manager_t *graphics_manager, u8 *workspace_name) {
    if (!graphics_manager) return NULL;
    for (int i = 0; i < graphics_manager->workspaces_total; i++) {
        gworkspace_t *ws = graphics_manager->workspaces[i];
        u8 *wsname = ws->name;
        int seek = 0;
        int eq = 1;
        while (workspace_name[seek] != '\0') {
            if (wsname[seek] == '\0') {
                eq = 0;
                break;
            }
            if (wsname[seek] != workspace_name[seek]) {
                eq = 0;
                break;
            }
            seek += 1;
        }
        if (eq) return ws;
    }
    return NULL;
}
gworkspace_t *gworkspace_get_pos(graphics_manager_t *graphics_manager, u64 pos) {
    if (!graphics_manager) return NULL;
    for (int i = 0; i < graphics_manager->workspaces_total; i++) {
        gworkspace_t *ws = graphics_manager->workspaces[i];
        if (ws->pos_y == pos) return ws;
    }
    return NULL;
}
int gworkspace_posx_get(gworkspace_t *gworkspace, u64 *out) {
    if (!gworkspace) return 1;
    if (gworkspace->sessions_len == 0) {
        *out = 0;
        return 0;
    }
    u64 list_len = gworkspace->sessions_len;
    u64 ret = 0;
    for (int i = 0; i < list_len; i++) {
        ret += gworkspace->sessions[i]->box.width + gworkspace->gap_x;
    }
    *out = ret;
    return 0;
}