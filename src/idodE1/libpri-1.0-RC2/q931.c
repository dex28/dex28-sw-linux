/*
 * libpri: An implementation of Primary Rate ISDN
 *
 * Written by Mark Spencer <markster@linux-support.net>
 *
 * Copyright (C) 2001, Linux Support Services, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */
 
#include "libpri.h"
#include "pri_internal.h"
#include "pri_q921.h"
#include "pri_q931.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_MAND_IES 10

struct msgtype {
	int msgnum;
	unsigned char *name;
	int mandies[MAX_MAND_IES];
};

struct msgtype msgs[] = {
	/* Call establishment messages */
	{ Q931_ALERTING, "ALERTING" },
	{ Q931_CALL_PROCEEDING, "CALL PROCEEDING" },
	{ Q931_CONNECT, "CONNECT" },
	{ Q931_CONNECT_ACKNOWLEDGE, "CONNECT ACKNOWLEDGE" },
	{ Q931_PROGRESS, "PROGRESS", { Q931_PROGRESS_INDICATOR } },
	{ Q931_SETUP, "SETUP", { Q931_BEARER_CAPABILITY, Q931_CHANNEL_IDENT } },
	{ Q931_SETUP_ACKNOWLEDGE, "SETUP ACKNOWLEDGE" },
	
	/* Call disestablishment messages */
	{ Q931_DISCONNECT, "DISCONNECT", { Q931_CAUSE } },
	{ Q931_RELEASE, "RELEASE" },
	{ Q931_RELEASE_COMPLETE, "RELEASE COMPLETE" },
	{ Q931_RESTART, "RESTART", { Q931_RESTART_INDICATOR } },
	{ Q931_RESTART_ACKNOWLEDGE, "RESTART ACKNOWLEDGE", { Q931_RESTART_INDICATOR } },

	/* Miscellaneous */
	{ Q931_STATUS, "STATUS", { Q931_CAUSE, Q931_CALL_STATE } },
	{ Q931_STATUS_ENQUIRY, "STATUS ENQUIRY" },
	{ Q931_USER_INFORMATION, "USER_INFORMATION" },
	{ Q931_SEGMENT, "SEGMENT" },
	{ Q931_CONGESTION_CONTROL, "CONGESTION CONTROL" },
	{ Q931_INFORMATION, "INFORMATION" },
	{ Q931_FACILITY, "FACILITY" },
	{ Q931_NOTIFY, "NOTIFY", { Q931_IE_NOTIFY_IND } },

	/* Call Management */
	{ Q931_HOLD, "HOLD" },
	{ Q931_HOLD_ACKNOWLEDGE, "HOLD ACKNOWLEDGE" },
	{ Q931_HOLD_REJECT, "HOLD REJECT" },
	{ Q931_RETRIEVE, "RETRIEVE" },
	{ Q931_RETRIEVE_ACKNOWLEDGE, "RETRIEVE ACKNOWLEDGE" },
	{ Q931_RETRIEVE_REJECT, "RETRIEVE REJECT" },
	{ Q931_RESUME, "RESUME" },
	{ Q931_RESUME_ACKNOWLEDGE, "RESUME ACKNOWLEDGE", { Q931_CHANNEL_IDENT } },
	{ Q931_RESUME_REJECT, "RESUME REJECT", { Q931_CAUSE } },
	{ Q931_SUSPEND, "SUSPEND" },
	{ Q931_SUSPEND_ACKNOWLEDGE, "SUSPEND ACKNOWLEDGE" },
	{ Q931_SUSPEND_REJECT, "SUSPEND REJECT" },

	/* Maintenance */
	{ NATIONAL_SERVICE, "SERVICE" },
	{ NATIONAL_SERVICE_ACKNOWLEDGE, "SERVICE ACKNOWLEDGE" },
};

struct msgtype causes[] = {
	{ PRI_CAUSE_UNALLOCATED, "Unallocated (unassigned) number" },
	{ PRI_CAUSE_NO_ROUTE_TRANSIT_NET, "No route to specified transmit network" },
	{ PRI_CAUSE_NO_ROUTE_DESTINATION, "No route to destination" },
	{ PRI_CAUSE_CHANNEL_UNACCEPTABLE, "Channel unacceptable" },
	{ PRI_CAUSE_CALL_AWARDED_DELIVERED, "Call awarded and being delivered in an established channel" },
	{ PRI_CAUSE_NORMAL_CLEARING, "Normal Clearing" },
	{ PRI_CAUSE_USER_BUSY, "User busy" },
	{ PRI_CAUSE_NO_USER_RESPONSE, "No user responding" },
	{ PRI_CAUSE_NO_ANSWER, "User alerting, no answer" },
	{ PRI_CAUSE_CALL_REJECTED, "Call Rejected" },
	{ PRI_CAUSE_NUMBER_CHANGED, "Number changed" },
	{ PRI_CAUSE_DESTINATION_OUT_OF_ORDER, "Destination out of order" },
	{ PRI_CAUSE_INVALID_NUMBER_FORMAT, "Invalid number format" },
	{ PRI_CAUSE_FACILITY_REJECTED, "Facility rejected" },
	{ PRI_CAUSE_RESPONSE_TO_STATUS_ENQUIRY, "Response to STATus ENQuiry" },
	{ PRI_CAUSE_NORMAL_UNSPECIFIED, "Normal, unspecified" },
	{ PRI_CAUSE_NORMAL_CIRCUIT_CONGESTION, "Circuit/channel congestion" },
	{ PRI_CAUSE_NETWORK_OUT_OF_ORDER, "Network out of order" },
	{ PRI_CAUSE_NORMAL_TEMPORARY_FAILURE, "Temporary failure" },
	{ PRI_CAUSE_SWITCH_CONGESTION, "Switching equipment congestion" },
	{ PRI_CAUSE_ACCESS_INFO_DISCARDED, "Access information discarded" },
	{ PRI_CAUSE_REQUESTED_CHAN_UNAVAIL, "Requested channel not available" },
	{ PRI_CAUSE_PRE_EMPTED, "Pre-empted" },
	{ PRI_CAUSE_FACILITY_NOT_SUBSCRIBED, "Facility not subscribed" },
	{ PRI_CAUSE_OUTGOING_CALL_BARRED, "Outgoing call barred" },
	{ PRI_CAUSE_INCOMING_CALL_BARRED, "Incoming call barred" },
	{ PRI_CAUSE_BEARERCAPABILITY_NOTAUTH, "Bearer capability not authorized" },
	{ PRI_CAUSE_BEARERCAPABILITY_NOTAVAIL, "Bearer capability not available" },
	{ PRI_CAUSE_BEARERCAPABILITY_NOTIMPL, "Bearer capability not implemented" },
	{ PRI_CAUSE_CHAN_NOT_IMPLEMENTED, "Channel not implemented" },
	{ PRI_CAUSE_FACILITY_NOT_IMPLEMENTED, "Facility not implemented" },
	{ PRI_CAUSE_INVALID_CALL_REFERENCE, "Invalid call reference value" },
	{ PRI_CAUSE_INCOMPATIBLE_DESTINATION, "Incompatible destination" },
	{ PRI_CAUSE_INVALID_MSG_UNSPECIFIED, "Invalid message unspecified" },
	{ PRI_CAUSE_MANDATORY_IE_MISSING, "Mandatory information element is missing" },
	{ PRI_CAUSE_MESSAGE_TYPE_NONEXIST, "Message type nonexist." },
	{ PRI_CAUSE_WRONG_MESSAGE, "Wrong message" },
	{ PRI_CAUSE_IE_NONEXIST, "Info. element nonexist or not implemented" },
	{ PRI_CAUSE_INVALID_IE_CONTENTS, "Invalid information element contents" },
	{ PRI_CAUSE_WRONG_CALL_STATE, "Message not compatible with call state" },
	{ PRI_CAUSE_RECOVERY_ON_TIMER_EXPIRE, "Recover on timer expiry" },
	{ PRI_CAUSE_MANDATORY_IE_LENGTH_ERROR, "Mandatory IE length error" },
	{ PRI_CAUSE_PROTOCOL_ERROR, "Protocol error, unspecified" },
	{ PRI_CAUSE_INTERWORKING, "Interworking, unspecified" },
};

struct msgtype facilities[] = {
       { PRI_NSF_SID_PREFERRED, "CPN (SID) preferred" },
       { PRI_NSF_ANI_PREFERRED, "BN (ANI) preferred" },
       { PRI_NSF_SID_ONLY, "CPN (SID) only" },
       { PRI_NSF_ANI_ONLY, "BN (ANI) only" },
       { PRI_NSF_CALL_ASSOC_TSC, "Call Associated TSC" },
       { PRI_NSF_NOTIF_CATSC_CLEARING, "Notification of CATSC Clearing or Resource Unavailable" },
       { PRI_NSF_OPERATOR, "Operator" },
       { PRI_NSF_PCCO, "Pre-subscribed Common Carrier Operator (PCCO)" },
       { PRI_NSF_SDN, "SDN (including GSDN)" },
       { PRI_NSF_TOLL_FREE_MEGACOM, "Toll Free MEGACOM" },
       { PRI_NSF_MEGACOM, "MEGACOM" },
       { PRI_NSF_ACCUNET, "ACCUNET Switched Digital Service" },
       { PRI_NSF_LONG_DISTANCE_SERVICE, "Long Distance Service" },
       { PRI_NSF_INTERNATIONAL_TOLL_FREE, "International Toll Free Service" },
       { PRI_NSF_ATT_MULTIQUEST, "AT&T MultiQuest" },
       { PRI_NSF_CALL_REDIRECTION_SERVICE, "Call Redirection Service" }
};

#define FLAG_PREFERRED 2
#define FLAG_EXCLUSIVE 4

#define RESET_INDICATOR_CHANNEL	0
#define RESET_INDICATOR_DS1		6
#define RESET_INDICATOR_PRI		7

#define TRANS_MODE_64_CIRCUIT	0x10
#define TRANS_MODE_2x64_CIRCUIT	0x11
#define TRANS_MODE_384_CIRCUIT	0x13
#define TRANS_MODE_1536_CIRCUIT	0x15
#define TRANS_MODE_1920_CIRCUIT	0x17
#define TRANS_MODE_MULTIRATE	0x18
#define TRANS_MODE_PACKET		0x40

#define RATE_ADAPT_56K			0x0f

#define LAYER_2_LAPB			0x46

#define LAYER_3_X25				0x66

/* The 4ESS uses a different audio field */
#define PRI_TRANS_CAP_AUDIO_4ESS	0x08


#define Q931_PROG_CALL_NOT_E2E_ISDN						0x01
#define Q931_PROG_CALLED_NOT_ISDN						0x02
#define Q931_PROG_CALLER_NOT_ISDN						0x03
#define Q931_PROG_INBAND_AVAILABLE						0x08
#define Q931_PROG_DELAY_AT_INTERF						0x0a
#define Q931_PROG_INTERWORKING_WITH_PUBLIC				0x10
#define Q931_PROG_INTERWORKING_NO_RELEASE				0x11
#define Q931_PROG_INTERWORKING_NO_RELEASE_PRE_ANSWER	0x12
#define Q931_PROG_INTERWORKING_NO_RELEASE_POST_ANSWER	0x13

#define CODE_CCITT					0x0
#define CODE_INTERNATIONAL 			0x1
#define CODE_NATIONAL 				0x2
#define CODE_NETWORK_SPECIFIC		0x3

#define LOC_USER					0x0
#define LOC_PRIV_NET_LOCAL_USER		0x1
#define LOC_PUB_NET_LOCAL_USER		0x2
#define LOC_TRANSIT_NET				0x3
#define LOC_PUB_NET_REMOTE_USER		0x4
#define LOC_PRIV_NET_REMOTE_USER	0x5
#define LOC_INTERNATIONAL_NETWORK	0x7
#define LOC_NETWORK_BEYOND_INTERWORKING	0xa

#define T_308			4000
#define T_305			30000
#define T_313			4000

struct q931_call {
	struct pri *pri;	/* PRI */
	int cr;		/* Call Reference */
	int forceinvert;	/* Force inversion of call number even if 0 */
	q931_call *next;
	/* Slotmap specified (bitmap of channels 31/24-1) (Channel Identifier IE) (-1 means not specified) */
	int slotmap;
	/* An explicit channel (Channel Identifier IE) (-1 means not specified) */
	int channelno;
	/* An explicit DS1 (-1 means not specified) */
	int ds1no;
	/* Channel flags (0 means none retrieved) */
	int chanflags;
	
	int alive;			/* Whether or not the call is alive */
	int acked;			/* Whether setup has been acked or not */
	int sendhangupack;		/* Whether or not to send a hangup ack */
	int proc;			/* Whether we've sent a call proceeding / alerting */
	
	int ri;			/* Restart Indicator (Restart Indicator IE) */

	/* Bearer Capability */
	int transcapability;
	int transmoderate;
	int transmultiple;
	int userl1;
	int userl2;
	int userl3;
	int rateadaption;
	
	int sentchannel;

	int progcode;			/* Progress coding */
	int progloc;			/* Progress Location */	
	int progress;			/* Progress indicator */
	
	int notify;			/* Notification */
	
	int causecode;			/* Cause Coding */
	int causeloc;			/* Cause Location */
	int cause;				/* Cause of clearing */
	
	int peercallstate;			/* Call state of peer as reported */
	int ourcallstate;		/* Our call state */
	int sugcallstate;		/* Status call state */
	
	int callerplan;
	int callerpres;			/* Caller presentation */
	char callernum[256];	/* Caller */
	char callername[256];
	
	int  calledplan;
	int nonisdn;
	char callednum[256];	/* Called Number */
	int complete;			/* no more digits coming */
	int newcall;		/* if the received message has a new call reference value */

	int retranstimer;		/* Timer for retransmitting DISC */
	int t308_timedout;		/* Whether t308 timed out once */
	int redirectingplan;
	int redirectingpres;
	int redirectingreason;	      
	char redirectingnum[256];

        int useruserprotocoldisc;
	char useruserinfo[256];
	char callingsubaddr[256];	/* Calling parties sub address */
};

#define FUNC_DUMP(name) void ((name))(int full_ie, q931_ie *ie, int len, char prefix)
#define FUNC_RECV(name) int ((name))(int full_ie, struct pri *pri, q931_call *call, int msgtype, q931_ie *ie, int len)
#define FUNC_SEND(name) int ((name))(int full_ie, struct pri *pri, q931_call *call, int msgtype, q931_ie *ie, int len)


struct ie {
	int ie;
	char *name;
	/* Dump an IE for debugging (preceed all lines by prefix) */
	FUNC_DUMP(*dump);
	/* Handle IE  returns 0 on success, -1 on failure */
	FUNC_RECV(*receive);
	/* Add IE to a message, return the # of bytes added or -1 on failure */
	FUNC_SEND(*transmit);
};

static char *code2str(int code, struct msgtype *codes, int max)
{
	int x;
	for (x=0;x<max; x++) 
		if (codes[x].msgnum == code)
			return codes[x].name;
	return "Unknown";
}

static void call_init(struct q931_call *c)
{
	memset(c, 0, sizeof(*c));
	c->alive = 0;
	c->sendhangupack = 0;
	c->forceinvert = -1;	
	c->cr = -1;
	c->slotmap = -1;
	c->channelno = -1;
	c->ds1no = 0;
	c->chanflags = 0;
	c->next = NULL;
	c->sentchannel = 0;
	c->newcall = 1;
	c->ourcallstate = Q931_CALL_STATE_NULL;
	c->peercallstate = Q931_CALL_STATE_NULL;
}

