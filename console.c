/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2020 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// console.c

#if !defined(_WIN32) || defined(__MINGW32__)
	# include <unistd.h>
#endif
#include <time.h>

#include "quakedef.h"
#include "thread.h"

// for u8_encodech
#include "ft2.h"

float con_cursorspeed = 4;

// lines up from bottom to display
int con_backscroll; // Baker: This is # of console text lines of backscroll

conbuffer_t con;
void *con_mutex = NULL;

#define CON_LINES(i)		CONBUFFER_LINES(&con, i)
#define CON_LINES_LAST		CONBUFFER_LINES_LAST(&con)
#define CON_LINES_COUNT		CONBUFFER_LINES_COUNT(&con)

cvar_t con_notifytime = {CF_CLIENT | CF_ARCHIVE, "con_notifytime","3", "how long notify lines last, in seconds"};
cvar_t con_notify = {CF_CLIENT | CF_ARCHIVE, "con_notify","4", "how many notify lines to show"};
cvar_t con_notifyalign = {CF_CLIENT | CF_ARCHIVE, "con_notifyalign", "", "how to align notify lines: 0 = left, 0.5 = center, 1 = right, empty string = game default)"};

cvar_t con_chattime = {CF_CLIENT | CF_ARCHIVE, "con_chattime","30", "how long chat lines last, in seconds"};
cvar_t con_chat = {CF_CLIENT | CF_ARCHIVE, "con_chat","0", "how many chat lines to show in a dedicated chat area"};
cvar_t con_chatpos = {CF_CLIENT | CF_ARCHIVE, "con_chatpos","0", "where to put chat (negative: lines from bottom of screen, positive: lines below notify, 0: at top)"};
cvar_t con_chatrect = {CF_CLIENT | CF_ARCHIVE, "con_chatrect","0", "use con_chatrect_x and _y to position con_notify and con_chat freely instead of con_chatpos"};
cvar_t con_chatrect_x = {CF_CLIENT | CF_ARCHIVE, "con_chatrect_x","", "where to put chat, relative x coordinate of left edge on screen (use con_chatwidth for width)"};
cvar_t con_chatrect_y = {CF_CLIENT | CF_ARCHIVE, "con_chatrect_y","", "where to put chat, relative y coordinate of top edge on screen (use con_chat for line count)"};
cvar_t con_chatwidth = {CF_CLIENT | CF_ARCHIVE, "con_chatwidth","1.0", "relative chat window width"};
cvar_t con_hints = {CF_CLIENT | CF_ARCHIVE, "con_hints", "0", "seconds before showing console hints when holding CTRL or 0 to disable [Zircon]"}; // Baker r8085: ProQuake looking clock option
cvar_t con_textsize = {CF_CLIENT | CF_ARCHIVE, "con_textsize","8", "console text size in virtual 2D pixels"};
cvar_t con_notifysize = {CF_CLIENT | CF_ARCHIVE, "con_notifysize","8", "notify text size in virtual 2D pixels"};
cvar_t con_chatsize = {CF_CLIENT | CF_ARCHIVE, "con_chatsize","8", "chat text size in virtual 2D pixels (if con_chat is enabled)"};
cvar_t con_chatsound = {CF_CLIENT | CF_ARCHIVE, "con_chatsound","1", "enables chat sound to play on message"};
cvar_t con_logcenterprint = {CF_CLIENT | CF_ARCHIVE, "con_logcenterprint","1", "centerprint messages will be logged to the console in singleplayer.  If 2, they will also be logged in deathmatch"}; // Baker r1421: centerprint logging to console

cvar_t con_chatsound_file = {CF_CLIENT, "con_chatsound_file","sound/misc/talk.wav", "The sound to play for chat messages"};
cvar_t con_chatsound_team_file = {CF_CLIENT, "con_chatsound_team_file","sound/misc/talk2.wav", "The sound to play for team chat messages"};
cvar_t con_chatsound_team_mask = {CF_CLIENT, "con_chatsound_team_mask","40","Magic ASCII code that denotes a team chat message"};

cvar_t con_zircon_autocomplete = {CF_CLIENT | CF_ARCHIVE, "con_zircon_autocomplete","1", "autocomplete always completes first possible match in full, TAB goes to next match, SHIFT TAB to prior match [Zircon]"}; // Baker r0062: full auto-completion


static void Con_Pos_f(cmd_state_t *cmd)
{
	if (cl.entities) {
		char vabuf[1024];
		va(vabuf, sizeof(vabuf),
			QUOTED_STR("origin") " " QUOTED_STR("%3.0f %3.0f %3.0f") NEWLINE
			QUOTED_STR("angles") " " QUOTED_STR("%3.0f %3.0f %3.0f") NEWLINE,
			cl.entities[cl.playerentity].state_current.origin[0],
			cl.entities[cl.playerentity].state_current.origin[1],
			cl.entities[cl.playerentity].state_current.origin[2],
			cl.entities[cl.playerentity].state_current.angles[0],
			cl.entities[cl.playerentity].state_current.angles[1],
			cl.entities[cl.playerentity].state_current.angles[2]
			);
			Clipboard_Set_Text (vabuf); // AUTH: "pos"
			Con_PrintLinef ("pos to clipboard: " NEWLINE "%s", vabuf);
	} else {
		Con_PrintLinef ("No entities");
	}


}

#include "console_autocomplete.c.h"

void Con_LogCenterPrint (const char *str)
{
	if (String_Match(str, cl.lastcenterstring))
		return; //ignore duplicates

	if (cl.gametype == GAME_DEATHMATCH && con_logcenterprint.value < 2) // default 1
		return; //don't log in deathmatch

	strlcpy (cl.lastcenterstring, str, sizeof(cl.lastcenterstring));

	if (con_logcenterprint.value) {
		Con_HidenotifyPrintLinef ("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37");
		Con_HidenotifyPrintLinef ("%s", str);
		Con_HidenotifyPrintLinef ("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37");
	}

}

cvar_t sys_specialcharactertranslation = {CF_CLIENT | CF_SERVER, "sys_specialcharactertranslation", "1", "terminal console conchars to ASCII translation (set to 0 if your conchars.tga is for an 8bit character set or if you want raw output)"};
#ifdef _WIN32
cvar_t sys_colortranslation = {CF_CLIENT | CF_SERVER, "sys_colortranslation", "0", "terminal console color translation (supported values: 0 = strip color codes, 1 = translate to ANSI codes, 2 = no translation)"};
#else
cvar_t sys_colortranslation = {CF_CLIENT | CF_SERVER, "sys_colortranslation", "1", "terminal console color translation (supported values: 0 = strip color codes, 1 = translate to ANSI codes, 2 = no translation)"};
#endif


cvar_t con_nickcompletion = {CF_CLIENT | CF_ARCHIVE, "con_nickcompletion", "1", "tab-complete nicks in console and message input"};
cvar_t con_nickcompletion_flags = {CF_CLIENT | CF_ARCHIVE, "con_nickcompletion_flags", "11", "Bitfield: "
				   "0: add nothing after completion. "
				   "1: add the last color after completion. "
				   "2: add a quote when starting a quote instead of the color. "
				   "4: will replace 1, will force color, even after a quote. "
				   "8: ignore non-alphanumerics. "
				   "16: ignore spaces. "};
#define NICKS_ADD_COLOR 1
#define NICKS_ADD_QUOTE 2
#define NICKS_FORCE_COLOR 4
#define NICKS_ALPHANUMERICS_ONLY 8
#define NICKS_NO_SPACES 16

cvar_t con_completion_playdemo = {CF_CLIENT | CF_ARCHIVE, "con_completion_playdemo", "*.dem", "completion pattern for the playdemo command"};
cvar_t con_completion_timedemo = {CF_CLIENT | CF_ARCHIVE, "con_completion_timedemo", "*.dem", "completion pattern for the timedemo command"};
cvar_t con_completion_exec = {CF_CLIENT | CF_ARCHIVE, "con_completion_exec", "*.cfg", "completion pattern for the exec command"};

cvar_t condump_stripcolors = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "condump_stripcolors", "1", "strip color codes from console dumps [Zircon default]"};

cvar_t rcon_password = {CF_CLIENT | CF_SERVER | CF_PRIVATE, "rcon_password", "", "password to authenticate rcon commands; NOTE: changing rcon_secure clears rcon_password, so set rcon_secure always before rcon_password; may be set to a string of the form user1:pass1 user2:pass2 user3:pass3 to allow multiple user accounts - the client then has to specify ONE of these combinations"};
cvar_t rcon_secure = {CF_CLIENT | CF_SERVER, "rcon_secure", "0", "force secure rcon authentication (1 = time based, 2 = challenge based); NOTE: changing rcon_secure clears rcon_password, so set rcon_secure always before rcon_password"};
cvar_t rcon_secure_challengetimeout = {CF_CLIENT, "rcon_secure_challengetimeout", "5", "challenge-based secure rcon: time out requests if no challenge came within this time interval"};
cvar_t rcon_address = {CF_CLIENT, "rcon_address", "", "server address to send rcon commands to (when not connected to a server)"};

int con_linewidth;
int con_vislines;

qbool con_initialized;

// used for server replies to rcon command
lhnetsocket_t *rcon_redirect_sock = NULL;
lhnetaddress_t *rcon_redirect_dest = NULL;
int rcon_redirect_bufferpos = 0;
char rcon_redirect_buffer[1400];
qbool rcon_redirect_proquakeprotocol = false;

// Baker r0004: Ctrl + up/down size console like JoeQuake

float console_user_pct = 1.00; // Open console percent
#define		CONSOLE_MINIMUM_PCT_0_10	0.10
#define		CONSOLE_MAX_USER_PCT_2_00	2.00

void Con_AdjustConsoleHeight(const float delta)
{
	console_user_pct += (float)delta;
	console_user_pct = bound(CONSOLE_MINIMUM_PCT_0_10, console_user_pct, CONSOLE_MAX_USER_PCT_2_00);
}


// generic functions for console buffers

void ConBuffer_Init(conbuffer_t *buf, int textsize, int maxlines, mempool_t *mempool)
{
	buf->active = true;
	buf->textsize = textsize;
	buf->textcon = (char *) Mem_Alloc(mempool, textsize);
	buf->maxlines = maxlines;
	buf->clines = (con_lineinfo_t *) Mem_Alloc(mempool, maxlines * sizeof(*buf->clines));
	buf->lines_first = 0;
	buf->lines_count = 0;
}

/*! The translation table between the graphical font and plain ASCII  --KB */
static char qfont_table[256] = {
	'\0', '#',  '#',  '#',  '#',  '.',  '#',  '#',
	'#',  9,    10,   '#',  ' ',  13,   '.',  '.',
	'[',  ']',  '0',  '1',  '2',  '3',  '4',  '5',
	'6',  '7',  '8',  '9',  '.',  '<',  '=',  '>',
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '<',

	'<',  '=',  '>',  '#',  '#',  '.',  '#',  '#',
	'#',  '#',  ' ',  '#',  ' ',  '>',  '.',  '.',
	'[',  ']',  '0',  '1',  '2',  '3',  '4',  '5',
	'6',  '7',  '8',  '9',  '.',  '<',  '=',  '>',
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '<'
};

/*
	SanitizeString strips color tags from the string in
	and writes the result on string out
*/
static void SanitizeString(char *in, char *out)
{
	while(*in)
	{
		if (*in == STRING_COLOR_TAG)
		{
			++in;
			if (!*in)
			{
				out[0] = STRING_COLOR_TAG;
				out[1] = 0;
				return;
			}
			else if (*in >= '0' && *in <= '9') // ^[0-9] found
			{
				++in;
				if (!*in)
				{
					*out = 0;
					return;
				} else if (*in == STRING_COLOR_TAG) // ^[0-9]^ found, don't print ^[0-9]
					continue;
			}
			else if (*in == STRING_COLOR_RGB_TAG_CHAR) // ^x found
			{
				if ( isxdigit(in[1]) && isxdigit(in[2]) && isxdigit(in[3]) )
				{
					in+=4;
					if (!*in)
					{
						*out = 0;
						return;
					} else if (*in == STRING_COLOR_TAG) // ^xrgb^ found, don't print ^xrgb
						continue;
				}
				else in--;
			}
			else if (*in != STRING_COLOR_TAG)
				--in;
		}
		*out = qfont_table[*(unsigned char *)in];
		++in;
		++out;
	}
	*out = 0;
}

/*
================
ConBuffer_Clear
================
*/
void ConBuffer_Clear (conbuffer_t *buf)
{
	buf->lines_count = 0;
}

/*
================
ConBuffer_Shutdown
================
*/
void ConBuffer_Shutdown(conbuffer_t *buf)
{
	buf->active = false;
	if (buf->textcon)
		Mem_Free(buf->textcon);
	if (buf->clines)
		Mem_Free(buf->clines);
	buf->textcon = NULL;
	buf->clines = NULL;
}

/*
================
ConBuffer_FixTimes

Notifies the console code about the current time
(and shifts back times of other entries when the time
went backwards)
================
*/
void ConBuffer_FixTimes(conbuffer_t *buf)
{
	int i;
	if (buf->lines_count >= 1) {
		double diff = cl.time - CONBUFFER_LINES_LAST(buf).addtime;
		if (diff < 0) {
			for(i = 0; i < buf->lines_count; i++)
				CONBUFFER_LINES(buf, i).addtime += diff;
		} // if
	} // if
}

/*
================
ConBuffer_DeleteLine

Deletes the first line from the console history.
================
*/
void ConBuffer_DeleteLine(conbuffer_t *buf)
{
	if (buf->lines_count == 0)
		return;
	--buf->lines_count;
	buf->lines_first = (buf->lines_first + 1) % buf->maxlines;
}

/*
================
ConBuffer_DeleteLastLine

Deletes the last line from the console history.
================
*/
void ConBuffer_DeleteLastLine(conbuffer_t *buf)
{
	if (buf->lines_count == 0)
		return;
	--buf->lines_count;
}

/*
================
ConBuffer_BytesLeft

Checks if there is space for a line of the given length, and if yes, returns a
pointer to the start of such a space, and NULL otherwise.
================
*/
static char *ConBuffer_BytesLeft(conbuffer_t *buf, int len)
{
	if (len > buf->textsize)
		return NULL;
	if (buf->lines_count == 0)
		return buf->textcon;
	else
	{
		char *firstline_start = buf->clines[buf->lines_first].pstart;
		char *lastline_onepastend = CONBUFFER_LINES_LAST(buf).pstart + CONBUFFER_LINES_LAST(buf).line_strlen;
		// the buffer is cyclic, so we first have two cases...
		if (firstline_start < lastline_onepastend) // buffer is contiguous
		{
			// put at end?
			if (len <= buf->textcon + buf->textsize - lastline_onepastend)
				return lastline_onepastend;
			// put at beginning?
			else if (len <= firstline_start - buf->textcon)
				return buf->textcon;
			else
				return NULL;
		}
		else // buffer has a contiguous hole
		{
			if (len <= firstline_start - lastline_onepastend)
				return lastline_onepastend;
			else
				return NULL;
		}
	}
}

/*
================
ConBuffer_AddLine

Appends a given string as a new line to the console.
================
*/
void ConBuffer_AddLine(conbuffer_t *buf, const char *line, int len, int mask)
{
	char *putpos;
	con_lineinfo_t *p;

	// developer_memory 1 during shutdown prints while conbuffer_t is being freed
	if (!buf->active)
		return;

	ConBuffer_FixTimes(buf);

	if (len >= buf->textsize) {
		// line too large?
		// only display end of line.
		line += len - buf->textsize + 1;
		len = buf->textsize - 1;
	}
	while(!(putpos = ConBuffer_BytesLeft(buf, len + 1)) || buf->lines_count >= buf->maxlines)
		ConBuffer_DeleteLine(buf);
	memcpy(putpos, line, len);
	putpos[len] = 0;
	++buf->lines_count;

	//fprintf(stderr, "Now have %d lines (%d -> %d).\n", buf->lines_count, buf->lines_first, CON_LINES_LAST);

	p = &CONBUFFER_LINES_LAST(buf);
	p->pstart = putpos;
	p->line_strlen = len;
	p->addtime = cl.time;
	p->mask = mask;
	p->line_num_rows_height = -1; // calculate when needed
}

int ConBuffer_FindPrevLine(conbuffer_t *buf, int mask_must, int mask_mustnot, int start)
{
	int i;
	if (start == -1)
		start = buf->lines_count;
	for(i = start - 1; i >= 0; --i)
	{
		con_lineinfo_t *l = &CONBUFFER_LINES(buf, i);

		if ((l->mask & mask_must) != mask_must)
			continue;
		if (l->mask & mask_mustnot)
			continue;

		return i;
	}

	return -1;
}

const char *ConBuffer_GetLine(conbuffer_t *buf, int i)
{
	static char copybuf[MAX_INPUTLINE_16384]; // client only
	con_lineinfo_t *l = &CONBUFFER_LINES(buf, i);
	size_t sz = l->line_strlen+1 > sizeof(copybuf) ? sizeof(copybuf) : l->line_strlen+1;
	strlcpy(copybuf, l->pstart, sz);
	return copybuf;
}

/*
==============================================================================

LOGGING

==============================================================================
*/

