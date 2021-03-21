#include "common_mtd.h"

#include <common.h>
#include <dm/device.h>

#include <malloc.h>

struct part_info*
siklu_get_part_by_label(const char *mtd_part) {
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;

	/* Make sure the NOR device is probed */
	run_command("sf probe", 0);

	if (mtdparts_init() != 0) {
		printf("Error initializing mtdparts!\n");
		return NULL;
	}

	/* Find our mtd partition's number and device */
	if (find_dev_and_part(mtd_part, &dev, &pnum, &part)) {
		printf("Partition %s not found!\n", mtd_part);
		return NULL;
	}

	return part;
}

char*
siklu_read_from_mtd(struct part_info *part) {
	char *buffer;
	char cmd[1024];
	int ret;

	buffer = malloc(part->size);
	if (buffer == NULL) {
		printk("Could not allocate memory for reading\n");
		return NULL;
	}

	/**
	 * There is no better way to do it on u-boot unfortunately,
	 * using mtd_read will crash the system and generally does not look 
	 * very stable.
	 */
	snprintf(cmd, sizeof(cmd),
		 "sf read %p %s",
		 buffer,
		 part->name);
	ret = run_command(cmd, 0);
	if (ret) {
		printf("Failed to read flash using \"%s\"\n", cmd);
		goto err_free;
	}

	return buffer;

	err_free:
	free(buffer);

	return NULL;
}

int
siklu_write_to_mtd(struct part_info *part, const char *data, size_t data_size) {
	char cmd[1024];
	int ret;

	/**
	 * There is no better way to do it on u-boot unfortunately,
	 * using mtd_read will crash the system and generally does not look 
	 * very stable.
	 */
	snprintf(cmd, sizeof(cmd),
		 "sf update %p %s %x",
		 data,
		 part->name,
		 (uint)data_size);
	ret = run_command(cmd, 0);
	if (ret) {
		printf("Failed to read flash using \"%s\"\n", cmd);
		return -EIO;
	}

	return 0;
}
