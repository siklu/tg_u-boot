/* Copyright 2020 by Siklu LTD. All rights reserved. */


#include <common.h>
#include <command.h>
#include <dm/device.h>
#include <asm/gpio.h>

typedef enum
{
	RESET_PRESSED,
	RESET_NOT_PRESSED,
	RESET_MAX_OPTIONS
} RESET_OPTION_TYPE;

const char CTU_SPBS_NAME[] = "ap_gpio071";

static RESET_OPTION_TYPE siklu_get_reset_status(void)
{
	RESET_OPTION_TYPE ret = RESET_MAX_OPTIONS;
	unsigned gpio_no;
	const char * failure = NULL;
	int reset_status;
	const char * spbs_name = NULL;

	if(of_machine_is_compatible("siklu,n366"))
	{
		spbs_name = CTU_SPBS_NAME;
	}
	else
	{
		failure = "non-suported board";
		goto error;
	}

	if(gpio_lookup_name(spbs_name, NULL, NULL, &gpio_no))
	{
		failure = "gpio_lookup_name";
		goto error;
	}

	if(gpio_request(gpio_no, "spbs"))
	{
		failure = "gpio_request";
		goto error;
	}

	if(gpio_direction_input(gpio_no))
	{
		failure = "gpio_direction_input";
		goto error_free_gpio;
	}

	reset_status = gpio_get_value(gpio_no);

	if (reset_status == 0)
		ret = RESET_PRESSED;
	else if(reset_status == 1)
		ret = RESET_NOT_PRESSED;
	else /*Expected -1 */
	{
		failure = "gpio_get_value";
		goto error_free_gpio;
	}

error_free_gpio:
	gpio_free(gpio_no);
error:
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

