// menu_maps.c.h

// int m_maplist_count
//maplist_s m_maplist[512];
int mapscursor;
int startrow, endrow;

int visiblerows01;
#define visiblerows visiblerows01


// Assume that almost nothing is "safe".
// vis rows can change
// maps list can change




void M_Menu_Maps_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_maps);
	m_entersound = true;
	GetMapList("", NULL, 0, /*is_menu_fill*/ true, /*autocompl*/ false, /*suppress*/ false);
	startrow = not_found_neg1, endrow = not_found_neg1;
}

static void M_Maps_Key(int k, int ascii)
{
	int lcase_ascii;
	switch (k) {
	case K_MOUSE2: // fall
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1) break;

		mapscursor = hotspotx_hover + startrow; // fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (m_maplist_count) {
			char vabuf[1024];
			maplist_s *mx = &m_maplist[mapscursor];
			va (vabuf, sizeof(vabuf), "map %s", mx->sm_a);
			menu_state_reenter = 1;
			Cbuf_AddTextLine (vabuf );

			Con_History_Maybe_Push (vabuf); // Au 15

			key_dest = key_game;
			menu_state_set_nova (m_none);
		}

		break;

	case K_SPACE:
		//ModList_RebuildList();
		break;

	case K_HOME:
		mapscursor = 0;
		break;

	case K_PGUP:
		mapscursor -= visiblerows / 2;
		if (mapscursor < 0) mapscursor = 0;
		break;

	case K_MWHEELUP:
		mapscursor -= visiblerows / 4;
		if (mapscursor < 0) mapscursor = 0;
		break;

	case K_PGDN:
		mapscursor += visiblerows / 2;
		if (mapscursor > m_maplist_count - 1) mapscursor = m_maplist_count - 1;
		break;

	case K_MWHEELDOWN:
		mapscursor += visiblerows / 4;
		if (mapscursor > m_maplist_count - 1) mapscursor = m_maplist_count - 1;
		break;

	case K_END:
		mapscursor = m_maplist_count - 1;
		break;

	case K_UPARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		mapscursor--;
		if (mapscursor < 0)
			mapscursor = 0;
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		mapscursor ++;
		if (mapscursor >= m_maplist_count)
			mapscursor = m_maplist_count - 1 ;
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		lcase_ascii = tolower(ascii);
		if (in_range ('a', lcase_ascii, 'z')) {
			int startx = mapscursor, count;
			char sprefix[2] = { (char)lcase_ascii, 0 };

			//int j;
			for (count = 0; count < m_maplist_count; ) {
				startx ++;
				if (startx >= m_maplist_count) {
					startx = 0;
				}
				maplist_s *mx = &m_maplist[startx];
				if (String_Does_Start_With_Caseless ((char *)mx->smtru_a, sprefix)) {
					mapscursor = startx;
					break;
				}


				count ++;
				if (count >= m_maplist_count)
					break;

			} // j


		}
		break;
	}

}


static void M_Maps_Draw (void)
{
	int n, y; //, visiblerows; //, startrow, endrow;
	cachepic_t *p0;
	const char *s_available = "Maps";

	//float redx = 0.5 + 0.2 * sin(realtime * M_PI);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_Background(640, vid_conheight.integer);

	M_Print (48 + 32, 32, s_available);

	// scroll the list as the cursor moves
	y = 8 * 6;
	visiblerows = (int)((menu_height - (2 * 8) - y) / 8 /*/ 2*/) - 8;
	startrow= mapscursor - (visiblerows /2);
	startrow= bound(0, mapscursor - (visiblerows /2), m_maplist_count - visiblerows);
	if (startrow < 0) {
		startrow = 0; // visiblerows can exceed m_maplist_count
	}
	endrow = min (startrow + visiblerows, m_maplist_count);

	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((640 - p0->width) / 2, 4, p0 /*"gfx/p_option"*/, NO_HOTSPOTS_0, NA0, NA0);
	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (n >= 0 && n < m_maplist_count) {
				if (n == mapscursor) drawsel_idx = (n - startrow) /*relative*/;
				maplist_s *mx = &m_maplist[n];
				//DrawQ_Pic(menu_x + (8 * 9), menu_y + y, NULL, (55 * 8) /*360*/, 8, n == mapscursor ? redx : 0, 0, 0, 0.5, 0);
				Hotspots_Add2 (menu_x + (8 * 9), menu_y + y, (55 * 8) /*360*/, 8, 1, hotspottype_button, n);
				M_ItemPrint ((8 +  0) * 10, y, (const char *)mx->smtru_a, true);
				M_ItemPrint ((8 + 18) * 10, y, (const char *)mx->sqbsp, true);
				M_ItemPrint2 ((8 + 21) * 10, y, (const char *)mx->smsg_a, true);
				y +=8;
			}
		}
	}
	else
	{
		M_Print(80, y, "No Maps found");
	}

	PPX_DrawSel_End ();
}

#undef visiblerows
