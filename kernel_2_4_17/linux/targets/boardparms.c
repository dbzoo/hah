/*
<:copyright-gpl 

 Copyright 2003 Broadcom Corp. All Rights Reserved. 
 
 This program is free software; you can distribute it and/or modify it 
 under the terms of the GNU General Public License (Version 2) as 
 published by the Free Software Foundation. 
 
 This program is distributed in the hope it will be useful, but WITHOUT 
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 for more details. 
 
 You should have received a copy of the GNU General Public License along 
 with this program; if not, write to the Free Software Foundation, Inc., 
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA. 

:>
*/
/**************************************************************************
 * File Name  : boardparms.c
 *
 * Description: This file contains the implementation for the BCM63xx board
 *              parameter access functions.
 * 
 * Updates    : 07/14/2003  Created.
 ***************************************************************************/

/* Includes. */
#include "boardparms.h"

/* Defines. */

/* Maximum number of Ethernet MACs. */
#define BP_MAX_ENET_MACS                        2

/* Typedefs */
typedef struct boardparameters
{
    char szBoardId[BP_BOARD_ID_LEN];        /* board id string */
    ETHERNET_MAC_INFO EnetMacInfos[BP_MAX_ENET_MACS];
    unsigned short usSdramSize;             /* SDRAM size and type */
    unsigned short usPsiSize;               /* persistent storage in K bytes */
    unsigned short usGpioRj11InnerPair;     /* GPIO pin or not defined */
    unsigned short usGpioRj11OuterPair;     /* GPIO pin or not defined */
    unsigned short usGpioPressAndHoldReset; /* GPIO pin or not defined */
    unsigned short usGpioVoipReset;         /* GPIO pin or not defined */
    unsigned short usGpioVoipIntr;          /* GPIO pin or not defined */
    unsigned short usGpioPcmciaReset;       /* GPIO pin or not defined */
    unsigned short usGpioUartRts;           /* GPIO pin or not defined */
    unsigned short usGpioUartCts;           /* GPIO pin or not defined */
    unsigned short usGpioLedAdsl;           /* GPIO pin or not defined */
    unsigned short usGpioLedAdslFail;       /* GPIO pin or not defined */
    unsigned short usGpioLedWireless;       /* GPIO pin or not defined */
    unsigned short usGpioLedUsb;            /* GPIO pin or not defined */
    unsigned short usGpioLedHpna;           /* GPIO pin or not defined */
    unsigned short usGpioLedWanData;        /* GPIO pin or not defined */
    unsigned short usGpioLedPpp;            /* GPIO pin or not defined */
    unsigned short usGpioLedPppFail;        /* GPIO pin or not defined */
    unsigned short usGpioLedVoip;           /* GPIO pin or not defined */
    unsigned short usGpioLedBlPowerOn;      /* GPIO pin or not defined */
    unsigned short usGpioLedBlAlarm;        /* GPIO pin or not defined */
    unsigned short usGpioLedBlResetCfg;     /* GPIO pin or not defined */
    unsigned short usGpioLedBlStop;         /* GPIO pin or not defined */
    unsigned short usExtIntrWireless;       /* ext intr or not defined */
    unsigned short usExtIntrAdslDyingGasp;  /* ext intr or not defined */
    unsigned short usExtIntrVoip;           /* ext intr or not defined */
    unsigned short usExtIntrHpna;           /* ext intr or not defined */
    unsigned short usCsHpna;                /* chip select not not defined */
    unsigned short usCsVoip;                /* chip select not not defined */
} BOARD_PARAMETERS, *PBOARD_PARAMETERS;

