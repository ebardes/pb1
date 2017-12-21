#ifndef _ACN_H_
#define _ACN_H_

#define DMX_CHANNELS 512

struct E131_2009
{
  /*
   * Root layer
   */
  uint8_t preamble_size		[2];
  uint8_t postamble_size	[2];
  uint8_t acn_packet_id		[12];
  uint8_t rlp_flags_and_length	[2];
  uint8_t rlp_version		[4];
  uint8_t cid_sender_id		[16];

  /*
   * E1.31 Framing layer
   */
  uint8_t efl_flags_and_length	[2];
  uint8_t efl_version		[4];
  uint8_t source		[64];
  uint8_t priority		[1];
  uint8_t reserved		[2];
  uint8_t seq_num		[1];
  uint8_t efl_options		[1];
  uint8_t universe		[2];

  /*
   * DMP Layer
   */
  uint8_t dl_flags_and_length	[2];
  uint8_t dl_version		[1];
  uint8_t dl_addr_type		[1];
  uint8_t first_addr		[2];
  uint8_t addr_inc		[2];
  uint8_t dmx_size		[2];
  uint8_t dmx_start_code	[1];
  uint8_t dmx_data		[DMX_CHANNELS];
};

extern const uint8_t raw_acn_packet[sizeof(struct E131_2009)];
#endif
