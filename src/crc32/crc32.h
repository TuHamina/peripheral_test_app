#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

void BZ2_initialise_crc (uint32_t *crc);

void BZ2_finalise_crc (uint32_t *crc);

void BZ2_update_crc (uint32_t *crc, uint8_t ch);

#endif // CRC32_H
