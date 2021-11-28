#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/ioctl.h>
#include <linux/errno.h>

#include "mybdev_ioctl.h"

static char *driver_name = "mybdev";
module_param(driver_name, charp, 0);

static int sect_size = 512;
module_param(sect_size, int, 0);

static int nr_sectors = 2 * 100 * 1024;
module_param(nr_sectors, int, 0);

static int rd_only = 0;
module_param(rd_only, int, 0);

#define MY_BLOCK_MINORS 1
#define KERNEL_SECTOR_SIZE 512

static int dev_major = 0;

struct my_block_dev 
{
        int size;
        u8 *data;
        struct blk_mq_tag_set tag_set;
        struct request_queue *queue;
        struct gendisk *gd;
};

static struct my_block_dev *myblock_dev = NULL;

static int mybdev_open(struct block_device *dev, fmode_t mode)
{
        printk(KERN_INFO "mybdev: device opened\n");
        return 0;
}

static void mybdev_release(struct gendisk *gd, fmode_t mode)
{
        printk(KERN_INFO "mybdev: device closed\n");
}

int mybdev_ioctl (struct block_device *bdev, fmode_t mode,
                  unsigned int cmd, unsigned long arg)
{
        switch (cmd) {
        case BLK_READ_ONLY:
                printk(KERN_INFO "mybdev: changing rd_only field\n");

                if (copy_from_user(&rd_only, (int *)arg, sizeof(int))) {
                        printk(KERN_ERR "mybdev: failed to get rd_only field\n");
                        return -EFAULT;
                }

                if ((rd_only != 0) && (rd_only != 1)) {
                        printk(KERN_ERR "mybdev: failed to get rd_only field(%d): set 0 or 1\n", rd_only);
                        return -EINVAL;
                }
                return 0;
        }

        return -ENOTTY;
}

static struct block_device_operations mybdev_fops = {
        .owner = THIS_MODULE,
        .open = mybdev_open,
        .release = mybdev_release,
        .ioctl = mybdev_ioctl,
};

static void mybdev_transfer(struct my_block_dev *dev, 
                            unsigned long sector, 
                            unsigned long nsect, 
                            char *buffer, 
                            int write)
{
        unsigned long offset = sector * KERNEL_SECTOR_SIZE;
        unsigned nbytes = nsect * KERNEL_SECTOR_SIZE;

        if ((offset + nbytes) > myblock_dev->size) {
                printk(KERN_ERR "mybdev: out of end write (%ld %d)\n", offset, nbytes);
                return;
        }

        if (write)
                memcpy(myblock_dev->data + offset, buffer, nbytes);
        else
                memcpy(buffer, myblock_dev->data + offset, nbytes);
}

static blk_status_t mybdev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
        struct request *req = bd->rq;
        struct bio_vec bvec;
        struct req_iterator iter;
        sector_t pos_sector = blk_rq_pos(req);
        void *buffer;
        blk_status_t ret;

        blk_mq_start_request(req);

        if (blk_rq_is_passthrough(req)) {
                printk(KERN_ERR "mybdev: Skip non-fs request\n");
                ret = BLK_STS_IOERR;
                goto out;
        }

        if ((rq_data_dir(req) == WRITE) && (rd_only == 1)) {
                printk(KERN_ERR "mybdev: write operation failed: permission denied\n");
                ret = BLK_STS_IOERR;
                goto out;
        }

        rq_for_each_segment(bvec, req, iter)
        {
                size_t num_sector = blk_rq_cur_sectors(req);
                printk(KERN_INFO "mybdev: dir %d sec %lld nr %ld\n", rq_data_dir(req), pos_sector, num_sector);
                buffer = page_address(bvec.bv_page) + bvec.bv_offset;
                mybdev_transfer(myblock_dev, pos_sector, num_sector, buffer, rq_data_dir(req) == WRITE);
                pos_sector += num_sector;
        }
        ret = BLK_STS_OK;

        printk(KERN_INFO "mybdev: request has been handled\n");
out:
        blk_mq_end_request(req, ret);

        return ret;
}

static struct blk_mq_ops mq_ops = {
    .queue_rq = mybdev_request,
};

