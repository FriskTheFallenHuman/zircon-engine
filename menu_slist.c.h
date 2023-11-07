// menu_slist.c.h

#define		msel_cursoric	drawsel_idx
#define		m_local_cursor		slist_cursor

static int slist_cursor;

typedef struct {
	int list_top;
	int list_cursor;
	int list_vis_rows;
} m_sx_s;

m_sx_s mxy;
#define mx1 mxy
void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_slist);
	m_entersound = true;

	M_Update_Return_Reason("");
	if (lanConfig_cursor == 2)
		Net_SlistQW_f();
	else
		Net_Slist_f();
}




int visiblerows00;
#define visiblerows visiblerows00

static void M_ServerList_Draw (void)
{
	int n, y, startrow, endrow, statnumplayers, statmaxplayers;
	cachepic_t *p0;
	const char *s;
	char vabuf[1024];

	//float redx = 0.5 + 0.2 * sin(realtime * M_PI);

	M_Background(640, vid_conheight.integer);
	// scroll the list as the cursor moves
	ServerList_GetPlayerStatistics(&statnumplayers, &statmaxplayers);
	s = va(vabuf, sizeof(vabuf), "%i/%i masters %i/%i servers %i/%i players", masterreplycount, masterquerycount, serverreplycount, serverquerycount, statnumplayers, statmaxplayers);
	M_PrintBronzey((640 - strlen(s) * 8) / 2, 32, s);
	if (*m_return_reason)
		M_Print(16, menu_height - 8, m_return_reason);

	//^7%-21.21s %-19.19s ^%c%-17.17s^7 %-20.20s
	M_PrintBronzey(0, 48, "  Ping Players  Name                     Game       Map");
	y = 60; //48; //10 * 6;
	visiblerows = (int)((menu_height - (/*2*/ 1 * 8) - y) / 8 / 1 /*2*/);
	
	startrow = bound(0, slist_cursor - (visiblerows / 1 /*2*/), serverlist_viewcount - visiblerows);
	endrow = Smallest(startrow + visiblerows, serverlist_viewcount);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic((640 - p0->width) / 2, 4, p0 /*"gfx/p_multi"*/, NO_HOTSPOTS_0, NA0, NA0);
	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (in_range_beyond (0, n, serverlist_viewcount) ) {
				if (n == slist_cursor) msel_cursoric = (n - startrow) /*relative*/;
				WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache)
				serverlist_entry_t *entry = ServerList_GetViewEntry(n);
				//DrawQ_Fill(menu_x, menu_y + y, 640, 16, n == mx1.list_cursor ? redx  : 0, 0, 0, 0.5, 0);
				Hotspots_Add2 (menu_x + 0, menu_y + y, 62*8, (8 * 1) + 1, 1, hotspottype_button, /*idx*/ n); // PPX DUR
#if 0
				M_PrintColored(0, y, entry->line1);y += 8;
				M_PrintColored(0, y, entry->line2);y += 8;
#else 
				WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache)
				M_PrintColored(0, y, entry->line1);y += 8;
				//M_PrintColored(0, y, entry->line2);y += 8;
#endif
			} // if
		} // for
	} // if
	else if (realtime - masterquerytime > 10)
	{
		if (masterquerycount)
			M_Print(0, y, "No servers found");
		else
			M_Print(0, y, "No master servers found (network problem?)");
	}
	else
	{
		if (serverquerycount)
			M_Print(0, y, "Querying servers");
		else
			M_Print(0, y, "Querying master servers");
	}

	PPX_DrawSel_End ();
}

static void M_ServerList_Key(int k, int ascii)
{
	char vabuf[1024];
	int pushed_idx = -1;
	switch (k) {
	case K_MOUSE2: // fall thru
	case K_ESCAPE:
		lanConfig_cursor = -1;
		M_Menu_LanConfig_f();
		
		break;

	case K_MOUSE1: 
		if (hotspotx_hover == not_found_neg1) 
			break; 
		pushed_idx = slist_cursor;
		slist_cursor = Hotspots_GetIdx (hotspotx_hover); // fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (serverlist_viewcount) {
			Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "connect \"%s\"", ServerList_GetViewEntry(slist_cursor)->info.cname));
		}

		if (pushed_idx >= 0) {
			slist_cursor = pushed_idx; // Stop DP from moving server list view for 1 frame
		}

		break;

	case K_SPACE: // This is "refresh list"

		
		if (lanConfig_cursor == 2)
			Net_SlistQW_f();
		else
			Net_Slist_f();
		break;

	case K_HOME:
		if (serverlist_viewcount)
			slist_cursor = 0;
		break;

	case K_END:
		if (serverlist_viewcount)
			slist_cursor = serverlist_viewcount - 1;
		break;

	case K_PGUP:
		slist_cursor -= visiblerows / 2;
		if (slist_cursor < 0) slist_cursor = 0;
		break;

	case K_MWHEELUP:
		slist_cursor -= visiblerows / 4;
		if (slist_cursor < 0) slist_cursor = 0;
		break;

	case K_PGDN:
		slist_cursor += visiblerows / 2;
		if (slist_cursor > serverlist_viewcount - 1) slist_cursor = serverlist_viewcount - 1;
		break;

	case K_MWHEELDOWN:
		slist_cursor += visiblerows / 4;
		if (slist_cursor > serverlist_viewcount - 1) slist_cursor = serverlist_viewcount - 1;
		break;


	case K_UPARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		slist_cursor --;
		if (slist_cursor < 0)
			slist_cursor = 0;//serverlist_viewcount - 1;
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		slist_cursor ++;
		if (slist_cursor >= serverlist_viewcount)
			slist_cursor = serverlist_viewcount - 1;
		break;

	case K_LEFTARROW:
		break;
	
	case K_RIGHTARROW:
		break;

	default:
		break;
	} // sw

}

#undef mx1

#undef msel_cursoric
#undef m_local_cursor
#undef visiblerows