// menu_lan.c.h

// Baker:

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
static int		lanConfig_cursor_table [] = {56 + 16, 76+ 16, 84+ 16, 120+ 16};
#define NUM_LANCONFIG_CMDS_4	4

static int 	lanConfig_port;
static char	lanConfig_portname[6];
static char	lanConfig_joinname[40];

void M_Menu_LanConfig_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_lanconfig);
	m_entersound = true;
	if (lanConfig_cursor == -1) {
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
	int		effective_count = JoiningGame ? (NUM_LANCONFIG_CMDS_4 - 1) : 2;

	if (lanConfig_cursor >= effective_count)
		lanConfig_cursor = effective_count;

	M_Background(320, 200, q_darken_true);

	M_DrawPic (16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	basex = (320-Draw_GetPicWidth(p0))/2;
	M_DrawPic (basex, 4, "gfx/p_multi", NO_HOTSPOTS_0, NA0, NA0);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	protocol = "TCP/IP";

	drawcur_y = 32;

	M_Print(basex, drawcur_y, va(vabuf, sizeof(vabuf), "%s - %s", startJoin, protocol));
	basex += 8;
	drawcur_y += 16;

	M_PrintBronzey(basex, drawcur_y, "Address");
	M_PrintBronzey(basex+9*8, drawcur_y, my_ipv4_address);
	M_PrintBronzey(basex+9*8, drawcur_y + 8, my_ipv6_address);
	drawcur_y += 16;

	M_Print(basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, sizeof(lanConfig_portname), 1);
	M_Print(basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);
	Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[0], (45 * 8) /*360*/, 8, 1, hotspottype_button, q_force_scale_0);

	if (JoiningGame) {
		M_Print(basex, lanConfig_cursor_table[1], "Search for DarkPlaces games...");
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[1], (45 * 8) /*360*/, 8, 1, hotspottype_button, q_force_scale_0);

		M_Print(basex, lanConfig_cursor_table[2], "Search for QuakeWorld games...");
		Hotspots_Add (menu_x + basex - 8 , menu_y + lanConfig_cursor_table[2], (45 * 8) /*360*/, 8, 1, hotspottype_button, q_force_scale_0);

		M_Print(basex, lanConfig_cursor_table[3]-16, "Join game at:");
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[3], (45 * 8) /*360*/, 8, 1, hotspottype_button, q_force_scale_0);
		M_DrawTextBox (basex+8, lanConfig_cursor_table[3]-8, sizeof(lanConfig_joinname), 1);
		M_Print(basex+16, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		Hotspots_Add (menu_x + basex - 8, menu_y + lanConfig_cursor_table[1], (45 * 8) /*360*/, 8, 1, hotspottype_button, q_force_scale_0);
		M_Print(basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(host.realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(host.realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(host.realtime*4)&1));

	if (*m_return_reason)
		M_Print(basex, 168, m_return_reason);

	PPX_DrawSel_End ();
}


static void M_LanConfig_Key(cmd_state_t *cmd, int key, int ascii)
{
	int		jj;
	char vabuf[1024];
	int		effective_count = JoiningGame ? NUM_LANCONFIG_CMDS_4 : 2;

	switch (key) {
	case K_MOUSE2: // fall
	case K_ESCAPE:
		lanConfig_cursor = 0;
		M_Menu_MultiPlayer_f (cmd);
		break;

	case K_HOME:
		lanConfig_cursor = 0;
		break;

	case K_END:
		lanConfig_cursor = effective_count - 1; // NUM_LANCONFIG_CMDS_4 - 1;
		break;

	// If there are no visiblerows limitations
	// in the menu, page and wheel do nothing.
	// case K_PGUP:
	// case K_MWHEELUP:
	// case K_PGDN:
	// case K_MWHEELDOWN:

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0) // K_UPARROW wrap
			lanConfig_cursor = effective_count - 1; // Wrap
		// when in start game menu, skip the unused search qw servers item
		//if (StartingGame && lanConfig_cursor == 2)
		//	lanConfig_cursor = 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= effective_count) // K_DOWNARROW wraps
			lanConfig_cursor = 0; // Wrap
		// when in start game menu, skip the unused search qw servers item
		//if (StartingGame && lanConfig_cursor == 1)
		//	lanConfig_cursor = 2;
		break;

	case K_MOUSE1: 
		if (hotspotx_hover == not_found_neg1) 
			break; 
		
		lanConfig_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		Cbuf_AddTextLine (cmd, "stopdemo");

		Cvar_SetValue(&cvars_all, "port", lanConfig_port);

		if (lanConfig_cursor == 1 || lanConfig_cursor == 2) {
			if (StartingGame) {
				M_Menu_GameOptions_f(cmd);
				break;
			}
			M_Menu_ServerList_f(cmd);
			break;
		}

		if (lanConfig_cursor == 3)
			Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "connect " QUOTED_S, lanConfig_joinname) );
		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0) {
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
			jj = (int)strlen(lanConfig_joinname);
			if (jj < (int)sizeof(lanConfig_joinname) - 1)
			{
				lanConfig_joinname[jj+1] = 0;
				lanConfig_joinname[jj] = ascii;
			}
		}

		if (ascii < '0' || ascii > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			jj = (int)strlen(lanConfig_portname);
			if (jj < (int)sizeof(lanConfig_portname) - 1)
			{
				lanConfig_portname[jj+1] = 0;
				lanConfig_portname[jj] = ascii;
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

	jj =  atoi(lanConfig_portname);
	if (jj <= 65535)
		lanConfig_port = jj;
	dpsnprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", (unsigned int) lanConfig_port);
}
