#ifndef NFC_TEST_H
#define NFC_TEST_H

#include <stdint.h>

#define MAX_REC_COUNT     1
#define NDEF_MSG_BUF_SIZE 128
#define NFCTEST_PAYLOAD_MAX 32

/* Initializes the Type 4 Tag with callback */
int nfctest_setup(void);

/*
 * Test entry point
 * mode == 1 → send (read-only)
 * mode == 2 → receive (read/write)
 */
int nfctest(int mode, uint8_t *data, size_t *data_length);

#endif /* NFC_TEST_H */