static char *binary(int b, int len) {
	static char res[33];
	int x;
	memset(res, 0, sizeof(res));
	if (len > 32)
		len = 32;
	for (x=1;x<=len;x++)
		res[x-1] = b & (1 << (len - x)) ? '1' : '0';
	return res;
}

static FUNC_RECV(receive_channel_id)
{	
	int x;
	int pos=0;
#ifdef NO_BRI_SUPPORT
 	if (!ie->data[0] & 0x20) {
		pri_error("!! Not PRI type!?\n");
 		return -1;
 	}
#endif
#ifndef NOAUTO_CHANNEL_SELECTION_SUPPORT
	if ((ie->data[0] & 3) != 1) {
		pri_error("!! Unexpected Channel selection %d\n", ie->data[0] & 3);
		return -1;
	}
#endif
	if (ie->data[0] & 0x08)
		call->chanflags = FLAG_EXCLUSIVE;
	else
		call->chanflags = FLAG_PREFERRED;
	pos++;
	if (ie->data[0] & 0x40) {
		/* DS1 specified -- stop here */
		call->ds1no = ie->data[1] & 0x7f;
		pos++;
	} 
	if (pos+2 < len) {
		/* More coming */
		if ((ie->data[pos] & 0x0f) != 3) {
			pri_error("!! Unexpected Channel Type %d\n", ie->data[1] & 0x0f);
			return -1;
		}
		if ((ie->data[pos] & 0x60) != 0) {
			pri_error("!! Invalid CCITT coding %d\n", (ie->data[1] & 0x60) >> 5);
			return -1;
		}
		if (ie->data[pos] & 0x10) {
			/* Expect Slot Map */
			call->slotmap = 0;
			pos++;
			for (x=0;x<3;x++) {
				call->slotmap <<= 8;
				call->slotmap |= ie->data[x + pos];
			}
			return 0;
		} else {
			pos++;
			/* Only expect a particular channel */
			call->channelno = ie->data[pos] & 0x7f;
			return 0;
		}
	} else
		return 0;
	return -1;
}

static FUNC_SEND(transmit_channel_id)
{
	int pos=0;
	/* Start with standard stuff */
	if (pri->switchtype == PRI_SWITCH_GR303_TMC)
		ie->data[pos] = 0x69;
	else
		ie->data[pos] = 0xa1;
	/* Add exclusive flag if necessary */
	if (call->chanflags & FLAG_EXCLUSIVE)
		ie->data[pos] |= 0x08;
	else if (!(call->chanflags & FLAG_PREFERRED)) {
		/* Don't need this IE */
		return 0;
	}

	if (call->ds1no > 0) {
		/* Note that we are specifying the identifier */
		ie->data[pos++] |= 0x40;
		/* We need to use the Channel Identifier Present thingy.  Just specify it and we're done */
		ie->data[pos++] = 0x80 | call->ds1no;
	} else
		pos++;
	if ((call->channelno > -1) || (call->slotmap != -1)) {
		/* We'll have the octet 8.2 and 8.3's present */
		ie->data[pos++] = 0x83;
		if (call->channelno > -1) {
			/* Channel number specified */
			ie->data[pos++] = 0x80 | call->channelno;
			return pos + 2;
		}
		/* We have to send a channel map */
		if (call->slotmap != -1) {
			ie->data[pos-1] |= 0x10;
			ie->data[pos++] = (call->slotmap & 0xff0000) >> 16;
			ie->data[pos++] = (call->slotmap & 0xff00) >> 8;
			ie->data[pos++] = (call->slotmap & 0xff);
			return pos + 2;
		}
	}
	if (call->ds1no > 0) {
		/* We're done */
		return pos + 2;
	}
	pri_error("!! No channel map, no channel, and no ds1?  What am I supposed to identify?\n");
	return -1;
}

static FUNC_DUMP(dump_channel_id)
{
	int pos=0;
	int x;
	int res = 0;
	static const char*	msg_chan_sel[] = {
		"No channel selected", "B1 channel", "B2 channel","Any channel selected" 
		"No channel selected", "As indicated in following octets", "Reserved","Any channel selected"
	};

	pri_message("%c Channel ID (len=%2d) [ Ext: %d  IntID: %s, %s Spare: %d, %s Dchan: %d\n",
		prefix, len, (ie->data[0] & 0x80) ? 1 : 0, (ie->data[0] & 0x40) ? "Explicit" : "Implicit",
		(ie->data[0] & 0x20) ? "PRI" : "Other", (ie->data[0] & 0x10) ? 1 : 0,
		(ie->data[0] & 0x08) ? "Exclusive" : "Preferred",  (ie->data[0] & 0x04) ? 1 : 0);
	pri_message("%c                        ChanSel: %s\n",
		prefix, msg_chan_sel[(ie->data[0] & 0x3) + ((ie->data[0]>>3) & 0x4)]);
	pos++;
	len--;
	if (ie->data[0] &  0x40) {
		/* Explicitly defined DS1 */
		pri_message("%c                       Ext: %d  DS1 Identifier: %d  \n", prefix, (ie->data[pos] & 0x80) >> 7, ie->data[pos] & 0x7f);
		pos++;
	} else {
		/* Implicitly defined DS1 */
	}
	if (pos+2 < len) {
		/* Still more information here */
		pri_message("%c                       Ext: %d  Coding: %d   %s Specified   Channel Type: %d\n",
				prefix, (ie->data[pos] & 0x80) >> 7, (ie->data[pos] & 60) >> 5, 
				(ie->data[pos] & 0x10) ? "Slot Map" : "Number", ie->data[pos] & 0x0f);
		if (!(ie->data[pos] & 0x10)) {
			/* Number specified */
			pos++;
			pri_message("%c                       Ext: %d  Channel: %d ]\n", prefix, (ie->data[pos] & 0x80) >> 7, 
				(ie->data[pos]) & 0x7f);
		} else {
			pos++;
			/* Map specified */
			for (x=0;x<3;x++) {
				res <<= 8;
				res |= ie->data[pos++];
			}
			pri_message("%c                       Map: %s ]\n", prefix, binary(res, 24));
		}
	} else pri_message("                         ]\n");				
}

static char *ri2str(int ri)
{
	static struct msgtype ris[] = {
		{ 0, "Indicated Channel" },
		{ 6, "Single DS1 Facility" },
		{ 7, "All DS1 Facilities" },
	};
	return code2str(ri, ris, sizeof(ris) / sizeof(ris[0]));
}

static FUNC_DUMP(dump_restart_indicator)
{
	pri_message("%c Restart Indentifier (len=%2d) [ Ext: %d  Spare: %d  Resetting %s (%d) ]\n", 
		prefix, len, (ie->data[0] & 0x80) >> 7, (ie->data[0] & 0x78) >> 3, ri2str(ie->data[0] & 0x7), ie->data[0] & 0x7);
}

static FUNC_RECV(receive_restart_indicator)
{
	/* Pretty simple */
	call->ri = ie->data[0] & 0x7;
	return 0;
}

static FUNC_SEND(transmit_restart_indicator)
{
	/* Pretty simple */
	switch(call->ri) {
	case 0:
	case 6:
	case 7:
		ie->data[0] = 0x80 | (call->ri & 0x7);
		break;
	case 5:
		/* Switch compatibility */
		ie->data[0] = 0xA0 | (call->ri & 0x7);
		break;
	default:
		pri_error("!! Invalid restart indicator value %d\n", call->ri);
		return-1;
	}
	return 3;
}

static char *redirection_reason2str(int mode)
{
	static struct msgtype modes[] = {
		{ PRI_REDIR_UNKNOWN, "Unknown" },
		{ PRI_REDIR_FORWARD_ON_BUSY, "Forwarded on busy" },
		{ PRI_REDIR_FORWARD_ON_NO_REPLY, "Forwarded on no reply" },
		{ PRI_REDIR_DEFLECTION, "Call deflected" },
		{ PRI_REDIR_DTE_OUT_OF_ORDER, "Called DTE out of order" },
		{ PRI_REDIR_FORWARDED_BY_DTE, "Forwarded by called DTE" },
		{ PRI_REDIR_UNCONDITIONAL, "Forwarded unconditionally" },
	};
	return code2str(mode, modes, sizeof(modes) / sizeof(modes[0]));
}

static char *cap2str(int mode)
{
	static struct msgtype modes[] = {
		{ PRI_TRANS_CAP_SPEECH, "Speech" },
		{ PRI_TRANS_CAP_DIGITAL, "Unrestricted digital information" },
		{ PRI_TRANS_CAP_RESTRICTED_DIGITAL, "Restricted digital information" },
		{ PRI_TRANS_CAP_3_1K_AUDIO, "3.1kHz audio" },
		{ PRI_TRANS_CAP_7K_AUDIO, "7kHz audio" },
		{ PRI_TRANS_CAP_VIDEO, "Video" },
		{ PRI_TRANS_CAP_AUDIO_4ESS, "3.1khz audio (4ESS)" },
	};
	return code2str(mode, modes, sizeof(modes) / sizeof(modes[0]));
}

static char *mode2str(int mode)
{
	static struct msgtype modes[] = {
		{ TRANS_MODE_64_CIRCUIT, "64kbps, circuit-mode" },
		{ TRANS_MODE_2x64_CIRCUIT, "2x64kbps, circuit-mode" },
		{ TRANS_MODE_384_CIRCUIT, "384kbps, circuit-mode" },
		{ TRANS_MODE_1536_CIRCUIT, "1536kbps, circuit-mode" },
		{ TRANS_MODE_1920_CIRCUIT, "1920kbps, circuit-mode" },
		{ TRANS_MODE_MULTIRATE, "Multirate (Nx64kbps)" },
		{ TRANS_MODE_PACKET, "Packet Mode" },
	};
	return code2str(mode, modes, sizeof(modes) / sizeof(modes[0]));
}

static char *l12str(int proto)
{
	static struct msgtype protos[] = {
		{ PRI_LAYER_1_ITU_RATE_ADAPT, "ITU Rate Adaption" },
		{ PRI_LAYER_1_ULAW, "u-Law" },
		{ PRI_LAYER_1_ALAW, "A-Law" },
		{ PRI_LAYER_1_G721, "G.721 ADPCM" },
		{ PRI_LAYER_1_G722_G725, "G.722/G.725 7kHz Audio" },
		{ PRI_LAYER_1_G7XX_384K, "G.7xx 384k Video" },
		{ PRI_LAYER_1_NON_ITU_ADAPT, "Non-ITU Rate Adaption" },
		{ PRI_LAYER_1_V120_RATE_ADAPT, "V.120 Rate Adaption" },
		{ PRI_LAYER_1_X31_RATE_ADAPT, "X.31 Rate Adaption" },
	};
	return code2str(proto, protos, sizeof(protos) / sizeof(protos[0]));
}

static char *ra2str(int proto)
{
	static struct msgtype protos[] = {
		{ RATE_ADAPT_56K, "from 56kbps" },
	};
	return code2str(proto, protos, sizeof(protos) / sizeof(protos[0]));
}

static char *l22str(int proto)
{
	static struct msgtype protos[] = {
		{ LAYER_2_LAPB, "LAPB" },
	};
	return code2str(proto, protos, sizeof(protos) / sizeof(protos[0]));
}

static char *l32str(int proto)
{
	static struct msgtype protos[] = {
		{ LAYER_3_X25, "X.25" },
	};
	return code2str(proto, protos, sizeof(protos) / sizeof(protos[0]));
}

static FUNC_DUMP(dump_bearer_capability)
{
	int pos=2;
	pri_message("%c Bearer Capability (len=%2d) [ Ext: %d  Q.931 Std: %d  Info transfer capability: %s (%d)\n",
		prefix, len, (ie->data[0] & 0x80 ) >> 7, (ie->data[0] & 0x60) >> 5, cap2str(ie->data[0] & 0x1f), (ie->data[0] & 0x1f));
	pri_message("%c                              Ext: %d  Trans mode/rate: %s (%d)\n", prefix, (ie->data[1] & 0x80) >> 7, mode2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f);
	if ((ie->data[1] & 0x7f) == 0x18) {
	    pri_message("%c                              Ext: %d  Transfer rate multiplier: %d x 64\n", prefix, (ie->data[2] & 0x80) >> 7, ie->data[2] & 0x7f);
		pos++;
	}
	/* Stop here if no more */
	if (pos >= len)
		return;
	if ((ie->data[1] & 0x7f) != TRANS_MODE_PACKET) {
		/* Look for octets 5 and 5.a if present */
		pri_message("%c                              Ext: %d  User information layer 1: %s (%d)\n", prefix, (ie->data[pos] >> 7), l12str(ie->data[pos] & 0x7f), ie->data[pos] & 0x7f);
		if ((ie->data[pos] & 0x7f) == PRI_LAYER_1_ITU_RATE_ADAPT)
			pri_message("%c                                Ext: %d  Rate adaptatation: %s (%d)\n", prefix, ie->data[pos] >> 7, ra2str(ie->data[pos] & 0x7f), ie->data[pos] & 0x7f);
		pos++;
	} else {
		/* Look for octets 6 and 7 but not 5 and 5.a */
		pri_message("%c                              Ext: %d  User information layer 2: %s (%d)\n", prefix, ie->data[pos] >> 7, l22str(ie->data[pos] & 0x7f), ie->data[pos] & 0x7f);
		pos++;
		pri_message("%c                              Ext: %d  User information layer 3: %s (%d)\n", prefix, ie->data[pos] >> 7, l32str(ie->data[pos] & 0x7f), ie->data[pos] & 0x7f);
		pos++;
	}
}

static FUNC_RECV(receive_bearer_capability)
{
	int pos=2;
	if (ie->data[0] & 0x60) {
		pri_error("!! non-standard Q.931 standard field\n");
		return -1;
	}
	call->transcapability = ie->data[0] & 0x1f;
	call->transmoderate = ie->data[1] & 0x7f;
	if (call->transmoderate == PRI_TRANS_CAP_AUDIO_4ESS)
		call->transmoderate = PRI_TRANS_CAP_3_1K_AUDIO;
	if (call->transmoderate != TRANS_MODE_PACKET) {
		call->userl1 = ie->data[pos] & 0x7f;
		if (call->userl1 == PRI_LAYER_1_ITU_RATE_ADAPT) {
			call->rateadaption = ie->data[++pos] & 0x7f;
		}
		pos++;
	} else {
		/* Get 6 and 7 */
		call->userl2 = ie->data[pos++] & 0x7f;
		call->userl3 = ie->data[pos] & 0x7f;
	}
	return 0;
}

static FUNC_SEND(transmit_bearer_capability)
{
	int tc;
	tc = call->transcapability;
	if (pri->subchannel) {
		/* Bearer capability is *hard coded* in GR-303 */
		ie->data[0] = 0x88;
		ie->data[1] = 0x90;
		return 4;
	}
	if (pri->switchtype == PRI_SWITCH_ATT4ESS) {
		/* 4ESS uses a different trans capability for 3.1khz audio */
		if (tc == PRI_TRANS_CAP_3_1K_AUDIO)
			tc = PRI_TRANS_CAP_AUDIO_4ESS;
	}
	ie->data[0] = 0x80 | tc;
	ie->data[1] = call->transmoderate | 0x80;
	if ((tc & PRI_TRANS_CAP_DIGITAL)&&(pri->switchtype == PRI_SWITCH_EUROISDN_E1)) {
		/* Apparently EuroISDN switches don't seem to like user layer 2/3 */
		return 4;
	}
	if (call->transmoderate != TRANS_MODE_PACKET) {
		/* If you have an AT&T 4ESS, you don't send any more info */
		if ((pri->switchtype != PRI_SWITCH_ATT4ESS) && (call->userl1 > -1)) {
			ie->data[2] = call->userl1 | 0x80; /* XXX Ext bit? XXX */
			if (call->userl1 == PRI_LAYER_1_ITU_RATE_ADAPT) {
				ie->data[3] = call->rateadaption | 0x80;
				return 6;
			}
			return 5;
		} else
			return 4;
	} else {
		ie->data[2] = 0x80 | call->userl2;
		ie->data[3] = 0x80 | call->userl3;
		return 6;
	}
}

