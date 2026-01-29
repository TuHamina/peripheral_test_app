#ifndef NFC_TEST_FIELD_DETECT_H
#define NFC_TEST_FIELD_DETECT_H

struct nfct_field_info {
    uint32_t fieldpresent;
    uint32_t nfctagstate;
    uint32_t last_fp_seen; 

    uint32_t freq_raw;
    uint32_t freq_hz;
};

int nfct_sense_on_off(int submode);
int check_field_presence(int timeout_ms, struct nfct_field_info *info);

#endif /* NFC_TEST_FIELD_DETECT_H */
