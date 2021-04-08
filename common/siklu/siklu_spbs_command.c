/* Copyright 2020 by Siklu LTD. All rights reserved. */


#include <common.h>
#include <command.h>
#include <dm/device.h>
#include <asm/arch-qca-common/gpio.h>

typedef enum
{
	RESET_PRESSED,
	RESET_NOT_PRESSED,
	RESET_MAX_OPTIONS
} RESET_OPTION_TYPE;

const int CTU_SPBS_OFFSET = 71;

static RESET_OPTION_TYPE siklu_get_reset_status(void)
{
	RESET_OPTION_TYPE ret = RESET_MAX_OPTIONS;
	int gpio_no = CTU_SPBS_OFFSET;
	const char * failure = NULL;
	int reset_status;

	qca_gpio_init(gpio_no);

	reset_status = gpio_get_value(gpio_no);

	if (reset_status == 0)
		ret = RESET_PRESSED;
	else if(reset_status == 1)
		ret = RESET_NOT_PRESSED;
	else /*Expected -1 */
	{
		failure = "gpio_get_value";
	}

	qca_gpio_deinit(gpio_no);

	if(ret == RESET_MAX_OPTIONS)
		printf("Failed on %s", failure);
	return ret;
}

static int do_siklu_show_reset_button_status(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	RESET_OPTION_TYPE status = siklu_get_reset_status();

	int ret = CMD_RET_FAILURE;
	if (status == RESET_PRESSED)
	{
		printf("reset button pressed\n");
		ret = CMD_RET_SUCCESS;
	}
	else if(status == RESET_NOT_PRESSED)
	{
		printf("reset button NOT pressed\n");
		ret = CMD_RET_SUCCESS;
	}
	else
		printf("Unknown\n");
	return ret;
}

U_BOOT_CMD(spbs, 1, 0, do_siklu_show_reset_button_status,
	"Show reset button status, pressed or not pressed",
	"Show reset button status, pressed or not pressed"
);