/// \name Logging
//@{
cvar_t log_file = {CF_CLIENT | CF_SERVER, "log_file", "", "filename to log messages to"};
cvar_t log_file_stripcolors = {CF_CLIENT | CF_SERVER, "log_file_stripcolors", "1", "strip color codes from log messages [Zircon default]"}; // Baker r1403 log_file_stripcolors defaults 1 for legibility of console logs
cvar_t log_dest_udp = {CF_CLIENT | CF_SERVER, "log_dest_udp", "", "UDP address to log messages to (in QW rcon compatible format); multiple destinations can be separated by spaces; DO NOT SPECIFY DNS NAMES HERE"};
char log_dest_buffer[1400]; // UDP packet
size_t log_dest_buffer_pos;
unsigned int log_dest_buffer_appending;
char crt_log_file [MAX_OSPATH] = "";
qfile_t *logfile = NULL;

unsigned char *logqueue = NULL;
size_t logq_ind = 0;
size_t logq_size = 0;

void Log_ConPrint (const char *msg);
//@}
static void Log_DestBuffer_Init(void)
{
	memcpy(log_dest_buffer, "\377\377\377\377n", 5); // QW rcon print
	log_dest_buffer_pos = 5;
}

static void Log_DestBuffer_Flush_NoLock(void)
{
	lhnetaddress_t log_dest_addr;
	lhnetsocket_t *log_dest_socket;
	const char *s = log_dest_udp.string;
	qbool have_opened_temp_sockets = false;
	if (s) if (log_dest_buffer_pos > 5)
	{
		++log_dest_buffer_appending;
		log_dest_buffer[log_dest_buffer_pos++] = 0;

		if (!NetConn_HaveServerPorts() && !NetConn_HaveClientPorts()) // then temporarily open one
 		{
			have_opened_temp_sockets = true;
			NetConn_OpenServerPorts(true);
		}

		while(COM_ParseToken_Console(&s))
			if (LHNETADDRESS_FromString(&log_dest_addr, com_token, 26000))
			{
				log_dest_socket = NetConn_ChooseClientSocketForAddress(&log_dest_addr);
				if (!log_dest_socket)
					log_dest_socket = NetConn_ChooseServerSocketForAddress(&log_dest_addr);
				if (log_dest_socket)
					NetConn_WriteString(log_dest_socket, log_dest_buffer, &log_dest_addr);
			}

		if (have_opened_temp_sockets)
			NetConn_CloseServerPorts();
		--log_dest_buffer_appending;
	}
	log_dest_buffer_pos = 0;
}

/*
====================
Log_DestBuffer_Flush
====================
*/
void Log_DestBuffer_Flush(void)
{
	if (con_mutex)
		Thread_LockMutex(con_mutex);
	Log_DestBuffer_Flush_NoLock();
	if (con_mutex)
		Thread_UnlockMutex(con_mutex);
}

static const char *Log_Timestamp (const char *desc)
{
	static char timestamp [128]; // init/shutdown only
	time_t crt_time;
#if _MSC_VER >= 1400
	struct tm crt_tm;
#else
	struct tm *crt_tm;
#endif
	char timestring [64];

	// Build the time stamp (ex: "Wed Jun 30 21:49:08 1993");
	time (&crt_time);
#if _MSC_VER >= 1400
	localtime_s (&crt_tm, &crt_time);
	strftime (timestring, sizeof (timestring), "%a %b %d %H:%M:%S %Y", &crt_tm);
#else
	crt_tm = localtime (&crt_time);
	strftime (timestring, sizeof (timestring), "%a %b %d %H:%M:%S %Y", crt_tm);
#endif

	if (desc != NULL)
		dpsnprintf (timestamp, sizeof (timestamp), "====== %s (%s) ======\n", desc, timestring);
	else
		dpsnprintf (timestamp, sizeof (timestamp), "====== %s ======\n", timestring);

	return timestamp;
}

static void Log_Open (void)
{
	if (logfile != NULL || log_file.string[0] == '\0')
		return;

	logfile = FS_OpenRealFile(log_file.string, "a", fs_quiet_FALSE); // WRITE-EON debug log
	if (logfile != NULL)
	{
		strlcpy (crt_log_file, log_file.string, sizeof (crt_log_file));
		FS_Print (logfile, Log_Timestamp ("Log started"));
	}
}

/*
====================
Log_Close
====================
*/
void Log_Close (void)
{
	qfile_t *logfilecopy = logfile;

	if (logfile == NULL)
		return;

	FS_Print (logfilecopy, Log_Timestamp ("Log stopped"));
	FS_Print (logfilecopy, "\n");
	logfile = NULL;
	FS_Close (logfilecopy);

	crt_log_file[0] = '\0';
}


/*
====================
Log_Start
====================
*/
void Log_Start (void)
{
	size_t pos;
	size_t n;
	Log_Open ();

	// Dump the contents of the log queue into the log file and free it
	if (logqueue != NULL)
	{
		unsigned char *temp = logqueue;
		logqueue = NULL;
		if (logq_ind != 0)
		{
			if (logfile != NULL)
				FS_Write (logfile, temp, logq_ind);
			if (*log_dest_udp.string)
			{
				for(pos = 0; pos < logq_ind; )
				{
					if (log_dest_buffer_pos == 0)
						Log_DestBuffer_Init();
					n = min(sizeof(log_dest_buffer) - log_dest_buffer_pos - 1, logq_ind - pos);
					memcpy(log_dest_buffer + log_dest_buffer_pos, temp + pos, n);
					log_dest_buffer_pos += n;
					Log_DestBuffer_Flush_NoLock();
					pos += n;
				}
			}
		}
		Mem_Free (temp);
		logq_ind = 0;
		logq_size = 0;
	}
}



/*
================
Log_ConPrint
================
*/
void Log_ConPrint (const char *msg)
{
	static qbool inprogress = false;

	// don't allow feedback loops with memory error reports
	if (inprogress)
		return;
	inprogress = true;

	// Until the host is completely initialized, we maintain a log queue
	// to store the messages, since the log can't be started before
	if (logqueue != NULL)
	{
		size_t remain = logq_size - logq_ind;
		size_t len = strlen (msg);

		// If we need to enlarge the log queue
		if (len > remain)
		{
			size_t factor = ((logq_ind + len) / logq_size) + 1;
			unsigned char *newqueue;

			logq_size *= factor;
			newqueue = (unsigned char *)Mem_Alloc (tempmempool, logq_size);
			memcpy (newqueue, logqueue, logq_ind);
			Mem_Free (logqueue);
			logqueue = newqueue;
			remain = logq_size - logq_ind;
		}
		memcpy (&logqueue[logq_ind], msg, len);
		logq_ind += len;

		inprogress = false;
		return;
	}

	// Check if log_file has changed
	if (strcmp (crt_log_file, log_file.string) != 0)
	{
		Log_Close ();
		Log_Open ();
	}

	// If a log file is available
	if (logfile != NULL)
	{
		if (log_file_stripcolors.integer)
		{
			// sanitize msg
			size_t len = strlen(msg);
			char *sanitizedmsg = (char *)Mem_Alloc(tempmempool, len + 1);
			memcpy (sanitizedmsg, msg, len);
			SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
			FS_Print (logfile, sanitizedmsg);
			Mem_Free(sanitizedmsg);
		}
		else
		{
			FS_Print (logfile, msg);
		}
	}

	inprogress = false;
}


/*
================
Log_Printf
================
*/
void Log_Printf (const char *logfilename, const char *fmt, ...)
{
	qfile_t *file;

	file = FS_OpenRealFile(logfilename, "a", fs_quiet_true);
	if (file != NULL)
	{
		va_list argptr;

		va_start (argptr, fmt);
		FS_VPrintf (file, fmt, argptr);
		va_end (argptr);

		FS_Close (file);
	}
}


/*
==============================================================================

CONSOLE

==============================================================================
*/

// Baker: Where does key_dest get set to console?
void Con_ToggleConsole (void) // Method "kill" uses is Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.
{
	if (Sys_CheckParm ("-noconsole"))
		if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_USER_1) == false)
			return; // only allow the key bind to turn off console

	// toggle the 'user wants console' bit
	Flag_Toggle (key_consoleactive, KEY_CONSOLEACTIVE_USER_1); Consel_MouseReset("Con_ToggleConsole");

#if 1 // Baker 1013.1
	// Baker: Key_ClearEditLine returns 1, calls Partial_Reset_Undo_Selection_Reset, key_linepos to 1, clears text
	key_linepos = Key_ClearEditLine(true); // SEL/UNDO Con_ToggleConsole calls Partial_Reset_Undo_Selection_Reset

	// Baker: This is the only real place both of these variables are reset
	// Everything calls Con_ToggleConsole
	SET___ con_backscroll = 0; history_line = -1;
#endif

	Consel_MouseReset ("Con_ToggleConsole");
	Con_ClearNotify(); // Baker: Really?  Do other engines do that?  No.  But's not a big deal.
}

// Baker r1003: close console for map/load/etc.
// map, load <game>, restart, changelevel, connect, reconnect, kill
void Con_CloseConsole_If_Client (void)
{
	if (!host_isclient.integer) return;
 	if (Have_Flag(key_consoleactive, KEY_CONSOLEACTIVE_USER_1))
		Con_ToggleConsole();
}

/*
================
Con_ToggleConsole_f
================
*/

void Con_ToggleConsole_f(cmd_state_t *cmd)
{
	Con_ToggleConsole ();
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int i;
	for(i = 0; i < CON_LINES_COUNT; ++i)
		if (!(CON_LINES(i).mask & CON_MASK_CHAT))
			CON_LINES(i).mask |= CON_MASK_HIDENOTIFY;
}

static void Con_MsgCmdMode(cmd_state_t *cmd, signed char mode)
{
	if (cls.demoplayback && mode >= 0)
		return;
	KeyDest_Set (key_message); // key_dest = key_message;
	chat_mode = mode;
	if (Cmd_Argc(cmd) > 1) {
		c_dpsnprintf1(chat_buffer, "%s ", Cmd_Args(cmd));
		chat_bufferpos = (unsigned int)strlen(chat_buffer);
	}
}

/*
================
Con_MessageMode_f

"say"
================
*/
static void Con_MessageMode_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, 0);
}

/*
================
Con_MessageMode2_f

"say_team"
================
*/
static void Con_MessageMode2_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, 1);
}

/*
================
Con_CommandMode_f
================
*/
static void Con_CommandMode_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, -1);
}

/*
================
Con_CheckResize
================
*/
void Con_CheckResize (void)
{
	int i, width;
	float f;

	f = bound(1, con_textsize.value, 128);
	if (f != con_textsize.value)
		Cvar_SetValueQuick(&con_textsize, f);
	width = (int)floor(vid_conwidth.value / con_textsize.value);
	width = bound(1, width, con.textsize/4);
		// FIXME uses con in a non abstracted way

	if (width == con_linewidth)
		return;

	con_linewidth = width;

	for(i = 0; i < CON_LINES_COUNT; ++i)
		CON_LINES(i).line_num_rows_height = -1; // recalculate when next needed

	Con_ClearNotify();
	con_backscroll = 0;
}

//[515]: the simplest command ever
//LadyHavoc: not so simple after I made it print usage...
static void Con_Maps_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) > 2)
	{
		Con_PrintLinef ("usage: maps [mapnameprefix]");
		return;
	}
	else if (Cmd_Argc(cmd) == 2)
		GetMapList(Cmd_Argv(cmd, 1), NULL, 0, /*is_menu_fill*/ false, /*autocompl*/ false, /*suppress*/ false);
	else
		GetMapList("", NULL, 0, /*is_menu_fill*/ false, /*autocompl*/ false, /*suppress*/ false);
}


static void _Con_PrintFile_f(cmd_state_t *cmd)
{
	ccs *sfilename = Cmd_Argv(cmd, 1); // If too few args what is this ANSWER ""
	if (sfilename[0] == 0) {
		Con_PrintLinef ("Specify filename for printfile");
		return;
	}


	ccs *text_a = (ccs *)FS_LoadFile(sfilename, tempmempool, fs_quiet_true, fs_size_ptr_null);
	if (!text_a) {
		Con_PrintLinef ("File " QUOTED_S " not found", sfilename);
		return;
	}


	stringlist_t alist = {0};
	stringlistappend_split_lines_cr_scrub (&alist, text_a);
	for (int idx = 0; idx < alist.numstrings; idx ++) {
		ccs *s = alist.strings[idx];
		Con_PrintLinef ("%04d: %s", idx, s);
	}

	stringlistfreecontents (&alist);
	Mem_FreeNull_ (text_a);
}

static void Con_ConDump_f(cmd_state_t *cmd)
{
	int i;
	qfile_t *file;
	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("usage: condump <filename>");
		return;
	}
	file = FS_OpenRealFile(Cmd_Argv(cmd, 1), "w", fs_quiet_FALSE); // WRITE-EON condump
	if (!file) {
		Con_PrintLinef (CON_ERROR "condump: unable to write file " QUOTED_S, Cmd_Argv(cmd, 1));
		return;
	}
	if (con_mutex) Thread_LockMutex(con_mutex);
	for(i = 0; i < CON_LINES_COUNT; ++i)
	{
		if (condump_stripcolors.integer)
		{
			// sanitize msg
			size_t len = CON_LINES(i).line_strlen;
			char *sanitizedmsg = (char *)Mem_Alloc(tempmempool, len + 1);
			memcpy (sanitizedmsg, CON_LINES(i).pstart, len);
			SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
			FS_Write(file, sanitizedmsg, strlen(sanitizedmsg));
			Mem_Free(sanitizedmsg);
		}
		else
		{
			FS_Write(file, CON_LINES(i).pstart, CON_LINES(i).line_strlen);
		}
		FS_Write(file, "\n", 1);
	}
	if (con_mutex) Thread_UnlockMutex(con_mutex);
	FS_Close(file);
}


// Baker r3101: "copy" and "copy ents"
static void Con_Copy_Ents_f(void)
{
	if (!sv.active || !sv.worldmodel) {
		Con_PrintLinef ("Not running a server");
		return;
	}

	char *entities, entname[MAX_QPATH_128];
	dpsnprintf (entname, sizeof(entname), "%s.ent", sv.worldnamenoextension);
	if ((entities = (char *)FS_LoadFile(entname, tempmempool, fs_quiet_true, fs_size_ptr_null))) {
		Clipboard_Set_Text(entities); // AUTH: Copy ents
		Mem_Free(entities);
		Con_PrintLinef ("Entities from .ent copied to clipboard");
	} else {
		Clipboard_Set_Text(sv.worldmodel->brush.entities); // AUTH: copy ents
		Con_PrintLinef ("Entities from .bsp copied to clipboard");
	}
}

// Copy "showtex" current texture
static void Con_Copy_Tex_f(void)
{
	if (!sv.active || !sv.worldmodel) {
		Con_PrintLinef ("Not running a server");
		return;
	}

	extern cvar_t showtex;
	vec3_t org;
	vec3_t dest;
	vec3_t temp;
	trace_t cltrace = {0};
	int hitnetentity = -1;
	char texstring[MAX_QPATH_128];

	Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, org);
	VectorSet(temp, 65536, 0, 0);
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, dest);
	// clear the traces as we may or may not fill them out, and mark them with an invalid fraction so we know if we did
	memset(&cltrace, 0, sizeof(cltrace));
	cltrace.fraction = 2.0;

	trace_t CL_TraceLine(const vec3_t start, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, int hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities, qbool hitsurfaces);
	cltrace = CL_TraceLine(org, dest, MOVE_HITMODEL_4, NULL,
			SUPERCONTENTS_SOLID | SUPERCONTENTS_WATER | SUPERCONTENTS_SLIME |
			SUPERCONTENTS_LAVA | SUPERCONTENTS_SKY
		,
		0,
		showtex.integer >= 2 ? MATERIALFLAGMASK_TRANSLUCENT : 0, collision_extendmovelength.value,
		true, false, &hitnetentity, true, true);
	if (cltrace.hittexture)
		c_strlcpy (texstring, cltrace.hittexture->name);
	else
		c_strlcpy (texstring, "(no texture hit)");

	Clipboard_Set_Text (texstring); // AUTH: copy tex
	Con_PrintLinef ("texturename " QUOTED_S " copied to clipboard", texstring);
}

