// keys_other.c.h


//============================================================================

signed char chat_mode; // 0 for say, 1 for say_team, -1 for command
char chat_buffer[MAX_INPUTLINE_16384];
int chat_bufferpos = 0;

static void Key_Message (cmd_state_t *cmd, int key, int ascii)
{
	int linepos;

	key = Key_Convert_NumPadKey(key);

	if (key == K_ENTER || key == K_KP_ENTER || ascii == 10 || ascii == 13)
	{
		if (chat_mode < 0)
			Cmd_ExecuteString (cmd, chat_buffer, src_local, /*lockmutex?*/ true); // not Cbuf_AddText to allow semiclons in args; however, this allows no variables then. Use aliases!
		else
			CL_ForwardToServerf ("%s %s", chat_mode ? "say_team" : "say ", chat_buffer);

		KeyDest_Set (key_game); // key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	if (key == K_ESCAPE) {
		KeyDest_Set (key_game); // key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	linepos = Key_Parse_CommonKeys(cmd, false, key, ascii);
	if (linepos >= 0)
	{
		chat_bufferpos = linepos;
		return;
	}

	// ctrl+key generates an ascii value < 32 and shows a char from the charmap
	if (ascii > 0 && ascii < 32 && utf8_enable.integer)
		ascii = 0xE000 + ascii;

	if (!ascii)
		return;							// non printable

	chat_bufferpos = Key_AddChar(ascii, false);
}

/*
===================
Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int
Key_StringToKeynum (const char *str)
{
	Uchar ch;
	const keyname_t  *kn;

	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn = keynames; kn->name; kn++) {
		if (String_Match_Caseless (str, kn->name))
			return kn->keynum;
	}

	// non-ascii keys are Unicode codepoints, so give the character if it's valid;
	// error message have more than one character, don't allow it
	ch = u8_getnchar(str, &str, 3);
	return (ch == 0 || *str != 0) ? -1 : (int)ch;
}

/*
===================
Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *
Key_KeynumToString (int keynum, char *tinystr, size_t tinystrlength)
{
	const keyname_t  *kn;

	// -1 is an invalid code
	if (keynum < 0)
		return "<KEY NOT FOUND>";

	// search overrides first, because some characters are special
	for (kn = keynames; kn->name; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	// if it is printable, output it as a single character
	if (keynum > 32)
	{
		u8_fromchar(keynum, tinystr, tinystrlength);
		return tinystr;
	}

	// if it is not overridden and not printable, we don't know what to do with it
	return "<UNKNOWN KEYNUM>";
}


qbool
Key_SetBinding (int keynum, int bindmap, const char *binding)
{
	char *newbinding;
	size_t l;

	if (keynum == -1 || keynum >= MAX_KEYS_44032)
		return false;
	if ((bindmap < 0) || (bindmap >= MAX_BINDMAPS_8))
		return false;

// free old bindings
	if (keybindings[bindmap][keynum]) {
		Z_Free (keybindings[bindmap][keynum]);
		keybindings[bindmap][keynum] = NULL;
	}
	if (!binding[0]) // make "" binds be removed --blub
		return true;
// allocate memory for new binding
	l = strlen (binding);
	newbinding = (char *)Z_Malloc (l + 1);
	memcpy (newbinding, binding, l + 1);
	newbinding[l] = 0;
	keybindings[bindmap][keynum] = newbinding;
	return true;
}

void Key_GetBindMap(int *fg, int *bg)
{
	if (fg)
		*fg = key_bmap;
	if (bg)
		*bg = key_bmap2;
}

qbool Key_SetBindMap(int fg, int bg)
{
	if (fg >= MAX_BINDMAPS_8)
		return false;
	if (bg >= MAX_BINDMAPS_8)
		return false;
	if (fg >= 0)
		key_bmap = fg;
	if (bg >= 0)
		key_bmap2 = bg;
	return true;
}

static void
Key_In_Unbind_f(cmd_state_t *cmd)
{
	int         b, m;
	char *errchar = NULL;

	if (Cmd_Argc (cmd) != 3) {
		Con_PrintLinef ("in_unbind <bindmap> <key> : remove commands from a key");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS_8) || (errchar && *errchar)) {
		Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1) {
		Con_PrintLinef (QUOTED_S " isn't a valid key", Cmd_Argv(cmd, 2));
		return;
	}

	if (!Key_SetBinding (b, m, ""))
		Con_PrintLinef ("Key_SetBinding failed for unknown reason");
}

static void
Key_In_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b, m;
	char        line[MAX_INPUTLINE_16384];
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3 && c != 4) {
		Con_PrintLinef ("in_bind <bindmap> <key> [command] : attach a command to a key");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS_8) || (errchar && *errchar)) {
		Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1 || b >= MAX_KEYS_44032) {
		Con_PrintLinef (QUOTED_S " isn't a valid key", Cmd_Argv(cmd, 2));
		return;
	}

	if (c == 3) {
		if (keybindings[m][b])
			Con_PrintLinef (QUOTED_S " = " QUOTED_S, Cmd_Argv(cmd, 2), keybindings[m][b]);
		else
			Con_PrintLinef (QUOTED_S " is not bound", Cmd_Argv(cmd, 2));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 3; i < c; i++) {
		c_strlcat (line, Cmd_Argv(cmd, i) );
		if (i != (c - 1))
			strlcat (line, " ", sizeof (line));
	}

	if (!Key_SetBinding (b, m, line))
		Con_PrintLinef ("Key_SetBinding failed for unknown reason");
}

static void
Key_In_Bindmap_f(cmd_state_t *cmd)
{
	int         m1, m2, c;
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3) {
		Con_PrintLinef ("in_bindmap <bindmap> <fallback>: set current bindmap and fallback");
		return;
	}

	m1 = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m1 < 0) || (m1 >= MAX_BINDMAPS_8) || (errchar && *errchar)) {
		Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 1));
		return;
	}

	m2 = strtol(Cmd_Argv(cmd, 2), &errchar, 0);
	if ((m2 < 0) || (m2 >= MAX_BINDMAPS_8) || (errchar && *errchar)) {
		Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 2));
		return;
	}

	key_bmap = m1;
	key_bmap2 = m2;
}

static void
Key_Unbind_f(cmd_state_t *cmd)
{
	int         b;

	if (Cmd_Argc (cmd) != 2) {
		Con_PrintLinef ("unbind <key> : remove commands from a key");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1) {
		Con_PrintLinef (QUOTED_S " isn't a valid key", Cmd_Argv(cmd, 1));
		return;
	}

	if (!Key_SetBinding (b, 0, ""))
		Con_PrintLinef ("Key_SetBinding failed for unknown reason");
}

static void
Key_Unbindall_f(cmd_state_t *cmd)
{
	int         i, j;

	for (j = 0; j < MAX_BINDMAPS_8; j++)
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
			if (keybindings[j][i])
				Key_SetBinding (i, j, "");
}

static void
Key_PrintBindList(int j)
{
	char bindbuf[MAX_INPUTLINE_16384];
	char tinystr[TINYSTR_LEN_4];
	const char *p;
	int i;

	for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
	{
		p = keybindings[j][i];
		if (p)
		{
			Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false);
			if (j == 0)
				Con_PrintLinef (CON_BRONZE "%s " CON_WHITE "= " QUOTED_S, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
			else
				Con_PrintLinef (CON_BRONZE "bindmap %d: " CON_BRONZE "%s " CON_WHITE "= " QUOTED_S, j, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
		}
	}
}

static void
Key_In_BindList_f(cmd_state_t *cmd)
{
	int m;
	char *errchar = NULL;

	if (Cmd_Argc(cmd) >= 2)
	{
		m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
		if ((m < 0) || (m >= MAX_BINDMAPS_8) || (errchar && *errchar)) {
			Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 1));
			return;
		}
		Key_PrintBindList(m);
	}
	else
	{
		for (m = 0; m < MAX_BINDMAPS_8; m++)
			Key_PrintBindList(m);
	}
}

static void
Key_BindList_f(cmd_state_t *cmd)
{
	Key_PrintBindList(0);
}

static void
Key_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b;
	char        line[MAX_INPUTLINE_16384];

	c = Cmd_Argc (cmd);

	if (c != 2 && c != 3) {
		Con_PrintLinef ("bind <key> [command] : attach a command to a key");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1 || b >= MAX_KEYS_44032) {
		Con_PrintLinef (QUOTED_S " isn't a valid key", Cmd_Argv(cmd, 1));
		return;
	}

	if (c == 2) {
		if (keybindings[0][b])
			Con_PrintLinef (QUOTED_S " = " QUOTED_S, Cmd_Argv(cmd, 1), keybindings[0][b]);
		else
			Con_PrintLinef (QUOTED_S " is not bound", Cmd_Argv(cmd, 1));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 2; i < c; i++) {
		strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
		if (i != (c - 1))
			strlcat (line, " ", sizeof (line));
	}

	if (!Key_SetBinding (b, 0, line))
		Con_PrintLinef ("Key_SetBinding failed for unknown reason");
}

/*
============
Writes lines containing "bind key value"
============
*/
void
Key_WriteBindings (qfile_t *f)
{
	int         i, j;
	char bindbuf[MAX_INPUTLINE_16384];
	char tinystr[TINYSTR_LEN_4];
	const char *p;

	// Override default binds
	FS_Printf(f, "unbindall" NEWLINE);

	for (j = 0; j < MAX_BINDMAPS_8; j++)
	{
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
		{
			p = keybindings[j][i];
			if (p)
			{
				Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false); // don't need to escape $ because cvars are not expanded inside bind
				if (j == 0)
					FS_Printf (f, "bind %s " QUOTED_S NEWLINE, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
				else
					FS_Printf (f, "in_bind %d %s " QUOTED_S NEWLINE, j, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
			}
		}
	}
}


const char *Key_GetBind (int key, int bindmap)
{
	const char *bind;
	if (key < 0 || key >= MAX_KEYS_44032)
		return NULL;
	if (bindmap >= MAX_BINDMAPS_8)
		return NULL;
	if (bindmap >= 0)
	{
		bind = keybindings[bindmap][key];
	}
	else
	{
		bind = keybindings[key_bmap][key];
		if (!bind)
			bind = keybindings[key_bmap2][key];
	}
	return bind;
}

void Key_FindKeysForCommand (const char *command, int *keys, int numkeys, int bindmap)
{
	int		count;
	int		j;
	const char	*b;

	for (j = 0;j < numkeys;j++)
		keys[j] = -1;

	if (bindmap >= MAX_BINDMAPS_8)
		return;

	count = 0;

	for (j = 0; j < MAX_KEYS_44032; ++j)
	{
		b = Key_GetBind(j, bindmap);
		if (!b)
			continue;
		if (String_Match (b, command) )
		{
			keys[count++] = j;
			if (count == numkeys)
				break;
		}
	}
}

void VID_Alt_Enter_f (void) // Baker: ALT-ENTER
{
	if (cls.state == ca_disconnected)
		goto ok_to_restart;

	// Baker: If there is no map loaded, never block ALT-ENTER.
	if (!cl.worldmodel) {
		goto ok_to_restart;
	}

	// Baker: This is to prevent crash that can occur during map load if ALT-ENTER is hit early in map load.
	if (developer.value) {
		if (!scr_loading && cls.signon == SIGNONS_4 && cls.world_frames && cls.world_start_realtime) {
			// Baker: Not working so far ...
			double dirtytime = host.realtime;// Sys_DirtyTime ();
			double delta_time = dirtytime - cls.world_start_realtime;
			if (delta_time > 1 /*seconds ALT-ENTER*/) {
				goto ok_to_restart;
			}
	
			Con_PrintLinef ("ALT-ENTER: Please wait %1.1f seconds ...", 1.0 - delta_time);
	
			return; // Not ok
		}
	
		Con_DPrintLinef ("Ignored ALT-ENTER because loading state or not fully connected");
	
		return; // Not ok
	}

ok_to_restart:

#ifdef __ANDROID__
	Con_PrintLinef ("vid_restart not supported for this build");

	return;
#endif // __ANDROID__

	if (vid.fullscreen) {
		// Full-screen to window
		Cvar_SetValueQuick (&vid_width, vid_window_width.integer);
		Cvar_SetValueQuick (&vid_height, vid_window_height.integer);
		Cvar_SetValueQuick (&vid_fullscreen, 0);
	} else {
		Cvar_SetValueQuick (&vid_width, vid_fullscreen_width.integer);
		Cvar_SetValueQuick (&vid_height, vid_fullscreen_height.integer);
		Cvar_SetValueQuick (&vid_fullscreen, 1);
	}
	Cbuf_AddTextLine (cmd_local, "vid_restart");
}

/*
===================
Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
static char tbl_keyascii[MAX_KEYS_44032];
static keydest_e tbl_keydest[MAX_KEYS_44032];

typedef struct eventqueueitem_s
{
	int key;
	int ascii;
	qbool down;
}
eventqueueitem_t;
static int events_blocked = 0;
static eventqueueitem_t eventqueue[32];
unsigned eventqueue_idx = 0;

static void Key_EventQueue_Add(int key, int ascii, qbool down)
{
	if (eventqueue_idx < sizeof(eventqueue) / sizeof(*eventqueue))
	{
		eventqueue[eventqueue_idx].key = key;
		eventqueue[eventqueue_idx].ascii = ascii;
		eventqueue[eventqueue_idx].down = down;
		++eventqueue_idx;
	}
}

void Key_EventQueue_Block(void)
{
	// block key events until call to Unblock
	events_blocked = true;
}

void Key_EventQueue_Unblock(void)
{
	// unblocks key events again
	unsigned i;
	events_blocked = false;
	for(i = 0; i < eventqueue_idx; ++i)
		Key_Event(eventqueue[i].key, eventqueue[i].ascii, eventqueue[i].down);
	eventqueue_idx = 0;
}


#include "keys_other_select.c.h"

// Baker r0001 - ALT-ENTER support

qbool ignore_enter_up = false;

WARP_X_ (Sys_SendKeyEvents)
void Key_Event (int key, int ascii, qbool down)
{
	cmd_state_t *cmd = cmd_local;
	const char *bind;
	qbool q;
	keydest_e keydest = key_dest;
	char vabuf[1024];

	if (key < 0 || key >= MAX_KEYS_44032)
		return;

	if (developer_keycode.integer)
		Con_PrintLinef ("key %d ascii %d", key, ascii);

	if (events_blocked) {
		Key_EventQueue_Add(key, ascii, down);
		return;
	}

// Baker r0001 - ALT-ENTER support
#if 1 // ALT-ENTER
	if (key == K_ENTER) {
		if (keydown[K_ALT] && down) { // Baker 2000
				VID_Alt_Enter_f();
			ignore_enter_up = true;
			return; // Didn't happen!
		}
		else if (ignore_enter_up && !down)
		{
			ignore_enter_up = false;
			if (!down) return; // Didn't happen!
		}
	}
#endif

	// get key binding
	bind = keybindings[key_bmap][key];
	if (!bind)
		bind = keybindings[key_bmap2][key];

	if (developer_insane.integer)
		Con_DPrintLinef ("Key_Event(%d, '%c', %s) keydown %d bind " QUOTED_S, key, ascii ? ascii : '?', down ? "down" : "up", keydown[key], bind ? bind : "");

	if (key_consoleactive)
		keydest = key_console;

	if (down)
	{
		// increment key repeat count each time a down is received so that things
		// which want to ignore key repeat can ignore it
		keydown[key] = min(keydown[key] + 1, 2);
		if (keydown[key] == 1) {
			tbl_keyascii[key] = ascii;
			tbl_keydest[key] = keydest;
		} else {
			ascii = tbl_keyascii[key];
			keydest = tbl_keydest[key];
		}
	}
	else
	{
		// clear repeat count now that the key is released
		keydown[key] = 0;
		keydest = tbl_keydest[key];
		ascii = tbl_keyascii[key];
	}

	if (keydest == key_void)
		return;

	// key_consoleactive is a flag not a key_dest because the console is a
	// high priority overlay ontop of the normal screen (designed as a safety
	// feature so that developers and users can rescue themselves from a bad
	// situation).
	//
	// this also means that toggling the console on/off does not lose the old
	// key_dest state

	// specially handle escape (togglemenu) and shift-escape (toggleconsole)
	// engine bindings, these are not handled as normal binds so that the user
	// can recover from a completely empty bindmap
	if (isin2 (key, 96, 178) && ascii == K_SUPERSCRIPT_2) {
		// Con_PrintLinef ("SUPER: key %d ascii %d", key, ascii);
		Con_ToggleConsole (); // Baker: SHIFT-ESC toggle of console.
		tbl_keydest[key] = key_void; // esc release should go nowhere (especially not to key_menu or key_game)
		return;
	}

	if (key == K_ESCAPE) {
		// ignore key repeats on escape
		if (keydown[key] > 1)
			return;

		// escape does these things:
		// key_consoleactive - close console
		// key_message - abort messagemode
		// key_menu - go to parent menu (or key_game)
		// key_game - open menu

		// in all modes shift-escape toggles console
		if (keydown[K_SHIFT])
		{
			if (down)
			{
				Con_ToggleConsole (); // Baker: SHIFT-ESC toggle of console.
				tbl_keydest[key] = key_void; // esc release should go nowhere (especially not to key_menu or key_game)
			}
			return;
		}

		extern int cl_videoplaying;
		//#pragma message ("When we get cin_open working .. make sure this is ok")
		if (cl_videoplaying) { // K_ESCAPE .. Baker: We want it to stop the video
			CL_VideoStop (REASON_USER_ESCAPE_2);
			return; // And do nothing else, including do not toggle menu
		}

		// Baker: This is K_ESCAPE ONLY!
		switch (keydest) {
			case key_console:
				while (down) {
					if (g_consel.a.drag_state > drag_state_none_0 /*K_ESCAPE*/) {
						Consel_MouseReset ("ESC while in console with selection");
						break; // break the while
					}
					if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4)) {
						Flag_Remove_From (key_consoleactive, KEY_CONSOLEACTIVE_USER_1);
#ifdef CONFIG_MENU
						WARP_X_ (M_ToggleMenu)
						Consel_MouseReset ("Forced MR_ToggleMenu Pressed key while in console"); MR_ToggleMenu(1); // conexit .. K_ESCAPE in console comes here
#endif
					}
					else {
						// K_ESC can come here if in console
						Con_ToggleConsole (); // Baker: We are in the console, a key is pressed and the console is not forced (we are connected)
											  //  So the user has the console open and now we close it.
					}
					break; // break while 1
				} // while 
				break;

			case key_message:
				if (down)
					Key_Message (cmd, key, ascii); // that'll close the message input
				break;

			case key_menu:
			case key_menu_grabbed:
