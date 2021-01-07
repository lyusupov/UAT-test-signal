/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Copyright (C) 2018-2021 Linar Yusupov
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***** Includes *****/
/* Standard C Libraries */
#include <stdlib.h>
#include <unistd.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)

/* Board Header files */
#include "Board.h"
#include "smartrf_settings/smartrf_settings.h"

#include "UAT_data.h"

#include "fec/char.h"
#include "fec/rs-common.h"

/***** Defines *****/

#define SHORT_FRAME_DATA_BITS (144)
#define SHORT_FRAME_BITS (SHORT_FRAME_DATA_BITS+96)
#define SHORT_FRAME_DATA_BYTES (SHORT_FRAME_DATA_BITS/8)
#define SHORT_FRAME_BYTES (SHORT_FRAME_BITS/8)

#define LONG_FRAME_DATA_BITS (272)
#define LONG_FRAME_BITS (LONG_FRAME_DATA_BITS+112)
#define LONG_FRAME_DATA_BYTES (LONG_FRAME_DATA_BITS/8)
#define LONG_FRAME_BYTES (LONG_FRAME_BITS/8)

#define UPLINK_BLOCK_DATA_BITS (576)
#define UPLINK_BLOCK_BITS (UPLINK_BLOCK_DATA_BITS+160)
#define UPLINK_BLOCK_DATA_BYTES (UPLINK_BLOCK_DATA_BITS/8)
#define UPLINK_BLOCK_BYTES (UPLINK_BLOCK_BITS/8)

#define UPLINK_FRAME_BLOCKS (6)
#define UPLINK_FRAME_DATA_BITS (UPLINK_FRAME_BLOCKS * UPLINK_BLOCK_DATA_BITS)
#define UPLINK_FRAME_BITS (UPLINK_FRAME_BLOCKS * UPLINK_BLOCK_BITS)
#define UPLINK_FRAME_DATA_BYTES (UPLINK_FRAME_DATA_BITS/8)
#define UPLINK_FRAME_BYTES (UPLINK_FRAME_BITS/8)

#define UAT_SYNCWORD_LSB    ((unsigned char) 0xe2)

/* Packet TX Configuration */
#define PAYLOAD_LENGTH      (LONG_FRAME_BYTES + sizeof(UAT_SYNCWORD_LSB))

#define PACKET_INTERVAL     999000     /* Set packet interval to 1000000us or 1000ms */
//#define PACKET_INTERVAL     500000    /* Set packet interval to 500000us or 500ms */
//#define PACKET_INTERVAL     200000    /* Set packet interval to 200000us or 200ms */

/***** Prototypes *****/

/***** Variable declarations *****/

static uint8_t packet[UPLINK_BLOCK_BYTES];

static RF_Object rfObject;
static RF_Handle rfHandle;

/* Pin driver handle */
static PIN_Handle ledPinHandle;
static PIN_State ledPinState;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] =
{
    Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
#if defined Board_CC1352R1_LAUNCHXL
    Board_DIO30_RFSW | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
#endif
#ifdef POWER_MEASUREMENT
#if defined(Board_CC1350_LAUNCHXL)
    Board_DIO30_SWPWR | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
#endif
#endif
    PIN_TERMINATE
};

/***** Function definitions *****/

