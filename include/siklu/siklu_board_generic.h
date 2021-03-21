#ifndef __SIKLU_BOARD_GENERIC_H
#define __SIKLU_BOARD_GENERIC_H


/**
 * Get the board HW revision
 * @param out: HW revision 
 * @return <0 on error, 0 otherwise. 
 */
int 
siklu_get_hw_revision(u32 *hw_revsion);

/**
 * print siklu board HW revision
 */
void 
siklu_print_hw_revision (void);

#endif


/**
 * get cpu name. 
 * @param out: cpu_name 
 * @return <0 on error, 0 otherwise. 
 */
int 
siklu_get_cpu_name (const char **cpu_name);


/**
 * get cpu config register 
 * This is a register that holds general config attributes of the CPU (e.g, freq mode). it is needed for BIST
 * @param out: config_reg 
 * @return <0 on error, 0 otherwise. 
 */
int 
siklu_get_cpu_config_register(uint64_t *config_reg);

