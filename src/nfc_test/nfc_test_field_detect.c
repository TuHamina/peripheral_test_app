#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrfx.h>
#include <nrfx_nfct.h>
#include <hal/nrf_nfct.h>

#include "nfc_test_field_detect.h"

#define NFC_FIELD_OK      0
#define NFC_FIELD_TIMEOUT 1

K_TIMER_DEFINE(field_timer, NULL, NULL);

#define NFCT_FREQMEASURE_START (*(volatile uint32_t *)0x5F985018)
#define NFCT_FREQMEASURE_DONE (*(volatile uint32_t *)0x5F985120)
#define NFCT_MEASUREDFREQ (*(volatile uint32_t *)0x5F985434)

#define NFCT_SENSE_ACTIVATE 1
#define NFCT_SENSE_DISABLE  2

LOG_MODULE_REGISTER(nfctest_nrfx_test, LOG_LEVEL_INF);

int nfct_sense_apply_submode(int submode)
{
    NRF_NFCT->TASKS_SENSE = 1;
    LOG_INF("NFCT SENSE ON");
    LOG_INF("NFCTAGSTATE=0x%08X", (uint32_t)NRF_NFCT->NFCTAGSTATE);

    switch (submode)
    {
        case NFCT_SENSE_ACTIVATE:
            NRF_NFCT->TASKS_ACTIVATE = 1;
            LOG_INF("After ACTIVATE: NFCTAGSTATE=0x%08X",
                    (uint32_t)NRF_NFCT->NFCTAGSTATE);
            break;

        case NFCT_SENSE_DISABLE:
            NRF_NFCT->TASKS_DISABLE = 1;
            LOG_INF("After DISABLE: NFCTAGSTATE=0x%08X",
                    (uint32_t)NRF_NFCT->NFCTAGSTATE);
            break;

        default:
            break;
    }

    return 0;
}

int nfct_sense_on_off(int submode)
{
    switch (submode)
    {
        case NFCT_SENSE_ACTIVATE:
        case NFCT_SENSE_DISABLE:
            return nfct_sense_apply_submode(submode);

        default:
            return -EINVAL;
    }
}

int check_field_presence(uint32_t timeout_ms, struct nfct_field_info *info)
{
    uint32_t start = k_uptime_get_32();

    info->last_fp_seen = 0;
    info->fieldpresent = 0;
    info->nfctagstate  = 0;
    info->freq_raw     = 0;
    info->freq_hz      = 0;

    nfct_sense_apply_submode(NFCT_SENSE_ACTIVATE);

    while (NRF_NFCT->NFCTAGSTATE == 0) 
    {
        if ((k_uptime_get_32() - start) > timeout_ms) 
        {
            return NFC_FIELD_TIMEOUT;
        }
        k_sleep(K_MSEC(1));
    }

    while (1) 
    {

        info->last_fp_seen = NRF_NFCT->FIELDPRESENT;

        if ((k_uptime_get_32() - start) > timeout_ms) 
        {
            NFCT_FREQMEASURE_DONE = 0;
            NFCT_FREQMEASURE_START = 0;
            info->fieldpresent = info->last_fp_seen;
            info->nfctagstate  = NRF_NFCT->NFCTAGSTATE;
            return NFC_FIELD_TIMEOUT;
        }

        if (info->last_fp_seen != 0)
        {
            info->fieldpresent = 1;
            break;
        }

        k_sleep(K_MSEC(10));
    }

    NFCT_FREQMEASURE_DONE = 0;
    NFCT_FREQMEASURE_START = 1;

    while (NFCT_FREQMEASURE_DONE == 0) 
    { 
        if ((k_uptime_get_32() - start) > timeout_ms) 
        {
            info->nfctagstate = NRF_NFCT->NFCTAGSTATE;
            return NFC_FIELD_TIMEOUT;
        }

        k_sleep(K_MSEC(1));
    }

    info->freq_raw = NFCT_MEASUREDFREQ;

    info->freq_hz = (16000000ULL * info->freq_raw) / 1000ULL;

    info->nfctagstate = NRF_NFCT->NFCTAGSTATE;

    NFCT_FREQMEASURE_DONE = 0;
    NFCT_FREQMEASURE_START = 0;

    info->nfctagstate = NRF_NFCT->NFCTAGSTATE;

    return NFC_FIELD_OK;
}
