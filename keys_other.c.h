// keys_other.c.h


//============================================================================

int chat_mode;
char		chat_buffer[MAX_INPUTLINE];
unsigned int	chat_bufferlen = 0;

static void
Key_Message (int key, int ascii)
{
	char vabuf[1024];
	if (key == K_ENTER || key == K_KP_ENTER || ascii == 10 || ascii == 13)
	{
		if(chat_mode < 0)
			Cmd_ExecuteString(chat_buffer, src_command, true); // not Cbuf_AddText to allow semiclons in args; however, this allows no variables then. Use aliases!
		else
			CL_ForwardToServer(va(vabuf, sizeof(vabuf), "%s %s", chat_mode ? "say_team" : "say ", chat_buffer));

		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	// TODO add support for arrow keys and simple editing

	if (key == K_ESCAPE) {
		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_BACKSPACE) {
		if (chat_bufferlen) {
			chat_bufferlen = (unsigned int)u8_prevbyte(chat_buffer, chat_bufferlen);
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (key == K_TAB) {
		chat_bufferlen = Nicks_CompleteChatLine(chat_buffer, sizeof(chat_buffer), chat_bufferlen);
		return;
	}

	if (keydown[K_CTRL] && key == 'v') {
		char *cbd, *p;
		// Baker: Sys_GetClipboardData is now a static buffer.  At least for now.
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
			if (i + chat_bufferlen >= sizeof (chat_buffer) )
				i= sizeof (chat_buffer) - chat_bufferlen - 1;
			if (i > 0)
			{
				int chatpos = chat_bufferlen;
				cbd[i] = 0;

				memmove(chat_buffer + chatpos + i, chat_buffer + chatpos, sizeof(chat_buffer) - chatpos - i);
				memcpy(chat_buffer + chatpos, cbd, i);
				//key_linepos += i; // Roguez
				//Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			}
			//Z_Free(cbd);
			chat_bufferlen = (unsigned int)strlen(chat_buffer);
		}

		return;
	}

	// ctrl+key generates an ascii value < 32 and shows a char from the charmap
	if (ascii > 0 && ascii < 32 && utf8_enable.integer)
		ascii = 0xE000 + ascii;

	if (chat_bufferlen == sizeof (chat_buffer) - 1)
		return;							// all full

	if (!ascii)
		return;							// non printable

	chat_bufferlen += u8_fromchar(ascii, chat_buffer+chat_bufferlen, sizeof(chat_buffer) - chat_bufferlen - 1);

	//chat_buffer[chat_bufferlen++] = ascii;
	//chat_buffer[chat_bufferlen] = 0;
}

//============================================================================


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
	const keyname_t  *kn;

	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn = keynames; kn->name; kn++) {
		if (String_Does_Match_Caseless (str, kn->name))
			return kn->keynum;
	}
	return -1;
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
	if (keynum > 32 && keynum < 256)
	{
		if (tinystrlength >= 2)
		{
			tinystr[0] = keynum;
			tinystr[1] = 0;
		}
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

	if (keynum == -1 || keynum >= MAX_KEYS)
		return false;
	if ((bindmap < 0) || (bindmap >= MAX_BINDMAPS))
		return false;

// free old bindings
	if (keybindings[bindmap][keynum]) {
		Z_Free (keybindings[bindmap][keynum]);
		keybindings[bindmap][keynum] = NULL;
	}
	if(!binding[0]) // make "" binds be removed --blub
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
	if(fg)
		*fg = key_bmap;
	if(bg)
		*bg = key_bmap2;
}

qbool Key_SetBindMap(int fg, int bg)
{
	if(fg >= MAX_BINDMAPS)
		return false;
	if(bg >= MAX_BINDMAPS)
		return false;
	if(fg >= 0)
		key_bmap = fg;
	if(bg >= 0)
		key_bmap2 = bg;
	return true;
}

static void
Key_In_Unbind_f (void)
{
	int         b, m;
	char *errchar = NULL;

	if (Cmd_Argc () != 3) {
		Con_Print("in_unbind <bindmap> <key> : remove commands from a key\n");
		return;
	}

	m = strtol(Cmd_Argv (1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv (2));
	if (b == -1) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv (2));
		return;
	}

	if(!Key_SetBinding (b, m, ""))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bind_f (void)
{
	int         i, c, b, m;
	char        cmd[MAX_INPUTLINE];
	char *errchar = NULL;

	c = Cmd_Argc ();

	if (c != 3 && c != 4) {
		Con_Print("in_bind <bindmap> <key> [command] : attach a command to a key\n");
		return;
	}

	m = strtol(Cmd_Argv (1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv (2));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv (2));
		return;
	}

	if (c == 3) {
		if (keybindings[m][b])
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv (2), keybindings[m][b]);
		else
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv (2));
		return;
	}
