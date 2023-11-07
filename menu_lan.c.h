// menu_lan.c.h


//=============================================================================
/* LAN CONFIG MENU */

//static int		lanConfig_cursor = -1; // moved
static int		lanConfig_cursor_table [] = {56, 76, 84, 120};
#define NUM_LANCONFIG_CMDS	4

static int 	lanConfig_port;
static char	lanConfig_portname[6];
static char	lanConfig_joinname[40];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_lanconfig);
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame)
			lanConfig_cursor = 1;
	}
	if (StartingGame)
		lanConfig_cursor = 1;
	lanConfig_port = 26000;
	dpsnprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", (unsigned int) lanConfig_port);

	M_Update_Return_Reason("");
}


static void M_LanConfig_Draw (void)
{
	cachepic_t	*p0;
	int		basex;
	const char	*startJoin;
	const char	*protocol;
	char vabuf[1024];

	M_Background(320, 200);

	M_DrawPic (16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	basex = (320-p0->width)/2;
	M_DrawPic (basex, 4, p0 /*CPC("gfx/p_multi")*/, NO_HOTSPOTS_0, NA0, NA0);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	protocol = "TCP/IP";
	M_Print(basex, 32, va(vabuf, sizeof(vabuf), "%s - %s", startJoin, protocol));
	basex += 8;

	M_Print(basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, sizeof(lanConfig_portname), 1);
	M_Print(basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);
	Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[0], (45 * 8) /*360*/, 8, 1, hotspottype_button);

	if (JoiningGame)
	{
		M_Print(basex, lanConfig_cursor_table[1], "Search for DarkPlaces games...");
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[1], (45 * 8) /*360*/, 8, 1, hotspottype_button);

		M_Print(basex, lanConfig_cursor_table[2], "Search for QuakeWorld games...");
		Hotspots_Add (menu_x + basex - 8 , menu_y + lanConfig_cursor_table[2], (45 * 8) /*360*/, 8, 1, hotspottype_button);

		M_Print(basex, lanConfig_cursor_table[3]-16, "Join game at:");
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[3], (45 * 8) /*360*/, 8, 1, hotspottype_button);
		M_DrawTextBox (basex+8, lanConfig_cursor_table[3]-8, sizeof(lanConfig_joinname), 1);
		M_Print(basex+16, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[1], (45 * 8) /*360*/, 8, 1, hotspottype_button);
		M_Print(basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_Print(basex, 168, m_return_reason);

	PPX_DrawSel_End ();
}


static void M_LanConfig_Key (int key, int ascii)
{
	int		l;
	char vabuf[1024];

	switch (key)
	{
	case K_MOUSE2: // fall
	case K_ESCAPE:	
		lanConfig_cursor = 0;
		M_Menu_MultiPlayer_f ();
		break;

	case K_HOME:
		lanConfig_cursor = 0;
		break;

	case K_END:
		lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
		// when in start game menu, skip the unused search qw servers item
		if (StartingGame && lanConfig_cursor == 2)
			lanConfig_cursor = 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		// when in start game menu, skip the unused search qw servers item
		if (StartingGame && lanConfig_cursor == 1)
			lanConfig_cursor = 2;
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else lanConfig_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		Cbuf_AddTextLine ("stopdemo");

		Cvar_SetValue("port", lanConfig_port);

		if (lanConfig_cursor == 1 || lanConfig_cursor == 2)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_ServerList_f();
			break;
		}

		if (lanConfig_cursor == 3)
			Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "connect \"%s\"", lanConfig_joinname) );
		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (ascii < 32)
			break;

		if (lanConfig_cursor == 3)
		{
			l = (int)strlen(lanConfig_joinname);
			if (l < (int)sizeof(lanConfig_joinname) - 1)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = ascii;
			}
		}

		if (ascii < '0' || ascii > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = (int)strlen(lanConfig_portname);
			if (l < (int)sizeof(lanConfig_portname) - 1)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = ascii;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 3)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  atoi(lanConfig_portname);
	if (l <= 65535)
		lanConfig_port = l;
	dpsnprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", (unsigned int) lanConfig_port);
}
