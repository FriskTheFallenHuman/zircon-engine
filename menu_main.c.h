// menu_main.c.h

//=============================================================================
/* MAIN MENU */

static int	m_main_cursor;


static int MAIN_ITEMS = 4; // Nehahra: Menu Disable


void M_Menu_Main_f (void)
{
	const char *s;
	s = "gfx/mainmenu";

	if (iszirc /*gamemode == GAME_ZEROX*/) {

		
	}

	if (gamemode == GAME_NEHAHRA) {
		if (FS_FileExists("maps/neh1m4.bsp")) {
			if (FS_FileExists("hearing.dem")) {
				Con_DPrint("Main menu: Nehahra movie and game detected.\n");
				NehGameType = TYPE_BOTH;
			} else {
				Con_DPrint("Nehahra game detected.\n");
				NehGameType = TYPE_GAME;
			}
		} else {
			if (FS_FileExists("hearing.dem")) {
				Con_DPrint("Nehahra movie detected.\n");
				NehGameType = TYPE_DEMO;
			} else {
				Con_DPrint("Nehahra not found.\n");
				NehGameType = TYPE_GAME; // could just complain, but...
			}
		}
		if (NehGameType == TYPE_DEMO)			MAIN_ITEMS = 4;
		else if (NehGameType == TYPE_GAME)		MAIN_ITEMS = 5;
		else									MAIN_ITEMS = 6;
	}
	else MAIN_ITEMS = 5;

	// check if the game data is missing and use a different main menu if so
	m_missingdata = !forceqmenu.integer && Draw_CachePic (s)->tex == r_texture_notexture;
	if (m_missingdata)
		MAIN_ITEMS = 2;

	key_dest = key_menu;
	menu_state_set_nova (m_main);
	m_entersound = true;
}

 
static void M_Main_Draw_Zirc (void);
static void M_Main_Draw (void)
{
	int		f;
	cachepic_t	*p0;
	char vabuf[1024];
	
	if (m_missingdata)
	{
		float y;
		const char *s;
		M_Background(640, 480); //fall back is always to 640x480, this makes it most readable at that.
		y = 480/3-16;
		s = "You have reached this menu due to missing or unlocatable content/data";M_PrintRed ((640-strlen(s)*8)*0.5, (480/3)-16, s);y+=8;
		y+=8;
		s = "You may consider adding";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "-basedir /path/to/game";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "to your launch commandline";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		M_Print (640/2 - 48, 480/2, "Open Console"); //The console usually better shows errors (failures)
		M_Print (640/2 - 48, 480/2 + 8, "Quit");
		M_DrawCharacter(640/2 - 56, 480/2 + (8 * m_main_cursor), 12+((int)(realtime*4)&1));
		return;
	}

	if (gamemode == GAME_QUAKE3_QUAKE1) {

	}

	if (0 && iszirc) {
		M_Main_Draw_Zirc ();
		return;
	}


	M_Background(320, 200);
	M_DrawPic (16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/ttl_main");

	M_DrawPic ( (320-p0->width)/2, 4, p0 /*CPC("gfx/ttl_main")*/, NO_HOTSPOTS_0, NA0, NA0);
// Nehahra
	if (gamemode == GAME_NEHAHRA) {
		if (NehGameType == TYPE_BOTH)			M_DrawPic (72, 32, CPC("gfx/mainmenu"), MAIN_ITEMS, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
		else if (NehGameType == TYPE_GAME)		M_DrawPic (72, 32, CPC("gfx/gamemenu"), MAIN_ITEMS, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
		else									M_DrawPic (72, 32, CPC("gfx/demomenu"), MAIN_ITEMS, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
	}
	else {
		M_DrawPic (72, 32, CPC("gfx/mainmenu"), 5, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
	}

	f = (int)(realtime * 10)%6;

	M_DrawPic (54, 32 + m_main_cursor * 20, CPC(va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1)), NO_HOTSPOTS_0, NA0, NA0);
}



static void M_Main_Key (int key, int ascii)
{
	
	switch (key) {
	case K_ESCAPE:	case K_MOUSE2:
		key_dest = key_game;
		menu_state_set_nova (m_none);
		//cls.demonum = m_save_demonum;
		//if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
		//	CL_NextDemo ();
		break;
	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else m_main_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;

		if (m_missingdata) {
			switch (m_main_cursor) {
			case 0:	if (cls.state == ca_connected) {
						menu_state_set_nova (m_none);
						key_dest = key_game;
					} // if
					Con_ToggleConsole_f ();
					break;

			case 1:	M_Menu_Quit_f ();
					break;
			} // sw
		} // m_missingdata
		else if (gamemode == GAME_NEHAHRA) {
			#include "menu_main_neh.c.h"
		} // GAME_NEHAHRA
		//else if (gamemode == GAME_ZEROX) {

		//}
		else {
			switch (m_main_cursor) {
			case 0:	M_Menu_SinglePlayer_f ();	break;
			case 1: M_Menu_MultiPlayer_f ();	break;
			case 2:	M_Menu_OptionsNova_f ();		break;
			case 3:
				if (1) M_Menu_Maps_f ();
				else M_Menu_Help_f ();
				break;
			case 4: M_Menu_Quit_f (); break;
			}

		}
		break;


	case K_HOME: 
		m_main_cursor = 0;
		break;

	case K_END:
		m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	} // sw
}
