/***************************************************************************
 * Mud Telopt Handler 1.3 by Igor van den Hoven.               06 Apr 2009 *
 ***************************************************************************/

#include <string.h>
#include <stdarg.h>

#include "mud.h"
#include "telnet.h"

/* Externals */
bool write_to_descriptor( DESCRIPTOR_DATA *d, const char *txt, int length );
size_t skill_package_table_size( void );
extern int top_area;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;

#define TELOPT_DEBUG 1
#define ANNOUNCE_WILL   BV01
#define ANNOUNCE_DO     BV02

const char iac_will_ttype[]       = { IAC, WILL, TELOPT_TTYPE, 0 };
const char iac_sb_ttype_is[]      = { IAC, SB,   TELOPT_TTYPE, ENV_IS, 0 };
const char iac_sb_naws[]          = { IAC, SB,   TELOPT_NAWS, 0 };
const char iac_will_new_environ[] = { IAC, WILL, TELOPT_NEW_ENVIRON, 0 };
const char iac_sb_new_environ[]   = { IAC, SB,   TELOPT_NEW_ENVIRON, ENV_IS,0 };
const char iac_do_mssp[]          = { IAC, DO,   TELOPT_MSSP, 0 };
const char iac_do_mccp[]          = { IAC, DO,   TELOPT_MCCP, 0 };
const char iac_dont_mccp[]        = { IAC, DONT, TELOPT_MCCP, 0 };

/* Exported functions */
int translate_telopts( DESCRIPTOR_DATA *d, char *src, int srclen, char *out );
void announce_support( DESCRIPTOR_DATA *d );
int start_compress( DESCRIPTOR_DATA *d );
void end_compress( DESCRIPTOR_DATA *d );
void write_compressed( DESCRIPTOR_DATA *d );
void send_echo_on( DESCRIPTOR_DATA *d );
void send_echo_off( DESCRIPTOR_DATA *d );

/* Private functions */
static void debug_telopts( DESCRIPTOR_DATA *d, unsigned char *src,
			   int srclen );
static int process_will_ttype( DESCRIPTOR_DATA *d, unsigned char *src,
			       int srclen );
static int process_sb_ttype_is( DESCRIPTOR_DATA *d, unsigned char *src,
				int srclen );
static int process_sb_naws( DESCRIPTOR_DATA *d, unsigned char *src,
			    int srclen );
static int process_will_new_environ( DESCRIPTOR_DATA *d, unsigned char *src,
				     int srclen );
static int process_sb_new_environ( DESCRIPTOR_DATA *d, unsigned char *src,
				   int srclen );
static int process_do_mssp( DESCRIPTOR_DATA *d, unsigned char *src,
			    int srclen );
static int process_do_mccp( DESCRIPTOR_DATA *d, unsigned char *src,
			    int srclen );
static int process_dont_mccp( DESCRIPTOR_DATA *d, unsigned char *src,
			      int srclen );
