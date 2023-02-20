#include <common.h>

#include "common_boot.h"
#include "common_config.h"
#include "common_fdt.h"
#include <siklu/siklu_board_generic.h>
#include <linux/err.h>

#define BOOT_COMMAND "bootm"
#define BOOT_DIR "/boot"
#define KERNEL_PATH BOOT_DIR"/fitImage"
#define FTD_VENDOR "qcom"
#define FTD_FILE "ipq6018-siklu-ctu-100"
#define PROP_PRODUCT_SUBTYPE "SE_product_subtype"

void setup_bootargs(const char *bootargs) {
	static char formatted_bootargs[1024];
	const char *mtdparts;
	const char *old_bootargs;

	old_bootargs = getenv("bootargs");
	mtdparts = getenv("mtdparts");
	/* For IPQ60xx restore the default NAND only mtdparts */
	if (IS_ENABLED(CONFIG_ARCH_IPQ6018))
		mtdparts = MTDPARTS_DEFAULT;

	snprintf(formatted_bootargs, sizeof(formatted_bootargs), "%s %s %s",
			bootargs, old_bootargs ? old_bootargs : "",
			mtdparts ? mtdparts : "");
	setenv("bootargs", formatted_bootargs);
}

char *kernel_load_address(void)
{
	return getenv("kernel_addr_r");
}

char *kernel_path(void)
{
	return KERNEL_PATH;
}

char *dtb_load_address(void)
{
	char *env;

	env = getenv("fdt_addr_r");

	return env ? env : "0";
}

char *dtb_path(void)
{
	static char dtpath[128];

	snprintf(dtpath, sizeof(dtpath), BOOT_DIR "/%s", getenv("fdtfile"));

	return dtpath;
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
	static char buff[256];
	int ret;
	char *boot_cmd_format;
	const char *subtype;
	
	subtype = product_subtype();
	if (subtype) {
		boot_cmd_format = "%s %s#conf-%s_%s-%s.dtb";
		printf("SIKLU BOOT: Using product subtype %s\n", subtype);
	}
	else {
		boot_cmd_format = "%s %s#conf-%s_%s.dtb";
		printf("SIKLU BOOT: Using default product subtype\n");
	}
	snprintf(buff, sizeof(buff), boot_cmd_format, BOOT_COMMAND,
		kernel_load_address(), FTD_VENDOR, FTD_FILE, subtype);
	
	ret = run_command(buff, 0);
	
	/* If we are here, we could not run the command */
	if (ret) {
		printk(KERN_ERR "Could not load kernel from using command \"%s\"", 
				buff);
	}
	
	return ret;
}
