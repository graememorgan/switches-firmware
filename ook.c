/*
 * USB interface for controlling wireless sockets
 *
 * (c) Graeme Morgan 2015
 *
 * Based on:
 */

#include <delays.h>

#include "castoplug.h"
#include "spi.h"

// Microchip Application Library includes
#include "./USB/usb.h"
#include "./USB/usb_function_hid.h"

// Ensure we have the correct target PIC device family

// Define the globals for the USB data in the USB RAM of the PIC18F*53 * LEDS
#pragma udata USB_VARIABLES=0x500
unsigned char ReceivedDataBuffer[64];
unsigned char ToSendDataBuffer[64];

USB_HANDLE USBOutHandle = 0;
USB_HANDLE USBInHandle = 0;

// set up the clock. we use a 20 MHz external oscillator
#pragma config PLLDIV   = 5
#pragma config CPUDIV   = OSC1_PLL2
#pragma config USBDIV   = 2
#pragma config FOSC     = HSPLL_HS
// disable power-up timer
#pragma config PWRT     = OFF
// enable brown-out reset
#pragma config BOR      = ON
#pragma config BORV     = 3
// enable internal 3.3V regulator for USB
#pragma config VREGEN   = ON
// disable the watchdog timer
#pragma config WDT      = OFF
#pragma config WDTPS    = 32768
// enable the MCLR pin
#pragma config MCLRE    = ON
// use PORTB pins as digital IO
#pragma config PBADEN   = OFF
// disable single-supply programming
#pragma config LVP      = OFF

// Private function prototypes
void processUsbCommands(void);
void USBCBSendResume(void);

#pragma code

#define INDICATOR PORTAbits.RA1

void main(void) {
    // Configure ports as inputs (1) or outputs(0)
    TRISA = 0b00000000;
    TRISB = 0b00000001;
    TRISC = 0b00000000;


    // Initialize the variable holding the USB handle for the last transmission
    USBOutHandle = 0;
    USBInHandle = 0;

    // Initialise the USB device
    USBDeviceInit();

    INDICATOR = 0;

    while (TRUE) {
        USBDeviceTasks();

        if ((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl == 1)) {
            continue;
        }

        processUsbCommands();
    }
}

// Process USB commands
unsigned char gethexdigit(unsigned char d) {
    if (d >= 'a' && d <= 'f') return d - 'a' + 10;
    else if (d >= 'A' && d <= 'F') return d - 'A' + 10;
    else return d - '0';
}
unsigned char gethex(unsigned char* s) {
    return gethexdigit(s[0]) * 16 + gethexdigit(s[1]);
}

void processUsbCommands(void) {
    unsigned char i, d, l;

    if (!HIDRxHandleBusy(USBOutHandle)) {
        // light the indicator while processing USB input
        INDICATOR = 1;
        // Command mode
        if (ReceivedDataBuffer[0] == 'r') {
            ReceivedDataBuffer[3] = '\0';
            i = gethex(ReceivedDataBuffer + 1);
            d = rfm22_read_reg(i);
            if (!HIDTxHandleBusy(USBInHandle)) {
                l = sprintf(ToSendDataBuffer, "read %#02x = %#02x\n", i, d);
                USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer[0], l);
            }
        } else if (ReceivedDataBuffer[0] == 'w') {
            i = gethex(ReceivedDataBuffer + 1);
            d = gethex(ReceivedDataBuffer + 4);
            rfm22_write_reg(i, d);
            if (!HIDTxHandleBusy(USBInHandle)) {
                l = sprintf(ToSendDataBuffer, "wrote %#02x = %#02x\n", i, d);
                USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer[0], l);
            }
        }
        else if (ReceivedDataBuffer[0] == 's') {
            castoplug_setup();
            // the receivers seem to actually NEED the data to be repeated.
            // I've not had them work, even unreliably, with just one
            // send. they work (badly) with two.
            for (i = 0; i < 5; i++) {
                castoplug_send(ReceivedDataBuffer[1] - 'a', ReceivedDataBuffer[2] - '1', ReceivedDataBuffer[3] - '0');
                // the length of the delay here also seems somewhat critical--
                // they didn't work with just 2ms.
                delay_ms(5);
            }
            // reply just to let the driver confirm that we did something.
            if (!HIDTxHandleBusy(USBInHandle)) {
                l = sprintf(ToSendDataBuffer, "switch %c%c=%c\n", ReceivedDataBuffer[1], ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&ToSendDataBuffer[0], l);
            }
        } else if (ReceivedDataBuffer[0] == '\0') {
            //noop
        }
        INDICATOR = 0;

        // Re-arm the OUT endpoint for the next packet
        USBOutHandle = HIDRxPacket(HID_EP, (BYTE*) & ReceivedDataBuffer, 64);
    }
}

// USB callbacks

void USBCBSuspend(void) {
}

void USBCBWakeFromSuspend(void) {
}

void USBCB_SOF_Handler(void) {
}

void USBCBErrorHandler(void) {
}


void USBCBCheckOtherReq(void) {
    USBCheckHIDRequest();
}

void USBCBStdSetDscHandler(void) {
}

void USBCBInitEP(void) {
    // Enable the HID endpoint
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);

    // Re-arm the OUT endpoint for the next packet
    USBOutHandle = HIDRxPacket(HID_EP, (BYTE*) & ReceivedDataBuffer, 64);
}

// Send resume call-back

void USBCBSendResume(void) {
    static WORD delay_count;

    // Verify that the host has armed us to perform remote wakeup.
    if (USBGetRemoteWakeupStatus()) {
        // Verify that the USB bus is suspended (before we send remote wakeup signalling).
        if (USBIsBusSuspended()) {
            USBMaskInterrupts();

            // Bring the clock speed up to normal running state
            USBCBWakeFromSuspend();
            USBSuspendControl = 0;
            USBBusIsSuspended = FALSE;

            // Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            // device must continuously see 5ms+ of idle on the bus, before it sends
            // remote wakeup signalling.  One way to be certain that this parameter
            // gets met, is to add a 2ms+ blocking delay here (2ms plus at
            // least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            // 5ms+ total delay since start of idle).
            delay_count = 3600U;
            do {
                delay_count--;
            } while (delay_count);

            // Start RESUME signaling for 1-13 ms
            USBResumeControl = 1;
            delay_count = 1800U;
            do {
                delay_count--;
            } while (delay_count);
            USBResumeControl = 0;

            USBUnmaskInterrupts();
        }
    }
}

// USB callback function handler

BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size) {
    switch (event) {
        case EVENT_TRANSFER:
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_CONFIGURED:
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        default:
            break;
    }
    return TRUE;
}
