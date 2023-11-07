// menu_si.c.h


//=============================================================================
/* SINGLE PLAYER MENU */

static int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_singleplayer);
	m_entersound = true;
}


static void M_SinglePlayer_Draw (void)
{
	cachepic_t	*p;
	char vabuf[1024];

	M_Background(320, 200);

	M_DrawPic (16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p = Draw_CachePic ("gfx/ttl_sgl");

	
	int		f = (int)(realtime * 10)%6;

	M_DrawPic ( (320-p->width)/2, 4, p /*CPC("gfx/ttl_sgl")*/, NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (72, 32, CPC("gfx/sp_menu"), 3, USE_IMAGE_SIZE_NEG1, USE_IMAGE_SIZE_NEG1);

	M_DrawPic (54, 32 + m_singleplayer_cursor * 20, CPC(va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1)), NO_HOTSPOTS_0, NA0, NA0);
}


static void M_SinglePlayer_Key (int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: // fall
	case K_ESCAPE:	
		M_Menu_Main_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else m_singleplayer_cursor = hotspotx_hover; // fall thru
	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor) {
		case 0:
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddTextLine ("disconnect");
			Cbuf_AddTextLine ("maxplayers 1");
			Cbuf_AddTextLine ("deathmatch 0");
			Cbuf_AddTextLine ("coop 0");
			Cbuf_AddTextLine ("startmap_sp");
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			if (!sv.active || (cl.islocalgame && cl.intermission) ) {
				M_Menu_NoSave_f ();
				
			} else {
				M_Menu_Save_f ();
			}

			
			break;
		} // sw

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_HOME: 
		m_singleplayer_cursor = 0;
		break;

	case K_END:
		m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;
	} // sw
}
