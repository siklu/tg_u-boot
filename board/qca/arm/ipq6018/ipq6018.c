/*
 * Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <environment.h>
#include <asm/arch-qca-common/qpic_nand.h>
#include <asm/arch-qca-common/gpio.h>
#include <asm/arch-qca-common/uart.h>
#include <ipq6018.h>
#include <mmc.h>
#include <sdhci.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;
struct sdhci_host mmc_host;

const char *rsvd_node = "/reserved-memory";
const char *del_node[] = {"uboot",
			  "sbl",
			  NULL};
const add_node_t add_fdt_node[] = {{}};

void uart2_configure_mux(void)
{
	unsigned long cfg_rcgr;

	cfg_rcgr = readl(GCC_BLSP1_UART2_APPS_CFG_RCGR);
	/* Clear mode, src sel, src div */
	cfg_rcgr &= ~(GCC_UART_CFG_RCGR_MODE_MASK |
			GCC_UART_CFG_RCGR_SRCSEL_MASK |
			GCC_UART_CFG_RCGR_SRCDIV_MASK);

	cfg_rcgr |= ((UART2_RCGR_SRC_SEL << GCC_UART_CFG_RCGR_SRCSEL_SHIFT)
			& GCC_UART_CFG_RCGR_SRCSEL_MASK);

	cfg_rcgr |= ((UART2_RCGR_SRC_DIV << GCC_UART_CFG_RCGR_SRCDIV_SHIFT)
			& GCC_UART_CFG_RCGR_SRCDIV_MASK);

	cfg_rcgr |= ((UART2_RCGR_MODE << GCC_UART_CFG_RCGR_MODE_SHIFT)
			& GCC_UART_CFG_RCGR_MODE_MASK);

	writel(cfg_rcgr, GCC_BLSP1_UART2_APPS_CFG_RCGR);
}

void uart2_set_rate_mnd(unsigned int m,
			unsigned int n, unsigned int two_d)
{
	writel(m, GCC_BLSP1_UART2_APPS_M);
	writel(NOT_N_MINUS_M(n, m), GCC_BLSP1_UART2_APPS_N);
	writel(NOT_2D(two_d), GCC_BLSP1_UART2_APPS_D);
}

int uart2_trigger_update(void)
{
	unsigned long cmd_rcgr;
	int timeout = 0;

	cmd_rcgr = readl(GCC_BLSP1_UART2_APPS_CMD_RCGR);
	cmd_rcgr |= UART2_CMD_RCGR_UPDATE;
	writel(cmd_rcgr, GCC_BLSP1_UART2_APPS_CMD_RCGR);

	while (readl(GCC_BLSP1_UART2_APPS_CMD_RCGR) & UART2_CMD_RCGR_UPDATE) {
		if (timeout++ >= CLOCK_UPDATE_TIMEOUT_US) {
			printf("Timeout waiting for UART2 clock update\n");
			return -ETIMEDOUT;
		}
		udelay(1);
	}
	cmd_rcgr = readl(GCC_BLSP1_UART2_APPS_CMD_RCGR);
	return 0;
}

void uart2_toggle_clock(void)
{
	unsigned long cbcr_val;

	cbcr_val = readl(GCC_BLSP1_UART2_APPS_CBCR);
	cbcr_val |= UART2_CBCR_CLK_ENABLE;
	writel(cbcr_val, GCC_BLSP1_UART2_APPS_CBCR);
}

void uart2_clock_config(unsigned int m,
			unsigned int n, unsigned int two_d)
{
	uart2_configure_mux();
	uart2_set_rate_mnd(m, n, two_d);
	uart2_trigger_update();
	uart2_toggle_clock();
}

void qca_serial_init(struct ipq_serial_platdata *plat)
{
	int node, uart2_node;

	writel(1, GCC_BLSP1_UART1_APPS_CBCR);

	node = fdt_path_offset(gd->fdt_blob, "/serial@78B3000/serial_gpio");
	if (node < 0) {
		printf("Could not find serial_gpio node\n");
		return;
	}

	if (plat->port_id == 1) {
		uart2_node = fdt_path_offset(gd->fdt_blob, "uart2");
		if (uart2_node < 0) {
			printf("Could not find uart2 node\n");
			return;
		}
		node = fdt_subnode_offset(gd->fdt_blob,
				uart2_node, "serial_gpio");
		uart2_clock_config(plat->m_value, plat->n_value, plat->d_value);
		writel(1, GCC_BLSP1_UART2_APPS_CBCR);
	}
	qca_gpio_init(node);
}

