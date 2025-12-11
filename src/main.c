
/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "nfctest.h"

int main(void)
{
    const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint8_t uart_input;

    if (!device_is_ready(uart_dev))
    {
        printk("UART not ready\n");
        return -1;
    }
    printk("UART NFC test ready. Type 'A' for mode 1, 'B' for mode 2\n");

    if (nfctest_setup() < 0)
    {
        printk("NFC setup failed. Cannot setup NFC T2T library\n");
        return -1;
    }

    printk("NFC ready\n");

    /* Main loop: read input from UART and trigger NFC operations */
    while (1)
    {
        /* Poll for UART input (non-blocking) */
        if (uart_poll_in(uart_dev, &uart_input) == 0)
        {
            /* Choose NFC operation mode based on user input */
            if (uart_input == '1')
            {
                /* Mode 1: send/read-only NDEF message */
                nfctest(1);
            }
            else if (uart_input == '2')
            {
                /* Mode 2: receive/writeable NDEF message */
                nfctest(2);
            }
            else
            {
                printk("Unknown input\n");
            }
        }

        k_msleep(100);
    }
}

