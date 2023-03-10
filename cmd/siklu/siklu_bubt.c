/*
 * Copyright 2023 by Siklu Ltd. All rights reserved.
 */

#include <common.h>
#include <version.h>
#include <command.h>
#include <string.h>

static int do_siklu_bubt(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *expected_siklu_ver;
	const char *file_name;
	const char *actual_siklu_ver;
	char cmd[256];

	if (argc != 3) {
		return CMD_RET_USAGE;
	}
	expected_siklu_ver = argv[1];
	file_name = argv[2];
	actual_siklu_ver = strrchr(version_string, ' ');
	if (!actual_siklu_ver) {
		printf("Invalid u-boot version string [%s]\n", version_string);
		return CMD_RET_FAILURE;
	}
	actual_siklu_ver++;
	if (*actual_siklu_ver == '\0') {
		printf("Invalid u-boot version string\n");
		return CMD_RET_FAILURE;
	}
	printf("actual_siklu_ver=[%s]\n", actual_siklu_ver);
	if (strcmp(actual_siklu_ver, expected_siklu_ver) == 0) {
		printf("u-boot upgrade is not required.\n");
		return CMD_RET_SUCCESS;
	}
	printf("u-boot upgrade is required.\n");
	snprintf(cmd, sizeof(cmd), "bubt %s spi tftp", file_name);
	return run_command(cmd, 0);
}

U_BOOT_CMD(
		siklu_bubt,
		3,
		0,
		do_siklu_bubt,
		"<expected-siklu-version> <file-name>\n",
		"If the actual u-boot version matches the expected\n"
		"siklu version of u-boot, this command will do nothing.\n"
		"Overwise, this command will load the u-boot image from\n"
		"tftp server and burn the u-boot image to the NOR flash\n"
		"and then reboot the device.\n"
		"Example:\n"
		"\tsiklu_bubt 1.0.0-21894-2b1d294a35 flash-image.bin\n"
);
