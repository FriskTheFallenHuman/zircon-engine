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

#include "darkplaces.h"
#include "cl_video.h"
#include "utf8lib.h"
#include "csprogs.h"


cvar_t con_closeontoggleconsole = {CVAR_SAVE, "con_closeontoggleconsole","1", "allows toggleconsole binds to close the console as well; when set to 2, this even works when not at the start of the line in console input; when set to 3, this works even if the toggleconsole key is the color tag"};

/*
key up events are sent even if in console mode
*/

char		key_line[MAX_INPUTLINE];
int			key_linepos;
int			key_sellength;
qbool	key_insert = true;	// insert key toggle (for editing)
keydest_t	key_dest;
int			key_consoleactive;
char		*keybindings[MAX_BINDMAPS][MAX_KEYS];

int			history_line;
char		history_savedline[MAX_INPUTLINE];
char		history_searchstring[MAX_INPUTLINE];
qbool	history_matchfound = false;
conbuffer_t history;

extern cvar_t	con_textsize;

#include "keys_undo.c.h"

#include "keys_history.c.h"
WARP_X_ (Con_CompleteCommandLine)
void Partial_Reset (void)
{
	freenull3_ (spartial_a)
	//Sys_PrintToTerminal2 ("Partial Reset\n");//va3("First : %s\n", spartial_first_temp_a));
	//Con_PrintLinef ("Partial Reset");//va3("First : %s\n", spartial_first_temp_a));
}

typedef enum { cursor_reset, cursor_reset_abs, select_clear, cursor_select, cursor_select_all} cursor_e;
void Key_Console_Cursor_Move(int netchange, cursor_e action)
{
	switch (action)
	{
	case cursor_reset:		key_linepos = 1, key_sellength = 0; break;
	case cursor_reset_abs:	key_linepos = netchange, key_sellength = 0; break;
	case select_clear:		key_linepos += netchange, key_sellength = 0; break;
	case cursor_select:		key_linepos += netchange, key_sellength += netchange; break;
	case cursor_select_all:	key_linepos = netchange + 1, key_sellength = netchange; break;
	}
}



int keyposstart ()
{
	int netstart = key_sellength > 0 ? key_linepos - key_sellength : key_linepos;
	return netstart;
}


int keyposlength()
{
	int netstart = key_sellength > 0 ? key_sellength :  - key_sellength;
	return netstart;
}

int keyposbeyond ()
{
	return keyposstart() + keyposlength();
}

// folder
void Key_Console_Delete_Selection_Move_Cursor_ClearSel ()
{
	// PIX cursor at 2, sellength -2.  cursor at 4, sellength 2.  2 and 3
	//int len = strlen(workline);
	int posstart		= keyposstart();
	int sz0				= keyposlength();
	int posbeyond		= posstart + sz0;
	
	int keylinelen		= strlen(key_line);
	int cursormovement	= key_sellength > 0 ? -key_sellength : 0;
	int netmovelen		= keylinelen - posstart + 1; // +1 to move the null term too

	memmove (&key_line[posstart], &key_line[posbeyond], netmovelen);

	key_linepos += cursormovement;
	key_sellength = 0; // select_clear
	
}

void Selection_Line_Reset_Clear (void)
{
	key_linepos = 1;
	key_sellength = 0; // select_clear
}

/*
====================
Interactive line editing and console scrollback
====================
*/



