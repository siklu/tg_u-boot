#ifndef __SIKLU_BANK_MANAGEMENT_H__
#define __SIKLU_BANK_MANAGEMENT_H__

struct software_bank_t {
	const char* bank_label;
};

/**
 * Get the current software bank.
 * @return a pointer to a valid software_bank_t.
 */
struct software_bank_t* bank_management_get_current_bank(void);

#endif