static int skip_sb( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
static void descriptor_printf( DESCRIPTOR_DATA *d, const char *fmt, ...);
static void process_compressed( DESCRIPTOR_DATA *d );

struct telopt_type
{
  int      size;
  const char   * code;
  int   (* func) (DESCRIPTOR_DATA *d, unsigned char *src, int srclen);
};

const struct telopt_type telopt_table [] =
{
  { 3, iac_will_ttype,       &process_will_ttype},
  { 4, iac_sb_ttype_is,      &process_sb_ttype_is},

  { 3, iac_sb_naws,          &process_sb_naws},

  { 3, iac_will_new_environ, &process_will_new_environ},
  { 4, iac_sb_new_environ,   &process_sb_new_environ},

  { 3, iac_do_mssp,          &process_do_mssp},

  { 3, iac_do_mccp,          &process_do_mccp},
  { 3, iac_dont_mccp,        &process_dont_mccp},

  { 0, NULL,                 NULL}
};

const char *telcmds[] =
  {
    "EOF",   "SUSP",  "ABORT", "EOR",   "SE",
    "NOP",   "DMARK", "BRK",   "IP",    "AO",
    "AYT",   "EC",    "EL",    "GA",    "SB",
    "WILL",  "WONT",  "DO",    "DONT",  "IAC"
  };

struct telnet_type
{
  const char      *name;
  int       flags;
};

struct telnet_type telnet_table[] =
  {
    {    "BINARY",                0 },
    {    "ECHO",                  0 }, /* Local echo */
    {    "RCP",                   0 },
    {    "SUPPRESS GA",           0 }, /* Character mode */
    {    "NAME",                  0 },
    {    "STATUS",                0 },
    {    "TIMING MARK",           0 },
    {    "RCTE",                  0 },
    {    "NAOL",                  0 },
    {    "NAOP",                  0 },
    {    "NAORCD",                0 },
    {    "NAOHTS",                0 },
    {    "NAOHTD",                0 },
    {    "NAOFFD",                0 },
    {    "NAOVTS",                0 },
    {    "NAOVTD",                0 },
    {    "NAOLFD",                0 },
    {    "EXTEND ASCII",          0 },
    {    "LOGOUT",                0 },
    {    "BYTE MACRO",            0 },
    {    "DATA ENTRY TERML",      0 },
    {    "SUPDUP",                0 },
    {    "SUPDUP OUTPUT",         0 },
    {    "SEND LOCATION",         0 },
    {    "TERMINAL TYPE",         ANNOUNCE_DO }, /* Terminal Type */
    {    "EOR",                   0 }, /* End of Record */
    {    "TACACS UID",            0 },
    {    "OUTPUT MARKING",        0 },
    {    "TTYLOC",                0 },
    {    "3270 REGIME",           0 },
    {    "X.3 PAD",               0 },
    {    "NAWS",                  ANNOUNCE_DO }, /* Negotiate About Window Size */
    {    "TSPEED",                0 },
    {    "LFLOW",                 0 },
    {    "LINEMODE",              0 },
    {    "XDISPLOC",              0 },
    {    "OLD_ENVIRON",           0 },
    {    "AUTH",                  0 },
    {    "ENCRYPT",               0 },
    {    "NEW_ENVIRON",           ANNOUNCE_DO }, /* Used to detect Win32 telnet */
    {    "TN3270E",               0 },
    {    "XAUTH",                 0 },
    {    "CHARSET",               0 },
    {    "RSP",                   0 },
    {    "COM PORT",              0 },
    {    "SLE",                   0 },
    {    "STARTTLS",              0 },
    {    "KERMIT",                0 },
    {    "SEND-URL",              0 },
    {    "FORWARD_X",             0 },
    {    "50",                    0 },
    {    "51",                    0 },
    {    "52",                    0 },
    {    "53",                    0 },
    {    "54",                    0 },
    {    "55",                    0 },
    {    "56",                    0 },
    {    "57",                    0 },
    {    "58",                    0 },
    {    "59",                    0 },
    {    "60",                    0 },
    {    "61",                    0 },
    {    "62",                    0 },
    {    "63",                    0 },
    {    "64",                    0 },
    {    "65",                    0 },
    {    "66",                    0 },
    {    "67",                    0 },
    {    "68",                    0 },
    {    "69",                    0 },
    {    "MSSP",                  ANNOUNCE_WILL },
    {    "71",                    0 },
    {    "72",                    0 },
    {    "73",                    0 },
    {    "74",                    0 },
    {    "75",                    0 },
    {    "76",                    0 },
    {    "77",                    0 },
    {    "78",                    0 },
    {    "79",                    0 },
    {    "80",                    0 },
    {    "81",                    0 },
    {    "82",                    0 },
    {    "83",                    0 },
    {    "84",                    0 },
    {    "MCCP1",                 0 }, /* Obsolete - don't use. */
    {    "MCCP2",                 ANNOUNCE_WILL }, /* Mud Client Compression Protocol */
    {    "87",                    0 },
    {    "88",                    0 },
    {    "89",                    0 },
    {    "MSP",                   0 },
    {    "MXP",                   0 },
    {    "MSP2",                  0 }, /* Unadopted */
    {    "ZMP",                   0 }, /* Unadopted */
    {    "94",                    0 },
    {    "95",                    0 },
    {    "96",                    0 },
    {    "97",                    0 },
    {    "98",                    0 },
    {    "99",                    0 },
    {    "100",                   0 },
    {    "101",                   0 },
    {    "102",                   0 }, /* Aardwolf */
    {    "103",                   0 },
    {    "104",                   0 },
    {    "105",                   0 },
    {    "106",                   0 },
    {    "107",                   0 },
    {    "108",                   0 },
    {    "109",                   0 },
    {    "110",                   0 },
    {    "111",                   0 },
    {    "112",                   0 },
    {    "113",                   0 },
    {    "114",                   0 },
    {    "115",                   0 },
    {    "116",                   0 },
    {    "117",                   0 },
    {    "118",                   0 },
    {    "119",                   0 },
    {    "120",                   0 },
    {    "121",                   0 },
    {    "122",                   0 },
    {    "123",                   0 },
    {    "124",                   0 },
    {    "125",                   0 },
    {    "126",                   0 },
    {    "127",                   0 },
    {    "128",                   0 },
    {    "129",                   0 },
    {    "130",                   0 },
    {    "131",                   0 },
    {    "132",                   0 },
    {    "133",                   0 },
    {    "134",                   0 },
    {    "135",                   0 },
    {    "136",                   0 },
    {    "137",                   0 },
    {    "138",                   0 },
    {    "139",                   0 },
    {    "140",                   0 },
    {    "141",                   0 },
    {    "142",                   0 },
    {    "143",                   0 },
    {    "144",                   0 },
    {    "145",                   0 },
    {    "146",                   0 },
    {    "147",                   0 },
    {    "148",                   0 },
    {    "149",                   0 },
    {    "150",                   0 },
    {    "151",                   0 },
    {    "152",                   0 },
    {    "153",                   0 },
    {    "154",                   0 },
    {    "155",                   0 },
    {    "156",                   0 },
    {    "157",                   0 },
    {    "158",                   0 },
    {    "159",                   0 },
    {    "160",                   0 },
    {    "161",                   0 },
    {    "162",                   0 },
    {    "163",                   0 },
    {    "164",                   0 },
    {    "165",                   0 },
    {    "166",                   0 },
    {    "167",                   0 },
    {    "168",                   0 },
    {    "169",                   0 },
    {    "170",                   0 },
    {    "171",                   0 },
    {    "172",                   0 },
    {    "173",                   0 },
    {    "174",                   0 },
    {    "175",                   0 },
    {    "176",                   0 },
    {    "177",                   0 },
    {    "178",                   0 },
    {    "179",                   0 },
    {    "180",                   0 },
    {    "181",                   0 },
    {    "182",                   0 },
    {    "183",                   0 },
    {    "184",                   0 },
    {    "185",                   0 },
    {    "186",                   0 },
    {    "187",                   0 },
    {    "188",                   0 },
    {    "189",                   0 },
    {    "190",                   0 },
    {    "191",                   0 },
    {    "192",                   0 },
    {    "193",                   0 },
    {    "194",                   0 },
    {    "195",                   0 },
    {    "196",                   0 },
    {    "197",                   0 },
    {    "198",                   0 },
    {    "199",                   0 },
    {    "200",                   0 }, /* Achaea */
    {    "201",                   0 },
    {    "202",                   0 },
    {    "203",                   0 },
    {    "204",                   0 },
    {    "205",                   0 },
    {    "206",                   0 },
    {    "207",                   0 },
    {    "208",                   0 },
    {    "209",                   0 },
    {    "210",                   0 },
    {    "211",                   0 },
    {    "212",                   0 },
    {    "213",                   0 },
    {    "214",                   0 },
    {    "215",                   0 },
    {    "216",                   0 },
    {    "217",                   0 },
    {    "218",                   0 },
    {    "219",                   0 },
    {    "220",                   0 },
    {    "221",                   0 },
    {    "222",                   0 },
    {    "223",                   0 },
    {    "224",                   0 },
    {    "225",                   0 },
    {    "226",                   0 },
    {    "227",                   0 },
    {    "228",                   0 },
    {    "229",                   0 },
    {    "230",                   0 },
    {    "231",                   0 },
    {    "232",                   0 },
    {    "233",                   0 },
    {    "234",                   0 },
    {    "235",                   0 },
    {    "236",                   0 },
    {    "237",                   0 },
    {    "238",                   0 },
    {    "239",                   0 },
    {    "240",                   0 },
    {    "241",                   0 },
    {    "242",                   0 },
    {    "243",                   0 },
    {    "244",                   0 },
    {    "245",                   0 },
    {    "246",                   0 },
    {    "247",                   0 },
    {    "248",                   0 },
    {    "249",                   0 },
    {    "250",                   0 },
    {    "251",                   0 },
    {    "252",                   0 },
    {    "253",                   0 },
    {    "254",                   0 },
    {    "255",                   0 }
  };

/*
  Call this to announce support for telopts marked as such in tables.c
*/

void announce_support( DESCRIPTOR_DATA *d)
{
  int i = 0;

  for (i = 0 ; i < 255 ; i++)
    {
      if (telnet_table[i].flags)
	{
	  if (IS_SET(telnet_table[i].flags, ANNOUNCE_WILL))
	    {
	      descriptor_printf(d, "%c%c%c", IAC, WILL, i);
	    }

	  if (IS_SET(telnet_table[i].flags, ANNOUNCE_DO))
	    {
	      descriptor_printf(d, "%c%c%c", IAC, DO, i);
	    }
	}
    }
}

/*
  This is the main routine that strips out and handles telopt negotiations.
  It also deals with \r and \0 so commands are separated by a single \n.
*/

int translate_telopts(DESCRIPTOR_DATA *d, char *src, int srclen, char *out)
{
  int cnt = 0, skip = 0;
  unsigned char *pti = NULL, *pto = NULL;

  pti = (unsigned char*) src;
  pto = (unsigned char*) out;

  if (d->teltop)
    {
      if (d->teltop + srclen + 1 < MAX_INPUT_LENGTH)
	{
	  memcpy(d->telbuf + d->teltop, src, srclen);
	  srclen += d->teltop;
	  pti = (unsigned char*) d->telbuf;
	}
      d->teltop = 0;
    }

  while (srclen > 0)
    {
      switch (*pti)
	{
	case IAC:
	  skip = 2;
	  debug_telopts(d, pti, srclen);

	  for (cnt = 0 ; telopt_table[cnt].code ; cnt++)
	    {
	      if (srclen < telopt_table[cnt].size)
		{
		  if (!memcmp(pti, telopt_table[cnt].code, srclen))
		    {
		      skip = telopt_table[cnt].size;
		      break;
		    }
		}
	      else
		{
		  if (!memcmp(pti, telopt_table[cnt].code, telopt_table[cnt].size))
		    {
		      skip = telopt_table[cnt].func(d, pti, srclen);
		      break;
		    }
		}
	    }

	  if (telopt_table[cnt].code == NULL && srclen > 1)
	    {
	      switch (pti[1])
		{
		case WILL:
		case DO:
		case WONT:
		case DONT:
		  skip = 3;
		  break;

		case SB:
		  skip = skip_sb(d, pti, srclen);
		  break;

		case IAC:
		  *pto++ = *pti++;
		  srclen--;
		  skip = 1;
		  break;

		default:
		  if (TELCMD_OK(pti[1]))
		    {
		      skip = 2;
		    }
		  else
		    {
		      skip = 1;
		    }
		  break;
		}
	    }

	  if (skip <= srclen)
	    {
	      pti += skip;
	      srclen -= skip;
	    }
	  else
	    {
	      memcpy(d->telbuf, pti, srclen);
	      d->teltop = srclen;

	      *pto = 0;
	      return strlen(out);
	    }
	  break;

	case '\r':
	  if (srclen > 1 && pti[1] == '\0')
	    {
	      *pto++ = '\n';
	    }
	  pti++;
	  srclen--;
	  break;

	case '\0':
	  pti++;
	  srclen--;
	  break;

	default:
	  *pto++ = *pti++;
	  srclen--;
	  break;
	}
    }
  *pto = 0;

  return strlen(out);
}

void debug_telopts( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
#if SWR2_DEBUG_TELNET == 1
  if (srclen > 1 && TELOPT_DEBUG)
    {
      switch(src[1])
	{
	case IAC:
	  log_printf("D%d@%s RCVD IAC IAC", d->descriptor, d->host);
	  break;

	case DO:
	case DONT:
	case WILL:
	case WONT:
	case SB:
	  if (srclen > 2)
	    {
	      log_printf("D%d@%s RCVD IAC %s %s", d->descriptor, d->host, TELCMD(src[1]), TELOPT(src[2]));
	    }
	  else
	    {
	      log_printf("D%d@%s RCVD IAC %s ?", d->descriptor, d->host, TELCMD(src[1]));
	    }
	  break;

	default:
	  if (TELCMD_OK(src[1]))
	    {
	      log_printf("D%d@%s RCVD IAC %s", d->descriptor, d->host, TELCMD(src[1]));
	    }
	  else
	    {
	      log_printf("D%d@%s RCVD IAC %d", d->descriptor, d->host, src[1]);
	    }
	  break;
	}
    }
  else
    {
      log_printf("D%d@%s RCVD IAC ?", d->descriptor, d->host);
    }
#endif
}

/*
  Send to client to have it disable local echo
*/
void send_echo_off( DESCRIPTOR_DATA *d )
{
  descriptor_printf(d, "%c%c%c", IAC, WILL, TELOPT_ECHO);
}

/*
  Send to client to have it enable local echo
*/
void send_echo_on( DESCRIPTOR_DATA *d )
{
  descriptor_printf(d, "%c%c%c", IAC, WONT, TELOPT_ECHO);
}

/*
  Terminal Type negotiation - make sure d->terminal_type is initialized.
*/
int process_will_ttype( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  if (*d->terminal_type == 0)
    {
      descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
    }
  return 3;
}

int process_sb_ttype_is( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  char val[MAX_INPUT_LENGTH];
  char *pto = val;
  int i = 0;

  if (skip_sb(d, src, srclen) > srclen)
    {
      return srclen + 1;
    }

  for (i = 4 ; i < srclen && src[i] != SE ; i++)
    {
      switch (src[i])
	{
	default:			
	  *pto++ = src[i];
	  break;

	case IAC:
	  *pto = 0;
	  STRFREE( d->terminal_type );
	  d->terminal_type = STRALLOC( val );
	  break;
	}
    }
  return i + 1;
}

/*
  NAWS: Negotiate About Window Size
*/
int process_sb_naws( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  int i, j;

  d->cols = d->rows = 0;

  if (skip_sb(d, src, srclen) > srclen)
    {
      return srclen + 1;
    }

  for (i = 3, j = 0 ; i < srclen && j < 4 ; i++, j++)
    {
      switch (j)
	{
	case 0:
	  d->cols += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
	  break;
	case 1:
	  d->cols += (src[i] == IAC) ? src[i++] : src[i];
	  break;
	case 2:
	  d->rows += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
	  break;
	case 3:
	  d->rows += (src[i] == IAC) ? src[i++] : src[i];
	  break;
	}
    }

  return skip_sb(d, src, srclen);
}

/*
  NEW ENVIRON, used here to discover Windows telnet.
*/
int process_will_new_environ( DESCRIPTOR_DATA *d, unsigned char *src,
			      int srclen )
{
  descriptor_printf(d, "%c%c%c%c%c%s%c%c", IAC, SB, TELOPT_NEW_ENVIRON,
		    ENV_SEND, ENV_VAR, "SYSTEMTYPE", IAC, SE);

  return 3;
}

int process_sb_new_environ( DESCRIPTOR_DATA *d, unsigned char *src,
			    int srclen )
{
  char var[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
  char *pto = NULL;
  int i = 0;

  if (skip_sb(d, src, srclen) > srclen)
    {
      return srclen + 1;
    }

  var[0] = val[0] = 0;
  i = 4;

  while (i < srclen && src[i] != SE)
    {
      switch (src[i])
	{
	case ENV_VAR:
	case ENV_USR:
	  i++;
	  pto = var;

	  while (i < srclen && src[i] >= 32 && src[i] != IAC)
	    {
	      *pto++ = src[i++];
	    }
	  *pto = 0;
	  break;

	case ENV_VAL:
	  i++;
	  pto = val;

	  while (i < srclen && src[i] >= 32 && src[i] != IAC)
	    {
	      *pto++ = src[i++];
	    }
	  *pto = 0;

	  if (!str_cmp(var, "SYSTEMTYPE") && !str_cmp(val, "WIN32"))
	    {
	      STRFREE( d->terminal_type );
	      d->terminal_type = STRALLOC( "WIN32" );
	    }
	  break;

	default:
	  i++;
	  break;
	}
    }
  return i + 1;
}

/*
 * Data for MUD Server Status Protocol (MSSP).
 *
 * Please try following the established conventions as
 * outlined here: http://www.mudbytes.net/index.php?a=articles&s=MSSP_Fields
 */
static const char *MSSP_FIELD_MUD_NAME        = "SWR2 Refactor Devel";
static const char *MSSP_FIELD_HOSTNAME        = "swr2.totj.net";
static const char *MSSP_FIELD_IP              = "97.107.139.29";
static const char *MSSP_FIELD_CODEBASE        = "SWR2 Refactor";
static const char *MSSP_FIELD_CONTACT         = "kai@swr2.totj.net";
static const char *MSSP_FIELD_CREATED         = "2009";
static const char *MSSP_FIELD_ICON            = "";
static const char *MSSP_FIELD_LANGUAGE        = "English";
static const char *MSSP_FIELD_LOCATION        = "United States";
static const char *MSSP_FIELD_MINIMUM_AGE     = "13";
static const char *MSSP_FIELD_WEBSITE         = "http://swr2.totj.net";
static const char *MSSP_FIELD_FAMILY          = "DikuMUD";
static const char *MSSP_FIELD_GENRE           = "Science Fiction";
static const char *MSSP_FIELD_SUBGENRE        = "None";
static const char *MSSP_FIELD_GAMEPLAY        = "Player versus Player";
static const char *MSSP_FIELD_GAMESYSTEM      = "Custom";
static const char *MSSP_FIELD_INTERMUD        = "IMC2";
static const char *MSSP_FIELD_STATUS          = "Open Beta";
static const int   MSSP_FIELD_LEVELS          = 0;
static const int   MSSP_FIELD_RACES           = 1;
static const int   MSSP_FIELD_ANSI            = 1;
static const int   MSSP_FIELD_MCCP            = 1;
static const int   MSSP_FIELD_MCP             = 0;
static const int   MSSP_FIELD_MSP             = 0;
static const int   MSSP_FIELD_MXP             = 0;
static const int   MSSP_FIELD_PUEBLO          = 0;
static const int   MSSP_FIELD_VT100           = 0;
static const int   MSSP_FIELD_XTERM256        = 0;
static const int   MSSP_FIELD_PAY_TO_PLAY     = 0;
static const int   MSSP_FIELD_PAY_FOR_PERKS   = 0;
static const int   MSSP_FIELD_HIRING_BUILDERS = 0;
static const int   MSSP_FIELD_HIRING_CODERS   = 0;

extern int port;

/*
  MSSP: Mud Server Status Protocol
*/
int process_do_mssp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  char buffer[MAX_STRING_LENGTH] = { 0 };

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "NAME",
              MSSP_VAL, MSSP_FIELD_MUD_NAME);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PLAYERS",
	      MSSP_VAL, num_descriptors);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "UPTIME",
	      MSSP_VAL, boot_time);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "HOSTNAME",
	      MSSP_VAL, MSSP_FIELD_HOSTNAME);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PORT",
              MSSP_VAL, port);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CODEBASE",
	      MSSP_VAL, MSSP_FIELD_CODEBASE);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CONTACT",
	      MSSP_VAL, MSSP_FIELD_CONTACT);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CREATED",
	      MSSP_VAL, MSSP_FIELD_CREATED);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "ICON",
              MSSP_VAL, MSSP_FIELD_ICON);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "IP",
	      MSSP_VAL, MSSP_FIELD_IP);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LANGUAGE",
	      MSSP_VAL, MSSP_FIELD_LANGUAGE);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LOCATION",
	      MSSP_VAL, MSSP_FIELD_LOCATION);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "MINIMUM AGE",
	      MSSP_VAL, MSSP_FIELD_MINIMUM_AGE);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "WEBSITE",
	      MSSP_VAL, MSSP_FIELD_WEBSITE);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "FAMILY",
	      MSSP_VAL, MSSP_FIELD_FAMILY);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GENRE",
	      MSSP_VAL, MSSP_FIELD_GENRE);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMEPLAY",
	      MSSP_VAL, MSSP_FIELD_GAMEPLAY);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMESYSTEM",
	      MSSP_VAL, MSSP_FIELD_GAMESYSTEM);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "INTERMUD",
	      MSSP_VAL, MSSP_FIELD_INTERMUD);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "STATUS",
	      MSSP_VAL, MSSP_FIELD_STATUS);

  cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "SUBGENRE",
	      MSSP_VAL, MSSP_FIELD_SUBGENRE);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "AREAS",
	      MSSP_VAL, top_area);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HELPFILES",
	      MSSP_VAL, top_help);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MOBILES",
	      MSSP_VAL, top_mob_index);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "OBJECTS",
	      MSSP_VAL, top_obj_index);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ROOMS",
	      MSSP_VAL, top_room);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "CLASSES",
	      MSSP_VAL, skill_package_table_size() );

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "LEVELS",
	      MSSP_VAL, MSSP_FIELD_LEVELS );

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "RACES",
	      MSSP_VAL, MSSP_FIELD_RACES );

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "SKILLS",
	      MSSP_VAL, top_sn);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ANSI",
              MSSP_VAL, MSSP_FIELD_ANSI);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCCP",
              MSSP_VAL, MSSP_FIELD_MCCP);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCP",
	      MSSP_VAL, MSSP_FIELD_MCP);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MSP",
	      MSSP_VAL, MSSP_FIELD_MSP);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MXP",
	      MSSP_VAL, MSSP_FIELD_MXP);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PUEBLO",
	      MSSP_VAL, MSSP_FIELD_PUEBLO);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "VT100",
	      MSSP_VAL, MSSP_FIELD_VT100);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "XTERM 256 COLORS",
	      MSSP_VAL, MSSP_FIELD_XTERM256);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY TO PLAY",
	      MSSP_VAL, MSSP_FIELD_PAY_TO_PLAY);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY FOR PERKS",
	      MSSP_VAL, MSSP_FIELD_PAY_FOR_PERKS);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING BUILDERS",
	      MSSP_VAL, MSSP_FIELD_HIRING_BUILDERS);

  cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING CODERS",
	      MSSP_VAL, MSSP_FIELD_HIRING_CODERS);

  descriptor_printf(d, "%c%c%c%s%c%c", IAC, SB, TELOPT_MSSP, buffer, IAC, SE);

  return 3;
}

