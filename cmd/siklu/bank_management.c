#include "bank_management.h"

#include <common.h>

#include "common_config.h"
#include "common_fdt.h"
#include "definitions.h"

static struct software_bank_t first_bank = {
		.bank_label = "first_bank",
};

static struct software_bank_t second_bank = {
		.bank_label = "second_bank",
};

/**
 * Create and write bank configuration to the flash.
 * @param bank bank to load from.
 */
static void
create_bank_management_info_for_bank(const struct software_bank_t *bank) {
	void *fdt;
	
	fdt = siklu_create_fdt_from_mtd_part(CONFIG_SIKLU_BANK_MGMT_MTD_PART);
	siklu_fdt_setprop_string(fdt, "/", PROP_CURRENT_BANK, bank->bank_label);
	
	siklu_write_fdt_to_mtd_part(CONFIG_SIKLU_BANK_MGMT_MTD_PART, fdt);
}

struct software_bank_t* bank_management_get_current_bank(void) {
	u_char *fdt = NULL;
	struct software_bank_t *bank;
	const char *config_bank_name;
	
	fdt = siklu_read_fdt_from_mtd_part(CONFIG_SIKLU_BANK_MGMT_MTD_PART);
	if (! fdt) {
		printk(KERN_ERR "Could not read bank management info from \"%s\"\n",
			   CONFIG_SIKLU_BANK_MGMT_MTD_PART);
		goto fail;
	}

	config_bank_name = siklu_fdt_getprop_string(fdt, "/", PROP_CURRENT_BANK, NULL);
	if (IS_ERR(config_bank_name)) {
		printf("Could not read current bank\n");
		goto fail_free;
	}
	
	if (strcmp(config_bank_name, first_bank.bank_label) == 0) {
		bank = &first_bank;
	} else if (strcmp(config_bank_name, second_bank.bank_label) == 0) {
		bank = &second_bank;
	} else {
		goto fail_free;
	}
	
	free(fdt);
	
	return bank;
	
fail_free:
	free(fdt);
fail:
	/**
	 * By default (e.g. if there were no banks defined)
	 * load from the first bank and fix the management info.
	 */
	bank = &first_bank;
	create_bank_management_info_for_bank(bank);
	
	return bank;
}

struct software_bank_t* bank_management_handle_auto_switch(struct software_bank_t *bank) {
	u_char *fdt = NULL;
	int8_t boot_tries_left = -1; // negative is disabled
	const char *config_boot_tries_left;
	bool do_fdt_write = false;

	fdt = siklu_read_fdt_from_mtd_part(CONFIG_SIKLU_BANK_MGMT_MTD_PART);
	if (! fdt) {
		printk(KERN_ERR "Could not read bank management info from \"%s\"\n",
			   CONFIG_SIKLU_BANK_MGMT_MTD_PART);
		return bank;
	}

	config_boot_tries_left = siklu_fdt_getprop_string(fdt, "/", PROP_BOOT_TRIES_LEFT, NULL);
	if (IS_ERR(config_boot_tries_left)) {
		printf("SIKLU_BOOT: Could not read boot_tries_left\n");
		return bank;
	}

	printf("SIKLU BOOT: boot_tries_left = %s\n", config_boot_tries_left);
	boot_tries_left = simple_strtol(config_boot_tries_left, NULL, 10);
	if (boot_tries_left < 0) {
		// do nothing
		return bank;
	} else if (boot_tries_left == 0) {
		// out of tries, switch bank
		if (bank != &first_bank && bank != &second_bank) {
			printf("SIKLU_BOOT: Could not switch bank, wrong bank specified\n");
		} else {
			bank = (bank == &first_bank) ? &second_bank : &first_bank;
			siklu_fdt_setprop_string(fdt, "/", PROP_CURRENT_BANK, bank->bank_label);
			do_fdt_write = true;
		}
	}

	// decrement boot_tries_left for next boot
	char boot_tries_left_str[5]; // "-123\0"
	if (0 < snprintf(boot_tries_left_str, sizeof(boot_tries_left_str), "%d", boot_tries_left - 1)) {
		siklu_fdt_setprop_string(fdt, "/", PROP_BOOT_TRIES_LEFT, boot_tries_left_str);
		do_fdt_write = true;
	}

	if (do_fdt_write) {
		siklu_write_fdt_to_mtd_part(CONFIG_SIKLU_BANK_MGMT_MTD_PART, fdt);
	}

	return bank;
}