// copy the rest of the command line
	cmd[0] = 0;							// start out with a null string
	for (i = 3; i < c; i++) {
		strlcat (cmd, Cmd_Argv (i), sizeof (cmd));
		if (i != (c - 1))
			strlcat (cmd, " ", sizeof (cmd));
	}

	if(!Key_SetBinding (b, m, cmd))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bindmap_f (void)
{
	int         m1, m2, c;
	char *errchar = NULL;

	c = Cmd_Argc ();

	if (c != 3) {
		Con_Print("in_bindmap <bindmap> <fallback>: set current bindmap and fallback\n");
		return;
	}

	m1 = strtol(Cmd_Argv (1), &errchar, 0);
	if ((m1 < 0) || (m1 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(1));
		return;
	}

	m2 = strtol(Cmd_Argv (2), &errchar, 0);
	if ((m2 < 0) || (m2 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(2));
		return;
	}

	key_bmap = m1;
	key_bmap2 = m2;
}

static void
Key_Unbind_f (void)
{
	int         b;

	if (Cmd_Argc () != 2) {
		Con_Print("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b == -1) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	if(!Key_SetBinding (b, 0, ""))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_Unbindall_f (void)
{
	int         i, j;

	for (j = 0; j < MAX_BINDMAPS; j++)
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
			if (keybindings[j][i])
				Key_SetBinding (i, j, "");
}

static void
Key_PrintBindList(int j)
{
	char bindbuf[MAX_INPUTLINE];
	char tinystr[2];
	const char *p;
	int i;

	for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
	{
		p = keybindings[j][i];
		if (p)
		{
			Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false);
			if (j == 0)
				Con_Printf("^2%s ^7= \"%s\"\n", Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
			else
				Con_Printf("^3bindmap %d: ^2%s ^7= \"%s\"\n", j, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
		}
	}
}

static void
Key_In_BindList_f (void)
{
	int m;
	char *errchar = NULL;

	if(Cmd_Argc() >= 2)
	{
		m = strtol(Cmd_Argv(1), &errchar, 0);
		if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
			Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(1));
			return;
		}
		Key_PrintBindList(m);
	}
	else
	{
		for (m = 0; m < MAX_BINDMAPS; m++)
			Key_PrintBindList(m);
	}
}

static void
Key_BindList_f (void)
{
	Key_PrintBindList(0);
}

static void
Key_Bind_f (void)
{
	int         i, c, b;
	char        cmd[MAX_INPUTLINE];

	c = Cmd_Argc ();

	if (c != 2 && c != 3) {
		Con_Print("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv (1));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv (1));
		return;
	}

	if (c == 2) {
		if (keybindings[0][b])
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv (1), keybindings[0][b]);
		else
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv (1));
		return;
	}
// copy the rest of the command line
	cmd[0] = 0;							// start out with a null string
	for (i = 2; i < c; i++) {
		strlcat (cmd, Cmd_Argv (i), sizeof (cmd));
		if (i != (c - 1))
			strlcat (cmd, " ", sizeof (cmd));
	}

	if(!Key_SetBinding (b, 0, cmd))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
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
	char bindbuf[MAX_INPUTLINE];
	char tinystr[2];
	const char *p;

	for (j = 0; j < MAX_BINDMAPS; j++)
	{
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
		{
			p = keybindings[j][i];
			if (p)
			{
				Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false); // don't need to escape $ because cvars are not expanded inside bind
				if (j == 0)
					FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
				else
					FS_Printf(f, "in_bind %d %s \"%s\"\n", j, Key_KeynumToString (i, tinystr, sizeof(tinystr)), bindbuf);
			}
		}
	}
}


