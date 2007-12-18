/*
 *  alba_db.c -- Albatross Database Management
 *
 *  Copyright (c) IPTC AB., 2002-5. All Rights Reserved.
 */

/******************************** Description *********************************/
/*
 *  Albatross Management routines for Albatross device that can be called from 
 *  ASP functions
 */

/********************************* Includes ***********************************/
#include    "ctype.h"
#include    "emfdb.h"
#include    "webs.h"
#include    "um.h"

#include "alba_db.h"

/**************************** External References *****************************/

extern char ALBA_PLATFORM [];

/********************************** Defines ***********************************/

extern int isAlbaMode( char* mode_of_operation );
extern int albaPortCount;

struct CODEC_mapt
{
    int payload_type;
    char* verb;
};

static struct CODEC_mapt CODEC_map[ 10 ] =
{ 
    {   0, "G.711 A-Law (64kbps)" }, 
    {   1, "G.711 u-Law (64kbps)" }, 
    {   2, "G.726-40 (40kbps)"    }, 
    {   3, "G.726-32 (32kbps)"    }, 
    {   4, "G.726-24 (24kbps)"    }, 
    {   5, "G.726-16 (16kbps)"    },
    {   6, "G.729A (8kbps)"       }, 
    {   7, "G.729AB (&lt;8kbps)"  },
    { 250, "MPEG-2.5 (16kbps)"    },
    { 251, "MPEG-2.5 (8kbps)"     }
};

static int didGwPorts = -1; // gw-ports database identifier
static int didExtPorts = -1; // ext-ports database identifier
static int didUsers = -1; // users database identifier

/*************************** Database functions *******************************/

/******************************************************************************/
/*
 *  getFirstRowData() - return a pointer to the first non-blank key value
 *                          in the given column for the given table.
 */

static char_t *getFirstRowData( int did, char_t* tableName, char_t* columnName, int* rid )
{
    char_t* columnData;
    int     row = 0;
    int     check = 0;
/*
 *  Move through table until we retrieve the first row with non-null 
 *  column data.
 */
    columnData = NULL;
    while ( ( check = dbReadStr( did, tableName, columnName, row, 
        &columnData ) == 0 ) || ( check == DB_ERR_ROW_DELETED ) ) 
    {
        if ( columnData && *columnData ) 
        {
            *rid = row;
            return columnData;
        }
        row++;
    }

    return NULL;
}

/******************************************************************************/
/*
 *  getNextRowData() -  return a pointer to the first non-blank 
 *                      key value following the given one.
 */

