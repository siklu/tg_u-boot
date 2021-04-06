#include "siklu_load_device_configurations.h"
#include "common_config.h"
#include "common_fdt.h"
#include "definitions.h"

#include <common.h>
#include <stdlib.h>
#include <linux/err.h>
#include <net.h>

/* Copied from net.h in newer mainline U-Boot */
#define ARP_HLEN_ASCII (ARP_HLEN * 2) + (ARP_HLEN - 1)

/* Copied from lib/net_utils.c in newer mainline U-Boot */
static
void string_to_enetaddr(const char *addr, uint8_t *enetaddr)
{
	char *end;
	int i;

	if (!enetaddr)
		return;

	for (i = 0; i < 6; ++i) {
		enetaddr[i] = addr ? simple_strtoul(addr, &end, 16) : 0;
		if (addr)
			addr = (*end) ? end + 1 : end;
	}
}

/* From cmd/nvedit.c in newer mainline U-Boot */
static int eth_env_get_enetaddr(const char *name, uint8_t *enetaddr)
{
	string_to_enetaddr(getenv(name), enetaddr);
	return is_valid_ethaddr(enetaddr);
}

/* From cmd/nvedit.c in newer mainline U-Boot */
static int eth_env_set_enetaddr(const char *name, const uint8_t *enetaddr)
{
	char buf[ARP_HLEN_ASCII + 1];

	if (eth_env_get_enetaddr(name, (uint8_t *)buf))
		return -EEXIST;

	sprintf(buf, "%pM", enetaddr);

	return setenv(name, buf);
}

/* From net/eth_common.c in newer mainline U-Boot */
static int eth_env_set_enetaddr_by_index(const char *base_name, int index,
		uchar *enetaddr)
{
	char enetvar[32];
	sprintf(enetvar, index ? "%s%daddr" : "%saddr", base_name, index);
	return eth_env_set_enetaddr(enetvar, enetaddr);
}

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

static int load_default_mac_addresses_to_env(void) {
	unsigned char mac[ARP_HLEN];

	printf("Warning: Using default mac address (%s)\n", CONFIG_SIKLU_DEFAULT_MAC_ADDRESS);

	load_mac_from_fdt_prop(CONFIG_SIKLU_DEFAULT_MAC_ADDRESS,
		sizeof(CONFIG_SIKLU_DEFAULT_MAC_ADDRESS),
		mac);

	populate_ethaddr_env(mac, DEFAULT_ALLOCATED_MACS);

	return 0;
}

int load_siklu_device_configurations() {
	u_char *fdt;
	int ret;

	fdt = siklu_read_fdt_from_mtd_part(CONFIG_SIKLU_CONFIG_MTD_PART);
	if (! fdt) {
		return load_default_mac_addresses_to_env();
	}

	ret = load_mac_addresses_to_env(fdt);
	if (ret) {
		ret = load_default_mac_addresses_to_env();
	}

	free(fdt);

	return ret;
}