/* Variables */
#if defined(_BCM96345_) || defined(CONFIG_BCM96345)
static BOARD_PARAMETERS g_bcm96345r =
{
    "96345R",                               /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_GPIO_11_AH,                          /* usGpioRj11InnerPair */
    BP_GPIO_12_AH,                          /* usGpioRj11OuterPair */
    BP_GPIO_13_AH,                          /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_GPIO_8_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_8_AH,                           /* usGpioLedWanData */
    BP_GPIO_9_AH,                           /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_10_AH,                          /* usGpioLedBlAlarm */
    BP_GPIO_9_AH,                           /* usGpioLedBlResetCfg */
    BP_GPIO_8_AH,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_EXT_INTR_0,                          /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96345gw2 =
{
    /* A hardware jumper determines whether GPIO 13 is used for Press and Hold
     * Reset or RTS.
     */
    "96345GW2",                             /* szBoardId */
    {{BP_ENET_EXTERNAL_PHY_REVERSE_MII,     /* ucPhyType */
      0x00,                                 /* ucPhyAddress */
      BP_GPIO_0_AH,                         /* usGpioPhySpiSck */
      BP_GPIO_4_AH,                         /* usGpioPhySpiSs */
      BP_GPIO_12_AH,                        /* usGpioPhySpiMosi */
      BP_GPIO_11_AH},                       /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_16MB_1_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_13_AH,                          /* usGpioPressAndHoldReset */
    BP_GPIO_6_AH,                           /* usGpioVoipReset */
    BP_GPIO_15_AH,                          /* usGpioVoipIntr */
    BP_GPIO_2_AH,                           /* usGpioPcmciaReset */
    BP_GPIO_13_AH,                          /* usGpioUartRts */
    BP_GPIO_9_AH,                           /* usGpioUartCts */
    BP_GPIO_8_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_GPIO_7_AH,                           /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_8_AH,                           /* usGpioLedWanData */
    BP_NOT_DEFINED,                         /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_10_AH,                          /* usGpioLedBlAlarm */
    BP_GPIO_7_AH,                           /* usGpioLedBlResetCfg */
    BP_GPIO_8_AH,                           /* usGpioLedBlStop */
    BP_EXT_INTR_2,                          /* usExtIntrWireless */
    BP_EXT_INTR_0,                          /* usExtIntrAdslDyingGasp */
    BP_EXT_INTR_1,                          /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_CS_2                                 /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96345gw =
{
    "96345GW",                              /* szBoardId */
    {{BP_ENET_EXTERNAL_PHY_NO_REVERSE_MII,  /* ucPhyType */
      0x00,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_16MB_1_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_GPIO_11_AH,                          /* usGpioRj11InnerPair */
    BP_GPIO_1_AH,                           /* usGpioRj11OuterPair */
    BP_GPIO_13_AH,                          /* usGpioPressAndHoldReset */
    BP_GPIO_6_AH,                           /* usGpioVoipReset */
    BP_GPIO_15_AH,                          /* usGpioVoipIntr */
    BP_GPIO_2_AH,                           /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_GPIO_8_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_GPIO_10_AH,                          /* usGpioLedWireless */
    BP_GPIO_7_AH,                           /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_8_AH,                           /* usGpioLedWanData */
    BP_NOT_DEFINED,                         /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_9_AH,                           /* usGpioLedBlAlarm */
    BP_GPIO_10_AH,                          /* usGpioLedBlResetCfg */
    BP_GPIO_8_AH,                           /* usGpioLedBlStop */
    BP_EXT_INTR_2,                          /* usExtIntrWireless */
    BP_EXT_INTR_0,                          /* usExtIntrAdslDyingGasp */
    BP_EXT_INTR_1,                          /* usExtIntrVoip */
    BP_EXT_INTR_3,                          /* usExtIntrHpna */
    BP_CS_1,                                /* usCsHpna */
    BP_CS_2                                 /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96335r =
{
    "96335R",                               /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_14_AH,                          /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_GPIO_9_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_9_AH,                           /* usGpioLedWanData */
    BP_GPIO_8_AH,                           /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_10_AH,                          /* usGpioLedBlAlarm */
    BP_GPIO_8_AH,                           /* usGpioLedBlResetCfg */
    BP_GPIO_9_AH,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96345r0 =
{
    "96345R0",                              /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_NOT_DEFINED,                         /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_GPIO_8_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_9_AH,                           /* usGpioLedWanData */
    BP_GPIO_9_AH,                           /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_9_AH,                           /* usGpioLedBlAlarm */
    BP_GPIO_8_AH,                           /* usGpioLedBlResetCfg */
    BP_GPIO_8_AH,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96345rs =
{
    "96345RS",                              /* szBoardId */
    {{BP_ENET_EXTERNAL_PHY_NO_REVERSE_MII,  /* ucPhyType */
      0x00,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_GPIO_11_AH,                          /* usGpioRj11InnerPair */
    BP_GPIO_12_AH,                          /* usGpioRj11OuterPair */
    BP_GPIO_13_AH,                          /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_GPIO_8_AH,                           /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_8_AH,                           /* usGpioLedWanData */
    BP_GPIO_9_AH,                           /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_GPIO_10_AH,                          /* usGpioLedBlAlarm */
    BP_GPIO_9_AH,                           /* usGpioLedBlResetCfg */
    BP_GPIO_8_AH,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_EXT_INTR_0,                          /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static PBOARD_PARAMETERS g_BoardParms[] =
    {&g_bcm96345r, &g_bcm96345gw2, &g_bcm96345gw, &g_bcm96335r, &g_bcm96345r0,
     &g_bcm96345rs, 0};
#endif

#if defined(_BCM96348_) || defined(CONFIG_BCM96348)

static BOARD_PARAMETERS g_bcm96348r =
{
    "96348R",                               /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_7_AH,                           /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_GPIO_2_AL,                           /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_3_AL,                           /* usGpioLedWanData */
    BP_GPIO_3_AL,                           /* usGpioLedPpp */
    BP_GPIO_4_AL,                           /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_GPIO_0_AL,                           /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_GPIO_3_AL,                           /* usGpioLedBlResetCfg */
    BP_GPIO_1_AL,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96348gw =
{
    "96348GW",                              /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_EXTERNAL_PHY_REVERSE_MII,     /* ucPhyType */
      0,                                    /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED}},                     /* usGpioPhySpiMiso */
    BP_MEMORY_16MB_2_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_7_AH,                           /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_GPIO_2_AL,                           /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_3_AL,                           /* usGpioLedWanData */
    BP_GPIO_3_AL,                           /* usGpioLedPpp */
    BP_GPIO_4_AL,                           /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_GPIO_0_AL,                           /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_GPIO_3_AL,                           /* usGpioLedBlResetCfg */
    BP_GPIO_1_AL,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96348sv =
{
    "96348SV",                              /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_EXTERNAL_PHY_NO_REVERSE_MII,  /* ucPhyType */
      0x1f,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED}},                      /* usGpioPhySpiMiso */
    BP_MEMORY_32MB_2_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_NOT_DEFINED,                         /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_NOT_DEFINED,                         /* usGpioLedWanData */
    BP_NOT_DEFINED,                         /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_NOT_DEFINED,                         /* usGpioLedBlResetCfg */
    BP_NOT_DEFINED,                         /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static PBOARD_PARAMETERS g_BoardParms[] =
    {&g_bcm96348r, &g_bcm96348gw, &g_bcm96348sv, 0};
#endif

#if defined(_BCM96338_) || defined(CONFIG_BCM96338)

static BOARD_PARAMETERS g_bcm96338r =
{
    "96338R",                               /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_NO_PHY}},                     /* ucPhyType */
    BP_MEMORY_8MB_1_CHIP,                   /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_7_AH,                           /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_GPIO_2_AL,                           /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_3_AL,                           /* usGpioLedWanData */
    BP_GPIO_3_AL,                           /* usGpioLedPpp */
    BP_GPIO_4_AL,                           /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_GPIO_0_AL,                           /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_GPIO_3_AL,                           /* usGpioLedBlResetCfg */
    BP_GPIO_1_AL,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96338gw =
{
    "96338GW",                              /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_EXTERNAL_PHY_REVERSE_MII,     /* ucPhyType */
      0,                                    /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED}},                     /* usGpioPhySpiMiso */
    BP_MEMORY_16MB_2_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_GPIO_7_AH,                           /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_GPIO_2_AL,                           /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_GPIO_3_AL,                           /* usGpioLedWanData */
    BP_GPIO_3_AL,                           /* usGpioLedPpp */
    BP_GPIO_4_AL,                           /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_GPIO_0_AL,                           /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_GPIO_3_AL,                           /* usGpioLedBlResetCfg */
    BP_GPIO_1_AL,                           /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static BOARD_PARAMETERS g_bcm96338sv =
{
    "96338SV",                              /* szBoardId */
    {{BP_ENET_INTERNAL_PHY,                 /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED},                      /* usGpioPhySpiMiso */
     {BP_ENET_EXTERNAL_PHY_NO_REVERSE_MII,  /* ucPhyType */
      0x1f,                                 /* ucPhyAddress */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSck */
      BP_NOT_DEFINED,                       /* usGpioPhySpiSs */
      BP_NOT_DEFINED,                       /* usGpioPhySpiMosi */
      BP_NOT_DEFINED}},                      /* usGpioPhySpiMiso */
    BP_MEMORY_32MB_2_CHIP,                  /* usSdramSize */
    16,                                     /* usPsiSize */
    BP_NOT_DEFINED,                         /* usGpioRj11InnerPair */
    BP_NOT_DEFINED,                         /* usGpioRj11OuterPair */
    BP_NOT_DEFINED,                         /* usGpioPressAndHoldReset */
    BP_NOT_DEFINED,                         /* usGpioVoipReset */
    BP_NOT_DEFINED,                         /* usGpioVoipIntr */
    BP_NOT_DEFINED,                         /* usGpioPcmciaReset */
    BP_NOT_DEFINED,                         /* usGpioUartRts */
    BP_NOT_DEFINED,                         /* usGpioUartCts */
    BP_NOT_DEFINED,                         /* usGpioLedAdsl */
    BP_NOT_DEFINED,                         /* usGpioLedAdslFail */
    BP_NOT_DEFINED,                         /* usGpioLedWireless */
    BP_NOT_DEFINED,                         /* usGpioLedUsb */
    BP_NOT_DEFINED,                         /* usGpioLedHpna */
    BP_NOT_DEFINED,                         /* usGpioLedWanData */
    BP_NOT_DEFINED,                         /* usGpioLedPpp */
    BP_NOT_DEFINED,                         /* usGpioLedPppFail */
    BP_NOT_DEFINED,                         /* usGpioLedVoip */
    BP_NOT_DEFINED,                         /* usGpioLedBlPowerOn */
    BP_NOT_DEFINED,                         /* usGpioLedBlAlarm */
    BP_NOT_DEFINED,                         /* usGpioLedBlResetCfg */
    BP_NOT_DEFINED,                         /* usGpioLedBlStop */
    BP_NOT_DEFINED,                         /* usExtIntrWireless */
    BP_NOT_DEFINED,                         /* usExtIntrAdslDyingGasp */
    BP_NOT_DEFINED,                         /* usExtIntrVoip */
    BP_NOT_DEFINED,                         /* usExtIntrHpna */
    BP_NOT_DEFINED,                         /* usCsHpna */
    BP_NOT_DEFINED                          /* usCsVoip */
};

static PBOARD_PARAMETERS g_BoardParms[] =
    {&g_bcm96338r, &g_bcm96338gw, &g_bcm96338sv, 0};
#endif


static PBOARD_PARAMETERS g_pCurrentBp = 0;

/**************************************************************************
 * Name       : bpstrcmp
 *
 * Description: String compare for this file so it does not depend on an OS.
 *              (Linux kernel and CFE share this source file.)
 *
 * Parameters : [IN] dest - destination string
 *              [IN] src - source string
 *
 * Returns    : -1 - dest < src, 1 - dest > src, 0 dest == src
 ***************************************************************************/
static int bpstrcmp(const char *dest,const char *src);
static int bpstrcmp(const char *dest,const char *src)
{
    while (*src && *dest)
    {
        if (*dest < *src) return -1;
        if (*dest > *src) return 1;
        dest++;
        src++;
    }

    if (*dest && !*src) return 1;
    if (!*dest && *src) return -1;
    return 0;
} /* bpstrcmp */

/**************************************************************************
 * Name       : BpSetBoardId
 *
 * Description: This function find the BOARD_PARAMETERS structure for the
 *              specified board id string and assigns it to a global, static
 *              variable.
 *
 * Parameters : [IN] pszBoardId - Board id string that is saved into NVRAM.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_FOUND - Error, board id input string does not
 *                  have a board parameters configuration record.
 ***************************************************************************/
int BpSetBoardId( char *pszBoardId )
{
    int nRet = BP_BOARD_ID_NOT_FOUND;
    PBOARD_PARAMETERS *ppBp;

    for( ppBp = g_BoardParms; *ppBp; ppBp++ )
    {
        if( !bpstrcmp((*ppBp)->szBoardId, pszBoardId) )
        {
            g_pCurrentBp = *ppBp;
            nRet = BP_SUCCESS;
            break;
        }
    }

    return( nRet );
} /* BpSetBoardId */

/**************************************************************************
 * Name       : BpGetBoardIds
 *
 * Description: This function returns all of the supported board id strings.
 *
 * Parameters : [OUT] pszBoardIds - Address of a buffer that the board id
 *                  strings are returned in.  Each id starts at BP_BOARD_ID_LEN
 *                  boundary.
 *              [IN] nBoardIdsSize - Number of BP_BOARD_ID_LEN elements that
 *                  were allocated in pszBoardIds.
 *
 * Returns    : Number of board id strings returned.
 ***************************************************************************/
int BpGetBoardIds( char *pszBoardIds, int nBoardIdsSize )
{
    PBOARD_PARAMETERS *ppBp;
    int i;
    char *src;
    char *dest;

    for( i = 0, ppBp = g_BoardParms; *ppBp && nBoardIdsSize;
        i++, ppBp++, nBoardIdsSize--, pszBoardIds += BP_BOARD_ID_LEN )
    {
        dest = pszBoardIds;
        src = (*ppBp)->szBoardId;
        while( *src )
            *dest++ = *src++;
        *dest = '\0';
    }

    return( i );
} /* BpGetBoardIds */

/**************************************************************************
 * Name       : BpGetEthernetMacInfo
 *
 * Description: This function returns all of the supported board id strings.
 *
 * Parameters : [OUT] pEnetInfos - Address of an array of ETHERNET_MAC_INFO
 *                  buffers.
 *              [IN] nNumEnetInfos - Number of ETHERNET_MAC_INFO elements that
 *                  are pointed to by pEnetInfos.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 ***************************************************************************/
int BpGetEthernetMacInfo( PETHERNET_MAC_INFO pEnetInfos, int nNumEnetInfos )
{
    int i, nRet;

    if( g_pCurrentBp )
    {
        for( i = 0; i < nNumEnetInfos; i++, pEnetInfos++ )
        {
            if( i < BP_MAX_ENET_MACS )
            {
                unsigned char *src = (unsigned char *)
                    &g_pCurrentBp->EnetMacInfos[i];
                unsigned char *dest = (unsigned char *) pEnetInfos;
                int len = sizeof(ETHERNET_MAC_INFO);
                while( len-- )
                    *dest++ = *src++;
            }
            else
                pEnetInfos->ucPhyType = BP_ENET_NO_PHY;
        }

        nRet = BP_SUCCESS;
    }
    else
    {
        for( i = 0; i < nNumEnetInfos; i++, pEnetInfos++ )
            pEnetInfos->ucPhyType = BP_ENET_NO_PHY;

        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetEthernetMacInfo */

/**************************************************************************
 * Name       : BpGetSdramSize
 *
 * Description: This function returns a constant that describees the board's
 *              SDRAM type and size.
 *
 * Parameters : [OUT] pulSdramSize - Address of short word that the SDRAM size
 *                  is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 ***************************************************************************/
int BpGetSdramSize( unsigned long *pulSdramSize )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulSdramSize = g_pCurrentBp->usSdramSize;
        nRet = BP_SUCCESS;
    }
    else
    {
        *pulSdramSize = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetSdramSize */

/**************************************************************************
 * Name       : BpGetPsiSize
 *
 * Description: This function returns the persistent storage size in K bytes.
 *
 * Parameters : [OUT] pulPsiSize - Address of short word that the persistent
 *                  storage size is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 ***************************************************************************/
int BpGetPsiSize( unsigned long *pulPsiSize )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulPsiSize = g_pCurrentBp->usPsiSize;
        nRet = BP_SUCCESS;
    }
    else
    {
        *pulPsiSize = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetPsiSize */

/**************************************************************************
 * Name       : BpGetRj11InnerOuterPairGpios
 *
 * Description: This function returns the GPIO pin assignments for changing
 *              between the RJ11 inner pair and RJ11 outer pair.
 *
 * Parameters : [OUT] pusInner - Address of short word that the RJ11 inner pair
 *                  GPIO pin is returned in.
 *              [OUT] pusOuter - Address of short word that the RJ11 outer pair
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, values are returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetRj11InnerOuterPairGpios( unsigned short *pusInner,
    unsigned short *pusOuter )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusInner = g_pCurrentBp->usGpioRj11InnerPair;
        *pusOuter = g_pCurrentBp->usGpioRj11OuterPair;

        if( g_pCurrentBp->usGpioRj11InnerPair != BP_NOT_DEFINED &&
            g_pCurrentBp->usGpioRj11OuterPair != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusInner = *pusOuter = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetRj11InnerOuterPairGpios */

/**************************************************************************
 * Name       : BpGetPressAndHoldResetGpio
 *
 * Description: This function returns the GPIO pin assignment for the press
 *              and hold reset button.
 *
 * Parameters : [OUT] pusValue - Address of short word that the press and hold
 *                  reset button GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetPressAndHoldResetGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioPressAndHoldReset;

        if( g_pCurrentBp->usGpioPressAndHoldReset != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetPressAndHoldResetGpio */

/**************************************************************************
 * Name       : BpGetVoipResetGpio
 *
 * Description: This function returns the GPIO pin assignment for the VOIP
 *              Reset operation.
 *
 * Parameters : [OUT] pusValue - Address of short word that the VOIP reset
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetVoipResetGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioVoipReset;

        if( g_pCurrentBp->usGpioVoipReset != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetVoipResetGpio */

/**************************************************************************
 * Name       : BpGetVoipIntrGpio
 *
 * Description: This function returns the GPIO pin assignment for VoIP interrupt.
 *
 * Parameters : [OUT] pusValue - Address of short word that the VOIP interrupt
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetVoipIntrGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioVoipIntr;

        if( g_pCurrentBp->usGpioVoipIntr != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetVoipIntrGpio */

/**************************************************************************
 * Name       : BpGetPcmciaResetGpio
 *
 * Description: This function returns the GPIO pin assignment for the PCMCIA
 *              Reset operation.
 *
 * Parameters : [OUT] pusValue - Address of short word that the PCMCIA reset
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetPcmciaResetGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioPcmciaReset;

        if( g_pCurrentBp->usGpioPcmciaReset != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetPcmciaResetGpio */

/**************************************************************************
 * Name       : BpGetUartRtsCtsGpios
 *
 * Description: This function returns the GPIO pin assignments for RTS and CTS
 *              UART signals.
 *
 * Parameters : [OUT] pusRts - Address of short word that the UART RTS GPIO
 *                  pin is returned in.
 *              [OUT] pusCts - Address of short word that the UART CTS GPIO
 *                  pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, values are returned.
 *              BP_BOARD_ID_NOT_SET - Error, board id input string does not
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetRtsCtsUartGpios( unsigned short *pusRts, unsigned short *pusCts )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusRts = g_pCurrentBp->usGpioUartRts;
        *pusCts = g_pCurrentBp->usGpioUartCts;

        if( g_pCurrentBp->usGpioUartRts != BP_NOT_DEFINED &&
            g_pCurrentBp->usGpioUartCts != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusRts = *pusCts = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetUartRtsCtsGpios */


#if 0 

/**************************************************************************
 * Name       : BpGetAdslLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the ADSL
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the ADSL LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetAdslLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedAdsl;

        if( g_pCurrentBp->usGpioLedAdsl != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetAdslLedGpio */

/**************************************************************************
 * Name       : BpGetAdslFailLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the ADSL
 *              LED that is used when there is a DSL connection failure.
 *
 * Parameters : [OUT] pusValue - Address of short word that the ADSL LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetAdslFailLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedAdslFail;

        if( g_pCurrentBp->usGpioLedAdslFail != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetAdslFailLedGpio */

/**************************************************************************
 * Name       : BpGetWirelessLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the Wireless
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the Wireless LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetWirelessLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedWireless;

        if( g_pCurrentBp->usGpioLedWireless != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetWirelessLedGpio */

/**************************************************************************
 * Name       : BpGetUsbLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the USB
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the USB LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetUsbLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedUsb;

        if( g_pCurrentBp->usGpioLedUsb != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetUsbLedGpio */

/**************************************************************************
 * Name       : BpGetHpnaLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the HPNA
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the HPNA LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetHpnaLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedHpna;

        if( g_pCurrentBp->usGpioLedHpna != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetHpnaLedGpio */

/**************************************************************************
 * Name       : BpGetWanDataLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the WAN Data
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the WAN Data LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetWanDataLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedWanData;

        if( g_pCurrentBp->usGpioLedWanData != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetWanDataLedGpio */

/**************************************************************************
 * Name       : BpGetPppLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the PPP
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the PPP LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetPppLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedPpp;

        if( g_pCurrentBp->usGpioLedPpp != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetPppLedGpio */

/**************************************************************************
 * Name       : BpGetPppFailLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the PPP
 *              LED that is used when there is a PPP connection failure.
 *
 * Parameters : [OUT] pusValue - Address of short word that the PPP LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetPppFailLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedPppFail;

        if( g_pCurrentBp->usGpioLedPppFail != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetPppFailLedGpio */

/**************************************************************************
 * Name       : BpGetBootloaderPowerOnLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the power
 *              on LED that is set by the bootloader.
 *
 * Parameters : [OUT] pusValue - Address of short word that the alarm LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetBootloaderPowerOnLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedBlPowerOn;

        if( g_pCurrentBp->usGpioLedBlPowerOn != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetBootloaderPowerOn */

/**************************************************************************
 * Name       : BpGetBootloaderAlarmLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the alarm
 *              LED that is set by the bootloader.
 *
 * Parameters : [OUT] pusValue - Address of short word that the alarm LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetBootloaderAlarmLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedBlAlarm;

        if( g_pCurrentBp->usGpioLedBlAlarm != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetBootloaderAlarmLedGpio */

/**************************************************************************
 * Name       : BpGetBootloaderResetCfgLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the reset
 *              configuration LED that is set by the bootloader.
 *
 * Parameters : [OUT] pusValue - Address of short word that the reset
 *                  configuration LED GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetBootloaderResetCfgLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedBlResetCfg;

        if( g_pCurrentBp->usGpioLedBlResetCfg != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetBootloaderResetCfgLedGpio */

/**************************************************************************
 * Name       : BpGetBootloaderStopLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the break
 *              into bootloader LED that is set by the bootloader.
 *
 * Parameters : [OUT] pusValue - Address of short word that the break into
 *                  bootloader LED GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetBootloaderStopLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedBlStop;

        if( g_pCurrentBp->usGpioLedBlStop != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetBootloaderStopLedGpio */

/**************************************************************************
 * Name       : BpGetVoipLedGpio
 *
 * Description: This function returns the GPIO pin assignment for the VOIP
 *              LED.
 *
 * Parameters : [OUT] pusValue - Address of short word that the VOIP LED
 *                  GPIO pin is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetVoipLedGpio( unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pusValue = g_pCurrentBp->usGpioLedVoip;

        if( g_pCurrentBp->usGpioLedVoip != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetVoipLedGpio */

#endif 


/**************************************************************************
 * Name       : BpGetWirelessExtIntr
 *
 * Description: This function returns the Wireless external interrupt number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the wireless
 *                  external interrupt number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetWirelessExtIntr( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usExtIntrWireless;

        if( g_pCurrentBp->usExtIntrWireless != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetWirelessExtIntr */

/**************************************************************************
 * Name       : BpGetAdslDyingGaspExtIntr
 *
 * Description: This function returns the ADSL Dying Gasp external interrupt
 *              number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the ADSL Dying Gasp
 *                  external interrupt number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetAdslDyingGaspExtIntr( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usExtIntrAdslDyingGasp;

        if( g_pCurrentBp->usExtIntrAdslDyingGasp != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetAdslDyingGaspExtIntr */

/**************************************************************************
 * Name       : BpGetVoipExtIntr
 *
 * Description: This function returns the VOIP external interrupt number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the VOIP
 *                  external interrupt number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetVoipExtIntr( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usExtIntrVoip;

        if( g_pCurrentBp->usExtIntrVoip != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetVoipExtIntr */

/**************************************************************************
 * Name       : BpGetHpnaExtIntr
 *
 * Description: This function returns the HPNA external interrupt number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the HPNA
 *                  external interrupt number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetHpnaExtIntr( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usExtIntrHpna;

        if( g_pCurrentBp->usExtIntrHpna != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetHpnaExtIntr */

/**************************************************************************
 * Name       : BpGetHpnaChipSelect
 *
 * Description: This function returns the HPNA chip select number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the HPNA
 *                  chip select number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetHpnaChipSelect( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usCsHpna;

        if( g_pCurrentBp->usCsHpna != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetHpnaChipSelect */

/**************************************************************************
 * Name       : BpGetVoipChipSelect
 *
 * Description: This function returns the VOIP chip select number.
 *
 * Parameters : [OUT] pulValue - Address of short word that the VOIP
 *                  chip select number is returned in.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 *              BP_VALUE_NOT_DEFINED - At least one return value is not defined
 *                  for the board.
 ***************************************************************************/
int BpGetVoipChipSelect( unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp )
    {
        *pulValue = g_pCurrentBp->usCsVoip;

        if( g_pCurrentBp->usCsVoip != BP_NOT_DEFINED )
        {
            nRet = BP_SUCCESS;
        }
        else
        {
            nRet = BP_VALUE_NOT_DEFINED;
        }
    }
    else
    {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetVoipChipSelect */

