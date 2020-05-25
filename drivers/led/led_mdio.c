/*
 * led_mdio.c
 *
 *  Created on: May 24, 2020
 *      Author: shmuels
 */


// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <led.h>
#include <malloc.h>
#include <phy.h>
#include <dm/lists.h>
#include <dm/device-internal.h>
#include <led_mdio.h>

struct led_mdio_priv {
	struct phy_device *phydev;
	int (*pre_write_hook) (struct phy_device* phydev);
	u16 devad;
	u16 regnum;
	u16 reg_mask;
	u16 val_on;
	u16 val_off;
};

static enum led_state_t mdio_led_get_state(struct udevice *dev)
{
	struct led_mdio_priv *priv = dev_get_priv(dev);
	int ret = phy_read(priv->phydev, priv->devad, priv->regnum);
	int val = (ret&priv->reg_mask);

	if (val == priv->val_off)
		return LEDST_OFF;
	else
		return LEDST_ON;
}

static int mdio_led_set_state(struct udevice *dev, enum led_state_t state)
{
	struct led_mdio_priv *priv = dev_get_priv(dev);
	int ret = phy_read(priv->phydev, priv->devad, priv->regnum);
	int regval;
	switch (state) {
	case LEDST_OFF:
		regval = priv->val_off;
		break;
	case LEDST_ON:
		regval = priv->val_on;
		break;
	case LEDST_TOGGLE:
		state = mdio_led_get_state(dev) == LEDST_OFF ? LEDST_ON : LEDST_OFF;
		return mdio_led_set_state(dev, state);
	default:
		return -ENOSYS;
	}
	if (priv->pre_write_hook != NULL)
		priv->pre_write_hook(priv->phydev);

	return phy_write(priv->phydev, priv->devad, priv->regnum, (ret&(~priv->reg_mask))|regval);
}

struct udevice* led_mdio_create (const char* label, struct phy_device *phydev, int (*pre_write_hook) (struct phy_device* phydev),
									u16 devad, u16 regnum, u16 reg_mask, u16 val_on, u16 val_off)
{
	struct led_mdio_priv *priv;
	struct udevice* dev;
	device_bind(phydev->dev, DM_GET_DRIVER(led_mdio), label, NULL, -1, &dev);
	priv = dev_get_priv(dev);
	priv->devad = devad;
	priv->pre_write_hook = pre_write_hook;
	priv->reg_mask = reg_mask;
	priv->val_off = val_off;
	priv->val_on = val_on;
	priv->regnum = regnum;
	priv->phydev = phydev;
	
	return dev;
}

static int led_mdio_bind(struct udevice *dev)
{
	struct led_uc_plat *uc_plat;

	uc_plat = dev_get_uclass_platdata(dev);
	uc_plat->label = dev->name;

	return 0;
}

static const struct led_ops mdio_led_ops = {
	.set_state	= mdio_led_set_state,
	.get_state	= mdio_led_get_state,
};


U_BOOT_DRIVER(led_mdio) = {
	.name	= "mdio_led",
	.id	= UCLASS_LED,
	.ops	= &mdio_led_ops,
	.priv_auto_alloc_size = sizeof(struct led_mdio_priv),
	.bind = led_mdio_bind,
};