static char_t* getNextRowData( int did, char_t* tableName, char_t* columnName, 
                                char_t* keyLast, int* rid )
{
    char_t* key;
    int     row;
    int     check;
/*
 *  Position row counter to row where the given key value was found
 */
    row = 0;
    key = NULL;

    while ( ( ( ( check = dbReadStr( did, tableName, columnName, row++, 
        &key ) ) == 0 ) || ( check == DB_ERR_ROW_DELETED ) ) &&
        ( ( key == NULL ) || ( gstrcmp( key, keyLast ) != 0 ) ) ) 
    {
    }
/*
 *  If the last key value was not found, return NULL
 */
    if ( !key || gstrcmp( key, keyLast ) != 0 ) 
    {
        return NULL;
    }
/*
 *  Move through table until we retrieve the next row with a non-null key
 */
    while ( ( ( check = dbReadStr( did, tableName, columnName, row, &key ) ) 
        == 0 ) || ( check == DB_ERR_ROW_DELETED ) ) 
    {
        if ( key && *key && ( gstrcmp( key, keyLast ) != 0 ) ) 
        {
            *rid = row;
            return key;
        }
        row++;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
/* listGatewayPorts() - prints all data from GW_PORTS_TABLE. PORT is the key. */

static int listGatewayPorts( int eid, webs_t wp, int argc, char_t** argv )
{
    int i;
    int rid; // current row identifier
    GW_PORT* rec; // record pointer
    char* port = NULL; // current port
    char* load_port = NULL; // load settings from this port

    // Make db empty (not to have row duplicates)
    //
    dbZero( didGwPorts ); 
    dbLoad( didGwPorts, GW_DB_NAME, 0  );

    // Get current port and port to load settings from
    //
    port = websGetVar( wp, "port", "default" );
    load_port = websGetVar( wp, "defaults", NULL ) ? "default" : port;

    // Find port which settings should be loaded
    //
    int new_row = 0;
    rid = dbSearchStr( didGwPorts, GW_PORTS_TABLE, "PORT", load_port, 0 ); 
    if ( rid < 0 ) // if port does not exist, add it
    {
        new_row = 1;
        rid = dbAddRow( didGwPorts, GW_PORTS_TABLE );
        if ( rid < 0 )
            return 1;
    }

    // Get row pointer
    //
    rec = dbGetRowPtr( didGwPorts, GW_PORTS_TABLE, rid );
    if ( ! rec )
        return 1;

    if ( new_row )
    {
        // Override default value only for "PORT", LOCAL_UDP" column:
        //
        dbUpdateStr( &rec->PORT, load_port );

        if ( strcmp( port, "default" ) != 0 )
        {
            if ( isAlbaMode( "VR" ) ) 
                rec->LOCAL_UDP = 15050 + atoi( port ) - 1;
            else
                rec->LOCAL_UDP = 15000 + atoi( port ) - 1;
            }
        }

    // Display tabbed list with all ports
    //
    websWrite( wp, "<table border=0 style='margin-bottom:0;padding-bottom:0;'><tr><td>Port:&nbsp;</td><td>"
        "<table border=1 cellspacing=0 cellpadding=2 style='border-collapse: collapse' bordercolor='#111111'>" );

    websWrite( wp, "<td bgcolor=%s><a href='Config3.asp?port=default' style='%stext-decoration:none'>"
        "&nbsp;&nbsp;default&nbsp;&nbsp;</a></td>",
        strcmp( port, "default" ) == 0 ? "#000080" : "#FFFFFF",
        strcmp( port, "default" ) == 0 ? "color=white;" : ""
        );

    for ( i = 0; i < albaPortCount; i++ )
    {
        websWrite( wp, "<td bgcolor=%s><a href='Config3.asp?port=%d' style='%stext-decoration:none'>"
           "&nbsp;&nbsp;<b>%d</b>&nbsp;&nbsp;</a></td>", 
           i + 1 == atoi( port ) ? "#000080" : "#FFFFFF", i + 1, 
           i + 1 == atoi( port ) ? "color=white;" : "", i + 1 ); 
    }
    websWrite( wp, "</tr></table></td></tr></table>" );

    // Display table with port settings
    //
    websWrite( wp, "<table border=0 colspacing=2 cellpadding=1 style='margin-top:0;padding-top:0;'>" );

    websWrite( wp, "<tr height=5><td colspan=2><input type=hidden name=port value='%s'</td></tr>", port );

    if ( strcmp( port, "default" ) == 0 )
    {
        websWrite( wp, "<TR bgcolor=#F0F0E0><td colspan=2 align=left>"
            "<font color=#800000><b>Default Settings</b></font></td></tr>" );
    }
    else
    {
        websWrite( wp, "<TR bgcolor=#D0D0C0>"
                "<TD colspan=2 style='padding-left:5px'><font color=#800000><b>Port %s</b></font>"
                "</TD></TR>", port );

        if ( isAlbaMode( "GATEWAY" ) || isAlbaMode( "VR" ) )
        {
            // Print DN
            //
            websWrite( wp, 
                "<TR bgcolor=#F0F0F0>"
                "<TD style='padding-left:10px'>Directory Number:</TD><TD>&nbsp;"
                "<INPUT type=text name=DN value='%s' size=16 maxlength=16></TD></TR>",
                rec->DN ? rec->DN : ""
                );
        }

        // Print USE_DEFAULTS
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Use Default Settings:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=USE_DEFAULTS value=true onclick='check_dis();'%s></TD></TR>",
            rec->USE_DEFAULTS ? " checked" : "" 
            );
    }

    if ( ! isAlbaMode( "VR" ) ) 
    {
        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Network &mdash; Inbound</b></TD></TR>" );

        // Print LOCAL_UDP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>RTP UDP Port Base:</TD>"
            "<TD>&nbsp;<INPUT type=text name=LOCAL_UDP value='%d'"
            " size=5 maxlength=5 style='text-align:right'>&nbsp;</TD></TR>", rec->LOCAL_UDP );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Network &mdash; Outbound</b></TD></TR>" );

        // Print CODEC
        //
        websWrite( wp, "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Voice Frame Encoder (CODEC):</TD>" );
        websWrite( wp, "<TD>&nbsp;<SELECT name=CODEC>" );

        for ( i = 0; i <= 7; i++ )
        {
            websWrite( wp, "<option value=%d%s>%s</option>", 
                CODEC_map[i].payload_type, 
                rec->CODEC == CODEC_map[i].payload_type ? " selected" : "", CODEC_map[i].verb 
                );
        }

        websWrite( wp, "</SELECT>&nbsp;</TD></TR>" );

        // Print PKT_SIZE
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>RTP Packet Payload Size:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=PKT_SIZE value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;samples</TD></TR>", rec->PKT_SIZE );

        // Print PKT_COMPRTP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Compressed RTP Header Enable:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=PKT_COMPRTP value=true %s></TD></TR>",
            rec->PKT_COMPRTP ? " checked" : "" 
            );

        // Print DSCP_UDP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Diffserv codepoint for UDP:</TD>"
            "<TD>&nbsp;<INPUT type=text name=DSCP_UDP value='0x%02X'"
            " size=5 maxlength=5 style='text-align:right'>&nbsp;</TD></TR>", rec->DSCP_UDP );

        // Print DSCP_TCP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Diffserv codepoint for TCP:</TD>"
            "<TD>&nbsp;<INPUT type=text name=DSCP_TCP value='0x%02X'"
            " size=5 maxlength=5 style='text-align:right'>&nbsp;</TD></TR>", rec->DSCP_TCP );

        // Print BCH_LOOP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>B-Channel Loopback:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=BCH_LOOP value=true %s></TD></TR>",
            rec->BCH_LOOP ? " checked" : "" 
            );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Adaptive Jitter Buffer</b></TD></TR>" );

        // Print JIT_MINLEN
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Minimum Jitter Buffer Length:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=JIT_MINLEN value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;millisec</TD></TR>", rec->JIT_MINLEN );

        // Print JIT_MAXLEN
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Maximum Jitter Buffer Length:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=JIT_MAXLEN value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;millisec</TD></TR>", rec->JIT_MAXLEN );

        // Print JIT_ALPHA
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Predictor Smoothness:</TD>"
            "<TD>&nbsp;<INPUT type=text name=JIT_ALPHA value='%d'"
            " size=5 maxlength=3 style='text-align:right'></TD></TR>", rec->JIT_ALPHA );

        // Print JIT_BETA
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Variance Coefficient:</TD>"
            "<TD>&nbsp;<INPUT type=text name=JIT_BETA value='%d'"
            " size=5 maxlength=3 style='text-align:right'></TD></TR>", rec->JIT_BETA );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Line Echo Canceller (LEC)</b></TD></TR>" );

        // Print LEC
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Line Echo Canceller Enable:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=LEC value=true %s></TD></TR>",
            rec->LEC ? " checked" : "" 
            );

        // Print LEC_LEN
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Maximum Echo Tail Length:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=LEC_LEN value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;ms</TD></TR>", rec->LEC_LEN );

        // Print LEC_INIT
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Keep State Between Calls:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=LEC_INIT value=true %s></TD></TR>",
            rec->LEC_INIT ? " checked" : "" 
            );

        // Print LEC_MINERL
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Minimum Echo Return Loss (ERL):</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=LEC_MINERL value='%d'"
            " size=5 maxlength=3 style='text-align:right'>&nbsp;dB</TD></TR>", rec->LEC_MINERL );

        // Print NFE_MINNFL
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Minimum Noise Floor Level:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NFE_MINNFL value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;dBm0</TD></TR>", rec->NFE_MINNFL );

        // Print NFE_MAXNFL
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Maximum Noise Floor Level:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NFE_MAXNFL value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;dBm0</TD></TR>", rec->NFE_MAXNFL );

        // Print NLP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Non-Linear Processor (NLP) Enable:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=NLP value=true %s></TD></TR>",
            rec->NLP ? " checked" : "" 
            );

        // Print NLP_MINVPL
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Minimum Voice Power Level:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NLP_MINVPL value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;dBm0</TD></TR>", rec->NLP_MINVPL );

        // Print NLP_HANGT
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>NLP Hangover Time:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NLP_HANGT value='%d'"
            " size=5 maxlength=5 style='text-align:right'>&nbsp;ms</TD></TR>", rec->NLP_HANGT );

        // Print FMTD
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Fax/Modem Tone Detection Enable:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=FMTD value=true %s></TD></TR>",
            rec->FMTD ? " checked" : "" 
            );

        // Print GEN_P0DB
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Reference Level for 0 dBm:</TD>"
            "<TD>&nbsp;<INPUT type=text name=GEN_P0DB value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;</TD></TR>", rec->GEN_P0DB );

        // Print PKT_BLKSZ
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>LEC Block Size:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=PKT_BLKSZ value='%d'"
            " size=5 maxlength=3 style='text-align:right'>&nbsp;samples</TD></TR>", rec->PKT_BLKSZ );

        // Print PKT_ISRBLKSZ
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>ISR Block Size:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=PKT_ISRBLKSZ value='%d'"
            " size=5 maxlength=3 style='text-align:right'>&nbsp;samples</TD></TR>", rec->PKT_ISRBLKSZ );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Miscellaneous</b></TD></TR>" );

        // Print TX_AMP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>PCM-out Amplification Level:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=TX_AMP value='%d'"
            " size=5 maxlength=3 style='text-align:right'>&nbsp;dB</TD></TR>", rec->TX_AMP );

    }
    else // VR
    {
        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Voice Recording</b></TD></TR>" );

        // Print CODEC
        //
        websWrite( wp, "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>CODEC</TD>" );
        websWrite( wp, "<TD>&nbsp;<SELECT name=CODEC>" );

        for ( i = 0; i <= 9; i++ )
        {
            if ( i == 6 || i == 7 ) 
                continue; // skip G.729

            if ( strcmp( ALBA_PLATFORM, "arm" ) == 0 && ( i == 8 || i == 9 ) )
                continue; // skip MP3 on arm

            websWrite( wp, "<option value=%d%s>%s</option>", 
                CODEC_map[i].payload_type, 
                rec->CODEC == CODEC_map[i].payload_type ? " selected" : "", CODEC_map[i].verb 
                );
        }

        websWrite( wp, "</SELECT>&nbsp;</TD></TR>" );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>NAS Distribution List</b></TD></TR>" );

        // Print NAS URL1
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>FTP URL (1):</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NAS_URL1 value='%s'"
            " size=50 maxlength=128 style='text-align:left'>&nbsp;</TD></TR>", rec->NAS_URL1 ? rec->NAS_URL1 : "" );

        // Print NAS URL2
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>FTP URL (2):</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=NAS_URL2 value='%s'"
            " size=50 maxlength=128 style='text-align:left'>&nbsp;</TD></TR>", rec->NAS_URL2 ? rec->NAS_URL2 : "" );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Recorded File Size</b></TD></TR>" );

        // Print SPLIT_FSIZE
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Max Recorded File Size:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=SPLIT_FSIZE value='%d'"
            " size=8 maxlength=8 style='text-align:right'>&nbsp;sec (0 if unlimited)</TD></TR>", rec->SPLIT_FSIZE );

        // Print VR_MINFSIZE
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Min Recorded File Size:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=VR_MINFSIZE value='%d'"
            " size=8 maxlength=8 style='text-align:right'>&nbsp;ms</TD></TR>", rec->VR_MINFSIZE );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Voice Activity Detection (VAD)</b></TD></TR>" );

        // Print VRVAD_ACTIVE
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Voice Activity Detection Enable:</TD>"
            "<TD>&nbsp;<INPUT type=checkbox name=VRVAD_ACTIVE value=true %s></TD></TR>",
            rec->VRVAD_ACTIVE ? " checked" : "" 
            );

        // Print VRVAD_THRSH
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>VAD Trheshold Level:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=VRVAD_THRSH value='%d'"
            " size=8 maxlength=8 style='text-align:right'>&nbsp;(PCM level)</TD></TR>", rec->VRVAD_THRSH );

        // Print VRVAD_HANGT
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>VAD Hangover-Time:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=VRVAD_HANGT value='%d'"
            " size=8 maxlength=8 style='text-align:right'>&nbsp;ms</TD></TR>", rec->VRVAD_HANGT );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Recording Control</b></TD></TR>" );

        // Print STARTREC_KEY
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Start Recording Key:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=STARTREC_KEY value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;(-1 for automatic start)</TD></TR>", rec->STARTREC_KEY );

        // Print STOPREC_KEY
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Stop Recording Key:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=STOPREC_KEY value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;(-1 if disabled)</TD></TR>", rec->STOPREC_KEY );

        // Print BOMB_KEY
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>Bomb-threat Key:</TD>"
            "<TD style='color:gray'>&nbsp;<INPUT type=text name=BOMB_KEY value='%d'"
            " size=5 maxlength=4 style='text-align:right'>&nbsp;(-1 if disabled)</TD></TR>", rec->BOMB_KEY );

        // Print LOCAL_UDP
        //
        websWrite( wp, 
            "<TR bgcolor=#F0F0F0><TD style='padding-left:10px'>UDP Port Base:</TD>"
            "<TD>&nbsp;<INPUT type=text name=LOCAL_UDP value='%d'"
            " size=5 maxlength=5 style='text-align:right'>&nbsp;</TD></TR>", rec->LOCAL_UDP );

        websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 style='padding-left:5px'>"
            "<b>Network &mdash; Outbound</b></TD></TR>" );
    }

    websWrite( wp, "<TR bgcolor=#D0D0C0><TD colspan=2 height=3></TD></TR>" );

    websWrite( wp, "</table>" );
    ejSetResult(eid, "1" );

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
/* saveGatewayPorts() - saves data to GW_PORTS_TABLE */

