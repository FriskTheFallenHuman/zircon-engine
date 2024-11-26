/*
	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

// keys.c

#include "quakedef.h"
#include "cl_video.h"
#include "utf8lib.h"
#include "csprogs.h"

//#pragma message ("Seek input from non-US keyboard users on tilde always closing console.  Quakespasm does it, so it is ok right?")
cvar_t con_closeontoggleconsole = {CF_CLIENT | CF_ARCHIVE, "con_closeontoggleconsole","4", "allows toggleconsole binds to close the console as well; when set to 2, this even works when not at the start of the line in console input; when set to 3, this works even if the toggleconsole key is the color tag; 4, same as 3 except backquote never emits to console under any cirumstances (Quake compat) [Zircon default]"};  // Baker r0092 option for tilde to never ever emit to console.

/*
key up events are sent even if in console mode
*/

char		key_line[MAX_INPUTLINE_16384];
int			key_linepos;
int			key_sellength;			// Number of characters selected

// Baker: DarkPlaces may reveal the color code
// So the sellength needs to subtract every completed color code
// So we need to subtract any
// ^ followed by a decimal digit 0-9
// However, it will show the color code if the cursor is after a ^ followed by a digit
// So the line[key_linepos] is at a digit
// And the line[key_linepos - 1] is at a ^2
// What if cursor select highlights the 2 but not the ^?  Is that possible?
// That scenario could affect the start as well?  Or the end?
//int			key_sellength_left;	// Number of characters displayed
//int			key_sellength_right;
//int			key_sellength_count;

qbool	key_insert = true;	// insert key toggle (for editing)
keydest_e	key_dest;
int			key_consoleactive;
char		*keybindings[MAX_BINDMAPS_8][MAX_KEYS_44032];

int			history_line;
char		history_savedline[MAX_INPUTLINE_16384];
char		history_searchstring[MAX_INPUTLINE_16384];
qbool	history_matchfound = false;
conbuffer_t history;

extern cvar_t	con_textsize;

#include "keys_undo.c.h"

#include "keys_history.c.h"

//// key modifier states -- Baker: Moved to keys.h
//#define KM_NONE           (!keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
//#define KM_CTRL_SHIFT_ALT ( keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
//#define KM_CTRL_SHIFT     ( keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
//#define KM_CTRL_ALT       ( keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])
//#define KM_SHIFT_ALT      (!keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
//#define KM_CTRL           ( keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
//#define KM_SHIFT          (!keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
//#define KM_ALT            (!keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])

/*
====================
Interactive line editing and console scrollback
====================
*/

int Key_AddChar(int unicode, qbool is_console)
{
	char *line;
	char buf[16];
	int len, blen, linepos;

	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
	}

	if (linepos >= MAX_INPUTLINE_16384-1)
		return linepos;

	blen = u8_fromchar(unicode, buf, sizeof(buf));
	if (!blen)
		return linepos;
	len = (int)strlen(&line[linepos]);
	// check insert mode, or always insert if at end of line
	if (key_insert || len == 0)
	{
		if (linepos + len + blen >= MAX_INPUTLINE_16384)
			return linepos;
		// can't use strcpy to move string to right
		len++;
		if (linepos + blen + len >= MAX_INPUTLINE_16384)
			return linepos;
		memmove(&line[linepos + blen], &line[linepos], len);
	}
	else if (linepos + len + blen - u8_bytelen(line + linepos, 1) >= MAX_INPUTLINE_16384)
		return linepos;
	memcpy(line + linepos, buf, blen);
	if (blen > len)
		line[linepos + blen] = 0;
	linepos += blen;
	return linepos;
}

int keyposstart (void)
{
	int netstart = key_sellength > 0 ? key_linepos - key_sellength : key_linepos;
	return netstart;
}


int keyposlength(void)
{
	int netstart = key_sellength > 0 ? key_sellength :  - key_sellength;
	return netstart;
}

int keyposbeyond (void)
{
	return keyposstart() + keyposlength();
}

void Key_Console_Cursor_Move(int netchange, cursor_e action)
{
	switch (action) {
	case cursor_reset_0:	key_linepos = ONE_CHAR_1, key_sellength = 0; break;
	case cursor_reset_abs:	key_linepos = netchange, key_sellength = 0; break;
	case selection_clear:	key_linepos += netchange, key_sellength = 0; break;
	case cursor_select:		key_linepos += netchange, key_sellength += netchange; break;

	// Baker: cursor_select_all and cursor_reset_0
	// are absolute, not relative
	// Since keyline[0] is "["
	// We need to use 1 instead, so ONE_CHAR_1
	// cursor_select_all, we expect to get the strlen
	// at keyline[1]
	case cursor_select_all:	key_linepos = ONE_CHAR_1 + netchange, SET___ key_sellength = netchange; break;
	}

	// baker_detect_colorcodes or UTF8 (any char > 127) here, if so key_sellength is 0.
	// ^(DECIMAL)
	// ^(X)
	if (key_sellength) {
		int is_pure = true;

		if (utf8_enable.integer) {
			is_pure = false;
		} else {
			char *s = &key_line[1];
			int slen = (int)strlen (s);

			for (int n = 1; n <  slen; n ++) {
				if (s[n] == '^' || s[n] > CHAR_TILDE_126) {
					is_pure = false;
					break;
				} // if
			} // for
		} // if
		if (is_pure == false) {
baker_stupid_evasion:
			key_sellength = 0;
		}
	}
}

