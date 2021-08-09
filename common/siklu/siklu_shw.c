#include <common.h>
#include <spi_flash.h>
#include <linux/mtd/nand.h>
#include <asm/arch-qca-common/qpic_nand.h>
#include <siklu/siklu_board_generic.h>

DECLARE_GLOBAL_DATA_PTR;

// show HW revision
void show_hw_revision (void)
{
	siklu_print_hw_revision();
}

// show board model
static void show_board_model (void)
{
	const char *model;

	model = fdt_getprop(gd->fdt_blob, 0, "model", NULL);

	printf("Model: %s\n", model ? model : "Unknown");
}

//  get nand params (all parmas are out params)
static int  get_nand_params (struct mtd_info **mtd_p, unsigned *nand_maf_id,
		unsigned *nand_dev_id)
{
	struct mtd_info *mtd = NULL;
	struct qpic_nand_dev *qpic;

	mtd = get_mtd_device(NULL, 0);
	if (mtd == NULL)
	{
		return CMD_RET_FAILURE;
	}

	qpic = MTD_QPIC_NAND_DEV(mtd);
	if (qpic == NULL)
	{
		return CMD_RET_FAILURE;
	}

	*mtd_p = mtd;
	*nand_maf_id = qpic->vendor;
	*nand_dev_id = qpic->device;

	return CMD_RET_SUCCESS;
}


// show nand manufacturer info
static void show_nand_manufacturer_info (struct mtd_info *mtd, int nand_maf_id)
{
	struct nand_onfi_params *onfi_params = &MTD_NAND_CHIP(mtd)->onfi_params;

	// nand name and model
	printf("Name: %s %s, ", onfi_params->manufacturer, onfi_params->model);

	// nand Manufacturer ID
	printf("Manufacturer ID: 0x%02x, ",nand_maf_id);
}


// show nand info
static void show_nand_info (void)
{
	int ret = CMD_RET_SUCCESS;
	struct mtd_info *mtd;
	unsigned nand_maf_id;
	unsigned nand_dev_id;

	printf("NAND: ");

	ret = get_nand_params (&mtd, &nand_maf_id, &nand_dev_id);
	if (ret != CMD_RET_SUCCESS)
	{
			printf("Unknown\n");
			return;
	}

	show_nand_manufacturer_info(mtd, nand_maf_id);

	// nand size
	printf("Size: %llu MIB, ", mtd->size >> 20);

	// nand Chip ID
	printf("Chip ID: 0x%02x\n", nand_dev_id);
}


// show DRAM (DDR)
static void show_dram_info (void)
{
	puts("DRAM: ");
	print_size(gd->ram_size, "\n");
}


// show SF (NOR)
static void show_sf_info (void)
{
	struct spi_flash *flash;

	printf("SF: ");

	/* max_hz and spi_mode parameters are ignored */
	flash = spi_flash_probe(0, 0, 0, 0);
	if (flash == NULL)
	{
		printf("Unknown\n");
		return;
	}

	printf("%s with page size ", flash->name);
	print_size(flash->page_size, ", erase size ");
	print_size(flash->erase_size, ", total ");
	print_size(flash->size, "");
	printf ("\n");
}


// show CPU
static void show_cpu_info (void)
{
	int ret = 0;

	printf("CPU: ");

	//name
	printf("Name: ");

	const char *cpu_name = NULL;

	ret = siklu_get_cpu_name(&cpu_name);
	if ((!ret) && cpu_name)
	{
		printf("%s, ",cpu_name);
	}
	else
	{
		printf("%s, ", ret == -ENOSYS  ? "Not implemented" : "Unknown");
	}

	//config register
	uint64_t config_reg = 0;
	printf("config register: ");

	ret = siklu_get_cpu_config_register(&config_reg);
	if (!ret)
	{
		printf("0x%llx\n",config_reg);
	}
	else
	{
		printf("%s\n", ret == -ENOSYS  ? "Not implemented" : "Unknown");
	}
}


// main shw command function
static int do_shw(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	show_hw_revision();
	show_board_model();
	show_dram_info();
	show_nand_info();
	show_sf_info();
	show_cpu_info();

	return CMD_RET_SUCCESS;
}


U_BOOT_CMD(
	shw,								//name
	1,									//max params
	0,									//rep
	do_shw,								//func
	"dispaly HW info (siklu command)",	//help
	"" 									//usage
);