#ifdef CONFIG_MENU
				MR_KeyEvent (key, ascii, down);
#endif
				break;

			case key_game:
				// csqc has priority over toggle menu if it wants to (e.g. handling escape for UI stuff in-game.. :sick:)
				q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
#ifdef CONFIG_MENU
				if (!q && down) {
					WARP_X_ (M_ToggleMenu)
					Consel_MouseReset ("User key did it");  MR_ToggleMenu (1); // conexit
				}
#endif
				break;

			default:
				Con_PrintLinef ("Key_Event: Bad key_dest");
		}
		return;
	}

	// send function keydowns to interpreter no matter what mode is (unless the menu has specifically grabbed the keyboard, for rebinding keys)
	// VorteX: Omnicide does bind F* keys
	if (keydest != key_menu_grabbed)
	if (in_range(K_F1, key, K_F12) && gamemode != GAME_BLOODOMNICIDE) {
		if (bind)
		{
			if (keydown[key] == 1 && down)
			{
				// button commands add keynum as a parm
				if (bind[0] == '+')
					Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "%s %d", bind, key));
				else
				{
					Cbuf_AddTextLine (cmd, bind);
				}
			} else if (bind[0] == '+' && !down && keydown[key] == 0)
				Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "-%s %d", bind + 1, key));
		}
		return;
	}

