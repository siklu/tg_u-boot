#include <common.h>

#include "common_boot.h"
#include "common_fdt.h"
#include <siklu/siklu_board_generic.h>
#include <linux/err.h>

#define BOOT_DIR "/boot"

void setup_bootargs(const char *bootargs) {
	char formatted_bootargs[1024];
	const char *mtdparts;
	const char *old_bootargs;

	old_bootargs = getenv("bootargs");
	mtdparts = getenv("mtdparts");

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
	if (IS_ENABLED(CONFIG_ARCH_IPQ6018))
		return BOOT_DIR "/fitImage";
	else if (IS_ENABLED(CONFIG_ARM64))
		return BOOT_DIR "/Image";
	else
		return BOOT_DIR "/zImage";
}

char *dtb_load_address(void)
{
	return getenv("fdt_addr_r");
}

char *dtb_path(void)
{
	static char dtpath[128];

	snprintf(dtpath, sizeof(dtpath), BOOT_DIR "/%s", getenv("fdtfile"));

	return dtpath;
}

static char *boot_command(void)
{
	if (IS_ENABLED(CONFIG_ARCH_IPQ6018))
		return "bootm";
	else if (IS_ENABLED(CONFIG_ARM64))
		return "booti";
	else
		return "bootz";
}

int load_kernel_image(void) {
	char buff[256];
	int ret;
	char formatted_bootargs[1024];
	const char *old_bootargs;
	char *boot_cmd_format;
	unsigned long fdt_addr;

	if (strict_strtoul(dtb_load_address(), 16, &fdt_addr) < 0)
		return -EINVAL;

	const char* fdt_param = siklu_fdt_getprop_string((void *)fdt_addr, "/chosen",
			"bootargs", NULL);

	if (!IS_ERR(fdt_param)) {
		old_bootargs = getenv("bootargs");
		snprintf(formatted_bootargs, sizeof(formatted_bootargs), "%s %s", old_bootargs, fdt_param);
		printf("SIKLU BOOT: Added DTS-specific bootargs: %s\n", fdt_param);
		setenv("bootargs", formatted_bootargs);
	}

	if (IS_ENABLED(CONFIG_ARCH_IPQ6018))
		boot_cmd_format = "%s %s";
	else
		boot_cmd_format = "%s %s - %s";

	snprintf(buff, sizeof(buff), boot_cmd_format, boot_command(),
			kernel_load_address(), dtb_load_address());
	
	ret = run_command(buff, 0);
	
	/* If we are here, we could not run the command */
	if (ret) {
		printk(KERN_ERR "Could not load kernel from using command \"%s\"", 
				buff);
	}
	
	return ret;
}