void
Key_Init (void)
{
	Key_History_Init();
	key_line[0] = ']';
	key_line[1] = 0;
	key_linepos = 1;
	Partial_Reset ();

//
// register our functions
//
	Cmd_AddCommand ("in_bind", Key_In_Bind_f, "binds a command to the specified key in the selected bindmap");
	Cmd_AddCommand ("in_unbind", Key_In_Unbind_f, "removes command on the specified key in the selected bindmap");
	Cmd_AddCommand ("in_bindlist", Key_In_BindList_f, "bindlist: displays bound keys for all bindmaps, or the given bindmap");
	Cmd_AddCommand ("in_bindmap", Key_In_Bindmap_f, "selects active foreground and background (used only if a key is not bound in the foreground) bindmaps for typing");

	Cmd_AddCommand ("bind", Key_Bind_f, "binds a command to the specified key in bindmap 0");
	Cmd_AddCommand ("unbind", Key_Unbind_f, "removes a command on the specified key in bindmap 0");
	Cmd_AddCommand ("bindlist", Key_BindList_f, "bindlist: displays bound keys for bindmap 0 bindmaps");
	Cmd_AddCommand ("unbindall", Key_Unbindall_f, "removes all commands from all keys in all bindmaps (leaving only shift-escape and escape)");

	Cmd_AddCommand ("history", Key_History_f, "prints the history of executed commands (history X prints the last X entries, history -c clears the whole history)");

	Cvar_RegisterVariable (&con_closeontoggleconsole);
}

void
Key_Shutdown (void)
{
	Key_History_Shutdown();
}

const char *Key_GetBind (int key, int bindmap)
{
	const char *bind;
	if (key < 0 || key >= MAX_KEYS)
		return NULL;
	if(bindmap >= MAX_BINDMAPS)
		return NULL;
	if(bindmap >= 0)
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

	if(bindmap >= MAX_BINDMAPS)
		return;

	count = 0;

	for (j = 0; j < MAX_KEYS; ++j)
	{
		b = Key_GetBind(j, bindmap);
		if (!b)
			continue;
		if (String_Does_Match (b, command) )
		{
			keys[count++] = j;
			if (count == numkeys)
				break;
		}
	}
}

void VID_Alt_Enter_f (void) // Baker 2000
{
#ifdef __ANDROID__
	Con_PrintLinef ("vid_restart not supported for this build");

	return;
#endif // __ANDROID__

	//
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
	Cbuf_AddTextLine ("vid_restart");
}

/*
===================
Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
static char tbl_keyascii[MAX_KEYS];
static keydest_t tbl_keydest[MAX_KEYS];

typedef struct eventqueueitem_s
{
	int key;
	int ascii;
	qbool down;
}
eventqueueitem_t;
static int events_blocked = 0;
static eventqueueitem_t eventqueue[32];
static unsigned eventqueue_idx = 0;

static void Key_EventQueue_Add(int key, int ascii, qbool down)
{
	if(eventqueue_idx < sizeof(eventqueue) / sizeof(*eventqueue))
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

qbool ignore_enter_up = false; // Baker 2000

void
Key_Event (int key, int ascii, qbool down)
{
	const char *bind;
	qbool q;
	keydest_t keydest = key_dest;
	char vabuf[1024];

	if (key < 0 || key >= MAX_KEYS)
		return;

	if(events_blocked)
	{
		Key_EventQueue_Add(key, ascii, down);
		return;
	}

#if 1
	if (key == K_ENTER) {
		if (keydown[K_ALT] && down) { // Baker 2000
			VID_Alt_Enter_f ();
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
		Con_DPrintf("Key_Event(%i, '%c', %s) keydown %i bind \"%s\"\n", key, ascii ? ascii : '?', down ? "down" : "up", keydown[key], bind ? bind : "");

	if(key_consoleactive)
		keydest = key_console;

	if (down)
	{
		// increment key repeat count each time a down is received so that things
		// which want to ignore key repeat can ignore it
		keydown[key] = min(keydown[key] + 1, 2);
		if(keydown[key] == 1) {
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

	if(keydest == key_void)
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
	if (key == K_ESCAPE || (cls.demoplayback && keydest == key_game && isin4 (key, K_ALT, K_TAB, K_SHIFT, K_CTRL)==false ) /**/ ) {
		// ignore key repeats on escape
		if (keydown[key] > 1)
			return;

		if ((cls.demoplayback && keydest == key_game) && con_closeontoggleconsole.integer && bind && String_Does_Match_Caseless (bind, "toggleconsole") ) {
			goto toggleco;
		}

		// escape does these things:
		// key_consoleactive - close console
		// key_message - abort messagemode
		// key_menu - go to parent menu (or key_game)
		// key_game - open menu

		// in all modes shift-escape toggles console
		if (keydown[K_SHIFT] && key == K_ESCAPE)
		{
			if(down)
			{
				Con_ToggleConsole_f ();
				tbl_keydest[key] = key_void; // esc release should go nowhere (especially not to key_menu or key_game)
			}
			return;
		}

		switch (keydest) {
			case key_console:
				if (down)
				{
					if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4)) {
						Flag_Remove_From (key_consoleactive, KEY_CONSOLEACTIVE_USER_1);
#ifdef CONFIG_MENU
						MR_ToggleMenu(1); // conexit
#endif
					}
					else
						Con_ToggleConsole_f(); // conexit
				}
				break;

			case key_message:
				if (down)
					Key_Message (key, ascii); // that'll close the message input
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
					MR_ToggleMenu(1); // conexit
				}
