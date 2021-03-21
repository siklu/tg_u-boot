#ifndef __SIKLU_COMMON_FDT_H__
#define __SIKLU_COMMON_FDT_H__

#include <common.h>

/**
 * Get string property data from an FDT blob   
 * @param fdt fdt blob.
 * @param path path to the property. 
 * @param prop_name the actual property name.
 * @param string_len if not NULL, will be assigned the property size. 
 * @return IS_ERR() on error, pointer to the property data otherwise.
 * 
 * This function will return -EINVAL if the value is not a string.
 */
const char *
siklu_fdt_getprop_string(void *fdt, const char *path, const char *prop_name, size_t *string_len);

/**
 * Set a string property.
 * @param fdt fdt blob
 * @param path fdt path
 * @param prop_name fdt prop name
 * @param value property value. 
 * @return 0 on success.
 */
int
siklu_fdt_setprop_string(void *fdt, const char *path, const char *prop_name, const char *value);

/**
 * Convert a string property to integer.
 * @param fdt fdt blob
 * @param path fdt path
 * @param prop_name fdt prop name
 * @param default_value in case of failure, return this value.
 * @return stoul(value) on success, @param default_value othereise.
 */
u32 
siklu_fdt_getprop_u32_default(void *fdt, const char *path, const char *prop_name, u32 default_value);

#endif
