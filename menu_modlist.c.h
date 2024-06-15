// menu_modlist.c.h

#define		local_cursor	modlist_cursor
#define		local_count		modlist_count
#define		visiblerows		m_modlist_visiblerows	

int m_modlist_visiblerows;

//=============================================================================
/* MODLIST MENU */
// same limit of mod dirs as in fs.c
#define MODLIST_MAXDIRS 16
static int modlist_enabled [MODLIST_MAXDIRS];	//array of indexs to modlist
static int modlist_numenabled;			//number of enabled (or in process to be..) mods

typedef struct modlist_entry_s
{
	qbool loaded;	// used to determine whether this entry is loaded and running
	int enabled;		// index to array of modlist_enabled

	// name of the modification, this is (will...be) displayed on the menu entry
	char name[128];
	// directory where we will find it
	char dir[MAX_QPATH_128];
} modlist_entry_t;

static int modlist_cursor;
//static int modlist_viewcount;

static int modlist_count = 0;
static modlist_entry_t modlist[MODLIST_MAX_ENTRIES_256];

static void ModList_RebuildList(void)
{
	int i,j;
	stringlist_t list;
	const char *description;

	stringlistinit(&list);
	listdirectory(&list, fs_basedir, fs_all_files_empty_string);
	stringlistsort(&list, true);
	modlist_count = 0;
	modlist_numenabled = fs_numgamedirs;
	for (i = 0;i < list.numstrings && modlist_count < MODLIST_MAX_ENTRIES_256; i++) {
		// quickly skip names with dot characters - generally these are files, not directories
		if (strchr(list.strings[i], '.')) continue;

		// reject any dirs that are part of the base game
		if (gamedirname1 && String_Match_Caseless(gamedirname1, list.strings[i])) continue;
		//if (gamedirname2 && String_Match_Caseless(gamedirname2, list.strings[i])) continue;

		// check if we can get a description of the gamedir (from modinfo.txt),
		// or if the directory is valid but has no description (fs_checkgamedir_missing)
		// otherwise this isn't a valid gamedir
		description = FS_CheckGameDir(list.strings[i]);
		if (description == NULL || description == fs_checkgamedir_missing) continue;

		c_strlcpy (modlist[modlist_count].dir, list.strings[i]);
		//check currently loaded mods
		modlist[modlist_count].loaded = false;
		if (fs_numgamedirs)
			for (j = 0; j < fs_numgamedirs; j++)
				if (String_Match_Caseless(fs_gamedirs[j], modlist[modlist_count].dir))
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
	char gamedirs[MODLIST_MAXDIRS][MAX_QPATH_128];

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

	if ((cls.state == ca_connected && !cls.demoplayback) || sv.active)
	{
		// actually, changing during game would work fine, but would be stupid
		Con_PrintLinef ("Can not change gamedir while client is connected or server is running!");
		return;
	}

	FS_ChangeGameDirs (modlist_numenabled, gamedirs, q_tx_complain_true, q_fail_on_missing_true);
}

void M_Menu_ModList_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_modlist);
	m_entersound = true;
	local_cursor = 0;
	M_Update_Return_Reason("");
	ModList_RebuildList();
}

static void M_Menu_ModList_AdjustSliders (int dir)
{
	int i;
	S_LocalSound ("sound/misc/menu3.wav");

	// stop adding mods, we reach the limit
	if (!modlist[local_cursor].loaded && (modlist_numenabled == MODLIST_MAXDIRS)) return;
	modlist[local_cursor].loaded = !modlist[local_cursor].loaded;
	if (modlist[local_cursor].loaded)
	{
		modlist[local_cursor].enabled = modlist_numenabled;
		//push the value on the enabled list
		modlist_enabled[modlist_numenabled++] = local_cursor;
	}
	else
	{
		//eliminate the value from the enabled list
		for (i = modlist[local_cursor].enabled; i < modlist_numenabled; i++)
		{
			modlist_enabled[i] = modlist_enabled[i+1];
			modlist[modlist_enabled[i]].enabled--;
		}
		modlist_numenabled--;
	}
}

static void M_ModList_Draw (void)
{
	int n, startrow, endrow;
	cachepic_t *p0;
	const char *s_available = "Available Mods";
	const char *s_enabled = "Enabled Mods";

	// use as much vertical space as available
	M_Background(640, vid_conheight.integer, q_darken_true);

	M_PrintRed(48 + 32, 32, s_available);
	M_PrintRed(432, 32, s_enabled);
	// Draw a list box with all enabled mods
	DrawQ_Pic(menu_x + 432, menu_y + 48, NULL, 172, 8 * modlist_numenabled, 0, 0, 0, 0.5, 0);
	for (int idx = 0; idx < modlist_numenabled; idx ++)
		M_PrintRed(432, 48 + idx * 8, modlist[modlist_enabled[idx]].dir);

	if (*m_return_reason)
		M_Print(16, menu_height - 8, m_return_reason);
	// scroll the list as the cursor moves
	drawcur_y = 48;
	visiblerows = (int)((menu_height - 16 - drawcur_y) / 8 / 2);

	// Baker: Do it this way because a short list may have more visible rows than the list count
	// so using bound doesn't work. 
	startrow = local_cursor - (visiblerows / 2);
	if (startrow > local_count - visiblerows)
		startrow = local_count - visiblerows;
	if (startrow < 0)
		startrow = 0; // visiblerows can exceed local_count

	endrow = Smallest(startrow + visiblerows, local_count);

	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((640 - Draw_GetPicWidth(p0)) / 2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);
	float redx = 0.5 + 0.2 * sin(host.realtime * M_PI); 
	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			DrawQ_Pic(menu_x + 40, menu_y + drawcur_y, NULL, 360, 8, n == local_cursor ? redx : 0, 0, 0, 0.5, 0);
			Hotspots_Add (menu_x + 40, menu_y + drawcur_y, 360 /*(55 * 8)*/ /*360*/, 8, 1, hotspottype_slider, q_force_scale_0);
			M_ItemPrint(80, drawcur_y, modlist[n].dir, true);
			M_DrawCheckbox(48, drawcur_y, modlist[n].loaded);
			drawcur_y +=8;
		}
	} // if
	else
	{
		M_Print(80, drawcur_y, "No Mods found");
	}
	PPX_DrawSel_End ();
}

static void M_ModList_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		ModList_Enable ();
		M_Menu_Options_Classic_f(cmd);
		break;

	case K_MOUSE1: if (!Hotspots_DidHit () ) { return;	}  local_cursor = hotspotx_hover; // PPX Key2 fall thru
	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		ModList_Enable ();
		break;

	case K_SPACE:
		S_LocalSound ("sound/misc/menu2.wav");
		ModList_RebuildList();
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

	case K_LEFTARROW:
leftus:
		M_Menu_ModList_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_ModList_AdjustSliders (1);
		break;

	default:
		break;
	}

}

#undef local_count
#undef local_cursor
#undef visiblerows
