// menu_saveload.h

#define		msel_cursoric	m_optcursor		// frame cursor
#define		m_local_cursor		load_cursor

//=============================================================================
/* LOAD/SAVE MENU */

static int		load_cursor;		///< 0 < load_cursor < MAX_SAVEGAMES

static char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
static int		loadable[MAX_SAVEGAMES];

static void M_ScanSaves (void)
{
	int		i, j;
	size_t	len;
	char	name[MAX_OSPATH];
	char	buf[SAVEGAME_COMMENT_LENGTH + 256];
	const char *t;
	qfile_t	*f;
//	int		version;

	for (i=0 ; i < MAX_SAVEGAMES; i++) {
		strlcpy (m_filenames[i], "--- UNUSED SLOT ---", sizeof(m_filenames[i]));
		loadable[i] = false;
		dpsnprintf (name, sizeof(name), "s%d.sav", (int)i);
		f = FS_OpenRealFile (name, "rb", false);
		if (!f)
			continue;
		// read enough to get the comment
		len = FS_Read(f, buf, sizeof(buf) - 1);
		len = min(len, sizeof(buf)-1);
		buf[len] = 0;
		t = buf;
		// version
		COM_ParseToken_Simple(&t, false, false, true);
		//version = atoi(com_token);
		// description
		COM_ParseToken_Simple(&t, false, false, true);
		strlcpy (m_filenames[i], com_token, sizeof (m_filenames[i]));

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		FS_Close (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	menu_state_set_nova (m_load);
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
#if 1
	// LadyHavoc: allow saving multiplayer games
	if (cl.islocalgame && cl.intermission)
		return;
#else
	if (cl.intermission)
		return;
	if (!cl.islocalgame)
		return;
#endif
	m_entersound = true;
	menu_state_set_nova (m_save);
	key_dest = key_menu;
	M_ScanSaves ();
}


static void M_Load_Draw (void)
{
	cachepic_t	*p0;

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_Background(320, 200);

	p0 = Draw_CachePic ("gfx/p_load");
	M_DrawPic ( (320-p0->width)/2, 4, p0 /*CPC("gfx/p_load")*/, NO_HOTSPOTS_0, NA0, NA0);

	drawcur_y = 32;
	for (drawidx = 0; drawidx < MAX_SAVEGAMES ; drawidx ++) {
		Hotspots_Add (menu_x + 16 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider); // PPX DUR
		M_Print(16, drawcur_y, m_filenames[drawidx]);
		if (drawidx == load_cursor) drawsel_idx = drawidx;
		drawcur_y += 8;
	}

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Save_Draw (void)
{
	cachepic_t	*p0;

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_Background(320, 200);

	p0 = Draw_CachePic ("gfx/p_save");
	M_DrawPic ( (320-p0->width)/2, 4, p0 /*CPC("gfx/p_save")*/, NO_HOTSPOTS_0, NA0, NA0);

	drawcur_y = 32;
	for (drawidx = 0; drawidx < MAX_SAVEGAMES ; drawidx ++) {
		Hotspots_Add (menu_x + 16 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_button); // PPX DUR
		M_Print(16, drawcur_y, m_filenames[drawidx]);
		if (drawidx == load_cursor) drawsel_idx = drawidx;
		drawcur_y += 8;
	}

// line cursor
	M_DrawCharacter (8, 32 + load_cursor * 8, 12+((int)(realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Load_Key (int k, int ascii)
{
	char vabuf[1024];
	switch (k) {
	case K_MOUSE2: // fall
	case K_ESCAPE:	
		M_Menu_SinglePlayer_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else load_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		menu_state_set_nova (m_none);
		key_dest = key_game;

		// issue the load command
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "load s%d", load_cursor) );
		return;

	case K_HOME: 
		load_cursor = 0;
		break;

	case K_END:
		load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


static void M_Save_Key (int k, int ascii)
{
	char vabuf[1024];
	switch (k) {
	case K_MOUSE2: // Fall
	case K_ESCAPE:	
		M_Menu_SinglePlayer_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else load_cursor = hotspotx_hover; // fall thru
	case K_ENTER:
		menu_state_set_nova (m_none);
		key_dest = key_game;
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "save s%d", load_cursor));
		return;

	case K_HOME: 
		load_cursor = 0;
		break;

	case K_END:
		load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

#undef msel_cursoric
#undef m_local_cursor
