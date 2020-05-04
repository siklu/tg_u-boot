#include "siklu_load_device_configurations.h"
#include "common_config.h"
#include "common_fdt.h"
#include "definitions.h"

#include <common.h>
#include <stdlib.h>
#include <linux/err.h>
#include <net.h>

static 
void load_mac_from_fdt_prop(const void *prop, size_t prop_size, 
		u_char *mac)
{
	char mac_str[ARP_HLEN_ASCII + 1] = { 0 };

	strncpy(mac_str, prop, min_t(size_t, prop_size, sizeof(mac_str)));
	string_to_enetaddr(mac_str, mac);
}

/**
 * Convert one mac into @param number_of_macs ethaddr.
 * @param mac first mac.
 * @param number_of_macs number of mac allocated for the device.
 * @todo Add test for this function, especially for edge cases.
 */
void populate_ethaddr_env(u_char *mac, uint number_of_macs) {
	uint i;
	uint32_t three_last_bytes = 0;

	/* Convert mac[3..5] to a 32bit integer */
	three_last_bytes |= (uint)mac[5];
	three_last_bytes |= (uint)mac[4] << 8u;
	three_last_bytes |= (uint)mac[3] << 16u;
	
	for(i = 0; i < number_of_macs; ++i) {
		eth_env_set_enetaddr_by_index("eth", i, mac);

		three_last_bytes += 1;
		
		/* Convert mac[3..5] back to array. */
		mac[5] = (three_last_bytes & 0x0000ffu);
		mac[4] = (three_last_bytes & 0x00ff00u) >> 8u; 
		mac[3] = (three_last_bytes & 0xff0000u) >> 16u;
	}
}

/**
 * Load mac addresses from FTB to u-boot environment
 * @return 
 * @todo add tests.
 */
static int load_mac_addresses_to_env(u_char *fdt) {
	const void *prop;
	size_t prop_size;
	uint number_of_macs;
	unsigned char mac[ARP_HLEN];

	prop = siklu_fdt_getprop_string(fdt, "/", MAC_ADDRESS_PROP_NAME, &prop_size);
	if (IS_ERR(prop)) {
		return (int)PTR_ERR(prop);
	}

	load_mac_from_fdt_prop(prop, prop_size, mac);

	number_of_macs = siklu_fdt_getprop_u32_default(fdt, "/", 
			ALLOCATED_MAC_ADDRESSES, DEFAULT_ALLOCATED_MACS);
	
	populate_ethaddr_env(mac, number_of_macs);
	
	return 0;
}

int load_siklu_device_configurations() {
	u_char *fdt;
	
	fdt = siklu_read_fdt_from_mtd_part(CONFIG_SIKLU_CONFIG_MTD_PART);
	if (! fdt)
		return -1;

	if (load_mac_addresses_to_env(fdt))
		return -1;
	
	free(fdt);

	return 0;
}