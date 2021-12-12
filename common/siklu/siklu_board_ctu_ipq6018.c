#include <asm/io.h>
#include <asm/arch-qca-common/gpio.h>
#include <siklu/siklu_board_ctu_ipq6018.h>

static void set_led_pwm(void)
{
    /* Configure PWM dividers */
    writel(0x7a11f423, 0x1941020);
    writel(0x9f, 0x1941024);
    /* Enable PWM */
    writel(0xc000009f, 0x1941024);
}

/*
 * The green color on the power LED is connected to GPIO 24. GPIO 24 is muxed
 * to the PWM2 function. Configure PWM2 to 10 Hz with 50% duty cycle to make the
 * LED blink.
 */
void siklu_ctu_ipq6018_power_leds_init(void)
{
    /* Set adss_pwm_clk_src source to gpll0 */
    writel(0x10f, 0x181c00c);
    /* Enable adss_pwm_clk_src */
    writel(0x80000001, 0x181c008);
    /* Enable gcc_adss_pwm_clk */
    writel(0x80000001, 0x181c020);

    set_led_pwm();
    /* For some reason we have to repeat that to take affect */
    set_led_pwm();
}
