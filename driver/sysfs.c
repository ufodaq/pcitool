/**
 *
 * @file sysfs.c
 * @brief This file contains the functions providing the SysFS-interface.
 * @author Guillermo Marcus
 * @date 2010-03-01
 *
 */
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/kernel.h>

#include "base.h"

#define SYSFS_GET_PRIVDATA dev_get_drvdata(dev)
#define SYSFS_GET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, char *buf)
#define SYSFS_SET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)

#define SYSFS_ATTR_NAME(name) (dev_attr_##name)

#define SYSFS_ATTR_CREATE(name) do { \
	int err = device_create_file(privdata->class_dev, &SYSFS_ATTR_NAME(name)); \
	if (err != 0) return err; \
    } while (0)

#define SYSFS_ATTR_REMOVE(name) do { \
	device_remove_file(privdata->class_dev, &SYSFS_ATTR_NAME(name)); \
    } while (0)

#ifdef ENABLE_IRQ
static SYSFS_GET_FUNCTION(pcidriver_show_irq_count)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;

    return snprintf(buf, PAGE_SIZE, "%d\n", privdata->irq_count);
}

static SYSFS_GET_FUNCTION(pcidriver_show_irq_queues)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    int i, offset;

    /* output will be truncated to PAGE_SIZE */
    offset = snprintf(buf, PAGE_SIZE, "Queue\tOutstanding IRQs\n");
    for (i = 0; i < PCIDRIVER_INT_MAXSOURCES; i++)
        offset += snprintf(buf+offset, PAGE_SIZE-offset, "%d\t%d\n", i, atomic_read(&(privdata->irq_outstanding[i])) );

    return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}
#endif

static SYSFS_GET_FUNCTION(pcidriver_show_mmap_mode)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;

    return snprintf(buf, PAGE_SIZE, "%d\n", privdata->mmap_mode);
}

static SYSFS_SET_FUNCTION(pcidriver_store_mmap_mode)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    int mode = -1;

    /* Set the mmap-mode if it is either PCIDRIVER_MMAP_PCI or PCIDRIVER_MMAP_KMEM */
    if (sscanf(buf, "%d", &mode) == 1 &&
            (mode == PCIDRIVER_MMAP_PCI || mode == PCIDRIVER_MMAP_KMEM))
        privdata->mmap_mode = mode;

    return strlen(buf);
}

static SYSFS_GET_FUNCTION(pcidriver_show_mmap_area)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;

    return snprintf(buf, PAGE_SIZE, "%d\n", privdata->mmap_area);
}

static SYSFS_SET_FUNCTION(pcidriver_store_mmap_area)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    int temp = -1;

    sscanf(buf, "%d", &temp);

    if ((temp >= PCIDRIVER_BAR0) && (temp <= PCIDRIVER_BAR5))
        privdata->mmap_area = temp;

    return strlen(buf);
}

static SYSFS_GET_FUNCTION(pcidriver_show_kmem_count)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;

    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&(privdata->kmem_count)));
}

static SYSFS_SET_FUNCTION(pcidriver_store_kmem_alloc)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    kmem_handle_t kmem_handle;

    /* FIXME: guillermo: is validation of parsing an unsigned int enough? */
    if (sscanf(buf, "%lu", &kmem_handle.size) == 1)
        pcidriver_kmem_alloc(privdata, &kmem_handle);

    return strlen(buf);
}

static SYSFS_SET_FUNCTION(pcidriver_store_kmem_free)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    unsigned int id;
    pcidriver_kmem_entry_t *kmem_entry;

    /* Parse the ID of the kernel memory to be freed, check bounds */
    if (sscanf(buf, "%u", &id) != 1 ||
            (id >= atomic_read(&(privdata->kmem_count))))
        goto err;

    if ((kmem_entry = pcidriver_kmem_find_entry_id(privdata,id)) == NULL)
        goto err;

    pcidriver_kmem_free_entry(privdata, kmem_entry );
err:
    return strlen(buf);
}

static SYSFS_GET_FUNCTION(pcidriver_show_kbuffers)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    int offset = 0;
    struct list_head *ptr;
    pcidriver_kmem_entry_t *entry;

    /* print the header */
    offset += snprintf(buf, PAGE_SIZE, "kbuf#\tcpu addr\tsize\n");

    spin_lock(&(privdata->kmemlist_lock));
    list_for_each(ptr, &(privdata->kmem_list)) {
        entry = list_entry(ptr, pcidriver_kmem_entry_t, list);

        /* print entry info */
        if (offset > PAGE_SIZE) {
            spin_unlock( &(privdata->kmemlist_lock) );
            return PAGE_SIZE;
        }

        offset += snprintf(buf+offset, PAGE_SIZE-offset, "%3d\t%08lx\t%lu\n", entry->id, (unsigned long)(entry->dma_handle), entry->size );
    }

    spin_unlock(&(privdata->kmemlist_lock));

    /* output will be truncated to PAGE_SIZE */
    return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}

