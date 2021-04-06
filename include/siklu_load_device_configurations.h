#ifndef __SIKLU_DEVICE_CONFIGURATIONS_H__
#define __SIKLU_DEVICE_CONFIGURATIONS_H__

#include <config.h>

#ifdef CONFIG_SIKLU

/**
 * Load device-specific configuration to u-boot's environment.
 * @return <0 on error, 0 otherwise.
 */
int
load_siklu_device_configurations(void);

#endif

#endif
