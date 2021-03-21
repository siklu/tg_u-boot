#ifndef __SIKLU_COMMON_BOOT_H__
#define __SIKLU_COMMON_BOOT_H__

/**
 * Like booti <image_address> - <dtb_address>
 * @return < 0 on failure, should not return on success.
 */
int load_kernel_image(void);

/**
 * Configure boot arguments for the next boot.
 * @param bootargs boot arguments string.
 */
void setup_bootargs(const char *bootargs);

/**
 * Kernel load address
 * @return hex string of kernel load address
 */
char *kernel_load_address(void);

/**
 * Kernel file path
 * @return path to kernel image location on filesystem
 */
char *kernel_path(void);

/**
 * Device tree load address
 * @return hex string of DT load address
 */
char *dtb_load_address(void);

/**
 * Device tree file path
 * @return path to dtb file location on filesystem
 */
char *dtb_path(void);

#endif