void reset_crashdump(void)
{
	unsigned int ret = 0;
	qca_scm_sdi();
	ret = qca_scm_dload(CLEAR_MAGIC);
	if (ret)
		printf ("Error in reseting the Magic cookie\n");
	return;
}

void emmc_clock_config()
{
	/* Enable root clock generator */
	writel(readl(GCC_SDCC1_APPS_CBCR)|0x1, GCC_SDCC1_APPS_CBCR);
	/* Add 10us delay for CLK_OFF to get cleared */
	udelay(10);
	/* PLL0 - 192Mhz */
	writel(0x20B, GCC_SDCC1_APPS_CFG_RCGR);
	/* Delay for clock operation complete */
	udelay(10);
	writel(0x1, GCC_SDCC1_APPS_M);
	writel(0xFC, GCC_SDCC1_APPS_N);
	writel(0xFD, GCC_SDCC1_APPS_D);
	/* Delay for clock operation complete */
	udelay(10);
	/* Update APPS_CMD_RCGR to reflect source selection */
	writel(readl(GCC_SDCC1_APPS_CMD_RCGR)|0x1, GCC_SDCC1_APPS_CMD_RCGR);
	/* Add 10us delay for clock update to complete */
	udelay(10);
}

void emmc_clock_disable(void)
{
	/* Clear divider */
	writel(0x0, GCC_SDCC1_MISC);
}

void board_mmc_deinit(void)
{
	emmc_clock_disable();
}

void emmc_clock_reset(void)
{
	writel(0x1, GCC_SDCC1_BCR);
	udelay(10);
	writel(0x0, GCC_SDCC1_BCR);
}

void emmc_sdhci_init(void)
{
	writel(readl(MSM_SDC1_BASE) | (1 << 7), MSM_SDC1_BASE); //SW_RST
	udelay(10);
}
int board_mmc_init(bd_t *bis)
{
	int node;
	int ret = 0;
	qca_smem_flash_info_t *sfi = &qca_smem_flash_info;

	node = fdt_path_offset(gd->fdt_blob, "mmc");
	if (node < 0) {
		printf("sdhci: Node Not found, skipping initialization\n");
		return -1;
	}

	mmc_host.ioaddr = (void *)MSM_SDC1_SDHCI_BASE;
	mmc_host.voltages = MMC_VDD_165_195;
	mmc_host.version = SDHCI_SPEC_300;
	mmc_host.cfg.part_type = PART_TYPE_EFI;
	mmc_host.quirks = SDHCI_QUIRK_BROKEN_VOLTAGE;

	emmc_clock_disable();
	emmc_clock_reset();
	udelay(10);
	emmc_clock_config();
	emmc_sdhci_init();

	if (add_sdhci(&mmc_host, 200000000, 400000)) {
		printf("add_sdhci fail!\n");
		return -1;
	}

	if (!ret && sfi->flash_type == SMEM_BOOT_MMC_FLASH) {
		ret = board_mmc_env_init(mmc_host);
	}

	return ret;
}
void board_nand_init(void)
{
#ifdef CONFIG_QCA_SPI
	int gpio_node;
#endif

	qpic_nand_init();

#ifdef CONFIG_QCA_SPI
	gpio_node = fdt_path_offset(gd->fdt_blob, "/spi/spi_gpio");
	if (gpio_node >= 0) {
		qca_gpio_init(gpio_node);
		ipq_spi_init(CONFIG_IPQ_SPI_NOR_INFO_IDX);
	}
#ifdef CONFIG_SPI_NAND
	if (fdtdec_get_uint(gd->fdt_blob, 0, "spi_nand_available", 0))
		spi_nand_init();
#endif
#endif
}

