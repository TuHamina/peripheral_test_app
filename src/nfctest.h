#ifndef NFC_TEST_H
#define NFC_TEST_H

#include <stdint.h>

#define NDEF_MSG_BUF_SIZE 128

/* Initializes the Type 4 Tag with callback */
int nfctest_setup(void);

/*
 * Test entry point
 * mode == 1 → send (read-only)
 * mode == 2 → receive (read/write)
 */
void nfctest(int mode);

#endif /* NFC_TEST_H */