char *pri_plan2str(int plan)
{
	static struct msgtype plans[] = {
		{ PRI_INTERNATIONAL_ISDN, "International number in ISDN" },
		{ PRI_NATIONAL_ISDN, "National number in ISDN" },
		{ PRI_LOCAL_ISDN, "Local number in ISDN" },
		{ PRI_PRIVATE, "Private numbering plan" },
		{ PRI_UNKNOWN, "Unknown numbering plan" },
	};
	return code2str(plan, plans, sizeof(plans) / sizeof(plans[0]));
}

static char *npi2str(int plan)
{
	static struct msgtype plans[] = {
		{ PRI_NPI_UNKNOWN, "Unknown Number Plan" },
		{ PRI_NPI_E163_E164, "ISDN/Telephony Numbering Plan (E.164/E.163)" },
		{ PRI_NPI_X121, "Data Numbering Plan (X.121)" },
		{ PRI_NPI_F69, "Telex Numbering Plan (F.69)" },
		{ PRI_NPI_NATIONAL, "National Standard Numbering Plan" },
		{ PRI_NPI_PRIVATE, "Private Numbering Plan" },
		{ PRI_NPI_RESERVED, "Reserved Number Plan" },
	};
	return code2str(plan, plans, sizeof(plans) / sizeof(plans[0]));
}

static char *ton2str(int plan)
{
	static struct msgtype plans[] = {
		{ PRI_TON_UNKNOWN, "Unknown Number Type" },
		{ PRI_TON_INTERNATIONAL, "International Number" },
		{ PRI_TON_NATIONAL, "National Number" },
		{ PRI_TON_NET_SPECIFIC, "Network Specific Number" },
		{ PRI_TON_SUBSCRIBER, "Subscriber Number" },
		{ PRI_TON_ABBREVIATED, "Abbreviated number" },
		{ PRI_TON_RESERVED, "Reserved Number" },
	};
	return code2str(plan, plans, sizeof(plans) / sizeof(plans[0]));
}

static char *subaddrtype2str(int plan)
{
	static struct msgtype plans[] = {
		{ 0, "NSAP (X.213/ISO 8348 AD2)" },
		{ 2, "User Specified" },
	};
	return code2str(plan, plans, sizeof(plans) / sizeof(plans[0]));
}

char *pri_pres2str(int pres)
{
	static struct msgtype press[] = {
		{ PRES_ALLOWED_USER_NUMBER_NOT_SCREENED, "Presentation permitted, user number not screened" },
		{ PRES_ALLOWED_USER_NUMBER_PASSED_SCREEN, "Presentation permitted, user number passed network screening" },
		{ PRES_ALLOWED_USER_NUMBER_FAILED_SCREEN, "Presentation permitted, user number failed network screening" },
		{ PRES_ALLOWED_NETWORK_NUMBER, "Presentation allowed of network provided number" },
		{ PRES_PROHIB_USER_NUMBER_NOT_SCREENED, "Presentation prohibited, user number not screened" },
		{ PRES_PROHIB_USER_NUMBER_PASSED_SCREEN, "Presentation prohibited, user number passed network screening" },
		{ PRES_PROHIB_USER_NUMBER_FAILED_SCREEN, "Presentation prohibited, user number failed network screening" },
		{ PRES_PROHIB_NETWORK_NUMBER, "Presentation prohibited of network provided number" },
	};
	return code2str(pres, press, sizeof(press) / sizeof(press[0]));
}

static void q931_get_number(unsigned char *num, int maxlen, unsigned char *src, int len)
{
	if (len > maxlen - 1) {
		num[0] = 0;
		return;
	}
	memcpy(num, src, len);
	num[len] = 0;
}

static FUNC_DUMP(dump_called_party_number)
{
	char cnum[256];

	q931_get_number(cnum, sizeof(cnum), ie->data + 1, len - 3);
	pri_message("%c Called Number (len=%2d) [ Ext: %d  TON: %s (%d)  NPI: %s (%d) '%s' ]\n",
		prefix, len, ie->data[0] >> 7, ton2str((ie->data[0] >> 4) & 0x07), (ie->data[0] >> 4) & 0x07, npi2str(ie->data[0] & 0x0f), ie->data[0] & 0x0f, cnum);
}

static FUNC_DUMP(dump_called_party_subaddr)
{
	char cnum[256];
	q931_get_number(cnum, sizeof(cnum), ie->data + 1, len - 3);
	pri_message("%c Called Sub-Address (len=%2d) [ Ext: %d  Type: %s (%d) O: %d '%s' ]\n",
		prefix, len, ie->data[0] >> 7,
		subaddrtype2str((ie->data[0] & 0x70) >> 4), (ie->data[0] & 0x70) >> 4,
		(ie->data[0] & 0x08) >> 3, cnum);
}

static FUNC_DUMP(dump_calling_party_number)
{
	char cnum[256];
	if (ie->data[0] & 0x80)
		q931_get_number(cnum, sizeof(cnum), ie->data + 1, len - 3);
	else
		q931_get_number(cnum, sizeof(cnum), ie->data + 2, len - 4);
	pri_message("%c Calling Number (len=%2d) [ Ext: %d  TON: %s (%d)  NPI: %s (%d)\n", prefix, len, ie->data[0] >> 7, ton2str((ie->data[0] >> 4) & 0x07), (ie->data[0] >> 4) & 0x07, npi2str(ie->data[0] & 0x0f), ie->data[0] & 0x0f);
	if (ie->data[0] & 0x80)
		pri_message("%c                           Presentation: %s (%d) '%s' ]\n", prefix, pri_pres2str(0), 0, cnum);
	else
		pri_message("%c                           Presentation: %s (%d) '%s' ]\n", prefix, pri_pres2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f, cnum);
}

static FUNC_DUMP(dump_calling_party_subaddr)
{
	char cnum[256];
	q931_get_number(cnum, sizeof(cnum), ie->data + 2, len - 4);
	pri_message("%c Calling Sub-Address (len=%2d) [ Ext: %d  Type: %s (%d) O: %d '%s' ]\n",
		prefix, len, ie->data[0] >> 7,
		subaddrtype2str((ie->data[0] & 0x70) >> 4), (ie->data[0] & 0x70) >> 4,
		(ie->data[0] & 0x08) >> 3, cnum);
}

static FUNC_DUMP(dump_redirecting_number)
{
	char cnum[256];
	int i = 0;
	/* To follow Q.931 (4.5.1), we must search for start of octet 4 by
	   walking through all bytes until one with ext bit (8) set to 1 */
	do {
		switch(i) {
		case 0:	/* Octet 3 */
			pri_message("%c Redirecting Number (len=%2d) [ Ext: %d  TON: %s (%d)  NPI: %s (%d)",
				prefix, len, ie->data[0] >> 7, ton2str((ie->data[0] >> 4) & 0x07), (ie->data[0] >> 4) & 0x07, npi2str(ie->data[0] & 0x0f), ie->data[0] & 0x0f);
			break;
		case 1: /* Octet 3a */
			pri_message("\n%c                               Ext: %d Presentation: %s (%d)",
				prefix, ie->data[1] >> 7, pri_pres2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f);
			break;
		case 2: /* Octet 3b */
			pri_message("\n%c                               Ext: %d Reason: %s (%d)",
				prefix, ie->data[2] >> 7, redirection_reason2str(ie->data[2] & 0x7f), ie->data[2] & 0x7f);
			break;
		}
	}
	while(!(ie->data[i++]& 0x80));
	q931_get_number(cnum, sizeof(cnum), ie->data + i, ie->len - i);
	pri_message(" '%s' ]\n", cnum);
}

static FUNC_DUMP(dump_connected_number)
{
	char cnum[256];
	int i = 0;
	/* To follow Q.931 (4.5.1), we must search for start of octet 4 by
	   walking through all bytes until one with ext bit (8) set to 1 */
	do {
		switch(i) {
		case 0:	/* Octet 3 */
			pri_message("%c Connected Number (len=%2d) [ Ext: %d  TON: %s (%d)  NPI: %s (%d)",
				prefix, len, ie->data[0] >> 7, ton2str((ie->data[0] >> 4) & 0x07), (ie->data[0] >> 4) & 0x07, npi2str(ie->data[0] & 0x0f), ie->data[0] & 0x0f);
			break;
		case 1: /* Octet 3a */
			pri_message("\n%c                             Ext: %d Presentation: %s (%d)",
				prefix, ie->data[1] >> 7, pri_pres2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f);
			break;
		}
	}
	while(!(ie->data[i++]& 0x80));
	q931_get_number(cnum, sizeof(cnum), ie->data + i, ie->len - i);
	pri_message(" '%s' ]\n", cnum);
}


static FUNC_RECV(receive_redirecting_number)
{        
        call->redirectingplan = ie->data[0] & 0x7f;
        call->redirectingpres = ie->data[1] & 0x7f;
        call->redirectingreason = ie->data[2] & 0x0f;

        q931_get_number(call->redirectingnum, sizeof(call->redirectingnum), ie->data + 3, len - 5);
	return 0;
}


static FUNC_DUMP(dump_redirecting_subaddr)
{
	char cnum[256];
	q931_get_number(cnum, sizeof(cnum), ie->data + 2, len - 4);
	pri_message("%c Redirecting Sub-Address (len=%2d) [ Ext: %d  Type: %s (%d) O: %d '%s' ]\n",
		prefix, len, ie->data[0] >> 7,
		subaddrtype2str((ie->data[0] & 0x70) >> 4), (ie->data[0] & 0x70) >> 4,
		(ie->data[0] & 0x08) >> 3, cnum);
}

static FUNC_RECV(receive_calling_party_subaddr)
{
	/* copy digits to call->callingsubaddr */
 	q931_get_number(call->callingsubaddr, sizeof(call->callingsubaddr), ie->data + 2, len - 4);
	return 0;
}

static FUNC_RECV(receive_called_party_number)
{
	/* copy digits to call->callednum */
 	q931_get_number(call->callednum, sizeof(call->callednum), ie->data + 1, len - 3);
	call->calledplan = ie->data[0] & 0x7f;
	return 0;
}

static FUNC_SEND(transmit_called_party_number)
{
	ie->data[0] = 0x80 | call->calledplan;
	if (strlen(call->callednum)) 
		memcpy(ie->data + 1, call->callednum, strlen(call->callednum));
	return strlen(call->callednum) + 3;
}

static FUNC_RECV(receive_calling_party_number)
{
        int extbit;
        
        call->callerplan = ie->data[0] & 0x7f;
        extbit = (ie->data[0] >> 7) & 0x01;

        if (extbit) {
	  q931_get_number(call->callernum, sizeof(call->callernum), ie->data + 1, len - 3);
	  call->callerpres = 0; /* PI presentation allowed
				   SI user-provided, not screened */        
        } else {
	  q931_get_number(call->callernum, sizeof(call->callernum), ie->data + 2, len - 4);
	  call->callerpres = ie->data[1] & 0x7f;
        }
	return 0;
}

static FUNC_SEND(transmit_calling_party_number)
{
	ie->data[0] = call->callerplan;
	ie->data[1] = 0x80 | call->callerpres;
	if (strlen(call->callednum)) 
		memcpy(ie->data + 2, call->callernum, strlen(call->callernum));
	return strlen(call->callernum) + 4;
}

static FUNC_DUMP(dump_user_user)
{
	int x;
	pri_message("%c User-User Information (len=%2d) [", prefix, len);
	for (x=0;x<ie->len;x++)
		pri_message(" %02x", ie->data[x] & 0x7f);
	pri_message(" ]\n");
}


static FUNC_RECV(receive_user_user)
{        
        call->useruserprotocoldisc = ie->data[0] & 0xff;
        if (call->useruserprotocoldisc == 4) /* IA5 */
          q931_get_number(call->useruserinfo, sizeof(call->useruserinfo), ie->data + 1, len - 3);
	return 0;
}

static char *prog2str(int prog)
{
	static struct msgtype progs[] = {
		{ Q931_PROG_CALL_NOT_E2E_ISDN, "Call is not end-to-end ISDN; further call progress information may be available inband." },
		{ Q931_PROG_CALLED_NOT_ISDN, "Called equipment is non-ISDN." },
		{ Q931_PROG_CALLER_NOT_ISDN, "Calling equipment is non-ISDN." },
		{ Q931_PROG_INBAND_AVAILABLE, "Inband information or appropriate pattern now available." },
		{ Q931_PROG_DELAY_AT_INTERF, "Delay in response at called Interface." },
		{ Q931_PROG_INTERWORKING_WITH_PUBLIC, "Interworking with a public network." },
		{ Q931_PROG_INTERWORKING_NO_RELEASE, "Interworking with a network unable to supply a release signal." },
		{ Q931_PROG_INTERWORKING_NO_RELEASE_PRE_ANSWER, "Interworking with a network unable to supply a release signal before answer." },
		{ Q931_PROG_INTERWORKING_NO_RELEASE_POST_ANSWER, "Interworking with a network unable to supply a release signal after answer." },
	};
	return code2str(prog, progs, sizeof(progs) / sizeof(progs[0]));
}

static char *coding2str(int cod)
{
	static struct msgtype cods[] = {
		{ CODE_CCITT, "CCITT (ITU) standard" },
		{ CODE_INTERNATIONAL, "Non-ITU international standard" }, 
		{ CODE_NATIONAL, "National standard" }, 
		{ CODE_NETWORK_SPECIFIC, "Network specific standard" },
	};
	return code2str(cod, cods, sizeof(cods) / sizeof(cods[0]));
}

static char *loc2str(int loc)
{
	static struct msgtype locs[] = {
		{ LOC_USER, "User" },
		{ LOC_PRIV_NET_LOCAL_USER, "Private network serving the local user" },
		{ LOC_PUB_NET_LOCAL_USER, "Public network serving the local user" },
		{ LOC_TRANSIT_NET, "Transit network" },
		{ LOC_PUB_NET_REMOTE_USER, "Public network serving the remote user" },
		{ LOC_PRIV_NET_REMOTE_USER, "Private network serving the remote user" },
		{ LOC_INTERNATIONAL_NETWORK, "International network" },
		{ LOC_NETWORK_BEYOND_INTERWORKING, "Network beyond the interworking point" },
	};
	return code2str(loc, locs, sizeof(locs) / sizeof(locs[0]));
}

static FUNC_DUMP(dump_progress_indicator)
{
	pri_message("%c Progress Indicator (len=%2d) [ Ext: %d  Coding: %s (%d) 0: %d   Location: %s (%d)\n",
		prefix, len, ie->data[0] >> 7, coding2str((ie->data[0] & 0x60) >> 5), (ie->data[0] & 0x60) >> 5,
		(ie->data[0] & 0x10) >> 4, loc2str(ie->data[0] & 0xf), ie->data[0] & 0xf);
	pri_message("%c                               Ext: %d  Progress Description: %s (%d) ]\n",
		prefix, ie->data[1] >> 7, prog2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f);
}