#if CONFIG_MENU
	if (key_consoleactive && isin1(key, K_MOUSE1)) {
		int mouse_action_happened = Consel_Key_Event_Check_Did_Action (down);
		if (mouse_action_happened == TREAT_MOUSEUP_2) {
			Consel_MouseReset ("MR_ToggleMenu MOUSE1 UP"); MR_ToggleMenu(1); // MOUSE1 UP
			return;
		} else 
		if (mouse_action_happened == TREAT_CONSUMED_MOUSE_ACTION_1) return; // GET OUT
	} // End select cursor

	// Baker: If we are disconnected or fully connected open menu (don't open menu during signon process -- but does that achieve that?  And does it matter?).
	if (down &&
		isin2(key, K_MOUSE1, K_MOUSE2) &&
		(keydest == key_console || (keydest == key_game && cls.demoplayback)) &&
		(cls.state != ca_connected ||
		(cls.state == ca_connected && cls.signon == SIGNONS_4))) { // KEY_EVENT

		WARP_X_(M_ToggleMenu)
		if (Have_Flag(key_consoleactive, KEY_CONSOLEACTIVE_USER_1)) // conexit
			Con_ToggleConsole();

		WARP_X_ (M_ToggleMenu) // Goes to M_ToggleMenu without CSQC
#ifdef CONFIG_MENU
		Consel_MouseReset ("MR_ToggleMenu MOUSE1"); MR_ToggleMenu(1); // MOUSE1
#endif
		return;
	}
