#ifndef __SIKLU_BANK_MANAGEMENT_H__
#define __SIKLU_BANK_MANAGEMENT_H__

#include <linux/types.h>

struct software_bank_t {
	const char* bank_label;
};

/**
 * Get the current software bank, update current bank if failover happened
 * @param do_failover  do a bank switch and save changes
 * @return a pointer to a valid software_bank_t.
 */
struct software_bank_t* bank_management_get_current_bank(bool do_failover);

/**
 * Get current boot_tries_left, decrease value for next boot
 * @return count of boot tries left or -1 on fail
 */
int8_t bank_management_get_boot_tries_left();

#endif