void set_flash_secondary_type(qca_smem_flash_info_t *smem)
{
	return;
};

#ifdef CONFIG_USB_XHCI_IPQ
void board_usb_deinit(int id)
{
	void __iomem *boot_clk_ctl, *usb_bcr, *qusb2_phy_bcr;
	void __iomem *usb_phy_bcr, *usb_gen_cfg, *usb_guctl, *phy_base;

	if (id == 0) {
		boot_clk_ctl = GCC_USB_0_BOOT_CLOCK_CTL;
		usb_bcr = GCC_USB0_BCR;
		qusb2_phy_bcr = GCC_QUSB2_0_PHY_BCR;
		usb_phy_bcr = GCC_USB0_PHY_BCR;
		usb_gen_cfg = USB30_1_GENERAL_CFG;
		usb_guctl = USB30_1_GUCTL;
		phy_base = USB30_PHY_1_QUSB2PHY_BASE;
	}
	else if (id == 1) {
		boot_clk_ctl = GCC_USB_1_BOOT_CLOCK_CTL;
		usb_bcr = GCC_USB1_BCR;
		qusb2_phy_bcr = GCC_QUSB2_1_PHY_BCR;
		phy_base = USB30_PHY_2_QUSB2PHY_BASE;
	}
	else {
		return;
	}
	//Enable USB2 PHY Power down
	setbits_le32(phy_base+0xB4, 0x1);

	if (id == 0) {
		writel(0x8000, GCC_USB0_PHY_CFG_AHB_CBCR);
		writel(0xcff0, GCC_USB0_MASTER_CBCR);
		writel(0, GCC_SYS_NOC_USB0_AXI_CBCR);
		writel(0, GCC_SNOC_BUS_TIMEOUT1_AHB_CBCR);
		writel(0, GCC_USB0_SLEEP_CBCR);
		writel(0, GCC_USB0_MOCK_UTMI_CBCR);
		writel(0, GCC_USB0_AUX_CBCR);
	}
	else if (id == 1) {
		writel(0x8000, GCC_USB1_PHY_CFG_AHB_CBCR);
		writel(0xcff0, GCC_USB1_MASTER_CBCR);
		writel(0, GCC_SNOC_BUS_TIMEOUT2_AHB_CBCR);
		writel(0, GCC_USB1_SLEEP_CBCR);
		writel(0, GCC_USB1_MOCK_UTMI_CBCR);
	}

	//GCC_QUSB2_0_PHY_BCR
	setbits_le32(qusb2_phy_bcr, 0x1);
	mdelay(10);
	clrbits_le32(qusb2_phy_bcr, 0x1);

	//GCC_USB0_PHY_BCR
	setbits_le32(usb_phy_bcr, 0x1);
	mdelay(10);
	clrbits_le32(usb_phy_bcr, 0x1);

	//GCC Reset USB0 BCR
	setbits_le32(usb_bcr, 0x1);
	mdelay(10);
	clrbits_le32(usb_bcr, 0x1);
}

