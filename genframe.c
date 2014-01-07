#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "acn.h"


#define SET(t, n) _set(t, n, sizeof(t))

uint8_t pid[] = { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };

void _set(uint8_t *p, unsigned int value, int size)
{
  while (size-- > 0)
  {
    p[size] = value & 0xff;
    value >>= 8;
  }
}


int main()
{
  int bpl = 16;
  FILE *fp = fopen("acnraw.h", "w");
  struct E131_2009 packet;
  memset(&packet, 0, sizeof(packet));

  /*
   * Initialize the packet
   */
  SET(packet.preamble_size, 0x0010);
  SET(packet.rlp_version, 0x0004);
  SET(packet.efl_version, 0x0002);
  memcpy(packet.acn_packet_id, pid, sizeof(pid));
  strcpy((char*)packet.source, "Eric's ACN Generator");

  uint8_t *p = (uint8_t*) &packet;
  for (int i = 0; i < sizeof(packet); i++)
  {
    fprintf(fp, "0x%02x,", p[i]);
    if ((i % bpl) == bpl-1)
      fprintf(fp, "\n");
  }
  
  fclose(fp);

  printf("size: %d\n", sizeof(packet));
  return 0;
}
