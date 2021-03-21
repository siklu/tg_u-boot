#include "common_fdt.h"

#include <linux/err.h>
#include <fdt_support.h>

const void*
fdt_getprop_by_path(void *fdt, const char *path, const char *prop_name, size_t *prop_len) {
	int offset;
	const void *prop_data;
	int len_or_error;

	offset = fdt_path_offset(fdt, path);
	if (offset < 0) {
		return ERR_PTR(offset);
	}

	prop_data = fdt_getprop(fdt, offset, prop_name, &len_or_error);
	if (! prop_data) {
		return ERR_PTR(len_or_error);
	}

	if (prop_len)
		*prop_len = (size_t) len_or_error;

	return prop_data;
}

const char *
siklu_fdt_getprop_string(void *fdt, const char *path, const char *prop_name, size_t *string_len) {
	size_t prop_length;
	const char *prop_data;

	prop_data = fdt_getprop_by_path(fdt, path, prop_name, &prop_length);
	if (IS_ERR(prop_data)) {
		return prop_data;
	}

	if (prop_data[prop_length-1] != '\0') {
		return ERR_PTR(-EINVAL);
	}

	if (string_len)
		*string_len = prop_length - 1;

	return prop_data;
}

int siklu_fdt_setprop_string(void *fdt, const char *path, const char *prop_name, const char *value) {
	int offset = fdt_path_offset(fdt, path);

	if (offset < 0)
		return offset;

	return fdt_setprop_string(fdt, offset, prop_name, value);
}

u32 siklu_fdt_getprop_u32_default(void *fdt, const char *path, const char *prop_name, u32 default_value) {
	const char *string_value;
	u32 value;
	char *endp;

	/* Get prop as string */
	string_value = siklu_fdt_getprop_string(fdt, path, prop_name, NULL);
	if (IS_ERR(string_value)) {
		return default_value;
	}

	/* Parse string to integer */
	value = simple_strtoul(string_value, &endp, 0);
	if (string_value == endp) {
		/* We could not parse the string, return the default */
		printf("Warning: Could not parse string value \"%s\" to integer,"
			   "returning default value %d\n",
			   string_value,
			   default_value);
		return default_value;
	}

	return value;
}