static void usb_clock_init(int id)
{
	if (id == 0) {
		writel(0x222000, GCC_USB0_GDSCR);
		writel(0, GCC_SYS_NOC_USB0_AXI_CBCR);
		writel(0, GCC_SNOC_BUS_TIMEOUT1_AHB_CBCR);
		writel(0x10b, GCC_USB0_MASTER_CFG_RCGR);
		writel(0x1, GCC_USB0_MASTER_CMD_RCGR);
		writel(1, GCC_SYS_NOC_USB0_AXI_CBCR);
		writel(0xcff1, GCC_USB0_MASTER_CBCR);
		writel(1, GCC_SNOC_BUS_TIMEOUT1_AHB_CBCR);
		writel(1, GCC_USB0_SLEEP_CBCR);
		writel(0x210b, GCC_USB0_MOCK_UTMI_CFG_RCGR);
		writel(0x1, GCC_USB0_MOCK_UTMI_M);
		writel(0xf7, GCC_USB0_MOCK_UTMI_N);
		writel(0xf6, GCC_USB0_MOCK_UTMI_D);
		writel(0x3, GCC_USB0_MOCK_UTMI_CMD_RCGR);
		writel(1, GCC_USB0_MOCK_UTMI_CBCR);
		writel(0x8001, GCC_USB0_PHY_CFG_AHB_CBCR);
		writel(1, GCC_USB0_AUX_CBCR);
		writel(1, GCC_USB0_PIPE_CBCR);
	}
	else if (id == 1) {
		writel(0x222000, GCC_USB1_GDSCR);
		writel(0, GCC_SNOC_BUS_TIMEOUT2_AHB_CBCR);
		writel(0xcff1, GCC_USB1_MASTER_CBCR);
		writel(1, GCC_SNOC_BUS_TIMEOUT2_AHB_CBCR);
		writel(1, GCC_USB1_SLEEP_CBCR);
		writel(0x210b, GCC_USB1_MOCK_UTMI_CFG_RCGR);
		writel(0x1, GCC_USB1_MOCK_UTMI_M);
		writel(0xf7, GCC_USB1_MOCK_UTMI_N);
		writel(0xf6, GCC_USB1_MOCK_UTMI_D);
		writel(0x3, GCC_USB1_MOCK_UTMI_CMD_RCGR);
		writel(1, GCC_USB1_MOCK_UTMI_CBCR);
		writel(0x8001, GCC_USB1_PHY_CFG_AHB_CBCR);
	}
}

