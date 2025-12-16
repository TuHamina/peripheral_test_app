#ifndef NFC_TEST_H
#define NFC_TEST_H

#include <stdint.h>

#define MAX_REC_COUNT     1
#define MAX_NDEF_RECORDS  1
#define NDEF_MSG_BUF_SIZE 128
#define NFC_TEST_RX_MAX     128

/* Initializes the Type 4 Tag with callback */
int nfctest_setup(void);

/*
 * Test entry point
 * mode == 1 → send (read-only)
 * mode == 2 → receive (read/write)
 */
int nfctest(int mode, const uint8_t *data, size_t data_length);

int nfctest_get_rx_text(uint8_t *buf, size_t buf_size, size_t *out_len);

#endif /* NFC_TEST_H */