#endif // CONFIG_MENU



	// send input to console if it wants it
	if (keydest == key_console)
	{
		if (!down)
			return;
		// con_closeontoggleconsole enables toggleconsole keys to close the
		// console, as long as they are not the color prefix character
		// (special exemption for german keyboard layouts)
		if (con_closeontoggleconsole.integer &&
			bind &&
			String_Starts_With (bind, "toggleconsole") &&
			Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_USER_1) &&
			(con_closeontoggleconsole.integer >= ((ascii != STRING_COLOR_TAG) ? 2 : 3) || key_linepos == 1))
		{
			Con_ToggleConsole (); // Baker: This is high priority toggle console, right?
			return;
		}

		if (Sys_CheckParm ("-noconsole"))
			return; // only allow the key bind to turn off console

		Key_Console (cmd, key, ascii);
		return;
	}

	// handle toggleconsole in menu too
	if (keydest == key_menu)
	{
		if (down && con_closeontoggleconsole.integer &&
			bind &&
			String_Starts_With(bind, "toggleconsole") &&
			ascii != STRING_COLOR_TAG)
		{
			Cbuf_AddTextLine (cmd, "toggleconsole");  // Deferred to next frame so we're not sending the text event to the console.
			tbl_keydest[key] = key_void; // key release should go nowhere (especially not to key_menu or key_game)
			return;
		}
	}

	// ignore binds while a video is played, let the video system handle the key event
	if (cl_videoplaying)
	{
		if (gamemode == GAME_BLOODOMNICIDE) // menu controls key events
#ifdef CONFIG_MENU
			MR_KeyEvent(key, ascii, down);
#else
			{
			}
#endif
		else
			CL_Video_KeyEvent (key, ascii, keydown[key] != 0);
		return;
	}

	// anything else is a key press into the game, chat line, or menu
	switch (keydest)
	{
		case key_message:
			if (down)
				Key_Message (cmd, key, ascii);
			break;
		case key_menu:
		case key_menu_grabbed:
#ifdef CONFIG_MENU
			MR_KeyEvent (key, ascii, down);
#endif
			break;
		case key_game:
			q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
			// ignore key repeats on binds and only send the bind if the event hasnt been already processed by csqc
			if (!q && bind)
			{
				if (keydown[key] == 1 && down)
				{
					// First key down
					// button commands add keynum as a parm
					if (bind[0] == '+') {
						Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "%s %d", bind, key));
					}
					else
					{
						Cbuf_AddTextLine (cmd, bind);
					}
				} else if (bind[0] == '+' && !down && keydown[key] == 0) {
					// Key up
					Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "-%s %d", bind + 1, key));
				}
			}
			break;
		default:
			Con_PrintLinef ("Key_Event: Bad key_dest");
	}
}

