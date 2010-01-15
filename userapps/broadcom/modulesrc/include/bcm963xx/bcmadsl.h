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
//**************************************************************************
// File Name  : Adsl.h
//
// Description: This file contains the definitions, structures and function
//              prototypes for ADSL PHY interface
//
//**************************************************************************
#if !defined(_BCMADSL_H_)
#define _BCMADSL_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* Incldes. */
#include "AdslMibDef.h"

//**************************************************************************
// Type Definitions
//**************************************************************************

// Return status values
typedef enum BcmAdslStatus
{
    BCMADSL_STATUS_SUCCESS = 0,
    BCMADSL_STATUS_ERROR
} BCMADSL_STATUS;

// Return status values
typedef enum AdslLinkState
{
    BCM_ADSL_LINK_UP = 0,
    BCM_ADSL_LINK_DOWN,
	BCM_ADSL_TRAINING_G992_EXCHANGE,
	BCM_ADSL_TRAINING_G992_CHANNEL_ANALYSIS,
	BCM_ADSL_TRAINING_G992_STARTED,
	BCM_ADSL_TRAINING_G994,
	BCM_ADSL_G994_NONSTDINFO_RECEIVED,
    BCM_ADSL_BERT_COMPLETE,
	BCM_ADSL_ATM_IDLE,
    BCM_ADSL_EVENT,
    BCM_ADSL_G997_FRAME_RECEIVED,
    BCM_ADSL_G997_FRAME_SENT
} ADSL_LINK_STATE;

#ifndef DISABLE_ADSL_OLD_DEF
#define	ADSL_LINK_UP		BCM_ADSL_LINK_UP
#define	ADSL_LINK_DOWN		BCM_ADSL_LINK_DOWN
#endif

/* ADSL test modes */
typedef enum AdslTestMode
{
    ADSL_TEST_NORMAL = 0,
    ADSL_TEST_REVERB,
    ADSL_TEST_MEDLEY,
    ADSL_TEST_SELECT_TONES,
    ADSL_TEST_NO_AUTO_RETRAIN
} ADSL_TEST_MODE;

// ADSL_CHANNEL_ADDR Contains ADSL Utopia PHY addresses
typedef struct AdslChannelAddr
{
    UINT16 usFastChannelAddr;
    UINT16 usInterleavedChannelAddr;
} ADSL_CHANNEL_ADDR, *PADSL_CHANNEL_ADDR;

// ADSL_CONNECTION_INFO Contains ADSL Connection Info
typedef struct AdslConnectionInfo
{
	ADSL_LINK_STATE LinkState; 
    UINT32 ulFastUpStreamRate;
    UINT32 ulFastDnStreamRate;
    UINT32 ulInterleavedUpStreamRate;
    UINT32 ulInterleavedDnStreamRate;
} ADSL_CONNECTION_INFO, *PADSL_CONNECTION_INFO;

//swda.030507
typedef struct _adslConnectionCounter {
    char            link_state[16];
	int		        line_up_count;
	char		    system_up_time_duration[13];
    char		    line_up_time_duration[13];
} adslConnectionCounter;

/* OEM parameter definition */
#define ADSL_OEM_G994_VENDOR_ID       1    /* Vendor ID used during G.994 handshake */
#define ADSL_OEM_G994_XMT_NS_INFO     2    /* G.994 non-standard info field to send */
#define ADSL_OEM_G994_RCV_NS_INFO     3    /* G.994 received non-standard */
#define ADSL_OEM_EOC_VENDOR_ID        4    /* EOC reg. 0 */
#define ADSL_OEM_EOC_VERSION          5    /* EOC reg. 1 */
#define ADSL_OEM_EOC_SERIAL_NUMBER    6    /* EOC reg. 2 */
#define ADSL_OEM_T1413_VENDOR_ID      7    /* Vendor ID used during T1.413 handshake */
#define ADSL_OEM_T1413_EOC_VENDOR_ID  8    /* EOC reg. 0 (vendor ID) in T1.413 mode */

/* XMT gain definitions */
#define ADSL_XMT_GAIN_AUTO			  0x80000000

typedef struct
{
    int				diagCmd;
    int				diagMap;
    int				logTime;
	int				srvIpAddr;
	int				gwIpAddr;
} ADSL_DIAG, *PADSL_DIAG;

