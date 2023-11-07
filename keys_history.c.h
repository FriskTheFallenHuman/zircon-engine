// keys_history.c.h


static void Key_History_Init(void)
{
	qfile_t *historyfile;
	ConBuffer_Init(&history, HIST_TEXTSIZE, HIST_MAXLINES, zonemempool);

// not necessary for mobile
//#ifndef DP_MOBILETOUCH  Baker: say yes we want! 
	// Baker 1004.3
	historyfile = FS_OpenRealFile("zircon_history.txt", "rb", false); // rb to handle unix line endings on windows too Baker 1004.2
	if(historyfile)
	{
		char buf[MAX_INPUTLINE];
		int bufpos;
		int c;

		bufpos = 0;
		for(;;)
		{
			c = FS_Getc(historyfile);
			if(c < 0 || c == 0 || c == '\r' || c == '\n')
			{
				if(bufpos > 0)
				{
					buf[bufpos] = 0;
					ConBuffer_AddLine(&history, buf, bufpos, 0);
					bufpos = 0;
				}
				if(c < 0)
					break;
			}
			else
			{
				if(bufpos < MAX_INPUTLINE - 1)
					buf[bufpos++] = c;
			}
		}

		FS_Close(historyfile);
	}
//#endif

	history_line = -1;
}

void Key_History_Write (void)
{
	// TODO write history to a file

// not necessary for mobile
//#ifndef DP_MOBILETOUCH
	// Baker 1004.4
	qfile_t *historyfile = FS_OpenRealFile("zircon_history.txt", "w", false); // Baker 1004.1
	if(historyfile) {
		int i;
		for(i = 0; i < CONBUFFER_LINES_COUNT(&history); ++i)
			FS_Printf(historyfile, "%s\n", ConBuffer_GetLine(&history, i));
		FS_Close(historyfile);
	}
//#endif
}

static void Key_History_Shutdown(void)
{
	// TODO write history to a file
	Key_History_Write ();
// not necessary for mobile
//#ifndef DP_MOBILETOUCH
//	// Baker 1004.4
//	qfile_t *historyfile = FS_OpenRealFile("zircon_history.txt", "w", false); // Baker 1004.1
//	if(historyfile)
//	{
//		int i;
//		for(i = 0; i < CONBUFFER_LINES_COUNT(&history); ++i)
//			FS_Printf(historyfile, "%s\n", ConBuffer_GetLine(&history, i));
//		FS_Close(historyfile);
//	}
//#endif
//
	ConBuffer_Shutdown(&history);
}

void Con_History_Maybe_Push (const char *stext)
{
	do {
		// Disqualifiers? Go!
		if (stext == 0)											break; // empty text
		if (String_Does_Start_With (stext, "quit") )			break; // quit
		if (String_Does_Start_With (stext, "rcon_password"))	break; // rcon_password

		// not disqualified ...
		char sline[MAX_INPUTLINE];
		c_strlcpy (sline, stext);
		String_Edit_RTrim_Whitespace_Including_Spaces (sline);

		if (sline[0] == 0)										break; // empty text

		if (1) {
			int lastline = CONBUFFER_LINES_COUNT(&history) - 1;
			if (lastline != -1) {
				char sdupchek[MAX_INPUTLINE] = {0};
				c_strlcpy (sdupchek, ConBuffer_GetLine (&history, lastline));
				if (String_Does_Match (sline, sdupchek))		{
																break; // dup!
				}
			}
		}

		ConBuffer_AddLine (&history, sline, (int)strlen(sline), CON_MASK_NONE_0);
	} while (0);
}

static void Key_History_Push (void)
{
	const char *stext = &key_line[1];
	Con_History_Maybe_Push (stext);
	
	Con_PrintLinef ("%s", key_line); // don't mark empty lines as history
	history_line = -1;
	if (history_matchfound)
		history_matchfound = false;
}

static qbool Key_History_Get_foundCommand(void)
{
	if (!history_matchfound)
		return false;
	strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
	key_linepos = (int)strlen(key_line);
	history_matchfound = false;
	return true;
}

static void Key_History_Up(void)
{
	if (history_line == -1) // editing the "new" line
		c_strlcpy (history_savedline, key_line + 1);

	if (Key_History_Get_foundCommand())
		return;

	if (history_line == -1) {
		history_line = CONBUFFER_LINES_COUNT(&history) - 1;
		if (history_line != -1) {
			strlcpy(key_line + 1, ConBuffer_GetLine (&history, history_line), sizeof(key_line) - 1);
			key_linepos = (int)strlen(key_line);
		}
	}
	else if (history_line > 0)
	{
		--history_line; // this also does -1 -> 0, so it is good
		strlcpy (key_line + 1, ConBuffer_GetLine (&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Down(void)
{
	if(history_line == -1) // editing the "new" line
		return;

	if (Key_History_Get_foundCommand())
		return;

	if(history_line < CONBUFFER_LINES_COUNT(&history) - 1)
	{
		++history_line;
		strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
	}
	else
	{
		history_line = -1;
		strlcpy(key_line + 1, history_savedline, sizeof(key_line) - 1);
	}

	key_linepos = (int)strlen(key_line);
}

static void Key_History_First(void)
{
	if(history_line == -1) // editing the "new" line
		strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (CONBUFFER_LINES_COUNT(&history) > 0)
	{
		history_line = 0;
		strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Last(void)
{
	if(history_line == -1) // editing the "new" line
		strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (CONBUFFER_LINES_COUNT(&history) > 0)
	{
		history_line = CONBUFFER_LINES_COUNT(&history) - 1;
		strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Find_Backwards(void)
{
	int i;
	const char *partial = key_line + 1;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (history_line == -1) // editing the "new" line
		strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (strcmp(key_line + 1, history_searchstring)) // different string? Start a new search
	{
		strlcpy(history_searchstring, key_line + 1, sizeof(history_searchstring));
		i = CONBUFFER_LINES_COUNT(&history) - 1;
	}
	else if (history_line == -1)
		i = CONBUFFER_LINES_COUNT(&history) - 1;
	else
		i = history_line - 1;

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for ( ; i >= 0; i--)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("^2%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			history_line = i;
			history_matchfound = true;
			return;
		}
}

static void Key_History_Find_Forwards(void)
{
	int i;
	const char *partial = key_line + 1;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (history_line == -1) // editing the "new" line
		return;

	if (strcmp(key_line + 1, history_searchstring)) // different string? Start a new search
	{
		strlcpy(history_searchstring, key_line + 1, sizeof(history_searchstring));
		i = 0;
	}
	else i = history_line + 1;

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for ( ; i < CONBUFFER_LINES_COUNT(&history); i++)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("^2%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			history_line = i;
			history_matchfound = true;
			return;
		}
}

static void Key_History_Find_All(void)
{
	const char *partial = key_line + 1;
	int i, count = 0;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));
	Con_Printf("History commands containing \"%s\":\n", key_line + 1);

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for (i=0; i<CONBUFFER_LINES_COUNT(&history); i++)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("%s%*i^7 %s\n", (i == history_line) ? "^2" : "^3", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			count++;
		}
	Con_Printf("%i result%s\n\n", count, (count != 1) ? "s" : "");
}

// "history" cmd
static void Key_History_f(void)
{
	char *errchar = NULL;
	int i = 0;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (Cmd_Argc () > 1)
	{
		if (String_Does_Match(Cmd_Argv (1), "-c"))
		{
			ConBuffer_Clear(&history);
			return;
		}
		i = strtol(Cmd_Argv (1), &errchar, 0);
		if ((i < 0) || (i > CONBUFFER_LINES_COUNT(&history)) || (errchar && *errchar))
			i = 0;
		else
			i = CONBUFFER_LINES_COUNT(&history) - i;
	}

	for ( ; i<CONBUFFER_LINES_COUNT(&history); i++)
		Con_Printf("^3%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
	Con_Printf("\n");
}

static int	key_bmap, key_bmap2;
static unsigned char keydown[MAX_KEYS];	// 0 = up, 1 = down, 2 = repeating

typedef struct keyname_s
{
	const char	*name;
	int			keynum;
}
keyname_t;

static const keyname_t   keynames[] = {
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},

	// spacer so it lines up with keys.h

	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INSERT},
	{"DEL", K_DELETE},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"PAUSE", K_PAUSE},

	{"NUMLOCK", K_NUMLOCK},
	{"CAPSLOCK", K_CAPSLOCK},
	{"SCROLLOCK", K_SCROLLOCK},

	{"KP_INS",			K_KP_INSERT },
	{"KP_0", K_KP_0},
	{"KP_END",			K_KP_END },
	{"KP_1", K_KP_1},
	{"KP_DOWNARROW",	K_KP_DOWNARROW },
	{"KP_2", K_KP_2},
	{"KP_PGDN",			K_KP_PGDN },
	{"KP_3", K_KP_3},
	{"KP_LEFTARROW",	K_KP_LEFTARROW },
	{"KP_4", K_KP_4},
	{"KP_5", K_KP_5},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW },
	{"KP_6", K_KP_6},
	{"KP_HOME",			K_KP_HOME },
	{"KP_7", K_KP_7},
	{"KP_UPARROW",		K_KP_UPARROW },
	{"KP_8", K_KP_8},
	{"KP_PGUP",			K_KP_PGUP },
	{"KP_9", K_KP_9},
	{"KP_DEL",			K_KP_DELETE },
	{"KP_PERIOD", K_KP_PERIOD},
	{"KP_SLASH",		K_KP_SLASH },
	{"KP_DIVIDE", K_KP_DIVIDE},
	{"KP_MULTIPLY", K_KP_MULTIPLY},
	{"KP_MINUS", K_KP_MINUS},
	{"KP_PLUS", K_KP_PLUS},
	{"KP_ENTER", K_KP_ENTER},
	{"KP_EQUALS", K_KP_EQUALS},

	{"PRINTSCREEN", K_PRINTSCREEN},



	{"MOUSE1", K_MOUSE1},

	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MWHEELUP", K_MWHEELUP},
	{"MWHEELDOWN", K_MWHEELDOWN},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},
	{"MOUSE6", K_MOUSE6},
	{"MOUSE7", K_MOUSE7},
	{"MOUSE8", K_MOUSE8},
	{"MOUSE9", K_MOUSE9},
	{"MOUSE10", K_MOUSE10},
	{"MOUSE11", K_MOUSE11},
	{"MOUSE12", K_MOUSE12},
	{"MOUSE13", K_MOUSE13},
	{"MOUSE14", K_MOUSE14},
	{"MOUSE15", K_MOUSE15},
	{"MOUSE16", K_MOUSE16},




	{"JOY1",  K_JOY1},
	{"JOY2",  K_JOY2},
	{"JOY3",  K_JOY3},
	{"JOY4",  K_JOY4},
	{"JOY5",  K_JOY5},
	{"JOY6",  K_JOY6},
	{"JOY7",  K_JOY7},
	{"JOY8",  K_JOY8},
	{"JOY9",  K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},






	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"X360_DPAD_UP", K_X360_DPAD_UP},
	{"X360_DPAD_DOWN", K_X360_DPAD_DOWN},
	{"X360_DPAD_LEFT", K_X360_DPAD_LEFT},
	{"X360_DPAD_RIGHT", K_X360_DPAD_RIGHT},
	{"X360_START", K_X360_START},
	{"X360_BACK", K_X360_BACK},
	{"X360_LEFT_THUMB", K_X360_LEFT_THUMB},
	{"X360_RIGHT_THUMB", K_X360_RIGHT_THUMB},
	{"X360_LEFT_SHOULDER", K_X360_LEFT_SHOULDER},
	{"X360_RIGHT_SHOULDER", K_X360_RIGHT_SHOULDER},
	{"X360_A", K_X360_A},
	{"X360_B", K_X360_B},
	{"X360_X", K_X360_X},
	{"X360_Y", K_X360_Y},
	{"X360_LEFT_TRIGGER", K_X360_LEFT_TRIGGER},
	{"X360_RIGHT_TRIGGER", K_X360_RIGHT_TRIGGER},
	{"X360_LEFT_THUMB_UP", K_X360_LEFT_THUMB_UP},
	{"X360_LEFT_THUMB_DOWN", K_X360_LEFT_THUMB_DOWN},
	{"X360_LEFT_THUMB_LEFT", K_X360_LEFT_THUMB_LEFT},
	{"X360_LEFT_THUMB_RIGHT", K_X360_LEFT_THUMB_RIGHT},
	{"X360_RIGHT_THUMB_UP", K_X360_RIGHT_THUMB_UP},
	{"X360_RIGHT_THUMB_DOWN", K_X360_RIGHT_THUMB_DOWN},
	{"X360_RIGHT_THUMB_LEFT", K_X360_RIGHT_THUMB_LEFT},
	{"X360_RIGHT_THUMB_RIGHT", K_X360_RIGHT_THUMB_RIGHT},

	{"JOY_UP", K_JOY_UP},
	{"JOY_DOWN", K_JOY_DOWN},
	{"JOY_LEFT", K_JOY_LEFT},
	{"JOY_RIGHT", K_JOY_RIGHT},

	{"SEMICOLON", ';'},			// because a raw semicolon separates commands
	{"TILDE", '~'},
	{"BACKQUOTE", '`'},
	{"QUOTE", '"'},
	{"APOSTROPHE", '\''},
	{"BACKSLASH", '\\'},		// because a raw backslash is used for special characters

	{"MIDINOTE0", K_MIDINOTE0},
	{"MIDINOTE1", K_MIDINOTE1},
	{"MIDINOTE2", K_MIDINOTE2},
	{"MIDINOTE3", K_MIDINOTE3},
	{"MIDINOTE4", K_MIDINOTE4},
	{"MIDINOTE5", K_MIDINOTE5},
	{"MIDINOTE6", K_MIDINOTE6},
	{"MIDINOTE7", K_MIDINOTE7},
	{"MIDINOTE8", K_MIDINOTE8},
	{"MIDINOTE9", K_MIDINOTE9},
	{"MIDINOTE10", K_MIDINOTE10},
	{"MIDINOTE11", K_MIDINOTE11},
	{"MIDINOTE12", K_MIDINOTE12},
	{"MIDINOTE13", K_MIDINOTE13},
	{"MIDINOTE14", K_MIDINOTE14},
	{"MIDINOTE15", K_MIDINOTE15},
	{"MIDINOTE16", K_MIDINOTE16},
	{"MIDINOTE17", K_MIDINOTE17},
	{"MIDINOTE18", K_MIDINOTE18},
	{"MIDINOTE19", K_MIDINOTE19},
	{"MIDINOTE20", K_MIDINOTE20},
	{"MIDINOTE21", K_MIDINOTE21},
	{"MIDINOTE22", K_MIDINOTE22},
	{"MIDINOTE23", K_MIDINOTE23},
	{"MIDINOTE24", K_MIDINOTE24},
	{"MIDINOTE25", K_MIDINOTE25},
	{"MIDINOTE26", K_MIDINOTE26},
	{"MIDINOTE27", K_MIDINOTE27},
	{"MIDINOTE28", K_MIDINOTE28},
	{"MIDINOTE29", K_MIDINOTE29},
	{"MIDINOTE30", K_MIDINOTE30},
	{"MIDINOTE31", K_MIDINOTE31},
	{"MIDINOTE32", K_MIDINOTE32},
	{"MIDINOTE33", K_MIDINOTE33},
	{"MIDINOTE34", K_MIDINOTE34},
	{"MIDINOTE35", K_MIDINOTE35},
	{"MIDINOTE36", K_MIDINOTE36},
	{"MIDINOTE37", K_MIDINOTE37},
	{"MIDINOTE38", K_MIDINOTE38},
	{"MIDINOTE39", K_MIDINOTE39},
	{"MIDINOTE40", K_MIDINOTE40},
	{"MIDINOTE41", K_MIDINOTE41},
	{"MIDINOTE42", K_MIDINOTE42},
	{"MIDINOTE43", K_MIDINOTE43},
	{"MIDINOTE44", K_MIDINOTE44},
	{"MIDINOTE45", K_MIDINOTE45},
	{"MIDINOTE46", K_MIDINOTE46},
	{"MIDINOTE47", K_MIDINOTE47},
	{"MIDINOTE48", K_MIDINOTE48},
	{"MIDINOTE49", K_MIDINOTE49},
	{"MIDINOTE50", K_MIDINOTE50},
	{"MIDINOTE51", K_MIDINOTE51},
	{"MIDINOTE52", K_MIDINOTE52},
	{"MIDINOTE53", K_MIDINOTE53},
	{"MIDINOTE54", K_MIDINOTE54},
	{"MIDINOTE55", K_MIDINOTE55},
	{"MIDINOTE56", K_MIDINOTE56},
	{"MIDINOTE57", K_MIDINOTE57},
	{"MIDINOTE58", K_MIDINOTE58},
	{"MIDINOTE59", K_MIDINOTE59},
	{"MIDINOTE60", K_MIDINOTE60},
	{"MIDINOTE61", K_MIDINOTE61},
	{"MIDINOTE62", K_MIDINOTE62},
	{"MIDINOTE63", K_MIDINOTE63},
	{"MIDINOTE64", K_MIDINOTE64},
	{"MIDINOTE65", K_MIDINOTE65},
	{"MIDINOTE66", K_MIDINOTE66},
	{"MIDINOTE67", K_MIDINOTE67},
	{"MIDINOTE68", K_MIDINOTE68},
	{"MIDINOTE69", K_MIDINOTE69},
	{"MIDINOTE70", K_MIDINOTE70},
	{"MIDINOTE71", K_MIDINOTE71},
	{"MIDINOTE72", K_MIDINOTE72},
	{"MIDINOTE73", K_MIDINOTE73},
	{"MIDINOTE74", K_MIDINOTE74},
	{"MIDINOTE75", K_MIDINOTE75},
	{"MIDINOTE76", K_MIDINOTE76},
	{"MIDINOTE77", K_MIDINOTE77},
	{"MIDINOTE78", K_MIDINOTE78},
	{"MIDINOTE79", K_MIDINOTE79},
	{"MIDINOTE80", K_MIDINOTE80},
	{"MIDINOTE81", K_MIDINOTE81},
	{"MIDINOTE82", K_MIDINOTE82},
	{"MIDINOTE83", K_MIDINOTE83},
	{"MIDINOTE84", K_MIDINOTE84},
	{"MIDINOTE85", K_MIDINOTE85},
	{"MIDINOTE86", K_MIDINOTE86},
	{"MIDINOTE87", K_MIDINOTE87},
	{"MIDINOTE88", K_MIDINOTE88},
	{"MIDINOTE89", K_MIDINOTE89},
	{"MIDINOTE90", K_MIDINOTE90},
	{"MIDINOTE91", K_MIDINOTE91},
	{"MIDINOTE92", K_MIDINOTE92},
	{"MIDINOTE93", K_MIDINOTE93},
	{"MIDINOTE94", K_MIDINOTE94},
	{"MIDINOTE95", K_MIDINOTE95},
	{"MIDINOTE96", K_MIDINOTE96},
	{"MIDINOTE97", K_MIDINOTE97},
	{"MIDINOTE98", K_MIDINOTE98},
	{"MIDINOTE99", K_MIDINOTE99},
	{"MIDINOTE100", K_MIDINOTE100},
	{"MIDINOTE101", K_MIDINOTE101},
	{"MIDINOTE102", K_MIDINOTE102},
	{"MIDINOTE103", K_MIDINOTE103},
	{"MIDINOTE104", K_MIDINOTE104},
	{"MIDINOTE105", K_MIDINOTE105},
	{"MIDINOTE106", K_MIDINOTE106},
	{"MIDINOTE107", K_MIDINOTE107},
	{"MIDINOTE108", K_MIDINOTE108},
	{"MIDINOTE109", K_MIDINOTE109},
	{"MIDINOTE110", K_MIDINOTE110},
	{"MIDINOTE111", K_MIDINOTE111},
	{"MIDINOTE112", K_MIDINOTE112},
	{"MIDINOTE113", K_MIDINOTE113},
	{"MIDINOTE114", K_MIDINOTE114},
	{"MIDINOTE115", K_MIDINOTE115},
	{"MIDINOTE116", K_MIDINOTE116},
	{"MIDINOTE117", K_MIDINOTE117},
	{"MIDINOTE118", K_MIDINOTE118},
	{"MIDINOTE119", K_MIDINOTE119},
	{"MIDINOTE120", K_MIDINOTE120},
	{"MIDINOTE121", K_MIDINOTE121},
	{"MIDINOTE122", K_MIDINOTE122},
	{"MIDINOTE123", K_MIDINOTE123},
	{"MIDINOTE124", K_MIDINOTE124},
	{"MIDINOTE125", K_MIDINOTE125},
	{"MIDINOTE126", K_MIDINOTE126},
	{"MIDINOTE127", K_MIDINOTE127},

	{NULL, 0}
};

WARP_X_ (GetGameCommands_Count)
int GetXKeyList_Count (const char *s_prefix)
{
	int				nummy = (int)ARRAY_COUNT(keynames);

	int				num_found = 0;
	int				j;
	stringlist_t	matchedSet;
	stringlistinit  (&matchedSet); // this does not allocate

	if (1) {
		for (j = 0; j < nummy; j++) {
			const char *sxy =  keynames[j].name;
			if (!sxy) continue;	// yay!  the last entry is bad
			if (String_Does_Start_With_Caseless (sxy, "KP_"))	continue;	// do not want JOY, Xbox live for now
			if (String_Does_Start_With_Caseless (sxy, "AUX"))   continue;	// do not want
			if (String_Does_Start_With_Caseless (sxy, "MIDI"))  continue;	// do not want
				
			if (String_Does_Start_With_Caseless (sxy, s_prefix) == false)
				continue;

			stringlistappend (&matchedSet, sxy);
			
		} // while

		// SORT
		stringlistsort (&matchedSet, true);

		for (j = 0; j < matchedSet.numstrings; j ++) {
			char *sxy = matchedSet.strings[j];
			if (String_Does_Start_With_Caseless (sxy, s_prefix) == false)
				continue;

			num_found ++;

			SPARTIAL_EVAL_
		} // for

		stringlistfreecontents (&matchedSet);

		return num_found;
	}

	return 0;
}

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

// no callers?
void
Key_ClearEditLine (int edit_line)
{
	memset (key_line, '\0', sizeof(key_line));
	key_line[0] = ']';
	key_linepos = 1;
	Partial_Reset (); Con_Undo_Clear (); Selection_Line_Reset_Clear (); // 
}