static FUNC_RECV(receive_display)
{
	unsigned char *data;
	data = ie->data;
	if (data[0] & 0x80) {
		/* Skip over character set */
		data++;
		len--;
	}
	q931_get_number(call->callername, sizeof(call->callername), data, len - 2);
	return 0;
}

static FUNC_SEND(transmit_display)
{
	int i;
	if ((pri->switchtype != PRI_SWITCH_NI1) && strlen(call->callername)) {
		i = 0;
		if(pri->switchtype != PRI_SWITCH_EUROISDN_E1) {
			ie->data[0] = 0xb1;
			++i;
		}
		memcpy(ie->data + i, call->callername, strlen(call->callername));
		return 2 + i + strlen(call->callername);
	}
	return 0;
}

static FUNC_RECV(receive_progress_indicator)
{
	call->progloc = ie->data[0] & 0xf;
	call->progcode = (ie->data[0] & 0x60) >> 5;
	call->progress = (ie->data[1] & 0x7f);
	return 0;
}

static FUNC_RECV(receive_facility)
{
	if (ie->len < 14) {
		pri_error("!! Facility message shorter than 14 bytes\n");
		return 0;
	}
	if (ie->data[13] + 14 == ie->len) {
		q931_get_number(call->callername, sizeof(call->callername) - 1, ie->data + 14, ie->len - 14);
	} 
	return 0;
}

static FUNC_SEND(transmit_progress_indicator)
{
	/* Can't send progress indicator on GR-303 -- EVER! */
	if (!pri->subchannel && (call->progress > 0)) {
		ie->data[0] = 0x80 | (call->progcode << 5)  | (call->progloc);
		ie->data[1] = 0x80 | (call->progress);
		return 4;
	} else {
		/* Leave off */
		return 0;
	}
}
static FUNC_SEND(transmit_call_state)
{
	if (call->ourcallstate > -1 ) {
		ie->data[0] = call->ourcallstate;
		return 3;
	}
	return 0;
}

static FUNC_RECV(receive_call_state)
{
	call->sugcallstate = ie->data[0] & 0x3f;
	return 0;
}

static char *callstate2str(int callstate)
{
	static struct msgtype callstates[] = {
		{ 0, "Null" },
		{ 1, "Call Initiated" },
		{ 2, "Overlap sending" },
		{ 3, "Outgoing call  Proceeding" },
		{ 4, "Call Delivered" },
		{ 6, "Call Present" },
		{ 7, "Call Received" },
		{ 8, "Connect Request" },
		{ 9, "Incoming Call Proceeding" },
		{ 10, "Active" },
		{ 11, "Disconnect Request" },
		{ 12, "Disconnect Indication" },
		{ 15, "Suspend Request" },
		{ 17, "Resume Request" },
		{ 19, "Release Request" },
		{ 22, "Call Abort" },
		{ 25, "Overlap Receiving" },
		{ 61, "Restart Request" },
		{ 62, "Restart" },
	};
	return code2str(callstate, callstates, sizeof(callstates) / sizeof(callstates[0]));
}

static FUNC_DUMP(dump_call_state)
{
	pri_message("%c Call State (len=%2d) [ Ext: %d  Coding: %s (%d) Call state: %s (%d)\n",
		prefix, len, ie->data[0] >> 7, coding2str((ie->data[0] & 0xC0) >> 6), (ie->data[0] & 0xC0) >> 6,
		callstate2str(ie->data[0] & 0x3f), ie->data[0] & 0x3f);
}

static FUNC_DUMP(dump_call_identity)
{
	int x;
	pri_message("%c Call Identity (len=%2d) [ ", prefix, len);
	for (x=0;x<ie->len;x++) 
		pri_message("0x%02X ", ie->data[x]);
	pri_message(" ]\n");
}

static FUNC_DUMP(dump_time_date)
{
	pri_message("%c Time Date (len=%2d) [ ", prefix, len);
	if (ie->len > 0)
		pri_message("%02d", ie->data[0]);
	if (ie->len > 1)
		pri_message("-%02d", ie->data[1]);
	if (ie->len > 2)
		pri_message("-%02d", ie->data[2]);
	if (ie->len > 3)
		pri_message(" %02d", ie->data[3]);
	if (ie->len > 4)
		pri_message(":%02d", ie->data[4]);
	if (ie->len > 5)
		pri_message(":%02d", ie->data[5]);
	pri_message(" ]\n");
}

static FUNC_DUMP(dump_display)
{
	int x, y;
	char *buf = malloc(len + 1);
	char tmp[80]="";
	if (buf) {
		x=y=0;
		if ((x < ie->len) && (ie->data[x] & 0x80)) {
			sprintf(tmp, "Charset: %02x ", ie->data[x] & 0x7f);
			++x;
		}
		for (y=x; x<ie->len; x++) 
			buf[x] = ie->data[x] & 0x7f;
		buf[x] = '\0';
		pri_message("%c Display (len=%2d) %s[ %s ]\n", prefix, ie->len, tmp, &buf[y]);
		free(buf);
	}
}

static void dump_ie_data(unsigned char *c, int len)
{
	char tmp[1024] = "";
	int x=0;
	int lastascii = 0;
	while(len) {
		if (((*c >= 'A') && (*c <= 'Z')) ||
		    ((*c >= 'a') && (*c <= 'z')) ||
		    ((*c >= '0') && (*c <= '9'))) {
			if (!lastascii) {
				if (strlen(tmp)) { 
					tmp[x++] = ',';
					tmp[x++] = ' ';
				}
				tmp[x++] = '\'';
			}
			tmp[x++] = *c;
			lastascii = 1;
		} else {
			if (lastascii) {
				tmp[x++] = '\'';
			}
			if (strlen(tmp)) { 
				tmp[x++] = ',';
				tmp[x++] = ' ';
			}
			sprintf (tmp + x, "0x%02x", *c);
			x += 4;
			lastascii = 0;
		}
		c++;
		len--;
	}
	if (lastascii)
		tmp[x++] = '\'';
	pri_message(tmp);
}

static FUNC_DUMP(dump_facility)
{
	pri_message("%c Facility (len=%2d, codeset=%d) [ ", prefix, len, Q931_IE_CODESET(full_ie));
	dump_ie_data(ie->data, ie->len);
	pri_message(" ]\n");
}

static FUNC_DUMP(dump_network_spec_fac)
{
       pri_message("%c Network-Specific Facilities (len=%2d) [ ", prefix, ie->len);
       if (ie->data[0] == 0x00) {
               pri_message (code2str(ie->data[1], facilities, sizeof(facilities) / sizeof(facilities[0])));
       }
       else
               dump_ie_data(ie->data, ie->len);
       pri_message(" ]\n");
}

static FUNC_RECV(receive_network_spec_fac)
{
       return 0;
}

static FUNC_SEND(transmit_network_spec_fac)
{
       if (pri->nsf != PRI_NSF_NONE) {
               ie->data[0] = 0x00;
               ie->data[1] = pri->nsf;
               return 4;
       } else {
               /* Leave off */
               return 0;
       }
}

char *pri_cause2str(int cause)
{
	return code2str(cause, causes, sizeof(causes) / sizeof(causes[0]));
}

static char *pri_causeclass2str(int cause)
{
	static struct msgtype causeclasses[] = {
		{ 0, "Normal Event" },
		{ 1, "Normal Event" },
		{ 2, "Network Congestion" },
		{ 3, "Service or Option not Available" },
		{ 4, "Service or Option not Implemented" },
		{ 5, "Invalid message" },
		{ 6, "Protocol Error" },
		{ 7, "Interworking" },
	};
	return code2str(cause, causeclasses, sizeof(causeclasses) / sizeof(causeclasses[0]));
}

static FUNC_DUMP(dump_cause)
{
	int x;
	pri_message("%c Cause (len=%2d) [ Ext: %d  Coding: %s (%d) 0: %d   Location: %s (%d)\n",
		prefix, len, ie->data[0] >> 7, coding2str((ie->data[0] & 0x60) >> 5), (ie->data[0] & 0x60) >> 5,
		(ie->data[0] & 0x10) >> 4, loc2str(ie->data[0] & 0xf), ie->data[0] & 0xf);
	pri_message("%c                  Ext: %d  Cause: %s (%d), class = %s (%d) ]\n",
		prefix, (ie->data[1] >> 7), pri_cause2str(ie->data[1] & 0x7f), ie->data[1] & 0x7f, 
			pri_causeclass2str((ie->data[1] & 0x7f) >> 4), (ie->data[1] & 0x7f) >> 4);
	for (x=2;x<ie->len;x++) 
		pri_message("%c              Cause data %d: %02x (%d)\n", prefix, x-1, ie->data[x], ie->data[x]);
}

static FUNC_RECV(receive_cause)
{
	call->causeloc = ie->data[0] & 0xf;
	call->causecode = (ie->data[0] & 0x60) >> 5;
	call->cause = (ie->data[1] & 0x7f);
	return 0;
}

static FUNC_SEND(transmit_cause)
{
	if (call->cause > 0) {
		ie->data[0] = 0x80 | (call->causecode << 5)  | (call->causeloc);
		ie->data[1] = 0x80 | (call->cause);
		return 4;
	} else {
		/* Leave off */
		return 0;
	}
}

static FUNC_DUMP(dump_sending_complete)
{
	pri_message("%c Sending Complete (len=%2d)\n", prefix, len);
}

static FUNC_RECV(receive_sending_complete)
{
	/* We've got a "Complete" message: Exect no further digits. */
	call->complete = 1; 
	return 0;
}

static FUNC_SEND(transmit_sending_complete)
{
	if ((pri->overlapdial && call->complete) || /* Explicit */
		(!pri->overlapdial && ((pri->switchtype == PRI_SWITCH_EUROISDN_E1) || 
		/* Implicit */   	   (pri->switchtype == PRI_SWITCH_EUROISDN_T1)))) {
		/* Include this single-byte IE */
		return 1;
	}
	return 0;
}

static char *notify2str(int info)
{
	/* ITU-T Q.763 */
	static struct msgtype notifies[] = {
		{ PRI_NOTIFY_USER_SUSPENDED, "User suspended" },
		{ PRI_NOTIFY_USER_RESUMED, "User resumed" },
		{ PRI_NOTIFY_BEARER_CHANGE, "Bearer service change (DSS1)" },
		{ PRI_NOTIFY_ASN1_COMPONENT, "ASN.1 encoded component (DSS1)" },
		{ PRI_NOTIFY_COMPLETION_DELAY, "Call completion delay" },
		{ PRI_NOTIFY_CONF_ESTABLISHED, "Conference established" },
		{ PRI_NOTIFY_CONF_DISCONNECTED, "Conference disconnected" },
		{ PRI_NOTIFY_CONF_PARTY_ADDED, "Other party added" },
		{ PRI_NOTIFY_CONF_ISOLATED, "Isolated" },
		{ PRI_NOTIFY_CONF_REATTACHED, "Reattached" },
		{ PRI_NOTIFY_CONF_OTHER_ISOLATED, "Other party isolated" },
		{ PRI_NOTIFY_CONF_OTHER_REATTACHED, "Other party reattached" },
		{ PRI_NOTIFY_CONF_OTHER_SPLIT, "Other party split" },
		{ PRI_NOTIFY_CONF_OTHER_DISCONNECTED, "Other party disconnected" },
		{ PRI_NOTIFY_CONF_FLOATING, "Conference floating" },
		{ PRI_NOTIFY_WAITING_CALL, "Call is waiting call" },
		{ PRI_NOTIFY_DIVERSION_ACTIVATED, "Diversion activated (DSS1)" },
		{ PRI_NOTIFY_TRANSFER_ALERTING, "Call transfer, alerting" },
		{ PRI_NOTIFY_TRANSFER_ACTIVE, "Call transfer, active" },
		{ PRI_NOTIFY_REMOTE_HOLD, "Remote hold" },
		{ PRI_NOTIFY_REMOTE_RETRIEVAL, "Remote retrieval" },
		{ PRI_NOTIFY_CALL_DIVERTING, "Call is diverting" },
	};
	return code2str(info, notifies, sizeof(notifies) / sizeof(notifies[0]));
}

static FUNC_DUMP(dump_notify)
{
	pri_message("%c Notification indicator (len=%2d): Ext: %d  %s (%d)\n", prefix, len, ie->data[0] >> 7, notify2str(ie->data[0] & 0x7f), ie->data[0] & 0x7f);
}

static FUNC_RECV(receive_notify)
{
	call->notify = ie->data[0] & 0x7F;
	return 0;
}

static FUNC_SEND(transmit_notify)
{
	if (call->notify >= 0) {
		ie->data[0] = 0x80 | call->notify;
		return 3;
	}
	return 0;
}

static FUNC_DUMP(dump_shift)
{
	pri_message("%c %sLocking Shift (len=%02d): Requested codeset %d\n", prefix, (full_ie & 8) ? "Non-" : "", len, full_ie & 7);
}

static char *lineinfo2str(int info)
{
	/* NAPNA ANI II digits */
	static struct msgtype lineinfo[] = {
		{  0, "Plain Old Telephone Service (POTS)" },
		{  1, "Multiparty line (more than 2)" },
		{  2, "ANI failure" },
		{  6, "Station Level Rating" },
		{  7, "Special Operator Handling Required" },
		{ 20, "Automatic Identified Outward Dialing (AIOD)" },
		{ 23, "Coing or Non-Coin" },
		{ 24, "Toll free translated to POTS originated for non-pay station" },
		{ 25, "Toll free translated to POTS originated from pay station" },
		{ 27, "Pay station with coin control signalling" },
		{ 29, "Prison/Inmate Service" },
		{ 30, "Intercept (blank)" },
		{ 31, "Intercept (trouble)" },
		{ 32, "Intercept (regular)" },
		{ 34, "Telco Operator Handled Call" },
		{ 52, "Outward Wide Area Telecommunications Service (OUTWATS)" },
		{ 60, "TRS call from unrestricted line" },
		{ 61, "Cellular/Wireless PCS (Type 1)" },
		{ 62, "Cellular/Wireless PCS (Type 2)" },
		{ 63, "Cellular/Wireless PCS (Roaming)" },
		{ 66, "TRS call from hotel/motel" },
		{ 67, "TRS call from restricted line" },
		{ 70, "Line connected to pay station" },
		{ 93, "Private virtual network call" },
	};
	return code2str(info, lineinfo, sizeof(lineinfo) / sizeof(lineinfo[0]));
}

static FUNC_DUMP(dump_line_information)
{
	pri_message("%c Originating Line Information (len=%02d): %s (%d)\n", prefix, len, lineinfo2str(ie->data[0]), ie->data[0]);
}

static FUNC_RECV(receive_line_information)
{
	return 0;
}

static FUNC_SEND(transmit_line_information)
{
#if 0	/* XXX Is this IE possible for 4ESS? XXX */
	if(pri->switchtype == PRI_SWITCH_ATT4ESS) {
		ie->data[0] = 0;
		return 3;
	}
#endif
	return 0;
}

