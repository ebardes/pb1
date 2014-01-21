#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/fpu.h"
#include "driverlib/adc.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

#ifndef _MAC_H
#define _MAC_H

#include "acn.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern void memcpy(uint8_t* dst, const uint8_t *src, long len);
extern void wiz_init(void);
extern void lcd_init(void);
extern void ssi_setup(void);
extern void usb_init(void );
extern void usb_tick(void );
extern void acn_transmit(volatile struct E131_2009 *packet);
extern void InitConsole(void);

#define UNIVERSE 10
#endif