#endif
				break;

			default:
				Con_Printf ("Key_Event: Bad key_dest\n");
		}
		return;
	}


toggleco:

	// send function keydowns to interpreter no matter what mode is (unless the menu has specifically grabbed the keyboard, for rebinding keys)
	// VorteX: Omnicide does bind F* keys
	if (keydest != key_menu_grabbed)
	if (key >= K_F1 && key <= K_F12) {
		if (bind)
		{
			if(keydown[key] == 1 && down)
			{
				// button commands add keynum as a parm
				if (bind[0] == '+')
					Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "%s %d", bind, key));
				else
				{
					Cbuf_AddTextLine (bind);

				}
			} else if(bind[0] == '+' && !down && keydown[key] == 0)
				Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "-%s %d", bind + 1, key));
		}
		return;
	}

	// send input to console if it wants it
	if (keydest == key_console)
	{
		if (!down)
			return;
		// con_closeontoggleconsole enables toggleconsole keys to close the
		// console, as long as they are not the color prefix character
		// (special exemption for german keyboard layouts)
		if (con_closeontoggleconsole.integer && bind && !strncmp(bind, "toggleconsole", strlen("toggleconsole")) && (key_consoleactive & KEY_CONSOLEACTIVE_USER_1) && (con_closeontoggleconsole.integer >= ((ascii != STRING_COLOR_TAG) ? 2 : 3) || key_linepos == 1))
		{
			Con_ToggleConsole_f ();
			return;
		}

		if (Sys_CheckParm ("-noconsole"))
			return; // only allow the key bind to turn off console

		Key_Console (key, ascii);
		return;
	}

	// handle toggleconsole in menu too
	if (keydest == key_menu)
	{
		if (down && con_closeontoggleconsole.integer && bind && !strncmp(bind, "toggleconsole", strlen("toggleconsole")) && ascii != STRING_COLOR_TAG)
		{
			Cbuf_AddTextLine ("toggleconsole");  // Deferred to next frame so we're not sending the text event to the console.
			tbl_keydest[key] = key_void; // key release should go nowhere (especially not to key_menu or key_game)
			return;
		}
	}

	// ignore binds while a video is played, let the video system handle the key event
	if (cl_videoplaying)
	{
		CL_Video_KeyEvent (key, ascii, keydown[key] != 0);
		return;
	}

	// anything else is a key press into the game, chat line, or menu
	switch (keydest)
	{
		case key_message:
			if (down)
				Key_Message (key, ascii);
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
				if(keydown[key] == 1 && down)
				{
					// button commands add keynum as a parm
					if (bind[0] == '+')
						Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "%s %d", bind, key));
					else
					{
						Cbuf_AddTextLine (bind);
					}
				} else if(bind[0] == '+' && !down && keydown[key] == 0)
					Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "-%s %d", bind + 1, key));
			}
			break;
		default:
			Con_Printf ("Key_Event: Bad key_dest\n");
	}
}

// a helper to simulate release of ALL keys
void
Key_ReleaseAll (void)
{
#if 0
	int key;
	// clear the event queue first
	eventqueue_idx = 0;
	// then send all down events (possibly into the event queue)
	for(key = 0; key < MAX_KEYS; ++key)
		if(keydown[key])
			Key_Event(key, 0, false);
	// now all keys are guaranteed down (once the event queue is unblocked)
	// and only future events count
#endif
}

// Baker 1002.2
void Key_Release_Keys (void)
{
	int      keynum;

    for (keynum = 0 ; keynum < MAX_KEYS /*1024 in dp*/; keynum++) {
		if (keydown[keynum])
			Key_Event (keynum, /*ascii*/ 0, /*down*/ false);
	}
}

/*
===================
Key_ClearStates
===================
*/
void
Key_ClearStates (void)
{
	memset(keydown, 0, sizeof(keydown));
}




