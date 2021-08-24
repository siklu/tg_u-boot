#include <asm/arch-qca-common/gpio.h>
#include <siklu/siklu_board_ctu_ipq6018.h>

#define POWER_LED_YELLOW_COLOR_GPIO_PIN 23

void siklu_ctu_ipq6018_power_leds_init(void)
{
    /*
    Yellow color of the power led does not turn on automatically after board power on 
    because of HW bug so we need to do it manually 
    */
    gpio_direction_output(POWER_LED_YELLOW_COLOR_GPIO_PIN, 1);
}