// a helper to simulate release of ALL keys
void Key_ReleaseAll(void)
{
	extern kbutton_t	in_mlook, in_klook;
	extern kbutton_t	in_left, in_right, in_forward, in_back;
	extern kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
	extern kbutton_t	in_strafe, in_speed, in_jump, in_attack, in_use;
#if 0 // gcc unused
    extern kbutton_t	in_up, in_down;
#endif // 0
	// LadyHavoc: added 6 new buttons
	extern kbutton_t	in_button3, in_button4, in_button5, in_button6, in_button7, in_button8;
	//even more
	extern kbutton_t	in_button9, in_button10, in_button11, in_button12, in_button13, in_button14, in_button15, in_button16;


#define ClearBtnState(x) \
	if (x.state) { \
		memset (&x, 0, sizeof(x)); \
	} // Ender

	Con_DPrintLinef ("Key_ReleaseAll");
	int key;
	// clear the event queue first
	eventqueue_idx = 0;
	// then send all down events (possibly into the event queue)
	for (key = 0; key < MAX_KEYS_44032; ++key) {
		// Baker: What if keydown[key] > 1?
		if (keydown[key])
			Key_Event(/*scancode*/ key, /*ascii*/ 0, /*isdown*/ false);
		if (developer.integer > 0 && keydown[key]) {
			Con_DPrintLinef ("Key_ReleaseAll key %d is still down", key);
		}

	}
	// now all keys are guaranteed up (once the event queue is unblocked)
	// and only future events count

	ClearBtnState(in_mlook);
	ClearBtnState(in_klook);
	ClearBtnState(in_left);
	ClearBtnState(in_right);
	ClearBtnState(in_moveleft);
	ClearBtnState(in_moveright);
	ClearBtnState(in_forward);
	ClearBtnState(in_back);
	ClearBtnState(in_lookup);
	ClearBtnState(in_lookdown);
	ClearBtnState(in_strafe);
	ClearBtnState(in_speed);
	ClearBtnState(in_jump);
	ClearBtnState(in_attack);
	ClearBtnState(in_use);
	ClearBtnState(in_button3);
	ClearBtnState(in_button4);
	ClearBtnState(in_button5);
	ClearBtnState(in_button6);
	ClearBtnState(in_button7);
	ClearBtnState(in_button8);
	ClearBtnState(in_button9);
	ClearBtnState(in_button10);
	ClearBtnState(in_button11);
	ClearBtnState(in_button12);
	ClearBtnState(in_button13);
	ClearBtnState(in_button14);
	ClearBtnState(in_button15);
	ClearBtnState(in_button16);

}

