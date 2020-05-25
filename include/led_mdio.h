/*
 * led_mdio.h
 *
 *  Created on: May 24, 2020
 *      Author: shmuels
 */

#ifndef INCLUDE_LED_MDIO_H_
#define INCLUDE_LED_MDIO_H_

struct udevice* led_mdio_create (const char* label, struct phy_device *phydev, int (*pre_write_hook) (struct phy_device* phydev),
									u16 devad, u16 regnum, u16 reg_mask, u16 val_on, u16 val_off);



#endif /* INCLUDE_LED_MDIO_H_ */
