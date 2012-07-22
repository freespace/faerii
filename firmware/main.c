/* Name: main.c
 * Project: hid-data, example how to use HID for data transfer
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id$
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/eeprom.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */

#include "usbdrv.h"

#include "config.h"
#include "types.h"
#include "ctrBlock.h"
#include "rgb.h"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char const usbHidReportDescriptor[22] = {    /* USB report descriptor */
  0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
  0x09, 0x01,                    // USAGE (Vendor Usage 1)
  0xa1, 0x01,                    // COLLECTION (Application)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
  0x75, 0x08,                    //   REPORT_SIZE (8)
  0x95, 0x80,                    //   REPORT_COUNT (128)
  0x09, 0x00,                    //   USAGE (Undefined)
  0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
  0xc0                           // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* The following variables store the status of the current data transfer */
static uchar  currentAddress;
static uchar  bytesRemaining;
static uchar  command;
/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionRead(uchar *data, uchar len) {
  if(len > bytesRemaining) len = bytesRemaining;

  eeprom_read_block(data, (uchar *)0 + currentAddress, len);
  currentAddress += len;
  bytesRemaining -= len;

  return len;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionWrite(uchar *data, uchar len) {
  if (command == NO_CMD) {
    command = data[0];
    data += 1;
    len -= 1;
  }

  if (command == WRITE_CMD) {
    if(bytesRemaining) {
      if(len > bytesRemaining)
        len = bytesRemaining;

      eeprom_write_block(data, (uchar *)0 + currentAddress, len);
      currentAddress += len;
      bytesRemaining -= len;
    }

    return bytesRemaining == 0; /* return 1 if this was the last chunk */
  } else if (command == GOTO_CMD) {
    // there should one data byte
    if (len == 1) return ctrBlockGoto(data[0]) == NULL;
    else return 1;
  } else if (command == RESTART_CMD) {
    // there should be no more data bytes
    if (len == 0) {
      rgbSetup();
      return 0;
    } else return 1;
  } else return 1;
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t *rq = (void *)data;

  // XXX as per documented in usbdrv.h, the maximum data we can
  // transfer per exchange is 254 bytes. We thus truncate
  // bytesRemaining if it is over this limit
  if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {  /* HID class request */
    if(rq->bRequest == USBRQ_HID_GET_REPORT) {  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
      bytesRemaining = rq->wLength.bytes[0];
      if (bytesRemaining == 255) bytesRemaining = 254;
      currentAddress = 0;
      return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
    } else if(rq->bRequest == USBRQ_HID_SET_REPORT) {
      bytesRemaining = rq->wLength.bytes[0];
      if (bytesRemaining == 255) bytesRemaining = 254;
      currentAddress = 0;

      // decrement bytesRemaining because command is eating up one byte
      bytesRemaining -= 1;
      command = NO_CMD;
      return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
    }
  } else {
    /* ignore vendor type requests, we don't use any */
  }
  return 0;
}

/* ------------------------------------------------------------------------- */

int main(void) {
  uchar   i;

  wdt_enable(WDTO_1S);
  /* Even if you don't use the watchdog, turn it off here. On newer devices,
   * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
   */
  /* RESET status: all port bits are inputs without pull-up.
   * That's the way we need D+ and D-. Therefore we don't need any
   * additional hardware initialization.
   */
  usbInit();
  usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */

  for(i=0;--i;) {       /* fake USB disconnect for > 250 ms */
    wdt_reset();
    _delay_ms(1);
  }

  // might as well take advantage of the fact
  // and do some setup here
  rgbSetup();

  usbDeviceConnect();

  sei();

  for(;;) {        /* main event loop */
    usbPoll();
    wdt_reset();
    usbPoll();
    rgbPoll();
  }
  return 0;
}

/* ------------------------------------------------------------------------- */
#define abs(x) ((x) > 0 ? (x) : (-x))

// Called by V-USB after device reset
void hadUsbReset() {
  int frameLength, targetLength = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
  int bestDeviation = 9999;
  uchar trialCal, bestCal, step, region, hasBestCal;


  // do a binary search in regions 0-127 and 128-255 to get optimum OSCCAL
  for(region = hasBestCal = 0; region <= 1; region++) {
    frameLength = 0;
    trialCal = (region == 0) ? 0 : 128;

    for(step = 64; step > 0; step >>= 1) {
      if(frameLength < targetLength) // true for initial iteration
        trialCal += step; // frequency too low
      else
        trialCal -= step; // frequency too high

      OSCCAL = trialCal;
      frameLength = usbMeasureFrameLength();

      if(abs(frameLength-targetLength) < bestDeviation) {
        bestCal = trialCal; // new optimum found
        bestDeviation = abs(frameLength -targetLength);
      }
    }
  }

  if (hasBestCal) OSCCAL = bestCal;
}
#undef abs