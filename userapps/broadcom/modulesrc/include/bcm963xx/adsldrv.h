/*
<:copyright-broadcom 
 
 Copyright (c) 2002 Broadcom Corporation 
 All Rights Reserved 
 No portions of this material may be reproduced in any form without the 
 written permission of: 
          Broadcom Corporation 
          16215 Alton Parkway 
          Irvine, California 92619 
 All information contained in this document is Broadcom Corporation 
 company private, proprietary, and trade secret. 
 
:>
*/
/***************************************************************************
 * File Name  : AdslDrv.h
 *
 * Description: This file contains the definitions and structures for the
 *              Linux IOCTL interface that used between the user mode ADSL
 *              API library and the kernel ADSL API driver.
 *
 * Updates    : 11/02/2001  lkaplan.  Created.
 ***************************************************************************/

#if !defined(_ADSLDRV_H_)
#define _ADSLDRV_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* Incldes. */
#include <bcmadsl.h>

/* Defines. */
#define ADSLDRV_MAJOR            208 /* arbitrary unused value */

#define ADSLIOCTL_CHECK \
    _IOR(ADSLDRV_MAJOR, 0, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_INITIALIZE \
    _IOWR(ADSLDRV_MAJOR, 1, ADSLDRV_INITIALIZE)
#define ADSLIOCTL_UNINITIALIZE \
    _IOR(ADSLDRV_MAJOR, 2, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_CONNECTION_START \
    _IOWR(ADSLDRV_MAJOR, 3, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_CONNECTION_STOP \
    _IOR(ADSLDRV_MAJOR, 4, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_GET_PHY_ADDR \
    _IOR(ADSLDRV_MAJOR, 5, ADSLDRV_PHY_ADDR)
#define ADSLIOCTL_SET_PHY_ADDR \
    _IOWR(ADSLDRV_MAJOR, 6, ADSLDRV_PHY_ADDR)
#define ADSLIOCTL_MAP_ATM_PORT_IDS \
    _IOWR(ADSLDRV_MAJOR, 7, ADSLDRV_MAP_ATM_PORT)
#define ADSLIOCTL_GET_CONNECTION_INFO \
    _IOR(ADSLDRV_MAJOR, 8, ADSLDRV_CONNECTION_INFO)
#define ADSLIOCTL_DIAG_COMMAND \
    _IOR(ADSLDRV_MAJOR, 9, ADSLDRV_DIAG)
#define ADSLIOCTL_GET_OBJ_VALUE \
    _IOR(ADSLDRV_MAJOR, 10, ADSLDRV_GET_OBJ)
#define ADSLIOCTL_START_BERT \
    _IOR(ADSLDRV_MAJOR, 11, ADSLDRV_BERT)
#define ADSLIOCTL_STOP_BERT \
    _IOR(ADSLDRV_MAJOR, 12, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_CONFIGURE \
    _IOR(ADSLDRV_MAJOR, 13, ADSLDRV_CONFIGURE)
#define ADSLIOCTL_TEST \
    _IOR(ADSLDRV_MAJOR, 14, ADSLDRV_TEST)
#define ADSLIOCTL_GET_CONSTEL_POINTS \
    _IOR(ADSLDRV_MAJOR, 15, ADSLDRV_GET_CONSTEL_POINTS)
#define ADSLIOCTL_GET_VERSION \
    _IOR(ADSLDRV_MAJOR, 16, ADSLDRV_GET_VERSION)
#define ADSLIOCTL_SET_SDRAM_BASE \
    _IOR(ADSLDRV_MAJOR, 17, ADSLDRV_SET_SDRAM_BASE)
#define ADSLIOCTL_RESET_STAT_COUNTERS \
    _IOR(ADSLDRV_MAJOR, 18, ADSLDRV_STATUS_ONLY)
#define ADSLIOCTL_SET_OEM_PARAM \
    _IOR(ADSLDRV_MAJOR, 19, ADSLDRV_SET_OEM_PARAM)
#define ADSLIOCTL_START_BERT_EX \
    _IOR(ADSLDRV_MAJOR, 20, ADSLDRV_BERT_EX)
#define ADSLIOCTL_STOP_BERT_EX \
    _IOR(ADSLDRV_MAJOR, 21, ADSLDRV_STATUS_ONLY)
//swda.030507
#define ADSLIOCTL_GET_ADSL_CONNECTION_COUNTER \
    _IOR(ADSLDRV_MAJOR, 22, ADSLDRV_GET_ADSL_CONNECTION_COUNTER)
//#define MAX_ADSLDRV_IOCTL_COMMANDS   22
#define MAX_ADSLDRV_IOCTL_COMMANDS   23

/* Typedefs. */
typedef struct
{
    BCMADSL_STATUS bvStatus;
} ADSLDRV_STATUS_ONLY, *PADSLDRV_STATUS_ONLY;

typedef struct
{
    ADSL_FN_NOTIFY_CB	pFnNotifyCb;
    UINT32				ulParm;
	adslCfgProfile		*pAdslCfg;
    BCMADSL_STATUS		bvStatus;
} ADSLDRV_INITIALIZE, *PADSLDRV_INITIALIZE;

typedef struct
{
    ADSL_CHANNEL_ADDR ChannelAddr;
    BCMADSL_STATUS bvStatus;
} ADSLDRV_PHY_ADDR, *PADSLDRV_PHY_ADDR;

typedef struct
{
	UINT16 usAtmFastPortId;
	UINT16 usAtmInterleavedPortId;
    BCMADSL_STATUS bvStatus;
} ADSLDRV_MAP_ATM_PORT, *PADSLDRV_MAP_ATM_PORT;

typedef struct
{
    ADSL_CONNECTION_INFO ConnectionInfo;
    BCMADSL_STATUS bvStatus;
} ADSLDRV_CONNECTION_INFO, *PADSLDRV_CONNECTION_INFO;

typedef struct
{
    int				diagCmd;
    int				diagMap;
    int				logTime;
	int				srvIpAddr;
	int				gwIpAddr;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_DIAG, *PADSLDRV_DIAG;

typedef struct
{
	char			*objId;
	int				objIdLen;
	char			*dataBuf;
	long			dataBufLen;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_GET_OBJ, *PADSLDRV_GET_OBJ;

typedef struct
{
	unsigned long	totalBits;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_BERT, *PADSLDRV_BERT;

typedef struct
{
	unsigned long	totalSec;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_BERT_EX, *PADSLDRV_BERT_EX;

typedef struct
{
	adslCfgProfile		*pAdslCfg;
    BCMADSL_STATUS		bvStatus;
} ADSLDRV_CONFIGURE, *PADSLDRV_CONFIGURE;

typedef struct
{
	unsigned long	testCmd;
	unsigned long	xmtStartTone;
	unsigned long	xmtNumTones;
	unsigned long	rcvStartTone;
	unsigned long	rcvNumTones;
    char			*xmtToneMap;
    char			*rcvToneMap;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_TEST, *PADSLDRV_TEST;

typedef struct
{
	int				toneId;
	ADSL_CONSTELLATION_POINT	*pointBuf;
	int				numPoints;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_GET_CONSTEL_POINTS, *PADSLDRV_GET_CONSTEL_POINTS;

typedef struct
{
	adslVersionInfo		*pAdslVer;
    BCMADSL_STATUS		bvStatus;
} ADSLDRV_GET_VERSION, *PADSLDRV_GET_VERSION;

typedef struct
{
	unsigned long	sdramBaseAddr;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_SET_SDRAM_BASE, *PADSLDRV_SET_SDRAM_BASE;


typedef struct
{
	int				paramId;
	void			*buf;
	int				len;
    BCMADSL_STATUS	bvStatus;
} ADSLDRV_SET_OEM_PARAM, *PADSLDRV_SET_OEM_PARAM;

//swda.030528
typedef struct
{
    adslConnectionCounter   *pAdslConnectCnt;
    BCMADSL_STATUS	        bvStatus;
} ADSLDRV_GET_ADSL_CONNECTION_COUNTER, *PADSLDRV_GET_ADSL_CONNECTION_COUNTER;

#if defined(__cplusplus)
}
#endif

#endif // _ADSLDRV_H_

