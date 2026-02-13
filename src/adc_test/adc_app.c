#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>

LOG_MODULE_REGISTER(saadc, LOG_LEVEL_DBG);
 
#define ADC_NODE DT_NODELABEL(adc)
 
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
static int16_t sample_buffer;
static bool adc_initialized;
 
static const struct adc_channel_cfg channel_cfg = 
{
    .gain = ADC_GAIN_1_2,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 0,
    .input_positive = NRF_SAADC_AIN0,
};
 
static const struct adc_sequence sequence = 
{
    .channels = BIT(0),
    .buffer = &sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution = 12,
};

int adc_app_init(void)
{
    int err;

    if (adc_initialized) 
    {
        return 0;
    }

    if (!device_is_ready(adc_dev)) 
    {
        return -ENODEV;
    }

    err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err) 
    {
        return err;
    }

    adc_initialized = true;
    return 0;
}

int adc_app_read_mv(int32_t *mv)
{
    int err;

    if (mv == NULL) 
    {
        return -EINVAL;
    }

    err = adc_app_init();
    if (err) 
    {
        return err;
    }

    err = adc_read(adc_dev, &sequence);
    if (err) 
    {
        return err;
    }

    *mv = sample_buffer;
    err = adc_raw_to_millivolts(600,
                               ADC_GAIN_1_2,
                               sequence.resolution,
                               mv);
    if (err < 0) 
    {
        return err;
    }

    return 0;
}