static int saveGatewayPorts( int eid, webs_t wp, int argc, char_t** argv )
{
    int rc; // return code from dbXX function
    int rid; // current row identifier
    char* port = NULL;

    // Make db empty (not to have row duplicates)
    //
    dbZero( didGwPorts ); 
    dbLoad( didGwPorts, GW_DB_NAME, 0  );

    port = websGetVar( wp, "port", NULL );
    if ( ! port )
    {
        ejSetResult( eid, "Port not specified! " );
        return 1;
    }

    // Find port
    //
    int new_row = 0;
    rid = dbSearchStr( didGwPorts, GW_PORTS_TABLE, "PORT", port, 0); 
    if ( rid < 0 ) // if port does not exist, add it
    {
        new_row = 1;
        rid = dbAddRow( didGwPorts, GW_PORTS_TABLE );
        if ( rid < 0 )
            return 1;
    }

    // Get row pointer
    //
    GW_PORT* rec = dbGetRowPtr( didGwPorts, GW_PORTS_TABLE, rid );
    if ( ! rec )
        return 1;

    if ( new_row )
    {
        // Override default value only for "PORT"
        //
        dbUpdateStr( &rec->PORT, port );
    }

    // scanf data from web to local variables
    //
    dbUpdateStr( &rec->DN, websGetVar( wp, "DN", "" ) );
    dbUpdateStr( &rec->NAS_URL1, websGetVar( wp, "NAS_URL1", "" ) );
    dbUpdateStr( &rec->NAS_URL2, websGetVar( wp, "NAS_URL2", "" ) );

    if ( ! websGetVar( wp, "LOCAL_UDP", NULL ) )
    {
        if ( isAlbaMode( "VR" ) )
            rec->LOCAL_UDP = 15050 + atoi( port );
        else
            rec->LOCAL_UDP = 15000 + atoi( port );
        }
    else
        sscanf( websGetVar( wp, "LOCAL_UDP", "" ), "%d", &rec->LOCAL_UDP );

    rec->USE_DEFAULTS = strcmp( websGetVar( wp, "USE_DEFAULTS", "" ), "true" ) == 0;
    rec->LEC          = strcmp( websGetVar( wp, "LEC",          "" ), "true" ) == 0;
    rec->LEC_INIT     = strcmp( websGetVar( wp, "LEC_INIT",     "" ), "true" ) == 0;
    rec->NLP          = strcmp( websGetVar( wp, "NLP",          "" ), "true" ) == 0;
    rec->FMTD         = strcmp( websGetVar( wp, "FMTD",         "" ), "true" ) == 0;
    rec->PKT_COMPRTP  = strcmp( websGetVar( wp, "PKT_COMPRTP",  "" ), "true" ) == 0;
    rec->BCH_LOOP     = strcmp( websGetVar( wp, "BCH_LOOP",     "" ), "true" ) == 0;
    rec->VRVAD_ACTIVE = strcmp( websGetVar( wp, "VRVAD_ACTIVE", "" ), "true" ) == 0;

    sscanf( websGetVar( wp, "DSCP_UDP",     "" ), "%i", &rec->DSCP_UDP     );
    sscanf( websGetVar( wp, "DSCP_TCP",     "" ), "%i", &rec->DSCP_TCP     );
    sscanf( websGetVar( wp, "CODEC",        "" ), "%d", &rec->CODEC        );
    sscanf( websGetVar( wp, "JIT_MINLEN",   "" ), "%d", &rec->JIT_MINLEN   );
    sscanf( websGetVar( wp, "JIT_MAXLEN",   "" ), "%d", &rec->JIT_MAXLEN   );
    sscanf( websGetVar( wp, "JIT_ALPHA",    "" ), "%d", &rec->JIT_ALPHA    );
    sscanf( websGetVar( wp, "JIT_BETA",     "" ), "%d", &rec->JIT_BETA     );
    sscanf( websGetVar( wp, "LEC_LEN",      "" ), "%d", &rec->LEC_LEN      );
    sscanf( websGetVar( wp, "LEC_MINERL",   "" ), "%d", &rec->LEC_MINERL   );
    sscanf( websGetVar( wp, "NFE_MINNFL",   "" ), "%d", &rec->NFE_MINNFL   );
    sscanf( websGetVar( wp, "NFE_MAXNFL",   "" ), "%d", &rec->NFE_MAXNFL   );
    sscanf( websGetVar( wp, "NLP_MINVPL",   "" ), "%d", &rec->NLP_MINVPL   );
    sscanf( websGetVar( wp, "NLP_HANGT",    "" ), "%d", &rec->NLP_HANGT    );
    sscanf( websGetVar( wp, "PKT_ISRBLKSZ", "" ), "%d", &rec->PKT_ISRBLKSZ );
    sscanf( websGetVar( wp, "PKT_BLKSZ",    "" ), "%d", &rec->PKT_BLKSZ    );
    sscanf( websGetVar( wp, "PKT_SIZE",     "" ), "%d", &rec->PKT_SIZE     );
    sscanf( websGetVar( wp, "GEN_P0DB",     "" ), "%d", &rec->GEN_P0DB     );
    sscanf( websGetVar( wp, "TX_AMP",       "" ), "%d", &rec->TX_AMP       );
    sscanf( websGetVar( wp, "BOMB_KEY",     "" ), "%d", &rec->BOMB_KEY     );
    sscanf( websGetVar( wp, "STARTREC_KEY", "" ), "%d", &rec->STARTREC_KEY );
    sscanf( websGetVar( wp, "STOPREC_KEY",  "" ), "%d", &rec->STOPREC_KEY  );
    sscanf( websGetVar( wp, "SPLIT_FSIZE",  "" ), "%d", &rec->SPLIT_FSIZE  );
    sscanf( websGetVar( wp, "VR_MINFSIZE",  "" ), "%d", &rec->VR_MINFSIZE  );
    sscanf( websGetVar( wp, "VRVAD_THRSH",  "" ), "%d", &rec->VRVAD_THRSH  );
    sscanf( websGetVar( wp, "VRVAD_HANGT",  "" ), "%d", &rec->VRVAD_HANGT  );

    // Sanity check (mostly boundary tests)

    if ( rec->LOCAL_UDP < 0 ) rec->LOCAL_UDP = 0;
    else if ( rec->LOCAL_UDP > 65535 ) rec->LOCAL_UDP = 65535;

    if ( rec->DSCP_UDP < 0 ) rec->DSCP_UDP = 0;
    else if ( rec->DSCP_UDP > 255 ) rec->DSCP_UDP = 255;

    if ( rec->DSCP_TCP < 0 ) rec->DSCP_TCP = 0;
    else if ( rec->DSCP_TCP > 255 ) rec->DSCP_TCP = 255;

    if ( rec->CODEC < 0 ) rec->CODEC = 0;
    else if ( rec->CODEC > 255 ) rec->CODEC = 0;

    if ( rec->JIT_MINLEN < 0 ) rec->JIT_MINLEN = 0;
    else if ( rec->JIT_MINLEN > 320 ) rec->JIT_MINLEN = 320;

    if ( rec->JIT_MAXLEN < 0 ) rec->JIT_MAXLEN = 0;
    else if ( rec->JIT_MAXLEN > 320 ) rec->JIT_MAXLEN = 320;

    if ( rec->JIT_MINLEN > rec->JIT_MAXLEN ) rec->JIT_MAXLEN = rec->JIT_MINLEN;

    if ( rec->JIT_ALPHA < 2 ) rec->JIT_ALPHA = 2;
    else if ( rec->JIT_ALPHA > 12 ) rec->JIT_ALPHA = 12;

    if ( rec->JIT_BETA < 1 ) rec->JIT_BETA = 1;
    else if( rec->JIT_BETA > 8 ) rec->JIT_BETA = 8;

    if ( rec->LEC_LEN < 32 ) rec->LEC_LEN = 32;
    else if ( rec->LEC_LEN > 128 ) rec->LEC_LEN = 128;

    if ( rec->LEC_MINERL != 0 && rec->LEC_MINERL != 3 && rec->LEC_MINERL != 6 )
        rec->LEC_MINERL = 6;

    if ( rec->NFE_MINNFL < -62 ) rec->NFE_MINNFL = -62;
    else if ( rec->NFE_MINNFL > -22 ) rec->NFE_MINNFL = -22;

    if ( rec->NFE_MAXNFL < -62 ) rec->NFE_MAXNFL = -62;
    else if ( rec->NFE_MAXNFL > -22 ) rec->NFE_MAXNFL = -22;

    if ( rec->NFE_MINNFL > rec->NFE_MAXNFL ) rec->NFE_MINNFL = -62;

    if ( rec->NLP_MINVPL < -78 ) rec->NLP_MINVPL = -78;
    else if ( rec->NLP_MINVPL > -40 && rec->NLP_MINVPL != 0 ) rec->NLP_MINVPL = -40;

    if ( rec->NLP_HANGT < 25 ) rec->NLP_HANGT = 25;
    else if ( rec->NLP_HANGT > 1000 ) rec->NLP_HANGT = 1000;

    if ( rec->PKT_ISRBLKSZ != 0 && rec->PKT_ISRBLKSZ != 1
        && rec->PKT_ISRBLKSZ != 2 && rec->PKT_ISRBLKSZ != 4
        && rec->PKT_ISRBLKSZ != 8 ) rec->PKT_ISRBLKSZ = 0;

    if ( rec->PKT_BLKSZ < 40 ) rec->PKT_BLKSZ = 40;
    else if ( rec->PKT_BLKSZ > 80 ) rec->PKT_BLKSZ = 80;

    if ( rec->PKT_SIZE < rec->PKT_BLKSZ ) rec->PKT_SIZE = rec->PKT_BLKSZ;
    else if ( rec->PKT_SIZE > 160 ) rec->PKT_SIZE = 160;
    
    if ( rec->GEN_P0DB < 1000 ) rec->GEN_P0DB = 1000;
    else if ( rec->GEN_P0DB > 8000 ) rec->GEN_P0DB = 8000;

    if ( rec->TX_AMP < -18 ) rec->TX_AMP = -18;
    else if ( rec->TX_AMP > 18 ) rec->TX_AMP = 18;

    if ( rec->BOMB_KEY < 0 ) rec->BOMB_KEY = -1;
    else if ( rec->BOMB_KEY > 255 ) rec->BOMB_KEY = 255;

    if ( rec->STARTREC_KEY < 0 ) rec->STARTREC_KEY = -1;
    else if ( rec->STARTREC_KEY > 255 ) rec->STARTREC_KEY = 255;

    if ( rec->STOPREC_KEY < 0 ) rec->STOPREC_KEY = -1;
    else if ( rec->STOPREC_KEY > 255 ) rec->STOPREC_KEY = 255;

    if ( rec->SPLIT_FSIZE <= 0 ) rec->SPLIT_FSIZE = -1;

    if ( rec->VR_MINFSIZE <= 0 ) rec->VR_MINFSIZE = 0;

    if ( rec->VRVAD_THRSH <= 0 ) rec->VRVAD_THRSH = 50;
    else if ( rec->VRVAD_THRSH > 32000 ) rec->VRVAD_THRSH = 50;

    if ( rec->VRVAD_HANGT <= 0 ) rec->VRVAD_HANGT = 5000;
    else if ( rec->VRVAD_HANGT > 32000 ) rec->VRVAD_HANGT = 5000;

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didGwPorts, GW_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database!" );
        return 1;
    }

    ejSetResult(eid, "1" );

    if ( websGetVar( wp, "apply", NULL ) )
    {
        char cmd[ 80 ];
        sprintf( cmd, "/albatross/bin/c54setp %s %s", port, websGetVar( wp, "apply" , "none" ) );
        system( cmd );
        if ( isAlbaMode( "VR" ) )
            sprintf( cmd, "killall idodVR" );
        system( cmd );
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
/* listExtenderPorts() - prints all data from EXT_DB_NAME, PORT is the key. */

static int listExtenderPorts( int eid, webs_t wp, int argc, char_t** argv )
{
    int i;
    int rid; // current row identifier

    dbZero( didExtPorts ); // make db empty (not to have row duplicates)
    dbLoad( didExtPorts, EXT_DB_NAME, 0  );

    // scan ports 1..6
    //
    for ( i = -1; i < albaPortCount; i++ )
    {
        char  port[ 32 ];
        char  suffix[ 8 ];

        if ( i == -1 )
        {  
            strcpy( port, "default" ); 
            strcpy( suffix, "def" );
        }
        else
        {
            sprintf( port, "%d", i + 1 );
            sprintf( suffix, "%d", i + 1 );
        }

        int new_row = 0;
        rid = dbSearchStr( didExtPorts, EXT_PORTS_TABLE, "PORT", port, 0 ); 
        if ( rid < 0 ) // if port does not exist, add it
        {
            new_row = 1;
            rid = dbAddRow( didExtPorts, EXT_PORTS_TABLE );
            if ( rid < 0 )
                return 1;
        }

        // Get row pointer
        //
        EXT_PORT* rec = dbGetRowPtr( didExtPorts, EXT_PORTS_TABLE, rid );
        if ( ! rec )
            return 1;

        if ( new_row && i >= 0 )
            rec->AUTH = -1; // AUTH=-1 means default

        // Print Port#
        //
        if (i == -1 )
            websWrite( wp, "<tr bgcolor=#F0F0F0>"
                       "<td><font size=-2>Defaults:</font></td>" );
        else
            websWrite( wp, "<tr bgcolor=#F0F0F0><td><b>%s</b></td>", port );
    
        // Print DN
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=DN%s value='%s' size=6 maxlength=16></TD>",
            suffix, rec->DN ? rec->DN : ""
            );

        // Print IPADDR
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=IPADDR%s value='%s' size=15 maxlength=30></TD>",
            suffix, rec->IPADDR ? rec->IPADDR : ""
            );

        // Print TCPPORT
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=TCPPORT%s value='%d' size=4></TD>",
            suffix, rec->TCPPORT
            );

        // Print UID
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=UID%s value='%s' size=9 maxlength=30></TD>",
            suffix, rec->UID ? rec->UID : ""
            );

        // Print PWD
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=PWD%s value='%s' size=9 maxlength=30></TD>",
            suffix, rec->PWD ? rec->PWD : ""
            );

        // Print AUTH
        //
        websWrite( wp, "<TD><SELECT name=AUTH%s>", suffix );
        websWrite( wp, "<option value=-1%s></option>", rec->AUTH == -1 ? 
                       " selected" : "" );
        websWrite( wp, "<option value=0%s>User</option>", rec->AUTH == 0 ? 
                       " selected" : "" );
        websWrite( wp, "<option value=1%s>Admin</option>", rec->AUTH == 1 ? 
                       " selected" : "" );
        websWrite( wp, "<option value=2%s>Monitor</option>", rec->AUTH == 2 ? 
                       " selected" : "" );
        websWrite( wp, "</SELECT></TD>" );

        // Print AUTOCONNECT
        //
        websWrite( wp, 
            "<TD><INPUT type=checkbox name=AUTOCONNECT%s value=true %s></TD>",
            suffix, rec->AUTOCONNECT ? " checked" : "" 
            );

        // Print RECONNECT
        //
        websWrite( wp, 
            "<TD><INPUT type=checkbox name=RECONNECT%s value=true %s></TD>",
            suffix, rec->RECONNECT ? " checked" : "" 
            );

        websWrite( wp, "</tr>" );
        }

    ejSetResult( eid, "1" );

    return 0;
}