void Key_ReleaseAll_f(cmd_state_t *cmd)
{
	Key_ReleaseAll ();
}

void Key_Shutdown (void)
{
	Key_History_Shutdown();
}

void Key_InitOnce (void)
{
	Key_History_Init();
	key_linepos = Key_ClearEditLine(true);

//
// register our functions
//
	Cmd_AddCommand(CF_CLIENT, "in_bind", Key_In_Bind_f, "binds a command to the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_unbind", Key_In_Unbind_f, "removes command on the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindlist", Key_In_BindList_f, "bindlist: displays bound keys for all bindmaps, or the given bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindmap", Key_In_Bindmap_f, "selects active foreground and background (used only if a key is not bound in the foreground) bindmaps for typing");

	Cmd_AddCommand(CF_CLIENT, "in_releaseall", Key_ReleaseAll_f, "releases all currently pressed keys (debug command)");

	Cmd_AddCommand(CF_CLIENT, "bind", Key_Bind_f, "binds a command to the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "unbind", Key_Unbind_f, "removes a command on the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "bindlist", Key_BindList_f, "bindlist: displays bound keys for bindmap 0 bindmaps");
	Cmd_AddCommand(CF_CLIENT, "unbindall", Key_Unbindall_f, "removes all commands from all keys in all bindmaps (leaving only shift-escape and escape)");

	Cmd_AddCommand(CF_CLIENT, "history", Key_History_f, "prints the history of executed commands (history X prints the last X entries, history -c clears the whole history)");

	Cvar_RegisterVariable (&con_closeontoggleconsole);
}