static void usb_init_ssphy(int index)
{
	void __iomem *phybase;

	if (index == 0) {
		phybase = USB30_PHY_1_USB3PHY_AHB2PHY_BASE;
	}
	else if (index == 1) {
		phybase = USB30_PHY_2_USB2PHY_AHB2PHY_BASE;
	} else
		return;

	out_8(phybase + USB3_PHY_POWER_DOWN_CONTROL,0x1);
	out_8(phybase + QSERDES_COM_SYSCLK_EN_SEL,0x1a);
	out_8(phybase + QSERDES_COM_BIAS_EN_CLKBUFLR_EN,0x08);
	out_8(phybase + QSERDES_COM_CLK_SELECT,0x30);
	out_8(phybase + QSERDES_COM_BG_TRIM,0x0f);
	out_8(phybase + QSERDES_RX_UCDR_FASTLOCK_FO_GAIN,0x0b);
	out_8(phybase + QSERDES_COM_SVS_MODE_CLK_SEL,0x01);
	out_8(phybase + QSERDES_COM_HSCLK_SEL,0x00);
	out_8(phybase + QSERDES_COM_CMN_CONFIG,0x06);
	out_8(phybase + QSERDES_COM_PLL_IVCO,0x0f);
	out_8(phybase + QSERDES_COM_SYS_CLK_CTRL,0x06);
	out_8(phybase + QSERDES_COM_DEC_START_MODE0,0x82);
	out_8(phybase + QSERDES_COM_DIV_FRAC_START1_MODE0,0x55);
	out_8(phybase + QSERDES_COM_DIV_FRAC_START2_MODE0,0x55);
	out_8(phybase + QSERDES_COM_DIV_FRAC_START3_MODE0,0x03);
	out_8(phybase + QSERDES_COM_CP_CTRL_MODE0,0x0b);
	out_8(phybase + QSERDES_COM_PLL_RCTRL_MODE0,0x16);
	out_8(phybase + QSERDES_COM_PLL_CCTRL_MODE0,0x28);
	out_8(phybase + QSERDES_COM_INTEGLOOP_GAIN0_MODE0,0x80);
	out_8(phybase + QSERDES_COM_LOCK_CMP1_MODE0,0x15);
	out_8(phybase + QSERDES_COM_LOCK_CMP2_MODE0,0x34);
	out_8(phybase + QSERDES_COM_LOCK_CMP3_MODE0,0x00);
	out_8(phybase + QSERDES_COM_CORE_CLK_EN,0x00);
	out_8(phybase + QSERDES_COM_LOCK_CMP_CFG,0x00);
	out_8(phybase + QSERDES_COM_VCO_TUNE_MAP,0x00);
	out_8(phybase + QSERDES_COM_BG_TIMER,0x0a);
	out_8(phybase + QSERDES_COM_SSC_EN_CENTER,0x01);
	out_8(phybase + QSERDES_COM_SSC_PER1,0x31);
	out_8(phybase + QSERDES_COM_SSC_PER2,0x01);
	out_8(phybase + QSERDES_COM_SSC_ADJ_PER1,0x00);
	out_8(phybase + QSERDES_COM_SSC_ADJ_PER2,0x00);
	out_8(phybase + QSERDES_COM_SSC_STEP_SIZE1,0xde);
	out_8(phybase + QSERDES_COM_SSC_STEP_SIZE2,0x07);
	out_8(phybase + QSERDES_RX_UCDR_SO_GAIN,0x06);
	out_8(phybase + QSERDES_RX_RX_EQU_ADAPTOR_CNTRL2,0x02);
	out_8(phybase + QSERDES_RX_RX_EQU_ADAPTOR_CNTRL3,0x6c);
	out_8(phybase + QSERDES_RX_RX_EQU_ADAPTOR_CNTRL3,0x4c);
	out_8(phybase + QSERDES_RX_RX_EQU_ADAPTOR_CNTRL4,0xb8);
	out_8(phybase + QSERDES_RX_RX_EQ_OFFSET_ADAPTOR_CNTRL,0x77);
	out_8(phybase + QSERDES_RX_RX_OFFSET_ADAPTOR_CNTRL2,0x80);
	out_8(phybase + QSERDES_RX_SIGDET_CNTRL,0x03);
	out_8(phybase + QSERDES_RX_SIGDET_DEGLITCH_CNTRL,0x16);
	out_8(phybase + QSERDES_RX_SIGDET_ENABLES,0x0c);
	out_8(phybase + QSERDES_TX_HIGHZ_TRANSCEIVEREN_BIAS_D,0x45);
	out_8(phybase + QSERDES_TX_RCV_DETECT_LVL_2,0x12);
	out_8(phybase + QSERDES_TX_LANE_MODE,0x06);
	out_8(phybase + PCS_TXDEEMPH_M6DB_V0,0x15);
	out_8(phybase + PCS_TXDEEMPH_M3P5DB_V0,0x0e);
	out_8(phybase + PCS_FLL_CNTRL2,0x83);
	out_8(phybase + PCS_FLL_CNTRL1,0x02);
	out_8(phybase + PCS_FLL_CNT_VAL_L,0x09);
	out_8(phybase + PCS_FLL_CNT_VAL_H_TOL,0xa2);
	out_8(phybase + PCS_FLL_MAN_CODE,0x85);
	out_8(phybase + PCS_LOCK_DETECT_CONFIG1,0xd1);
	out_8(phybase + PCS_LOCK_DETECT_CONFIG2,0x1f);
	out_8(phybase + PCS_LOCK_DETECT_CONFIG3,0x47);
	out_8(phybase + PCS_POWER_STATE_CONFIG2,0x1b);
	out_8(phybase + PCS_RXEQTRAINING_WAIT_TIME,0x75);
	out_8(phybase + PCS_RXEQTRAINING_RUN_TIME,0x13);
	out_8(phybase + PCS_LFPS_TX_ECSTART_EQTLOCK,0x86);
	out_8(phybase + PCS_PWRUP_RESET_DLY_TIME_AUXCLK,0x04);
	out_8(phybase + PCS_TSYNC_RSYNC_TIME,0x44);
	out_8(phybase + PCS_RCVR_DTCT_DLY_P1U2_L,0xe7);
	out_8(phybase + PCS_RCVR_DTCT_DLY_P1U2_H,0x03);
	out_8(phybase + PCS_RCVR_DTCT_DLY_U3_L,0x40);
	out_8(phybase + PCS_RCVR_DTCT_DLY_U3_H,0x00);
	out_8(phybase + PCS_RX_SIGDET_LVL,0x88);
	out_8(phybase + USB3_PCS_TXDEEMPH_M6DB_V0,0x17);
	out_8(phybase + USB3_PCS_TXDEEMPH_M3P5DB_V0,0x0f);
	out_8(phybase + QSERDES_RX_SIGDET_ENABLES,0x0);
	out_8(phybase + USB3_PHY_START_CONTROL,0x03);
	out_8(phybase + USB3_PHY_SW_RESET,0x00);
}

