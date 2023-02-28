#include <common.h>

#include "common_boot.h"
#include "common_fdt.h"
#include "common_config.h"
#include <siklu/siklu_board_generic.h>
#include <linux/err.h>

#define BOOT_DIR "/boot"
#define VENDOR "marvell"
#define PROP_PRODUCT_SUBTYPE "SE_product_subtype"

void setup_bootargs(const char *bootargs) {
	char formatted_bootargs[1024];
	const char *mtdparts;
	const char *old_bootargs;
	struct siklu_board *board;
	const char *additional_board_bootargs;

	old_bootargs = env_get("bootargs");
	mtdparts = env_get("mtdparts");

	snprintf(formatted_bootargs, sizeof(formatted_bootargs), "%s %s %s",
			bootargs, old_bootargs ? old_bootargs : "",
			mtdparts ? mtdparts : "");
	env_set("bootargs", formatted_bootargs);
}

char *kernel_load_address(void)
{
	return env_get("kernel_addr_r");
}

char *kernel_path(void)
{
	return BOOT_DIR "/Image";
}

char *kernel_fit_path(void)
{
	return BOOT_DIR "/fitImage";
}

char *dtb_load_address(void)
{
	char *env = env_get("fdt_addr_r");

	return env ? env : "0";
}

char *dtb_path(void)
{
	static char dtpath[128];

	snprintf(dtpath, sizeof(dtpath), BOOT_DIR "/%s", env_get("fdtfile"));

	return dtpath;
}

static bool is_fit_image(void)
{
	char *env = env_get("is_fit_image");
	return env && simple_strtoul(env, NULL, 10);
}

void enable_fit_image(void)
{
	env_set_ulong("is_fit_image", 1UL);
}

void disable_fit_image()
{
	env_set_ulong("is_fit_image", 0UL);
}

static char *boot_command(void)
{
	return is_fit_image() ? "bootm" : "booti";
}

static const char *product_subtype(void) {
	void *siklu_device_config;
	const char *subtype;
	siklu_device_config = siklu_read_fdt_from_mtd_part(CONFIG_SIKLU_CONFIG_MTD_PART);
	if (!siklu_device_config) {
		printk(KERN_ERR "Could not read siklu device config from \"%s\"\n",
				CONFIG_SIKLU_BANK_MGMT_MTD_PART);
		return NULL;
	}
	subtype = siklu_fdt_getprop_string(siklu_device_config, "/", PROP_PRODUCT_SUBTYPE, NULL);
	if (IS_ERR(subtype)) {
		printf("SIKLU_BOOT: Could not read " PROP_PRODUCT_SUBTYPE "\n");
		subtype = NULL;
	}
	free(siklu_device_config);
	return subtype;
}

int load_kernel_image(void) {
	char buff[256];
	int ret;
	char formatted_bootargs[1024];
	const char *old_bootargs;
	unsigned int fdt_addr;
	char *fdt_file;
	const char *subtype;
	char *vendor;

	if (is_fit_image()) {
		subtype = product_subtype();
		if (subtype) {
			printf("SIKLU BOOT: Using product subtype %s\n", subtype);
			/* Boot fitImage with the default device tree and the subtype overlay by the format:
			bootm <kernel address>#conf-<vendor>_<dtb file name>#conf-<vendor>_<overlay file name> */
			snprintf(buff, sizeof(buff), "%s %s#conf-%s_%s.dtb#conf-%s_%s.dtbo", boot_command(),
				kernel_load_address(), VENDOR, CONFIG_DEFAULT_DEVICE_TREE, VENDOR, subtype);
		}
		else {
			printf("SIKLU BOOT: Using default product subtype\n");
			/* Boot fitImage with the default device tree by the format:
			bootm <kernel address>#conf-<vendor>_<FTD file name> */
			snprintf(buff, sizeof(buff), "%s %s#conf-%s_%s.dtb", boot_command(),
			kernel_load_address(), VENDOR, CONFIG_DEFAULT_DEVICE_TREE);
		}
	}
	else {
		snprintf(buff, sizeof(buff), "%s %s - %s", boot_command(),
			kernel_load_address(), dtb_load_address());
	}
	
	ret = run_command(buff, 0);
	
	/* If we are here, we could not run the command */
	if (ret) {
		printk(KERN_ERR "Could not load kernel from using command \"%s\"", 
				buff);
	}
	
	return ret;
}