/*
  MCCP: Mud Client Compression Protocol
*/
void *zlib_alloc( void *opaque, unsigned int items, unsigned int size )
{
  return calloc(items, size);
}

void zlib_free( void *opaque, void *address ) 
{
  free(address);
}

int start_compress( DESCRIPTOR_DATA *d )
{
  const char start_mccp[] = { IAC, SB, TELOPT_MCCP, IAC, SE, 0 };
  z_stream *stream = NULL;

  if (d->mccp)
    {
      return TRUE;
    }

  CREATE(stream, z_stream, 1);

  stream->next_in	    = NULL;
  stream->avail_in    = 0;

  stream->next_out    = sysdata.mccp_buf;
  stream->avail_out   = COMPRESS_BUF_SIZE;

  stream->data_type   = Z_ASCII;
  stream->zalloc      = zlib_alloc;
  stream->zfree       = zlib_free;
  stream->opaque      = Z_NULL;

  /*
    12, 5 = 32K of memory, more than enough
  */

  if (deflateInit2(stream, Z_BEST_COMPRESSION, Z_DEFLATED, 12, 5, Z_DEFAULT_STRATEGY) != Z_OK)
    {
      log_printf("start_compress: failed deflateInit2 D%d@%s", d->descriptor, d->host);
      DISPOSE(stream);

      return FALSE;
    }

  write_to_descriptor(d, start_mccp, 0);

  /*
    The above call must send all pending output to the descriptor, since from now on we'll be compressing.
  */

  d->mccp = stream;

  return TRUE;
}