static int listExtPortsMisc( int eid, webs_t wp, int argc, char_t** argv )
{
    // Must be called just after listExtenderPorts !!!
    
    int i;
    int rid; // current row identifier

    // scan ports 1..6
    //
    for ( i = 0; i < albaPortCount; i++ )
    {
        char  port[ 32 ];
        char  suffix[ 8 ];

        sprintf( port, "%d", i + 1 );
        sprintf( suffix, "%d", i + 1 );

        int new_row = 0;
        rid = dbSearchStr( didExtPorts, EXT_PORTS_TABLE, "PORT", port, 0 ); 
        if ( rid < 0 ) // if port does not exist, add it
        {
            new_row = 1;
            rid = dbAddRow( didExtPorts, EXT_PORTS_TABLE );
            if ( rid < 0 )
                return 1;
        }

        // Get row pointer
        //
        EXT_PORT* rec = dbGetRowPtr( didExtPorts, EXT_PORTS_TABLE, rid );
        if ( ! rec )
            return 1;

        // Print Port#
        //
        websWrite( wp, "<tr bgcolor=#F0F0F0><td><b>%s</b></td>", port );
    
        // Print TAUD configuration
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=TAUD%s value='%s' size=30 maxlength=30></TD>",
            suffix, rec->TAUD ? rec->TAUD : ""
            );

        // Print RETRYC
        //
        websWrite( wp, 
            "<TD><INPUT type=text name=RETRYC%s value='%d' size=5 maxlength=5></TD>",
            suffix, rec->RETRYC
            );

        websWrite( wp, "</tr>" );
        }

    ejSetResult( eid, "1" );

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
/* saveExtenderPorts() - saves data to EXT_DB_NAME */