struct ie ies[] = {
	/* Codeset 0 - Common */
	{ NATIONAL_CHANGE_STATUS, "Change Status" },
	{ Q931_LOCKING_SHIFT, "Locking Shift", dump_shift },
	{ Q931_BEARER_CAPABILITY, "Bearer Capability", dump_bearer_capability, receive_bearer_capability, transmit_bearer_capability },
	{ Q931_CAUSE, "Cause", dump_cause, receive_cause, transmit_cause },
	{ Q931_CALL_STATE, "Call State", dump_call_state, receive_call_state, transmit_call_state },
	{ Q931_CHANNEL_IDENT, "Channel Identification", dump_channel_id, receive_channel_id, transmit_channel_id },
	{ Q931_PROGRESS_INDICATOR, "Progress Indicator", dump_progress_indicator, receive_progress_indicator, transmit_progress_indicator },
	{ Q931_NETWORK_SPEC_FAC, "Network-Specific Facilities", dump_network_spec_fac, receive_network_spec_fac, transmit_network_spec_fac },
	{ Q931_INFORMATION_RATE, "Information Rate" },
	{ Q931_TRANSIT_DELAY, "End-to-End Transit Delay" },
	{ Q931_TRANS_DELAY_SELECT, "Transmit Delay Selection and Indication" },
	{ Q931_BINARY_PARAMETERS, "Packet-layer Binary Parameters" },
	{ Q931_WINDOW_SIZE, "Packet-layer Window Size" },
	{ Q931_CLOSED_USER_GROUP, "Closed User Group" },
	{ Q931_REVERSE_CHARGE_INDIC, "Reverse Charging Indication" },
	{ Q931_CALLING_PARTY_NUMBER, "Calling Party Number", dump_calling_party_number, receive_calling_party_number, transmit_calling_party_number },
	{ Q931_CALLING_PARTY_SUBADDR, "Calling Party Subaddress", dump_calling_party_subaddr, receive_calling_party_subaddr },
	{ Q931_CALLED_PARTY_NUMBER, "Called Party Number", dump_called_party_number, receive_called_party_number, transmit_called_party_number },
	{ Q931_CALLED_PARTY_SUBADDR, "Called Party Subaddress", dump_called_party_subaddr },
	{ Q931_REDIRECTING_NUMBER, "Redirecting Number", dump_redirecting_number, receive_redirecting_number },
	{ Q931_REDIRECTING_SUBADDR, "Redirecting Subaddress", dump_redirecting_subaddr },
	{ Q931_TRANSIT_NET_SELECT, "Transit Network Selection" },
	{ Q931_RESTART_INDICATOR, "Restart Indicator", dump_restart_indicator, receive_restart_indicator, transmit_restart_indicator },
	{ Q931_LOW_LAYER_COMPAT, "Low-layer Compatibility" },
	{ Q931_HIGH_LAYER_COMPAT, "High-layer Compatibility" },
	{ Q931_PACKET_SIZE, "Packet Size" },
	{ Q931_IE_FACILITY, "Facility" , dump_facility, receive_facility },
	{ Q931_IE_REDIRECTION_NUMBER, "Redirection Number" },
	{ Q931_IE_REDIRECTION_SUBADDR, "Redirection Subaddress" },
	{ Q931_IE_FEATURE_ACTIVATE, "Feature Activation" },
	{ Q931_IE_INFO_REQUEST, "Feature Request" },
	{ Q931_IE_FEATURE_IND, "Feature Indication" },
	{ Q931_IE_SEGMENTED_MSG, "Segmented Message" },
	{ Q931_IE_CALL_IDENTITY, "Call Identity", dump_call_identity },
	{ Q931_IE_ENDPOINT_ID, "Endpoint Identification" },
	{ Q931_IE_NOTIFY_IND, "Notification Indicator", dump_notify, receive_notify, transmit_notify },
	{ Q931_DISPLAY, "Display", dump_display, receive_display, transmit_display },
	{ Q931_IE_TIME_DATE, "Date/Time", dump_time_date },
	{ Q931_IE_KEYPAD_FACILITY, "Keypad Facility" },
	{ Q931_IE_SIGNAL, "Signal" },
	{ Q931_IE_SWITCHHOOK, "Switch-hook" },
	{ Q931_IE_USER_USER, "User-User", dump_user_user, receive_user_user },
	{ Q931_IE_ESCAPE_FOR_EXT, "Escape for Extension" },
	{ Q931_IE_CALL_STATUS, "Call Status" },
	{ Q931_IE_CHANGE_STATUS, "Change Status" },
	{ Q931_IE_CONNECTED_ADDR, "Connected Number", dump_connected_number },
	{ Q931_IE_CONNECTED_NUM, "Connected Number", dump_connected_number },
	{ Q931_IE_ORIGINAL_CALLED_NUMBER, "Original Called Number" },
	{ Q931_IE_USER_USER_FACILITY, "User-User Facility" },
	{ Q931_IE_UPDATE, "Update" },
	{ Q931_SENDING_COMPLETE, "Sending Complete", dump_sending_complete, receive_sending_complete, transmit_sending_complete },
	/* Codeset 6 - Network specific */
	{ Q931_IE_FACILITY | Q931_CODESET(6), "Facility", dump_facility, receive_facility },
	{ Q931_IE_ORIGINATING_LINE_INFO, "Originating Line Information", dump_line_information, receive_line_information, transmit_line_information },
	/* Codeset 7 */
};

static char *ie2str(int ie) 
{
	unsigned int x;

	/* Special handling for Locking/Non-Locking Shifts */
	switch (ie & 0xf8) {
	case Q931_LOCKING_SHIFT:
		switch (ie & 7) {
		case 0:
			return "!! INVALID Locking Shift To Codeset 0";
		case 1:
			return "Locking Shift To Codeset 1";
		case 2:
			return "Locking Shift To Codeset 2";
		case 3:
			return "Locking Shift To Codeset 3";
		case 4:
			return "Locking Shift To Codeset 4";
		case 5:
			return "Locking Shift To Codeset 5";
		case 6:
			return "Locking Shift To Codeset 6";
		case 7:
			return "Locking Shift To Codeset 7";
		}
	case Q931_NON_LOCKING_SHIFT:
		switch (ie & 7) {
		case 0:
			return "Non-Locking Shift To Codeset 0";
		case 1:
			return "Non-Locking Shift To Codeset 1";
		case 2:
			return "Non-Locking Shift To Codeset 2";
		case 3:
			return "Non-Locking Shift To Codeset 3";
		case 4:
			return "Non-Locking Shift To Codeset 4";
		case 5:
			return "Non-Locking Shift To Codeset 5";
		case 6:
			return "Non-Locking Shift To Codeset 6";
		case 7:
			return "Non-Locking Shift To Codeset 7";
		}
	default:
		for (x=0;x<sizeof(ies) / sizeof(ies[0]); x++) 
			if (ie == ies[x].ie)
				return ies[x].name;
		return "Unknown Information Element";
	}
}	

static inline unsigned int ielen(q931_ie *ie)
{
	if ((ie->ie & 0x80) != 0)
		return 1;
	else
		return 2 + ie->len;
}

static char *msg2str(int msg)
{
	unsigned int x;
	for (x=0;x<sizeof(msgs) / sizeof(msgs[0]); x++) 
		if (msgs[x].msgnum == msg)
			return msgs[x].name;
	return "Unknown Message Type";
}

static inline int q931_cr(q931_h *h)
{
	int cr = 0;
	int x;
	if (h->crlen > 3) {
		pri_error("Call Reference Length Too long: %d\n", h->crlen);
		return -1;
	}
	switch (h->crlen) {
		case 2: 
			for (x=0;x<h->crlen;x++) {
				cr <<= 8;
				cr |= h->crv[x];
			}
			break;
		case 1:
			cr = h->crv[0];
			if (cr & 0x80) {
				cr &= ~0x80;
				cr |= 0x8000;
			}
			break;
		default:
			pri_error("Call Reference Length not supported: %d\n", h->crlen);
	}
	return cr;
}

static inline void q931_dumpie(int codeset, q931_ie *ie, char prefix)
{
	unsigned int x;
	int full_ie = Q931_FULL_IE(codeset, ie->ie);
	int base_ie;

	pri_message("%c [", prefix);
	pri_message("%02x", ie->ie);
	if (!(ie->ie & 0x80)) {
		pri_message(" %02x", ielen(ie)-2);
		for (x = 0; x + 2 < ielen(ie); ++x)
			pri_message(" %02x", ie->data[x]);
	}
	pri_message("]\n");

	/* Special treatment for shifts */
	if((full_ie & 0xf0) == Q931_LOCKING_SHIFT)
		full_ie &= 0xff;

	base_ie = (((full_ie & ~0x7f) == Q931_FULL_IE(0, 0x80)) && ((full_ie & 0x70) != 0x20)) ? full_ie & ~0x0f : full_ie;

	for (x=0;x<sizeof(ies) / sizeof(ies[0]); x++) 
		if (ies[x].ie == base_ie) {
			if (ies[x].dump)
				ies[x].dump(full_ie, ie, ielen(ie), prefix);
			else
				pri_message("%c IE: %s (len = %d)\n", prefix, ies[x].name, ielen(ie));
			return;
		}
	
	pri_error("!! %c Unknown IE %d (len = %d)\n", prefix, base_ie, ielen(ie));
}

static q931_call *q931_getcall(struct pri *pri, int cr)
{
	q931_call *cur, *prev;
	cur = *pri->callpool;
	prev = NULL;
	while(cur) {
		if (cur->cr == cr)
			return cur;
		prev = cur;
		cur = cur->next;
	}
	/* No call exists, make a new one */
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("-- Making new call for cr %d\n", cr);
	cur = malloc(sizeof(struct q931_call));
	if (cur) {
		call_init(cur);
		/* Call reference */
		cur->cr = cr;
		cur->pri = pri;
		/* Append to end of list */
		if (prev)
			prev->next = cur;
		else
			*pri->callpool = cur;
	}
	return cur;
}

q931_call *q931_new_call(struct pri *pri)
{
	q931_call *cur;
	do {
		cur = *pri->callpool;
		pri->cref++;
		if (pri->cref > 32767)
			pri->cref = 1;
		while(cur) {
			if (cur->cr == (0x8000 | pri->cref))
				break;
			cur = cur->next;
		}
	} while(cur);
	return q931_getcall(pri, pri->cref | 0x8000);
}

