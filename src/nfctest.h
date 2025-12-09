#ifndef NFC_TEST_H
#define NFC_TEST_H

#include <stdint.h>

#define NDEF_MSG_BUF_SIZE 128

int nfctest_setup(void);
int nfctest_send_data(void);
void nfctest(int mode);

#endif /* NFC_TEST_H */