void end_compress( DESCRIPTOR_DATA *d )
{
  if (d->mccp == NULL)
    {
      return;
    }

  d->mccp->next_in	= NULL;
  d->mccp->avail_in	= 0;

  d->mccp->next_out	= sysdata.mccp_buf;
  d->mccp->avail_out	= COMPRESS_BUF_SIZE;

  if (deflate(d->mccp, Z_FINISH) != Z_STREAM_END)
    {
      log_printf("end_compress: failed to deflate D%d@%s", d->descriptor, d->host);
    }

  process_compressed(d);

  if (deflateEnd(d->mccp) != Z_OK)
    {
      log_printf("end_compress: failed to deflateEnd D%d@%s", d->descriptor, d->host);
    }

  DISPOSE(d->mccp);
  d->mccp = NULL;
}

void write_compressed( DESCRIPTOR_DATA *d )
{
  d->mccp->next_in    = (unsigned char*) d->outbuf;
  d->mccp->avail_in   = d->outtop;

  d->mccp->next_out   = sysdata.mccp_buf;
  d->mccp->avail_out  = COMPRESS_BUF_SIZE;

  d->outtop           = 0;

  if (deflate(d->mccp, Z_SYNC_FLUSH) != Z_OK)
    {
      return;
    }

  process_compressed(d);
}

void process_compressed( DESCRIPTOR_DATA *d )
{
  int length = COMPRESS_BUF_SIZE - d->mccp->avail_out;

  if (send(d->descriptor, sysdata.mccp_buf, length, 0) < 1)
    {
      log_printf("process_compressed D%d@%s", d->descriptor, d->host);
      return;
    }
}

int process_do_mccp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  start_compress(d);

  return 3;
}

int process_dont_mccp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  end_compress(d);

  return 3;
}

/*
  Returns the length of a telnet subnegotiation, return srclen + 1 for incomplete state.
*/
int skip_sb( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
  int i;

  for (i = 1 ; i < srclen ; i++)
    {
      if (src[i] == SE && src[i-1] == IAC)
	{
	  return i + 1;
	}
    }

  return srclen + 1;
}

/*
  Utility function
*/
void descriptor_printf( DESCRIPTOR_DATA *d, const char *fmt, ... )
{
  char buf[MAX_STRING_LENGTH];
  int size = 0;
  va_list args;

  va_start(args, fmt);

  size = vsprintf(buf, fmt, args);

  va_end(args);

  write_to_descriptor(d, buf, size);
}
