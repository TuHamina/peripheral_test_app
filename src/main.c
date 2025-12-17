#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include "nfctest.h"

LOG_MODULE_REGISTER(shell_nfctest);

static int cmd_nfctest(const struct shell *sh, size_t argc, char **argv)
{
    int mode;
    int ret;

    uint8_t resp_buf[NFCTEST_PAYLOAD_MAX] = {0};
    size_t resp_len = 0;

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

    if (mode == 1)
    {
        if (argc < 3)
        {
            shell_print(sh, "Missing text for send mode");
            return -EINVAL;
        }

        resp_len = strlen(argv[2]);
        if (resp_len >= NFCTEST_PAYLOAD_MAX)
        {
            shell_print(sh, "Text too long (max %d)", NFCTEST_PAYLOAD_MAX - 1);
            return -EINVAL;
        }

        memcpy(resp_buf, argv[2], resp_len);
        resp_buf[resp_len] = '\0';

        shell_print(sh, "NFC text set to: %s", resp_buf);
    }

    shell_print(sh, "Starting NFC test mode %d", mode);

    ret = nfctest(mode, resp_buf, &resp_len);

    if (ret == 0)
    {
        if (mode == 2)
        {
            shell_print(sh, "NFC RX TEXT: %s", resp_buf);
        }

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

int main(void)
{
    if (nfctest_setup() < 0)
    {
            LOG_ERR("NFC setup failed. Cannot setup NFC T2T library\n");
            return -1;
    }

    LOG_INF("NFC ready\n");
}

