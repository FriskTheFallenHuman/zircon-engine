// menu_modlist.c.h

#define		msel_cursoric	m_optcursor		// frame cursor
#define		m_local_cursor		modlist_cursor



static void ModList_RebuildList(void)
{
	int i,j;
	stringlist_t list;
	const char *description;

	stringlistinit(&list);
	listdirectory(&list, fs_basedir, "");
	stringlistsort(&list, true);
	modlist_count = 0;
	modlist_numenabled = fs_numgamedirs;
	for (i = 0;i < list.numstrings && modlist_count < MODLIST_TOTALSIZE;i++)
	{
		// quickly skip names with dot characters - generally these are files, not directories
		if (strchr(list.strings[i], '.')) continue;

		// reject any dirs that are part of the base game
		if (gamedirname1 && String_Does_Match_Caseless(gamedirname1, list.strings[i])) continue;
		//if (gamedirname2 && String_Does_Match_Caseless(gamedirname2, list.strings[i])) continue;

		// check if we can get a description of the gamedir (from modinfo.txt),
		// or if the directory is valid but has no description (fs_checkgamedir_missing)
		// otherwise this isn't a valid gamedir
		description = FS_CheckGameDir(list.strings[i]);
		if (description == NULL || description == fs_checkgamedir_missing) continue;

		strlcpy (modlist[modlist_count].dir, list.strings[i], sizeof(modlist[modlist_count].dir));
		//check currently loaded mods
		modlist[modlist_count].loaded = false;
		if (fs_numgamedirs)
			for (j = 0; j < fs_numgamedirs; j++)
				if (String_Does_Match_Caseless(fs_gamedirs[j], modlist[modlist_count].dir))
				{
					modlist[modlist_count].loaded = true;
					modlist[modlist_count].enabled = j;
					modlist_enabled[j] = modlist_count;
					break;
				}
		modlist_count ++;
	}
	stringlistfreecontents(&list);
}






static void ModList_Enable (void)
{
	int i;
	int numgamedirs;
	char gamedirs[MODLIST_MAXDIRS][MAX_QPATH];

	// copy our mod list into an array for FS_ChangeGameDirs
	numgamedirs = modlist_numenabled;
	for (i = 0; i < modlist_numenabled; i++)
		strlcpy (gamedirs[i], modlist[modlist_enabled[i]].dir,sizeof (gamedirs[i]));

	// this code snippet is from FS_ChangeGameDirs
	if (fs_numgamedirs == numgamedirs)
	{
		for (i = 0;i < numgamedirs;i++)
			if (strcasecmp(fs_gamedirs[i], gamedirs[i]))
				break;
		if (i == numgamedirs)
			return; // already using this set of gamedirs, do nothing
	}

#if 0
	// this part is basically the same as the FS_GameDir_f function
	if ((cls.state == ca_connected && !cls.demoplayback) || sv.active)
	{
		// actually, changing during game would work fine, but would be stupid
		Con_Printf("Can not change gamedir while client is connected or server is running!\n");
		return;
	}
#else 
	if ((cls.state == ca_connected && !cls.demoplayback) || sv.active)
	{
		// actually, changing during game would work fine, but would be stupid
		// Con_Printf("Can not change gamedir while client is connected or server is running!\n");
		//return;
		Con_Printf("Disconnecting...\n");
	}

	// halt demo playback to close the file
	
	CL_Disconnect();
	if (sv.active)
		Host_ShutdownServer ();

	{
	extern int is_game_switch;
	is_game_switch = true; // This does what?  Clear models thoroughly.  As opposed to video restart which shouldn't?
	FS_ChangeGameDirs(numgamedirs, gamedirs, true, true);
	is_game_switch = false;
	}
#endif

	FS_ChangeGameDirs (modlist_numenabled, gamedirs, true, true);
}

void M_Menu_ModList_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_modlist);
	m_entersound = true;
	modlist_cursor = 0;
	M_Update_Return_Reason("");
	ModList_RebuildList();
}