//void Key_Sel_Length_Display_Count_Refresh (void)
//{
//	if (!key_sellength) {
//		key_sellength_cursor
//}

void Key_Console_Delete_Selection_Move_Cursor_ClearSel ()
{
	// PIX cursor at 2, sellength -2.  cursor at 4, sellength 2.  2 and 3
	//int len = strlen(workline);
	int posstart		= keyposstart();
	int sz0				= keyposlength();
	int posbeyond		= posstart + sz0;

	int keylinelen		= (int)strlen(key_line);
	int cursormovement	= key_sellength > 0 ? -key_sellength : 0;
	int netmovelen		= keylinelen - posstart + 1; // +1 to move the null term too

	memmove (&key_line[posstart], &key_line[posbeyond], netmovelen);

	key_linepos += cursormovement;
	SET___ key_sellength = 0; // select_clear

}

int Key_Console_Cursor_Move_Simplex(int cursor_now, int is_shifted)
{
	int		newpos	= cursor_now;
	int		oldpos	= key_linepos;
	int		delta	= newpos - oldpos;

	Key_Console_Cursor_Move (delta, is_shifted ? cursor_select : selection_clear); // Reset selection

#if 0 // def _DEBUG
	char vabuf[1024];
	const char *s = va(vabuf, sizeof(vabuf),
		"START %d LENGTH %d DELTA %d SHIFTED? %d" NEWLINE,
		key_linepos, key_sellength, delta, is_shifted);
	DebugPrintLinef (s);
#endif
	return key_linepos; // Changed
}

