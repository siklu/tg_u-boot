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
	if (IS_ENABLED(CONFIG_ARM64))
		return BOOT_DIR "/Image";
	else
		return BOOT_DIR "/zImage";
}

char *kernel_fit_path(void) {
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
	if (is_fit_image())
		return "bootm";
  	else if	(IS_ENABLED(CONFIG_ARM64))
		return "booti";
	else
		return "bootz";
}

int load_kernel_image(void) {
	char buff[256];
	int ret;
	char formatted_bootargs[1024];
	const char *old_bootargs;
	unsigned int fdt_addr;
	char *fdt_file;
	char *vendor;

	if (is_fit_image()) {
		fdt_file = env_get("fdtfile");
		if (fdt_file == NULL) {
			printk(KERN_ERR "Could not find FDT file environment variable");
			return -EINVAL;
		}
		vendor = env_get("fdtvendor");
		if (vendor == NULL) {
			printk(KERN_ERR "Could not find FDT file environment variable");
			return -EINVAL;
		}
		/* Boot fitImage with the current conf */
		snprintf(buff, sizeof(buff), "%s %s#conf-%s_%s", boot_command(),
			kernel_load_address(), vendor, fdt_file);
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
