#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("A simple example of kmem_cache usage");
MODULE_VERSION("0.1");

/*
 * insmod kmem_cache.ko
 * cat /proc/slabinfo
 */

#define NUM_OF_CACHE 128
#define NUM_OF_OBJECTS 256

struct kmem_test {
    struct kmem_cache *cache;
    void              *objects[NUM_OF_OBJECTS];
};

static struct kmem_test tests[NUM_OF_CACHE] = {0};
static int              kindex                = 0;
static int              oindex                = 0;

static void kmem_cache_create_by_name_size(char *name, size_t size) {
    int index = kindex++;

    // Create a memory cache for objects of size 128 bytes
    tests[index].cache = kmem_cache_create(name, size, 0, SLAB_HWCACHE_ALIGN, NULL);
    if (!tests[index].cache) {
        printk(KERN_ERR "kmem_cache_example: Failed to create cache\n");
        return;
    }

    // Allocate objects from the cache
    tests[index].objects[oindex++] = kmem_cache_alloc(tests[index].cache, GFP_KERNEL);
    tests[index].objects[oindex++] = kmem_cache_alloc(tests[index].cache, GFP_KERNEL);
    tests[index].objects[oindex++] = kmem_cache_alloc(tests[index].cache, GFP_KERNEL);

    return;
}

static int __init kmem_cache_example_init(void) {

    printk(KERN_INFO "kmem_cache_example: Initializing the module\n");

    kmem_cache_create_by_name_size("my_cache1", 64);
    kmem_cache_create_by_name_size("my_cache2", 128);
    kmem_cache_create_by_name_size("my_cache3", 256);
    kmem_cache_create_by_name_size("my_cache4", 512);
    kmem_cache_create_by_name_size("my_cache5", 1024);
    kmem_cache_create_by_name_size("my_cache6", 2048);
    kmem_cache_create_by_name_size("my_cache7", 4096);
    kmem_cache_create_by_name_size("my_cache8", 8192);

    return 0;
}

static void __exit kmem_cache_example_exit(void) {
    int i = 0;
    int j = 0;

    printk(KERN_INFO "kmem_cache_example: Exiting the module\n");

    for (i = 0; i < NUM_OF_CACHE; i++) {
        if (tests[i].cache) {
            for (j = 0; j < NUM_OF_OBJECTS; j++) {
                if (tests[i].objects[j])
                    kmem_cache_free(tests[i].cache, tests[i].objects[j]);
            }
            printk(KERN_INFO "kmem_cache_example: Destroying the cache\n");
            kmem_cache_destroy(tests[i].cache);
        }
    }

    return;
}

module_init(kmem_cache_example_init);
module_exit(kmem_cache_example_exit);
