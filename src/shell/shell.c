#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include "nfc_test.h"
#include "crc32_test.h"
#include "nfc_test_field_detect.h"

int cmd_nfctest(const struct shell *sh, size_t argc, char **argv)
{
    int mode = -1;
    int ret = 0;

    uint8_t ndef_text_buf[NFCTEST_PAYLOAD_MAX] = {0};
    size_t ndef_text_len = 0;

    if (argc < 2)
    {
        shell_print(sh, "Usage: nfctest <mode> [args]");
        shell_print(sh, "  mode 1: set <payload> as tag value");
        shell_print(sh, "  mode 2: set empty tag, wait for write");
        shell_print(sh, "  mode 3: NFCT sense on/off");
        shell_print(sh, "  mode 4: field presence test");
        return -EINVAL;
    }

    if (strcmp(argv[1], "1") == 0)
        mode = 1;
    else if (strcmp(argv[1], "2") == 0)
        mode = 2;
    else if (strcmp(argv[1], "3") == 0)
        mode = 3;
    else if (strcmp(argv[1], "4") == 0)
        mode = 4;
    else
    {
        shell_print(sh, "Invalid mode, use 1–4");
        return -EINVAL;
    }

    switch (mode)
    {
    case 1: /* Read mode */
        if (argc < 3)
        {
            shell_print(sh, "Missing text for mode 1");
            return -EINVAL;
        }

        ndef_text_len = strlen(argv[2]);
        if (ndef_text_len >= NFCTEST_PAYLOAD_MAX)
        {
            shell_print(sh, "Text too long (max %d)",
                        NFCTEST_PAYLOAD_MAX - 1);
            return -EINVAL;
        }

        memcpy(ndef_text_buf, argv[2], ndef_text_len);
        ndef_text_buf[ndef_text_len] = '\0';

        shell_print(sh, "NFC text set to: %s", ndef_text_buf);
        break;

    case 2: /* Write mode */
        break;

    case 3: /* Sense (Activate/Disable) on/off */
    {
        int submode;

        if (argc < 3)
        {
            shell_print(sh, "Usage: nfctest 3 <submode>");
            shell_print(sh, "  submode 1 = Sense ON");
            shell_print(sh, "  submode 2 = Sense OFF");
            return -EINVAL;
        }

        if (strcmp(argv[2], "1") == 0)
            submode = 1;
        else if (strcmp(argv[2], "2") == 0)
            submode = 2;
        else
        {
            shell_print(sh, "Invalid submode, use 1 or 2");
            return -EINVAL;
        }

        shell_print(sh,
            "Starting NFC test mode 3, submode %d",
            submode);

        ret = nfct_sense_on_off(submode);

        shell_print(sh, ret ? "FAIL (%d)" : "OK", ret);
        return ret;
    }

    case 4: /* Field presence test */
    {
        struct nfct_field_info info;

        shell_print(sh, "Starting NFC test mode 4");

        ret = check_field_presence(1000, &info);

        shell_print(sh, "check_field_presence ret=%d", ret);
        shell_print(sh, "last_fp_seen = 0x%08X", info.last_fp_seen);
        shell_print(sh, "FIELDPRESENT = 0x%08X", info.fieldpresent);
        shell_print(sh, "NFCTAGSTATE  = 0x%08X", info.nfctagstate);
        shell_print(sh, "FREQ RAW     = %u", info.freq_raw);
        shell_print(sh, "FREQ (Hz)    = %u", info.freq_hz);

        shell_print(sh, ret ? "FAIL (%d)" : "OK", ret);
        return ret;
    }

    default:
        return -EINVAL;
    }

    shell_print(sh, "Starting NFC test mode %d", mode);

    ret = nfctest(mode, ndef_text_buf, &ndef_text_len);

    if (ret == 0 && mode == 2)
    {
        shell_print(sh, "NFC RX TEXT: %s", ndef_text_buf);
    }

    shell_print(sh, ret ? "FAIL (%d)" : "OK", ret);
    return ret;
}

SHELL_CMD_REGISTER(nfctest, NULL,
                   "NFC test command",
                   cmd_nfctest);

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

    if (argv[2][0] == '-') 
    {
        shell_print(sh, "Word count must be positive");
        return -EINVAL;
    }

    address   = strtoul(argv[1], NULL, 0);
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