// returns -1 if no key has been recognized
// returns linepos (>= 0) otherwise
// if is_console is true can modify key_line (doesn't change key_linepos)
int Key_Parse_CommonKeys(cmd_state_t *cmd, qbool is_console, int key, int unicode)
{
	char *line;
	int linepos, linestart;
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

	// Any non-shift action should clear the selection?
	// Remember the typing of a normal key needs to stomp the selection

	while (key_consoleactive && g_consel.a.drag_state == drag_state_drag_completed_3) {
		if (keydown[K_CTRL] && isin1 (key, 'c')) {
			Consel_Copy ();
			Consel_MouseReset ("Copy action");
			return key_linepos;
		}
		break; // while
	} // while

#if 1 // October 18 2024
	if (key == K_CTRL && unicode == 0 && is_console) {
		// Keydown only
		WARP_X_ (SCR_InfoBar_f)
		extern float scr_controldown_time;
		extern cvar_t con_hints;
		if (!scr_controldown_time && con_hints.value > 0)
			scr_controldown_time = host.realtime; // Down start
	}
#endif

	int ispaste =  (keydown[K_CTRL] && isin1 (key, 'v') ) ||
					(keydown[K_SHIFT] && isin2 (key, K_INSERT, K_KP_INSERT) );

	if (is_console && key_sellength) {
		// clipboard setters: copy is CTRL+C or CTRL-X (cut, which copies) or SHIFT+DEL (cut, which copies)
		int iscopy = (keydown[K_CTRL] && isin2 (key, 'x', 'c')) ||
						(keydown[K_SHIFT] && key == K_DELETE);

		// clipboard retrievers: CTRL+V and SHIFT+INSERT

		// Remove the selection, which could be several things really
		// but these won't do their normal thing.
		int isremoveatom = (keydown[K_CTRL] && isin1 (key, 'x'))
						|| key == K_BACKSPACE || key == K_DELETE;

		if (iscopy && key_sellength) {
			char sbuf[MAX_INPUTLINE_16384];

			int s0 = keyposstart ();
			int sz = keyposlength ();

			memcpy (sbuf, &key_line[s0], sz);
			sbuf[sz] = NULL_CHAR_0;

			Sys_SetClipboardData (sbuf);
			// Con_PrintLinef ("Clipboard Set " QUOTED_S, buf);
#if 1
			//Cbuf_AddTextLinef (cmd, "play hknight/hit.wav");

#else // Baker: This caused screen flicker
			S_LocalSound ("hknight/hit.wav");
#endif
			if (!isremoveatom) {
				return key_linepos;
			}
		}

		// If we are pasting, we delete the selection first.  If we are removing, obviously same.
		if (ispaste || isremoveatom) {
			Partial_Reset ();
			Key_Console_Delete_Selection_Move_Cursor_ClearSel ();
			//int j = 5;
		}

		linepos = is_console ? key_linepos : chat_bufferpos; // Update this variable

		if (isremoveatom) {
			// Already did everything we needed to do.  This is delete and backspace.
			return key_linepos;
		}
	}

	if (is_console && key == 'a' && KM_CTRL) { // CTRL-A
		// (X) kx: CTRL-A
		Partial_Reset_Undo_Normal_Selection_Reset ();
		//Partial_Reset ();  Con_Undo_Point (q_undo_action_none_0, q_was_a_space_false);
		Key_Console_Cursor_Move ((int)strlen(&line[1]), cursor_select_all); // Reset selection
		return key_linepos;
	}

	if (ispaste) {
		// THIS CAN STILL BE CONSOLE IF NO SELECTION
		// kx: CTRL-V - paste
		char *cbd, *p;

		if (is_console) {
			Partial_Reset_Undo_Normal_Selection_Reset (); // SEL/UNDO CTRL-V paste, reset partial
		}

		if ((cbd = Sys_GetClipboardData_Alloc()) != 0) {
			int slen = strlen(cbd);
#if 1
			p = cbd;
			while (*p) {
				// Baker: Turn CRLF into "; "
				if (*p == '\r' && *(p+1) == '\n') {
					*p++ = ';';
					*p++ = ' ';
				}
				// Baker: Turn backspace, newline, carriage return to ;
				else if (*p == '\n' || *p == '\r' || *p == '\b')
					*p++ = ';';
				else
					p++;
			}
			int slen2 = strlen(cbd);
#else
			strtok(cbd, "\n\r\b");
#endif
			int i = (int)strlen(cbd);
			if (i + linepos >= MAX_INPUTLINE_16384)
				i= MAX_INPUTLINE_16384 - linepos - 1;
			if (i > 0) {
				cbd[i] = 0;
				int cursor0 = linepos + i;
				// Baker: Copy ]frogs
				//             ]GRAPESfrogs
				char	*dest = &line[linepos + i];
				char	*src  = &line[linepos];
				int oldlinesize = linesize; // sizeof(key_line)
				size_t  count = linesize  - linepos - i;
				memmove(line + linepos + i, line + linepos, linesize - linepos - i);
				memcpy(line + linepos, cbd, i);
				linepos += i;
				//int j = 5;
			}

			Z_Free(cbd); // Baker: Sys_GetClipboardData_Alloc zallocs!
		} // CLIPBOARD DATA
		return linepos;
	} // CTRL-V PASTE


#if !defined(QUALKER) && !defined(OBJECTN) // CTRL-P Pause Freeze
	if (key == 'p' && KM_CTRL && is_console) { // Baker: CTRL-P Pause Freeze
		// (XU) kx: CTRL-P - pause freeze
		extern cvar_t sv_cheats;
		if (gamemode == GAME_NORMAL) {
			if (cl.islocalgame && sv_cheats.integer && r_refdef.scene.worldmodel) {
				Cvar_SetValueQuick (&sv_freezenonclients, !sv_freezenonclients.integer);
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-P: Pause freeze %s (sv_freezenonclients %d)\" silent", sv_freezenonclients.integer ? "ON" : "OFF", sv_freezenonclients.integer);
			} else if (!sv_cheats.integer) {
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-P: Pause freeze CANNOT cheats disabled\" silent");
			} else {
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-P: Pause freeze CANNOT - no map\" silent");
			}
		}
		return linepos;
	}
#endif

#if !defined(QUALKER) && !defined(OBJECTN) // tool_inspector
	if (key == 'i' && KM_CTRL && is_console) { // Baker: CTRL-I toggle tool_inspector
		// (XU) kx: CTRL-I - toggle inspector
		extern cvar_t showtex;
		Cvar_SetValueQuick (&tool_inspector, !tool_inspector.integer);
		Cvar_SetValueQuick (&showtex, tool_inspector.integer);
		Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-I: toolinspector %d with showtex %d\" silent", tool_inspector.integer, showtex.integer);
		return linepos;
	}
#endif

#if !defined(QUALKER) && !defined(OBJECTN) // CTRL-SHIFT-P showpos
	if (key == 'p' && KM_CTRL_SHIFT && is_console) { // Baker: CTRL-SHIFT-P showpos
		extern cvar_t sv_cheats;
		extern cvar_t showpos;
		extern cvar_t showangles;
		if (gamemode == GAME_NORMAL) {
			if (1) {
				Cvar_SetValueQuick (&showangles, !showpos.integer);
				Cvar_SetValueQuick (&showpos, !showpos.integer);
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-SHIFT-P: showpos/showangles %d\" silent", showpos.integer);
			}
		}
		return linepos;
	}
