/* Copyright 2020 by Siklu LTD. All rights reserved. */

#include <common.h>
#include <command.h>
#include <led.h>

extern void qca8075_phy_siklu_led_ctl(int led, int on);

/* led_state_t is from newer mainline U-Boot led.h */
enum led_state_t {
	LEDST_OFF = 0,
	LEDST_ON = 1,
	LEDST_TOGGLE,
#ifdef CONFIG_LED_BLINK
	LEDST_BLINK,
#endif

	LEDST_COUNT,
};

typedef enum
{
	SIKLU_LED_POWER,
	SIKLU_LED_WLAN,
	SIKLU_LED_ETH1,
	SIKLU_LED_MAX
} SIKLU_LED_TYPE;

typedef enum
{
	SIKLU_YELLOW_COLOR,
	SIKLU_GREEN_COLOR,
	SIKLU_COLOR_MAX
} SIKLU_LED_COLOR;

typedef enum
{
	LED_METHOD_GPIO,
	LED_METHOD_ETH_PHY,
} SIKLU_LED_METHOD;

typedef struct
{
	SIKLU_LED_METHOD method;
	union {
		unsigned int yellow_gpio;
		int yellow_id;
	};
	union {
		unsigned int green_gpio;
		int green_id;
	};
} siklu_dual_led_def_t;

static const char* const siklu_led_label_to_str[SIKLU_LED_MAX] =
{
	[SIKLU_LED_POWER]	= "power",
	[SIKLU_LED_WLAN]	= "wlan",
	[SIKLU_LED_ETH1]	= "eth1",
};


static int siklu_led_set_state(const siklu_dual_led_def_t *def, SIKLU_LED_COLOR color, enum led_state_t state) {
	unsigned int led_gpio;
	int value;

	value = (state == LEDST_OFF) ? 0 : 1;

	if ((state == LEDST_OFF) && (color == SIKLU_COLOR_MAX)) {
	// turn off all colors for this led
		for(int color_idx = 0; color_idx < SIKLU_COLOR_MAX; ++color_idx) {
			siklu_led_set_state(def, color_idx, LEDST_OFF);
		}
		return 0;
	}

	if (def->method == LED_METHOD_ETH_PHY) {	
		int led_id = color == SIKLU_GREEN_COLOR ? def->green_id
				: def->yellow_id;
		qca8075_phy_siklu_led_ctl(led_id, value);
		return 0;
	}

	led_gpio = (color == SIKLU_GREEN_COLOR ? def->green_gpio : def->yellow_gpio);
	gpio_direction_output(led_gpio, value);

	return 0;
}

static const siklu_dual_led_def_t siklu_led_ipq6010_ctu[] =
{
	[SIKLU_LED_POWER] = {
		.method = LED_METHOD_GPIO,
		.yellow_gpio = 23,
		.green_gpio  = 24,
	},
	[SIKLU_LED_WLAN] = {
		.method = LED_METHOD_GPIO,
		.yellow_gpio = 70,
		.green_gpio  = 73,
	},
	[SIKLU_LED_ETH1] = {
		.method = LED_METHOD_ETH_PHY,
		.yellow_id = 100,
		.green_id  = 1000,
	},
};

/*
 * gets a string of sled name and returns its position in the sled array
 */
static SIKLU_LED_TYPE sled_str_to_led_type(const char *sled)
{
	for (int i = 0; i < SIKLU_LED_MAX; i++)
	{
		if (strncmp(sled, siklu_led_label_to_str[i], strlen(sled)) == 0)
			return i;
	}
	return SIKLU_LED_MAX;
}

static enum led_state_t sled_status_to_led_status(const char* status)
{
	if (strcmp(status, "o") == 0)
		return LEDST_OFF;
	else
		return LEDST_ON;
}

static int sled_color_to_sled_pos(const char* color)
{
	if (strcmp(color, "y") == 0)
		return SIKLU_YELLOW_COLOR;
	else if (strcmp(color, "g") == 0)
		return SIKLU_GREEN_COLOR;
	else
		return SIKLU_COLOR_MAX;
}

/*
 * takes care of ALL command in siklu u-boot
 */
static int all_leds_color(SIKLU_LED_COLOR led_color)
{
	int i;

	for(i = 0; i < ARRAY_SIZE(siklu_led_ipq6010_ctu); ++i) {
		siklu_led_set_state(&siklu_led_ipq6010_ctu[i], led_color, LEDST_ON);
	}
	return 0;
}

static int turn_off_all_leds(void) {
	int i;

	for(i = 0; i < ARRAY_SIZE(siklu_led_ipq6010_ctu); ++i) {
			siklu_led_set_state(&siklu_led_ipq6010_ctu[i], SIKLU_COLOR_MAX, LEDST_OFF);
	}
	return 0;
}

static int do_led_control(cmd_tbl_t *cmdtp, int flag, int argc,
					  char *const argv[])
{
	enum led_state_t cmd;

	int ret, led_color;
	SIKLU_LED_TYPE led_type;

	/* Validate arguments */
	if (argc != 3)
		return CMD_RET_USAGE;

	cmd = sled_status_to_led_status(argv[2]);
	led_color = sled_color_to_sled_pos(argv[2]);
	if(led_color == SIKLU_COLOR_MAX && cmd != LEDST_OFF) {
		printf("Invalid led status: %s", argv[2]);
		return CMD_RET_USAGE;
	}

	if (strcmp(argv[1], "all") == 0) {		
		if(cmd == LEDST_ON) {
			all_leds_color(led_color);
		} else {
			turn_off_all_leds();
		}

		return 0;
	}

	led_type = sled_str_to_led_type(argv[1]);
	if(led_type >= ARRAY_SIZE(siklu_led_ipq6010_ctu)) {
		printf("Invalid led: %s\n", argv[1]);
		return CMD_RET_FAILURE;
	}
	
	ret = siklu_led_set_state(&siklu_led_ipq6010_ctu[led_type], led_color, cmd);
	if(ret) {
		printf("Failed to set led: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
		sled,
		3,
		0,
		do_led_control,
		"siklu led control",
		"Allows you to control the power, wlan and eth1 leds\n sled < power | wlan | eth1 | all > < o | g | y >"
);