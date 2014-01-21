#include "mac.h"

void memcpy(uint8_t* dst, const uint8_t *src, long len)
{
  while (len--)
    *dst++=*src++;
}
