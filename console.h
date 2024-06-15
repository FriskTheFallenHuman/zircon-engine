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

#include <stddef.h>
#include "qtypes.h"
#include "cmd.h"
#include "lhnet.h"

//
// console
//
extern int con_totallines;
extern int con_backscroll;

extern qbool con_initialized;

void Con_Rcon_Redirect_Init(lhnetsocket_t *sock, lhnetaddress_t *dest, qbool proquakeprotocol);
void Con_Rcon_Redirect_End(void);
void Con_Rcon_Redirect_Abort(void);

/// If the line width has changed, reformat the buffer.
void Con_CheckResize (void);
void Con_InitOnce (void);
void Con_Init_Commands (void);
void Con_Shutdown (void);
void Con_DrawConsole (int lines);

/// Prints to a chosen console target
void Con_MaskPrint(int mask, const char *msg);

// Prints to a chosen console target
void Con_MaskPrintf(int mask, const char *fmt, ...) DP_FUNC_PRINTF(2);

/// Prints to all appropriate console targets, and adds timestamps
void Con_Print(const char *txt);

/// Prints to all appropriate console targets.
void Con_Printf (const char *fmt, ...) DP_FUNC_PRINTF(1);

void Con_PrintLinef (const char *fmt, ...) DP_FUNC_PRINTF(1);  // Baker 1009: Jan 19 2022

#define Con_PrintVarInt(var) Con_PrintLinef (STRINGIFY(var) " %d", (int)(var))
#define Con_PrintVarFloat(var) Con_PrintLinef (STRINGIFY(var) " %f", (float)(var))
#define Con_PrintVarString(var) Con_PrintLinef (STRINGIFY(var) " %s", var)
#define Con_PrintVarVector(var) Con_PrintLinef (STRINGIFY(var) " %f %f %f", var[0], var[1], var[2])


void Con_LogCenterPrint(const char *str); // Baker r1421: centerprint logging to console

void Con_HidenotifyPrintLinef(const char *fmt, ...) DP_FUNC_PRINTF(1); // Baker r1421: centerprint logging to console

/// A Con_Print that only shows up if the "developer" cvar is set.
void Con_DPrint(const char *msg);

/// A Con_Printf that only shows up if the "developer" cvar is set
void Con_DPrintf (const char *fmt, ...) DP_FUNC_PRINTF(1);
void Con_DPrintLinef (const char *fmt, ...) DP_FUNC_PRINTF(1);
void Con_Clear_f(cmd_state_t *cmd);

void Con_DrawNotify (void);

/// Clear all notify lines.
void Con_ClearNotify (void);
void Con_ToggleConsole (void);
void Con_CloseConsole_If_Client(void); // // Baker r1003: close console for map/load/etc.

/// wrapper function to attempt to either complete the command line
/// or to list possible matches grouped by type
/// (i.e. will display possible variables, aliases, commands
/// that match what they've typed so far)
int Con_CompleteCommandLine(cmd_state_t *cmd, qbool is_console);
int Con_CompleteCommandLine_Zircon(cmd_state_t *cmd, qbool is_console, qbool is_shifted, qbool is_from_nothing);

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

#define CON_WARN "^3"
#define CON_ERROR "^3"

#define CON_RED "^1"
#define CON_BRONZE "^3"
#define CON_WHITE "^7"

#define CON_GREEN "^2"
#define CON_CYAN "^5"


// CON_MASK_PRINT is the default (Con_Print/Con_Printf)
// CON_MASK_DEVELOPER is used by Con_DPrint/Con_DPrintf
#define CON_MASK_NONE_0			0
#define CON_MASK_CHAT 1
#define CON_MASK_INPUT 2
#define CON_MASK_DEVELOPER 4
#define CON_MASK_PRINT 8
#define CON_MASK_HIDENOTIFY 128

typedef struct con_lineinfo_s
{
	char *pstart;			// Baker: This is a pointer in con->text, pstart is not allocated
	size_t line_strlen;		// Baker: Is this strlen or what exactly is this.
	int mask;				// CON_MASK_DEVELOPER, CON_MASK_CHAT, CON_MASK_HIDENOTIFY and such

	/// used only by console.c
	double addtime;
	int line_num_rows_height; ///< recalculated line height when needed (-1 to unset) // Baker: This is NOT pixels
}
con_lineinfo_t;

typedef struct conbuffer_s
{
	qbool active;
	int textsize;
	char *textcon;
	int maxlines; // Baker: 4096
	con_lineinfo_t *clines;
	int lines_first;
	int lines_count; ///< cyclic buffer
}
conbuffer_t;

#define CON_TRUEIDX(x)		(( con.lines_first + (x) ) % con.maxlines)
#define THRESHOLD_5 5