WARP_X_(Con_ConDump_f)
void Con_Copy_f(cmd_state_t *cmd)
{
	int j;

	if (Cmd_Argc(cmd) == 2 && String_Match_Caseless(Cmd_Argv(cmd, 1), "ents")) {
		Con_Copy_Ents_f();
		return;
	}

	if (Cmd_Argc(cmd) == 2 && String_Match_Caseless(Cmd_Argv(cmd, 1), "tex")) {
		Con_Copy_Tex_f();
		return;
	}

	if (Cmd_Argc(cmd) != 1) {
		Con_PrintLinef ("usage: copy [ents or tex]");
		return;
	}

	if (con_mutex) Thread_LockMutex(con_mutex);
#if 1
	baker_string_t *k_console = BakerString_Create_Malloc ("");
	char sanitized_msg_buf[MAX_INPUTLINE_16384];
	for (j = 0; j < CON_LINES_COUNT; ++j) {
		// sanitize msg
		con_lineinfo_t *cc = &CON_LINES(j);
		if (Have_Flag(cc->mask, CON_MASK_DEVELOPER) && !developer.integer)
			continue;

		size_t len = CON_LINES(j).line_strlen;
//		if (len >= sizeof(sanitized_msg_buf)) {
	//		int j = 5;
		//}
		//char *sanitizedmsg = (char *)Mem_Alloc (tempmempool, len + 1);
		memcpy (sanitized_msg_buf, CON_LINES(j).pstart, len);
		sanitized_msg_buf[len] = 0;

		SanitizeString (sanitized_msg_buf, sanitized_msg_buf); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
		BakerString_Cat_No_Collide (k_console, strlen(sanitized_msg_buf), sanitized_msg_buf);
		BakerString_Cat_No_Collide (k_console, STRINGLEN(NEWLINE), NEWLINE);
		//strlcat (s_msg_alloc, sanitizedmsg, total_length_required);
		//strlcat (s_msg_alloc, "\n", total_length_required);
		//Mem_Free(sanitizedmsg);
	} // for


#else
	int total_length_required = ONE_CHAR_1;
	for (j = 0; j < CON_LINES_COUNT; ++j) {
		// sanitize msg
		con_lineinfo_t *cc = &CON_LINES(j);
		if (Have_Flag(cc->mask, CON_MASK_DEVELOPER) && !developer.integer)
			continue;

		size_t len = CON_LINES(j).len;
		char *sanitizedmsg = (char *)Mem_Alloc(tempmempool, len + 1);
		memcpy(sanitizedmsg, CON_LINES(j).start, len);
		SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
		total_length_required += ((int)strlen(sanitizedmsg) + ONE_CHAR_1);
		Mem_Free(sanitizedmsg);
	}
	char *s_msg_alloc = (char *)calloc(total_length_required, 1);
	for (j = 0; j < CON_LINES_COUNT; ++j) {
		// sanitize msg
		con_lineinfo_t *cc = &CON_LINES(j);
		if (Have_Flag(cc->mask, CON_MASK_DEVELOPER) && !developer.integer)
			continue;

		size_t len = CON_LINES(j).len;
		char *sanitizedmsg = (char *)Mem_Alloc(tempmempool, len + 1);
		memcpy(sanitizedmsg, CON_LINES(j).start, len);
		SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
		strlcat (s_msg_alloc, sanitizedmsg, total_length_required);
		strlcat (s_msg_alloc, "\n", total_length_required);
		Mem_Free(sanitizedmsg);
	}
#endif
	if (con_mutex) Thread_UnlockMutex(con_mutex);
#if 1
	Clipboard_Set_Text (k_console->string); // AUTH: Con_Copy_f
	BakerString_Destroy_And_Null_It (&k_console);
#else
	Clipboard_Set_Text (s_msg_alloc); // Con_Copy_f
	freenull_(s_msg_alloc);
#endif
	Con_PrintLinef ("Copied console to clipboard");
}

WARP_X_ (Con_Copy_f)
void Consel_Copy (void)
{
	if (con_mutex) Thread_LockMutex(con_mutex);

	baker_string_t *k_console = BakerString_Create_Malloc ("");
	char sanitized_msg_buf[MAX_INPUTLINE_16384];
	for (int j = 0; j < CON_LINES_COUNT; j ++) {
		// sanitize msg
		con_lineinfo_t *cc = &CON_LINES(j);
		int realidx = CON_TRUEIDX (j);
		int in_the_range1 = 
			(g_consel.a.mousedown_end_row_index > g_consel.a.mousedown_row_index) ?
			in_range (g_consel.a.mousedown_row_index,realidx, g_consel.a.mousedown_end_row_index) :
			in_range (g_consel.a.mousedown_end_row_index,realidx, g_consel.a.mousedown_row_index);

		if (in_the_range1 == false)
			continue;

		if (Have_Flag(cc->mask, CON_MASK_DEVELOPER) && !developer.integer)
			continue;

		size_t len = CON_LINES(j).line_strlen;
		memcpy (sanitized_msg_buf, CON_LINES(j).pstart, len);
		sanitized_msg_buf[len] = 0;

		SanitizeString (sanitized_msg_buf, sanitized_msg_buf); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
		BakerString_Cat_No_Collide (k_console, strlen(sanitized_msg_buf), sanitized_msg_buf);
		BakerString_Cat_No_Collide (k_console, STRINGLEN(NEWLINE), NEWLINE);
	} // for

	if (con_mutex) Thread_UnlockMutex(con_mutex);

	Clipboard_Set_Text (k_console->string); // AUTH: Con_Copy_f
	BakerString_Destroy_And_Null_It (&k_console);

	//Con_PrintLinef ("Copied selected console text to clipboard");
}

// Baker r3102: "folder" command
static void Con_Folder_f(cmd_state_t *cmd)
{
#ifdef __ANDROID__
	Con_PrintLinef ("folder opening not supported for this build");
	return;
#endif // __ANDROID__
#if defined(MACOSX)
#if !defined(CORE_XCODE) // XCODE we use .m file, if "brew" using gcc assume objective is not avail
	Con_PrintLinef ("folder opening not supported for this build");
	return;
#endif
#endif // __ANDROID__

	const char *s_current_dir_notrail = File_Getcwd_SBuf();

	Con_DPrintLinef ("s_current_dir: %s", s_current_dir_notrail);
	Con_DPrintLinef ("fs_gamedir: %s", fs_gamedir);			// "C:\Users\Main\Documents/My Games/zircon/id1/"
	Con_DPrintLinef ("fs_basedir: %s", fs_basedir);				// ""
	Con_DPrintLinef ("fs_userdir: %s", fs_userdir);				// "" or "C:\Users\Main\Documents/My Games/zircon/"

	// s_current_dir.. "c:\zircon" no trail
	//   linux with s_current_dir tends to be a clusterfuck
	// fs_gamedir
	// fs_gamedir

	// fs_basedir ""

	// fs_userdir
	// "C:\Users\Main\Documents/My Games/zircon/"
	// gamedirname1 "id1"

	WARP_X_("dir", "folder", FS_AddGameHierarchy)

	char sgdwork[1024];
	c_strlcpy (sgdwork, fs_gamedir);					// "id1/"
	File_URL_Remove_Trailing_Unix_Slash(sgdwork);		// "id1"

	const char *slastcom = File_URL_SkipPath(sgdwork); // "id1"
	char sgamedirlast[1024];
	c_strlcpy(sgamedirlast, slastcom);  // "id1"
	File_URL_Remove_Trailing_Unix_Slash(sgamedirlast);  // "id1" // sgamedirlast is like "id1" or "travail" or whatever

	int is_underdir = Cmd_Argc(cmd) == 2 ? true : false;

	int j;

	for (j = 0; j < 2; j++) {
		char spath[1024];

		// 0 is userdir if it exists
		// 1 is last gamedir
		if (j == 0) {
#ifdef CORE_XCODE
			continue; // don't
#endif
			if (fs_userdir[0] == 0)
				continue;
			va(spath, sizeof(spath), "%s%s/", fs_userdir, sgamedirlast);
		} else {
#ifdef CORE_XCODE
			// This only applies to XCODE .app, if you build Mac terminal app using brew from command line this does not apply
			// We are dealing with a .app, not a command line
			// Our current working directory is somewhere stupid
			// However we have the fs_gamedir and fs_basedir ok
			//Con_PrintLinef ("Last gamedir is %s", sgamedirlast);
			c_strlcpy(spath, fs_gamedir);
#else
			va(spath, sizeof(spath), "%s/%s/", s_current_dir_notrail, sgamedirlast);
#endif
		}

		if (is_underdir) c_strlcat(spath, Cmd_Argv(cmd, 1));

		File_URL_Remove_Trailing_Unix_Slash(spath);

		Con_PrintLinef ("Opening folder %s", spath);
		if (!Folder_Open(spath)) {
			Con_PrintLinef ("Opening folder failed");
			//return;
		}

	} // for

}

void Con_Clear_f(cmd_state_t *cmd)
{
	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_Clear(&con);
	if (con_mutex) Thread_UnlockMutex(con_mutex);
}

static void Con_RCon_ClearPassword_c(cvar_t *var)
{
	// whenever rcon_secure is changed to 0, clear rcon_password for
	// security reasons (prevents a send-rcon-password-as-plaintext
	// attack based on NQ protocol session takeover and svc_stufftext)
	if (var->integer <= 0)
		Cvar_SetQuick(&rcon_password, "");
}


/*
================
Con_PrintToHistory

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be displayed
If no console is visible, the notify window will pop up.
================
*/
static void Con_PrintToHistory(const char *txt, int mask)
{
	// process:
	//   \n goes to next line
	//   \r deletes current line and makes a new one

	static int cr_pending = 0;
	static char buf[CON_TEXTSIZE_1_MB]; // con_mutex
	static int bufpos = 0;

	if (!con.textcon) // FIXME uses a non-abstracted property of con
		return;

	for(; *txt; ++txt)
	{
		if (cr_pending)
		{
			ConBuffer_DeleteLastLine(&con);
			cr_pending = 0;
		}
		switch(*txt)
		{
			case 0:
				break;
			case '\r':
				ConBuffer_AddLine(&con, buf, bufpos, mask);
				bufpos = 0;
				cr_pending = 1;
				break;
			case '\n':
				ConBuffer_AddLine(&con, buf, bufpos, mask);
				bufpos = 0;
				break;
			default:
				buf[bufpos++] = *txt;
				if (bufpos >= con.textsize - 1) // FIXME uses a non-abstracted property of con
				{
					ConBuffer_AddLine(&con, buf, bufpos, mask);
					bufpos = 0;
				}
				break;
		}
	}
}

void Con_Rcon_Redirect_Init(lhnetsocket_t *sock, lhnetaddress_t *dest, qbool proquakeprotocol)
{
	rcon_redirect_sock = sock;
	rcon_redirect_dest = dest;
	rcon_redirect_proquakeprotocol = proquakeprotocol;
	if (rcon_redirect_proquakeprotocol)
	{
		// reserve space for the packet header
		rcon_redirect_buffer[0] = 0;
		rcon_redirect_buffer[1] = 0;
		rcon_redirect_buffer[2] = 0;
		rcon_redirect_buffer[3] = 0;
		// this is a reply to a CCREQ_RCON
		rcon_redirect_buffer[4] = (unsigned char)CCREP_RCON;
	}
	else
		memcpy(rcon_redirect_buffer, "\377\377\377\377n", 5); // QW rcon print
	rcon_redirect_bufferpos = 5;
}

static void Con_Rcon_Redirect_Flush(void)
{
	if (rcon_redirect_sock)
	{
		rcon_redirect_buffer[rcon_redirect_bufferpos] = 0;
		if (rcon_redirect_proquakeprotocol)
		{
			// update the length in the packet header
			StoreBigLong((unsigned char *)rcon_redirect_buffer, NETFLAG_CTL | (rcon_redirect_bufferpos & NETFLAG_LENGTH_MASK));
		}
		NetConn_Write(rcon_redirect_sock, rcon_redirect_buffer, rcon_redirect_bufferpos, rcon_redirect_dest);
	}
	memcpy(rcon_redirect_buffer, "\377\377\377\377n", 5); // QW rcon print
	rcon_redirect_bufferpos = 5;
	rcon_redirect_proquakeprotocol = false;
}

void Con_Rcon_Redirect_End(void)
{
	Con_Rcon_Redirect_Flush();
	rcon_redirect_dest = NULL;
	rcon_redirect_sock = NULL;
}

void Con_Rcon_Redirect_Abort(void)
{
	rcon_redirect_dest = NULL;
	rcon_redirect_sock = NULL;
}

/*
================
Con_Rcon_AddChar
================
*/
/// Adds a character to the rcon buffer.
static void Con_Rcon_AddChar(int c)
{
	if (log_dest_buffer_appending)
		return;
	++log_dest_buffer_appending;

	// if this print is in response to an rcon command, add the character
	// to the rcon redirect buffer

	if (rcon_redirect_dest)
	{
		rcon_redirect_buffer[rcon_redirect_bufferpos++] = c;
		if (rcon_redirect_bufferpos >= (int)sizeof(rcon_redirect_buffer) - 1)
			Con_Rcon_Redirect_Flush();
	}
	else if (*log_dest_udp.string) // don't duplicate rcon command responses here, these are sent another way
	{
		if (log_dest_buffer_pos == 0)
			Log_DestBuffer_Init();
		log_dest_buffer[log_dest_buffer_pos++] = c;
		if (log_dest_buffer_pos >= sizeof(log_dest_buffer) - 1) // minus one, to allow for terminating zero
			Log_DestBuffer_Flush_NoLock();
	}
	else
		log_dest_buffer_pos = 0;

	--log_dest_buffer_appending;
}

/**
 * Convert an RGB color to its nearest quake color.
 * I'll cheat on this a bit by translating the colors to HSV first,
 * S and V decide if it's black or white, otherwise, H will decide the
 * actual color.
 * @param _r Red (0-255)
 * @param _g Green (0-255)
 * @param _b Blue (0-255)
 * @return A quake color character.
 */
static char Sys_Con_NearestColor(const unsigned char _r, const unsigned char _g, const unsigned char _b)
{
	float r = ((float)_r)/255.0;
	float g = ((float)_g)/255.0;
	float b = ((float)_b)/255.0;
	float min = min(r, min(g, b));
	float max = max(r, max(g, b));

	int h; ///< Hue angle [0,360]
	float s; ///< Saturation [0,1]
	float v = max; ///< In HSV v == max [0,1]

	if (max == min)
		s = 0;
	else
		s = 1.0 - (min/max);

	// Saturation threshold. We now say 0.2 is the minimum value for a color!
	if (s < 0.2)
	{
		// If the value is less than half, return a black color code.
		// Otherwise return a white one.
		if (v < 0.5)
			return '0';
		return '7';
	}

	// Let's get the hue angle to define some colors:
	if (max == min)
		h = 0;
	else if (max == r)
		h = (int)(60.0 * (g-b)/(max-min))%360;
	else if (max == g)
		h = (int)(60.0 * (b-r)/(max-min) + 120);
	else // if (max == b) redundant check
		h = (int)(60.0 * (r-g)/(max-min) + 240);

	if (h < 36) // *red* to orange
		return '1';
	else if (h < 80) // orange over *yellow* to evilish-bright-green
		return '3';
	else if (h < 150) // evilish-bright-green over *green* to ugly bright blue
		return '2';
	else if (h < 200) // ugly bright blue over *bright blue* to darkish blue
		return '5';
	else if (h < 270) // darkish blue over *dark blue* to cool purple
		return '4';
	else if (h < 330) // cool purple over *purple* to ugly swiny red
		return '6';
	else // ugly red to red closes the circly
		return '1';
}

