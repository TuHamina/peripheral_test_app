#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "crc32_test.h"
#include "crc32.h"

LOG_MODULE_REGISTER(crc32_test);

uint32_t crc32_bzip2_words(const uint32_t *data, size_t words_len)
{
    uint32_t crc;
    BZ2_initialise_crc(&crc);

    /*
    * Input order matches bzip2 CRC32 implementation:
    * words are processed in reverse order and bytes MSB-first.
    */
    for (size_t w = words_len; w > 0; w--) 
    {
        uint32_t v = data[w - 1];

        uint8_t b0 = (v >> 24) & 0xFF;
        uint8_t b1 = (v >> 16) & 0xFF;
        uint8_t b2 = (v >> 8)  & 0xFF;
        uint8_t b3 =  v        & 0xFF;

        BZ2_update_crc(&crc, b0);
        BZ2_update_crc(&crc, b1);
        BZ2_update_crc(&crc, b2);
        BZ2_update_crc(&crc, b3);
    }

    BZ2_finalise_crc(&crc);
    return crc;
}

struct crc_result crc32_words_check(uint32_t address, size_t words_len, int mode)
{
    struct crc_result res = {0};

    if ((address & 0x3u) != 0) 
    {
        res.status = CRC_INVALID;
        return res;
    }

    if (words_len == 0) 
    {
        res.status = CRC_INVALID;
        return res;
    }

    const uint32_t *data = (const uint32_t *)address;
    uint32_t crc = crc32_bzip2_words(data, words_len);

    res.crc = crc;

    if (mode == 0) 
    {
        res.status = CRC_UNUSED;
        return res;
    }

    if (mode == 1) 
    {
        uintptr_t crc_addr = address + words_len * sizeof(uint32_t);
        uint32_t stored_crc = *(uint32_t *)crc_addr;

        if (stored_crc == crc) 
        { 
            res.status = CRC_OK; 
        } 
        else 
        { 
            res.status = CRC_FAIL; 
        }
        
        res.crc = stored_crc; 
        res.crc_ref = crc;
        
        return res;
    }

    res.status = CRC_INVALID;

    return res;
}