typedef struct {
// Every frame that the console is drawn.
// If console was not drawn, please set this to 0
	//int		was_consoledrawn;		// 
	// Baker: If drag distance > tolerance (5 pixels?)
	// Then start selection


//	int				mouse_is_down;
//	int				have_select;

//	unsigned int	mousedown_frame; // host.superframecount .. 
	struct {
		double		mousedown_time;
		float		mousedown_fx;
		float		mousedown_fy;
		int			mousedown_row_index;
		int			mousedown_col_index;
		int			mousedown_end_row_index;
		int			mousedown_end_col_index;
		int			drag_state;
		qbool		prior_was_selection;
	} a;

	struct {
	  unsigned int	console_draw_frame; // host.superframecount .. 

		int			draw_row_last_index;	// con line index
		int			draw_row_top_pixel_y;	// This y pixel coord of TOP of last index.

		float		conscalewidth;		// So we don't have to guess
		float		conscaleheight;
	} draww;
} consel_t;

typedef enum { // SlideState_e
    drag_state_none_0 				= 0,
    drag_state_awaiting_threshold_1 = 1,
    drag_state_dragging_2 			= 2,
	drag_state_drag_completed_3 	= 3,
} drag_state_e;

#define TREAT_IGNORE_0					0
#define TREAT_CONSUMED_MOUSE_ACTION_1	1
#define TREAT_MOUSEUP_2					2


#define IS_DOWN_MOUSEMOVE_2	2

extern consel_t g_consel; // Baker: SELECTEON


WARP_X_ (con, history)  // Baker: Both of these are conbuffer_t

#define CONBUFFER_LINES(buf, i) (buf)->clines[((buf)->lines_first + (i)) % (buf)->maxlines]
#define CONBUFFER_LINES_COUNT(buf) ((buf)->lines_count)
#define CONBUFFER_LINES_LAST(buf) CONBUFFER_LINES(buf, CONBUFFER_LINES_COUNT(buf) - 1)

void ConBuffer_Init(conbuffer_t *buf, int textsize, int maxlines, mempool_t *mempool);
void ConBuffer_Clear (conbuffer_t *buf);
void ConBuffer_Shutdown(conbuffer_t *buf);

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

// Baker r0004: Ctrl + up/down size console like JoeQuake
extern float console_user_pct;
void Con_AdjustConsoleHeight(const float delta);

typedef struct {
	unsigned char *s_name_after_maps_folder_a;
	unsigned char *s_name_trunc_16_a;
	unsigned char *s_map_title_trunc_28_a;
	unsigned char *s_bsp_code;
} maplist_s;

#define MAXMAPLIST_4096 4096
extern maplist_s m_maplist[MAXMAPLIST_4096];
extern int m_maplist_count;

// Return value is true if it found any?
qbool GetMapList (const char *s, char *completedname, int completednamebufferlength, 
	int is_menu_fill, int is_autocomplete, int is_suppress_print);

typedef struct autocomplete_s {
	// Baker: names with trailing _a means allocated.
	// These are freed / set with malloc/free

	//                                          e2 is being autocompleted with cursor in middle of line
	//                                          e2MX is current autocomplete
										// ]map e2MX ; deathmatch 1
										//        __   < -- MX is autocompleted from e2
	char	*p_text_partial_start;		// At e2 
	char	*p_text_completion_start;	//        _    start pos of the autocomplete @ the M
	char	*p_text_beyond_autocomplete;//          _  on the character after the MX 
	int		is_at_first_arg;
	int		is_from_nothing;			// Why do we care?
	int		searchtype;
	int		search_partial_offset;
	char	*s_command0_a;				// Previous autocomplete command line "map"
	char	*s_completion_a;			// If in an autocomplete like above, this would be "e2MX"
	char	*text_after_autocomplete_a;
	char	*s_search_partial_a;		// If in an autocomplete like above, this would be "e2" otherwise NULL

// Frame
	char	*s_match_before_a;			// Alphabetical previous (or NULL if top)
	char	*s_match_after_a;			// Alpbabetical next (OR NULL if last)
	char	*s_match_alphatop_a;		// First entry, for wraparound
	char	*s_match_alphalast_a;		// Last entry, for wraparound
} autocomplete_t;

extern autocomplete_t _g_autocomplete;

typedef enum {
	cursor_reset_0 = 0, 
	cursor_reset_abs, 
	selection_clear, 
	cursor_select, 
	cursor_select_all
} cursor_e;

void Partial_Reset (void); // Autocomplete
void Partial_Reset_Undo_Normal_Selection_Reset (void);
void Partial_Reset_Undo_Navis_Selection_Reset (void);
void Con_Undo_Point (int action, int was_space);
void Key_Console_Cursor_Move(int netchange, cursor_e action);

WARP_X_ (consel_t)
void Consel_MouseReset (const char *reason);
void Consel_MouseMove_Check (void);
void Consel_Copy (void);

#endif // ! CONSOLE_H

