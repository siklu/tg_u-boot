#ifndef __SIKLU_COMMON_CONFIG_H__
#define __SIKLU_COMMON_CONFIG_H__

#include <linux/err.h>

/**
 * Load Flatten Device Tree from a NOR MTD partition.
 * @param mtd_part Name of the partition.
 * @return FDT blob, on success; NULL on error.
 */
void* 
siklu_read_fdt_from_mtd_part(const char *mtd_part); 

/**
 * Loads a FDT from a NOR MTD partition. If it is not valid, 
 * return an empty FDT in the size of the mtd partition.
 * @param mtd_part Name of the partition.
 * @return pointer to a valid FDT.
 */
void*
siklu_read_or_create_fdt_from_mtd_part(const char *mtd_part);

/**
 * Create an FDT in the size of an mtd_part.
 * @param mtd_part the mtd part name.
 * @return a pointer to the new FDT, or a NULL on failure.
 */
void *
siklu_create_fdt_from_mtd_part(const char *mtd_part);

/**
 * Write a Flatten Device Tree from Memory to a nor mtd partition. 
 * @param mtd_part Name of the partition to write to 
 * @param fdt FDT blob to write.
 * @return 0 on success -ERROR otherwise.
 */
int
siklu_write_fdt_to_mtd_part(const char *mtd_part, const void *fdt);

#endif