typedef struct
{
    short			x;
    short			y;
} ADSL_CONSTELLATION_POINT, *PADSL_CONSTELLATION_POINT;

#define ADSL_CONSTEL_DATA_ID		0
#define ADSL_CONSTEL_PILOT_ID		1

#define ADSL_MIB_INFO	adslMibInfo
typedef	ADSL_MIB_INFO   *PADSL_MIB_INFO;

typedef void (*ADSL_FN_NOTIFY_CB) (ADSL_LINK_STATE AdslLinkState, UINT32 ulParm); 

//**************************************************************************
// Function Prototypes
//**************************************************************************

BCMADSL_STATUS BcmAdsl_Check(void);
BCMADSL_STATUS BcmAdsl_Initialize(ADSL_FN_NOTIFY_CB pFnNotifyCb, UINT32 ulParm, adslCfgProfile *pAdslCfg);
#ifdef LINUX
BCMADSL_STATUS BcmAdsl_MapAtmPortIDs(UINT16 usAtmFastPortId, UINT16 usAtmInterleavedPortId);
#endif
BCMADSL_STATUS BcmAdsl_Uninitialize(void);
BCMADSL_STATUS BcmAdsl_ConnectionStart(void);
BCMADSL_STATUS BcmAdsl_ConnectionStop(void);
BCMADSL_STATUS BcmAdsl_GetPhyAddresses(PADSL_CHANNEL_ADDR pChannelAddr);
BCMADSL_STATUS BcmAdsl_SetPhyAddresses(PADSL_CHANNEL_ADDR pChannelAddr);
BCMADSL_STATUS BcmAdsl_GetConnectionInfo(PADSL_CONNECTION_INFO pConnectionInfo);
BCMADSL_STATUS BcmAdsl_DiagCommand(PADSL_DIAG pAdslDiag);
int BcmAdsl_GetObjectValue(char *objId, int objIdLen, char *dataBuf, long *dataBufLen);
BCMADSL_STATUS BcmAdsl_StartBERT(unsigned long totalBits);
BCMADSL_STATUS BcmAdsl_StopBERT(void);
BCMADSL_STATUS BcmAdsl_BertStartEx(unsigned long bertSec);
BCMADSL_STATUS BcmAdsl_BertStopEx(void);
BCMADSL_STATUS BcmAdsl_SendDyingGasp(void);
BCMADSL_STATUS BcmAdsl_Configure(adslCfgProfile *pAdslCfg);
BCMADSL_STATUS BcmAdsl_GetVersion(adslVersionInfo *pAdslVer);
BCMADSL_STATUS BcmAdsl_SetSDRAMBaseAddr(void *pAddr);
//swda.030528
BCMADSL_STATUS BcmAdsl_GetAdslConnectionCnt(adslConnectionCounter *padslconncnt);
BCMADSL_STATUS BcmAdsl_SetVcEntry (int gfc, int port, int vpi, int vci);
BCMADSL_STATUS BcmAdsl_SetVcEntryEx (int gfc, int port, int vpi, int vci, int pti_clp);

BCMADSL_STATUS BcmAdsl_ResetStatCounters(void);
BCMADSL_STATUS BcmAdsl_SetAtmLoopbackMode(void);
BCMADSL_STATUS BcmAdsl_SetTestMode(ADSL_TEST_MODE testMode);
BCMADSL_STATUS BcmAdsl_SelectTones(
	int		xmtStartTone, 
	int		xmtNumTones, 
	int		rcvStartTone,
	int		rcvNumTones, 
	char	*xmtToneMap,
	char	*rcvToneMap
	);
BCMADSL_STATUS BcmAdsl_SetDiagMode(int diagMode);

int BcmAdsl_GetConstellationPoints (int toneId, ADSL_CONSTELLATION_POINT *pointBuf, int numPoints);

int BcmAdsl_GetOemParameter (int paramId, void *buf, int len);
int BcmAdsl_SetOemParameter (int paramId, void *buf, int len);
int BcmAdsl_SetXmtGain(int gain);

UINT32	BcmAdsl_GetSelfTestMode(void);
void	BcmAdsl_SetSelfTestMode(UINT32 stMode);
UINT32	BcmAdsl_GetSelfTestResults(void);

#if defined(__cplusplus)
}
#endif

#endif // _BCMADSL_H_

