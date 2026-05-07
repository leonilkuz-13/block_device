#include <linux/module.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define DEVICE_CAPACITY 32768 // 16 MB

struct ram_device {
	sector_t capacity;
	u8 *memory_ptr;
	struct gendisk *gd;
	struct blk_mq_tag_set tag_set;
	struct request_queue *queue;
};

static int block_open(struct gendisk *gd, blk_mode_t mode)
{
	return 0;
}

static void block_release(struct gendisk *gd)
{
}

static blk_status_t queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct ram_device *dev = hctx->queue->queuedata;

	unsigned long offset = blk_rq_pos(rq) * SECTOR_SIZE;

	struct bio_vec biovec;
	struct req_iterator iter;
	blk_status_t status = BLK_STS_OK;

	blk_mq_start_request(rq);

	if (blk_rq_is_passthrough(rq)) {
		status = BLK_STS_IOERR;
		goto done;
	}

	if (offset + blk_rq_bytes(rq) > dev->capacity * SECTOR_SIZE) {
		status = BLK_STS_IOERR;
		goto done;
	}

	rq_for_each_segment(biovec, rq, iter) {
		size_t len = biovec.bv_len;
		void *buffer = kmap_local_page(biovec.bv_page) + biovec.bv_offset;

		if (rq_data_dir(rq) == WRITE)
			memcpy(dev->memory_ptr + offset, buffer, len);
		else
			memcpy(buffer, dev->memory_ptr + offset, len);

		kunmap_local(buffer);
		offset += len;
	}

done:
	blk_mq_end_request(rq, status);
	return BLK_STS_OK;
}

static const struct block_device_operations my_fops = {
	.owner = THIS_MODULE,
	.open = block_open,
	.release = block_release,
};

static const struct blk_mq_ops my_mq_ops = {
	.queue_rq = queue_rq,
};

static struct ram_device *device;

static int __init my_init(void)
{
	int error = 0;
	int major;

	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (device == NULL) {
		error = -ENOMEM;
		goto free_dev;
	}

	device->capacity = DEVICE_CAPACITY;
	device->memory_ptr = vmalloc(device->capacity * SECTOR_SIZE);

	if (device->memory_ptr == NULL) {
		error = -ENOMEM;
		goto free_dev;
	}

	device->tag_set.ops = &my_mq_ops;
	device->tag_set.nr_hw_queues = 1;
	device->tag_set.queue_depth = 32;
	device->tag_set.numa_node = NUMA_NO_NODE;
	device->tag_set.cmd_size = 0;
	device->tag_set.nr_maps = 1;
	device->tag_set.driver_data = device;

	error = blk_mq_alloc_tag_set(&device->tag_set);
	if (error) {
		pr_err("could not allocate memory for tag set\n");
		goto free_mem;
	}

	major = register_blkdev(0, "ram_block_device");
	if (major < 0) {
		error = major;
		goto free_tags;
	}

	device->gd = blk_mq_alloc_disk(&device->tag_set, NULL, device);
	if (IS_ERR(device->gd)) {
		error = PTR_ERR(device->gd);
		goto unregister_dev;
	}

	device->gd->major = major;
	device->gd->first_minor = 0;
	device->gd->minors = 1;
	device->gd->fops = &my_fops;
	snprintf(device->gd->disk_name, 32, "myramdisk");

	set_capacity(device->gd, device->capacity);

	error = add_disk(device->gd);
	if (error) {
		pr_err("myblk: failed to add disk\n");
		goto clean_disk;
	}

	pr_info("RAM block device successfully initialized!\n");
	return 0;

clean_disk:
	put_disk(device->gd);
unregister_dev:
	unregister_blkdev(major, "ram_block_device");
free_tags:
	blk_mq_free_tag_set(&device->tag_set);
free_mem:
	vfree(device->memory_ptr);
free_dev:
	kfree(device);
	return error;
}

static void __exit my_exit(void)
{
	int major = device->gd->major;

	del_gendisk(device->gd);
	put_disk(device->gd);

	unregister_blkdev(major, "ram_block_device");

	blk_mq_free_tag_set(&device->tag_set);
	vfree(device->memory_ptr);
	kfree(device);

	pr_info("RAM block device removed!\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leonid Kuzmischev");
MODULE_DESCRIPTION("RAM block device");
