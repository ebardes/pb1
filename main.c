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

#include "mac.h"

#define FADERS 75
#define BUMPS 24
#define KEYPAD 32
#define DEBUG

volatile uint32_t delay = 250;
volatile uint32_t millisec;
volatile uint32_t ms_delay;

#define SPI_SPEED 1000000

void SysTick_IntHandler(void)
{
  millisec++;
}

void setup()
{
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

  ms_delay = SysCtlClockGet() / 3000;

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

  // LED pins
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

  /*
   * LCD Control Pins
   *
   * PB4 - LCD4 - Register Select: 0 = Sel
   * PB6 - LCD5 - R/W: 1=Read/0=Write
   * PB7 - LCD6 - Enable H->L = latch
   */
#define LCD_PIN_RS  GPIO_PIN_4
#define LCD_PIN_RW  GPIO_PIN_6
#define LCD_PIN_EN  GPIO_PIN_7
#define LCD_PIN_ALL (GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7)

  GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, LCD_PIN_ALL);
  GPIOPadConfigSet(GPIO_PORTB_BASE, LCD_PIN_ALL, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

  lcd_init();

  /*
   * Bump sense pin and control pad sense pins
   *
   * PE2 = Bump Sense
   * PE4 = Column 3
   * PE5 = Column 2
   * PA6 = Column 1
   * PA7 = Column 0
   */
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5);
  GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

  GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

  /*
   * Setup the tick clock.
   */
  SysTickDisable();
  SysTickIntRegister(SysTick_IntHandler);
  SysTickPeriodSet(SysCtlClockGet()/1000);
  SysTickIntEnable();
  SysTickEnable();

  /*
   * ADC
   */
  GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

  ADCHardwareOversampleConfigure(ADC0_BASE, 64);
  ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
  ADCSequenceEnable(ADC0_BASE, 1);
  ADCIntClear(ADC0_BASE, 1);

  // Enable the GPIO pins for digital function.
  //
  GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
  GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, 0xF0);
  GPIOPadConfigSet(GPIO_PORTD_BASE, 0x0F, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOPadConfigSet(GPIO_PORTC_BASE, 0xF0, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

#ifdef DEBUG
  InitConsole();
#endif

#if NET
  SysCtlDelay(SysCtlClockGet()/4);
  SysCtlDelay(SysCtlClockGet()/4);
  SysCtlDelay(SysCtlClockGet()/4);

  /*
   * Freescale SPI Master
   */
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  GPIOPinConfigure(GPIO_PA2_SSI0CLK);
  GPIOPinConfigure(GPIO_PA3_SSI0FSS);
  GPIOPinConfigure(GPIO_PA4_SSI0RX);
  GPIOPinConfigure(GPIO_PA5_SSI0TX);
  GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

  SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
  SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, SPI_SPEED, 8);
  SSIEnable(SSI0_BASE);

  // Read any residual data from the SSI port.
  {
  long unsigned dummy;
  while(SSIDataGetNonBlocking(SSI0_BASE, &dummy))
    ; // empty
  }
  wiz_init();
#endif

//  SysCtlDelay(SysCtlClockGet()/4);

  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
}

#ifdef DEBUG
void InitConsole(void)
{
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTStdioConfig(0, 115200, SysCtlClockGet());
}
#endif

/*
 * Set the 8-bit data bus to x
 */
static inline void DataPins(uint8_t x)
{
  GPIOPinWrite(GPIO_PORTD_BASE, 0x0F, x & 0x0F);
  GPIOPinWrite(GPIO_PORTC_BASE, 0xF0, x & 0xF0);
}

static void lcd_cmd(uint8_t data)
{
  DataPins(data);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_RS, 0);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_ALL, LCD_PIN_EN);
  SysCtlDelay(10);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_EN, 0);
}

static void lcd_char(uint8_t data)
{
  DataPins(data);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_RS, LCD_PIN_RS);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_EN, LCD_PIN_EN);
  SysCtlDelay(10);
  GPIOPinWrite(GPIO_PORTB_BASE, LCD_PIN_EN, 0);
}

