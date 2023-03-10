/*
 * Copyright 2023 by Siklu Ltd. All rights reserved.
 */

#include <common.h>
#include <version.h>
#include <string.h>

#if 0

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
#endif

static int do_siklu_bubt(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *expected_siklu_ver;
	const char *file_name;
	const char *actual_siklu_ver;

	if (argc != 3) {
		return CMD_RET_USAGE;
	}
	expected_siklu_ver = argv[1];
	file_name = argv[2];

	printf("expected_siklu_ver=%s\n", expected_siklu_ver);
	printf("file_name=%s\n", file_name);
	printf("version_string=%s\n", version_string);

	actual_siklu_ver = strrchr(version_string, ' ');
	if (!actual_siklu_ver) {
		printf("Invalid u-boot version string [%s]\n", version_string);
		return CMD_RET_FAILURE;
	}
	actual_siklu_ver++;
	if (*actual_siklu_ver == '\0') {
		printf("Invalid actual u-boot version string\n");
		return CMD_RET_FAILURE;
	}
	printf("actual_siklu_ver=[%s]\n", actual_siklu_ver);

	return CMD_RET_SUCCESS;
#if 0
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "bubt %s spi tftp", file_name);
	return run_command(cmd, 0);
#endif
}

U_BOOT_CMD(
		siklu_bubt,
		3,
		0,
		do_siklu_bubt,
		"<expected-siklu-version> <file-name>\n",
		"If the actual u-boot version matches the expected\n"
		"siklu version of u-boot, this command will do nothing.\n"
		"Overwise, this command will load the u-boot image from"
		"tftp server and burn the u-boot image to the NOR flash\n"
		"and then reboot the device.\n"
		"Example:\n"
		"\tsiklu_bubt 1.0.0-21894-2b1d294a35 flash-image.bin\n"
);
