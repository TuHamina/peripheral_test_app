#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "nfc_test.h"
#include "crc32_test.h"

LOG_MODULE_REGISTER(shell_nfctest);

int main(void)
{
    if (nfctest_setup() < 0)
    {
            LOG_ERR("NFC setup failed");
            return -1;
    }

    LOG_INF("NFC ready");
}