#endif

#if !defined(QUALKER) && !defined(OBJECTN) // TRACELINE
	if (key == 't' && KM_CTRL && is_console) { // Baker: CTRL_T traceline
		extern cvar_t sv_cheats;
		if (gamemode == GAME_NORMAL) {
			if (!cl.islocalgame || !r_refdef.scene.worldmodel) {
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-T: traceline can't -- no server map\" silent");
			} if (!sv_cheats.integer) {
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-T: traceline can't -- sv_cheats is 0\" silent");
			} else if (tool_marker.integer) {
				Cvar_SetValueQuick (&tool_marker, 0);
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-T: traceline cleared \" silent");
			} else {
				Cbuf_AddTextLinef (cmd, "%s", "traceline");
				Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-T: traceline marked\" silent");
			}
		}
		return linepos;
	}

#endif

	if (key == 's' && KM_CTRL_SHIFT && is_console) { // Baker: CTRL-I toggle tool_inspector
		// (XU) kx: CTRL-I - toggle inspector
		//Cvar_SetValueQuick (&tool_inspector, !tool_inspector.integer);
		int is_on = false;
		// Baker Oct 5 2024 does not work? Why?:
		cvar_t *did_find = Cvar_FindVar (&cvars_all, "_saved_showfps", ALL_FLAGS_ANTIZERO);
		if (did_find == NULL) {
			// Exists
			//Cbuf_AddTextLinef (cmd, "echo start");
			//Cbuf_AddTextLinef (cmd, "echo start2");
			Cbuf_AddTextLinef (cmd, "set _saved_viewsize $viewsize; viewsize 120");
			Cbuf_AddTextLinef (cmd, "set _saved_crosshair $crosshair; crosshair 0");
			Cbuf_AddTextLinef (cmd, "set _saved_showfps $showfps; showfps 0");
			Cbuf_AddTextLinef (cmd, "set _saved_r_drawviewmodel $r_drawviewmodel; r_drawviewmodel 0");

			Cbuf_AddTextLinef (cmd, "viewsize 120; showfps 0; r_drawviewmodel 0; crosshair 0");
			//Cbuf_AddTextLinef (cmd, "echo end");
			is_on = true;
		} else {
			Cbuf_AddTextLinef (cmd, "viewsize $_saved_viewsize; unset _saved_viewsize");
			Cbuf_AddTextLinef (cmd, "crosshair $_saved_crosshair; unset _saved_crosshair");
			Cbuf_AddTextLinef (cmd, "showfps $_saved_showfps; unset _saved_showfps");
			Cbuf_AddTextLinef (cmd, "r_drawviewmodel $_saved_r_drawviewmodel; unset _saved_r_drawviewmodel");
		}

		Cbuf_AddTextLinef (cmd, "infobar 2 \"CTRL-SHIFT-S: screenshot settings = %s\" silent", is_on ? "ON" : "OFF" );
		return linepos;
	}

	if (key == 'u' && KM_CTRL) { // like vi/readline ^u: delete currently edited line
		// (XU) kx: CTRL-U - clear line
		if (is_console)
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-U: cleared edit line\" silent");
		return Key_ClearEditLine(is_console); // SEL/UNDO CTRL-U covered since it calls Key_ClearEditLine
	}

	if (key == K_SPACE && KM_CTRL && is_console) {
		// (X) kx: CTRL-SPACE
		if (is_console) {
			Partial_Reset_Undo_Normal_Selection_Reset ();
		}

		// Note: it is impossible for us to be shifted, KM_CTRL is exclusively CTRL held
		return Con_CompleteCommandLine_Zircon (cmd, is_console,
			keydown[K_SHIFT], q_is_from_nothing_true);
	}

	if (key == K_TAB) {
		if (is_console && KM_CTRL) { // append the cvar value to the cvar name
			// kx: CTRL-TAB - insert cvar value ???
			int		cvar_len, cvar_str_len, chars_to_move;
			char	k;
			char	cvar[MAX_INPUTLINE_16384];
			const char *cvar_str;

			if (is_console) {
				// (U) kx: CTRL-TAB
				Partial_Reset_Undo_Normal_Selection_Reset (); // SEL/UNDO CTRL-TAB (cvar paste), reset partial
			}

			// go to the start of the variable
			while(--linepos)
			{
				k = line[linepos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			}
			linepos++;

			// save the variable name in cvar
			for(cvar_len=0; (k = line[linepos + cvar_len]) != 0; cvar_len++)
			{
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
				cvar[cvar_len] = k;
			}
			if (cvar_len==0)
				return linepos;
			cvar[cvar_len] = 0;

			// go to the end of the cvar
			linepos += cvar_len;

			// save the content of the variable in cvar_str
			cvar_str = Cvar_VariableString(&cvars_all, cvar, CF_CLIENT | CF_SERVER);
			cvar_str_len = (int)strlen(cvar_str);
			if (cvar_str_len==0)
				return linepos;

			// insert space and cvar_str in line
			chars_to_move = (int)strlen(&line[linepos]);
			if (linepos + 1 + cvar_str_len + chars_to_move < MAX_INPUTLINE_16384) {
				if (chars_to_move)
					memmove(&line[linepos + 1 + cvar_str_len], &line[linepos], chars_to_move);
				line[linepos++] = ' ';
				memcpy(&line[linepos], cvar_str, cvar_str_len);
				linepos += cvar_str_len;
				line[linepos + chars_to_move] = 0;
			}
			else
				Con_PrintLinef ("Couldn't append cvar value, edit line too long.");

			return linepos;
		} // CTRL + in_console .. append cvar value

		if (is_console) {
			// (X) kx: K_TAB - don't reset partial, reset selection.
			if (_g_autocomplete.s_search_partial_a) {
				// We don't want to set an undo point for each tab completion
				Con_Undo_Point (q_undo_action_normal_0, q_was_a_space_false);
			}

			// Reset selection
			Key_Console_Cursor_Move (q_netchange_zero, selection_clear);
			return Con_CompleteCommandLine_Zircon (cmd, is_console, keydown[K_SHIFT], q_is_from_nothing_false);
		}

		if (KM_NONE) {
			return Con_CompleteCommandLine(cmd, is_console);
		}
	} // TAB

	// Advanced Console Editing by Radix radix@planetquake.com
	// Added/Modified by EvilTypeGuy eviltypeguy@qeradiant.com
	// Enhanced by [515]
	// Enhanced by terencehill

	// delete char before cursor
	if ((key == K_BACKSPACE && KM_NONE) || (key == 'h' && KM_CTRL)) {
		// (X) kx: BACKSPACE
		if (is_console) {
			// Baker: Remove atom form addressed above
			// this is only backwards delete
			if (linepos > linestart) {
				int ch_behind = line[linepos - 1];
				Con_Undo_Point (q_undo_action_delete_neg_1, ch_behind == ' ' ? q_was_a_space_true : q_was_a_space_false);
			}
			Partial_Reset_Undo_Navis_Selection_Reset (); // reset partial, reset selection
		}

		if (linepos > linestart) {
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			int newpos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			strlcpy(line + newpos, line + linepos, linesize + 1 - linepos);
			linepos = newpos;
		} // if
		return linepos;
	} // BACKSPACE

	// delete char on cursor
	if ((key == K_DELETE || key == K_KP_DELETE) && KM_NONE) {
		// (XU) kx: DELETE - forward delete
		size_t linelen;
		linelen = strlen(line);

		if (is_console) {
			// Baker: Remove atom form addressed above
			// this is only forward delete
			if (linepos < (int)linelen) {
				int ch_ahead = line[linepos];
				Con_Undo_Point (q_undo_action_delete_neg_1, ch_ahead == ' ' ? q_was_a_space_true : q_was_a_space_false);
			}

			Partial_Reset_Undo_Navis_Selection_Reset ();
		}
		if (linepos < (int)linelen)
			memmove(line + linepos, line + linepos + u8_bytelen(line + linepos, 1), linelen - linepos);
		return linepos;
	} // DELETE

	// move cursor to the previous character
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW) {
		if (is_console) {
			// Baker: This is navigation.  It ends a partial.
			Partial_Reset ();
			Con_Undo_Point (q_undo_action_normal_0, q_was_a_space_false); // kx (1): ANY LEFT
		}
		if (KM_CTRL || KM_CTRL_SHIFT) { // move cursor to the previous word
			int		pos;
			char	k;
			if (linepos <= linestart + 1) {
				if (is_console) {
					// (X) kx: CTRL-LEFT, CTRL-SHIFT-LEFT
					linepos = Key_Console_Cursor_Move_Simplex (1, keydown[K_SHIFT]);
					return linepos;
				}

				return linestart;
			}
			pos = linepos;

			do {
				k = line[--pos];
				if (!(k == '\"' || k == ';' || k == ' ' || k == '\'' || k == '/'))
					break;
			} while(pos > linestart); // skip all "; ' after the word

			if (pos == linestart)
				return linestart;

			do {
				k = line[--pos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\''  || k == '/')
				{
					pos++;
					break;
				}
			} while(pos > linestart);

			linepos = pos;

			if (is_console) { // kx (2): CTRL-SHIFT-LEFT, CTRL-LEFT
				linepos = Key_Console_Cursor_Move_Simplex (linepos, keydown[K_SHIFT]);
			}

			return linepos;
		} // KM_CTRL


		if (KM_NONE || KM_SHIFT) {
			if (linepos <= linestart + 1) {
				if (is_console) {
					linepos = Key_Console_Cursor_Move_Simplex (1, keydown[K_SHIFT]);
					return linepos;
				}
				return linestart;
			}
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			linepos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;

			if (is_console) { // kx (2): SHIFT-LEFT, LEFT
				// (X) kx: LEFT
				linepos = Key_Console_Cursor_Move_Simplex (linepos, keydown[K_SHIFT]);
			}

			return linepos;
		}
	} // LEFT

	// move cursor to the next character
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW) {
		if (is_console) {
			// Baker: This is navigation.  It ends a partial.
			Partial_Reset ();
			Con_Undo_Point (q_undo_action_normal_0, q_was_a_space_false);  // kx (1): ANY RIGHT
		}

		if (KM_CTRL || KM_CTRL_SHIFT) { // move cursor to the next word
			// (X) kx: CTRL-RIGHT
			int		pos, len;
			char	k;
			len = (int)strlen(line);
			if (linepos >= len) {
				if (is_console) {
					// kx: CTRL-LEFT, CTRL-SHIFT-LEFT
					linepos = Key_Console_Cursor_Move_Simplex (len, keydown[K_SHIFT]);
					return linepos;
				}
				return linepos;
			}
			pos = linepos;

			// Baker: UTF8 cannot collide with ASCII (0-127)
			// So this is fine.
			while (++pos < len) {
				k = line[pos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'' || k == '/')
					break;
			} // while

			if (pos < len) { // skip all "; ' after the word
				while(++pos < len) {
					k = line[pos];
					if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
						break;
				} // while
			} // if
			linepos = pos;

			if (is_console) {  // kx (2): CTRL-RIGHT / CTRL-SHIFT-RIGHT
				linepos = Key_Console_Cursor_Move_Simplex (linepos, keydown[K_SHIFT]);
			}
			return linepos;
		} // CTRL-RIGHT

		if (KM_NONE || KM_SHIFT) {
			// (X) kx: RIGHT
			if (linepos >= (int)strlen(line)) {
				if (is_console) {
					linepos = Key_Console_Cursor_Move_Simplex (linepos, keydown[K_SHIFT]);
					return linepos;
				}
				return linepos;
			}

			linepos += (int)u8_bytelen(line + linepos, 1);

			if (is_console) { // kx (2): RIGHT / SHIFT-RIGHT
				linepos = Key_Console_Cursor_Move_Simplex (linepos, keydown[K_SHIFT]);
			}
			return linepos;
		} // RIGHTARROW
	} // any RIGHTARROW

	// Baker r0003: Thin cursor / no text overwrite mode

	//if ((key == K_INSERT || key == K_KP_INSERT) && KM_NONE) // toggle insert mode
	//{
	//	key_insert ^= 1;
	//	return linepos;
	//}

	if (key == K_HOME || key == K_KP_HOME) {
		if (is_console && KM_CTRL) {
			// (X) kx: CTRL-HOME
			con_backscroll = CON_TEXTSIZE_1_MB;
			return linepos;
		}

		if (KM_NONE || KM_SHIFT) {
			if (is_console) {
				// (X) kx: HOME - cursor nav
				// Baker: This is navigation.  It ends a partial.
				Partial_Reset ();
				Con_Undo_Point (q_undo_action_normal_0, q_was_a_space_false);  // kx (1): ANY LEFT
				linepos = Key_Console_Cursor_Move_Simplex (1, keydown[K_SHIFT]);
				return linepos;
			}
			return linestart;
		}
	} // HOME

	if (key == K_END || key == K_KP_END) {
		if (is_console && KM_CTRL) {
			// (X) kx: CTRL-END - inert - scroll history view to end
			con_backscroll = 0;
			return linepos;
		}

		if (KM_NONE || KM_SHIFT) {
			if (is_console) {
				// (X) kx: END - cursor nav
				// Baker: This is navigation.  It ends a partial.
				Partial_Reset ();
				Con_Undo_Point (q_undo_action_normal_0, q_was_a_space_false);  // kx (1): ANY LEFT
				linepos = Key_Console_Cursor_Move_Simplex ((int)strlen(line), keydown[K_SHIFT]);
				return linepos;
			}

			return (int)strlen(line);
		}
	} // END

	return -1;
}


