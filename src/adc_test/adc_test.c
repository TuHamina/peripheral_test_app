/* 
 * Register-level SAADC test.
 * Currently returns only zero values.
 * Kept for debugging and comparison purposes.
 */

#include <zephyr/kernel.h>
#include <nrfx.h>
#include <hal/nrf_saadc.h>

#include <zephyr/sys/printk.h>
#include <nrfx_saadc.h>

#include <zephyr/toolchain.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>

#define SAADC_TACQ_DEFAULT 20U

void adc_test(void)
{
    NRF_P1->PIN_CNF[4] =     (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |     (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |     (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos);
    NRF_SAADC->EVENTS_END = 0;
    printk("EVENTS_END = %u\n", NRF_SAADC->EVENTS_END);

    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_12bit;
    NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass;

    NRF_SAADC->CH[0].CONFIG =
        (SAADC_CH_CONFIG_GAIN_Gain1_2 << SAADC_CH_CONFIG_GAIN_Pos)
        | (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos)
        | (SAADC_TACQ_DEFAULT << SAADC_CH_CONFIG_TACQ_Pos)
        | (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos);

    NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_CONNECT_NC;
    //NRF_SAADC->CH[0].PSELN =
      //  (SAADC_CH_PSELN_CONNECT_NC << SAADC_CH_PSELN_CONNECT_Pos);

    NRF_SAADC->CH[0].PSELP =
        (SAADC_CH_PSELP_CONNECT_AnalogInput << SAADC_CH_PSELP_CONNECT_Pos) |
        (1U << SAADC_CH_PSELP_PORT_Pos) |
        (4U << SAADC_CH_PSELP_PIN_Pos);

    printk("ENABLE = %u\n", NRF_SAADC->ENABLE);
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled;
    printk("ENABLE = %u\n", NRF_SAADC->ENABLE);

    // // NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
    // // NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
    // // while (!NRF_SAADC->EVENTS_CALIBRATEDONE) {}

    static int16_t sample;

    NRF_SAADC->RESULT.PTR = (uint32_t)&sample;
    NRF_SAADC->RESULT.MAXCNT = 1;

    NRF_SAADC->TASKS_START = 1;
    while (!NRF_SAADC->EVENTS_STARTED);

    NRF_SAADC->TASKS_SAMPLE = 1;
    while (!NRF_SAADC->EVENTS_END);

    NRF_SAADC->EVENTS_STARTED = 0;
    printk("Sample = 0x%08X\n", sample);
    NRF_SAADC->EVENTS_END = 0;

    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled;
}