static void
Key_Console (int key, int unicode)
{
#if 1
	if (key == K_UPARROW && keydown[K_CTRL]) {
		Con_AdjustConsoleHeight (-0.05); // Baker: Decrease by 5%
		return;
	}

	if (key == K_DOWNARROW && keydown[K_CTRL]) {
		Con_AdjustConsoleHeight (+0.05); // Baker: Increase by 5%
		return;
	}
#endif

	// LadyHavoc: copied most of this from Q2 to improve keyboard handling
	switch (key) {
	case K_KP_SLASH:		key = '/';	break;
	case K_KP_MINUS:		key = '-';	break;
	case K_KP_PLUS:			key = '+';	break;
	case K_KP_HOME:			key = '7';	break;
	case K_KP_UPARROW:		key = '8';	break;
	case K_KP_PGUP:			key = '9';	break;
	case K_KP_LEFTARROW:	key = '4';	break;
	case K_KP_5:			key = '5';	break;
	case K_KP_RIGHTARROW:	key = '6';	break;
	case K_KP_END:			key = '1';	break;
	case K_KP_DOWNARROW:	key = '2';	break;
	case K_KP_PGDN:			key = '3';	break;
	case K_KP_INSERT:		key = '0';	break;
	case K_KP_DELETE:			key = '.';	break;
	} // sw

	//if (key != K_TAB && key != K_SHIFT) {
	//	// What doesn't reset a partial?
	//	Partial_Reset ();
	//}

	// Any non-shift action should clear the selection?
	// Remember the typing of a normal key needs to stomp the selection
	if (key_sellength) {
		// clipboard setters: copy is CTRL+C or CTRL-X (cut, which copies) or SHIFT+DEL (cut, which copies)
		int iscopy = (keydown[K_CTRL] && (key == 'X' || key == 'x' || key == 'C' || key == 'c')) ||
						(keydown[K_SHIFT] && key == K_DELETE);

		// clipboard retrievers: CTRL+V and SHIFT+INSERT
		int ispaste =  (keydown[K_CTRL] && (key == 'V' || key == 'v')) ||
						(keydown[K_SHIFT] && key == K_INSERT);

		// Remove the selection, which should be several things really but these won't do their normal thing.
		int isremoveatom = (keydown[K_CTRL] && (key == 'X' || key == 'x')) || key == K_BACKSPACE || key == K_DELETE;

		if (iscopy && key_sellength) {
			char buf[MAX_INPUTLINE];
			//int netstart = key_sellength > 0 ? key_linepos - key_sellength : key_linepos;
			int s0 = keyposstart ();
			int sz = keyposlength ();

			memcpy (buf, &key_line[s0], sz);
			buf[sz]=0;
			Sys_SetClipboardData (buf);
//			Con_PrintLinef ("Clipboard Set " QUOTED_S, buf);
			S_LocalSound ("hknight/hit.wav");
		}

		// If we are pasting, we delete the selection first.  If we are removing, obviously same.
		if (ispaste || isremoveatom) {
			Con_Undo_Point (/*action*/ 0, /*was_space*/ false);
			Key_Console_Delete_Selection_Move_Cursor_ClearSel ();
		}

		if (isremoveatom) {
			// Already did everything we needed to do.  This is delete and backspace.
			Partial_Reset ();
			return;
		}
	}

	// ENTER
	if (isin2 (key, K_ENTER, K_KP_ENTER)) {
		Partial_Reset (); Con_Undo_Clear ();

		Cbuf_AddText (key_line + 1);	// skip the ]
		Cbuf_AddText ("\n");

		Key_History_Push();

		key_line[0] = ']';
		key_line[1] = 0;	// EvilTypeGuy: null terminate
		key_linepos = 1;
		
		Selection_Line_Reset_Clear (); // ENTER  // conexit

		//if (con_zircon_enter_removes_backscroll.value) {
			con_backscroll = 0; history_line = -1; // Au 15
		//}
		// force an update, because the command may take some time
		if (cls.state == ca_disconnected)
			CL_UpdateScreen ();
		return;
	} // ENTER

	// K_TAB
	if (key == K_TAB) {
		// Enhanced command completion
		// by EvilTypeGuy eviltypeguy@qeradiant.com
		// Thanks to Fett, Taniwha
		if (spartial_a)
			Con_Undo_Point (/*action*/ 0, /*was_space*/ false); // We don't want to set an undo point for each tab completion

		Key_Console_Cursor_Move (0, select_clear); // Reset selection
		if (con_zircon_autocomplete.value) {
			Con_CompleteCommandLine_Zircon(keydown[K_SHIFT], /*from air*/ false);
		}
		else {
			Con_CompleteCommandLine();
		}
		return;
	} // K_TAB

	// CTRL + SPACE
	if (keydown[K_CTRL] && key == K_SPACE) {
		if (con_zircon_autocomplete.value) {
			Key_Console_Cursor_Move (0, select_clear); // Reset selection
			Partial_Reset (); Con_Undo_Point (0,0);

			Con_CompleteCommandLine_Zircon(/*shifted*/ false, /*from air*/ true);
			return;
		}
	} // CTRL + SPACE

	// K_BACKSPACE  DPKEY CTRL-H delete char before cursor
	if (key == K_BACKSPACE || (keydown[K_CTRL] && key == 'h')) {
		Partial_Reset ();
		if (key_linepos > 1) {
			Con_Undo_Point (/*delete*/ -1, /*was space?*/ key_line[key_linepos - 1] == ' ');
            // Delete 1 char from end before cursor
            // Cursor is at  key_line[key_linepos]
            char *s_text_at_cursor = &key_line[key_linepos];
            // The current byte offset.
            // The current byte offset from the start is at key_linepos - 1 since that is after the ]
            // This does not explain the +1, the +1 is needed for the ]
            int key_linepos_start_of_char_before_cursor = (int)u8_prevbyte(/*start*/ key_line, key_linepos);
            char *s_before_backspace = &key_line[key_linepos_start_of_char_before_cursor];
            int keyline_newpos = key_linepos - (s_text_at_cursor - s_before_backspace);
            size_t size_of_move = strlen(s_text_at_cursor) + ONE_CHAR_1; // + 1 null term

        
 			memmove (s_before_backspace, s_text_at_cursor, size_of_move);
			key_linepos = keyline_newpos; // Roguez
			//Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		}
		return;
	} // K_BACKSPACE

	// delete char on cursor
	if (isin2 (key, K_DELETE, K_KP_DELETE)) {
		Partial_Reset ();

		if (keydown[K_SHIFT]) // What is shift del?
			return; // Anything we would do was already handled

		size_t linelen;
		linelen = strlen(key_line);
		if (key_linepos >= (int)linelen)
			return; // EOL

		Con_Undo_Point (/*delete*/ -1, /*was space?*/ key_line[key_linepos - 1] == ' ');

		if (key_linepos < (int)linelen)
			memmove(key_line + key_linepos, key_line + key_linepos + u8_bytelen(key_line + key_linepos, 1), linelen - key_linepos);
		key_sellength = 0; // Roguez
		//Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		return;
	}

	if (isin2 (key, K_HOME, K_KP_HOME)) {
		Partial_Reset ();  Con_Undo_Point (0,0);
		if (keydown[K_CTRL])
			con_backscroll = CON_TEXTSIZE;
		else {
			Key_Console_Cursor_Move (1 - key_linepos, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			//int		pos0 = key_linepos;
			//key_linepos = 1;
			//if (keydown[K_SHIFT]) {
			//	int		delta = key_linepos - pos0; // add to
			//	key_sellength += delta;
			//} else {
			//	Selection_Line_Reset_Clear (); // key_sellength = 0;
			//}

		}
		return;
	}

	if (isin2 (key, K_END, K_KP_END)) {
		Partial_Reset ();  Con_Undo_Point (0,0);
		if (keydown[K_CTRL])
			con_backscroll = 0;
		else {
			//int		pos0 = key_linepos;
			int		slen = (int)strlen(key_line);
			Key_Console_Cursor_Move (slen - key_linepos, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			//if (keydown[K_SHIFT]) {
			//	int		delta = key_linepos - pos0; // add to
			//	key_sellength += delta;
			//} else {
			//	key_sellength = 0;
			//}
		}
		return;
	}

	// CTRL + V
	if (   (keydown[K_CTRL] && key == 'v') || (keydown[K_SHIFT] && isin2(key, K_INSERT, K_KP_INSERT) )  ) {
		char *cbd, *p;
		Partial_Reset ();  Con_Undo_Point (0,0);
		if ((cbd = Sys_GetClipboardData()) != 0) {
			int i;
			// Baker 9100: new line, backspace, cr become ";"
#if 1
			p = cbd;
			while (*p) {
				if (*p == '\r' && *(p+1) == '\n') { *p++ = ';'; *p++ = ' '; } 
				else if (*p == '\n' || *p == '\r' || *p == '\b') { *p++ = ';'; }
				p++;
			}
#else
			strtok(cbd, "\n\r\b");
#endif
			i = (int)strlen(cbd);
			if (i + key_linepos >= MAX_INPUTLINE)
				i = MAX_INPUTLINE - key_linepos - 1;
			if (i > 0) {
				cbd[i] = 0;
				memmove(key_line + key_linepos + i, key_line + key_linepos, sizeof(key_line) - key_linepos - i);
				memcpy(key_line + key_linepos, cbd, i);
				key_linepos += i; // Roguez
				//Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			}
			//Z_Free(cbd);
		}
		return;
	} // CTRL + V




	// move cursor to the previous character
	if (isin2 (key, K_LEFTARROW, K_KP_LEFTARROW)) {
		Partial_Reset ();  Con_Undo_Point (0,0);
		if (key_linepos <= 1)
			return;

		int		pos0 = key_linepos;

		if(keydown[K_CTRL]) { // move cursor to the previous word
			//int		pos0 = key_linepos;
			int		pos;
			char	k;
			pos = key_linepos-1;

			if(pos) { // skip all "; ' after the word
				while(--pos) {
					k = key_line[pos];
					if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
						break;
				}
			}
			if(pos) {
				while(--pos) {
					k = key_line[pos];
					if(k == '\"' || k == ';' || k == ' ' || k == '\'')
						break;
				}
			}
			int		newpos = pos + 1; // key_line
			int		delta = newpos - pos0; // add to
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			//if ( 0 /*keydown[K_SHIFT]*/) {
			//	
			//	key_sellength += delta;
			//} /*else {
			//	key_sellength = 0;
			//}*/
		}
		else if(keydown[K_SHIFT]) { // move cursor to the previous character ignoring colors
			//int		pos0 = key_linepos;
			int		pos;
			size_t          inchar = 0;
			pos = (int)u8_prevbyte(key_line+1, key_linepos-1) + 1; // do NOT give the ']' to u8_prevbyte
			while (pos) {
				if(pos-1 > 0 && key_line[pos-1] == STRING_COLOR_TAG && isdigit(key_line[pos])) pos-=2;
				else if(pos-4 > 0 && key_line[pos-4] == STRING_COLOR_TAG && key_line[pos-3] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(key_line[pos-2]) && isxdigit(key_line[pos-1]) && isxdigit(key_line[pos])) pos-=5;
				else {
					if(pos-1 > 0 && key_line[pos-1] == STRING_COLOR_TAG && key_line[pos] == STRING_COLOR_TAG) // consider ^^ as a character
						pos--;
					pos--;
					break;
				}
			} // while
			// we need to move to the beginning of the character when in a wide character:
			u8_charidx(key_line, pos + 1, &inchar);
			int		newpos = (int)(pos + 1 - inchar); // key_line
			int		delta = newpos - pos0; // add to
			//key_linepos = (int)(pos + 1 - inchar);
			//if (1 /*keydown[K_SHIFT]*/) {
			//	int		delta = key_linepos - pos0; // add to
			//	key_sellength += delta;
			//} /*else {
			//	key_sellength = 0;
			//}*/
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		}
		else { // No Shift
			int		newpos = (int)u8_prevbyte(key_line+1, key_linepos-1) + 1; // key_line
			int		delta = newpos - pos0; // add to
			//key_linepos = (int)u8_prevbyte(key_line+1, key_linepos-1) + 1; // do NOT give the ']' to u8_prevbyte
			//key_sellength = 0;
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		}
		return;
	} // LEFT

#if 1
	if (isin2 (key, K_MOUSE1, K_MOUSE2) && (cls.state != ca_connected || (cls.state == ca_connected && cls.signon == SIGNONS))   ) {
		//int q;
		//q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
#ifdef CONFIG_MENU
		//if (!q && down) {
			WARP_X_ (M_ToggleMenu)
			if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_USER_1)) // conexit
				Con_ToggleConsole_f ();
			WARP_X_ (M_ToggleMenu)

			MR_ToggleMenu(1);
			return;
		//}
#endif
	}
#endif
	// move cursor to the next character
	if (isin2 (key, K_RIGHTARROW, K_KP_RIGHTARROW)) {

		Partial_Reset ();  Con_Undo_Point (0,0);

		if (key_linepos >= (int)strlen(key_line))
			return;

		int		pos0 = key_linepos;

		if(keydown[K_CTRL]) { // move cursor to the next word
			int		pos, len;
			int		pos00 = key_linepos;
			char	k;
			len = (int)strlen(key_line);
			pos = key_linepos;

			while (++pos < len) {
				k = key_line[pos];
				if(k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			}
			
			if (pos < len) // skip all "; ' after the word
				while(++pos < len) {
					k = key_line[pos];
					if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
						break;
				}
			//key_linepos = pos;
			int		newpos = pos; //(int)(pos + 1 - inchar); // key_line
			int		delta = newpos - pos00; // add to			
			//if (0 /*keydown[K_SHIFT]*/) {
			//	int		delta = key_linepos - pos00; // add to
			//	key_sellength += delta;
			//} else {
			//	key_sellength = 0;
			//}
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		} else if(keydown[K_SHIFT]) { // move cursor to the next character ignoring colors
			int		pos, len;
			int		pos00 = key_linepos;
			len = (int)strlen(key_line);
			pos = key_linepos;
			
			// go beyond all initial consecutive color tags, if any
			if (pos < len) {
				while (key_line[pos] == STRING_COLOR_TAG) {
					if(isdigit(key_line[pos+1])) pos+=2;
					else if(key_line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(key_line[pos+2]) && isxdigit(key_line[pos+3]) && isxdigit(key_line[pos+4])) pos+=5;
					else break;
				} // while
			}

			// skip the char
			if (key_line[pos] == STRING_COLOR_TAG && key_line[pos+1] == STRING_COLOR_TAG) // consider ^^ as a character
				pos++;
			pos += (int)u8_bytelen(key_line + pos, 1);
			
			// now go beyond all next consecutive color tags, if any
			if(pos < len) {
				while (key_line[pos] == STRING_COLOR_TAG) {
					if(isdigit(key_line[pos+1])) pos+=2;
					else if(key_line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(key_line[pos+2]) && isxdigit(key_line[pos+3]) && isxdigit(key_line[pos+4])) pos+=5;
					else break;
				} // while
			}
			//key_linepos = pos;
			int		newpos = pos;
			int		delta = newpos - pos00;
			//if (1 /*keydown[K_SHIFT]*/) {
			//	int		delta = key_linepos - pos00; // add to
			//	key_sellength += delta;
			//}/* else {
			//	key_sellength = 0;
			//}*/
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		}
		else { // No shift
			//key_linepos += (int)u8_bytelen(key_line + key_linepos, 1);
			int		newpos = key_linepos + (int)u8_bytelen(key_line + key_linepos, 1); //(int)(pos + 1 - inchar); // key_line
			int		delta = newpos - pos0; // add to

			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			
		}
		return;
	} // RIGHT

	if (keydown[K_CTRL] && key == 'z') {
		Partial_Reset ();
		if (keydown[K_CTRL]) {
			if (!Con_Undo_Walk (keydown[K_SHIFT] ? -1 : 1))
				Con_DPrintLinef ("End of undo buffer");
		}

		return;
	}

	#include "keys_darkplaces.h"
	

	// non printable
	if (unicode < 32)
		return;

	Partial_Reset (); Con_Undo_Point (1, key == SPACE_CHAR_32);

	// A unicode or ascii character
	if (key_sellength)
		Key_Console_Delete_Selection_Move_Cursor_ClearSel ();

	if (key_linepos < MAX_INPUTLINE - 1) {
		char buf[16];
		int len;
		int blen;
		blen = u8_fromchar(unicode, buf, sizeof(buf));
		if (!blen)
			return;
		len = (int)strlen(&key_line[key_linepos]);
		// check insert mode, or always insert if at end of line
		if (key_insert || len == 0) {
			if (key_linepos + len + blen >= MAX_INPUTLINE)
				return;
			// can't use strcpy to move string to right
			len++;
			if (key_linepos + blen + len >= MAX_INPUTLINE)
				return;
			memmove(&key_line[key_linepos + blen], &key_line[key_linepos], len);
		}
		else if (key_linepos + len + blen - u8_bytelen(key_line + key_linepos, 1) >= MAX_INPUTLINE)
			return;
		memcpy(key_line + key_linepos, buf, blen);
		if (blen > len)
			key_line[key_linepos + blen] = 0;
		// END OF FIXME
		//key_linepos += blen;
		Key_Console_Cursor_Move (blen, select_clear);
	}

	
} // Key_Console

#include "keys_other.c.h"