static void
Key_Console(cmd_state_t *cmd, int key, int unicode)
{
	int linepos;

#if 1
	// Baker r0004: Ctrl + up/down size console like JoeQuake
	if (key == K_UPARROW && keydown[K_CTRL]) {
		// (X) kx: CTRL-UP - inert
		Con_AdjustConsoleHeight(-0.05); // Baker: Decrease by 5%
		return;
	}

	if (key == K_DOWNARROW && keydown[K_CTRL]) {
		// (X) kx: CTRL-DOWN - inert
		Con_AdjustConsoleHeight(+0.05); // Baker: Increase by 5%
		return;
	}
#endif

	key = Key_Convert_NumPadKey(key);

	// Forbid Ctrl Alt shortcuts since on Windows they are used to type some characters
	// in certain non-English keyboards using the AltGr key (which emulates Ctrl Alt)
	// Reference: "Why Ctrl+Alt shouldn't be used as a shortcut modifier"
	//            https://blogs.msdn.microsoft.com/oldnewthing/20040329-00/?p=40003
	if (keydown[K_CTRL] && keydown[K_ALT])
		goto add_char;

	linepos = Key_Parse_CommonKeys(cmd, true, key, unicode);
	if (linepos >= 0)
	{
		key_linepos = linepos;
		return;
	}

	if ((key == K_ENTER || key == K_KP_ENTER) && KM_NONE) {
#if 1
		// Baker: We want to do this for multiplayer only and if there is no demo playback
		if (cl_chatmode.value && cls.state == ca_connected && cls.signon == SIGNONS_4 && !cls.demoplayback) {
			const char *s_check_line = &key_line[1]; // skip the "]"

			// 1. Require a non-local game or having svs.maxclients > 1
			if (cl.islocalgame == false || (sv.active && svs.maxclients > 1)) {
				// 2. Require an unknown command
				if (false == Cmd_Is_Lead_Word_A_Command_Cvar_Alias (cmd, s_check_line)) {
					Cbuf_AddText (cmd, "say ");
					Cbuf_AddTextLine (cmd, s_check_line);
					Key_History_Push();
					key_linepos = Key_ClearEditLine(q_is_console_true); // SEL/UNDO ENTER --> does a cleareditline
					con_backscroll = 0;
					return; // DONE!
				} // 2.
			} // 1.

			// If we are here, we didn't hit.
		}
#endif

		// (X) kx: ENTER
		Cbuf_AddTextLine (cmd, key_line+1);	// skip the ]
		Key_History_Push();
		key_linepos = Key_ClearEditLine(q_is_console_true); // SEL/UNDO ENTER --> does a cleareditline
		// force an update, because the command may take some time
		if (cls.state == ca_disconnected)
			CL_UpdateScreen ();
		con_backscroll = 0;
		return;
	}

	if (key == 'l' /* THIS L like LION, not a ONE */ && KM_CTRL) {
		// (X) kx: CTRL-LION - inert - clear console
		Cbuf_AddTextLine (cmd, "clear");
		return;
	}

	if (key == 'q' && KM_CTRL) { // like zsh ^q: push line to history, don't execute, and clear
		// (X) kx: CTRL-Q - clears line
		// clear line
		Key_History_Push();
		key_linepos = Key_ClearEditLine(true);  // SEL/UNDO CTRL-Q covered since it calls Key_ClearEditLine

		return;
	}

	// End Advanced Console Editing

	if (isin1(key, K_UPARROW) && KM_SHIFT) {
		con_backscroll += 1; // SCROLL 1 line at a time, Con_DrawConsole performs bounds check
		return;
	}

	if (isin1(key, K_DOWNARROW) && KM_SHIFT) {
		con_backscroll -= 1; // SCROLL 1 line at a time, Con_DrawConsole performs bounds check
		return;
	}

	if ( (isin2(key, K_UPARROW, K_KP_UPARROW) && KM_NONE) || (key == 'p' && KM_CTRL)) {
		// (X) kx: UP - stomps
		// This should cover
		Partial_Reset_Undo_Normal_Selection_Reset (); // SEL/UNDO: Up arrow because changes the line
		Key_History_Up();
		return;
	}

	if (((key == K_DOWNARROW || key == K_KP_DOWNARROW) && KM_NONE) || (key == 'n' && KM_CTRL)) {
		// (X) kx: DOWN - stomps
		// This should cover
		Partial_Reset_Undo_Normal_Selection_Reset (); // SEL/UNDO: Down arrow history because changes the line
		Key_History_Down();
		return;
	}

	if (keydown[K_CTRL]) {
		// prints all the matching commands
		if (key == 'f' && KM_CTRL) {
			// (X) kx: CTRL-F - inert
			// CTRL-F Baker: This does not affect line
			Key_History_Find_All();
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-F: find history command matching\" silent");
			return;
		}
		// Search forwards/backwards, pointing the history's index to the
		// matching command but without fetching it to let one continue the search.
		// To fetch it, it suffices to just press UP or DOWN.
		if (key == 'r' && KM_CTRL_SHIFT) {
			// (X) kx: CTRL-SHIFT-R - inert
			// CTRL-R Baker: This does not affect line
			Key_History_Find_Forwards();
			//if (is_console)
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-SHIFT-R: find history forward\" silent");
			return;
		}
		if (key == 'r' && KM_CTRL) {
			// (X) kx: CTRL-R - inert
			Key_History_Find_Backwards();
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-R: find history backwards\" silent");
			return;
		}

		// go to the last/first command of the history
		if (key == ',' && KM_CTRL) {
			// (X) kx: CTRL-COMMA - stomps line
			Partial_Reset_Undo_Clear_Selection_Reset (); // SEL/UNDO CTRL-COMMA sets current line
			Key_History_First();
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-COMMA: goto first command in history\" silent");
			return;
		}

		if (key == '.' && KM_CTRL) { // SEL/UNDO CTRL-DOT sets current line
			// (X) kx: CTRL-DOT - stomps line
			Partial_Reset_Undo_Clear_Selection_Reset (); // SEL/UNDO CTRL-COMMA sets current line
			Key_History_Last();
			Cbuf_AddTextLinef (cmd, "infobar 3 \"CTRL-DOT: goto recent command in history\" silent");
			return;
		}
	}

	if (key == K_PGUP || key == K_KP_PGUP) {
		if (KM_CTRL) {
			// (X) kx: CTRL-PGUP - inert
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			// (X) kx: PGUP - inert
			con_backscroll += ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	} // PGUP

	if (key == K_PGDN || key == K_KP_PGDN) {
		if (KM_CTRL) {
			// (X) kx: CTRL-PGDN - inert
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			// (X) kx: PGDN - inert
			con_backscroll -= ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	} // PGDN

	if (key == K_MWHEELUP) {
		if (KM_CTRL) {
			con_backscroll += 1;
			return;
		}

		if (KM_SHIFT) {
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			con_backscroll += 5;
			return;
		}
	} // WHEELUP

	if (key == K_MWHEELDOWN) {
		if (KM_CTRL) {
			con_backscroll -= 1;
			return;
		}

		if (KM_SHIFT) {
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			con_backscroll -= 5;
			return;
		}
	} // WHEELDOWN

	if (keydown[K_CTRL]) {
		// text zoom in
		// Baker: '+' is '=' when unshifted!
		// Baker: TerranceHill at DarkPlaces github says Italic keyboard does have a plus
		// Baker: Added the plus April 15 2024
		if (isin3 (key, /*plus but unshifted*/ '=', '+', K_KP_PLUS) && KM_CTRL) {
			// (X) kx: CTRL-PLUS - inert
			if (con_textsize.integer < 128)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer + 1);
			return;
		}
		// text zoom out
		if ((key == '-' || key == K_KP_MINUS) && KM_CTRL) {
			// (X) kx: CTRL-MINUS - inert
			if (con_textsize.integer > 1)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer - 1);
			return;
		}
		// text zoom reset
		if ((key == '0' || key == K_KP_INSERT) && KM_CTRL) {
			// (X) kx: CTRL-ZERO - inert
			Cvar_SetValueQuick(&con_textsize, atoi(Cvar_VariableDefString(&cvars_all, "con_textsize", CF_CLIENT | CF_SERVER)));
			return;
		}
	}

	if (keydown[K_CTRL] && key == 'z') {
		// (X) kx: CTRL-Z - undo
		Partial_Reset ();
		if (Con_Undo_Walk (keydown[K_SHIFT] ? -1 : 1) == false) {
			Con_DPrintLinef ("End of undo buffer"); // No more undos
		}
		return;
	}

add_char:

	// non printable
	if (unicode < 32)
		return;

	// Baker r0092 "tilde" key never emits exclusively to close console.
	if (unicode == CHAR_BACKQUOTE_96 && con_closeontoggleconsole.integer >= 4) {
		return; // con_closeontoggleconsole 4 is the tilde exclusively is to toggle console and never, ever emits.
	}

	// kx: Unicode emission - kill selection
#if 1 // Baker: should reset autocomplete, almost anything that is not TAB or SHIFT-TAB should
	Partial_Reset ();
#endif

	// We do this before, right?
	Con_Undo_Point (q_undo_action_add_1, unicode == SPACE_CHAR_32);

	// We are in Key_Console
	if (key_sellength) {
		// We are remove atom first!
		Key_Console_Delete_Selection_Move_Cursor_ClearSel ();
		// keep going ...
	}

	key_linepos = Key_AddChar(unicode, q_is_console_true);
}

//============================================================================

void KeyDest_Set (keydest_e newval)
{
	if (key_dest != newval) {
		Consel_MouseMove_Check ();
		key_dest = newval;
	}
}


//============================================================================

#include "keys_other.c.h"
