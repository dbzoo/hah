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

/*******************************************************************
 * DiagDef.h
 *
 *	Description:
 *		Diag definitions
 *
 * $Revision: 1.2 $
 *
 * $Id: DiagDef.h,v 1.2 2004/03/31 08:44:33 michaelc Exp $
 *
 * $Log: DiagDef.h,v $
 * Revision 1.2  2004/03/31 08:44:33  michaelc
 * Upgrade from V2.14.1a to V2.14.2 and test RTA770W ok
 *
 * Revision 1.18  2004/01/24 23:41:37  ilyas
 * Added DIAG_DEBUG_CMD_LOG_SAMPLES debug command
 *
 * Revision 1.17  2003/11/19 02:25:45  ilyas
 * Added definitions for LOG frame retransmission, time, ADSL2 plots
 *
 * Revision 1.16  2003/11/14 18:46:05  ilyas
 * Added G992p3 debug commands
 *
 * Revision 1.15  2003/10/02 19:50:41  ilyas
 * Added support for buffering data for AnnexI and statistical counters
 *
 * Revision 1.14  2003/09/03 19:45:11  ilyas
 * To refuse connection with older protocol versions
 *
 * Revision 1.13  2003/08/30 00:12:39  ilyas
 * Added support for running chip test regressions via DslDiags
 *
 * Revision 1.12  2003/08/12 00:19:28  ilyas
 * Improved image downloading protocol.
 * Added DEBUG command support
 *
 * Revision 1.11  2003/04/11 00:37:24  ilyas
 * Added DiagProtoFrame definition
 *
 * Revision 1.10  2003/03/25 00:10:07  ilyas
 * Added command for "long" BERT test
 *
 * Revision 1.9  2003/01/30 03:29:32  ilyas
 * Added PHY_CFG support and fixed printing showtime counters
 *
 * Revision 1.8  2002/12/16 20:56:38  ilyas
 * Added support for binary statuses
 *
 * Revision 1.7  2002/12/06 20:19:13  ilyas
 * Added support for binary statuses and scrambled status strings
 *
 * Revision 1.6  2002/11/05 00:18:27  ilyas
 * Added configuration dialog box for Eye tone selection.
 * Added Centillium CRC workaround to AnnexC config dialog
 * Bit allocation update on bit swap messages
 *
 * Revision 1.5  2002/07/30 23:23:43  ilyas
 * Implemented DIAG configuration command for AnnexA and AnnexC
 *
 * Revision 1.4  2002/07/30 22:47:15  ilyas
 * Added DIAG command for configuration
 *
 * Revision 1.3  2002/07/15 23:52:51  ilyas
 * iAdded switch RJ11 pair command
 *
 * Revision 1.2  2002/04/25 17:55:51  ilyas
 * Added mibGet command
 *
 * Revision 1.1  2002/04/02 22:56:39  ilyas
 * Support DIAG connection at any time; BERT commands
 *
 *
 ******************************************************************/

#define	LOG_PROTO_ID				"*L"

#define	DIAG_PARTY_ID_MASK			0x01
#define	LOG_PARTY_CLIENT			0x01
#define	LOG_PARTY_SERVER			0x00

#define	DIAG_DATA_MASK				0x0E
#define	DIAG_DATA_LOG				0x02
#define	DIAG_DATA_EYE				0x04
#define	DIAG_DATA_LOG_TIME			0x08

#define	DIAG_DATA_EX				0x80
#define	DIAG_PARTY_ID_MASK_EX		(DIAG_DATA_EX | DIAG_PARTY_ID_MASK)
#define	LOG_PARTY_SERVER_EX			(DIAG_DATA_EX | LOG_PARTY_SERVER)

#define	DIAG_ACK_FRAME_ACK_MASK		0x000000FF
#define	DIAG_ACK_FRAME_RCV_SHIFT	8
#define	DIAG_ACK_FRAME_RCV_MASK		0x0000FF00
#define	DIAG_ACK_FRAME_RCV_PRESENT	0x00010000
#define	DIAG_ACK_TIMEOUT			-1
#define	DIAG_ACK_LEN_INDICATION		-1

#define	LOG_CMD_RETR				238
#define	LOG_CMD_DEBUG				239
#define	LOG_CMD_BERT_EX				240
#define	LOG_CMD_CFG_PHY				241
#define	LOG_CMD_RESET				242
#define	LOG_CMD_SCRAMBLED_STRING	243
#define	LOG_CMD_EYE_CFG				244
#define	LOG_CMD_CONFIG_A			245
#define	LOG_CMD_CONFIG_C			246
#define	LOG_CMD_SWITCH_RJ11_PAIR	247
#define	LOG_CMD_MIB_GET				248
#define	LOG_CMD_LOG_STOP			249
#define	LOG_CMD_PING_REQ			250
#define	LOG_CMD_PING_RSP			251
#define	LOG_CMD_DISCONNECT			252
#define	LOG_CMD_STRING_DATA			253
#define	LOG_CMD_TEST_DATA			254
#define	LOG_CMD_CONNECT				255

typedef struct _LogProtoHeader {
	unsigned char	logProtoId[2];
	unsigned char	logPartyId;
	unsigned char	logCommmand;
} LogProtoHeader;

#define	LOG_FILE_PORT			5100
#define	LOG_MAX_BUF_SIZE		1400
#define	LOG_MAX_DATA_SIZE		(LOG_MAX_BUF_SIZE - sizeof(LogProtoHeader))

typedef struct {
	LogProtoHeader	diagHdr;
	unsigned char	diagData[LOG_MAX_DATA_SIZE];
} DiagProtoFrame;

#define	DIAG_DEBUG_CMD_READ_MEM				1
#define	DIAG_DEBUG_CMD_SET_MEM				2
#define	DIAG_DEBUG_CMD_RESET_CONNECTION		3
#define	DIAG_DEBUG_CMD_RESET_PHY			4
#define	DIAG_DEBUG_CMD_RESET_CHIP			5
#define	DIAG_DEBUG_CMD_EXEC_FUNC			6
#define	DIAG_DEBUG_CMD_EXEC_ADSL_FUNC		7
#define	DIAG_DEBUG_CMD_WRITE_FILE			8
#define	DIAG_DEBUG_CMD_G992P3_DEBUG			9
#define	DIAG_DEBUG_CMD_G992P3_DIAG_MODE		10
#define	DIAG_DEBUG_CMD_CLEAR_TIME			11
#define	DIAG_DEBUG_CMD_PRINT_TIME			12
#define	DIAG_DEBUG_CMD_LOG_SAMPLES			13

#define	DIAG_DEBUG_CMD_PRINT_STAT			21
#define	DIAG_DEBUG_CMD_CLEAR_STAT			22

typedef struct {
	unsigned short	cmd;
	unsigned short	cmdId;
	unsigned long	param1;
	unsigned long	param2;
	unsigned char	diagData[1];
} DiagDebugData;

#define	DIAG_TEST_CMD_LOAD					101
#define	DIAG_TEST_CMD_READ					102
#define	DIAG_TEST_CMD_WRITE					103
#define	DIAG_TEST_CMD_APPEND				104
#define	DIAG_TEST_CMD_TEST_COMPLETE			105

#define	DIAG_TEST_FILENAME_LEN				64

typedef struct {
	unsigned short	cmd;
	unsigned short	cmdId;
	unsigned long	offset;
	unsigned long	len;
	unsigned long	bufPtr;
	char			fileName[DIAG_TEST_FILENAME_LEN];
} DiagTestData;

typedef struct {
	unsigned long	frStart;
	unsigned long	frNum;
} DiagLogRetrData;
