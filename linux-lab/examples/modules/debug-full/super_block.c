#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <linux/blk_types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qwert");
MODULE_DESCRIPTION("A module to dump all super_blocks");

char file[128] = {0};

static void super_dump(struct super_block *sb, void *data)
{
    if (sb->s_bdev_file)
		d_path(&(sb->s_bdev_file->f_path), file, 128);
    else
		sprintf(file, "%s", "no_file");
    printk("%p, %s, %s, %s, %s, %s, %s\n", 
			sb, sb->s_root->d_iname, sb->s_type->name, sb->s_bdev == NULL ? "NULL" : sb->s_bdev->bd_device.init_name, file, sb->s_sysfs_name, sb->s_id);

    return;
}

static int __init superblock_dump_init(void)
{
    iterate_supers(super_dump, NULL);
    return 0;
}

static void __exit superblock_dump_exit(void)
{
    return;
}

module_init(superblock_dump_init);
module_exit(superblock_dump_exit);

