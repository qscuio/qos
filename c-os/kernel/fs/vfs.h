#ifndef QOS_VFS_H
#define QOS_VFS_H

#include <stdint.h>

#define QOS_VFS_MAX_NODES 128U
#define QOS_VFS_PATH_MAX 128U

enum {
    QOS_VFS_FS_INITRAMFS = 1u,
    QOS_VFS_FS_TMPFS = 2u,
    QOS_VFS_FS_PROCFS = 3u,
    QOS_VFS_FS_EXT2 = 4u,
};

void qos_vfs_reset(void);
int qos_vfs_create(const char *path);
int qos_vfs_exists(const char *path);
int qos_vfs_remove(const char *path);
uint32_t qos_vfs_count(void);
int qos_vfs_fs_kind(const char *path);
int qos_vfs_is_read_only(const char *path);

void vfs_init(void);

#endif
