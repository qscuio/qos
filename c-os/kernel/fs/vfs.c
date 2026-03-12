#include <stddef.h>
#include <string.h>

#include "../init_state.h"
#include "../kernel.h"
#include "vfs.h"

static char g_paths[QOS_VFS_MAX_NODES][QOS_VFS_PATH_MAX];
static uint8_t g_used[QOS_VFS_MAX_NODES];
static uint32_t g_count = 0;

static int path_is_mount_or_child(const char *path, const char *mount_path) {
    size_t mount_len = strlen(mount_path);
    return strncmp(path, mount_path, mount_len) == 0 &&
           (path[mount_len] == '\0' || path[mount_len] == '/');
}

static int path_valid(const char *path) {
    size_t len;

    if (path == NULL || path[0] != '/') {
        return 0;
    }

    len = strlen(path);
    if (len == 0 || len >= QOS_VFS_PATH_MAX) {
        return 0;
    }

    return 1;
}

static int find_path(const char *path) {
    uint32_t i = 0;
    while (i < QOS_VFS_MAX_NODES) {
        if (g_used[i] != 0 && strcmp(g_paths[i], path) == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

static int find_free_slot(void) {
    uint32_t i = 0;
    while (i < QOS_VFS_MAX_NODES) {
        if (g_used[i] == 0) {
            return (int)i;
        }
        i++;
    }
    return -1;
}

void qos_vfs_reset(void) {
    memset(g_paths, 0, sizeof(g_paths));
    memset(g_used, 0, sizeof(g_used));
    g_count = 0;
}

int qos_vfs_create(const char *path) {
    int idx;

    if (!path_valid(path)) {
        return -1;
    }

    if (find_path(path) >= 0) {
        return -1;
    }

    idx = find_free_slot();
    if (idx < 0) {
        return -1;
    }

    strcpy(g_paths[(uint32_t)idx], path);
    g_used[(uint32_t)idx] = 1;
    g_count++;
    return 0;
}

int qos_vfs_exists(const char *path) {
    if (!path_valid(path)) {
        return 0;
    }
    return find_path(path) >= 0 ? 1 : 0;
}

int qos_vfs_remove(const char *path) {
    int idx;

    if (!path_valid(path)) {
        return -1;
    }

    idx = find_path(path);
    if (idx < 0) {
        return -1;
    }

    g_used[(uint32_t)idx] = 0;
    g_paths[(uint32_t)idx][0] = '\0';
    g_count--;
    return 0;
}

uint32_t qos_vfs_count(void) {
    return g_count;
}

int qos_vfs_fs_kind(const char *path) {
    if (!path_valid(path)) {
        return -1;
    }

    if (path_is_mount_or_child(path, "/tmp")) {
        return QOS_VFS_FS_TMPFS;
    }
    if (path_is_mount_or_child(path, "/proc")) {
        return QOS_VFS_FS_PROCFS;
    }
    if (path_is_mount_or_child(path, "/data")) {
        return QOS_VFS_FS_EXT2;
    }
    return QOS_VFS_FS_INITRAMFS;
}

int qos_vfs_is_read_only(const char *path) {
    int kind = qos_vfs_fs_kind(path);
    if (kind < 0) {
        return -1;
    }
    return kind == QOS_VFS_FS_INITRAMFS || kind == QOS_VFS_FS_PROCFS ? 1 : 0;
}

void vfs_init(void) {
    qos_vfs_reset();
    qos_kernel_state_mark(QOS_INIT_VFS);
}