static int saveExtenderPorts( int eid, webs_t wp, int argc, char_t** argv )
{
    // FIRST: Save default connection parameters
    int i;
    int rc; // return code from dbXX function
    int rid; // current row identifier

    dbZero( didExtPorts ); // make db empty (not to have row duplicates)
    dbLoad( didExtPorts, EXT_DB_NAME, 0  );

    int TAUD_usb_count = 0; // guard to allow only one usb port association

    // scan ports 1..6
    //
    for ( i = -1; i < albaPortCount; i++ )
    {
        char port[ 32 ];
        char field[ 32 ];
        char suffix[ 8 ];

        if ( i == -1 )
        {  
            strcpy( port, "default" ); 
            strcpy( suffix, "def" );
        }
        else
        {
            sprintf( port, "%d", i + 1 );
            sprintf( suffix, "%d", i + 1 );
        }

        int new_row = 0;
        rid = dbSearchStr( didExtPorts, EXT_PORTS_TABLE, "PORT", port, 0 ); 
        if ( rid < 0 ) // if port does not exist, add it
        {
            new_row = 1;
            rid = dbAddRow( didExtPorts, EXT_PORTS_TABLE );
            if ( rid < 0 )
                return 1;
        }

        // Get row pointer
        //
        EXT_PORT* rec = dbGetRowPtr( didExtPorts, EXT_PORTS_TABLE, rid );
        if ( ! rec )
            return 1;

        if ( new_row )
        { 
            // Update PORT, only if the record is newly added
            //
            dbUpdateStr( &rec->PORT, port );
        }

        // Update DN
        //
        sprintf( field, "DN%s", suffix );
        dbUpdateStr( &rec->DN, websGetVar( wp, field, "" ) );

        // Update IPADDR
        //
        sprintf( field, "IPADDR%s", suffix );
        dbUpdateStr( &rec->IPADDR, websGetVar( wp, field, "" ) );

        // Update TCPPORT
        //
        sprintf( field, "TCPPORT%s", suffix );
        sscanf( websGetVar( wp, field, "0" ), "%d", &rec->TCPPORT );

        // Update UID
        //
        sprintf( field, "UID%s", suffix );
        dbUpdateStr( &rec->UID, websGetVar( wp, field, "" ) );

        // Update PWD
        //
        sprintf( field, "PWD%s", suffix );
        dbUpdateStr( &rec->PWD, websGetVar( wp, field, "" ) );

        // Update AUTH
        //
        sprintf( field, "AUTH%s", suffix );
        sscanf( websGetVar( wp, field, "" ), "%d", &rec->AUTH );
        if ( i == -1 && rec->AUTH < 0 ) rec->AUTH = 0; // must not be default for default

        // Update AUTOCONNECT
        //
        sprintf( field, "AUTOCONNECT%s", suffix );
        rec->AUTOCONNECT  = strcmp( websGetVar( wp, field, "" ), "true" ) == 0;

        // Update RECONNECT
        //
        sprintf( field, "RECONNECT%s", suffix );
        rec->RECONNECT  = strcmp( websGetVar( wp, field, "" ), "true" ) == 0;
        
        // Update TAUD
        //
        sprintf( field, "TAUD%s", suffix );
        {
            char* value = websGetVar( wp, field, "" );
            char method[ 32 ];
            int isMethodUSB = sscanf( value, " %s", method ) >= 1 
                && strcasecmp( method, "usb" ) == 0;
            if ( isMethodUSB && TAUD_usb_count > 0 )
                value = ""; // remove this value; usb can be single option
                
            dbUpdateStr( &rec->TAUD, value );
            
            TAUD_usb_count += isMethodUSB;
            }

        // Update RETRYC
        //
        sprintf( field, "RETRYC%s", suffix );
        sscanf( websGetVar( wp, field, "0" ), "%d", &rec->RETRYC );
        }

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didExtPorts, EXT_DB_NAME, 0 );
    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    ejSetResult( eid, "1" );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
