// menu_mp.c.h



//=============================================================================
/* MULTIPLAYER MENU */

static int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	3



void M_Menu_MultiPlayer_f (void)
{
	switch (lanConfig_cursor) {
	case_break 1: m_state = m_slist; Net_Slist_f();		M_Menu_ServerList_f(); return;
	case_break 2: m_state = m_slist; Net_SlistQW_f();	M_Menu_ServerList_f(); return;
	} // sw

	key_dest = key_menu;
	menu_state_set_nova (m_multiplayer);
	m_entersound = true;
}


static void M_MultiPlayer_Draw (void)
{
	int		f;
	cachepic_t	*p0;
	char vabuf[1024];

	M_Background(320, 200);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_DrawPic (16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic ( (320-p0->width)/2, 4, p0 /*CPC("gfx/p_multi")*/, NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (72, 32, CPC("gfx/mp_menu"), 3, USE_IMAGE_SIZE_NEG1, USE_IMAGE_SIZE_NEG1);

	f = (int)(realtime * 10)%6;

	M_DrawPic (54, 32 + m_multiplayer_cursor * 20, CPC(va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1)), NO_HOTSPOTS_0, NA0, NA0);
	PPX_DrawSel_End ();
}


static void M_MultiPlayer_Key (int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: // Fall ..
	case K_ESCAPE:	
		M_Menu_Main_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else m_multiplayer_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor) {
		case 0:
		case 1:
			M_Menu_LanConfig_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;
		} // sw
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_HOME:
		m_multiplayer_cursor = 0;
		break;

	case K_END:
		m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;


	} // sw
}
