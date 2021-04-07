#include "common_config.h"
#include "common_fdt.h"

#include <common.h>
#include <malloc.h>

static int
do_siklu_device_config_get(u_char *fdt, const char *path, const char *prop_name, int argc, char *const argv[]) {
	const char *prop_data;
	size_t prop_len;

	if (argc != 0) {
		return CMD_RET_USAGE;
	}

	prop_data = siklu_fdt_getprop_string(fdt, path, prop_name, &prop_len);
	if (IS_ERR(prop_data)) {
		printf("ERROR: Could not get property \"%s/%s\": %d",
			   path, prop_name, (int) PTR_ERR(prop_data));
		return CMD_RET_FAILURE;
	}

	printf("%s\n", prop_data);
	return CMD_RET_SUCCESS;
}

static int
do_siklu_device_config_set(u_char *fdt, const char *path, const char *prop_name, int argc, char *const argv[]) {
	int ret;

	if (argc != 1) {
		return CMD_RET_USAGE;
	}

	/* TODO: support creating of new nodes
	 * 		(to complete the path, or just get rid of the path altogether) */
	ret = siklu_fdt_setprop_string(fdt, path, prop_name, argv[0]);
	if (ret < 0) {
		printf("Failed to set property: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	ret = siklu_write_fdt_to_mtd_part(CONFIG_SIKLU_CONFIG_MTD_PART,
			fdt);
	if (ret) {
		printf("Failed to write data to device: %d", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
static int do_siklu_device_config(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	u_char *fdt;
	int ret;
	const char *path;
	const char *prop_name;

	fdt = siklu_read_or_create_fdt_from_mtd_part(CONFIG_SIKLU_CONFIG_MTD_PART);
	if (! fdt) {
		printf("ERROR: Could not read current configuration\n");
		return CMD_RET_FAILURE;
	}

	if (argc < 4) {
		return CMD_RET_USAGE;
	}

	path = argv[2];
	prop_name = argv[3];

	if (strcmp(argv[1], "set") == 0) {
		ret = do_siklu_device_config_set(fdt, path, prop_name, argc - 4, &argv[4]);
	} else if (strcmp(argv[1], "get") == 0) {
		ret = do_siklu_device_config_get(fdt, path, prop_name, argc - 4, &argv[4]);
	} else {
		return CMD_RET_USAGE;
	}

	free(fdt);

	return ret;
}

U_BOOT_CMD(
		siklu_device_config,
		5,
		0,
		do_siklu_device_config,
		"siklu_device_config set <path> <prop> <value>\n"
		"siklu_device_config get <path> <prop>",
		"Set or get values from the Siklu config storage\n"
);
