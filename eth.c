/*
   Copyright 2013 Bardes Lighting, LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include <stdint.h>
#include <stdbool.h>
#include "acn.h"
#include "mac.h"
#include "w5100.h"

#define STATIC

const uint8_t raw_acn_packet[sizeof(struct E131_2009)] = {
#include "acnraw.h"
};

void ssi_setup(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  GPIOPinConfigure(GPIO_PA2_SSI0CLK);
  GPIOPinConfigure(GPIO_PA3_SSI0FSS);
  GPIOPinConfigure(GPIO_PA4_SSI0RX);
  GPIOPinConfigure(GPIO_PA5_SSI0TX);
  GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

  SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
  SSIEnable(SSI0_BASE);

  // Read any residual data from the SSI port.
  long unsigned dummy;
  while(SSIDataGetNonBlocking(SSI0_BASE, &dummy))
    ; // empty

  SSIEnable(SSI0_BASE);
}

static int ssi_xchg(int n)
{
  SSIDataPut(SSI0_BASE, n);
  while(SSIBusy(SSI0_BASE));
  long unsigned v;
  SSIDataGet(SSI0_BASE, &v);
  v &= 0x00FF;
  return v;
}

#ifdef DEBUG
STATIC void InitConsole(void)
{
  //
  // Enable GPIO port A which is used for UART0 pins.
  // TODO: change this to whichever GPIO port you are using.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  //
  // Configure the pin muxing for UART0 functions on port A0 and A1.
  // This step is not necessary if your part does not support pin muxing.
  // TODO: change this to select the port/pin you are using.
  //
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);

  //
  // Select the alternate (UART) function for these pins.
  // TODO: change this to select the port/pin you are using.
  //
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  //
  // Initialize the UART for console I/O.
  //
  UARTStdioInit(0);
}
#endif

void wiz_write(long address, long byte)
{
  ssi_xchg(0xF0);
  ssi_xchg((address >> 8) & 0xFF);
  ssi_xchg(address & 0xFF);
  ssi_xchg(byte);
}

void wiz_write16(long address, long word)
{
  wiz_write(address, (word >> 8) & 0xFF);
  wiz_write(address+1, word & 0xFF);
}

STATIC void wiz_write_buffer(long address, const unsigned char *buffer, int length)
{
  while (length--)
  {
    wiz_write(address++,*buffer++);
  }
}

static int wiz_read(long address)
{
  long unsigned dummy;
  while(SSIDataGetNonBlocking(SSI0_BASE, &dummy))
    ; // empty

  ssi_xchg(0x0F);
  ssi_xchg((address >> 8) & 0xFF);
  ssi_xchg(address & 0xFF);
  return ssi_xchg(0);
}

static int wiz_read16(long address)
{
  return (wiz_read(address) << 8) | wiz_read(address+1);
}

void wiz_setip(const unsigned char addr[4], const unsigned char mask[4], const unsigned char gate[4], const unsigned char mac[6])
{
  wiz_write_buffer(0x0001, gate, 4);
  wiz_write_buffer(0x0005, mask, 4);
  wiz_write_buffer(0x0009, mac, 6);
  wiz_write_buffer(0x000f, addr, 4);
}

void wiz_open(int sock, const unsigned char dest[], int port)
{
  int reg = W5100_SKT_BASE(sock);

  wiz_write(reg+W5100_CR_OFFSET, W5100_SKT_CR_CLOSE);

  while (wiz_read(reg+W5100_CR_OFFSET))  ; // empty loop

  wiz_write(reg+W5100_MR_OFFSET, W5100_SKT_MR_UDP | W5100_SKT_MR_MULTI); // udp | multi-cast mode
  wiz_write16(reg+W5100_PORT_OFFSET, port);

  wiz_write_buffer(reg+W5100_DIPR_OFFSET, dest, 4);
  for (int i = 0; i < 6; i++)
    wiz_write(reg+W5100_DHAR_OFFSET+i, 0xff);
  wiz_write16(reg+W5100_DPORT_OFFSET, port);

  wiz_write(reg+W5100_CR_OFFSET, W5100_SKT_CR_OPEN);

  while (wiz_read(reg+W5100_CR_OFFSET))  ; // empty loop

}

/*
 * Send a packet of data
 */
void wiz_send(int sock, const uint8_t*data, int length)
{
  if (length == 0 || data == 0)
    return;

  int reg = W5100_SKT_BASE(sock);
  int free;
  while ((free = wiz_read16(reg+W5100_TX_FSR_OFFSET)) < length)
  {
    SysCtlDelay(100);
  }
  
  int offset = wiz_read16(reg+W5100_TX_WR_OFFSET);
#ifdef DEBUG
  int initialoffset = offset;
#endif
  while (length--)
  {
    int realaddr = W5100_TXBUFADDR + (offset & W5100_TX_BUF_MASK);
    wiz_write(realaddr, *data++);
    offset++;
  }

  wiz_write16(reg+W5100_TX_WR_OFFSET, offset);
  wiz_write(reg+W5100_TTL_OFFSET, 8);
  wiz_write(reg+W5100_CR_OFFSET, W5100_SKT_CR_SEND);
  while (wiz_read(reg+W5100_CR_OFFSET))  ; // empty loop
  
#ifdef DEBUG
  UARTprintf("Packet: initialoffset=%04x offset=%04x length=%04x\n", initialoffset, offset, length);
#endif
}

const unsigned char mac[]  = { 0x90, 0xa2, 0xda, 0xd0, 0xf1, 0xfe };
const unsigned char ip[]   = { 192,168,1,133 };
const unsigned char mask[] = { 255,255,255,0 };
const unsigned char gate[] = { 192,168,1,15 };
const unsigned char dest[] = { 239,255,0, UNIVERSE };

void wiz_init()
{
#ifdef DEBUG
  UARTprintf("Config IP\n");
#endif

  wiz_setip(ip, mask, gate, mac);
  wiz_open(0, dest, 5568);
}

void acn_transmit(volatile struct E131_2009 *packet)
{
  wiz_send(0, (unsigned char*) packet, sizeof(*packet));
}
