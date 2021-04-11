#include <common.h>
#include <siklu/siklu_board_generic.h>
#include <dm/device.h>
#include <asm/io.h>

#define CTU_HW_REVISION_GPIO_BIT_0 31
#define CTU_HW_REVISION_GPIO_BIT_1 32
#define CTU_HW_REVISION_GPIO_BIT_2 35
#define CTU_HW_REVISION_GPIO_BIT_3 36

// get the board HW revision
int siklu_get_hw_revision(u32 *hw_revision)
{
	*hw_revision |= gpio_get_value(CTU_HW_REVISION_GPIO_BIT_0) << 0;
	*hw_revision |= gpio_get_value(CTU_HW_REVISION_GPIO_BIT_1) << 1;
	*hw_revision |= gpio_get_value(CTU_HW_REVISION_GPIO_BIT_2) << 2;
	*hw_revision |= gpio_get_value(CTU_HW_REVISION_GPIO_BIT_3) << 3;

	return 0;
}

// print siklu HW revision
 void siklu_print_hw_revision (void)
{
	int ret = 0;
	u32 hw_revision = 0;

	ret  = siklu_get_hw_revision(&hw_revision);

	printf ("HW revision: ");
	if (!ret)
	{
		printf ("%u\n", hw_revision);
	}
	else
	{
		printf("%s\n", ret == -ENOSYS  ? "Not implemented" : "Unknown");
	}
}

// get CPU info
int siklu_get_cpu_name (const char **cpu_name)
{
	*cpu_name = "qcom,ipq6018";

	return 0;
}


// get CPU info
int siklu_get_cpu_config_register(uint64_t *config_reg)
{
	return -ENOSYS; // not implemented
}