/*
================
Con_MaskPrint
================
*/
extern cvar_t timestamps;
extern cvar_t timeformat;
extern qbool sys_nostdout;
void Con_MaskPrint(int additionalmask, const char *msg)
{
	static int mask = 0;
	static int index = 0;
	static char line[MAX_INPUTLINE_16384];

	if (con_mutex)
		Thread_LockMutex(con_mutex);

	for (;*msg;msg++)
	{
		Con_Rcon_AddChar(*msg);
		// if this is the beginning of a new line, print timestamp
		if (index == 0)
		{
			const char *timestamp = timestamps.integer ? Sys_TimeString(timeformat.string) : "";
			// reset the color
			// FIXME: 1. perhaps we should use a terminal system 2. use a constant instead of 7!
			line[index++] = STRING_COLOR_TAG;
			// assert( STRING_COLOR_DEFAULT < 10 )
			line[index++] = STRING_COLOR_DEFAULT + '0';
			// special color codes for chat messages must always come first
			// for Con_PrintToHistory to work properly
			if (*msg == 1 || *msg == 2 || *msg == 3)
			{
				// play talk wav
				if (*msg == 1)
				{
					if (con_chatsound.value)
					{
						if (msg[1] == con_chatsound_team_mask.integer && cl.foundteamchatsound)
							S_LocalSound (con_chatsound_team_file.string);
						else
							S_LocalSound (con_chatsound_file.string);
					}
				}

				// Send to chatbox for say/tell (1) and messages (3)
				// 3 is just so that a message can be sent to the chatbox without a sound.
				if (*msg == 1 || *msg == 3)
					mask = CON_MASK_CHAT;

				line[index++] = STRING_COLOR_TAG;
				line[index++] = '3';
				msg++;
				Con_Rcon_AddChar(*msg);
			}
			// store timestamp
			for (;*timestamp;index++, timestamp++)
				if (index < (int)sizeof(line) - 2)
					line[index] = *timestamp;
			// add the mask
			mask |= additionalmask;
		}
		// append the character
		line[index++] = *msg;
		// if this is a newline character, we have a complete line to print
		if (*msg == '\n' || index >= (int)sizeof(line) / 2)
		{
			// terminate the line
			line[index] = 0;
			// send to log file
			Log_ConPrint(line);

#if defined(__ANDROID__) && defined(_DEBUG)
		Sys_PrintToTerminal(line);
#endif // ! __ANDROID__

			// send to scrollable buffer
			if (con_initialized && cls.state != ca_dedicated)
			{
				Con_PrintToHistory(line, mask);
			}
			// send to terminal or dedicated server window
			if (!sys_nostdout)
			if (developer.integer || !(mask & CON_MASK_DEVELOPER))
			{
				if (sys_specialcharactertranslation.integer)
				{
					char *p;
					const char *q;
					p = line;
					while(*p)
					{
						int ch = u8_getchar(p, &q);
						if (ch >= 0xE000 && ch <= 0xE0FF && ((unsigned char) qfont_table[ch - 0xE000]) >= 0x20)
						{
							*p = qfont_table[ch - 0xE000];
							if (q > p+1)
								memmove(p+1, q, strlen(q)+1);
							p = p + 1;
						}
						else
							p = p + (q - p);
					}
				}

				if (sys_colortranslation.integer == 1) // ANSI
				{
					static char printline[MAX_INPUTLINE_16384 * 4 + 3];
						// 2 can become 7 bytes, rounding that up to 8, and 3 bytes are added at the end
						// a newline can transform into four bytes, but then prevents the three extra bytes from appearing
					int lastcolor = 0;
					const char *in;
					char *out;
					int color;
					for(in = line, out = printline; *in; ++in)
					{
						switch(*in)
						{
							case STRING_COLOR_TAG:
								if ( in[1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(in[2]) && isxdigit(in[3]) && isxdigit(in[4]) )
								{
									char r = tolower(in[2]);
									char g = tolower(in[3]);
									char b = tolower(in[4]);
									// it's a hex digit already, so the else part needs no check --blub
									if (isdigit(r)) r -= '0';
									else r -= 87;
									if (isdigit(g)) g -= '0';
									else g -= 87;
									if (isdigit(b)) b -= '0';
									else b -= 87;

									color = Sys_Con_NearestColor(r * 17, g * 17, b * 17);
									in += 3; // 3 only, the switch down there does the fourth
								}
								else
									color = in[1];

								switch(color)
								{
									case STRING_COLOR_TAG:
										++in;
										*out++ = STRING_COLOR_TAG;
										break;
									case '0':
									case '7':
										// normal color
										++in;
										if (lastcolor == 0) break; else lastcolor = 0;
										*out++ = 0x1B; *out++ = '['; *out++ = 'm';
										break;
									case '1':
										// light red
										++in;
										if (lastcolor == 1) break; else lastcolor = 1;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '1'; *out++ = 'm';
										break;
									case '2':
										// light green
										++in;
										if (lastcolor == 2) break; else lastcolor = 2;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '2'; *out++ = 'm';
										break;
									case '3':
										// yellow
										++in;
										if (lastcolor == 3) break; else lastcolor = 3;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '3'; *out++ = 'm';
										break;
									case '4':
										// light blue
										++in;
										if (lastcolor == 4) break; else lastcolor = 4;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '4'; *out++ = 'm';
										break;
									case '5':
										// light cyan
										++in;
										if (lastcolor == 5) break; else lastcolor = 5;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '6'; *out++ = 'm';
										break;
									case '6':
										// light magenta
										++in;
										if (lastcolor == 6) break; else lastcolor = 6;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '5'; *out++ = 'm';
										break;
									// 7 handled above
									case '8':
									case '9':
										// bold normal color
										++in;
										if (lastcolor == 8) break; else lastcolor = 8;
										*out++ = 0x1B; *out++ = '['; *out++ = '0'; *out++ = ';'; *out++ = '1'; *out++ = 'm';
										break;
									default:
										*out++ = STRING_COLOR_TAG;
										break;
								}
								break;
							case '\n':
								if (lastcolor != 0)
								{
									*out++ = 0x1B; *out++ = '['; *out++ = 'm';
									lastcolor = 0;
								}
								*out++ = *in;
								break;
							default:
								*out++ = *in;
								break;
						}
					}
					if (lastcolor != 0)
					{
						*out++ = 0x1B;
						*out++ = '[';
						*out++ = 'm';
					}
					*out++ = 0;
					Sys_PrintToTerminal(printline);
				}
				else if (sys_colortranslation.integer == 2) // Quake
				{
					Sys_PrintToTerminal(line);
				}
				else // strip
				{
					static char printline[MAX_INPUTLINE_16384]; // it can only get shorter here
					const char *in;
					char *out;
					for(in = line, out = printline; *in; ++in)
					{
						switch(*in)
						{
							case STRING_COLOR_TAG:
								switch(in[1])
								{
									case STRING_COLOR_RGB_TAG_CHAR:
										if ( isxdigit(in[2]) && isxdigit(in[3]) && isxdigit(in[4]) )
										{
											in+=4;
											break;
										}
										*out++ = STRING_COLOR_TAG;
										*out++ = STRING_COLOR_RGB_TAG_CHAR;
										++in;
										break;
									case STRING_COLOR_TAG:
										++in;
										*out++ = STRING_COLOR_TAG;
										break;
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										++in;
										break;
									default:
										*out++ = STRING_COLOR_TAG;
										break;
								}
								break;
							default:
								*out++ = *in;
								break;
						}
					}
					*out++ = 0;
					Sys_PrintToTerminal(printline);
				}
			}
			// empty the line buffer
			index = 0;
			mask = 0;
		}
	}

	if (con_mutex)
		Thread_UnlockMutex(con_mutex);
}

/*
================
Con_MaskPrintf
================
*/
void Con_MaskPrintf(int mask, const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(mask, msg);
}

/*
================
Con_Print
================
*/
void Con_Print(const char *msg)
{
	Con_MaskPrint(CON_MASK_PRINT, msg);
}

/*
================
Con_Printf
================
*/
void Con_Printf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(CON_MASK_PRINT, msg);
}

// Baker: print line support
// It's crazy to add \n to every damn Con_Printf
void Con_PrintLinef (const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	// Baker: We are doing -4 to ensure room for newline
	// TODO: Check that dpvsnprintf handles UTF-8 truncation properly?
	//  Nope. vsnprintf can truncate anywhere ...
	va_start(argptr,fmt);
	dpvsnprintf(msg, /*size*/ MAX_INPUTLINE_16384 - 4, fmt, argptr);
	va_end(argptr);

	// Baker: This max len is MAX_INPUTLINE_16384
	int s_len = (int)strlen(msg);
	msg[s_len + 0] = 10; 	// 0	 ---> 10
	msg[s_len + 1] = 0; 	// extra ---> 0

	Con_MaskPrint(CON_MASK_PRINT, msg);
}

// Baker r1421: centerprint logging to console
// Baker: CON_MASK_HIDENOTIFY means don't show in left-top of screen like "You got 5 health"
void Con_HidenotifyPrintLinef(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	int s_len = (int)strlen(msg);
	if (s_len >= MAX_INPUTLINE_16384 - 1) {
		msg[s_len - 1] = 10; // trunc
	} else {
		msg[s_len] = 10; // was 0
		msg[s_len+1] = 0; // extra
	}

	Con_MaskPrint (CON_MASK_HIDENOTIFY, msg);
}


/*
================
Con_DPrint
================
*/
void Con_DPrint(const char *msg)
{
	if (developer.integer < 0) // at 0, we still add to the buffer but hide
		return;

	Con_MaskPrint(CON_MASK_DEVELOPER, msg);
}

/*
================
Con_DPrintf
================
*/
void Con_DPrintf (const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	if (developer.integer < 0) // at 0, we still add to the buffer but hide
		return;

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(CON_MASK_DEVELOPER, msg);
}

void Con_DPrintLinef(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE_16384];

	if (developer.integer < 0) // at 0, we still add to the buffer but hide
		return;

	// Baker: We are doing -4 to ensure room for newline
	// TODO: Check that dpvsnprintf handles UTF-8 truncation properly?
	//  Nope. vsnprintf can truncate anywhere ...
	va_start(argptr, fmt);
	dpvsnprintf(msg, /*size*/ MAX_INPUTLINE_16384 - 4, fmt, argptr);
	va_end(argptr);

	// Baker: This max len is MAX_INPUTLINE_16384
	int s_len = (int)strlen(msg);
	msg[s_len + 0] = 10; 	// 0	 ---> 10
	msg[s_len + 1] = 0; 	// extra ---> 0

	Con_MaskPrint(CON_MASK_DEVELOPER, msg);
}

/*
==============================================================================

DRAWING

==============================================================================
*/

/*
================
Con_DrawInput

It draws either the console input line or the chat input line (if is_console is false)
The input line scrolls horizontally if typing goes beyond the right edge

Modified by EvilTypeGuy eviltypeguy@qeradiant.com
================
*/

// FFA extern const char *g_p_selbeyond;
// FFA extern int g_p_selbeyond_x;

