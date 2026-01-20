#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include "nfc_test.h"
#include "crc32_test.h"

static int cmd_nfctest(const struct shell *sh, size_t argc, char **argv)
{
    int mode;
    int ret;

    uint8_t ndef_text_buf[NFCTEST_PAYLOAD_MAX] = {0};
    size_t ndef_text_len = 0;

    if (argc < 2)
    {
        shell_print(sh, "nfctest <mode> [<payload>]");
        shell_print(sh, "mode 1: set <payload> as tag value. Wait for tag to be read by NFC reader.");
        shell_print(sh, "mode 2: set empty tag. Wait for tag to be written by NFC reader.");
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

        ndef_text_len = strlen(argv[2]);
        if (ndef_text_len >= NFCTEST_PAYLOAD_MAX)
        {
            shell_print(sh, "Text too long (max %d)", NFCTEST_PAYLOAD_MAX - 1);
            return -EINVAL;
        }

        memcpy(ndef_text_buf, argv[2], ndef_text_len);
        ndef_text_buf[ndef_text_len] = '\0';

        shell_print(sh, "NFC text set to: %s", ndef_text_buf);
    }

    shell_print(sh, "Starting NFC test mode %d", mode);

    ret = nfctest(mode, ndef_text_buf, &ndef_text_len);

    if (ret == 0)
    {
        if (mode == 2)
        {
            shell_print(sh, "NFC RX TEXT: %s", ndef_text_buf);
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

static int check_address(const char *s, uintptr_t *out_addr)
{
    uintptr_t addr;

    if (out_addr == NULL) 
    {
        return -EINVAL;
    }

    if (s == NULL) 
    {
        return -EINVAL;
    }

    if (strlen(s) < 3) 
    {
        return -EINVAL;
    }

    if (s[0] == '-') 
    {
        return -EINVAL;
    }

    if (!(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')))
    {
        return -EINVAL;
    }

    addr = (uintptr_t)strtoul(s, NULL, 16);

    if ((addr & 0x3) != 0) 
    {
        return -EINVAL;
    }

    *out_addr = addr;
    return 0;
}

static int cmd_crc32(const struct shell *sh, size_t argc, char **argv)
{
    uintptr_t address;
    size_t words_len;
    int mode;

    if (argc < 4)
    {
        shell_print(sh, "Usage: crc32 <address> <words> <mode>");
        return -1;
    }

    if (check_address(argv[1], &address) != 0)
    {
        shell_error(sh, "Invalid address");
        return -EINVAL;
    }

    if (argv[2][0] == '-') 
    {
        shell_print(sh, "Word count must be positive");
        return -EINVAL;
    }

    words_len = strtoul(argv[2], NULL, 0);

    if (strcmp(argv[3], "0") == 0)
    {
        mode = 0;
    }
    else if (strcmp(argv[3], "1") == 0)
    {
        mode = 1;
    } 
    else
    {
        shell_print(sh, "Invalid mode, use 0 or 1");
        return -EINVAL;
    }

    struct crc_result r = crc32_words_check(address, words_len, mode);

    if (r.status == CRC_INVALID) 
    {
        shell_print(sh, "Invalid parameters");
        return -EINVAL;
    }

    if (mode == 0) 
    {
        shell_print(sh, "0x%08X", r.crc);
        return 0;
    }

    shell_print(sh, "0x%08X 0x%08X %s",
                r.crc, r.crc_ref,
                (r.status == CRC_OK) ? "OK" : "FAIL");

    return 0;
}

SHELL_CMD_REGISTER(crc32, NULL,
                   "crc32 test command",
                   cmd_crc32);

static void mem32_read(uintptr_t addr, uint32_t num,
                       const struct shell *sh)
{
    volatile uint32_t *p = (volatile uint32_t *)addr;
    printk("num=%d\n", num);
    for (uint32_t i = 0; i < num; i++)
    {
        shell_print(sh,
            "0x%08X = 0x%08X, %d",
            (uint32_t)&p[i],
            p[i], i);
    }
}

static int cmd_mem32(const struct shell *sh,
                     size_t argc,
                     char **argv)
{
    if (argc != 3)
    {
        shell_error(sh,
            "Usage: mem32 <addr> <num>");
        return -EINVAL;
    }

    uintptr_t addr;
    if (check_address(argv[1], &addr) != 0)
    {
        shell_error(sh, "Invalid address");
        return -EINVAL;
    }

    if (argv[2][0] == '-')
    {
        shell_error(sh, "Value must be positive");
        return -EINVAL;
    }

    uint32_t num = strtoul(argv[2], NULL, 0);

    mem32_read(addr, num, sh);
    return 0;
}

SHELL_CMD_REGISTER(mem32, NULL,
    "Read 32-bit memory words",
    cmd_mem32);

static int cmd_w4(const struct shell *sh,
                     size_t argc,
                     char **argv)
{
    if (argc != 3)
    {
        shell_error(sh,
            "Usage: w4 <addr> <value>");
        return -EINVAL;
    }

    uintptr_t addr;
    if (check_address(argv[1], &addr) != 0)
    {
        shell_error(sh, "Invalid address");
        return -EINVAL;
    }

    if (argv[2][0] == '-')
    {
        shell_error(sh, "Value must be positive");
        return -EINVAL;
    }

    uint32_t value = strtoul(argv[2], NULL, 0);

    volatile uint32_t *p = (volatile uint32_t *)addr;
    *p = value;

    shell_print(sh, "OK: 0x%08X @ 0x%08X",
                value, (uint32_t)addr);
    
    return 0;
}

SHELL_CMD_REGISTER(w4, NULL,
    "Write 32-bit memory word",
    cmd_w4);
