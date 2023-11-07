// console.h

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

#ifndef CONSOLE_H
#define CONSOLE_H

//
// console
//
extern int con_totallines;
extern int con_backscroll;

extern cvar_t con_zircon_autocomplete;

extern qbool con_initialized;

void Con_Rcon_Redirect_Init(lhnetsocket_t *sock, lhnetaddress_t *dest, qbool proquakeprotocol);
void Con_Rcon_Redirect_End(void);
void Con_Rcon_Redirect_Abort(void);

/// If the line width has changed, reformat the buffer.
void Con_CheckResize (void);
void Con_Init (void);
void Con_Init_Commands (void);
void Con_Shutdown (void);
void Con_DrawConsole (int lines);

void Partial_Reset (void);
void Selection_Line_Reset_Clear (void);

/// Prints to a chosen console target
void Con_MaskPrint(int mask, const char *msg);

// Prints to a chosen console target
void Con_MaskPrintf(int mask, const char *fmt, ...) DP_FUNC_PRINTF(2);

/// Prints to all appropriate console targets, and adds timestamps
void Con_Print(const char *txt);

/// Prints to all appropriate console targets.
void Con_Printf(const char *fmt, ...) DP_FUNC_PRINTF(1);

void Con_PrintLinef(const char *fmt, ...)DP_FUNC_PRINTF(1); // Baker 1009: Jan 19 2022

void Con_LogCenterPrint(const char *str); // Baker 8501

/// A Con_Print that only shows up if the "developer" cvar is set.
void Con_DPrint(const char *msg);

/// A Con_Printf that only shows up if the "developer" cvar is set
void Con_DPrintf(const char *fmt, ...) DP_FUNC_PRINTF(1);
void Con_DPrintLinef(const char *fmt, ...) DP_FUNC_PRINTF(1);
void Con_Clear_f (void);

void Con_HidenotifyPrintLinef(const char *fmt, ...) DP_FUNC_PRINTF(1); //

void Con_DrawNotify (void);

/// Clear all notify lines.
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

int Nicks_CompleteChatLine(char *buffer, size_t size, unsigned int pos);

int GetMapList (const char *s, char *completedname, int completednamebufferlength, int is_menu_fill, int is_autocomplete, int is_suppress_print );

typedef struct {
	unsigned char *sm_a;
	unsigned char *smtru_a;
	unsigned char *smsg_a;
	unsigned char *sqbsp;
} maplist_s;
extern int m_maplist_count;
#define MAXMAPLIST_4096 4096
extern maplist_s m_maplist[MAXMAPLIST_4096];

//=============================================================================
/* MODLIST MENU */
// same limit of mod dirs as in fs.c
#define MODLIST_MAXDIRS 16
extern int modlist_enabled [MODLIST_MAXDIRS];	//array of indexs to modlist
extern int modlist_numenabled;			//number of enabled (or in process to be..) mods

typedef struct modlist_entry_s
{
	qbool loaded;	// used to determine whether this entry is loaded and running
	int enabled;		// index to array of modlist_enabled

	// name of the modification, this is (will...be) displayed on the menu entry
	char name[128];
	// directory where we will find it
	char dir[MAX_QPATH];
} modlist_entry_t;

extern int modlist_cursor;
//static int modlist_viewcount;

extern int modlist_count;
extern modlist_entry_t modlist[MODLIST_TOTALSIZE];

extern int lanConfig_cursor;

/// wrapper function to attempt to either complete the command line
/// or to list possible matches grouped by type
/// (i.e. will display possible variables, aliases, commands
/// that match what they've typed so far)
void Con_CompleteCommandLine_Zircon(int is_shifted, int is_from_nothing);
void Con_CompleteCommandLine(void);

/// Generic libs/util/console.c function to display a list
/// formatted in columns on the console
void Con_DisplayList(const char **list);


/*! \name log
 * @{
 */
void Log_Init (void);
void Log_Close (void);
void Log_Start (void);
void Log_DestBuffer_Flush (void); ///< call this once per frame to send out replies to rcon streaming clients

void Log_Printf(const char *logfilename, const char *fmt, ...) DP_FUNC_PRINTF(2);
//@}

#define CON_WARN ""
#define CON_ERROR ""

// CON_MASK_PRINT is the default (Con_Print/Con_Printf)
// CON_MASK_DEVELOPER is used by Con_DPrint/Con_DPrintf
#define CON_MASK_NONE_0			0
#define CON_MASK_CHAT			1
#define CON_MASK_INPUT			2
#define CON_MASK_DEVELOPER		4
#define CON_MASK_PRINT			8
#define CON_MASK_HIDENOTIFY		128

typedef struct con_lineinfo_s
{
	char *start;
	size_t len;
	int mask;

	/// used only by console.c
	double addtime;
	int height; ///< recalculated line height when needed (-1 to unset)
}
con_lineinfo_t;

typedef struct conbuffer_s
{
	qbool active;
	int textsize;
	char *text;
	int maxlines; // Baker: 4096
	con_lineinfo_t *lines;
	int lines_first;
	int lines_count; ///< cyclic buffer
}
conbuffer_t;

#define CONBUFFER_LINES(buf, i) (buf)->lines[((buf)->lines_first + (i)) % (buf)->maxlines]
#define CONBUFFER_LINES_COUNT(buf) ((buf)->lines_count)
#define CONBUFFER_LINES_LAST(buf) CONBUFFER_LINES(buf, CONBUFFER_LINES_COUNT(buf) - 1)

void ConBuffer_Init(conbuffer_t *buf, int textsize, int maxlines, mempool_t *mempool);
void ConBuffer_Clear (conbuffer_t *buf);
void ConBuffer_Shutdown(conbuffer_t *buf);

void Con_History_Maybe_Push (const char *stext);

/*! Notifies the console code about the current time
 * (and shifts back times of other entries when the time
 * went backwards)
 */
void ConBuffer_FixTimes(conbuffer_t *buf);

/// Deletes the first line from the console history.
void ConBuffer_DeleteLine(conbuffer_t *buf);

/// Deletes the last line from the console history.
void ConBuffer_DeleteLastLine(conbuffer_t *buf);

/// Appends a given string as a new line to the console.
void ConBuffer_AddLine(conbuffer_t *buf, const char *line, int len, int mask);
int ConBuffer_FindPrevLine(conbuffer_t *buf, int mask_must, int mask_mustnot, int start);
int ConBuffer_FindNextLine(conbuffer_t *buf, int mask_must, int mask_mustnot, int start);
const char *ConBuffer_GetLine(conbuffer_t *buf, int i);

void Con_Undo_Clear (void);
// Baker r0004: Ctrl + up/down size console like JoeQuake
extern float console_user_pct;
void Con_AdjustConsoleHeight(const float delta);


#endif // ! CONSOLE_H