static int hexbyte(const char *buf)
{
    int i;
    char c;

    c = buf[0];
    if (c >= '0' && c <= '9')
        i = (c - '0');
    else if (c >= 'a' && c <= 'f')
        i = (c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
        i = (c - 'A' + 10);
    else
        return -1;

    i <<= 4;
    c = buf[1];
    if (c >= '0' && c <= '9')
        return i | (c - '0');
    else if (c >= 'a' && c <= 'f')
        return i | (c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
        return i | (c - 'A' + 10);
    else
        return -1;
}

static void hex_to_bytes(const char *s, uint8_t *to)
{
    for (; *s; s += 2)
        *to++ = (uint8_t) hexbyte(s);
}

void free_rs_char(void *p){
  struct rs *rs = (struct rs *)p;

  free(rs->alpha_to);
  free(rs->index_of);
  free(rs->genpoly);
  free(rs);
}

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 * pad = padding bytes at front of shortened block
 */
void *init_rs_char(int symsize,int gfpoly,int fcr,int prim,
	int nroots,int pad){
  struct rs *rs;

#include "fec/init_rs.h"

  return rs;
}

void encode_rs_char(void *p,data_t *data, data_t *parity){
  struct rs *rs = (struct rs *)p;

#include "fec/encode_rs.h"

}

int decode_rs_char(void *p, data_t *data, int *eras_pos, int no_eras){
  int retval;
  struct rs *rs = (struct rs *)p;

#include "fec/decode_rs.h"

  return retval;
}

static void *rs_uplink;
static void *rs_adsb_short;
static void *rs_adsb_long;

#define UPLINK_POLY 0x187
#define ADSB_POLY 0x187

void init_fec(void)
{
    rs_adsb_short = init_rs_char(8, /* gfpoly */ ADSB_POLY, /* fcr */ 120, /* prim */ 1, /* nroots */ 12, /* pad */ 225);
    rs_adsb_long  = init_rs_char(8, /* gfpoly */ ADSB_POLY, /* fcr */ 120, /* prim */ 1, /* nroots */ 14, /* pad */ 207);
    rs_uplink     = init_rs_char(8, /* gfpoly */ UPLINK_POLY, /* fcr */ 120, /* prim */ 1, /* nroots */ 20, /* pad */ 163);
}

void *mainThread(void *arg0)
{
    RF_Params rfParams;
    RF_Params_init(&rfParams);

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, pinTable);
    if (ledPinHandle == NULL)
    {
        while(1);
    }

    RF_cmdPropTx.pktLen = PAYLOAD_LENGTH;
    RF_cmdPropTx.pPkt = packet;
    RF_cmdPropTx.startTrigger.triggerType = TRIG_NOW;

    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);

    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

    init_fec();
    packet[0] = UAT_SYNCWORD_LSB;
    hex_to_bytes(UAT_DATA, &packet[1]);
    void *rs = rs_adsb_long;

    encode_rs_char(rs, &packet[1], &packet[LONG_FRAME_DATA_BYTES+sizeof(UAT_SYNCWORD_LSB)]);

    while(1)
    {
        /* Send packet */
        RF_EventMask terminationReason = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTx,
                                                   RF_PriorityNormal, NULL, 0);

        switch(terminationReason)
        {
            case RF_EventLastCmdDone:
                // A stand-alone radio operation command or the last radio
                // operation command in a chain finished.
                break;
            case RF_EventCmdCancelled:
                // Command cancelled before it was started; it can be caused
            // by RF_cancelCmd() or RF_flushCmd().
                break;
            case RF_EventCmdAborted:
                // Abrupt command termination caused by RF_cancelCmd() or
                // RF_flushCmd().
                break;
            case RF_EventCmdStopped:
                // Graceful command termination caused by RF_cancelCmd() or
                // RF_flushCmd().
                break;
            default:
                // Uncaught error event
                while(1);
        }

        uint32_t cmdStatus = ((volatile RF_Op*)&RF_cmdPropTx)->status;
        switch(cmdStatus)
        {
            case PROP_DONE_OK:
                // Packet transmitted successfully
                break;
            case PROP_DONE_STOPPED:
                // received CMD_STOP while transmitting packet and finished
                // transmitting packet
                break;
            case PROP_DONE_ABORT:
                // Received CMD_ABORT while transmitting packet
                break;
            case PROP_ERROR_PAR:
                // Observed illegal parameter
                break;
            case PROP_ERROR_NO_SETUP:
                // Command sent without setting up the radio in a supported
                // mode using CMD_PROP_RADIO_SETUP or CMD_RADIO_SETUP
                break;
            case PROP_ERROR_NO_FS:
                // Command sent without the synthesizer being programmed
                break;
            case PROP_ERROR_TXUNF:
                // TX underflow observed during operation
                break;
            default:
                // Uncaught error event - these could come from the
                // pool of states defined in rf_mailbox.h
                while(1);
        }

#ifndef POWER_MEASUREMENT
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED1,!PIN_getOutputValue(Board_PIN_LED1));
#endif        
        /* Power down the radio */
        RF_yield(rfHandle);

        /* Sleep for PACKET_INTERVAL us */
        usleep(PACKET_INTERVAL);

    }
}
