// menu_saveload.c.h

#define 	local_count		MAX_SAVEGAMES_20
#define		local_cursor	load_cursor
#define 	visiblerows 	MAX_SAVEGAMES_20

//=============================================================================
/* LOAD/SAVE MENU */

static int		load_cursor;		///< 0 < load_cursor < MAX_SAVEGAMES_20

static char		m_filenames[local_count][SAVEGAME_COMMENT_LENGTH_39+1];
static int		loadable[local_count];

static void M_ScanSaves (void)
{
	int		i, j;
	size_t	len;
	char	name[MAX_OSPATH];
	char	buf[SAVEGAME_COMMENT_LENGTH_39 + 256];
	const char *t;
	qfile_t	*f;
//	int		version;

	for (i = 0 ; i < local_count ; i ++) {
		c_strlcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		c_dpsnprintf1 (name, "s%d.sav", (int)i);
		f = FS_OpenRealFile (name, "rb", fs_quiet_FALSE); // Baker: ScanSaves are from real file!!!
		if (!f)
			continue;
		// read enough to get the comment
		len = FS_Read(f, buf, sizeof(buf) - 1);
		len = min(len, sizeof(buf)-1);
		buf[len] = 0;
		t = buf;
		// version
		COM_Parse_Basic (&t);
		//version = atoi(com_token);
		// description
		COM_Parse_Basic (&t);
		c_strlcpy (m_filenames[i], com_token);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH_39 ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		FS_Close (f);
	}
}

void M_Menu_Load_f(cmd_state_t *cmd)
{
	if (sv_save_screenshots.integer) {
		M_Menu_Load2_f (cmd);
		return;
	}

	m_entersound = true;
	menu_state_set_nova (m_load);
	KeyDest_Set (key_menu); // key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f(cmd_state_t *cmd)
{
	if (!sv.active)
		return;

	// LadyHavoc: allow saving multiplayer games
	if (cl.islocalgame && cl.intermission)
		return;

	if (sv_save_screenshots.integer) {
		// Baker: The save game menu remains the same
	}

	m_entersound = true;
	menu_state_set_nova (m_save);
	KeyDest_Set (key_menu); // key_dest = key_menu;
	M_ScanSaves ();
}


static void M_Load_Draw (void)
{
	cachepic_t	*p0;

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_Background(320, 200, q_darken_true);

	p0 = Draw_CachePic ("gfx/p_load");
	M_DrawPic ( (320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_load", NO_HOTSPOTS_0, NA0, NA0);

	drawcur_y = 32;
	for (drawidx = 0 ; drawidx< local_count; drawidx++) {
		Hotspots_Add (menu_x + 16 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0); // PPX DUR
		M_Print(16, drawcur_y, m_filenames[drawidx]);
		if (drawidx == local_cursor) 
			drawsel_idx = drawidx;
		drawcur_y += 8;
	}

// line cursor
	M_DrawCharacter (8, 32 + local_cursor * 8, 12+((int)(host.realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Save_Draw (void)
{
	cachepic_t	*p0;

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_Background(320, 200, q_darken_true);

	p0 = Draw_CachePic ("gfx/p_save");
	M_DrawPic ( (320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_save", NO_HOTSPOTS_0, NA0, NA0);

	drawcur_y = 32;
	for (drawidx = 0; drawidx < local_count; drawidx++) {
		Hotspots_Add (menu_x + 16 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_button, q_force_scale_0); // PPX DUR
		M_Print(16, drawcur_y, m_filenames[drawidx]);
		if (drawidx == local_cursor) drawsel_idx = drawidx;
		drawcur_y += 8;
	}

// line cursor
	M_DrawCharacter (8, 32 + local_cursor*8, 12+((int)(host.realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Load_Key(cmd_state_t *cmd, int key, int ascii)
{
	char vabuf[1024];
	switch (key) {
	case K_MOUSE2: 
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		} 
		// Fall thru
	case K_ESCAPE:
		M_Menu_SinglePlayer_f(cmd);
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit() ) 
			return;
		local_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (!loadable[local_cursor])
			break;
		menu_state_set_nova (m_none);
		KeyDest_Set (key_game); // key_dest = key_game;

		// issue the load command
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "load s%d", local_cursor) );
		break;

	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) // PGUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) // K_MWHEELUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) // PGDN does not wrap, stops at end
			local_cursor = local_count - 1;
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count) 
		local_cursor = local_count - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0) // K_UPARROW wraps around to end
			local_cursor = local_count - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start
			local_cursor = 0;
		break;

leftus:
	case K_LEFTARROW:	
		break;

	case K_RIGHTARROW:	
		break;
	} // sw
}


static void M_Save_Key(cmd_state_t *cmd, int key, int ascii)
{
	char vabuf[1024];
	switch (key) {
	case K_MOUSE2:
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		} 
		// Fall thru
	case K_ESCAPE:
		M_Menu_SinglePlayer_f(cmd);
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit() ) 
			return;
		local_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:
		menu_state_set_nova (m_none);
		KeyDest_Set (key_game); // key_dest = key_game;
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "save s%d", local_cursor));
		break;

	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) // PGUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) // K_MWHEELUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count)  // PGDN does not wrap, stops at end
			local_cursor = local_count - 1;
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count) // K_MWHEELDOWN does not wrap, stops at end
			local_cursor = local_count - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0) // K_UPARROW wraps around to end
			local_cursor = local_count - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start
			local_cursor = 0;
		break;

leftus:
	case K_LEFTARROW:	
		break;

	case K_RIGHTARROW:	
		break;
	} // sw
}

#undef local_count
#undef local_cursor
#undef visiblerows