static SYSFS_GET_FUNCTION(pcidriver_show_umappings)
{
    int offset = 0;
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    struct list_head *ptr;
    pcidriver_umem_entry_t *entry;

    /* print the header */
    offset += snprintf(buf, PAGE_SIZE, "umap#\tn_pages\tsg_ents\n");

    spin_lock( &(privdata->umemlist_lock) );
    list_for_each( ptr, &(privdata->umem_list) ) {
        entry = list_entry(ptr, pcidriver_umem_entry_t, list );

        /* print entry info */
        if (offset > PAGE_SIZE) {
            spin_unlock( &(privdata->umemlist_lock) );
            return PAGE_SIZE;
        }

        offset += snprintf(buf+offset, PAGE_SIZE-offset, "%3d\t%lu\t%lu\n", entry->id,
                           (unsigned long)(entry->nr_pages), (unsigned long)(entry->nents));
    }

    spin_unlock( &(privdata->umemlist_lock) );

    /* output will be truncated to PAGE_SIZE */
    return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}

static SYSFS_SET_FUNCTION(pcidriver_store_umem_unmap)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;
    pcidriver_umem_entry_t *umem_entry;
    unsigned int id;

    if (sscanf(buf, "%u", &id) != 1 ||
            (id >= atomic_read(&(privdata->umem_count))))
        goto err;

    if ((umem_entry = pcidriver_umem_find_entry_id(privdata, id)) == NULL)
        goto err;

    pcidriver_umem_sgunmap(privdata, umem_entry);
err:
    return strlen(buf);
}

static SYSFS_GET_FUNCTION(pcidriver_show_kmem_entry)
{
    pcidriver_privdata_t *privdata = SYSFS_GET_PRIVDATA;

    /* As we can be sure that attr.name contains a filename which we
     * created (see _pcidriver_sysfs_initialize), we do not need to have
     * sanity checks but can directly call simple_strtol() */
    int id = simple_strtol(attr->attr.name + strlen("kbuf"), NULL, 10);
    pcidriver_kmem_entry_t *entry = pcidriver_kmem_find_entry_id(privdata, id);
    if (entry) {
        unsigned long addr = entry->cpua;
        unsigned long dma_addr = entry->dma_handle;

        if (entry->size >= 16) {
            pcidriver_kmem_sync_entry(privdata, entry, PCILIB_KMEM_SYNC_FROMDEVICE);
            return snprintf(buf, PAGE_SIZE, "buffer: %d\naddr: %lx\nhw addr: %llx\nbus addr: %lx\ntype: %lx\nuse: 0x%lx\nitem: %lu\nsize: %lu\nrefs: %lu\nhw ref: %i\nmode: 0x%lx\ndata: %8x %8x %8x %8x\n", id, addr, virt_to_phys((void*)addr), dma_addr, entry->type, entry->use, entry->item, entry->size, entry->refs&KMEM_REF_COUNT, (entry->refs&KMEM_REF_HW)?1:0, entry->mode, *(u32*)(entry->cpua), *(u32*)(entry->cpua + 4),  *(u32*)(entry->cpua + 8), *(u32*)(entry->cpua + 12));
        } else
            return snprintf(buf, PAGE_SIZE, "buffer: %d\naddr: %lx\nhw addr: %llx\nbus addr: %lx\ntype: %lx\nuse: 0x%lx\nitem: %lu\nsize: %lu\nrefs: %lu\nhw ref: %i\nmode: 0x%lx\n", id, addr, virt_to_phys((void*)addr), dma_addr, entry->type, entry->use, entry->item, entry->size, entry->refs&KMEM_REF_COUNT, (entry->refs&KMEM_REF_HW)?1:0, entry->mode);
    } else
        return snprintf(buf, PAGE_SIZE, "I am in the kmem_entry show function for buffer %d\n", id);
}

static SYSFS_GET_FUNCTION(pcidriver_show_umem_entry)
{
    return 0;
}


#ifdef ENABLE_IRQ
static DEVICE_ATTR(irq_count, S_IRUGO, pcidriver_show_irq_count, NULL);
static DEVICE_ATTR(irq_queues, S_IRUGO, pcidriver_show_irq_queues, NULL);
#endif