static void usb_init_phy(int index)
{
	void __iomem *boot_clk_ctl, *usb_bcr, *qusb2_phy_bcr;
	void __iomem *usb_phy_bcr, *usb3_phy_bcr, *usb_gen_cfg, *usb_guctl, *phy_base;

	if (index == 0) {
		boot_clk_ctl = GCC_USB_0_BOOT_CLOCK_CTL;
		usb_bcr = GCC_USB0_BCR;
		qusb2_phy_bcr = GCC_QUSB2_0_PHY_BCR;
		usb_phy_bcr = GCC_USB0_PHY_BCR;
		usb3_phy_bcr = GCC_USB3PHY_0_PHY_BCR;
		usb_gen_cfg = USB30_1_GENERAL_CFG;
		usb_guctl = USB30_1_GUCTL;
		phy_base = USB30_PHY_1_QUSB2PHY_BASE;
	}
	else if (index == 1) {
		boot_clk_ctl = GCC_USB_1_BOOT_CLOCK_CTL;
		usb_bcr = GCC_USB1_BCR;
		qusb2_phy_bcr = GCC_QUSB2_1_PHY_BCR;
		phy_base = USB30_PHY_2_QUSB2PHY_BASE;
	}
	else {
		return;
	}
	//2. Enable SS Ref Clock
	setbits_le32(GCC_USB_SS_REF_CLK_EN, 0x1);

	//3. Disable USB Boot Clock
	clrbits_le32(boot_clk_ctl, 0x0);

	//4. GCC Reset USB0 BCR
	setbits_le32(usb_bcr, 0x1);

	//5. Delay 100us
	mdelay(10);

	//6. GCC Reset USB0 BCR
	clrbits_le32(usb_bcr, 0x1);
	//7. GCC_QUSB2_0_PHY_BCR
	setbits_le32(qusb2_phy_bcr, 0x1);

	//8. GCC_USB0_PHY_BCR
	setbits_le32(usb_phy_bcr, 0x1);
	setbits_le32(usb3_phy_bcr, 0x1);

	//9. Delay 100us
	mdelay(10);

	//10. GCC_USB0_PHY_BCR
	clrbits_le32(usb3_phy_bcr, 0x1);
	clrbits_le32(usb_phy_bcr, 0x1);

	//11. GCC_QUSB2_0_PHY_BCR
	clrbits_le32(qusb2_phy_bcr, 0x1);

	//12. Delay 100us
	mdelay(10);

	//20. Config user control register
	writel(0x0c80c010, usb_guctl);

	//21. Enable USB2 PHY Power down
	setbits_le32(phy_base+0xB4, 0x1);

	//22. PHY Config Sequence
	out_8(phy_base+0x80, 0xF8);
	out_8(phy_base+0x84, 0x83);
	out_8(phy_base+0x88, 0x83);
	out_8(phy_base+0x8C, 0xC0);
	out_8(phy_base+0x9C, 0x14);
	out_8(phy_base+0x08, 0x30);
	out_8(phy_base+0x0C, 0x79);
	out_8(phy_base+0x10, 0x21);
	out_8(phy_base+0x90, 0x00);
	out_8(phy_base+0x18, 0x00);
	out_8(phy_base+0x1C, 0x9F);
	out_8(phy_base+0x04, 0x80);

	//23. Disable USB2 PHY Power down
	clrbits_le32(phy_base+0xB4, 0x1);

	usb_init_ssphy(index);
}

int ipq_board_usb_init(void)
{
	int i, nodeoff;
	char node, usb_node[4] = "usb\0";

	for (i=0; i<CONFIG_USB_MAX_CONTROLLER_COUNT; i++) {
		node = i + '0';
		usb_node[3] = node;
		nodeoff = fdt_path_offset(gd->fdt_blob, usb_node);
		if (!fdtdec_get_int(gd->fdt_blob, nodeoff, "qcom,emulation", 0)) {
			usb_clock_init(i);
			usb_init_phy(i);
		}
	}
	return 0;
}
#endif

