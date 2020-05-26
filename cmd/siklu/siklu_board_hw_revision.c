#include <common.h>
#include <siklu/common_gpio.h>

#define NUM_OF_HW_REVISION_BITS 4

#define HW_REVISION_GPIO_NAME_BIT_0 	"cpm_gpio126"  //  cpm bank 1 gpio 26 (CP0_MPP[58])
#define HW_REVISION_GPIO_NAME_BIT_1 	"cpm_gpio127"  //  cpm bank 1 gpio 27 (CP0_MPP[59])
#define HW_REVISION_GPIO_NAME_BIT_2 	"cps_gpio07"   //  cps bank 0 gpio 7  (CP1_MPP[7])
#define HW_REVISION_GPIO_NAME_BIT_3 	"cpm_gpio10"   //  cpm bank 1 gpio 0  (CP0_MPP[32])

static const char *hw_rev_gpio_bits_array[NUM_OF_HW_REVISION_BITS]={
	HW_REVISION_GPIO_NAME_BIT_0, //LSB
	HW_REVISION_GPIO_NAME_BIT_1, 
	HW_REVISION_GPIO_NAME_BIT_2, 
	HW_REVISION_GPIO_NAME_BIT_3};

// get the siklu board HW revision
int siklu_get_hw_revision (unsigned int *hw_revision)
{
	int ret = CMD_RET_SUCCESS;
	int val = 0;

	*hw_revision = 0;

	for (int i=0; i<NUM_OF_HW_REVISION_BITS; i++)
	{
		ret=siklu_read_gpio_by_name (hw_rev_gpio_bits_array[i], &val);
		if (ret != CMD_RET_SUCCESS)
		{
			return ret;	
		}
		
		*hw_revision += val << i;
	}
	
	return CMD_RET_SUCCESS;
}

