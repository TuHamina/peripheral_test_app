#ifndef ADC_APP_H
#define ADC_APP_H

#include <stdint.h>

int adc_app_init(void);
int adc_app_read_mv(int32_t *mv);

#endif // ADC_APP_H