static int __init mybdev_init(void)
{
        int status = 0;

        // register device
        dev_major = register_blkdev(dev_major, driver_name);

        if (dev_major < 0) {
                printk(KERN_ERR "mybdev: failed to register mubdev block device\n");

                return -EBUSY;
        }

        printk(KERN_INFO "mybdev: device has been registered\n");

        //myblock_dev alloc
        myblock_dev = kmalloc(sizeof(struct my_block_dev), GFP_KERNEL);

        if (myblock_dev == NULL) {
                printk("mybdev: failed to allocate struct myblock_dev\n");
                unregister_blkdev(dev_major, driver_name);

                return -ENOMEM;
        }
        printk(KERN_INFO "mybdev: myblock_dev allocated\n");

        // get memory
        myblock_dev->size = nr_sectors * sect_size;
        myblock_dev->data = vmalloc(myblock_dev->size);

        if (myblock_dev->data == NULL) {
                printk(KERN_ERR "mybdev: vmalloc failed\n");
                unregister_blkdev(dev_major, driver_name);
                kfree(myblock_dev);

                return -ENOMEM;
        }
        printk(KERN_INFO "mybdev: vmalloc: memory has been allocated\n");

        // initialize tag set
//         myblock_dev->tag_set.ops = &mq_ops;
//         myblock_dev->tag_set.nr_hw_queues = 1;
//         myblock_dev->tag_set.queue_depth = 128;
//         myblock_dev->tag_set.numa_node = NUMA_NO_NODE;
//         myblock_dev->tag_set.cmd_size = 0;
//         myblock_dev->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
// 
//         status = blk_mq_alloc_tag_set(&myblock_dev->tag_set);
// 
//         if (status) {
//                 printk(KERN_ERR "mybdev: failed to allocate tag set\n");
//                 printk(KERN_ERR "%d\n", status);
//                 unregister_blkdev(dev_major, driver_name);
//                 vfree(myblock_dev->data);
//                 kfree(myblock_dev);
// 
//                 return -ENOMEM;
//         }
//         printk(KERN_INFO "mybdev: tag set has been allocated\n");

        // initialize queue
        myblock_dev->queue = blk_mq_init_sq_queue(&myblock_dev->tag_set, &mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);
        if (IS_ERR(myblock_dev->queue)) {
                printk(KERN_ERR "mybdev: failed to allocate queue\n");
                unregister_blkdev(dev_major, driver_name);
                //blk_mq_free_tag_set(&myblock_dev->tag_set);
                vfree(myblock_dev->data);
                kfree(myblock_dev);

                return -ENOMEM;
        }

        blk_queue_logical_block_size(myblock_dev->queue, sect_size);
        myblock_dev->queue->queuedata = myblock_dev;

        // initialize the gendisk structure
        myblock_dev->gd = alloc_disk(MY_BLOCK_MINORS);
        if (!myblock_dev->gd) {
                printk(KERN_ERR "mybdev: failed to allocate disk\n");
                //blk_mq_free_tag_set(&myblock_dev->tag_set);
                blk_cleanup_queue(myblock_dev->queue);
                unregister_blkdev(dev_major, driver_name);
                vfree(myblock_dev->data);
                kfree(myblock_dev);

                return -ENOMEM;
        }
        printk(KERN_INFO "mybdev: disk has been allocated\n");

        myblock_dev->gd->major = dev_major;
        myblock_dev->gd->first_minor = 0;
        myblock_dev->gd->fops = &mybdev_fops;
        myblock_dev->gd->queue = myblock_dev->queue;
        myblock_dev->gd->private_data = myblock_dev;
        snprintf(myblock_dev->gd->disk_name, 32, driver_name);
        set_capacity(myblock_dev->gd, nr_sectors*(sect_size/KERNEL_SECTOR_SIZE));

        add_disk(myblock_dev->gd);

        printk(KERN_INFO "mybdev: disk has been added\n");

        return 0;
}

static void __exit mybdev_cleanup(void)
{
        if (myblock_dev->gd) {
            del_gendisk(myblock_dev->gd);
            put_disk(myblock_dev->gd);
        }

        //blk_mq_free_tag_set(&myblock_dev->tag_set);
        blk_cleanup_queue(myblock_dev->queue);
        unregister_blkdev(dev_major, driver_name);
        vfree(myblock_dev->data);
        kfree(myblock_dev);

        printk(KERN_INFO "mybdev: module unloaded\n");
}

module_init(mybdev_init);
module_exit(mybdev_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Fedorov");
