#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include "nfctest.h"

static int cmd_nfctest(const struct shell *sh, size_t argc, char **argv)
{
    int mode;
    int ret;

    uint8_t tx_buf[NDEF_MSG_BUF_SIZE] = {0};
    size_t tx_len = 0;

    if (argc < 2)
    {
        shell_print(sh, "Usage: nfctest <mode>");
        shell_print(sh, "mode 1: send (read-only)");
        shell_print(sh, "mode 2: receive (read/write)");
        return -1;
    }
    
    if (strcmp(argv[1], "1") == 0)
    {
        mode = 1;
    }
    else if (strcmp(argv[1], "2") == 0)
    {
        mode = 2;
    } 
    else
    {
        shell_print(sh, "Invalid mode, use 1 or 2");
        return -EINVAL;
    }

    if (mode == 1 && argc >= 3)
    {
        tx_len = strlen(argv[2]);
        if (tx_len >= NDEF_MSG_BUF_SIZE)
        {
            shell_print(sh, "Text too long (max %d)", NDEF_MSG_BUF_SIZE - 1);
            return -EINVAL;
        }

        memcpy(tx_buf, argv[2], tx_len);
        tx_buf[tx_len] = '\0';

        shell_print(sh, "NFC text set to: %s", tx_buf);
    }

    shell_print(sh, "Starting NFC test mode %d", mode);

    ret = nfctest(mode, tx_buf, tx_len);

    if (ret == 0 && mode == 2)
    {
        uint8_t rx_buf[128];
        size_t rx_len;

        if (nfctest_get_rx_text(rx_buf, sizeof(rx_buf), &rx_len) == 0)
        {
            shell_print(sh, "NFC RX TEXT: %s", rx_buf);
        }
        else
        {
            shell_print(sh, "No NFC RX data");
        }
    }

    if (ret == 0)
    {
        shell_print(sh, "OK");
    }
    else
    {
        shell_print(sh, "FAIL (%d)", ret);
    }

    return ret;
}

SHELL_CMD_REGISTER(nfctest, NULL,
                   "NFC test command",
                   cmd_nfctest);

static int cmd_demo_params(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++)
    {
		shell_print(sh, "  argv[%zd] = %s", cnt, argv[cnt]);
	}

	return 0;
}

SHELL_CMD_REGISTER(params, NULL,
                   "Print params command",
                   cmd_demo_params);

int main(void)
{
    // const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    // uint8_t uart_input;

    // if (!device_is_ready(uart_dev))
    // {
    //     printk("UART not ready\n");
    //     return -1;
    // }
    // printk("UART NFC test ready. Type 'A' for mode 1, 'B' for mode 2\n");

    //if (nfctest_setup() < 0)
    //{
    //     printk("NFC setup failed. Cannot setup NFC T2T library\n");
    //     return -1;
    //}

    //printk("NFC ready\n");

    // /* Main loop: read input from UART and trigger NFC operations */
    // while (1)
    // {
    //     /* Poll for UART input (non-blocking) */
    //     if (uart_poll_in(uart_dev, &uart_input) == 0)
    //     {
    //         /* Choose NFC operation mode based on user input */
    //         if (uart_input == '1')
    //         {
    //             /* Mode 1: send/read-only NDEF message */
    //             nfctest(1);
    //         }
    //         else if (uart_input == '2')
    //         {
    //             /* Mode 2: receive/writeable NDEF message */
    //             nfctest(2);
    //         }
    //         else
    //         {
    //             printk("Unknown input\n");
    //         }
    //     }

    //     k_msleep(100);
    // }

    if (nfctest_setup() < 0)
    {
            printk("NFC setup failed. Cannot setup NFC T2T library\n");
            return -1;
    }

    printk("NFC ready\n");
}

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif


