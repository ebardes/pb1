//*****************************************************************************
//
// usb_host_mouse.c - main application code for the host mouse example.
//
// Copyright (c) 2012-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.0.1.11577 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhub.h"
#include "usblib/host/usbhhidmouse.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Mouse Host (usb_host_mouse)</h1>
//!
//! This application demonstrates the handling of a USB mouse attached to the
//! evaluation kit.  Once attached, the position of the mouse pointer and the
//! state of the mouse buttons are output to the display.
//
//*****************************************************************************

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128
#define HUB_POOL_SIZE         (MAX_USB_DEVICES * HCD_MEMORY_SIZE)

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pui8HCDPool[HCD_MEMORY_SIZE];
uint8_t g_pui8HubPool[HUB_POOL_SIZE];

//*****************************************************************************
//
// The size of the mouse device interface's memory pool in bytes.
//
//*****************************************************************************
#define MOUSE_MEMORY_SIZE       128

//*****************************************************************************
//
// The memory pool to provide to the mouse device.
//
//*****************************************************************************
uint8_t mouseBuffer[MOUSE_MEMORY_SIZE];

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Mouse class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHIDClassDriver,
    &g_sUSBHubClassDriver,
    &g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The global value used to store the mouse instance value.
//
//*****************************************************************************
static tUSBHMouse *g_psMouseInstance;
static tHubInstance *g_psHubInstance;

//*****************************************************************************
//
// The global values used to store the mouse state.
//
//*****************************************************************************
static uint32_t g_ui32Buttons;
static int32_t g_i32DeltaX;
static int32_t g_i32DeltaY;

//*****************************************************************************
//
// The current USB operating mode - Host, Device or unknown.
//
//*****************************************************************************
tUSBMode g_eCurrentUSBMode;

//*****************************************************************************
//
// This enumerated type is used to hold the states of the mouse.
//
//*****************************************************************************
enum
{
    STATE_NO_DEVICE,
    STATE_MOUSE_INIT,
    STATE_MOUSE_CONNECTED,
    STATE_UNKNOWN_DEVICE,
    STATE_POWER_FAULT,
}
g_eMouseState;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mouse device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
//*****************************************************************************
void USBHCDEvents(void *pvData)
{
    const tEventInfo *pEventInfo = (tEventInfo *)pvData;

#ifdef DEBUG
    UARTprintf("Event: class %d, event %d\n", USBHCDDevClass(pEventInfo->ui32Instance, 0), pEventInfo->ui32Event);
#endif

    switch(pEventInfo->ui32Event)
    {
        //
        // New mouse detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Mouse.
            //
            if((USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_HID) && (USBHCDDevProtocol(pEventInfo->ui32Instance, 0) == USB_HID_PROTOCOL_MOUSE))
            {
#ifdef DEBUG
		UARTprintf("Mouse Connected\n");
#endif
                //
                // Proceed to the STATE_MOUSE_INIT state so that the main loop
                // can finish initialized the mouse since USBHMouseInit()
                // cannot be called from within a callback.
                //
                g_eMouseState = STATE_MOUSE_INIT;
            }
	    else if((USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_HUB))
	    {
#ifdef DEBUG
		UARTprintf("Hub Connected\n");
#endif
	    }
	    else
	    {
#ifdef DEBUG
		UARTprintf("Other Connected\n");
#endif
	    }

            break;
        }
        //
        // Unsupported device detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
#ifdef DEBUG
	    UARTprintf("Other Connected: class %d\n", USBHCDDevClass(pEventInfo->ui32Instance, 0));
#endif
            //
            // An unknown device was detected.
            //
            g_eMouseState = STATE_UNKNOWN_DEVICE;

            break;
        }
        //
        // Device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
#ifdef DEBUG
	    UARTprintf("Disconnected\n");
#endif
            //
            // Change the state so that the main loop knows that the device is
            // no longer present.
            //
            g_eMouseState = STATE_NO_DEVICE;

            //
            // Reset the button state.
            //
            g_ui32Buttons = 0;

            break;
        }
        //
        // Power Fault has occurred.
        //
        case USB_EVENT_POWER_FAULT:
        {
            //
            // No power means no device is present.
            //
            g_eMouseState = STATE_POWER_FAULT;

            break;
        }

        default:
        {
#ifdef DEBUG
	    UARTprintf("Unknown event: %d\n", pEventInfo->ui32Event);
#endif
            break;
        }
    }
}


