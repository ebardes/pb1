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

#include "inc/lm4f120h5qr.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include <stdint.h>

#include "acn.h"
#include "mac.h"

#define FADERS 75
#define BUMPS 24
#define DEBUG

volatile uint32_t time;
volatile uint32_t millisec;
volatile uint32_t delay = 1000;

void SysTick_IntHandler(void)
{
  millisec++;
}

void setup()
{
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

  SysCtlDelay(10);

  // LED pins
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  SysCtlDelay(20);


  /*
   * Bump sense pin
   */
  GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_7);
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2);

  GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

  /*
   * Setup the tick clock.
  SysTickDisable();
  SysTickIntRegister(SysTick_IntHandler);
  SysTickPeriodSet(SysCtlClockGet()/1000);
  SysTickIntEnable();
  SysTickEnable();
   */

#ifdef UART1
  /*
   * UART Init
   */
  GPIOPinConfigure(GPIO_PE1_U7TX);
  UARTConfigSetExpClk(UART7_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);
#endif

  /*
   * ADC
   */
  GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
  ADCHardwareOversampleConfigure(ADC0_BASE, 64); // Hardware averaging. ( 2, 4, 8, 16, 32, 64 )
  SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); //ADC Sample Rate set to 250 Kilo Samples Per Second


  // Enable the GPIO pins for digital function.
  //
  GPIO_PORTD_DIR_R = 0x0F;
  GPIO_PORTD_DEN_R = 0x0F;
  GPIO_PORTC_DIR_R = 0xF0;
  GPIO_PORTC_DEN_R = 0xF0;
  GPIOPadConfigSet(GPIO_PORTD_BASE, 0x0F, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOPadConfigSet(GPIO_PORTC_BASE, 0xF0, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

#ifdef DEBUG
  InitConsole();
#endif
  ssi_setup();

  // wiz_init();

  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
}

#ifdef DEBUG
void InitConsole(void)
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

#ifdef UART1
static void uart_putchar(uint8_t ch)
{
    UARTCharPutNonBlocking(UART7_BASE, ch);
    // UARTCharPut(UART7_BASE, ch);
}

static void write(char*str)
{
  while (*str)
    uart_putchar(*str++);
}

static void uart_hex8(uint8_t b)
{
  unsigned char z;

  z = ((b & 0xf0) >> 4) + '0';
  if (z > '9')
    z += 7;
  uart_putchar(z);

  z = (b & 0x0f) + '0';
  if (z > '9')
    z += 7;
  uart_putchar(z);
}

static void uart_hex16(uint16_t w)
{
  uart_hex8(w >> 8);
  uart_hex8(w & 0xff);
}
#endif

unsigned long ADC_In(int channel)
{
  unsigned long temp [8];
  unsigned long average = 0;
  char samples;
  int i;

  ADCSequenceDisable(ADC0_BASE, 1);
  ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 1);
  ADCSequenceStepConfigure(ADC0_BASE, 1, 0, channel | ADC_CTL_END);
  ADCSequenceEnable(ADC0_BASE, 1);
  
  ADCProcessorTrigger(ADC0_BASE, 1); //startsequence
  do {
     samples = ADCSequenceDataGet(ADC0_BASE, 1, temp);
     } while(!samples);
  
  for(i=0; i<samples; i++)
  {
    average += temp[i];
  }
  return (average/(samples));
}

volatile int16_t fader_table[FADERS+BUMPS];

#define SLOP 32

volatile struct E131_2009 packet;

int main(void)
{
  IntMasterDisable();
  setup();
  memcpy((void*)&packet, (void*)raw_acn_packet, sizeof(packet));

  IntMasterEnable();
  UARTprintf("Running\n");

  for (;;)
  {
    /*
     * This console has 75 faders and 24 bump buttons
     */
    for(int8_t i = 0; i < FADERS; i++)
    {
      // GPIOPinWrite(GPIO_PORTD_BASE, 0x0F, i & 0x0F);
      // GPIOPinWrite(GPIO_PORTC_BASE, 0xF0, i & 0xF0);

      GPIO_PORTD_DATA_R = (GPIO_PORTD_DATA_R & 0xF0) | (i & 0x0F);
      GPIO_PORTC_DATA_R = (GPIO_PORTC_DATA_R & 0x0F) | (i & 0xF0);

      // give the analog circuit time to settle
      SysCtlDelay(delay);

      ADC_In(0); // read twice - toss one away
      int16_t level = ADC_In(0);

      int16_t diff =  fader_table[i] - level;
      if (diff > SLOP || diff < -SLOP)
      {
	fader_table[i] = level;
      }
      else
      {
	fader_table[i] = (fader_table[i] * 3 + level) / 4;
      }

      if (i < BUMPS)
      {
	if (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2))
	  fader_table[i+FADERS] = 0;
        else
	  fader_table[i+FADERS] = 4096;
      }
    }

    packet.universe[1] = UNIVERSE;

    int8_t changed = 0;
    for (int i = 0; i < ARRAY_SIZE(fader_table); i++)
    {
      uint8_t dmx;
      long value = fader_table[i] - 128;
      if (value < 0) {
	dmx = 0;
      } else if (value > 4095-256) {
	dmx = 255;
      } else {
	dmx = value * 256 / (4095-256); // scale 12-bit value to 8-bit across an effective range that buffers the top and bottom
      }

      if (packet.dmx_data[i] != dmx)
      {
	changed=1;
#ifdef DEBUG
	UARTprintf("%2d=%3d\n", i+1, dmx);
#endif
	packet.dmx_data[i] = dmx;
      }
    }
    if (changed || time == 0)
    {
      packet.seq_num[0]++;
//      acn_transmit(&packet);
    }
    // SysCtlSleep();

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, (time & 0x01) << 1);
    time++;
    if (time >= 44)
      time=0;
  }

  return 0;
}
