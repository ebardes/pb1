#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "acn.h"

#define SET(t, n) _set(t, n, sizeof(t))

uint8_t pid[] = { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };
//uint8_t cid[] = { 0xe5, 0xd9, 0x88, 0x7c, 0x57, 0xfe, 0x9d, 0x3d, 0x08, 0xa7, 0x19, 0x29, 0x40, 0xa8, 0xe5, 0xd5 };
uint8_t cid[] = { 0xe5,0xd9,0x88,0x7c, 0x57,0xfe,0x9d,0x3d, 0x00,0x01, 0x00,0x01, 0x00,0x00,0x00,0x00 };

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
  SET(packet.rlp_flags_and_length, 0x726e);
  SET(packet.rlp_version, 0x0004); // E1.31 2009 spec
  memcpy(packet.acn_packet_id, pid, sizeof(pid));
  memcpy(packet.cid_sender_id, cid, sizeof(cid));

  SET(packet.efl_flags_and_length, 0x7258);
  SET(packet.efl_version, 0x0002);
  strncpy((char*)packet.source, "Status Console", sizeof(packet.source));

  SET(packet.dl_flags_and_length, 0x720B);
  SET(packet.dl_version, 0x02);
  SET(packet.dl_addr_type, 0xA1);
  SET(packet.addr_inc, 0x01);
  SET(packet.dmx_size, 0x201);

  // memset(packet.dmx_data, 0xff, sizeof(packet.dmx_data));

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