//*****************************************************************************
//
// USB Mode callback
//
// \param ui32Index is the zero-based index of the USB controller making the
//        callback.
// \param eMode indicates the new operating mode.
//
// This function is called by the USB library whenever an OTG mode change
// occurs and, if a connection has been made, informs us of whether we are to
// operate as a host or device.
//
// \return None.
//
//*****************************************************************************
void ModeCallback(uint32_t ui32Index, tUSBMode eMode)
{
    g_eCurrentUSBMode = eMode;
}

void HubCallback(tHubInstance *psMsInstance, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData)
{
#ifdef DEBUG
    UARTprintf("Hub Callback: %d\n", ui32Event);
#endif
    switch(ui32Event)
    {
    }
}

//*****************************************************************************
//
// This is the callback from the USB HID mouse handler.
//
// \param psMsInstance is ignored by this function.
// \param ui32Event is one of the valid events for a mouse device.
// \param ui32MsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a mouse has
// been plugged in or removed and any time mouse movement or button pressed
// is detected.
//
// \return None.
//
//*****************************************************************************
void MouseCallback(tUSBHMouse *psMsInstance, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData)
{
    switch(ui32Event)
    {
        //
        // Mouse button press detected.
        //
        case USBH_EVENT_HID_MS_PRESS:
	    UARTprintf("md%d\n", ui32MsgParam);
            break;

        //
        // Mouse button release detected.
        //
        case USBH_EVENT_HID_MS_REL:
	    UARTprintf("mu%d\n", ui32MsgParam);
	    break;

        //
        // Mouse X movement detected.
        //
        case USBH_EVENT_HID_MS_X:
	    UARTprintf("mx%d\n", ui32MsgParam);
            break;

        //
        // Mouse Y movement detected.
        //
        case USBH_EVENT_HID_MS_Y:
	    UARTprintf("my%d\n", ui32MsgParam);
            break;
    }
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
void usb_init(void)
{
    //
    // Enable Clocking to the USB controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    SysCtlUSBPLLEnable();

    GPIOPinConfigure(GPIO_PC7_USB0PFLT);  
    GPIOPinConfigure(GPIO_PC6_USB0EPEN); 

    //
    // Configure the required pins for USB operation.
    //
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Initially wait for device connection.
    //
    g_eMouseState = STATE_NO_DEVICE;

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, eUSBModeHost, ModeCallback);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Initialized the cursor.
    //
    g_ui32Buttons = 0;
    g_i32DeltaX = 0;
    g_i32DeltaY = 0;

    //
    // Open an instance of the mouse driver.  The mouse does not need
    // to be present at this time, this just saves a place for it and allows
    // the applications to be notified when a mouse is present.
    //
    g_psMouseInstance = USBHMouseOpen(MouseCallback, mouseBuffer, MOUSE_MEMORY_SIZE);

    g_psHubInstance = USBHHubOpen(HubCallback);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Initialize the USB controller for OTG operation with a 2ms polling
    // rate.
    //
    USBHCDInit(0, g_pui8HCDPool, HCD_MEMORY_SIZE);
}

void usb_tick()
{
    //
    // Tell the OTG state machine how much time has passed in
    // milliseconds since the last call.
    //
    USBHCDMain();

    switch(g_eMouseState)
    {
	//
	// This state is entered when the mouse is first detected.
	//
	case STATE_MOUSE_INIT:
	{
	    //
	    // Initialize the newly connected mouse.
	    //
	    USBHMouseInit(g_psMouseInstance);

	    //
	    // Proceed to the mouse connected state.
	    //
	    g_eMouseState = STATE_MOUSE_CONNECTED;

	    break;
	}

	case STATE_MOUSE_CONNECTED:
	{
	    //
	    // Nothing is currently done in the main loop when the mouse
	    // is connected.
	    //
	    break;
	}

	case STATE_NO_DEVICE:
	{
	    //
	    // The mouse is not connected so nothing needs to be done here.
	    //
	    break;
	}

	default:
	{
	    break;
	}
    }
}
