#ifndef __SIKLU_COMMON_MTD__
#define __SIKLU_COMMON_MTD__

#include <jffs2/load_kernel.h>

/**
 * Read data from an MTD partition.  
 * @param part the partition to read from.
 * @return NULL on failure, a pointer to the data on success.
 */
char*
siklu_read_from_mtd(struct part_info *part);

/**
 * Resolve mtd partition information by a string partition name.
 * @param mtd_part partition name to resolve.
 * @return NULL if the partition does not exist or other errors, a pointer to the partition's part_info otherwise.
 */
struct part_info*
siklu_get_part_by_label(const char *mtd_part);

int
siklu_write_to_mtd(struct part_info *part, const char *data, size_t data_size);

#endif