static void lcd_string(char *text)
{
  while (*text)
    lcd_char(*text++);
}

void lcd_init()
{
  SysCtlDelay(15 * ms_delay);
  lcd_cmd(0x30);         // command 0x30 = Wake up
  SysCtlDelay(15 * ms_delay);
  lcd_cmd(0x38);         // Function set: 8-bit/2-line
  SysCtlDelay(ms_delay);
  lcd_cmd(0x10);         // Set cursor
  SysCtlDelay(ms_delay);
  lcd_cmd(0x0c);         // Display ON; Cursor ON
  SysCtlDelay(ms_delay);
  lcd_cmd(0x06);         // Entry mode set
  SysCtlDelay(ms_delay);

  lcd_string("Bardes");
}

unsigned long ADC_In(void)
{
  uint32_t samples[4];
  uint32_t average = 0;

  ADCProcessorTrigger(ADC0_BASE, 1); //startsequence
  while(!ADCIntStatus(ADC0_BASE, 1, false))
    ;
  int n = ADCSequenceDataGet(ADC0_BASE, 1, samples);
  ADCIntClear(ADC0_BASE, 1);
  for (int i = 0; i < n; i++)
    average += samples[i];
  return average / n;
}

volatile int16_t fader_table[FADERS+BUMPS+KEYPAD];

#define SLOP 32

volatile struct E131_2009 packet;

int main(void)
{
  uint32_t frame = 0;

  IntMasterDisable();
  setup();
#if USB
  usb_init();
#endif
  memcpy((void*)&packet, (void*)raw_acn_packet, sizeof(packet));

  IntMasterEnable();
#ifdef DEBUG
  UARTprintf("Running\n");
#endif
  for (;;)
  {
    /*
     * This console has 75 faders and 24 bump buttons
     */
    for(int8_t i = 0; i < FADERS; i++)
    {
      if (i == 56) // bad fader
	continue;

      int8_t z;
      if (i < 72)
      {
	z = i >> 3;
	int8_t x = z % 3;
	int8_t y = z / 3;
	z = i % 8;
	z = z + 8 * (x*3+y);
      }
      else
      {
	z = i;
      }

      DataPins(i);

      // give the analog circuit time to settle
      SysCtlDelay(delay);

      int16_t level = ADC_In();
      int16_t diff =  fader_table[i] - level;
      if (diff > SLOP || diff < -SLOP)
      {
	fader_table[z] = level;
      }
      else
      {
	fader_table[z] = (fader_table[z] * 3 + level) / 4;
      }

      if (i < BUMPS)
      {
	if (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2))
	  fader_table[i+FADERS] = 0;
        else
	  fader_table[i+FADERS] = 4096;
      }
    }

    /*
     * Scan the control keypad one bit at a time.
     */
    uint8_t bit = ~1;
    for (int i = 0, tableoffset = BUMPS+FADERS; i < 8; i++, bit=(bit<<1)|0x01)
    {
      DataPins(bit);
      SysCtlDelay(delay);

      int scan = (( 
	  GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5) |
	  GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7)
	  )>>4) ^ 0x0f;

      fader_table[tableoffset++] = (scan & 0x08) ? 4096 : 0;
      fader_table[tableoffset++] = (scan & 0x04) ? 4096 : 0;
      fader_table[tableoffset++] = (scan & 0x02) ? 4096 : 0;
      fader_table[tableoffset++] = (scan & 0x01) ? 4096 : 0;
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
	UARTprintf("%d=%d\n", i+1, dmx);
#endif
	packet.dmx_data[i] = dmx;
      }
    }
    if (changed || frame == 0)
    {
      packet.seq_num[0]++;
#if NET
      acn_transmit(&packet);
#endif
    }

#if USB
    /*
     * Checkin on the USB subsystem
     */
    usb_tick();
#endif

    /*
     */
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, (frame & 0x01) << 1);
    frame++;
    if (frame >= 44)
      frame=0;
  }
}
