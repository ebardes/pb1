
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern void memcpy(uint8_t* dst, const uint8_t *src, long len);
extern void wiz_init(void);
extern void ssi_setup(void);
extern void acn_transmit(volatile struct E131_2009 *packet);
extern void InitConsole(void);

#define UNIVERSE 10
