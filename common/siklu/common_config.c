#include "common_config.h"

#include <common.h>
#include <dm/device.h>
#include <fdt_support.h>

#include "common_mtd.h"


/**
 * Make sure @param fdt is pointing to a valid fdt object.
 * @param fdt a pointer.
 * @return <0 on error, 0 otherwise. 
 */
static int 
validate_fdt(const char *fdt) {
	return fdt_check_header(fdt);
}

void* 
siklu_read_fdt_from_mtd_part(const char *mtd_part) {
	struct part_info *mtd;
	char *fdt_buff;
	
	mtd = siklu_get_part_by_label(mtd_part);
	if (mtd == NULL) {
		printk("Could not find partition \"%s\"\n", mtd_part);
		return NULL;
	}
	
	fdt_buff = siklu_read_from_mtd(mtd);
	if (fdt_buff == NULL)
		return NULL;
	
	if (validate_fdt(fdt_buff))
		goto err_free;

	fdt_set_totalsize(fdt_buff, mtd->size);
	
	return fdt_buff;

err_free:
	free(fdt_buff);
	return NULL;
}

int siklu_write_fdt_to_mtd_part(const char *mtd_part, const void *fdt) {
	struct part_info *mtd;

	mtd = siklu_get_part_by_label(mtd_part);
	if (mtd == NULL) {
		printk("Could not find partition \"%s\"\n", mtd_part);
		return -ENODEV;
	}
	
	return siklu_write_to_mtd(mtd, fdt, fdt_totalsize(fdt));
}

void *
siklu_create_fdt_from_mtd_part(const char *mtd_part) {
	void *fdt;
	struct part_info *mtd;
	int ret;

	mtd = siklu_get_part_by_label(mtd_part);
	if (mtd == NULL) {
		printk("Could not find partition \"%s\"\n", mtd_part);
		return NULL;
	}
	
	fdt = malloc(mtd->size);
	
	ret = fdt_create_empty_tree(fdt, mtd->size);
	if (ret) {
		printk("Failed to create an empty tree: %d\n", ret);\
		return NULL;
	}
	
	return fdt;
}

void *siklu_read_or_create_fdt_from_mtd_part(const char *mtd_part) {
	void *fdt;
	
	fdt = siklu_read_fdt_from_mtd_part(mtd_part);
	if (fdt)
		return fdt;
	
	return siklu_create_fdt_from_mtd_part(mtd_part);
}