static void M_Menu_ModList_AdjustSliders (void)
{
	int i;
	S_LocalSound ("sound/misc/menu3.wav");

	// stop adding mods, we reach the limit
	if (!modlist[modlist_cursor].loaded && (modlist_numenabled == MODLIST_MAXDIRS)) return;
	modlist[modlist_cursor].loaded = !modlist[modlist_cursor].loaded;
	if (modlist[modlist_cursor].loaded)
	{
		modlist[modlist_cursor].enabled = modlist_numenabled;
		//push the value on the enabled list
		modlist_enabled[modlist_numenabled++] = modlist_cursor;
	}
	else
	{
		//eliminate the value from the enabled list
		for (i = modlist[modlist_cursor].enabled; i < modlist_numenabled; i++)
		{
			modlist_enabled[i] = modlist_enabled[i+1];
			modlist[modlist_enabled[i]].enabled--;
		}
		modlist_numenabled--;
	}
}

int visiblerows12;
#define visiblerows visiblerows12

static void M_ModList_Draw (void)
{
	int n, y, visiblerows, start, end;
	cachepic_t *p0;
	const char *s_available = "Available Mods";
	const char *s_enabled = "Enabled Mods";

	// use as much vertical space as available
	M_Background(640, vid_conheight.integer);
	

	M_PrintRed(48 + 32, 32, s_available);
	M_PrintRed(432, 32, s_enabled);
	// Draw a list box with all enabled mods
	DrawQ_Pic(menu_x + 432, menu_y + 48, NULL, 172, 8 * modlist_numenabled, 0, 0, 0, 0.5, 0);
	for (y = 0; y < modlist_numenabled; y++)
		M_PrintRed(432, 48 + y * 8, modlist[modlist_enabled[y]].dir);

	if (*m_return_reason)
		M_Print(16, menu_height - 8, m_return_reason);
	// scroll the list as the cursor moves
	y = 48;
	visiblerows = (int)((menu_height - 16 - y) / 8 / 2);
	start = bound(0, modlist_cursor - (visiblerows >> 1), modlist_count - visiblerows);
	end = min(start + visiblerows, modlist_count);

	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((640 - p0->width) / 2, 4, p0 /*"gfx/p_option"*/, NO_HOTSPOTS_0, NA0, NA0);
	float redx = 0.5 + 0.2 * sin(realtime * M_PI); 
	if (end > start) {
		for (n = start;n < end;n++) {
			DrawQ_Pic(menu_x + 40, menu_y + y, NULL, 360, 8, n == modlist_cursor ? redx : 0, 0, 0, 0.5, 0);
			Hotspots_Add (menu_x + 40, menu_y + y, 360 /*(55 * 8)*/ /*360*/, 8, 1, hotspottype_slider);
			M_ItemPrint(80, y, modlist[n].dir, true);
			M_DrawCheckbox(48, y, modlist[n].loaded);
			y +=8;
		}
	} // if
	else
	{
		M_Print(80, y, "No Mods found");
	}
	PPX_DrawSel_End ();
}

static void M_ModList_Key(int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { modlist_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:	
		ModList_Enable ();
		M_Menu_Options_Classic_f ();
		break;

	//	S_LocalSound ("sound/misc/menu2.wav");
	//	ModList_Enable ();
	//	break;

	case K_SPACE:
		S_LocalSound ("sound/misc/menu2.wav");
		ModList_RebuildList();
		break;

	case K_HOME: 
		modlist_cursor = 0;
		break;

	case K_END:
		modlist_cursor = modlist_count - 1;
		break;

	case K_PGUP:
		modlist_cursor -= visiblerows / 2;
		if (modlist_cursor < 0) modlist_cursor = 0;
		break;

	case K_MWHEELUP:
		modlist_cursor -= visiblerows / 4;
		if (modlist_cursor < 0) modlist_cursor = 0;
		break;

	case K_PGDN:
		modlist_cursor += visiblerows / 2;
		if (modlist_cursor > modlist_count - 1) modlist_cursor = modlist_count - 1;
		break;

	case K_MWHEELDOWN:
		modlist_cursor += visiblerows / 4;
		if (modlist_cursor > modlist_count - 1) modlist_cursor = modlist_count - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		modlist_cursor--;
		if (modlist_cursor < 0)
			modlist_cursor = modlist_count - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		modlist_cursor++;
		if (modlist_cursor >= modlist_count)
			modlist_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		M_Menu_ModList_AdjustSliders ();
		break;



	case K_MOUSE1: if (!Hotspots_DidHit () ) { return;	}  modlist_cursor = hotspotx_hover; // PPX Key2 fall thru
	case K_ENTER:

	case K_RIGHTARROW:
		M_Menu_ModList_AdjustSliders ();
		break;

	default:
		break;
	}

}

#undef msel_cursoric
#undef m_local_cursor
#undef visiblerows