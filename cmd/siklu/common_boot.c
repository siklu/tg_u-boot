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

void fit_update_dtb_addr(void)
{
	void *fit_hdr = (void *) simple_strtoul(kernel_load_address(), NULL, 16);
	int fdt_offset;
	const void *fdt_data;
	size_t fdt_len;

	if (genimg_get_format(fit_hdr) != IMAGE_FORMAT_FIT)
		return;

	if (!fit_check_format(fit_hdr))
		return;

	fdt_offset = fit_image_get_node(fit_hdr, "fdt-marvell_armada-8040-n366.dtb");
	if (fdt_offset < 0)
		return;

	if (fit_image_get_data(fit_hdr, fdt_offset, &fdt_data, &fdt_len))
		return;

	env_set_hex("is_fit_image", 1UL);

	env_set_hex("fdt_addr_r", (unsigned long) fdt_data);
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

static char *boot_command(void)
{
	if (IS_ENABLED(CONFIG_ARM64))
		return "bootm";
	else
		return "bootz";
}

int load_kernel_image(void) {
	char buff[256];
	int ret;
	char formatted_bootargs[1024];
	const char *old_bootargs;
	char *boot_cmd_format;
	unsigned int fdt_addr;

	if (strict_strtoul(dtb_load_address(), 16, &fdt_addr) < 0)
		return -EINVAL;

	const char* fdt_param = siklu_fdt_getprop_string(fdt_addr, "/chosen", "bootargs", NULL);

	if (!IS_ERR(fdt_param)) {
		old_bootargs = env_get("bootargs");
		snprintf(formatted_bootargs, sizeof(formatted_bootargs), "%s %s", old_bootargs, fdt_param);
		printf("SIKLU BOOT: Added DTS-specific bootargs: %s\n", fdt_param);
		env_set("bootargs", formatted_bootargs);
	}

	boot_cmd_format = (env_get("is_fit_image") != NULL) ? "%s %s" :  "%s %s - %s";

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
