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
/****************************************************************************
 *
 * AdslMibOid.h 
 *
 * Description:
 *	SNMP object identifiers for ADSL MIB and other related MIBs
 *
 * Copyright (c) 1993-1998 AltoCom, Inc. All rights reserved.
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.3 $
 *
 * $Id: AdslMibDef.h,v 1.3 2004/03/31 08:44:33 michaelc Exp $
 *
 * $Log: AdslMibDef.h,v $
 * Revision 1.3  2004/03/31 08:44:33  michaelc
 * Upgrade from V2.14.1a to V2.14.2 and test RTA770W ok
 *
 * Revision 1.12  2003/10/17 21:02:12  ilyas
 * Added more data for ADSL2
 *
 * Revision 1.11  2003/10/14 00:55:27  ilyas
 * Added UAS, LOSS, SES error seconds counters.
 * Support for 512 tones (AnnexI)
 *
 * Revision 1.10  2003/09/29 18:39:51  ilyas
 * Added new definitions for AnnexI
 *
 * Revision 1.9  2003/07/18 19:14:34  ilyas
 * Merged with ADSL driver
 *
 * Revision 1.8  2003/07/08 18:34:16  ilyas
 * Added fields to adsl configuration structure
 *
 * Revision 1.7  2003/03/25 00:07:00  ilyas
 * Added "long" BERT supprt
 *
 * Revision 1.6  2003/02/27 07:10:52  ilyas
 * Added more configuration and status parameters (for EFNT)
 *
 * Revision 1.5  2003/01/23 20:29:37  ilyas
 * Added structure for ADSL PHY configuration command
 *
 * Revision 1.4  2002/11/13 21:32:49  ilyas
 * Added adjustK support for Centillium non-standard framing mode
 *
 * Revision 1.3  2002/10/31 01:35:50  ilyas
 * Fixed size of K for S=1/2
 *
 * Revision 1.2  2002/10/05 03:28:31  ilyas
 * Added extra definitions for Linux and VxWorks drivers.
 * Added definitions for SelfTest support
 *
 * Revision 1.1  2002/07/20 00:51:41  ilyas
 * Merged witchanges made for VxWorks/Linux driver.
 *
 * Revision 1.1  2001/12/21 22:39:30  ilyas
 * Added support for ADSL MIB data objects (RFC2662)
 *
 *
 *****************************************************************************/

#ifndef	AdslMibDefHeader
#define	AdslMibDefHeader

