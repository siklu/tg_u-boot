#include <common.h>
#include <command.h>
#include <dm.h>
#include <ubi_uboot.h>
#include <ubifs_uboot.h>

#include "common_boot.h"
#include "bank_management.h"

static int init_and_mount_ubifs_bank(struct software_bank_t *bank) {
	int ret;
	
	/* First of all, select the right partition */
	ret = ubi_part((char *)bank->bank_label, NULL);
	if (ret) {
		printk(KERN_ERR "Failed to load %s\n", bank->bank_label);
		return ret;
	}

	/* Then, initialize the ubifs subsystem, otherwise, 
	 * we would end up getting memory errors */
	ret = ubifs_init();
	if (ret) {
		printk("Failed to initialize ubifs\n");
		return ret; 
	}
	
	/* Then, mount the rootfs volume */
	ret = uboot_ubifs_mount("ubi:" CONFIG_SIKLU_ROOTFS_VOLUME_NANE);
	if (ret) {
		printk(KERN_ERR "Failed to mount %s on bank %s\n",
			   CONFIG_SIKLU_ROOTFS_VOLUME_NANE, bank->bank_label);
		return ret;
	}
	
	return 0;
}

/**
 * Load Kernel and DeviceTree images from a mounted ubifs volume. 
 * @return 0 on success.
 */
static int load_from_ubifs(void) {
	int ret;
	u32 kernel_addr = simple_strtoul(kernel_load_address(), NULL, 16);
	u32 dtb_addr = simple_strtoul(dtb_load_address(), NULL, 16);
	
	ret = ubifs_load(kernel_path(), kernel_addr, 0);
	if (ret) {
		printk(KERN_ERR "Failed to load %s\n", kernel_path());
		return ret;
	}

	ret = ubifs_load(dtb_path(), dtb_addr, 0);
	if (ret) {
		printk(KERN_ERR "Failed to load %s\n", dtb_path());
		return ret;
	}
	
	return 0;
}

/**
 * Setup bootargs so the kernel will load fs from @param bank.
 * @param bank a valid software bank.
 */
static void set_bootargs_for_bank(struct software_bank_t *bank) {
	char bootargs[1024];
	
	snprintf(bootargs, sizeof(bootargs),
			 "ubi.mtd=%s root=ubi0:%s rootfstype=ubifs",
			 bank->bank_label,
			 CONFIG_SIKLU_ROOTFS_VOLUME_NANE);

	setup_bootargs(bootargs);
} 

static int do_nand_boot(cmd_tbl_t *cmdtp, int flag, int argc,
					  char *const argv[])
{
	struct software_bank_t *bank;
	int ret;

	bank = bank_management_get_current_bank();

	printk("Loading images from bank %s...\n", bank->bank_label);
	ret = init_and_mount_ubifs_bank(bank);
	if (ret) {
		return CMD_RET_FAILURE;
	}
	
	ret = load_from_ubifs();
	if (ret) {
		return CMD_RET_FAILURE;
	}
	
	set_bootargs_for_bank(bank);
	
	ret = load_kernel_image();
	if (ret) {
		printk(KERN_ERR "Failed to load kernel image from bank \"%s\"", 
				bank->bank_label);
	}
	
	/* Should not return */
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(
		siklu_nand_boot,
		1, 
		0, 
		do_nand_boot,
		"siklu_nand_boot",
		"Loads the system using NAND\n"
);