static DEVICE_ATTR(mmap_mode, 0664, pcidriver_show_mmap_mode, pcidriver_store_mmap_mode);
static DEVICE_ATTR(mmap_area, 0664, pcidriver_show_mmap_area, pcidriver_store_mmap_area);
static DEVICE_ATTR(kmem_count, 0444, pcidriver_show_kmem_count, NULL);
static DEVICE_ATTR(kbuffers, 0444, pcidriver_show_kbuffers, NULL);
static DEVICE_ATTR(kmem_alloc, 0220, NULL, pcidriver_store_kmem_alloc);
static DEVICE_ATTR(kmem_free, 0220, NULL, pcidriver_store_kmem_free);
static DEVICE_ATTR(umappings, 0444, pcidriver_show_umappings, NULL);
static DEVICE_ATTR(umem_unmap, 0220, NULL, pcidriver_store_umem_unmap);

int pcidriver_create_sysfs_attributes(pcidriver_privdata_t *privdata) {
#ifdef ENABLE_IRQ
    SYSFS_ATTR_CREATE(irq_count);
    SYSFS_ATTR_CREATE(irq_queues);
#endif

    SYSFS_ATTR_CREATE(mmap_mode);
    SYSFS_ATTR_CREATE(mmap_area);
    SYSFS_ATTR_CREATE(kmem_count);
    SYSFS_ATTR_CREATE(kmem_alloc);
    SYSFS_ATTR_CREATE(kmem_free);
    SYSFS_ATTR_CREATE(kbuffers);
    SYSFS_ATTR_CREATE(umappings);
    SYSFS_ATTR_CREATE(umem_unmap);

    return 0;
}

void pcidriver_remove_sysfs_attributes(pcidriver_privdata_t *privdata) {
#ifdef ENABLE_IRQ
    SYSFS_ATTR_REMOVE(irq_count);
    SYSFS_ATTR_REMOVE(irq_queues);
#endif

    SYSFS_ATTR_REMOVE(mmap_mode);
    SYSFS_ATTR_REMOVE(mmap_area);
    SYSFS_ATTR_REMOVE(kmem_count);
    SYSFS_ATTR_REMOVE(kmem_alloc);
    SYSFS_ATTR_REMOVE(kmem_free);
    SYSFS_ATTR_REMOVE(kbuffers);
    SYSFS_ATTR_REMOVE(umappings);
    SYSFS_ATTR_REMOVE(umem_unmap);
}

/**
 *
 * Removes the file from sysfs and frees the allocated (kstrdup()) memory.
 *
 */
void pcidriver_sysfs_remove(pcidriver_privdata_t *privdata, struct device_attribute *sysfs_attr)
{
    device_remove_file(privdata->class_dev, sysfs_attr);
    kfree(sysfs_attr->attr.name);
}

/**
 *
 * Initializes the sysfs attributes for an kmem/umem-entry
 *
 */
static int _pcidriver_sysfs_initialize(pcidriver_privdata_t *privdata, int id, struct device_attribute *sysfs_attr, const char *fmtstring, SYSFS_GET_FUNCTION((*callback)))
{
    /* sysfs attributes for kmem buffers don’t make sense before 2.6.13, as
       we have no mmap support before */
    char namebuffer[16];

    /* allocate space for the name of the attribute */
    snprintf(namebuffer, sizeof(namebuffer), fmtstring, id);

    if ((sysfs_attr->attr.name = kstrdup(namebuffer, GFP_KERNEL)) == NULL)
        return -ENOMEM;

    sysfs_attr->attr.mode = S_IRUGO;
    sysfs_attr->show = callback;
    sysfs_attr->store = NULL;

    /* name and add attribute */
    if (device_create_file(privdata->class_dev, sysfs_attr) != 0)
        return -ENXIO; /* Device not configured. Not the really best choice, but hm. */

    return 0;
}

int pcidriver_sysfs_initialize_kmem(pcidriver_privdata_t *privdata, int id, struct device_attribute *sysfs_attr)
{
    return _pcidriver_sysfs_initialize(privdata, id, sysfs_attr, "kbuf%d", pcidriver_show_kmem_entry);
}

int pcidriver_sysfs_initialize_umem(pcidriver_privdata_t *privdata, int id, struct device_attribute *sysfs_attr)
{
    return _pcidriver_sysfs_initialize(privdata, id, sysfs_attr, "umem%d", pcidriver_show_umem_entry);
}
