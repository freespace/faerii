/* Name: hidtool.c
 * Project: hid-data example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hiddata.h"
#include "../firmware/usbconfig.h"  /* for device VID, PID, vendor name and product name */
#include "../firmware/config.h"

/* ------------------------------------------------------------------------- */

static char *usbErrorMessage(int errCode)
{
static char buffer[80];

    switch(errCode){
        case USBOPEN_ERR_ACCESS:      return "Access to device denied";
        case USBOPEN_ERR_NOTFOUND:    return "The specified device was not found";
        case USBOPEN_ERR_IO:          return "Communication error with device";
        default:
            sprintf(buffer, "Unknown USB error %d", errCode);
            return buffer;
    }
    return NULL;    /* not reached */
}

static usbDevice_t  *openDevice(void)
{
usbDevice_t     *dev = NULL;
unsigned char   rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
char            vendorName[] = {USB_CFG_VENDOR_NAME, 0}, productName[] = {USB_CFG_DEVICE_NAME, 0};
int             vid = rawVid[0] + 256 * rawVid[1];
int             pid = rawPid[0] + 256 * rawPid[1];
int             err;

    if((err = usbhidOpenDevice(&dev, vid, vendorName, pid, productName, 0)) != 0){
        fprintf(stderr, "error finding %s: %s\n", productName, usbErrorMessage(err));
        return NULL;
    }
    return dev;
}

/* ------------------------------------------------------------------------- */

static void hexdump(char *buffer, int len)
{
int     i;
FILE    *fp = stdout;

    for(i = 0; i < len; i++){
        if(i != 0){
            if(i % 4 == 0){
                fprintf(fp, "\n");
            }else{
                fprintf(fp, " ");
            }
        }
        fprintf(fp, "0x%02x", buffer[i] & 0xff);
    }
    if(i != 0)
        fprintf(fp, "\n");
}

static int  hexread(char *buffer, char *string, int buflen)
{
char    *s;
int     pos = 0;

    while((s = strtok(string, ", ")) != NULL && pos < buflen){
        string = NULL;
        buffer[pos++] = (char)strtol(s, NULL, 0);
    }
    return pos;
}

/* ------------------------------------------------------------------------- */

static void usage(char *myName)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  %s read <bytes to read>\n", myName);
    fprintf(stderr, "  %s write <list of bytes separated by ,>\n", myName);
    fprintf(stderr, "  %s goto <block # as uint8> \n", myName);
    fprintf(stderr, "  %s restart\n", myName);
}

int main(int argc, char **argv)
{
usbDevice_t *dev;
// for now, we are holding to the per-transfer data limit of 254 bytes
char        buffer[254+1];    /* room for dummy report ID */
int         err;

    if(argc < 2){
        usage(argv[0]);
        exit(1);
    }
    if((dev = openDevice()) == NULL)
        exit(1);
    if(strcasecmp(argv[1], "read") == 0){
        if (argc < 3) {
          usage(argv[0]);
          exit(1);
        }

        int len=254;
        if (sscanf(argv[2], "%d", &len) != 1) {
          fprintf(stderr, "error parsing numeric argument %s\n", argv[2]);
          exit(1);
        }

        if (len>254) {
          fprintf(stderr, "request number of bytes exceeds 254\n");
          exit(1);
        }

        // remember we have to read in the dummy report ID
        len +=1;

        if((err = usbhidGetReport(dev, 0, buffer, &len)) != 0){
            fprintf(stderr, "error reading data: %s\n", usbErrorMessage(err));
        }else{
            hexdump(buffer + 1, len - 1);
        }
    }else if(strcasecmp(argv[1], "write") == 0){
        int i, pos;
        memset(buffer, 0, sizeof(buffer));
        for(pos = 2, i = 2; i < argc && pos < sizeof(buffer); i++){
            pos += hexread(buffer + pos, argv[i], sizeof(buffer) - pos);
        }
        hexdump(buffer, pos);
        buffer[1] = CMD_WRITE;
        if((err = usbhidSetReport(dev, buffer, pos)) != 0)
            fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
        else printf("wrote out %u bytes, %u was user data\n", pos, pos-2);
    } else if (strcasecmp(argv[1], "restart") == 0) {
      buffer[0] = 0;
      buffer[1] = CMD_RESTART;
      int len = 2;
      if((err = usbhidSetReport(dev, buffer, len)) != 0)
          fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
      else printf("sent RESTART command\n");
    } else if (strcasecmp(argv[1], "goto") == 0) {
      buffer[0] = 0;
      buffer[1] = CMD_GOTO;
      int n;
      if (sscanf(argv[2], "%d", &n) == 1) {
        buffer[2] = n&0xff;
        int len = 3;

        if((err = usbhidSetReport(dev, buffer, len)) != 0)
            fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
        else printf("sent GOTO command\n");
      } else {
        fprintf(stderr, "error parsing number argument: %s\n", argv[2]);
        exit(1);
      }
    }else{
        usage(argv[0]);
        exit(1);
    }
    usbhidCloseDevice(dev);
    return 0;
}

/* ------------------------------------------------------------------------- */
