#ifndef CRC32_TEST_H
#define CRC32_TEST_H

#include <stdint.h>

enum crc_status 
{
    CRC_OK,
    CRC_FAIL,
    CRC_INVALID,
    CRC_UNUSED
};

struct crc_result 
{
    uint32_t crc;
    uint32_t crc_ref;
    enum crc_status status;
};

uint32_t crc32_bzip2_words(const uint32_t *data, size_t words_len);

struct crc_result crc32_words_check(uint32_t address, size_t words_len, int mode);

#endif // CRC32_TEST_H