#if defined(__cplusplus)
extern "C" {
#endif

/* 
**
**		ADSL configuration parameters 
**
*/

#define kAdslCfgModMask						(0x00000007 | 0x0000F000)
#define kAdslCfgModAny						0x00000000

#define kAdslCfgModGdmtOnly					0x00000001
#define kAdslCfgModGliteOnly				0x00000002
#define kAdslCfgModT1413Only				0x00000004
#define kAdslCfgModAnnexIOnly				0x00000004
#define kAdslCfgModAdsl2Only				0x00001000

#define kAdslCfgBitmapMask					0x00000018
#define kAdslCfgDBM							0x00000000
#define kAdslCfgFBM							0x00000008
#define kAdslCfgFBMSoL						0x00000010

#define kAdslCfgLinePairMask				0x00000020
#define kAdslCfgLineInnerPair				0x00000000
#define kAdslCfgLineOuterPair				0x00000020

#define kAdslCfgCentilliumCRCWorkAroundMask			0x00000040
#define kAdslCfgCentilliumCRCWorkAroundDisabled		0x00000000
#define kAdslCfgCentilliumCRCWorkAroundEnabled		0x00000040

#define kAdslCfgExtraData					0x00000080
#define kAdslCfgTrellisMask					(0x00000100 | kAdslCfgExtraData)
#define kAdslCfgTrellisOn					(0x00000100 | kAdslCfgExtraData)
#define kAdslCfgTrellisOff					(0 | kAdslCfgExtraData)
#define kAdslCfgExtraMask					0xFFFFFF80

#define kAdslCfgLOSMonitoringMask			0x00000200
#define kAdslCfgLOSMonitoringOff			0x00000200
#define kAdslCfgLOSMonitoringOn				0x00000000

#define kAdslCfgMarginMonitoringMask		0x00000400
#define kAdslCfgMarginMonitoringOn			0x00000400
#define kAdslCfgMarginMonitoringOff			0x00000000

#define kAdslCfgDemodCapMask				0x00000800
#define kAdslCfgDemodCapOn					0x00000800
#define kAdslCfgDemodCapOff					0x00000000

/* Flags 0x00001000 - 0x00008000 are reserved for modulation (see above) */

/* Upstream mode flags 0x00010000 - 0x00030000 */

#define kAdslCfgUpstreamModeMask			0x00030000
#define kAdslCfgUpstreamMax					0x00000000
#define kAdslCfgUpstreamSingle				0x00010000
#define kAdslCfgUpstreamDouble				0x00020000
#define kAdslCfgUpstreamTriple				0x00030000

#define kAdslCfgNoSpectrumOverlap			0x00040000

#define kAdslCfgDefaultTrainingMargin		-1
#define kAdslCfgDefaultShowtimeMargin		-1
#define kAdslCfgDefaultLOMTimeThld			-1

/* ADSL2 parameters */

#define kAdsl2CfgReachExOn					0x00000001

typedef struct _adslCfgProfile {
	long		adslAnnexCParam;
	long		adslAnnexAParam;
	long		adslTrainingMarginQ4;
	long		adslShowtimeMarginQ4;
	long		adslLOMTimeThldSec;
	long		adslDemodCapMask;
	long		adslDemodCapValue;
	long		adsl2Param;
} adslCfgProfile;

/* 
**
**		ADSL PHY configuration
**
*/

typedef struct _adslPhyCfg {
	long		demodCapMask;
	long		demodCap;
} adslPhyCfg;

/* 
**
**		ADSL version info parameters 
**
*/

#define	kAdslVersionStringSize				32

#define	kAdslTypeUnknown					0
#define	kAdslTypeAnnexA						1
#define	kAdslTypeAnnexB						2
#define	kAdslTypeAnnexC						3
#define	kAdslTypeSADSL						4

typedef struct _adslVersionInfo {
	unsigned short	phyType;
	unsigned short	phyMjVerNum;
	unsigned short	phyMnVerNum;
	char			phyVerStr[kAdslVersionStringSize];
	unsigned short	drvMjVerNum;
	unsigned short	drvMnVerNum;
	unsigned short	drvVerStr[kAdslVersionStringSize];
} adslVersionInfo;

/* 
**
**		ADSL self-test parameters 
**
*/

#define kAdslSelfTestLMEM					0x00000001
#define kAdslSelfTestSDRAM					0x00000002
#define kAdslSelfTestAFE					0x00000004
#define kAdslSelfTestQproc					0x00000008
#define kAdslSelfTestRS						0x00000010
#define kAdslSelfTestHostDma				0x00000020

#define kAdslSelfTestAll					((kAdslSelfTestHostDma - 1) | kAdslSelfTestHostDma)

#define kAdslSelfTestInProgress				0x40000000
#define kAdslSelfTestCompleted				0x80000000

/* MIB OID's for ADSL objects */

#define kOidMaxObjLen						80

#define kOidAdsl							94
#define kOidAdslInterleave					124
#define kOidAdslFast						125
#define kOidAtm								37
#define kOidAdslPrivate						255
#define kOidAdslPrivatePartial				254

#define kAdslMibAnnexAToneNum				256
#define kAdslMibToneNum						kAdslMibAnnexAToneNum
#define kAdslMibMaxToneNum					kAdslMibAnnexAToneNum*2*2

#define kOidAdslPrivSNR						1
#define kOidAdslPrivBitAlloc				2
#define kOidAdslPrivGain					3
#define kOidAdslPrivShowtimeMargin			4
#define kOidAdslPrivChanCharLin				5
#define kOidAdslPrivChanCharLog				6
#define kOidAdslPrivQuietLineNoise			7
#define kOidAdslPrivExtraInfo				255

#define kOidAdslLine						1
#define kOidAdslMibObjects					1

#define kOidAdslLineTable					1
#define kOidAdslLineEntry					1
#define kOidAdslLineCoding					1
#define kOidAdslLineType					2
#define kOidAdslLineSpecific			    3
#define kOidAdslLineConfProfile				4
#define kOidAdslLineAlarmConfProfile		5

#define kOidAdslAtucPhysTable				2
#define kOidAdslAturPhysTable				3
#define kOidAdslPhysEntry					1
#define kOidAdslPhysInvSerialNumber     	1
#define kOidAdslPhysInvVendorID             2
#define kOidAdslPhysInvVersionNumber    	3
#define kOidAdslPhysCurrSnrMgn          	4
#define kOidAdslPhysCurrAtn             	5
#define kOidAdslPhysCurrStatus          	6
#define kOidAdslPhysCurrOutputPwr       	7
#define kOidAdslPhysCurrAttainableRate  	8

#define kOidAdslAtucChanTable				4
#define kOidAdslAturChanTable				5
#define kOidAdslChanEntry					1
#define kOidAdslChanInterleaveDelay			1
#define kOidAdslChanCurrTxRate				2
#define kOidAdslChanPrevTxRate          	3
#define kOidAdslChanCrcBlockLength      	4

#define kOidAdslAtucPerfDataTable			6
#define kOidAdslAturPerfDataTable			7
#define kOidAdslPerfDataEntry				1
#define kOidAdslPerfLofs                 	1
#define kOidAdslPerfLoss                 	2
#define kOidAdslPerfLprs                 	3
#define kOidAdslPerfESs                  	4
#define kOidAdslPerfValidIntervals          5
#define kOidAdslPerfInvalidIntervals     	6
#define kOidAdslPerfCurr15MinTimeElapsed 	7
#define kOidAdslPerfCurr15MinLofs        	8
#define kOidAdslPerfCurr15MinLoss        	9
#define kOidAdslPerfCurr15MinLprs        	10
#define kOidAdslPerfCurr15MinESs         	11
#define kOidAdslPerfCurr1DayTimeElapsed     12
#define kOidAdslPerfCurr1DayLofs         	13
#define kOidAdslPerfCurr1DayLoss         	14
#define kOidAdslPerfCurr1DayLprs         	15
#define kOidAdslPerfCurr1DayESs          	16
#define kOidAdslPerfPrev1DayMoniSecs     	17
#define kOidAdslPerfPrev1DayLofs         	18
#define kOidAdslPerfPrev1DayLoss            19
#define kOidAdslPerfPrev1DayLprs         	20
#define kOidAdslPerfPrev1DayESs          	21

#define kOidAdslAtucPerfIntervalTable		8
#define kOidAdslAturPerfIntervalTable		9
#define kOidAdslPerfIntervalEntry			1
#define kOidAdslIntervalNumber				1
#define kOidAdslIntervalLofs				2
#define kOidAdslIntervalLoss				3
#define kOidAdslIntervalLprs				4
#define kOidAdslIntervalESs					5
#define kOidAdslIntervalValidData			6

#define kOidAdslAtucChanPerfTable					10
#define kOidAdslAturChanPerfTable					11
#define kOidAdslChanPerfEntry						1
#define kOidAdslChanReceivedBlks                 	1
#define kOidAdslChanTransmittedBlks              	2
#define kOidAdslChanCorrectedBlks                	3
#define kOidAdslChanUncorrectBlks                	4
#define kOidAdslChanPerfValidIntervals           	5
#define kOidAdslChanPerfInvalidIntervals         	6
#define kOidAdslChanPerfCurr15MinTimeElapsed     	7
#define kOidAdslChanPerfCurr15MinReceivedBlks    	8
#define kOidAdslChanPerfCurr15MinTransmittedBlks 	9
#define kOidAdslChanPerfCurr15MinCorrectedBlks   	10
#define kOidAdslChanPerfCurr15MinUncorrectBlks   	11
#define kOidAdslChanPerfCurr1DayTimeElapsed      	12
#define kOidAdslChanPerfCurr1DayReceivedBlks     	13
#define kOidAdslChanPerfCurr1DayTransmittedBlks  	14
#define kOidAdslChanPerfCurr1DayCorrectedBlks    	15
#define kOidAdslChanPerfCurr1DayUncorrectBlks    	16
#define kOidAdslChanPerfPrev1DayMoniSecs         	17
#define kOidAdslChanPerfPrev1DayReceivedBlks     	18
#define kOidAdslChanPerfPrev1DayTransmittedBlks  	19
#define kOidAdslChanPerfPrev1DayCorrectedBlks    	20
#define kOidAdslChanPerfPrev1DayUncorrectBlks    	21

#define kOidAdslAtucChanIntervalTable				12
#define kOidAdslAturChanIntervalTable				13
#define kOidAdslChanIntervalEntry					1
#define kOidAdslChanIntervalNumber					1
#define kOidAdslChanIntervalReceivedBlks        	2
#define kOidAdslChanIntervalTransmittedBlks     	3
#define kOidAdslChanIntervalCorrectedBlks       	4
#define kOidAdslChanIntervalUncorrectBlks       	5
#define kOidAdslChanIntervalValidData           	6

#define kOidAtmMibObjects		1
#define kOidAtmTcTable			4
#define kOidAtmTcEntry			1
#define kOidAtmOcdEvents		1
#define kOidAtmAlarmState		2

/* Adsl Channel coding */

#define	kAdslRcvDir			0
#define	kAdslXmtDir			1

#define	kAdslRcvActive		(1 << kAdslRcvDir)
#define	kAdslXmtActive		(1 << kAdslXmtDir)

#define	kAdslIntlChannel	0
#define	kAdslFastChannel	1

#define	kAdslTrellisOff		0
#define	kAdslTrellisOn		1

/* AnnexC modulation and bitmap types for the field (adslConnection.modType) */

#define kAdslModMask		0x7

#define	kAdslModGdmt		0
#define	kAdslModT1413		1
#define	kAdslModGlite		2
#define kAdslModAnnexI		3
#define kAdslModAdsl2		4
#define kAdslModAdsl2p		5

#define kAdslBitmapShift	3
#define kAdslBitmapMask		kAdslCfgBitmapMask
#define kAdslDBM		    (0 << kAdslBitmapShift)
#define kAdslFBM		    (1 << kAdslBitmapShift)
#define kAdslFBMSoL			(2 << kAdslBitmapShift)

#define kAdslUpstreamModeShift		5
#define kAdslUpstreamModeMask		(3 << kAdslUpstreamModeShift)
#define kAdslUpstreamModeSingle		(0 << kAdslUpstreamModeShift)
#define kAdslUpstreamModeDouble		(1 << kAdslUpstreamModeShift)
#define kAdslUpstreamModeTriple		(2 << kAdslUpstreamModeShift)

/* AdslLineCodingType definitions */

#define kAdslLineCodingOther		1
#define kAdslLineCodingDMT			2
#define kAdslLineCodingCAP			3
#define kAdslLineCodingQAM			4

/* AdslLineType definitions */

#define kAdslLineTypeNoChannel		1
#define kAdslLineTypeFastOnly		2
#define kAdslLineTypeIntlOnly		3
#define kAdslLineTypeFastOrIntl		4
#define kAdslLineTypeFastAndIntl	5

typedef struct _adslLineEntry {
	unsigned char	adslLineCoding;
	unsigned char	adslLineType;
} adslLineEntry;


/* AdslPhys status definitions */

#define kAdslPhysStatusNoDefect		(1 << 0)
#define kAdslPhysStatusLOF			(1 << 1)	/* lossOfFraming (not receiving valid frame) */
#define kAdslPhysStatusLOS			(1 << 2)	/* lossOfSignal (not receiving signal) */
#define kAdslPhysStatusLPR			(1 << 3)	/* lossOfPower */
#define kAdslPhysStatusLOSQ			(1 << 4)	/* lossOfSignalQuality */
#define kAdslPhysStatusLOM			(1 << 5)	/* lossOfMargin */

typedef struct _adslPhysEntry {
	long		adslCurrSnrMgn;
	long		adslCurrAtn;
	long		adslCurrStatus;
	long		adslCurrOutputPwr;
	long		adslCurrAttainableRate;
} adslPhysEntry;

#define kAdslPhysVendorIdLen		8
#define kAdslPhysSerialNumLen		32
#define kAdslPhysVersionNumLen		32

typedef struct _adslFullPhysEntry {
	char		adslSerialNumber[kAdslPhysSerialNumLen];
	char		adslVendorID[kAdslPhysVendorIdLen];
	char		adslVersionNumber[kAdslPhysVersionNumLen];
	long		adslCurrSnrMgn;
	long		adslCurrAtn;
	long		adslCurrStatus;
	long		adslCurrOutputPwr;
	long		adslCurrAttainableRate;
} adslFullPhysEntry;

/* Adsl channel entry definitions */

typedef struct _adslChanEntry {
    unsigned long		adslChanIntlDelay;
	unsigned long		adslChanCurrTxRate;
	unsigned long		adslChanPrevTxRate;
	unsigned long		adslChanCrcBlockLength;
} adslChanEntry;

/* Adsl performance data definitions */

typedef struct _adslPerfCounters {
	unsigned long		adslLofs;
	unsigned long		adslLoss;
	unsigned long		adslLols;	/* Loss of Link failures (ATUC only) */
	unsigned long		adslLprs;
	unsigned long		adslESs;	/* Count of Errored Seconds */
	unsigned long		adslInits;	/* Count of Line initialization attempts (ATUC only) */
	unsigned long		adslUAS;	/* Count of Unavailable Seconds */
	unsigned long		adslSES;	/* Count of Severely Errored Seconds */
	unsigned long		adslLOSS;	/* Count of LOS seconds */
} adslPerfCounters;

typedef struct _adslPerfDataEntry {
	adslPerfCounters	perfTotal;
	unsigned long				adslPerfValidIntervals;
	unsigned long				adslPerfInvalidIntervals;
	adslPerfCounters	perfCurr15Min;
	unsigned long				adslPerfCurr15MinTimeElapsed;
	adslPerfCounters	perfCurr1Day;
	unsigned long				adslPerfCurr1DayTimeElapsed;
	adslPerfCounters	perfPrev1Day;
	unsigned long				adslAturPerfPrev1DayMoniSecs;
} adslPerfDataEntry;

#define kAdslMibPerfIntervals		4

/* Adsl channel performance data definitions */

typedef struct _adslChanCounters {
	unsigned long		adslChanReceivedBlks;
	unsigned long		adslChanTransmittedBlks;
	unsigned long		adslChanCorrectedBlks;
	unsigned long		adslChanUncorrectBlks;
} adslChanCounters;

typedef struct _adslChanPerfDataEntry {
	adslChanCounters	perfTotal;
	unsigned long				adslChanPerfValidIntervals;
	unsigned long				adslChanPerfInvalidIntervals;
	adslChanCounters	perfCurr15Min;
	unsigned long				adslPerfCurr15MinTimeElapsed;
	adslChanCounters	perfCurr1Day;
	unsigned long				adslPerfCurr1DayTimeElapsed;
	adslChanCounters	perfPrev1Day;
	unsigned long				adslAturPerfPrev1DayMoniSecs;
} adslChanPerfDataEntry;

#define kAdslMibChanPerfIntervals	4

/* Adsl trap threshold definitions */

#define	kAdslEventLinkChange		0x001
#define	kAdslEventRateChange		0x002
#define	kAdslEventLofThresh			0x004
#define	kAdslEventLosThresh			0x008
#define	kAdslEventLprThresh			0x010
#define	kAdslEventESThresh			0x020
#define	kAdslEventFastUpThresh		0x040
#define	kAdslEventIntlUpThresh		0x080
#define	kAdslEventFastDownThresh	0x100
#define	kAdslEventIntlDwonThresh	0x200

typedef struct _adslThreshCounters {
	unsigned long		adslThreshLofs;
	unsigned long		adslThreshLoss;
	unsigned long		adslThreshLols;	/* Loss of Link failures (ATUC only) */
	unsigned long		adslThreshLprs;
	unsigned long		adslThreshESs;
	unsigned long		adslThreshFastRateUp;
	unsigned long		adslThreshIntlRateUp;
	unsigned long		adslThreshFastRateDown;
	unsigned long		adslThreshIntlRateDown;
} adslThreshCounters;


/* Atm PHY performance data definitions */

#define	kAtmPhyStateNoAlarm			1
#define	kAtmPhyStateLcdFailure		2

typedef struct _atmPhyDataEntrty {
	unsigned long		atmInterfaceOCDEvents;
	unsigned long		atmInterfaceTCAlarmState;
} atmPhyDataEntrty;

typedef struct _adslBertResults {
	unsigned long		bertTotalBits;
	unsigned long		bertErrBits;
} adslBertResults;

typedef struct {
	unsigned long		cntHi;
	unsigned long		cntLo;
} cnt64;

typedef struct _adslBertStatusEx {
	unsigned long		bertSecTotal;
	unsigned long		bertSecElapsed;
	unsigned long		bertSecCur;
	cnt64				bertTotalBits;
	cnt64				bertErrBits;
} adslBertStatusEx;

typedef struct _adslDataConnectionInfo {
	unsigned short		K;
	unsigned char		S, R, D;
} adslDataConnectionInfo;

typedef struct _adslConnectionInfo {
	unsigned char			chType;				/* fast or interleaved */
	unsigned char			modType;			/* modulation type: G.DMT or T1.413 */
	unsigned char			trellisCoding;		/* off(0) or on(1) */
	adslDataConnectionInfo	rcvInfo;
	adslDataConnectionInfo	xmtInfo;
} adslConnectionInfo;

typedef struct _adslConnectionDataStat {
	unsigned long			cntRS;	
	unsigned long			cntRSCor;	
	unsigned long			cntRSUncor;	
	unsigned long			cntSF;	
	unsigned long			cntSFErr;	
} adslConnectionDataStat;

typedef struct _adslConnectionStat {
	adslConnectionDataStat	rcvStat;
	adslConnectionDataStat	xmtStat;
} adslConnectionStat;

typedef struct _atmConnectionDataStat {
	unsigned long			cntHEC;
	unsigned long			cntOCD;
	unsigned long			cntLCD;
	unsigned long			cntES;
} atmConnectionDataStat;

typedef struct _atmConnectionStat {
	atmConnectionDataStat	rcvStat;
	atmConnectionDataStat	xmtStat;
} atmConnectionStat;

#define	kAdslFramingModeMask			0x0F
#define	kAtmFramingModeMask				0xF0
#define	kAtmHeaderCompression			0x80

/* ADSL2 data */

typedef struct _adslDiagModeData {
	long					loopAttn;
	long					signalAttn;
	long					snrMargin;
	long					attnDataRate;
	long					actXmtPower;
} adslDiagModeData;

typedef struct _adsl2ConnectionInfo {
	long					adsl2Mode;
	long					rcvRate;
	long					xmtRate;
} adsl2ConnectionInfo;

/* AdslMibGetObjectValue return codes */

#define	kAdslMibStatusSuccess			0
#define	kAdslMibStatusFailure			-1
#define	kAdslMibStatusNoObject			-2
#define	kAdslMibStatusObjectInvalid		-3
#define	kAdslMibStatusBufferTooSmall	-4
#define	kAdslMibStatusLastError			-4

/* Adsl training codes */

#define	kAdslTrainingIdle				0
#define	kAdslTrainingG994				1
#define	kAdslTrainingG992Started		2
#define	kAdslTrainingG992ChanAnalysis	3
#define	kAdslTrainingG992Exchange		4
#define	kAdslTrainingConnected			5

/* Global info structure */

typedef struct _adslMibInfo {
	adslLineEntry			adslLine;
	adslPhysEntry			adslPhys;
	adslChanEntry			adslChanIntl;
	adslChanEntry			adslChanFast;
	adslPerfDataEntry		adslPerfData;
	adslPerfCounters		adslPerfIntervals[kAdslMibPerfIntervals];
	adslChanPerfDataEntry	adslChanIntlPerfData;
	adslChanPerfDataEntry	adslChanFastPerfData;
	adslChanCounters		adslChanIntlPerfIntervals[kAdslMibChanPerfIntervals];
	adslChanCounters		adslChanFastPerfIntervals[kAdslMibChanPerfIntervals];

	adslThreshCounters		adslAlarm;

	atmPhyDataEntrty		adslChanIntlAtmPhyData;
	atmPhyDataEntrty		adslChanFastAtmPhyData;

	adslBertResults			adslBertRes;

	adslConnectionInfo		adslConnection;
	adslConnectionStat		adslStat;
	unsigned char			adslTrainingState;
	atmConnectionStat		atmStat;

	adslFullPhysEntry		adslAtucPhys;
	unsigned char			adslRxNonStdFramingAdjustK;
	unsigned char			adslFramingMode;
	adslBertStatusEx		adslBertStatus;
	long					afeRxPgaGainQ1;

	adslDiagModeData		adslDiag;
	adsl2ConnectionInfo		adsl2Info;
} adslMibInfo;

#if defined(__cplusplus)
}
#endif

#endif	/* AdslMibDefHeader */
