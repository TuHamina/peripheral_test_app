
/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

void nfctest_mode1(void)
{
    printk("NFC test mode 1 running\n");
}

void nfctest_send_data(const char *data)
{
    printk("NFC data sent\n");
}

void nfctest_receive_data(const char *data)
{
	printk("NFC data received\n");
}

void nfctest(int mode)
{
    if (mode == 1)
	{
		nfctest_mode1();
	}
    else if (mode == 2)
	{
        nfctest_send_data("TESTDATA");
	}
    else if (mode == 3)
	{
		nfctest_receive_data("TESTDATA");
	}
    else
    {
        printk("Invalid mode\n");
    }
}

int main(void)
{
    const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint8_t uart_input;

    if (!device_is_ready(uart_dev))
    {
        printk("UART not ready!\n");
        return -1;
    }
    printk("UART NFC test ready. Type 'A' for mode 1, 'B' for mode 2\n");

    while (1)
    {
        if (uart_poll_in(uart_dev, &uart_input) == 0)
        {
            if (uart_input == 'A') 
            {
                nfctest(1);
            }
            else if (uart_input == 'B')
            {
                nfctest(2);
            }
            else if (uart_input == 'C')
            {
                nfctest(3);
            }
            else
            {
                printk("Unknown input\n");
            }
        }
        k_msleep(100);
    }
}