static void q931_destroy(struct pri *pri, int cr, q931_call *c)
{
	q931_call *cur, *prev;
	prev = NULL;
	cur = *pri->callpool;
	while(cur) {
		if ((c && (cur == c)) || (!c && (cur->cr == cr))) {
			if (prev)
				prev->next = cur->next;
			else
				*pri->callpool = cur->next;
			if (pri->debug & PRI_DEBUG_Q931_STATE)
				pri_message("NEW_HANGUP DEBUG: Destroying the call, ourstate %s, peerstate %s\n",callstate2str(cur->ourcallstate),callstate2str(cur->peercallstate));
			if (cur->retranstimer)
				pri_schedule_del(pri, cur->retranstimer);
			free(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
	pri_error("Can't destroy call %d!\n", cr);
}

static void q931_destroycall(struct pri *pri, int cr)
{
	return q931_destroy(pri, cr, NULL);
}


void __q931_destroycall(struct pri *pri, q931_call *c) 
{
	if (pri && c)
		q931_destroy(pri,0, c);
	return;
}

static int add_ie(struct pri *pri, q931_call *call, int msgtype, int ie, q931_ie *iet, int maxlen, int *codeset)
{
	unsigned int x;
	int res;
	int have_shift;
	for (x=0;x<sizeof(ies) / sizeof(ies[0]);x++) {
		if (ies[x].ie == ie) {
			/* This is our baby */
			if (ies[x].transmit) {
				/* Prepend with CODE SHIFT IE if required */
				if (*codeset != Q931_IE_CODESET(ies[x].ie)) {
					/* Locking shift to codeset 0 isn't possible */
					iet->ie = Q931_IE_CODESET(ies[x].ie) | (Q931_IE_CODESET(ies[x].ie) ? Q931_LOCKING_SHIFT : Q931_NON_LOCKING_SHIFT);
					have_shift = 1;
					iet = (q931_ie *)((char *)iet + 1);
					maxlen--;
				}
				else
					have_shift = 0;
				iet->ie = ie;
				res = ies[x].transmit(ie, pri, call, msgtype, iet, maxlen);
				/* Error if res < 0 or ignored if res == 0 */
				if (res <= 0)
					return res;
				if ((iet->ie & 0x80) == 0) /* Multibyte IE */
					iet->len = res - 2;
				if (have_shift) {
					if (Q931_IE_CODESET(ies[x].ie))
						*codeset = Q931_IE_CODESET(ies[x].ie);
					return res + 1; /* Shift is single-byte IE */
				}
				return res;
			} else {
				pri_error("!! Don't know how to add an IE %s (%d)\n", ie2str(ie), ie);
				return -1;
			}
		}
	}
	pri_error("!! Unknown IE %d (%s)\n", ie, ie2str(ie));
	return -1;
}

static char *disc2str(int disc)
{
	static struct msgtype discs[] = {
		{ Q931_PROTOCOL_DISCRIMINATOR, "Q.931" },
		{ GR303_PROTOCOL_DISCRIMINATOR, "GR-303" },
		{ 0x3, "AT&T Maintenance" },
		{ 0x43, "New AT&T Maintenance" },
	};
	return code2str(disc, discs, sizeof(discs) / sizeof(discs[0]));
}

void q931_dump(q931_h *h, int len, int txrx)
{
	q931_mh *mh;
	char c;
	int x=0, r;
	int cur_codeset;
	int codeset;
	c = txrx ? '>' : '<';
	pri_message("%c Protocol Discriminator: %s (%d)  len=%d\n", c, disc2str(h->pd), h->pd, len);
	pri_message("%c Call Ref: len=%2d (reference %d/0x%X) (%s)\n", c, h->crlen, q931_cr(h), q931_cr(h), (h->crv[0] & 0x80) ? "Terminator" : "Originator");
	/* Message header begins at the end of the call reference number */
	mh = (q931_mh *)(h->contents + h->crlen);
	pri_message("%c Message type: %s (%d)\n", c, msg2str(mh->msg), mh->msg);
	/* Drop length of header, including call reference */
	len -= (h->crlen + 3);
	codeset = cur_codeset = 0;
	while(x < len) {
		r = ielen((q931_ie *)(mh->data + x));
		q931_dumpie(cur_codeset, (q931_ie *)(mh->data + x), c);
		switch (mh->data[x] & 0xf8) {
		case Q931_LOCKING_SHIFT:
			if ((mh->data[x] & 7) > 0)
				codeset = cur_codeset = mh->data[x] & 7;
			break;
		case Q931_NON_LOCKING_SHIFT:
			cur_codeset = mh->data[x] & 7;
			break;
		default:
			/* Reset temporary codeset change */
			cur_codeset = codeset;
		}
		x += r;
	}
	if (x > len) 
		pri_error("XXX Message longer than it should be?? XXX\n");
}

static int q931_handle_ie(int codeset, struct pri *pri, q931_call *c, int msg, q931_ie *ie)
{
	unsigned int x;
	int full_ie = Q931_FULL_IE(codeset, ie->ie);
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("-- Processing IE %d (cs%d, %s)\n", ie->ie, codeset, ie2str(full_ie));
	for (x=0;x<sizeof(ies) / sizeof(ies[0]);x++) {
		if (full_ie == ies[x].ie) {
			if (ies[x].receive)
				return ies[x].receive(full_ie, pri, c, msg, ie, ielen(ie));
			else {
				if (pri->debug & PRI_DEBUG_Q931_ANOMALY)
					pri_error("!! No handler for IE %d (cs%d, %s)\n", ie->ie, codeset, ie2str(full_ie));
				return -1;
			}
		}
	}
	pri_message("!! Unknown IE %d (cs%d, %s)\n", ie->ie, codeset, ie2str(full_ie));
	return -1;
}

static void init_header(struct pri *pri, q931_call *call, char *buf, q931_h **hb, q931_mh **mhb, int *len)
{
	/* Returns header and message header and modifies length in place */
	q931_h *h = (q931_h *)buf;
	q931_mh * mh = (q931_mh *)(h->contents + 2);
	h->pd = pri->protodisc;
	h->x0 = 0;		/* Reserved 0 */
	h->crlen = 2;	/* Two bytes of Call Reference.  Invert the top bit to make it from our sense */
	if (call->cr || call->forceinvert) {
		h->crv[0] = ((call->cr ^ 0x8000) & 0xff00) >> 8;
		h->crv[1] = (call->cr & 0xff);
	} else {
		/* Unless of course this has no call reference */
		h->crv[0] = 0;
		h->crv[1] = 0;
	}
	if (pri->subchannel) {
		/* On GR-303, top bit is always 0 */
		h->crv[0] &= 0x7f;
	}
	mh->f = 0;
	*hb = h;
	*mhb = mh;
	*len -= 5;
	
}

static int q931_xmit(struct pri *pri, q931_h *h, int len, int cr)
{
	q921_transmit_iframe(pri, h, len, cr);
	/* The transmit operation might dump the q921 header, so logging the q931
	   message body after the transmit puts the sections of the message in the
	   right order in the log */
	if (pri->debug & PRI_DEBUG_Q931_DUMP)
		q931_dump(h, len, 1);
#ifdef LIBPRI_COUNTERS
	pri->q931_txcount++;
#endif
	return 0;
}

static int send_message(struct pri *pri, q931_call *c, int msgtype, int ies[])
{
	unsigned char buf[1024];
	q931_h *h;
	q931_mh *mh;
	int len;
	int res;
	int offset=0;
	int x;
	int codeset;
	memset(buf, 0, sizeof(buf));
	len = sizeof(buf);
	init_header(pri, c, buf, &h, &mh, &len);
	mh->msg = msgtype;
	x=0;
	codeset = 0;
	while(ies[x] > -1) {
		res = add_ie(pri, c, mh->msg, ies[x], (q931_ie *)(mh->data + offset), len, &codeset);
		if (res < 0) {
			pri_error("!! Unable to add IE '%s'\n", ie2str(ies[x]));
			return -1;
		}
		offset += res;
		len -= res;
		x++;
	}
	/* Invert the logic */
	len = sizeof(buf) - len;
	q931_xmit(pri, h, len, 1);
	c->acked = 1;
	return 0;
}

static int status_ies[] = { Q931_CAUSE, Q931_CALL_STATE, -1 };

static int q931_status(struct pri *pri, q931_call *c, int cause)
{
	q931_call *cur = NULL;
	if (!cause)
		cause = PRI_CAUSE_RESPONSE_TO_STATUS_ENQUIRY;
	if (c->cr > -1)
		cur = *pri->callpool;
	while(cur) {
		if (cur->cr == c->cr) {
			cur->cause=cause;
			cur->causecode = CODE_CCITT;
			cur->causeloc = LOC_USER;
			break;
		}
		cur = cur->next;
	}
	if (!cur) {
		pri_message("YYY Here we get reset YYY\n");
		/* something went wrong, respond with "no such call" */
		c->ourcallstate = Q931_CALL_STATE_NULL;
		c->peercallstate = Q931_CALL_STATE_NULL;
		cur=c;
	}
	return send_message(pri, cur, Q931_STATUS, status_ies);
}

static int information_ies[] = { Q931_CALLED_PARTY_NUMBER, -1 };

int q931_information(struct pri *pri, q931_call *c, char digit)
{
	c->callednum[0]=digit;
	c->callednum[1]='\0';
	return send_message(pri, c, Q931_INFORMATION, information_ies);
}

static int restart_ack_ies[] = { Q931_CHANNEL_IDENT, Q931_RESTART_INDICATOR, -1 };

static int restart_ack(struct pri *pri, q931_call *c)
{
	c->ourcallstate = Q931_CALL_STATE_NULL;
	c->peercallstate = Q931_CALL_STATE_NULL;
	return send_message(pri, c, Q931_RESTART_ACKNOWLEDGE, restart_ack_ies);
}

static int notify_ies[] = { Q931_IE_NOTIFY_IND, -1 };

int q931_notify(struct pri *pri, q931_call *c, int channel, int info)
{
	if (info >= 0)
		c->notify = info & 0x7F;
	else
		c->notify = -1;
	return send_message(pri, c, Q931_NOTIFY, notify_ies);
}

#ifdef ALERTING_NO_PROGRESS
static int call_progress_ies[] = { -1 };
#else
static int call_progress_ies[] = { Q931_PROGRESS_INDICATOR, -1 };
#endif

int q931_call_progress(struct pri *pri, q931_call *c, int channel, int info)
{
	if (channel) { 
		c->ds1no = (channel & 0xff00) >> 8;
		channel &= 0xff;
		c->channelno = channel;		
	}
	if (info) {
		c->progloc = LOC_PRIV_NET_LOCAL_USER;
		c->progcode = CODE_CCITT;
		c->progress = Q931_PROG_INBAND_AVAILABLE;
	} else
		c->progress = -1;
	c->alive = 1;
	return send_message(pri, c, Q931_PROGRESS, call_progress_ies);
}

#ifdef ALERTING_NO_PROGRESS
static int call_proceeding_ies[] = { Q931_CHANNEL_IDENT, -1 };
#else
static int call_proceeding_ies[] = { Q931_CHANNEL_IDENT, Q931_PROGRESS_INDICATOR, -1 };
#endif

int q931_call_proceeding(struct pri *pri, q931_call *c, int channel, int info)
{
	if (channel) { 
		c->ds1no = (channel & 0xff00) >> 8;
		channel &= 0xff;
		c->channelno = channel;		
	}
	c->chanflags &= ~FLAG_PREFERRED;
	c->chanflags |= FLAG_EXCLUSIVE;
	c->ourcallstate = Q931_CALL_STATE_INCOMING_CALL_PROCEEDING;
	c->peercallstate = Q931_CALL_STATE_OUTGOING_CALL_PROCEEDING;
	if (info) {
		c->progloc = LOC_PRIV_NET_LOCAL_USER;
		c->progcode = CODE_CCITT;
		c->progress = Q931_PROG_INBAND_AVAILABLE;
	} else
		c->progress = -1;
	c->proc = 1;
	c->alive = 1;
	return send_message(pri, c, Q931_CALL_PROCEEDING, call_proceeding_ies);
}
#ifndef ALERTING_NO_PROGRESS
static int alerting_ies[] = { Q931_PROGRESS_INDICATOR, -1 };
#else
static int alerting_ies[] = { -1 };
#endif

int q931_alerting(struct pri *pri, q931_call *c, int channel, int info)
{
	if (!c->proc) 
		q931_call_proceeding(pri, c, channel, 0);
	if (info) {
		c->progloc = LOC_PRIV_NET_LOCAL_USER;
		c->progcode = CODE_CCITT;
		c->progress = Q931_PROG_INBAND_AVAILABLE;
	} else
		c->progress = -1;
	c->ourcallstate = Q931_CALL_STATE_CALL_RECEIVED;
	c->peercallstate = Q931_CALL_STATE_CALL_DELIVERED;
	c->alive = 1;
	return send_message(pri, c, Q931_ALERTING, alerting_ies);
}

static int connect_ies[] = {  Q931_CHANNEL_IDENT, Q931_PROGRESS_INDICATOR, -1 };
 
int q931_setup_ack(struct pri *pri, q931_call *c, int channel, int nonisdn)
{
	if (channel) { 
		c->ds1no = (channel & 0xff00) >> 8;
		channel &= 0xff;
		c->channelno = channel;		
	}
	c->chanflags &= ~FLAG_PREFERRED;
	c->chanflags |= FLAG_EXCLUSIVE;
	if (nonisdn && (pri->switchtype != PRI_SWITCH_DMS100)) {
		c->progloc  = LOC_PRIV_NET_LOCAL_USER;
		c->progcode = CODE_CCITT;
		c->progress = Q931_PROG_CALLED_NOT_ISDN;
	} else
		c->progress = -1;
	c->ourcallstate = Q931_CALL_STATE_OVERLAP_RECEIVING;
	c->peercallstate = Q931_CALL_STATE_OVERLAP_SENDING;
	c->alive = 1;
	return send_message(pri, c, Q931_SETUP_ACKNOWLEDGE, connect_ies);
}

static void pri_connect_timeout(void *data)
{
	struct q931_call *c = data;
	struct pri *pri = c->pri;
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("Timed out looking for connect acknowledge\n");
	q931_disconnect(pri, c, PRI_CAUSE_NORMAL_CLEARING);
	
}

static void pri_release_timeout(void *data)
{
	struct q931_call *c = data;
	struct pri *pri = c->pri;
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("Timed out looking for release complete\n");
	c->t308_timedout++;
	c->alive = 1;
	q931_release(pri, c, PRI_CAUSE_NORMAL_CLEARING);
}

static void pri_release_finaltimeout(void *data)
{
	struct q931_call *c = data;
	struct pri *pri = c->pri;
	c->alive = 1;
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("Final time-out looking for release complete\n");
	c->t308_timedout++;
	c->ourcallstate = Q931_CALL_STATE_NULL;
	c->peercallstate = Q931_CALL_STATE_NULL;
	pri->schedev = 1;
	pri->ev.e = PRI_EVENT_HANGUP_ACK;
	pri->ev.hangup.channel = c->channelno;
	pri->ev.hangup.cref = c->cr;
	pri->ev.hangup.cause = c->cause;
	pri->ev.hangup.call = c;
	q931_hangup(pri, c, c->cause);
}

static void pri_disconnect_timeout(void *data)
{
	struct q931_call *c = data;
	struct pri *pri = c->pri;
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("Timed out looking for release\n");
	c->alive = 1;
	q931_release(pri, c, PRI_CAUSE_NORMAL_CLEARING);
}

int q931_connect(struct pri *pri, q931_call *c, int channel, int nonisdn)
{
	if (channel) { 
		c->ds1no = (channel & 0xff00) >> 8;
		channel &= 0xff;
		c->channelno = channel;		
	}
	c->chanflags &= ~FLAG_PREFERRED;
	c->chanflags |= FLAG_EXCLUSIVE;
	if (nonisdn && (pri->switchtype != PRI_SWITCH_DMS100)) {
		c->progloc  = LOC_PRIV_NET_LOCAL_USER;
		c->progcode = CODE_CCITT;
		c->progress = Q931_PROG_CALLED_NOT_ISDN;
	} else
		c->progress = -1;
	c->ourcallstate = Q931_CALL_STATE_CONNECT_REQUEST;
	c->peercallstate = Q931_CALL_STATE_ACTIVE;
	c->alive = 1;
	/* Setup timer */
	if (c->retranstimer)
		pri_schedule_del(pri, c->retranstimer);
	if ((pri->localtype == PRI_CPE) && (!pri->subchannel))
		c->retranstimer = pri_schedule_event(pri, T_313, pri_connect_timeout, c);
	return send_message(pri, c, Q931_CONNECT, connect_ies);
}

static int release_ies[] = { Q931_CAUSE, -1 };

int q931_release(struct pri *pri, q931_call *c, int cause)
{
	c->ourcallstate = Q931_CALL_STATE_RELEASE_REQUEST;
	/* c->peercallstate stays the same */
	if (c->alive) {
		c->alive = 0;
		c->cause = cause;
		c->causecode = CODE_CCITT;
		c->causeloc = LOC_PRIV_NET_LOCAL_USER;
		if (c->acked) {
			if (c->retranstimer)
				pri_schedule_del(pri, c->retranstimer);
			if (!c->t308_timedout) {
				c->retranstimer = pri_schedule_event(pri, T_308, pri_release_timeout, c);
			} else {
				c->retranstimer = pri_schedule_event(pri, T_308, pri_release_finaltimeout, c);
			}
			return send_message(pri, c, Q931_RELEASE, release_ies);
		} else
			return send_message(pri, c, Q931_RELEASE_COMPLETE, release_ies); /* Yes, release_ies, not release_complete_ies */
	} else
		return 0;
}

static int restart_ies[] = { Q931_CHANNEL_IDENT, Q931_RESTART_INDICATOR, -1 };

int q931_restart(struct pri *pri, int channel)
{
	struct q931_call *c;
	c = q931_getcall(pri, 0 | 0x8000);
	if (!c)
		return -1;
	if (!channel)
		return -1;
	c->ri = 0;
	c->ds1no = (channel & 0xff00) >> 8;
	channel &= 0xff;
	c->channelno = channel;		
	c->chanflags &= ~FLAG_PREFERRED;
	c->chanflags |= FLAG_EXCLUSIVE;
	c->ourcallstate = Q931_CALL_STATE_RESTART;
	c->peercallstate = Q931_CALL_STATE_RESTART_REQUEST;
	return send_message(pri, c, Q931_RESTART, restart_ies);
}

static int disconnect_ies[] = { Q931_CAUSE, -1 };

int q931_disconnect(struct pri *pri, q931_call *c, int cause)
{
	c->ourcallstate = Q931_CALL_STATE_DISCONNECT_REQUEST;
	c->peercallstate = Q931_CALL_STATE_DISCONNECT_INDICATION;
	if (c->alive) {
		c->alive = 0;
		c->cause = cause;
		c->causecode = CODE_CCITT;
		c->causeloc = LOC_PRIV_NET_LOCAL_USER;
		c->sendhangupack = 1;
		if (c->retranstimer)
			pri_schedule_del(pri, c->retranstimer);
		c->retranstimer = pri_schedule_event(pri, T_305, pri_disconnect_timeout, c);
		return send_message(pri, c, Q931_DISCONNECT, disconnect_ies);
	} else
		return 0;
}

static int setup_ies[] = { Q931_BEARER_CAPABILITY, Q931_CHANNEL_IDENT, Q931_PROGRESS_INDICATOR, Q931_NETWORK_SPEC_FAC, Q931_DISPLAY,
	Q931_CALLING_PARTY_NUMBER, Q931_CALLED_PARTY_NUMBER, Q931_SENDING_COMPLETE, Q931_IE_ORIGINATING_LINE_INFO, -1 };

static int gr303_setup_ies[] =  { Q931_BEARER_CAPABILITY, Q931_CHANNEL_IDENT, -1 };

int q931_setup(struct pri *pri, q931_call *c, struct pri_sr *req)
{
	int res;
	
	
	c->transcapability = req->transmode;
	c->transmoderate = TRANS_MODE_64_CIRCUIT;
	if (!req->userl1)
		req->userl1 = PRI_LAYER_1_ULAW;
	c->userl1 = req->userl1;
	c->ds1no = (req->channel & 0xff00) >> 8;
	req->channel &= 0xff;
	if ((pri->localtype == PRI_CPE) && pri->subchannel) {
		req->channel = 0;
		req->exclusive = 0;
	}
		
	c->channelno = req->channel;		
	c->slotmap = -1;
	c->nonisdn = req->nonisdn;
	c->newcall = 0;		
	c->complete = req->numcomplete; 
	if (req->exclusive) 
		c->chanflags = FLAG_EXCLUSIVE;
	else if (c->channelno)
		c->chanflags = FLAG_PREFERRED;
	if (req->caller) {
		strncpy(c->callernum, req->caller, sizeof(c->callernum) - 1);
		c->callerplan = req->callerplan;
		if (req->callername)
			strncpy(c->callername, req->callername, sizeof(c->callername) - 1);
		else
			strcpy(c->callername, "");
		if ((pri->switchtype == PRI_SWITCH_DMS100) ||
		    (pri->switchtype == PRI_SWITCH_ATT4ESS)) {
			/* Doesn't like certain presentation types */
			if (!(req->callerpres & 0x7c))
				req->callerpres = PRES_ALLOWED_NETWORK_NUMBER;
		}
		c->callerpres = req->callerpres;
	} else {
		strcpy(c->callernum, "");
		strcpy(c->callername, "");
		c->callerplan = PRI_UNKNOWN;
		c->callerpres = PRES_NUMBER_NOT_AVAILABLE;
	}
	if (req->called) {
		strncpy(c->callednum, req->called, sizeof(c->callednum) - 1);
		c->calledplan = req->calledplan;
	} else
		return -1;

	if (req->nonisdn && (pri->switchtype == PRI_SWITCH_NI2))
		c->progress = Q931_PROG_CALLER_NOT_ISDN;
	else
		c->progress = -1;
	if (pri->subchannel)
		res = send_message(pri, c, Q931_SETUP, gr303_setup_ies);
	else
		res = send_message(pri, c, Q931_SETUP, setup_ies);
	if (!res) {
		c->alive = 1;
		/* make sure we call PRI_EVENT_HANGUP_ACK once we send/receive RELEASE_COMPLETE */
		c->sendhangupack = 1;
		c->ourcallstate = Q931_CALL_STATE_CALL_INITIATED;
		c->peercallstate = Q931_CALL_STATE_OVERLAP_SENDING;	
	}
	return res;
	
}

static int release_complete_ies[] = { -1 };

static int q931_release_complete(struct pri *pri, q931_call *c, int cause)
{
	int res = 0;
	c->ourcallstate = Q931_CALL_STATE_NULL;
	c->peercallstate = Q931_CALL_STATE_NULL;
	if (cause > -1) {
		c->cause = cause;
		c->causecode = CODE_CCITT;
		c->causeloc = LOC_PRIV_NET_LOCAL_USER;
		/* release_ies has CAUSE in it */
		res = send_message(pri, c, Q931_RELEASE_COMPLETE, release_ies);
	} else
		res = send_message(pri, c, Q931_RELEASE_COMPLETE, release_complete_ies);
	c->alive = 0;
	/* release the structure */
	res += q931_hangup(pri,c,cause);
	return res;
}

static int connect_acknowledge_ies[] = { -1 };

static int gr303_connect_acknowledge_ies[] = { Q931_CHANNEL_IDENT, -1 };

static int q931_connect_acknowledge(struct pri *pri, q931_call *c)
{
	if (pri->subchannel) {
		if (pri->localtype == PRI_CPE)
			return send_message(pri, c, Q931_CONNECT_ACKNOWLEDGE, gr303_connect_acknowledge_ies);
	} else
		return send_message(pri, c, Q931_CONNECT_ACKNOWLEDGE, connect_acknowledge_ies);
	return 0;
}

int q931_hangup(struct pri *pri, q931_call *c, int cause)
{
	int disconnect = 1;
	int release_compl = 0;
	if (pri->debug & PRI_DEBUG_Q931_STATE)
		pri_message("NEW_HANGUP DEBUG: Calling q931_hangup, ourstate %s, peerstate %s\n",callstate2str(c->ourcallstate),callstate2str(c->peercallstate));
	if (!pri || !c)
		return -1;
	/* If mandatory IE was missing, insist upon that cause code */
	if (c->cause == PRI_CAUSE_MANDATORY_IE_MISSING)
		cause = c->cause;
	if (cause == 34 || cause == 44 || cause == 82 || cause == 1 || cause == 81) {
		/* We'll send RELEASE_COMPLETE with these causes */
		disconnect = 0;
		release_compl = 1;
	}
	if (cause == 6 || cause == 7 || cause == 26) {
		/* We'll send RELEASE with these causes */
		disconnect = 0;
	}
	/* All other causes we send with DISCONNECT */
	switch(c->ourcallstate) {
	case Q931_CALL_STATE_NULL:
		if (c->peercallstate == Q931_CALL_STATE_NULL)
			/* free the resources if we receive or send REL_COMPL */
			q931_destroycall(pri, c->cr);
		else if (c->peercallstate == Q931_CALL_STATE_RELEASE_REQUEST)
			q931_release_complete(pri,c,cause);
		break;
	case Q931_CALL_STATE_CALL_INITIATED:
		/* we sent SETUP */
	case Q931_CALL_STATE_OVERLAP_SENDING:
		/* received SETUP_ACKNOWLEDGE */
	case Q931_CALL_STATE_OUTGOING_CALL_PROCEEDING:
		/* received CALL_PROCEEDING */
	case Q931_CALL_STATE_CALL_DELIVERED:
		/* received ALERTING */
	case Q931_CALL_STATE_CALL_PRESENT:
		/* received SETUP */
	case Q931_CALL_STATE_CALL_RECEIVED:
		/* sent ALERTING */
	case Q931_CALL_STATE_CONNECT_REQUEST:
		/* sent CONNECT */
	case Q931_CALL_STATE_INCOMING_CALL_PROCEEDING:
		/* we sent CALL_PROCEEDING */
	case Q931_CALL_STATE_ACTIVE:
		/* received CONNECT */
	case Q931_CALL_STATE_OVERLAP_RECEIVING:
		/* received SETUP_ACKNOWLEDGE */
		/* send DISCONNECT in general */
		if (c->peercallstate != Q931_CALL_STATE_NULL && c->peercallstate != Q931_CALL_STATE_DISCONNECT_REQUEST && c->peercallstate != Q931_CALL_STATE_DISCONNECT_INDICATION && c->peercallstate != Q931_CALL_STATE_RELEASE_REQUEST && c->peercallstate != Q931_CALL_STATE_RESTART_REQUEST && c->peercallstate != Q931_CALL_STATE_RESTART) {
			if (disconnect)
				q931_disconnect(pri,c,cause);
			else if (release_compl)
				q931_release_complete(pri,c,cause);
			else
				q931_release(pri,c,cause);
		} else 
			pri_error("Wierd, doing nothing but this shouldn't happen, ourstate %s, peerstate %s\n",callstate2str(c->ourcallstate),callstate2str(c->peercallstate));
		break;
	case Q931_CALL_STATE_DISCONNECT_REQUEST:
		/* sent DISCONNECT */
		q931_release(pri,c,cause);
		break;
	case Q931_CALL_STATE_DISCONNECT_INDICATION:
		/* received DISCONNECT */
		if (c->peercallstate == Q931_CALL_STATE_DISCONNECT_REQUEST) {
			c->alive = 1;
			q931_release(pri,c,cause);
		}
		break;
	case Q931_CALL_STATE_RELEASE_REQUEST:
		/* sent RELEASE */
		/* don't do anything, waiting for RELEASE_COMPLETE */
		break;
	case Q931_CALL_STATE_RESTART:
	case Q931_CALL_STATE_RESTART_REQUEST:
		/* sent RESTART */
		pri_error("q931_hangup shouldn't be called in this state, ourstate %s, peerstate %s\n",callstate2str(c->ourcallstate),callstate2str(c->peercallstate));
		break;
	default:
		pri_error("We're not yet handling hanging up when our state is %d, contact support@digium.com, ourstate %s, peerstate %s\n",callstate2str(c->ourcallstate),callstate2str(c->peercallstate));
		return -1;
	}
	/* we did handle hangup properly at this point */
	return 0;
}

int q931_receive(struct pri *pri, q931_h *h, int len)
{
	q931_mh *mh;
	q931_call *c;
	q931_ie *ie;
	unsigned int x;
	int y;
	int res;
	int r;
	int mandies[MAX_MAND_IES];
	int missingmand;
	int codeset, cur_codeset;
	int last_ie[8];
	memset(last_ie, 0, sizeof(last_ie));
	if (pri->debug & PRI_DEBUG_Q931_DUMP)
		q931_dump(h, len, 0);
#ifdef LIBPRI_COUNTERS
	pri->q931_rxcount++;
#endif
	mh = (q931_mh *)(h->contents + h->crlen);
	if ((h->pd == 0x3) || (h->pd == 0x43)) {
		/* This is the weird maintenance stuff.  We majorly
		   KLUDGE this by changing byte 4 from a 0xf (SERVICE) 
		   to a 0x7 (SERVICE ACKNOWLEDGE) */
		h->raw[h->crlen + 2] -= 0x8;
		q931_xmit(pri, h, len, 1);
		return 0;
	} else if (h->pd != pri->protodisc) {
		pri_error("Warning: unknown/inappropriate protocol discriminator received (%02x/%d)\n", h->pd, h->pd);
		return 0;
	}
	c = q931_getcall(pri, q931_cr(h));
	if (!c) {
		pri_error("Unable to locate call %d\n", q931_cr(h));
		return -1;
	}
	/* Preliminary handling */
	switch(mh->msg) {
	case Q931_RESTART:
		if (pri->debug & PRI_DEBUG_Q931_STATE)
			pri_message("-- Processing Q.931 Restart\n");
		/* Reset information */
		c->channelno = -1;
		c->slotmap = -1;
		c->chanflags = 0;
		c->ds1no = 0;
		c->ri = -1;
		break;
	case Q931_FACILITY:
		strcpy(c->callername, "");
		break;
	case Q931_SETUP:
		if (pri->debug & PRI_DEBUG_Q931_STATE)
			pri_message("-- Processing Q.931 Call Setup\n");
		c->channelno = -1;
		c->slotmap = -1;
		c->chanflags = 0;
		c->ds1no = 0;
		c->ri = -1;
		c->transcapability = -1;
		c->transmoderate = -1;
		c->transmultiple = -1;
		c->userl1 = -1;
		c->userl2 = -1;
		c->userl3 = -1;
		c->rateadaption = -1;
		c->calledplan = -1;
		c->callerplan = -1;
		c->callerpres = -1;
		strcpy(c->callernum, "");
		strcpy(c->callednum, "");
		strcpy(c->callername, "");
                c->redirectingplan = -1;
                c->redirectingpres = -1;
                c->redirectingreason = -1;
		strcpy(c->redirectingnum, "");
                c->useruserprotocoldisc = -1; 
		strcpy(c->useruserinfo, "");
		c->complete = 0;
		break;
	case Q931_CONNECT:
	case Q931_ALERTING:
	case Q931_PROGRESS:
		c->progress = -1;
		break;
	case Q931_CONNECT_ACKNOWLEDGE:
		if (c->retranstimer)
			pri_schedule_del(pri, c->retranstimer);
		c->retranstimer = 0;
		/* Fall through */
	case Q931_CALL_PROCEEDING:
		/* Do nothing */
		break;
	case Q931_RELEASE:
	case Q931_DISCONNECT:
		c->cause = -1;
		c->causecode = -1;
		c->causeloc = -1;
		if (c->retranstimer)
			pri_schedule_del(pri, c->retranstimer);
		c->retranstimer = 0;
		break;
	case Q931_RELEASE_COMPLETE:
		if (c->retranstimer)
			pri_schedule_del(pri, c->retranstimer);
		c->retranstimer = 0;
	case Q931_STATUS:
		c->cause = -1;
		c->causecode = -1;
		c->causeloc = -1;
		c->sugcallstate = -1;
		break;
	case Q931_RESTART_ACKNOWLEDGE:
		c->channelno = -1;
		break;
	case Q931_INFORMATION:
		break;
	case Q931_STATUS_ENQUIRY:
		break;
	case Q931_SETUP_ACKNOWLEDGE:
		break;
	case Q931_NOTIFY:
		break;
	case Q931_USER_INFORMATION:
	case Q931_SEGMENT:
	case Q931_CONGESTION_CONTROL:
	case Q931_HOLD:
	case Q931_HOLD_ACKNOWLEDGE:
	case Q931_HOLD_REJECT:
	case Q931_RETRIEVE:
	case Q931_RETRIEVE_ACKNOWLEDGE:
	case Q931_RETRIEVE_REJECT:
	case Q931_RESUME:
	case Q931_RESUME_ACKNOWLEDGE:
	case Q931_RESUME_REJECT:
	case Q931_SUSPEND:
	case Q931_SUSPEND_ACKNOWLEDGE:
	case Q931_SUSPEND_REJECT:
		pri_error("!! Not yet handling pre-handle message type %s (%d)\n", msg2str(mh->msg), mh->msg);
		/* Fall through */
	default:
		pri_error("!! Don't know how to post-handle message type %s (%d)\n", msg2str(mh->msg), mh->msg);
		q931_status(pri,c, PRI_CAUSE_MESSAGE_TYPE_NONEXIST);
		if (c->newcall) 
			q931_destroycall(pri,c->cr);
		return -1;
	}
	memset(mandies, 0, sizeof(mandies));
	missingmand = 0;
	for (x=0;x<sizeof(msgs) / sizeof(msgs[0]); x++)  {
		if (msgs[x].msgnum == mh->msg) {
			memcpy(mandies, msgs[x].mandies, sizeof(mandies));
		}
	}
	x = 0;
	/* Do real IE processing */
	len -= (h->crlen + 3);
	codeset = cur_codeset = 0;
	while(len) {
		ie = (q931_ie *)(mh->data + x);
		for (y=0;y<MAX_MAND_IES;y++) {
			if (mandies[y] == Q931_FULL_IE(cur_codeset, ie->ie))
				mandies[y] = 0;
		}
		r = ielen(ie);
		if (r > len) {
			pri_error("XXX Message longer than it should be?? XXX\n");
			return -1;
		}
		/* Special processing for codeset shifts */
		switch (ie->ie & 0xf8) {
		case Q931_LOCKING_SHIFT:
			y = ie->ie & 7;	/* Requested codeset */
			/* Locking shifts couldn't go to lower codeset, and couldn't follows non-locking shifts - verify this */
			if ((cur_codeset != codeset) && (pri->debug & PRI_DEBUG_Q931_ANOMALY))
				pri_message("XXX Locking shift immediately follows non-locking shift (from %d through %d to %d) XXX\n", codeset, cur_codeset, y);
			if (y > 0) {
				if ((y < codeset) && (pri->debug & PRI_DEBUG_Q931_ANOMALY))
					pri_error("!! Trying to locked downshift codeset from %d to %d !!\n", codeset, y);
				codeset = cur_codeset = y;
			}
			else {
				/* Locking shift to codeset 0 is forbidden by all specifications */
				pri_error("!! Invalid locking shift to codeset 0 !!\n");
			}
			break;
		case Q931_NON_LOCKING_SHIFT:
			cur_codeset = ie->ie & 7;
			break;
		default:
			/* Sanity check for IE code order */
			if (!(ie->ie & 0x80)) {
				if (last_ie[cur_codeset] > ie->ie) {
					if ((pri->debug & PRI_DEBUG_Q931_ANOMALY))
						pri_message("XXX Out-of-order IE %d at codeset %d (last was %d)\n", ie->ie, cur_codeset, last_ie[cur_codeset]);
				}
				else
					last_ie[cur_codeset] = ie->ie;
			}
			/* Ignore non-locking shifts for TR41459-based signalling */
			switch (pri->switchtype) {
			case PRI_SWITCH_LUCENT5E:
			case PRI_SWITCH_ATT4ESS:
				if (cur_codeset != codeset) {
					if ((pri->debug & PRI_DEBUG_Q931_DUMP))
						pri_message("XXX Ignoring IE %d for temporary codeset %d XXX\n", ie->ie, cur_codeset);
					break;
				}
				/* Fall through */
			default:
				y = q931_handle_ie(cur_codeset, pri, c, mh->msg, ie);
				/* XXX Applicable to codeset 0 only? XXX */
				if (!cur_codeset && !(ie->ie & 0xf0) && (y < 0))
					mandies[MAX_MAND_IES - 1] = Q931_FULL_IE(cur_codeset, ie->ie);
			}
			/* Reset current codeset */
			cur_codeset = codeset;
		}
		x += r;
		len -= r;
	}
	missingmand = 0;
	for (x=0;x<MAX_MAND_IES;x++) {
		if (mandies[x]) {
			/* check if there is no channel identification when we're configured as network -> that's not an error */
			if ((pri->localtype != PRI_NETWORK) || (mh->msg != Q931_SETUP) || (mandies[x] != Q931_CHANNEL_IDENT)) {
				pri_error("XXX Missing handling for mandatory IE %d (cs%d, %s) XXX\n", Q931_IE_IE(mandies[x]), Q931_IE_CODESET(mandies[x]), ie2str(mandies[x]));
				missingmand++;
			}
		}
	}
	
	/* Post handling */
	switch(mh->msg) {
	case Q931_RESTART:
		if (missingmand) {
			q931_status(pri, c, PRI_CAUSE_MANDATORY_IE_MISSING);
			q931_destroycall(pri, c->cr);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_RESTART;
		c->peercallstate = Q931_CALL_STATE_RESTART_REQUEST;
		/* Send back the Restart Acknowledge */
		restart_ack(pri, c);
		/* Notify user of restart event */
		pri->ev.e = PRI_EVENT_RESTART;
		pri->ev.restart.channel = c->channelno | (c->ds1no << 8);
		return Q931_RES_HAVEEVENT;
	case Q931_SETUP:
		if (missingmand) {
			q931_release_complete(pri, c, PRI_CAUSE_MANDATORY_IE_MISSING);
			break;
		}
		/* Must be new call */
		if (!c->newcall) {
			break;
		}
		c->newcall = 0;
		c->ourcallstate = Q931_CALL_STATE_CALL_PRESENT;
		c->peercallstate = Q931_CALL_STATE_CALL_INITIATED;
		/* it's not yet a call since higher level can respond with RELEASE or RELEASE_COMPLETE */
		c->alive = 0;
		pri->ev.e = PRI_EVENT_RING;
		pri->ev.ring.channel = c->channelno | (c->ds1no << 8);
		pri->ev.ring.callingpres = c->callerpres;
		pri->ev.ring.callingplan = c->callerplan;
		strncpy(pri->ev.ring.callingnum, c->callernum, sizeof(pri->ev.ring.callingnum) - 1);
		strncpy(pri->ev.ring.callingname, c->callername, sizeof(pri->ev.ring.callingname) - 1);
		pri->ev.ring.calledplan = c->calledplan;
		strncpy(pri->ev.ring.callingsubaddr, c->callingsubaddr, sizeof(pri->ev.ring.callingsubaddr) - 1);
		strncpy(pri->ev.ring.callednum, c->callednum, sizeof(pri->ev.ring.callednum) - 1);
                strncpy(pri->ev.ring.redirectingnum, c->redirectingnum, sizeof(pri->ev.ring.redirectingnum) - 1);
                strncpy(pri->ev.ring.useruserinfo, c->useruserinfo, sizeof(pri->ev.ring.useruserinfo) - 1);
		pri->ev.ring.flexible = ! (c->chanflags & FLAG_EXCLUSIVE);
		pri->ev.ring.cref = c->cr;
		pri->ev.ring.call = c;
		pri->ev.ring.layer1 = c->userl1;
		pri->ev.ring.complete = c->complete; 
		pri->ev.ring.ctype = c->transcapability;
		if (c->transmoderate != TRANS_MODE_64_CIRCUIT) {
			q931_release_complete(pri, c, PRI_CAUSE_BEARERCAPABILITY_NOTIMPL);
			break;
		}
		return Q931_RES_HAVEEVENT;
	case Q931_ALERTING:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_CALL_DELIVERED;
		c->peercallstate = Q931_CALL_STATE_CALL_RECEIVED;
		pri->ev.e = PRI_EVENT_RINGING;
		pri->ev.ringing.channel = c->channelno | (c->ds1no << 8);
		pri->ev.ringing.cref = c->cr;
		pri->ev.ringing.call = c;
		return Q931_RES_HAVEEVENT;
	case Q931_CONNECT:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		if (c->ourcallstate == Q931_CALL_STATE_ACTIVE) {
			q931_status(pri, c, PRI_CAUSE_WRONG_MESSAGE);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_ACTIVE;
		c->peercallstate = Q931_CALL_STATE_CONNECT_REQUEST;
		pri->ev.e = PRI_EVENT_ANSWER;
		pri->ev.answer.channel = c->channelno | (c->ds1no << 8);
		pri->ev.answer.cref = c->cr;
		pri->ev.answer.call = c;
		q931_connect_acknowledge(pri, c);
		return Q931_RES_HAVEEVENT;
	case Q931_FACILITY:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		pri->ev.e = PRI_EVENT_FACNAME;
		strncpy(pri->ev.facname.callingname, c->callername, sizeof(pri->ev.facname.callingname) - 1);
		strncpy(pri->ev.facname.callingnum, c->callernum, sizeof(pri->ev.facname.callingname) - 1);
		pri->ev.facname.channel = c->channelno | (c->ds1no << 8);
		pri->ev.facname.cref = c->cr;
		pri->ev.facname.call = c;
#if 0
		pri_message("Sending facility event (%s/%s)\n", pri->ev.facname.callingname, pri->ev.facname.callingnum);
#endif
		return Q931_RES_HAVEEVENT;
	case Q931_PROGRESS:
		if (missingmand) {
			q931_status(pri, c, PRI_CAUSE_MANDATORY_IE_MISSING);
			q931_destroycall(pri, c->cr);
			break;
		}
		pri->ev.e = PRI_EVENT_PROGRESS;
		/* Fall through */
	case Q931_CALL_PROCEEDING:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		if ((c->ourcallstate != Q931_CALL_STATE_CALL_INITIATED) &&
		    (c->ourcallstate != Q931_CALL_STATE_OVERLAP_SENDING) && 
		    (c->ourcallstate != Q931_CALL_STATE_CALL_DELIVERED) && 
		    (c->ourcallstate != Q931_CALL_STATE_OUTGOING_CALL_PROCEEDING)) {
			q931_status(pri,c,PRI_CAUSE_WRONG_MESSAGE);
			break;
		}
		pri->ev.proceeding.channel = c->channelno | (c->ds1no << 8);
		if (mh->msg == Q931_CALL_PROCEEDING) {
			pri->ev.e = PRI_EVENT_PROCEEDING;
			c->ourcallstate = Q931_CALL_STATE_OUTGOING_CALL_PROCEEDING;
			c->peercallstate = Q931_CALL_STATE_INCOMING_CALL_PROCEEDING;
		}
		return Q931_RES_HAVEEVENT;
	case Q931_CONNECT_ACKNOWLEDGE:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		if (c->ourcallstate != Q931_CALL_STATE_CONNECT_REQUEST) {
			q931_status(pri,c,PRI_CAUSE_WRONG_MESSAGE);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_ACTIVE;
		c->peercallstate = Q931_CALL_STATE_ACTIVE;
		break;
	case Q931_STATUS:
		if (missingmand) {
			q931_status(pri, c, PRI_CAUSE_MANDATORY_IE_MISSING);
			q931_destroycall(pri, c->cr);
			break;
		}
		if (c->newcall) {
			if (c->cr & 0x7fff)
				q931_release_complete(pri,c,PRI_CAUSE_WRONG_CALL_STATE);
			break;
		}
		/* Do nothing */
		/* Also when the STATUS asks for the call of an unexisting reference send RELEASE_COMPL */
		if ((pri->debug & PRI_DEBUG_Q931_ANOMALY) &&
		    (c->cause != PRI_CAUSE_INTERWORKING)) 
			pri_error("Received unsolicited status: %s\n", pri_cause2str(c->cause));
		if (!c->sugcallstate) {
			pri->ev.hangup.channel = c->channelno | (c->ds1no << 8);
			pri->ev.hangup.cref = c->cr;          		
			pri->ev.hangup.cause = c->cause;      		
			pri->ev.hangup.call = c;              		
			/* Free resources */
			c->ourcallstate = Q931_CALL_STATE_NULL;
			c->peercallstate = Q931_CALL_STATE_NULL;
			if (c->alive) {
				pri->ev.e = PRI_EVENT_HANGUP;
				res = Q931_RES_HAVEEVENT;
				c->alive = 0;
			} else if (c->sendhangupack) {
				res = Q931_RES_HAVEEVENT;
				pri->ev.e = PRI_EVENT_HANGUP_ACK;
				q931_hangup(pri, c, c->cause);
			} else {
				q931_hangup(pri, c, c->cause);
				res = 0;
			}
			if (res)
				return res;
		}
		break;
	case Q931_RELEASE_COMPLETE:
		c->ourcallstate = Q931_CALL_STATE_NULL;
		c->peercallstate = Q931_CALL_STATE_NULL;
		pri->ev.hangup.channel = c->channelno | (c->ds1no << 8);
		pri->ev.hangup.cref = c->cr;          		
		pri->ev.hangup.cause = c->cause;      		
		pri->ev.hangup.call = c;              		
		/* Free resources */
		if (c->alive) {
			pri->ev.e = PRI_EVENT_HANGUP;
			res = Q931_RES_HAVEEVENT;
			c->alive = 0;
		} else if (c->sendhangupack) {
			res = Q931_RES_HAVEEVENT;
			pri->ev.e = PRI_EVENT_HANGUP_ACK;
			pri_hangup(pri, c, c->cause);
		} else
			res = 0;
		if (res)
			return res;
		else
			q931_hangup(pri,c,c->cause);
		break;
	case Q931_RELEASE:
		if (missingmand) {
			/* Force cause to be mandatory IE missing */
			c->cause = PRI_CAUSE_MANDATORY_IE_MISSING;
		}
		if (c->ourcallstate == Q931_CALL_STATE_RELEASE_REQUEST) 
			c->peercallstate = Q931_CALL_STATE_NULL;
		else {
			c->peercallstate = Q931_CALL_STATE_RELEASE_REQUEST;
		}
		c->ourcallstate = Q931_CALL_STATE_NULL;
		pri->ev.e = PRI_EVENT_HANGUP;
		pri->ev.hangup.channel = c->channelno | (c->ds1no << 8);
		pri->ev.hangup.cref = c->cr;
		pri->ev.hangup.cause = c->cause;
		pri->ev.hangup.call = c;
		/* Don't send release complete if they send us release 
		   while we sent it, assume a NULL state */
		if (c->newcall)
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
		else 
			return Q931_RES_HAVEEVENT;
		break;
	case Q931_DISCONNECT:
		if (missingmand) {
			/* Still let user call release */
			c->cause = PRI_CAUSE_MANDATORY_IE_MISSING;
		}
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_DISCONNECT_INDICATION;
		c->peercallstate = Q931_CALL_STATE_DISCONNECT_REQUEST;
		c->sendhangupack = 1;
		/* Return such an event */
		pri->ev.e = PRI_EVENT_HANGUP_REQ;
		pri->ev.hangup.channel = c->channelno | (c->ds1no << 8);
		pri->ev.hangup.cref = c->cr;
		pri->ev.hangup.cause = c->cause;
		pri->ev.hangup.call = c;
		if (c->alive)
			return Q931_RES_HAVEEVENT;
		else
			q931_hangup(pri,c,c->cause);
		break;
	case Q931_RESTART_ACKNOWLEDGE:
		c->ourcallstate = Q931_CALL_STATE_NULL;
		c->peercallstate = Q931_CALL_STATE_NULL;
		pri->ev.e = PRI_EVENT_RESTART_ACK;
		pri->ev.restartack.channel = c->channelno | (c->ds1no << 8);
		return Q931_RES_HAVEEVENT;
	case Q931_INFORMATION:
		/* XXX We're handling only INFORMATION messages that contain
		       overlap dialing received digit
		       +  the "Complete" msg which is basically an EOF on further digits
		   XXX */
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		if (c->ourcallstate!=Q931_CALL_STATE_OVERLAP_RECEIVING)
			break;
		pri->ev.e = PRI_EVENT_INFO_RECEIVED;
		pri->ev.ring.call = c;
		pri->ev.ring.channel = c->channelno | (c->ds1no << 8);
		strncpy(pri->ev.ring.callednum, c->callednum, sizeof(pri->ev.ring.callednum) - 1);
		strncpy(pri->ev.ring.callingsubaddr, c->callingsubaddr, sizeof(pri->ev.ring.callingsubaddr) - 1);
		pri->ev.ring.complete = c->complete; 	/* this covers IE 33 (Sending Complete) */
		return Q931_RES_HAVEEVENT;
		break;
	case Q931_STATUS_ENQUIRY:
		if (c->newcall) {
			q931_release_complete(pri, c, PRI_CAUSE_INVALID_CALL_REFERENCE);
		} else
			q931_status(pri,c, 0);
		break;
	case Q931_SETUP_ACKNOWLEDGE:
		if (c->newcall) {
			q931_release_complete(pri,c,PRI_CAUSE_INVALID_CALL_REFERENCE);
			break;
		}
		c->ourcallstate = Q931_CALL_STATE_OVERLAP_SENDING;
		c->peercallstate = Q931_CALL_STATE_OVERLAP_RECEIVING;
		pri->ev.e = PRI_EVENT_SETUP_ACK;
		pri->ev.setup_ack.channel = c->channelno;
		return Q931_RES_HAVEEVENT;
	case Q931_NOTIFY:
		pri->ev.e = PRI_EVENT_NOTIFY;
		pri->ev.notify.channel = c->channelno;
		pri->ev.notify.info = c->notify;
		return Q931_RES_HAVEEVENT;
	case Q931_USER_INFORMATION:
	case Q931_SEGMENT:
	case Q931_CONGESTION_CONTROL:
	case Q931_HOLD:
	case Q931_HOLD_ACKNOWLEDGE:
	case Q931_HOLD_REJECT:
	case Q931_RETRIEVE:
	case Q931_RETRIEVE_ACKNOWLEDGE:
	case Q931_RETRIEVE_REJECT:
	case Q931_RESUME:
	case Q931_RESUME_ACKNOWLEDGE:
	case Q931_RESUME_REJECT:
	case Q931_SUSPEND:
	case Q931_SUSPEND_ACKNOWLEDGE:
	case Q931_SUSPEND_REJECT:
		pri_error("!! Not yet handling post-handle message type %s (%d)\n", msg2str(mh->msg), mh->msg);
		/* Fall through */
	default:
		
		pri_error("!! Don't know how to post-handle message type %s (%d)\n", msg2str(mh->msg), mh->msg);
		q931_status(pri,c, PRI_CAUSE_MESSAGE_TYPE_NONEXIST);
		if (c->newcall) 
			q931_destroycall(pri,c->cr);
		return -1;
	}
	return 0;
}

int q931_call_getcrv(struct pri *pri, q931_call *call, int *callmode)
{
	if (callmode)
		*callmode = call->cr & 0x7;
	return ((call->cr & 0x7fff) >> 3);
}

int q931_call_setcrv(struct pri *pri, q931_call *call, int crv, int callmode)
{
	call->cr = (crv << 3) & 0x7fff;
	call->cr |= (callmode & 0x7);
	return 0;
}