void ipq_fdt_fixup_usb_device_mode(void *blob)
{
	int nodeoff, ret, node;
	const char *usb_dr_mode = "peripheral"; /* Supported mode */
	const char *usb_max_speed = "high-speed";/* Supported speed */
	const char *usb_node[] = {"/soc/usb3@8A00000/dwc3@8A00000"};
	const char *usb_cfg;

	usb_cfg = getenv("usb_mode");
	if (!usb_cfg)
		return;

	if (strcmp(usb_cfg, usb_dr_mode)) {
		printf("fixup_usb: usb_mode can be either 'peripheral' or not set\n");
		return;
	}

	for (node = 0; node < ARRAY_SIZE(usb_node); node++) {
		nodeoff = fdt_path_offset(blob, usb_node[node]);
		if (nodeoff < 0) {
			printf("fixup_usb: unable to find node '%s'\n",
			       usb_node[node]);
			return;
		}
		ret = fdt_setprop(blob, nodeoff, "dr_mode",
				  usb_dr_mode,
				  (strlen(usb_dr_mode) + 1));
		if (ret)
			printf("fixup_usb: 'dr_mode' cannot be set");

		/* if mode is peripheral restricting to high-speed */
		ret = fdt_setprop(blob, nodeoff, "maximum-speed",
				  usb_max_speed,
				  (strlen(usb_max_speed) + 1));
		if (ret)
			printf("fixup_usb: 'maximum-speed' cannot be set");
	}
}

void enable_caches(void)
{
	qca_smem_flash_info_t *sfi = &qca_smem_flash_info;
	smem_get_boot_flash(&sfi->flash_type,
			    &sfi->flash_index,
			    &sfi->flash_chip_select,
			    &sfi->flash_block_size,
			    &sfi->flash_density);
	icache_enable();
	/*Skips dcache_enable during JTAG recovery */
	if (sfi->flash_type)
		dcache_enable();
}

void disable_caches(void)
{
	icache_disable();
	dcache_disable();
}

__weak int ipq_get_tz_version(char *version_name, int buf_size)
{
	return 1;
}
/**
 * Set the uuid in bootargs variable for mounting rootfilesystem
 */
int set_uuid_bootargs(char *boot_args, char *part_name, int buflen, bool gpt_flag)
{
	int ret, len;
	block_dev_desc_t *blk_dev;
	disk_partition_t disk_info;

	blk_dev = mmc_get_dev(mmc_host.dev_num);
	if (!blk_dev) {
		printf("Invalid block device name\n");
		return -EINVAL;
	}

	if (buflen <= 0 || buflen > MAX_BOOT_ARGS_SIZE)
		return -EINVAL;

#ifdef CONFIG_PARTITION_UUIDS
	ret = get_partition_info_efi_by_name(blk_dev,
			part_name, &disk_info);
	if (ret) {
		printf("bootipq: unsupported partition name %s\n",part_name);
		return -EINVAL;
	}
	if ((len = strlcpy(boot_args, "root=PARTUUID=", buflen)) >= buflen)
		return -EINVAL;
#else
	if ((len = strlcpy(boot_args, "rootfsname=", buflen)) >= buflen)
		return -EINVAL;
#endif
	boot_args += len;
	buflen -= len;

#ifdef CONFIG_PARTITION_UUIDS
	if ((len = strlcpy(boot_args, disk_info.uuid, buflen)) >= buflen)
		return -EINVAL;
#else
	if ((len = strlcpy(boot_args, part_name, buflen)) >= buflen)
		return -EINVAL;
#endif
	boot_args += len;
	buflen -= len;

	if (gpt_flag && strlcpy(boot_args, " gpt", buflen) >= buflen)
		return -EINVAL;

	return 0;
}

unsigned long timer_read_counter(void)
{
	return 0;
}

void reset_cpu(unsigned long a)
{
	while(1);
}

void ipq_fdt_fixup_socinfo(void *blob)
{
	return;
}

void fdt_fixup_auto_restart(void *blob)
{
	return;
}