//
/* listUsers() - prints all users from USERS_TABLE */

static int listUsers( int eid, webs_t wp, int argc, char_t** argv )
{
    char* NAME = NULL;
    int rid = 0; // current row identifier is i now
    int i;

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    NAME = getFirstRowData( didUsers, USERS_TABLE, "NAME", &rid  );

    websWrite( wp, 
        "<table border=0 cellspacing=2 cellpadding=2 >"
        "<tbody align=center><tr bgcolor=#D0D0C0>"
        "<td><b>Username</b></td><td><b>Password</b></td>"
        "<td><b>Group</b></td><td><b>Disabled</b></td></tr>" );

    // show all users
    //
    for ( i = 0; NAME != NULL; i++ )
    {
        // Get row pointer
        //
        USER* rec = dbGetRowPtr( didUsers, USERS_TABLE, rid );
        if ( ! rec )
            break;

        // Print User
        //
        websWrite( wp, "<tr bgcolor=#F0F0F0><td align=left >"
            "<INPUT type=hidden name=NAME%d value='%s'>"
            "<font onclick=\"selUser('%s');\" style=\"cursor:pointer\">"
            "<b>%s</b></font></td>", i, rec->NAME, rec->NAME, rec->NAME );

        // Print Password
        //
        websWrite( wp, "<TD><INPUT type=text name=PASS%d value='%s' size=15>"
                       "</TD>", i, rec->PASS ? rec->PASS : "" );

        // Print Group
        //
        websWrite( wp, "<TD><INPUT type=text name=GROUP%d value='%s' size=15>"
                       "</TD>", i, rec->GROUP ? rec->GROUP : "" );
   
        // Print Disable
        //
        websWrite( wp, "<TD><INPUT type=checkbox name=DISABLED%d value=true %s>"
                       "</TD></TR>", i, rec->DISABLED ? " checked" : "" );

        // Find next user
        //
        NAME = getNextRowData( didUsers, USERS_TABLE, "NAME", NAME, &rid );
    }   

    websWrite( wp, "<tr><td height=10></td></tr>" );

    if ( isAlbaMode( "VR" ) )
    {
        websWrite( wp, "<TR><TD colspan=3 align=left>" );
        websWrite( wp, "<b>Note:</b> Username <b>voice</b> has special meaning.<br>" );
        websWrite( wp, "It is used to access windows shared disk with voice recordings." );
        websWrite( wp, "</TD></TR>" );
        websWrite( wp, "<tr><td height=10></td></tr>" );
    }

    websWrite( wp, "</tbody></table>" );

    ejSetResult( eid, "1" );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
/* chk_name( name , type ) - checks is name consists only letters or digits */

static int chk_name( int eid, char* name, char* type )
{
    int i;

    if ( name[ 0 ] == '\0' ) // is name is empty string
    {
        if ( 0 == strcmp( type, "user" ) )
            ejSetResult( eid,"Username must not be empty." );
        return 1;
    }

    for ( i=0; name[ i ] != '\0'; i++ ) 
    { 
        if ( !isalpha( name[ i ] ) && !isdigit( name[ i ] ) )
        {
            if ( 0 == strcmp( type, "user" ) )
                   ejSetResult(eid,"Username must consists only"
                                   " of letters an digits." );
            return 1;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
/* trim( word ) - cuts all spaces from word */

char* trim( char* word )
{
    int i = 0;
    int j = 0;

    while ( word[ i ] != '\0' )
    {
        if ( word[ i ] != ' ' )
        {
            word[ j ] = word[ i ];
            ++i;
            ++j;
        }
        else
            ++i;
    }
    word[ j ] = '\0'; 
    return word;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
/* addUser( name ) - adds new user to USERS_TABLE and adds default authorization to USERAUTHS_TABLE */

static int addUser( int eid, webs_t wp, int argc, char_t** argv )
{
    int rc;
    int rid; // current row identifier
    char_t* username;
   
    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
    {
        ejSetResult( eid, "0" );
        return 1; // argument count is wrong
    }

    username = trim( username );
    /* checking username */
    if ( chk_name( eid, username, "user" ) == 1 )
    {
        return 1;
    }

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    rid = dbSearchStr( didUsers, USERS_TABLE, "NAME", username, 0 ); 
    if ( rid >= 0 )
    {
        ejSetResult( eid,"User already exists." );
        return 1;
    }

    // Adding new user to USERS_TABLE
    //
    rid = dbAddRow( didUsers, USERS_TABLE );
    if ( rid < 0 )
    {
        ejSetResult( eid, "Cannot add user! " );
        return 1;
    }

    // Get row pointer
    //
    USER* rec = dbGetRowPtr( didUsers, USERS_TABLE, rid );
    if ( ! rec )
        return 1;

    // Update username (other columns will have default values)
    //
    dbUpdateStr( &rec->NAME, username );

    // Adding default authorization to USERAUTHS_TABLE
    //
    rid = dbAddRow( didUsers, USERAUTHS_TABLE );
    if ( rid < 0 )
    {
        ejSetResult( eid, "Cannot add default authorization! " );
        return 1;
    }

    USERAUTH* rec2 = dbGetRowPtr( didUsers, USERAUTHS_TABLE, rid );
    if ( ! rec2 )
        return 1;

    // Update username
    //
    dbUpdateStr( &rec2->USERNAME, username );

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );
    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    ejSetResult( eid, "Added successfully!" );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
/* deleteUser( username ) - deleting user and all his authorizations*/

static int deleteUser( int eid, webs_t wp, int argc, char_t** argv )
{
    int rid; // current row identifier
    int nrow; // number of rows
    int rc; // return code 
    char* NAME=NULL;    
    char_t* username;

    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
        return 0; // argument count is wrong

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0 );

    rid = dbSearchStr( didUsers, USERS_TABLE, "NAME", username, 0 ); 
    if ( rid < 0 )
    {
        ejSetResult( eid,"User does not exist!" );
        return 0;
    }

    // delete user
    dbDeleteRow( didUsers, USERS_TABLE, rid ); 

    nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get number of rows
    for (rid=0; rid<nrow; rid++ )
    {
        rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME );
        if ( rc == DB_OK && strcmp( NAME, username ) == 0 )
        
             // deleting all users authorizations
            dbDeleteRow( didUsers, USERAUTHS_TABLE, rid );
    }

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    ejSetResult( eid, "Deleted successfully!" );

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/* saveUsers() - saving all users in USERS_TABLE */

static int saveUsers( int eid, webs_t wp, int argc, char_t** argv )
{
    int rc; // return code from dbXX function
    int rid; // current row identifier
    int i;
    char* PASS_TEMP;
    char* GROUP_TEMP;
 
    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    // save users
    //
    for ( i = 0 ;; i++ )
    {
        char field[ 32 ];
        char* NAME = NULL;
      
        sprintf( field, "NAME%d",i );
        NAME = websGetVar( wp, field, "" ); 
        if ( ! NAME || NAME[0] == '\0' )
              break;

        rid = dbSearchStr( didUsers, USERS_TABLE, "NAME", NAME, 0 ); 
        if ( rid < 0 )
              break;

        // Save Password
        //
        sprintf( field, "PASS%d", i );
        PASS_TEMP = websGetVar( wp, field, "" );
        PASS_TEMP = trim( PASS_TEMP );
 
        rc = dbWriteStr( didUsers, USERS_TABLE, "PASS", rid, PASS_TEMP );

        // Save Group 
        //
        sprintf( field, "GROUP%d", i );
        GROUP_TEMP = websGetVar( wp, field, "" );
        GROUP_TEMP = trim( GROUP_TEMP );

        rc = dbWriteStr( didUsers, USERS_TABLE, "GROUP", rid, GROUP_TEMP );

        // Save DISABLED
        //
        sprintf( field, "DISABLED%d", i );
            rc = dbWriteInt( didUsers, USERS_TABLE, "DISABLED", rid, 
            strcmp( websGetVar( wp, field, "" ), "true" ) == 0 ? 1 : 0 
            );

        if ( isAlbaMode( "VR" ) && strcmp( NAME, "voice" ) == 0 )
        {
            char cmd[ 128 ];
            if ( strcmp( ALBA_PLATFORM, "arm" ) == 0 )
            {
                sprintf( cmd, "( echo \"%s\"; echo \"%s\" ) | smbpasswd -s %s &", PASS_TEMP, PASS_TEMP, NAME );
            }
            else
            {
                sprintf( cmd, "smbpasswd %s %s &", NAME, PASS_TEMP );
            }
            system( cmd );
        }
    }    

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    ejSetResult( eid, "Saved successfully!" );

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
/* listAuth( username ) - prints all authorizations for user */

static int listAuthforUser( int eid, webs_t wp, int argc, char_t **argv )
{
    int rc; // return code from dbXX function
    int rid; // current row identificator
    int nrow; // number of rows
    char* NAME=NULL;
    int i=0;
    char_t* username;

    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
        return 0; // argument count is wrong

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get number of rows

    websWrite( wp, "<table border=0 cellspacing=2 cellpadding=2 >"
        "<tbody align=center><tr bgcolor=#D0D0C0>"
        "<td><b>Directory&nbsp;Number</b></td><td><b>Mode</b></td></tr>" );

    // show users auth
    //
    for ( rid=0; rid<nrow; rid++ )
    {
        rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME );
    
        char* DN = NULL;
        int MODE = 0;
        if ( ( rc == DB_OK ) && strcmp( NAME, username ) == 0 )
        {
                // Print DN
                //
                rc = dbReadStr( didUsers, USERAUTHS_TABLE, "DN", rid, &DN );
                websWrite( wp, "<TR bgcolor=#F0F0F0>"
                    "<TD align=left>"
                    "<INPUT type=hidden name=DN%d id=DN value='%s' size=15>"
                    "<font onclick=\"selAuth('%s');\" style=\"cursor : pointer\">"
                    "<b>%s</b></font></td>", 
                    i, DN ? DN : "", DN ? DN : "", DN ? DN : "" 
                    );

                // Print MODE
                //
                rc = dbReadInt( didUsers, USERAUTHS_TABLE, "MODE", rid, &MODE );
                websWrite( wp, "<TD><SELECT name=MODE%d>",i );
                websWrite( wp, "<option value=0%s>User</option>", 
                                MODE == 0 ? " selected" : "" );
                websWrite( wp, "<option value=1%s>Admin</option>",  
                                MODE == 1 ? " selected" : "" );
                websWrite( wp, "<option value=2%s>Monitor</option>",
                                 MODE == 2 ? " selected" : "" );
            
                websWrite( wp, "</SELECT></TD></TR>" );
                ++i;
        }
    }

    websWrite( wp, "</tbody><tr><td height=10></td></tr></table>" );

    if ( i == 0 )
        ejSetResult( eid, "User does not exist!" );
    else 
        ejSetResult( eid, "1" );

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/* delAuthforuser ( username ) - deleting authorization for user */

static int delAuthforUser( int eid, webs_t wp, int argc, char_t **argv )
{
    int rc; // return code form dbXX function 
    int rid; // current row identifier
    int nrow; // number of rows
    char* DN=NULL;
    char* DN_temp = NULL;
    char* NAME=NULL;
    int found=0;
    char_t* username;
 
    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
        return 0; // argument count is wrong

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get number of rows

    for ( rid=0; rid<nrow; rid++ )
    {
        rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME );
            
        if ( rc == DB_OK && strcmp( NAME, username ) == 0 )
        {
            DN =  websGetVar( wp, "dn", "" );
            rc = dbReadStr( didUsers, USERAUTHS_TABLE, "DN", rid, &DN_temp );
            if ( rc == DB_OK && strcmp( DN, DN_temp ) == 0 )
            {
                dbDeleteRow( didUsers, USERAUTHS_TABLE, rid );
                found = 1;
            }
        }
    }

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    if ( found == 0 )
        ejSetResult(eid, "Directory Number is not correct!" );
    else 
        ejSetResult(eid, "Deleted successfully!" );

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/* addAuthforUser( username ) - adds authorization for user */

static int addAuthforUser( int eid, webs_t wp, int argc, char_t **argv )
{
    int rc; // return code from dbXX function
    int rid; // current row identifier
    int nrow; // number of rows
    int i;
    char* DN=NULL;
    char* DN_temp=NULL;
    char* NAME=NULL;
    int found=0;
    char_t* username;

    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
        return 0; // argument count is wrong

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get numer of rows

    DN =  websGetVar( wp, "dn", "" );
    DN = trim( DN );
    
    for ( i = 0; DN[ i ] != '\0'; i++ )
    {
        DN[ i ] = toupper( DN[ i ] ); 
    }

    for ( rid=0; rid<nrow; rid++ )
    {
        rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME );
            
        if ( rc == DB_OK && strcmp( NAME, username ) == 0 )
        {
            rc = dbReadStr( didUsers, USERAUTHS_TABLE, "DN", rid, &DN_temp );
            if ( rc == DB_OK && strcmp( DN, DN_temp ) == 0 )
                found = 1;
        }
    }
    if ( found == 0 )
    {
        rid = dbAddRow( didUsers, USERAUTHS_TABLE );
        dbWriteStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, username );
        dbWriteStr( didUsers, USERAUTHS_TABLE, "DN", rid, DN );
        dbWriteInt( didUsers, USERAUTHS_TABLE, "MODE", rid, 0 );
        ejSetResult(eid, "Added successfully!" );
    }    
    else 
    {
        ejSetResult( eid, "Authorization for this DN already exist!" );
    }

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///
/* saveAuthforUser( username ) - saves all authorization for user */

static int saveAuthforUser( int eid, webs_t wp, int argc, char_t **argv )
{
    int rc; // return code from dbXX function
    int rid; // current row identifier
    int nrow; // number of rows
    char* NAME=NULL;
    char field[ 32 ];
    int MODE=0;
    int i=0;
    char_t* username;

    if ( ejArgs( argc, argv, T("%s"), &username ) < 1 ) 
        return 0; // argument count is wrong

    dbZero( didUsers ); // make db empty (not to have row duplicates)
    dbLoad( didUsers, USERS_DB_NAME, 0  );

    nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get number of rows

    for (rid = 0, i = 0; rid < nrow; rid++ )
    {
        rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME);
        if ( ( rc == DB_OK ) && strcmp( NAME, username ) == 0 )
        {
            sprintf( field,"DN%d", i );
            dbWriteStr( didUsers, USERAUTHS_TABLE, "DN", 
                        rid, websGetVar( wp, field, "" ) );
            sprintf( field,"MODE%d", i );
            sscanf( websGetVar( wp, field, "" ), "%d", &MODE );

            dbWriteInt( didUsers, USERAUTHS_TABLE, "MODE", rid, MODE );
        
            ++i;
        }
    }

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );

    if ( rc )
    {
        ejSetResult( eid, "Failed to save database! " );
        return 1;
    }

    ejSetResult( eid, "Saved successfully!" );

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/************************** Code for Initialization ***************************/
//Defines all available ASP functions

void albaDbDefineFunctions(void)
{
    // Gateway Ports Database
    didGwPorts = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didGwPorts, &gatewayPortTable); 
    basicSetProductDir( didGwPorts, ALBA_PRODUCT_DIR );

    // Extender Ports Database
    didExtPorts = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didExtPorts, &extenderPortTable);
    basicSetProductDir( didExtPorts, ALBA_PRODUCT_DIR );

    // Users & Authorizations Database
    didUsers = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didUsers, &userTable); // Users
    dbRegisterDBSchema( didUsers, &userAuthTable); // Authorizations
    basicSetProductDir( didUsers, ALBA_PRODUCT_DIR );

    websAspDefine(T("listGatewayPorts"), listGatewayPorts);
    websAspDefine(T("saveGatewayPorts"), saveGatewayPorts);
    websAspDefine(T("listExtenderPorts"), listExtenderPorts);
    websAspDefine(T("listExtPortsMisc"), listExtPortsMisc);
    websAspDefine(T("saveExtenderPorts"), saveExtenderPorts);
    websAspDefine(T("listUsers"), listUsers);
    websAspDefine(T("addUser"), addUser);
    websAspDefine(T("deleteUser"), deleteUser);
    websAspDefine(T("saveUsers"), saveUsers);
    websAspDefine(T("listAuthforUser"), listAuthforUser);
    websAspDefine(T("delAuthforUser"), delAuthforUser);
    websAspDefine(T("addAuthforUser"), addAuthforUser);
    websAspDefine(T("saveAuthforUser"), saveAuthforUser);
}

/******************************************************************************/