static void Con_DrawInput (qbool is_console, float x, float v, float inputsize)
{
	// Baker: DarkPlaces copies the draw input line for the console or chat buffer
	// to a text buffer every frame
	// it manipulates it and memmoves within text
	int text_slen, i, col_out, linepos, text_start, prefix_start = 0;
	char text[MAX_INPUTLINE_16384 + 5 + 9 + 1]; // space for ^xRGB, "say_team:" and \0
	float xo;
	size_t len_out;
	const char *prefix;
	dp_font_t *fnt;

	if (is_console && !key_consoleactive)
		return;		// don't draw anything

	if (is_console) {
		// empty prefix because ] is part of the console edit line
		prefix = "";
		c_strlcpy (text, key_line);
		linepos = key_linepos;
		fnt = FONT_CONSOLE;
	}
	else
	{
		if (chat_mode < 0)
			prefix = "]";
		else if (chat_mode)
			prefix = "say_team:";
		else
			prefix = "say:";
		strlcpy(text, chat_buffer, sizeof(text));
		linepos = chat_bufferpos;
		fnt = FONT_CHAT;
	}

	text_slen = (int)strlen(text);

	// make the color code visible when the cursor is inside it
	if (text[linepos] != 0) {
		for (i = 1; i < 5 && linepos - i > 0; ++i)
			if (text[linepos-i] == STRING_COLOR_TAG) {
				int caret_pos, ofs = 0;
				caret_pos = linepos - i;
				if (i == 1 && text[caret_pos+1] == STRING_COLOR_TAG)
					ofs = 1;
				else if (i == 1 && isdigit(text[caret_pos+1]))
					ofs = 2;
				else if (text[caret_pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(text[caret_pos+2]) && isxdigit(text[caret_pos+3]) && isxdigit(text[caret_pos+4]))
					ofs = 5;
				if (ofs && (size_t)(text_slen + ofs + 1) < sizeof(text))
				{
					int carets = 1;
					while(caret_pos - carets >= 1 && text[caret_pos - carets] == STRING_COLOR_TAG)
						++carets;
					if (carets & 1)
					{
						// str^2ing (displayed as string) --> str^2^^2ing (displayed as str^2ing)
						// str^^ing (displayed as str^ing) --> str^^^^ing (displayed as str^^ing)
						memmove(&text[caret_pos + ofs + 1], &text[caret_pos], text_slen - caret_pos);
						text[caret_pos + ofs] = STRING_COLOR_TAG;
						text_slen += ofs + 1;
						text[text_slen] = 0;
					}
				}
				break;
			}
	}

	if (is_console == false) {
		// Baker: Chat mode ...
		prefix_start = x;
		x += DrawQ_TextWidth(prefix, 0, inputsize, inputsize, false, fnt);
	}

	len_out = linepos;
	col_out = -1;
	xo = 0;

	if (linepos > 0)
		xo = DrawQ_TextWidth_UntilWidth_TrackColors(text, &len_out,
			inputsize, inputsize, &col_out, false, fnt, 1000000000);

	text_start = x + (vid_conwidth.value - x) * 0.95 - xo; // scroll
	if (text_start >= x)
		text_start = x;
	else if (!is_console)
		prefix_start -= (x - text_start);

	if (!is_console)
		DrawQ_String(prefix_start, v, prefix, 0, inputsize, inputsize, 1.0, 1.0, 1.0, 1.0, 0, NULL, false, fnt);


	WARP_X_ (DrawQ_String_Scale)

baker_font_setup_here:

#if 0 // // FFA
	// Baker the additional chars is onlu when linepos is in there
	if (0 && is_console && key_sellength) {
		int xcharpos0		= key_sellength > 0 ? key_linepos - key_sellength : key_linepos;
		int xcharlength		= key_sellength > 0 ? key_sellength :  - key_sellength;
		int xcharbeyond		= xcharpos0 + xcharlength;
		char *s_ptr_beyond	= &text[xcharbeyond];

		g_p_selbeyond		= s_ptr_beyond;
		g_p_selbeyond_x		= 0;
	} else {
		g_p_selbeyond		= NULL; // No request
		g_p_selbeyond_x		= 0;
	}
#endif

	DrawQ_String (
		text_start,			// start px
		v,					// start py
		text,				// text
		text_slen + 3,		// draw length in chars, might be altered above
		inputsize,			// width con_textsize.value or chatmode uses notifysize
		inputsize,			// height
		1.0, 1.0, 1.0, 1.0, // rgba
		0,					// flags
		NULL,				// color reply
		false,				// ignore color codes
		fnt					// dp_font_t *fnt
	);

	// draw a cursor on top of this
	// Baker: Faster blink easier to identify position of cursor when typing quickly
	if ((int)(host.realtime*con_cursorspeed) & 1) { // BLINK
		// cursor is visible
		if (utf8_enable.integer == 0) {
			text[0] = 11 + 130 * key_insert;	// either solid or triangle facing right
			text[1] = 0;
		}
		else
		{
			size_t len;
			const char *curbuf;
			char charbuf16[16];
			curbuf = u8_encodech(0xE000 + 11 + 130 * key_insert, &len, charbuf16);
			memcpy(text, curbuf, len);
			text[len] = 0;
		}

		// Baker: This is either console or chatmode
		if (is_console) {
			// Selection would be DRAWFLAG_ADDITIVE
			// Baker r0003: Thin cursor / no text overwrite mode
			DrawQ_Fill (text_start + xo + con_textsize.value * (1 / 8.0) /*hereo*/,									// x
			con_vislines - con_textsize.value * 2 /*+ con_textsize.value * (1/8.0)*/,						// y
			con_textsize.value * (1 / 8.0),															// w
			con_textsize.value * 1/*(6/8.0)*/, /*rgb*/ 0.75, 0.75, 0.75, /*a*/ 1.0, DRAWFLAG_NORMAL_0);	// h
		} else {
			// Baker: Is this always chat mode?  Yes
			DrawQ_String (text_start + xo, v, text, 0, inputsize,
				inputsize, 1.0, 1.0, 1.0, 1.0, 0, &col_out, false, fnt);
		}

	} // cursor visible

	if (is_console && key_sellength) {
#if 0 // gcc unused
		float draw_start = text_start + xo; // Baker: should always be accurate
#endif
		// Baker: What happens if we try to put UTF8 in a name?
		// How can we input UTF8 into DarkPlaces without the clipboard?
		// Baker: It's a bit more complex .. DarkPlaces supports ^xFC3 RGB color codes

		float x0 = text_start + xo - con_textsize.value * key_sellength;
		float y0 = con_vislines - con_textsize.value * 2;
		float w0 = con_textsize.value * key_sellength;
		float h0 = con_textsize.value * 1;

		if (w0 < 0) {
			x0 = x0 + w0;
			w0 = - w0;
		}

#if 0 // FFA
		if (g_p_selbeyond_x) {
			float jx0 = text_start + xo;
			float jx1 = g_p_selbeyond_x;
			float jxw = jx1 - jx0;
			float jw0 = jxw;
			if (jw0) {
				x0 = jx0;
				w0 = jxw;
			}
		}
		g_p_selbeyond		= NULL; // No request
		g_p_selbeyond_x		= 0;
#endif

		DrawQ_Fill (x0, y0, w0, h0, /*rgb*/ 0.5, 0.5, 0.5, /*a*/ 0.5, DRAWFLAG_ADDITIVE);	// h
	} // selection

}

typedef struct
{
	dp_font_t *font;
	float alignment; // 0 = left, 0.5 = center, 1 = right
	float fontsize;
	float x;
	float y;
	float width;
	float ymin, ymax;
	const char *continuationString;

	// PRIVATE:
	int colorindex; // init to -1
}
con_text_info_t;

static float Con_WordWidthFunc(void *passthrough, const char *w, size_t *length, float maxWidth)
{
	con_text_info_t *ti = (con_text_info_t *) passthrough;
	if (w == NULL)
	{
		ti->colorindex = -1;
		return ti->fontsize * ti->font->maxwidth;
	}
	if (maxWidth >= 0)
		return DrawQ_TextWidth_UntilWidth(w, length, ti->fontsize, ti->fontsize, false, ti->font, -maxWidth); // -maxWidth: we want at least one char
	else if (maxWidth == -1)
		return DrawQ_TextWidth(w, *length, ti->fontsize, ti->fontsize, false, ti->font);
	else
	{
		Sys_PrintfToTerminal ("Con_WordWidthFunc: can't get here (maxWidth should never be %f)\n", maxWidth);
		// Note: this is NOT a Con_Printf, as it could print recursively
		return 0;
	}
}

static int Con_CountLineFunc(void *passthrough, const char *line, size_t length, float width, qbool isContinuation)
{
	(void) passthrough;
	(void) line;
	(void) length;
	(void) width;
	(void) isContinuation;
	return 1;
}

static int Con_DisplayLineFunc(void *passthrough, const char *line, size_t length, float width, qbool isContinuation)
{
	con_text_info_t *ti = (con_text_info_t *) passthrough;

	if (ti->y < ti->ymin - 0.001)
		(void) 0;
	else if (ti->y > ti->ymax - ti->fontsize + 0.001)
		(void) 0;
	else
	{
		int x = (int) (ti->x + (ti->width - width) * ti->alignment);
		if (isContinuation && *ti->continuationString)
			x = (int) DrawQ_String(x, ti->y, ti->continuationString, strlen(ti->continuationString), ti->fontsize, ti->fontsize, 1.0, 1.0, 1.0, 1.0, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_false, ti->font);
		if (length > 0)
			DrawQ_String(x, ti->y, line, length, ti->fontsize, ti->fontsize, /*rgba*/ 1.0, 1.0, 1.0, 1.0, DRAWFLAG_NORMAL_0, /*outcolor*/ &(ti->colorindex), q_ignore_color_codes_false, ti->font);
	}

	ti->y += ti->fontsize;
	return 1;
}

static int Con_DrawNotifyRect(int mask_must, int mask_mustnot, float maxage, float x, float y, float width, float height, float fontsize, float alignment_x, float alignment_y, const char *continuationString)
{
	int i;
	int lines = 0;
	int maxlines = (int) floor(height / fontsize + 0.01f);
	int startidx;
	int nskip = 0;
	int continuationWidth = 0;
	size_t len;
	double t = cl.time; // saved so it won't change
	con_text_info_t ti;

	ti.font = (mask_must & CON_MASK_CHAT) ? FONT_CHAT : FONT_NOTIFY;
	ti.fontsize = fontsize;
	ti.alignment = alignment_x;
	ti.width = width;
	ti.ymin = y;
	ti.ymax = y + height;
	ti.continuationString = continuationString;

	len = 0;
	Con_WordWidthFunc(&ti, NULL, &len, -1);
	len = strlen(continuationString);
	continuationWidth = (int) Con_WordWidthFunc(&ti, continuationString, &len, -1);

	// first find the first line to draw by backwards iterating and word wrapping to find their length...
	startidx = CON_LINES_COUNT;
	for(i = CON_LINES_COUNT - 1; i >= 0; --i)
	{
		con_lineinfo_t *l = &CON_LINES(i);
		int mylines;

		if ((l->mask & mask_must) != mask_must)
			continue;
		if (l->mask & mask_mustnot)
			continue;
		if (maxage && (l->addtime < t - maxage))
			continue;

		// WE FOUND ONE!
		// Calculate its actual height...
		// Baker: So darkplaces will wordwrap super long text lines of notify text.
		mylines = COM_Wordwrap_Num_Rows_Drawn(l->pstart, l->line_strlen, continuationWidth, width, Con_WordWidthFunc, &ti, Con_CountLineFunc, &ti);
		if (lines + mylines >= maxlines)
		{
			nskip = lines + mylines - maxlines;
			lines = maxlines;
			startidx = i;
			break;
		}
		lines += mylines;
		startidx = i;
	}

	// then center according to the calculated amount of lines...
	ti.x = x;
	ti.y = y + alignment_y * (height - lines * fontsize) - nskip * fontsize;

	// then actually draw
	for(i = startidx; i < CON_LINES_COUNT; ++i)
	{
		con_lineinfo_t *l = &CON_LINES(i);

		if ((l->mask & mask_must) != mask_must)
			continue;
		if (l->mask & mask_mustnot)
			continue;
		if (maxage && (l->addtime < t - maxage))
			continue;

		COM_Wordwrap_Num_Rows_Drawn (l->pstart, l->line_strlen, continuationWidth, width, Con_WordWidthFunc, &ti, Con_DisplayLineFunc, &ti);
	}

	return lines;
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	float x, v;
	float chatstart, notifystart, inputsize, height;
	float align;
	int numChatlines;
	int chatpos;

	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_FixTimes(&con);

	numChatlines = con_chat.integer;

	chatpos = con_chatpos.integer;

	if (con_notify.integer < 0)
		Cvar_SetValueQuick(&con_notify, 0);
	if (gamemode == GAME_TRANSFUSION)
		v = 8; // vertical offset
	else
		v = 0;

	// GAME_NEXUIZ: center, otherwise left justify
	align = con_notifyalign.value;
	if (!*con_notifyalign.string) // empty string, evaluated to 0 above
	{
		if (IS_OLDNEXUIZ_DERIVED(gamemode))
			align = 0.5;
	}

	if (numChatlines || !con_chatrect.integer)
	{
		if (chatpos == 0)
		{
			// first chat, input line, then notify
			chatstart = v;
			notifystart = v + (numChatlines + 1) * con_chatsize.value;
		}
		else if (chatpos > 0)
		{
			// first notify, then (chatpos-1) empty lines, then chat, then input
			notifystart = v;
			chatstart = v + (con_notify.value + (chatpos - 1)) * con_notifysize.value;
		}
		else // if (chatpos < 0)
		{
			// first notify, then much space, then chat, then input, then -chatpos-1 empty lines
			notifystart = v;
			chatstart = vid_conheight.value - (-chatpos-1 + numChatlines + 1) * con_chatsize.value;
		}
	}
	else
	{
		// just notify and input
		notifystart = v;
		chatstart = 0; // shut off gcc warning
	}

	v = notifystart + con_notifysize.value * Con_DrawNotifyRect(0, CON_MASK_INPUT | CON_MASK_HIDENOTIFY | (numChatlines ? CON_MASK_CHAT : 0) | CON_MASK_DEVELOPER, con_notifytime.value, 0, notifystart, vid_conwidth.value, con_notify.value * con_notifysize.value, con_notifysize.value, align, 0.0, "");

	if (con_chatrect.integer)
	{
		x = con_chatrect_x.value * vid_conwidth.value;
		v = con_chatrect_y.value * vid_conheight.value;
	}
	else
	{
		x = 0;
		if (numChatlines) // only do this if chat area is enabled, or this would move the input line wrong
			v = chatstart;
	}
	height = numChatlines * con_chatsize.value;

	if (numChatlines)
	{
		Con_DrawNotifyRect(CON_MASK_CHAT, CON_MASK_INPUT, con_chattime.value, x, v, vid_conwidth.value * con_chatwidth.value, height, con_chatsize.value, 0.0, 1.0, "^3 ... ");
		v += height;
	}
	if (key_dest == key_message)
	{
		inputsize = (numChatlines ? con_chatsize : con_notifysize).value;
		Con_DrawInput(/*is console*/ false, x, v, inputsize);
	}
	else
		chat_bufferpos = 0;

	if (con_mutex) Thread_UnlockMutex(con_mutex);
}

/*
================
Con_LineHeight

Returns the height of a given console line; calculates it if necessary.
================
*/
int Con_LineHeight(int lineno)
{
	con_lineinfo_t *li = &CON_LINES(lineno);
	if (li->line_num_rows_height == -1)
	{
		float width = vid_conwidth.value;
		con_text_info_t ti;
		ti.fontsize = con_textsize.value;
		ti.font = FONT_CONSOLE;
		li->line_num_rows_height = COM_Wordwrap_Num_Rows_Drawn(li->pstart, li->line_strlen, 0, width, Con_WordWidthFunc, &ti, Con_CountLineFunc, NULL);
	}
	return li->line_num_rows_height;
}

/*
================
Con_DrawConsoleLine

Draws a line of the console; returns its height in lines.
If alpha is 0, the line is not drawn, but still wrapped and its height
returned.
================
*/
static int Con_DrawConsoleLine_Num_Rows_Drawn(int mask_must, int mask_mustnot, float y, int lineno, 
							   float ymin, float ymax)
{
	float width = vid_conwidth.value;
	con_text_info_t ti;
	con_lineinfo_t *li = &CON_LINES(lineno);

	if ((li->mask & mask_must) != mask_must)
		return 0;
	if ((li->mask & mask_mustnot) != 0)
		return 0;

	ti.continuationString = "";
	ti.alignment = 0;
	ti.fontsize = con_textsize.value;
	ti.font = FONT_CONSOLE;
	ti.x = 0;
	ti.y = y - (Con_LineHeight(lineno) - 1) * ti.fontsize;
	ti.ymin = ymin;
	ti.ymax = ymax;
	ti.width = width;

	return COM_Wordwrap_Num_Rows_Drawn (li->pstart, li->line_strlen, 0, width, Con_WordWidthFunc, &ti, Con_DisplayLineFunc, &ti);
}

/*
================
Con_LastVisibleLine

Calculates the last visible line index and how much to show of it based on
con_backscroll.
================
*/
static void Con_LastVisibleLine (int mask_must, int mask_mustnot, int *last_row_number, int *limitlast)
{
	int lines_seen = 0;
	int i;

	if (con_backscroll < 0)
		con_backscroll = 0;

	*last_row_number = 0;

	// Baker: con
	// con.lines which appears to be a pointer into text con.text?
	WARP_X_ (con)

	// now count until we saw con_backscroll actual lines
	for (i = CON_LINES_COUNT - 1; i >= 0; --i)
	if ((CON_LINES(i).mask & mask_must) == mask_must)
	if ((CON_LINES(i).mask & mask_mustnot) == 0) {
		int h = Con_LineHeight(i);

		// line is the last visible line?
		*last_row_number = i;
		if (lines_seen + h > con_backscroll && lines_seen <= con_backscroll) {
			*limitlast = lines_seen + h - con_backscroll;

			// Baker: I have not seen limitlast set to anything except 1
			//if (*limitlast > 1) {
			//	int j= 5;
			//}
			return;
		}

		lines_seen += h;
	}

	// if we get here, no line was on screen - scroll so that one line is
	// visible then.
	con_backscroll = lines_seen - 1;
	*limitlast = 1;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
WARP_X_ (CL_UpdateScreen_SCR_DrawScreen -> SCR_DrawConsole)

consel_t g_consel;

// Baker: Console tips
float       scr_controldown_time;


void Con_DrawConsole (int lines_in_pixels)
{
	float alpha, alpha0;
	double sx, sy;
	const int mask_must_0 = 0;
	int mask_mustnot_dev = (developer.integer>0) ? 0 : CON_MASK_DEVELOPER;
	cachepic_t *conbackpic;
	unsigned int conbackflags;

	if (lines_in_pixels <= 0)
		return;

	if (con_mutex) Thread_LockMutex(con_mutex);

	if (con_backscroll < 0)
		con_backscroll = 0;

	con_vislines = lines_in_pixels;

	r_draw2d_force = true;

// draw the background
	// DrawConsole -  always full alpha when not in game
	alpha0 = cls.signon == SIGNONS_4 ? scr_conalpha.value : 1.0f; 
	
	if ((alpha = alpha0 * scr_conalphafactor.value /*d: 1*/) > 0) {
		sx = scr_conscroll_x.value /*d: 0*/;
		sy = scr_conscroll_y.value /*d: 0*/;
		conbackflags = CACHEPICFLAG_FAILONMISSING_256; // So console is readable when game content is missing
		if (sx != 0 || sy != 0)
			conbackflags &= CACHEPICFLAG_NOCLAMP;
		conbackpic = scr_conbrightness.value /*d: 1*/>= 0.01f ? Draw_CachePic_Flags("gfx/conback", conbackflags) : NULL;
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if (Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines_in_pixels - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
		else
			DrawQ_Fill (0, lines_in_pixels - vid_conheight.integer, vid_conwidth.integer, vid_conheight.integer, 0.0f, 0.0f, 0.0f, alpha, 0);
	}

	// gfx/conback2 - Xonotic does use this
	if ((alpha = alpha0 * scr_conalpha2factor.value /*d: 0*/) > 0) {
		sx = scr_conscroll2_x.value;
		sy = scr_conscroll2_y.value;
		conbackpic = Draw_CachePic_Flags("gfx/conback2", (sx != 0 || sy != 0) ? CACHEPICFLAG_NOCLAMP : 0);
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if (Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines_in_pixels - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
	}

	// gfx/conback3 - Xonotic does use this
	if ((alpha = alpha0 * scr_conalpha3factor.value /*d: 0*/) > 0) {
		sx = scr_conscroll3_x.value;
		sy = scr_conscroll3_y.value;
		conbackpic = Draw_CachePic_Flags("gfx/conback3", (sx != 0 || sy != 0) ? CACHEPICFLAG_NOCLAMP : 0);
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if (Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines_in_pixels - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
	} // alpha > 0

	// Baker r0003: Thin cursor / no text overwrite mode
	DrawQ_String (/*x*/ vid_conwidth.integer -
							DrawQ_TextWidth (
								engineversionshort , 0, con_textsize.value, con_textsize.value,
								q_ignore_color_codes_true /* Baker corrected*/, FONT_CONSOLE
							),

			// Baker: Last row lines_in_pixels like 405 - height of a row.
			/*y*/ lines_in_pixels - con_textsize.value,
			/*string*/ engineversionshort,
			q_text_maxlen_0,
			/*scalex*/ con_textsize.value, //bronzey
			/*scaley*/ con_textsize.value, /*rgba*/ 0.90, 0.90, 0.90, 1.0, /*flags outcolor*/ DRAWFLAG_NORMAL_0,
			q_outcolor_null, q_ignore_color_codes_true, FONT_CONSOLE); // Baker 1007.2

// draw the text

	if (CON_LINES_COUNT > 0) {
		int lineno_in_conbuffer, last, limitlast;
		float y_pixels;
		float ymax = con_vislines - 2 * con_textsize.value;

		Con_LastVisibleLine (mask_must_0, mask_mustnot_dev, &last, &limitlast); // Baker: Sets last variable

		y_pixels = ymax - con_textsize.value;

		if (limitlast)
			y_pixels += (CON_LINES(last).line_num_rows_height - limitlast) * con_textsize.value;

		// Baker: Ok what is this "last"
		lineno_in_conbuffer = last; // Get things like 387 

		// Baker: i goes like 387 then goes 386, 385, 384 ...
		// We are drawing from bottom of screen up and then stopping ...
		g_consel.draww.console_draw_frame = host.superframecount;// was_consoledrawn = true;
		g_consel.draww.draw_row_last_index = last;
		g_consel.draww.draw_row_top_pixel_y = y_pixels;

		if (g_consel.draww.conscalewidth != vid_conwidth.value)
			g_consel.draww.conscalewidth = vid_conwidth.value;
		if (g_consel.draww.conscaleheight != vid_conheight.value)
			g_consel.draww.conscaleheight = vid_conheight.value;

		int in_select = key_consoleactive && g_consel.a.drag_state > drag_state_awaiting_threshold_1;

		while (1) {
			// Baker: I hope Con_DrawConsoleLine returns 1 always
			con_lineinfo_t *li = &CON_LINES(lineno_in_conbuffer);
			
			int advance_rows = 
				Con_DrawConsoleLine_Num_Rows_Drawn (mask_must_0, mask_mustnot_dev, y_pixels, lineno_in_conbuffer, 0, ymax);

			//if (li->line_num_rows_height == -1)
			li->line_num_rows_height = advance_rows;
			
			if (in_select && advance_rows) {
				int realidx = CON_TRUEIDX (lineno_in_conbuffer);
				int in_the_range1 = 
					(g_consel.a.mousedown_end_row_index > g_consel.a.mousedown_row_index) ?
					in_range (g_consel.a.mousedown_row_index,realidx, g_consel.a.mousedown_end_row_index) :
					in_range (g_consel.a.mousedown_end_row_index,realidx, g_consel.a.mousedown_row_index);

				if (in_the_range1) {
					int x = 0;
					int y = y_pixels - (advance_rows - 1) * con_textsize.value;
					int w = vid_conwidth.integer;
					int h = advance_rows * con_textsize.value;
					DrawQ_Fill (x, y, w, h, /*rgb*/ 0.5, 0.5, 0.5, /*a*/ 0.5, DRAWFLAG_ADDITIVE);	// h
				}
			} // in_select

			y_pixels -= advance_rows * con_textsize.value;
			if (lineno_in_conbuffer == 0)
				break; // top of console buffer
			if (y_pixels < 0)
				break; // top of console window
			limitlast = 0;
			lineno_in_conbuffer --;
		} // while
	} // if

// draw the input prompt, user text, and cursor if desired
	Con_DrawInput (/*is console*/ true, 0, con_vislines - con_textsize.value /*d:8*/ * 2, con_textsize.value);

	r_draw2d_force = false;

	WARP_X_ (DrawQ_TextWidth)

	if (!KM_CTRL_ANY && scr_controldown_time)
		scr_controldown_time = 0;

	if (key_sellength != 0 && KM_CTRL_ANY) {
		va_super (s, 256, "%d chars", abs(key_sellength));
			
			//GoogleRobotoFont_Check ();
			int textwidth_pels = DrawQ_TextWidth(s, maxlen_0, con_textsize.value, con_textsize.value, q_ignore_color_codes_false, FONT_CONSOLE);

			int curx = vid_conwidth.integer - (textwidth_pels + con_textsize.value * 2.5), cury = 10;
			int x = curx - 2;
			int y = cury - 2;//y_pixels - (advance_rows - 1) * con_textsize.value;
			int w = textwidth_pels + 2 * 2;
			int h = con_textsize.value + 2 * 2;
			DrawQ_Fill (x, y, w, h, q_rgba_alpha75_black_4_parms, DRAWFLAG_NORMAL_0);	// h

			float xafter = DrawQ_String_Scale (curx, cury, s, maxlen_0, 
				con_textsize.value /*d:8*/, con_textsize.value /*d:8*/, scale_1_0,
				scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, 
				ignorecolorcodes_false, FONT_CONSOLE );
	} else if (KM_CTRL_ANY && scr_controldown_time && con_hints.value > 0) {
		// Holding control for 1 second or more show keys
		float begin_time = (scr_controldown_time + con_hints.value);
		//DebugPrintLinef ("realtime %g begin_time %g", host.realtime, scr_controldown_time);
		while (scr_controldown_time && host.realtime >= begin_time) {
			float sizehere = con_textsize.value;
			int curx = sizehere * 0.5;
			int cury = sizehere * 2;
			ccs *ss[] = {
				"L: Clear Console",
				"I: Inspector",
				"P: Pause Freeze",
				"R: Recall Find",
				"T: Traceline",
				"+: Zoom In",
				"-: Zoom Out",
				"0: Zoom Reset",
				"SHIFT-P: Showpos",
				"SHIFT-S: Hide HUD",
				"",
				"UP:   Size console",
				"DOWN: Size console",
			};
			int textwidth_pels = DrawQ_TextWidth("123456789012345678", maxlen_0, sizehere, 
				sizehere, q_ignore_color_codes_false, FONT_CONSOLE);

			{
				int curx = vid_conwidth.integer - (textwidth_pels + con_textsize.value * 2.5);
				int x = curx - 2;
				int y = cury - 2;
				int w = textwidth_pels + 2 * 2;
				//int h = sizehere + 2 * 2;
				int h = ceil(ceil(sizehere) * 1.2) * (ARRAY_COUNT(ss));
				DrawQ_Fill (x-4, y-4, w+8, h+8, 0,0,0, alpha_0_5, DRAWFLAG_NORMAL_0);	// h

			}
			for (int idx = 0; idx < ARRAY_COUNT(ss); idx ++) {
				ccs *sxy = ss[idx];
				//if (!sxy[0])
				//	continue;
				//int textwidth_pels = DrawQ_TextWidth(sxy, maxlen_0, sizehere, 
				//	sizehere, q_ignore_color_codes_false, FONT_CONSOLE);
				
				int curx = vid_conwidth.integer - (textwidth_pels + con_textsize.value * 2.5);
				int x = curx - 2;
				int y = cury - 2;
				int w = textwidth_pels + 2 * 2;
				int h = sizehere + 2 * 2;
				//DrawQ_Fill (x-4, y-4, w+8, h+8, 0,0,0, alpha_1_0, DRAWFLAG_NORMAL_0);	// h
				DrawQ_Fill (x, y, w, h, 0.12, 0.09, 0.04, alpha_1_0, DRAWFLAG_NORMAL_0);	// h
				

				float xafter = DrawQ_String_Scale (curx, cury, sxy, maxlen_0, 
					sizehere, sizehere, scale_1_0,
					scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, 
					ignorecolorcodes_false, FONT_CONSOLE );

				cury += ceil(ceil(sizehere) * 1.2);
			} // for
				

			break;
		}	
	} // ! shift selection
	if (con_mutex) Thread_UnlockMutex(con_mutex);
}


/*
	Con_DisplayList

	New function for tab-completion system
	Added by EvilTypeGuy
	MEGA Thanks to Taniwha

*/
void Con_DisplayList(const char **list)
{
	int i = 0, pos = 0, len = 0, maxlen = 0, width = (con_linewidth - 4);
	const char **walk = list;

	while (*walk) {
		len = (int)strlen(*walk);
		if (len > maxlen)
			maxlen = len;
		walk++;
	}
	maxlen += 1;

	while (*list) {
		len = (int)strlen(*list);
		if (pos + maxlen >= width) {
			Con_Print("\n");
			pos = 0;
		}

		Con_Print(*list);
		for (i = 0; i < (maxlen - len); i++)
			Con_Print(" ");

		pos += maxlen;
		list++;
	}

	if (pos)
		Con_Print("\n\n");
}


// Now it becomes TRICKY :D --blub
static char Nicks_list[MAX_SCOREBOARD_255][MAX_SCOREBOARDNAME_128];	// contains the nicks with colors and all that
static char Nicks_sanlist[MAX_SCOREBOARD_255][MAX_SCOREBOARDNAME_128];	// sanitized list for completion when there are other possible matches.
// means: when somebody uses a cvar's name as his name, we won't ever get his colors in there...
static int Nicks_offset[MAX_SCOREBOARD_255]; // when nicks use a space, we need this to move the completion list string starts to avoid invalid memcpys
static int Nicks_matchpos;

// co against <<:BLASTER:>> is true!?
static int Nicks_strncasecmp_nospaces(char *a, char *b, unsigned int a_len)
{
	while(a_len)
	{
		if (tolower(*a) == tolower(*b))
		{
			if (*a == 0)
				return 0;
			--a_len;
			++a;
			++b;
			continue;
		}
		if (!*a)
			return -1;
		if (!*b)
			return 1;
		if (*a == ' ')
			return (*a < *b) ? -1 : 1;
		if (*b == ' ')
			++b;
		else
			return (*a < *b) ? -1 : 1;
	}
	return 0;
}
static int Nicks_strncasecmp(char *a, char *b, unsigned int a_len)
{
	char space_char;
	if (!(con_nickcompletion_flags.integer & NICKS_ALPHANUMERICS_ONLY))
	{
		if (con_nickcompletion_flags.integer & NICKS_NO_SPACES)
			return Nicks_strncasecmp_nospaces(a, b, a_len);
		return strncasecmp(a, b, a_len);
	}

	space_char = (con_nickcompletion_flags.integer & NICKS_NO_SPACES) ? 'a' : ' ';

	// ignore non alphanumerics of B
	// if A contains a non-alphanumeric, B must contain it as well though!
	while(a_len)
	{
		qbool alnum_a, alnum_b;

		if (tolower(*a) == tolower(*b))
		{
			if (*a == 0) // end of both strings, they're equal
				return 0;
			--a_len;
			++a;
			++b;
			continue;
		}
		// not equal, end of one string?
		if (!*a)
			return -1;
		if (!*b)
			return 1;
		// ignore non alphanumerics
		alnum_a = ( (*a >= 'a' && *a <= 'z') || (*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9') || *a == space_char);
		alnum_b = ( (*b >= 'a' && *b <= 'z') || (*b >= 'A' && *b <= 'Z') || (*b >= '0' && *b <= '9') || *b == space_char);
		if (!alnum_a) // b must contain this
			return (*a < *b) ? -1 : 1;
		if (!alnum_b)
			++b;
		// otherwise, both are alnum, they're just not equal, return the appropriate number
		else
			return (*a < *b) ? -1 : 1;
	}
	return 0;
}


/* Nicks_CompleteCountPossible

   Count the number of possible nicks to complete
 */
static int Nicks_CompleteCountPossible(char *line, int pos, char *s, qbool isCon)
{
	char name[MAX_SCOREBOARDNAME_128];
	int i, p;
	int match;
	int spos;
	int count = 0;

	if (!con_nickcompletion.integer)
		return 0;

	// changed that to 1
	if (!line[0])// || !line[1]) // we want at least... 2 written characters
		return 0;

	for(i = 0; i < cl.maxclients; ++i)
	{
		p = i;
		if (!cl.scores[p].name[0])
			continue;

		SanitizeString(cl.scores[p].name, name);
		//Con_Printf(" ^2Sanitized: ^7%s -> %s", cl.scores[p].name, name);

		if (!name[0])
			continue;

		match = -1;
		spos = pos - 1; // no need for a minimum of characters :)

		while(spos >= 0)
		{
			if (spos > 0 && line[spos-1] != ' ' && line[spos-1] != ';' && line[spos-1] != '\"' && line[spos-1] != '\'')
			{
				if (!(isCon && spos == 1)) // console start
				{
					--spos;
					continue;
				}
			}
			if (isCon && spos == 0)
				break;
			if (Nicks_strncasecmp(line+spos, name, pos-spos) == 0)
				match = spos;
			--spos;
		}
		if (match < 0)
			continue;
		//Con_Printf("Possible match: %s|%s\n", cl.scores[p].name, name);
		strlcpy(Nicks_list[count], cl.scores[p].name, sizeof(Nicks_list[count]));

		// the sanitized list
		strlcpy(Nicks_sanlist[count], name, sizeof(Nicks_sanlist[count]));
		if (!count)
		{
			Nicks_matchpos = match;
		}

		Nicks_offset[count] = s - (&line[match]);
		//Con_PrintLinef ("offset for %s: %d", name, Nicks_offset[count]);

		++count;
	}
	return count;
}

static void Cmd_CompleteNicksPrint(int count)
{
	int i;
	for(i = 0; i < count; ++i)
		Con_Printf("%s\n", Nicks_list[i]);
}

static void Nicks_CutMatchesNormal(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	c = (unsigned int)strlen(Nicks_sanlist[0]) - 1;
	for(i = 1; i < count; ++i)
	{
		l = (unsigned int)strlen(Nicks_sanlist[i]) - 1;
		if (l < c)
			c = l;

		for(l = 0; l <= c; ++l)
			if (tolower(Nicks_sanlist[0][l]) != tolower(Nicks_sanlist[i][l]))
			{
				c = l-1;
				break;
			}
	}
	Nicks_sanlist[0][c+1] = 0;
	//Con_Printf("List0: %s\n", Nicks_sanlist[0]);
}

static unsigned int Nicks_strcleanlen(const char *s)
{
	unsigned int l = 0;
	while(*s)
	{
		if ( (*s >= 'a' && *s <= 'z') ||
		    (*s >= 'A' && *s <= 'Z') ||
		    (*s >= '0' && *s <= '9') ||
		    *s == ' ')
			++l;
		++s;
	}
	return l;
}

static void Nicks_CutMatchesAlphaNumeric(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	char tempstr[sizeof(Nicks_sanlist[0])];
	char *a, *b;
	char space_char = (con_nickcompletion_flags.integer & NICKS_NO_SPACES) ? 'a' : ' '; // yes this is correct, we want NO spaces when no spaces

	c = (unsigned int)strlen(Nicks_sanlist[0]);
	for(i = 0, l = 0; i < (int)c; ++i)
	{
		if ( (Nicks_sanlist[0][i] >= 'a' && Nicks_sanlist[0][i] <= 'z') ||
		    (Nicks_sanlist[0][i] >= 'A' && Nicks_sanlist[0][i] <= 'Z') ||
		    (Nicks_sanlist[0][i] >= '0' && Nicks_sanlist[0][i] <= '9') || Nicks_sanlist[0][i] == space_char) // this is what's COPIED
		{
			tempstr[l++] = Nicks_sanlist[0][i];
		}
	}
	tempstr[l] = 0;

	for(i = 1; i < count; ++i)
	{
		a = tempstr;
		b = Nicks_sanlist[i];
		while(1)
		{
			if (!*a)
				break;
			if (!*b)
			{
				*a = 0;
				break;
			}
			if (tolower(*a) == tolower(*b))
			{
				++a;
				++b;
				continue;
			}
			if ( (*b >= 'a' && *b <= 'z') || (*b >= 'A' && *b <= 'Z') || (*b >= '0' && *b <= '9') || *b == space_char)
			{
				// b is alnum, so cut
				*a = 0;
				break;
			}
			++b;
		}
	}
	// Just so you know, if cutmatchesnormal doesn't kill the first entry, then even the non-alnums fit
	Nicks_CutMatchesNormal(count);
	//if (!Nicks_sanlist[0][0])
	if (Nicks_strcleanlen(Nicks_sanlist[0]) < strlen(tempstr))
	{
		// if the clean sanitized one is longer than the current one, use it, it has crap chars which definitely are in there
		strlcpy(Nicks_sanlist[0], tempstr, sizeof(Nicks_sanlist[0]));
	}
}

static void Nicks_CutMatchesNoSpaces(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	char tempstr[sizeof(Nicks_sanlist[0])];
	char *a, *b;

	c = (unsigned int)strlen(Nicks_sanlist[0]);
	for(i = 0, l = 0; i < (int)c; ++i)
	{
		if (Nicks_sanlist[0][i] != ' ') // here it's what's NOT copied
		{
			tempstr[l++] = Nicks_sanlist[0][i];
		}
	}
	tempstr[l] = 0;

	for(i = 1; i < count; ++i)
	{
		a = tempstr;
		b = Nicks_sanlist[i];
		while(1)
		{
			if (!*a)
				break;
			if (!*b)
			{
				*a = 0;
				break;
			}
			if (tolower(*a) == tolower(*b))
			{
				++a;
				++b;
				continue;
			}
			if (*b != ' ')
			{
				*a = 0;
				break;
			}
			++b;
		}
	}
	// Just so you know, if cutmatchesnormal doesn't kill the first entry, then even the non-alnums fit
	Nicks_CutMatchesNormal(count);
	//if (!Nicks_sanlist[0][0])
	//Con_Printf("TS: %s\n", tempstr);
	if (Nicks_strcleanlen(Nicks_sanlist[0]) < strlen(tempstr))
	{
		// if the clean sanitized one is longer than the current one, use it, it has crap chars which definitely are in there
		strlcpy(Nicks_sanlist[0], tempstr, sizeof(Nicks_sanlist[0]));
	}
}

static void Nicks_CutMatches(int count)
{
	if (con_nickcompletion_flags.integer & NICKS_ALPHANUMERICS_ONLY)
		Nicks_CutMatchesAlphaNumeric(count);
	else if (con_nickcompletion_flags.integer & NICKS_NO_SPACES)
		Nicks_CutMatchesNoSpaces(count);
	else
		Nicks_CutMatchesNormal(count);
}

static const char **Nicks_CompleteBuildList(int count)
{
	const char **buf;
	int bpos = 0;
	// the list is freed by Con_CompleteCommandLine, so create a char **
	buf = (const char **)Mem_Alloc(tempmempool, count * sizeof(const char *) + sizeof (const char *));

	for(; bpos < count; ++bpos)
		buf[bpos] = Nicks_sanlist[bpos] + Nicks_offset[bpos];

	Nicks_CutMatches(count);

	buf[bpos] = NULL;
	return buf;
}

/*
	Nicks_AddLastColor
	Restores the previous used color, after the autocompleted name.
*/
static int Nicks_AddLastColor(char *buffer, int pos)
{
	qbool quote_added = false;
	int match;
	int color = STRING_COLOR_DEFAULT + '0';
	char r = 0, g = 0, b = 0;

	if (con_nickcompletion_flags.integer & NICKS_ADD_QUOTE && buffer[Nicks_matchpos-1] == '\"')
	{
		// we'll have to add a quote :)
		buffer[pos++] = '\"';
		quote_added = true;
	}

	if ((!quote_added && con_nickcompletion_flags.integer & NICKS_ADD_COLOR) || con_nickcompletion_flags.integer & NICKS_FORCE_COLOR)
	{
		// add color when no quote was added, or when flags &4?
		// find last color
		for(match = Nicks_matchpos-1; match >= 0; --match)
		{
			if (buffer[match] == STRING_COLOR_TAG)
			{
				if ( isdigit(buffer[match+1]) )
				{
					color = buffer[match+1];
					break;
				}
				else if (buffer[match+1] == STRING_COLOR_RGB_TAG_CHAR)
				{
					if ( isxdigit(buffer[match+2]) && isxdigit(buffer[match+3]) && isxdigit(buffer[match+4]) )
					{
						r = buffer[match+2];
						g = buffer[match+3];
						b = buffer[match+4];
						color = -1;
						break;
					}
				}
			}
		}
		if (!quote_added)
		{
			if ( pos >= 2 && buffer[pos-2] == STRING_COLOR_TAG && isdigit(buffer[pos-1]) ) // when thes use &4
				pos -= 2;
			else if ( pos >= 5 && buffer[pos-5] == STRING_COLOR_TAG && buffer[pos-4] == STRING_COLOR_RGB_TAG_CHAR
					 && isxdigit(buffer[pos-3]) && isxdigit(buffer[pos-2]) && isxdigit(buffer[pos-1]) )
				pos -= 5;
		}
		buffer[pos++] = STRING_COLOR_TAG;
		if (color == -1)
		{
			buffer[pos++] = STRING_COLOR_RGB_TAG_CHAR;
			buffer[pos++] = r;
			buffer[pos++] = g;
			buffer[pos++] = b;
		}
		else
			buffer[pos++] = color;
	}
	return pos;
}


// return value is keyline_pos (trivally used to set keyline_pos)
// is_console better be true, this is not made for chat mode
// is_from_nothing means someone did "map " and pressed CTRL-SPACE
//       and we are autocompleting from thin air completing from 100% of context (so if would be all maps, for instance)

int GetEffectList_Count (const char *s_prefix);
int Con_CompleteCommandLine_Zircon(cmd_state_t *cmd, qbool is_console, qbool is_shifted, qbool is_from_nothing)
{
	autocomplete_t *ac = &_g_autocomplete;
	char *oldspartial = ac->s_search_partial_a;
	char *must_end_autocomplete = NULL;

	// Baker: Due to our goto hijinx, let's make sure these get zero filled here ...
	const char **list[4] = {0, 0, 0, 0};
	const char *s_match = "";

	//#pragma message ("Baker: So many string functions and actions here are likely not UTF8 safe")
	//#pragma message ("and neither is the standard DarkPlaces autocomplete, but ours does more and uses more functions")

	// We are continuing a previous autocomplete
	if (ac->s_search_partial_a)
		goto autocomplete_go;

	ac->p_text_partial_start		= NULL;
	ac->p_text_completion_start		= NULL;
	ac->p_text_beyond_autocomplete	= NULL;
	ac->is_at_first_arg				= 0;
	ac->is_from_nothing				= is_from_nothing;
	ac->searchtype					= 0;
	ac->search_partial_offset		= 0;
	freenull_ (ac->s_command0_a);
	freenull_ (ac->s_completion_a);

	// NEW AUTOCOMPLETE

	// Look back from cursor and find semi-colon to locate start of current search term
	int search_partial_offset = key_linepos;

	while (--search_partial_offset) {
		int k = key_line[search_partial_offset];
		if (k == '\"' || k == ';' || k == ' ' || k == '\'')
			break;
	}
	search_partial_offset ++;

	ac->search_partial_offset = search_partial_offset;


	// Baker: October 30 2024 - This is START OF ITEM TO COMPLETE "map e4" is "e4"
	ac->p_text_partial_start = &key_line[ac->search_partial_offset];		// what we wish to find

	// Baker: October 30 2024 - This is BEYOND THE CURRENT SEARCH TERM.
	ac->p_text_beyond_autocomplete = &key_line[key_linepos];
	setstr (ac->text_after_autocomplete_a, ac->p_text_beyond_autocomplete);

	//
	// LINE TERMINATED AT CURSOR START ...
	//
	int saved_cursor_char = ac->p_text_beyond_autocomplete[0];
	ac->p_text_beyond_autocomplete[0] = NULL_CHAR_0;

	// Sys_PrintToTerminal2 (va3("new Partial Set to: " QUOTED_S "\n", spartial512));
	if (ac->p_text_partial_start[0] == 0 && ac->is_from_nothing == false) {
		// Rejected empty autocomplete.
		ac->p_text_beyond_autocomplete[0] = saved_cursor_char;
exit_possible:
		return key_linepos; // Does this happen? "map " <-- press TAB, yes
	}

	// This process only works because we null terminated at cursor
	setstr (ac->s_search_partial_a, ac->p_text_partial_start);

	char *start_command = &key_line[1]; // After bracket "]map e2"
	// dp_strstr_reverse is REVERSE strstr

	// Baker: October 30 2024 - This works due to null termination above.
	char *s_last_semicolon_before = dp_strstr_reverse(&key_line[1], ";"); 

	// "color 4; map e2" --> in this situation, s_command0 is "map" not "color"
	if (s_last_semicolon_before)
		start_command = String_Skip_WhiteSpace_Including_Space(&s_last_semicolon_before[1]);

	// Baker: October 30 2024 - Last
#define MAX_WORDS_16 16
	char words_count = 0;
	char words[MAX_WORDS_16][256];
	char whole_line[1024];
	c_strlcpy (whole_line, start_command);
	ccs *data = start_command;
	for (int idx = 0; idx < MAX_WORDS_16; idx ++) {
		if (!COM_ParseToken_Console (&data))
			break;
		//size_t sz =sizeof(words[idx]); // Veryify is 256 ... it is verified
		c_strlcpy (words[idx], com_token);
		words_count ++;
	}

	// Baker: words_count - 1 is where we are

	int in_arg_number = words_count - 1; // 0 is the command

	if (isin2(*ac->p_text_partial_start, NULL_CHAR_0, SPACE_CHAR_32)) {
		// "map " .. shows 1 arg but we are completing #2 from nothing.
		in_arg_number ++;
	}

	char *space = strchr(&start_command[1], ' '); // Find first space after the command

	// Check if the first space after the command is where the partial is
	// "map "
	ac->is_at_first_arg = space && ac->p_text_partial_start == &space[1];

	if (space) {
		int saved = space[0];
		space[0] = NULL_CHAR_0;
		setstr (ac->s_command0_a, start_command); // Length?
		space[0] = saved;
	}

	ac->is_at_first_arg = ac->s_command0_a && &space[1] == ac->p_text_partial_start;

	// Determine search type
	char *command = ac->s_command0_a;

	//in_arg_number = ac->is_at_first_arg ?
	//if (ac->is_at_first_arg) {
	switch (in_arg_number) {
	case 0: // Command
		break;

	case 1: // Arg 1
		// First arg.

			 if (String_Isin3 (command, "map", "changelevel", "changelevel2") )	ac->searchtype = 1;
		else if (String_Isin3 (command, "save", "load", "jpegextract" ) )		ac->searchtype = 2;
		else if (String_Isin3 (command, "playdemo", "record", "timedemo") )		ac->searchtype = 3;
		else if (String_Isin2 (command, "game", "gamdir")		)				ac->searchtype = 4;
		else if (String_Isin2 (command, "exec", "saveconfig")	)				ac->searchtype = 5;
		else if (String_Isin2 (command, "sky", "loadsky") /**/)					ac->searchtype = 6;
		else if (String_Isin1 (command, "gl_texturemode") /**/)					ac->searchtype = 7;
		else if (String_Isin1 (command, "copy") /**/)							ac->searchtype = 8;
		else if (String_Isin1 (command, "edicts") /**/)							ac->searchtype = 9;
		else if (String_Isin1 (command, "r_editlights_edit") /**/)				ac->searchtype = 10;
		else if (String_Isin1 (command, "sv_cmd") /**/)							ac->searchtype = 11;
		else if (String_Isin1 (command, "cl_cmd") /**/)							ac->searchtype = 12;
//			else if (String_Isin1 (command, "menu_cmd") /**/)						ac->searchtype = 13;
		else if (String_Isin2 (command, "modelprecache","modeldecompile") )		ac->searchtype = 14;
		else if (String_Isin2 (command, "play","playvol") /**/)					ac->searchtype = 15;
		else if (String_Isin2 (command, "r_replacemaptexture", "texturefindpos") /**/)			ac->searchtype = 16;
		else if (String_Isin2 (command, "bind","unbind") /**/)					ac->searchtype = 17;
		else if (String_Isin4 (command, "folder","dir","ls", "jpegsplit") /**/)	ac->searchtype = 18;
		else if (String_Isin1 (command, "cvarlist") /**/)						ac->searchtype = 20;
		else if (String_Isin1 (command, "sv_protocolname") /**/)				ac->searchtype = 21;
		else if (String_Isin1 (command, "loadfont"))							ac->searchtype = 22;
		else if (String_Isin4 (command, "showmodel", "objmodeladjust", "objmodelsplit", "playermodel") /**/)	ac->searchtype = 23;
		else if (String_Isin1 (command, "envmap") /**/)							ac->searchtype = 24;
		else if (String_Isin1 (command, "zipinfo") /**/)						ac->searchtype = 25;
		else if (String_Isin1 (command, "parse") /**/)							ac->searchtype = 26;
		else if (String_Isin1 (command, "shaderprint") /**/)					ac->searchtype = 27;
		else if (String_Isin2 (command, "effectinfo_dump", "effectinfo_list"))	ac->searchtype = 28;
		else if (String_Isin1 (command, "playvideo"))							ac->searchtype = 29;
		else if (String_Isin1 (command, "zf"))									ac->searchtype = 31;
		else if (String_Isin1 (command, "printfile"))							ac->searchtype = 32;
		else if (String_Isin1 (command, "vegetationmake"))						ac->searchtype = 33;
		else if (String_Isin2 (command, "giftoshader","gifclip"))				ac->searchtype = 34;
		//else if (String_Isin1 (command, "objmake"))								ac->searchtype = 35;
		else if (String_Isin1 (command, "barmake"))								ac->searchtype = 23; // showmodel completion
		else if (String_Isin1 (command, "which"))								ac->searchtype = 36; // showmodel completion
		else if (String_Isin2 (command, "prvm_edictset", "prvm_edictget") /**/) ac->searchtype = 38; // fields?
		
//			else if (String_Isin1 (command, "loadfont"))							
//				ac->searchtype = 30;

		break;
	
	case 2:
		// SECOND ARGUMENT "map e4"
	//else if (ac->s_command0_a) {
		// We are 2nd argument or further down (or someone typed multiple spaces that's on them)
	         if (String_Isin1 (command, "r_replacemaptexture") /**/)			ac->searchtype = 19;
		else if (String_Isin1 (command, "loadfont") /**/)						ac->searchtype = 30;
		else if (String_Isin1 (command, "barmake") /**/)						ac->searchtype = 33; // .tga, .jpg, .png
		else if (String_Isin1 (command, "eset") /**/)							ac->searchtype = 37; // fields?
		 // end
		break;

	case 3:
		// 0    1   2
		// eset 30 origin
		// 0             1      2 3      4
		// prvm_edictset server # origin 5
		if (String_Isin2 (command, "prvm_edictset", "prvm_edictget") /**/) ac->searchtype = 37; // fields?
		break;

	default:
		break;
	} // sw

	ac->p_text_beyond_autocomplete[0] = saved_cursor_char;

	//
	// LINE TERMINATED AT CURSOR END ...
	//


autocomplete_go:

	// Reset these
	freenull_(ac->s_match_after_a);		freenull_(ac->s_match_alphalast_a);
	freenull_(ac->s_match_alphatop_a);	freenull_(ac->s_match_before_a);

	char *s = ac->s_search_partial_a;

	// First time autocomplete prints a list, 2nd time does not ..
	int is_quiet = oldspartial ? true : false;

	if (ac->searchtype)	{
		extern cvar_t prvm_sv_gamecommands; // Baker r7103 gamecommand autocomplete
		extern cvar_t prvm_cl_gamecommands; // Baker r7103 gamecommand autocomplete
		//extern cvar_t prvm_menu_gamecommands;

		switch (ac->searchtype) {
		case 1:  GetMapList				(s, q_reply_buf_NULL, q_reply_size_0, q_is_menu_fill_false, q_is_zautocomplete_true, is_quiet);	break;
		case 2:  GetFileList_Count		(q_folder_NULL, s, ".sav", q_strip_exten_true); break;
		case 3:  GetFileList_Count		(q_folder_NULL, s, ".dem", q_strip_exten_true); break;
		case 4:  GetModList_Count		(s);	break; // game
		case 5:  GetFileList_Count		(q_folder_NULL, s, ".cfg", q_strip_exten_false); break;
		case 6:  GetSkyList_Count		(s);	break;
		case 7:  GetTexMode_Count		(s);	break;
		case 8:  GetCommad_Count		(s, "ents,tex"); break;
		case 9:  GetEdictsCmd_Count		(s);	break;
		case 10: GetREditLightsEdit_Count(s);	break;
		case 11: GetGameCommands_Count  (s, prvm_sv_gamecommands.string);	break; // Baker r7103 gamecommand autocomplete
		case 12: GetGameCommands_Count  (s, prvm_cl_gamecommands.string);	break; // Baker r7103 gamecommand autocomplete
//		case 13: GetGameCommands_Count  (s, prvm_menu_gamecommands.string);	break;
		case 14: 
				GetModelPrecacheList_Count (s); break;
				//GetModelList_Count		(s);	break; // modelprecache, modeldecompile
		case 15: GetSoundList_Count		(s);	break; // "play" sound command
		case 16: GetTexWorld_Count		(s);	break; // r_replacemaptexture arg1 world textures
		case 17: GetKeyboardList_Count	(s);	break; // "bind", "unbind"
		case 18: GetFolderList_Count	(s);	break; // "dir", "ls", "folder"
		case 19: GetTexGeneric_Count	(s);	break; // r_replacemaptexture arg2 general textures
		case 20: GetCommad_Count		(s, "audit,changed,owned,newdefault"); break; // cvarlist "changed"
		case 21: GetCommad_Count		(s, "666,999,dp7,quake"); break; // sv_protocolname
		case 22: GetCommad_Count		(s, "centerprint,chat,console,default,infobar,menu,notify,sbar"); break;
		case 23: GetShowModelList_Count	(s);	break;
		case 24: GetCommad_Count		(s, "auto"); break;
		case 25: GetFileList_Count		(q_folder_NULL, s, ".zip", q_strip_exten_false); break;
		case 26: GetCommad_Count		(s, "clipboard"); break;
		case 27: GetShaderList_Count	(s); break;
		case 28: GetEffectList_Count	(s); break;
		case 29: GetVideoList_Count		(s /*".gif;*.jpg;*.dpv"*/); break;
		case 30: GetFileList_Count		(q_folder_NULL, s, ".ttf", q_strip_exten_false); break; // loadfont
		case 31: GetFileList_Count		("engine/", s, ".txt", q_strip_exten_false); break; // zf
		case 32: GetFileList_Count		(q_folder_NULL, s, "" /*.dem"*/, q_strip_exten_false); break; // printfile
		case 33: GetShowTGAJPGPNGList_Count	(s);	break;
		case 34: GetAnimatedGIF_Count	(s);	break;
		case 35: GetFileList_Count		(q_folder_NULL, s, "*.txt", q_strip_exten_false); break; // objmake source looks for .txt
		case 36: GetFileList_Count		(q_folder_NULL, s, "", q_strip_exten_false); break;
		case 37: 
			GetFieldListServer_Count		(s); 
			break;
		case 38: GetCommad_Count		(s, "client,menu,server"); break; // sv_protocolname
		} // switch

		if (ac->s_match_alphatop_a == NULL) {
exit_possible2:
			must_end_autocomplete = "No possible matches";
			goto exit_out; // No possible matches
		}
		goto search_completed;
	} // searchtype

	int c;
unidentified:
	// Unidentified completion - do cvars/commands/aliases -- count number of possible matches and print them
	c = Cmd_CompleteCountPossible(cmd, s, ac->is_from_nothing);
	if (c && is_quiet == false) {
		Con_PrintLinef (NEWLINE "%d possible command%s", c, (c > 1) ? "s: " : ":");
		Cmd_CompleteCommandPrint(cmd, s, ac->is_from_nothing);
	}
	int v = Cvar_CompleteCountPossible(cmd->cvars, s, CF_CLIENT | CF_SERVER, ac->is_from_nothing);
	if (v && is_quiet == false) {
		Con_PrintLinef (NEWLINE "%d possible variable%s", v, (v > 1) ? "s: " : ":");
		Cvar_CompleteCvarPrint(cmd->cvars, s, CF_CLIENT | CF_SERVER, is_from_nothing);
	}
	int a = Cmd_CompleteAliasCountPossible(cmd, s, ac->is_from_nothing);
	if (a && is_quiet == false) {
		Con_PrintLinef (NEWLINE "%d possible alias%s", a, (a > 1) ? "es: " : ":");
		Cmd_CompleteAliasPrint(cmd, s, ac->is_from_nothing);
	}

	if (c + v + a == 0) {
		// No possible matches
		must_end_autocomplete = "No possible alias, command, variable match";
		goto exit_out;
	}

	if (c)	s_match = *(list[0] = Cmd_CompleteBuildList(cmd, s, ac->is_from_nothing));
	if (v)	s_match = *(list[1] = Cvar_CompleteBuildList(cmd->cvars, s, CF_CLIENT | CF_SERVER, ac->is_from_nothing));
	if (a)	s_match = *(list[2] = Cmd_CompleteAliasBuildList(cmd, s, ac->is_from_nothing));

	int common_length, j;
	for (common_length = (int)strlen(s); ; common_length++) {
		const char **listitems;
		for (j = 0; j < 3; j++) {
			if (list[j]) {
				for (listitems = list[j]; *listitems; listitems ++) {
					if ((*listitems)[common_length] != s_match[common_length]) {
						goto search_completed;
					}
				} // for
			} // if
		} // for

		// all possible matches share this character, so we continue...
		if (s_match[common_length] == NULL_CHAR_0) {
			// if all matches ended at the same position, stop (this means there is only one match)
			goto search_completed;
		}
	} // for

search_completed:

	// SET COMPLETION
	if (oldspartial == NULL) {
		// No previous autocomplete, we are doing it for the first time so pick the alpha top
		setstr (ac->s_completion_a, ac->s_match_alphatop_a);
	} else if (is_shifted) {
		// We are shifted so use the "alpha before"
		if (ac->s_match_before_a == NULL) {
			 // Nothing before this item, loop around to last
			setstr (ac->s_completion_a, ac->s_match_alphalast_a);
		} else {
			setstr (ac->s_completion_a, ac->s_match_before_a);
		}
	} else {
		// We are not shifted, use the "alpha after"
		if (ac->s_match_after_a == NULL) {
			// Nothing after this item, loop around to top
			setstr (ac->s_completion_a, ac->s_match_alphatop_a);
		} else {
			setstr (ac->s_completion_a, ac->s_match_after_a);
		}
	}

	// We may skip printing if we only had one match, we still do the completion
	char *fill;
	int actual_len;
	int fill_len;
one_match_skip:

	// Baker: We are always adding a space after the autocomplete.

	fill = ac->s_completion_a;
	actual_len = (int)strlen(fill);
 	fill_len = actual_len + ONE_CHAR_1;

	// prevent a buffer overrun by limiting cmd_len according to remaining space
	fill_len = Smallest(fill_len, (int)sizeof(key_line) - ONE_CHAR_1 - ac->search_partial_offset);
	if (fill) {
		// start of complete .. copy the "cmd" (completion) over
		key_linepos = ac->search_partial_offset;
		memcpy (&key_line[key_linepos], fill, actual_len);
		key_line[key_linepos + fill_len - ONE_CHAR_1] = SPACE_CHAR_32;
		key_linepos += fill_len;

		// if there is only one match, add a space after it
		int is_only_one = oldspartial == NULL
			&& String_Match (ac->s_match_alphalast_a, ac->s_match_alphatop_a);
		if (key_linepos < (int)sizeof(key_line) - 1 && is_only_one) {
			// Only first partial complete shall do this
//			key_line[key_linepos ++] = ' ';
			must_end_autocomplete = "Only one match";
		} // if
	}

	// use strlcat to avoid a buffer overrun
	key_line[key_linepos] = 0;
	c_strlcat (key_line, ac->text_after_autocomplete_a);

	// free the command, cvar, and alias lists
	for (j = 0; j < 4; j++) {
		if (list[j]) Mem_Free((void *)list[j]);
	}

exit_out:

	if (must_end_autocomplete) {
		// Search terminated, likely due to ZERO or ONE result
		// Clear stuff
		Partial_Reset ();
	} else {
		// Save stuff

	}
	return key_linepos;
}

/*
	Con_CompleteCommandLine

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
	Enhanced to tab-complete map names by [515]

*/
int Con_CompleteCommandLine(cmd_state_t *cmd, qbool is_console)
{
	const char *cxtext = "";
	char *s;
	const char **list[4] = {0, 0, 0, 0};
	char s2[512];
	char command[512];
	int c, v, a, i, cmd_len, pos, k;
	int n; // nicks --blub
	const char *space, *patterns;
	char vabuf[1024];

	char *line;
	int linestart, linepos;
	unsigned int linesize;
	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
		linesize = sizeof(key_line);
		linestart = 1;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
		linesize = sizeof(chat_buffer);
		linestart = 0;
	}

	//find what we want to complete
	pos = linepos;
	while(--pos >= linestart)
	{
		k = line[pos];
		if (k == '\"' || k == ';' || k == ' ' || k == '\'')
			break;
	}
	pos++;

	s = line + pos;
	strlcpy(s2, line + linepos, sizeof(s2)); //save chars after cursor
	line[linepos] = 0; //hide them

	c = v = a = n = cmd_len = 0;
	// DarkPlaces Beta only completes names in chatmode
	if (!is_console)
		goto nicks;

	space = strchr(line + 1, ' ');
	if (space && pos == (space - line) + 1)
	{
		strlcpy(command, line + 1, min(sizeof(command), (unsigned int)(space - line)));

		patterns = Cvar_VariableString(cmd->cvars, va(vabuf, sizeof(vabuf), "con_completion_%s", command), CF_CLIENT | CF_SERVER); // TODO maybe use a better place for this?
		if (patterns && !*patterns)
			patterns = NULL; // get rid of the empty string

		if (String_Match(command, "map") || String_Match(command, "changelevel") || (patterns && String_Match(patterns, "map"))) {
			//maps search
			char t[MAX_QPATH_128];
			if (GetMapList(s, t, sizeof(t), q_is_menu_fill_false, q_is_zautocomplete_false, q_is_suppress_print_false)) {
				// first move the cursor
				linepos += (int)strlen(t) - (int)strlen(s);

				// and now do the actual work
				*s = 0;
				strlcat(line, t, MAX_INPUTLINE_16384);
				strlcat(line, s2, MAX_INPUTLINE_16384); //add back chars after cursor

				// and fix the cursor
				if (linepos > (int) strlen(line))
					linepos = (int) strlen(line);
			}
			return linepos;
		}
		else
		{
			if (patterns) {
				char t[MAX_QPATH_128];
				stringlist_t resultbuf, dirbuf;

				// Usage:
				//   // store completion patterns (space separated) for command foo in con_completion_foo
				//   set con_completion_foo "foodata/*.foodefault *.foo"
				//   foo <TAB>
				//
				// Note: patterns with slash are always treated as absolute
				// patterns; patterns without slash search in the innermost
				// directory the user specified. There is no way to "complete into"
				// a directory as of now, as directories seem to be unknown to the
				// FS subsystem.
				//
				// Examples:
				//   set con_completion_playermodel "models/player/*.zym models/player/*.md3 models/player/*.psk models/player/*.dpm"
				//   set con_completion_playdemo "*.dem"
				//   set con_completion_play "*.wav *.ogg"
				//
				// TODO somehow add support for directories; these shall complete
				// to their name + an appended slash.

				stringlistinit(&resultbuf);
				stringlistinit(&dirbuf);
				while(COM_ParseToken_Simple(&patterns, false, false, true))
				{
					fssearch_t *search;
					if (strchr(com_token, '/'))
					{
						search = FS_Search(com_token, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
					}
					else
					{
						const char *slash = strrchr(s, '/');
						if (slash)
						{
							strlcpy(t, s, min(sizeof(t), (unsigned int)(slash - s + 2))); // + 2, because I want to include the slash
							strlcat(t, com_token, sizeof(t));
							search = FS_Search(t, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
						}
						else
							search = FS_Search(com_token, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
					}
					if (search)
					{
						for(i = 0; i < search->numfilenames; ++i)
							if (!strncmp(search->filenames[i], s, strlen(s)))
								if (FS_FileOrDirectoryType(search->filenames[i]) == FS_FILETYPE_FILE_1)
									stringlistappend(&resultbuf, search->filenames[i]);
						FS_FreeSearch(search);
					}
				}

				// In any case, add directory names
				{
					fssearch_t *search;
					const char *slash = strrchr(s, '/');
					if (slash)
					{
						strlcpy(t, s, min(sizeof(t), (unsigned int)(slash - s + 2))); // + 2, because I want to include the slash
						strlcat(t, "*", sizeof(t));
						search = FS_Search (t, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
					}
					else
						search = FS_Search ("*", fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
					if (search)
					{
						for(i = 0; i < search->numfilenames; ++i)
							if (!strncmp(search->filenames[i], s, strlen(s)))
								if (FS_FileOrDirectoryType(search->filenames[i]) == FS_FILETYPE_DIRECTORY_2)
									stringlistappend(&dirbuf, search->filenames[i]);
						FS_FreeSearch(search);
					}
				}

				if (resultbuf.numstrings > 0 || dirbuf.numstrings > 0)
				{
					const char *p, *q;
					unsigned int matchchars;
					if (resultbuf.numstrings == 0 && dirbuf.numstrings == 1)
					{
						dpsnprintf(t, sizeof(t), "%s/", dirbuf.strings[0]);
					}
					else
					if (resultbuf.numstrings == 1 && dirbuf.numstrings == 0)
					{
						dpsnprintf(t, sizeof(t), "%s ", resultbuf.strings[0]);
					}
					else
					{
						stringlistsort(&resultbuf, true); // dirbuf is already sorted
						Con_PrintLinef (NEWLINE "%d possible filenames", resultbuf.numstrings + dirbuf.numstrings);
						for(i = 0; i < dirbuf.numstrings; ++i)
						{
							Con_Printf("^4%s^7/\n", dirbuf.strings[i]);
						}
						for(i = 0; i < resultbuf.numstrings; ++i)
						{
							Con_Printf("%s\n", resultbuf.strings[i]);
						}
						matchchars = sizeof(t) - 1;
						if (resultbuf.numstrings > 0)
						{
							p = resultbuf.strings[0];
							q = resultbuf.strings[resultbuf.numstrings - 1];
							for(; *p && *p == *q; ++p, ++q);
							matchchars = (unsigned int)(p - resultbuf.strings[0]);
						}
						if (dirbuf.numstrings > 0)
						{
							p = dirbuf.strings[0];
							q = dirbuf.strings[dirbuf.numstrings - 1];
							for(; *p && *p == *q; ++p, ++q);
							matchchars = min(matchchars, (unsigned int)(p - dirbuf.strings[0]));
						}
						// now p points to the first non-equal character, or to the end
						// of resultbuf.strings[0]. We want to append the characters
						// from resultbuf.strings[0] to (not including) p as these are
						// the unique prefix
						strlcpy(t, (resultbuf.numstrings > 0 ? resultbuf : dirbuf).strings[0], min(matchchars + 1, sizeof(t)));
					}

					// first move the cursor
					linepos += (int)strlen(t) - (int)strlen(s);

					// and now do the actual work
					*s = 0;
					strlcat(line, t, MAX_INPUTLINE_16384);
					strlcat(line, s2, MAX_INPUTLINE_16384); //add back chars after cursor

					// and fix the cursor
					if (linepos > (int) strlen(line))
						linepos = (int) strlen(line);
				}
				stringlistfreecontents(&resultbuf);
				stringlistfreecontents(&dirbuf);

				return linepos; // bail out, when we complete for a command that wants a file name
			}
		}
	}

	// Count number of possible matches and print them
	c = Cmd_CompleteCountPossible(cmd, s, q_is_from_nothing_false);
	if (c)
	{
		Con_PrintLinef (NEWLINE "%d possible command%s", c, (c > 1) ? "s: " : ":");
		Cmd_CompleteCommandPrint(cmd, s, q_is_from_nothing_false);
	}
	v = Cvar_CompleteCountPossible(cmd->cvars, s, CF_CLIENT | CF_SERVER, q_is_from_nothing_false);
	if (v)
	{
		Con_PrintLinef (NEWLINE "%d possible variable%s", v, (v > 1) ? "s: " : ":");
		Cvar_CompleteCvarPrint(cmd->cvars, s, CF_CLIENT | CF_SERVER, q_is_from_nothing_false);
	}
	a = Cmd_CompleteAliasCountPossible(cmd, s, q_is_from_nothing_false);
	if (a)
	{
		Con_PrintLinef (NEWLINE "%d possible alias%s", a, (a > 1) ? "es: " : ":");
		Cmd_CompleteAliasPrint(cmd, s, q_is_from_nothing_false);
	}

nicks:
	n = Nicks_CompleteCountPossible(line, linepos, s, is_console);
	if (n)
	{
		Con_PrintLinef (NEWLINE "%d possible nick%s", n, (n > 1) ? "s: " : ":");
		Cmd_CompleteNicksPrint(n);
	}

	if (!(c + v + a + n))	// No possible matches
	{
		if (s2[0])
			strlcpy(&line[linepos], s2, linesize - linepos);
		return linepos;
	}

	if (c)
		cxtext = *(list[0] = Cmd_CompleteBuildList(cmd, s, q_is_from_nothing_false));
	if (v)
		cxtext = *(list[1] = Cvar_CompleteBuildList(cmd->cvars, s, cmd->cvars_flagsmask, q_is_from_nothing_false));
	if (a)
		cxtext = *(list[2] = Cmd_CompleteAliasBuildList(cmd, s, q_is_from_nothing_false));
	if (n)
	{
		if (is_console)
			cxtext = *(list[3] = Nicks_CompleteBuildList(n));
		else
			cxtext = *(Nicks_CompleteBuildList(n));
	}

	for (cmd_len = (int)strlen(s); ; cmd_len ++)
	{
		const char **listitem;
		for (i = 0; i < 3; i++) {
			if (list[i]) {
				for (listitem = list[i];*listitem;listitem++) {
					if ((*listitem)[cmd_len] != cxtext[cmd_len])
						goto done;
				} // for
			} // if
		} // for i

		// all possible matches share this character, so we continue...
		if (!cxtext[cmd_len]) {
			// if all matches ended at the same position, stop
			// (this means there is only one match)
			break;
		}
	}
done:

	// prevent a buffer overrun by limiting cmd_len according to remaining space
	cmd_len = min(cmd_len, (int)linesize - 1 - pos);
	if (cxtext) {
		linepos = pos;
		memcpy(&line[linepos], cxtext, cmd_len);
		linepos += cmd_len;
		// if there is only one match, add a space after it
		if (c + v + a + n == 1 && linepos < (int)linesize - 1)
		{
			if (n)
			{ // was a nick, might have an offset, and needs colors ;) --blub
				linepos = pos - Nicks_offset[0];
				cmd_len = (int)strlen(Nicks_list[0]);
				cmd_len = min(cmd_len, (int)linesize - 3 - pos);

				memcpy(&line[linepos] , Nicks_list[0], cmd_len);
				linepos += cmd_len;
				if (linepos < (int)(linesize - 7)) // space for color code (^[0-9] or ^xrgb), space and \0
					linepos = Nicks_AddLastColor(line, linepos);
			}
			line[linepos++] = ' ';
		}
	}

	// use strlcat to avoid a buffer overrun
	line[linepos] = 0;
	strlcat(line, s2, linesize);

	if (!is_console)
		return linepos;

	// free the command, cvar, and alias lists
	for (i = 0; i < 4; i++)
		if (list[i])
			Mem_Free((void *)list[i]);

	return linepos;
}

void Con_Shutdown (void)
{
	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_Shutdown(&con);
	if (con_mutex) Thread_UnlockMutex(con_mutex);
	if (con_mutex) Thread_DestroyMutex(con_mutex);

	con_mutex = NULL;
}

/*
================
Con_InitOnce
================
*/

#include "jack_scripts.c.h"


void Con_InitOnce (void)
{
	con_linewidth = 80;
	ConBuffer_Init(&con, CON_TEXTSIZE_1_MB, CON_MAXLINES_16384, zonemempool);
	if (Thread_HasThreads())
		con_mutex = Thread_CreateMutex();

	// Allocate a log queue, this will be freed after configs are parsed
	logq_size = MAX_INPUTLINE_16384;
	logqueue = (unsigned char *)Mem_Alloc (tempmempool, logq_size);
	logq_ind = 0;

	Cvar_RegisterVariable (&sys_colortranslation);
	Cvar_RegisterVariable (&sys_specialcharactertranslation);

	Cvar_RegisterVariable (&log_file);
	Cvar_RegisterVariable (&log_file_stripcolors);
	Cvar_RegisterVariable (&log_dest_udp);

	// support for the classic Quake option
// COMMANDLINEOPTION: Console: -condebug logs console messages to zircon_log.log /*qconsole.log*/, see also log_file
#ifdef __ANDROID__
	if (1)
#else
	if (Sys_CheckParm ("-condebug") != 0)
#endif // __ANDROID__
		Cvar_SetQuick (&log_file, "zircon_log.log");

	// register our cvars
	Cvar_RegisterVariable (&con_chat);
	Cvar_RegisterVariable (&con_chatpos);
	Cvar_RegisterVariable (&con_chatrect_x);
	Cvar_RegisterVariable (&con_chatrect_y);
	Cvar_RegisterVariable (&con_chatrect);
	Cvar_RegisterVariable (&con_chatsize);
	Cvar_RegisterVariable (&con_chattime);
	Cvar_RegisterVariable (&con_chatwidth);
	Cvar_RegisterVariable (&con_hints);
	Cvar_RegisterVariable (&con_notify);
	Cvar_RegisterVariable (&con_notifyalign);
	Cvar_RegisterVariable (&con_notifysize);
	Cvar_RegisterVariable (&con_notifytime);
	Cvar_RegisterVariable (&con_textsize);
	Cvar_RegisterVariable (&con_chatsound);

	Cvar_RegisterVariable (&con_logcenterprint); // Baker r1421: centerprint logging to console
	Cvar_RegisterVariable (&con_zircon_autocomplete); // Baker r0062: full auto-completion

	Cvar_RegisterVariable (&con_chatsound_file);
	Cvar_RegisterVariable (&con_chatsound_team_file);
	Cvar_RegisterVariable (&con_chatsound_team_mask);

	// --blub
	Cvar_RegisterVariable (&con_nickcompletion);
	Cvar_RegisterVariable (&con_nickcompletion_flags);

	Cvar_RegisterVariable (&con_completion_playdemo); // *.dem
	Cvar_RegisterVariable (&con_completion_timedemo); // *.dem
	Cvar_RegisterVariable (&con_completion_exec); // *.cfg

	Cvar_RegisterVariable (&condump_stripcolors);

	Cvar_RegisterVariable(&rcon_address);
	Cvar_RegisterVariable(&rcon_secure);
	Cvar_RegisterCallback(&rcon_secure, Con_RCon_ClearPassword_c);
	Cvar_RegisterVariable(&rcon_secure_challengetimeout);
	Cvar_RegisterVariable(&rcon_password);

	// register our commands
	Cmd_AddCommand(CF_CLIENT, "toggleconsole", Con_ToggleConsole_f, "opens or closes the console");
	Cmd_AddCommand(CF_CLIENT, "messagemode", Con_MessageMode_f, "input a chat message to say to everyone");
	Cmd_AddCommand(CF_CLIENT, "messagemode2", Con_MessageMode2_f, "input a chat message to say to only your team");
	Cmd_AddCommand(CF_CLIENT, "commandmode", Con_CommandMode_f, "input a console command");
	Cmd_AddCommand(CF_SHARED, "clear", Con_Clear_f, "clear console history");
	Cmd_AddCommand(CF_SHARED, "maps", Con_Maps_f, "list information about available maps");
	Cmd_AddCommand(CF_SHARED, "condump", Con_ConDump_f, "output console history to a file (see also log_file)");
	Cmd_AddCommand(CF_SHARED, "printfile", _Con_PrintFile_f, "print a file to text [Zircon]");

#ifdef CONFIG_MENU
	// Client
	Cmd_AddCommand (CF_CLIENT, "copy", Con_Copy_f, "copy console history to clipboard scrubbing color colors based on condump_stripcolors.integer [Zircon]"); // Baker r3101: "copy" and "copy ents"
	Cmd_AddCommand (CF_CLIENT, "folder", Con_Folder_f, "open gamedir folder [Zircon]"); // Baker r3102: "folder" command
	Cmd_AddCommand (CF_CLIENT, "jack_scripts", Con_Jack_Scripts_f, "Update scripts/shaderlist.txt and write .shader copies to gamedir/_jack_shaders folder [Zircon]"); // Baker r7105
#else
	// Dedicated server
	// No folder, copy command
#endif

	Cmd_AddCommand (CF_CLIENT, "pos", Con_Pos_f, "print pos and angles [Zircon]"); // Baker r3171: "pos" command

	con_initialized = true;
	Con_DPrintLinef ("Console initialized."); // Baker: less console spam
}

