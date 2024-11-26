// menu_slist.c.h

server_player_info_t server_player_infos[QW_MAX_CLIENTS_32];
int server_player_infos_count;
void Commit_To_Cname (void);

WARP_X_ (last_nav_cname Net_SlistQW_f Commit_To_Cname NetConn_ClientParsePacket)

int slist_cursor;

#define SLIST_SORTBY_PING_0		0
#define SLIST_SORTBY_PLAYERS_1	1
#define SLIST_SORTBY_NAME_2		2
#define SLIST_SORTBY_GAME_3		3
#define SLIST_SORTBY_MAP_4		4

int serverlist_viewx_dirty;

WARP_X_ (serverlist_list, serverlist_list_count)

char last_nav_cname[128];

char slist_filter_word[64];
int slist_filter_players_only = 0;
int slist_sort_by = 1;
int slist_sort_ascending = false;

#include "menu_slist_support.c.h"

#define		local_count					serverlist_list_count
#define		local_cursor				slist_cursor
#define 	visiblerows 				m_slist_visiblerows
#define 	startrow 					m_slist_startrow
#define 	endrow 						m_slist_endrow
#define		local_click_time			m_slist_click_time
#define		local_scroll_is_blocked		m_slist_scroll_is_blocked

float	m_slist_click_time = 0;
qbool	m_slist_scroll_is_blocked;

int		m_slist_visiblerows;
int		m_slist_startrow;
int		m_slist_endrow;

WARP_X_ (Net_SlistQW_f, Net_Slist_f ServerList_QueryList)

// Baker: serverlist_viewlist is a subset of serverlist_cache (basically)
WARP_X_ (serverlist_viewlist serverlist_viewlist_count serverlist_cache)
WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache which line 1 is written)

void M_Menu_ServerList_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_slist_27);
	m_entersound = true;
	M_Update_Return_Reason("");

	if (menu_state_reenter == 0) {
		double elapsed = serverlist_list_query_time == 0 ? 9999 : serverlist_list_query_time - Sys_DirtyTime ();
		// 300 seconds.  User can manually refresh it.
		if (elapsed > 300) {
#if 1
		// Server browser -- what to do here? modlist.txt
            if (mod_list_folder_name[0] == 0) {
                Net_Slist_Both_f(cmd);
            }
            else {
                // Total conversion with 1 folder.
                Net_Slist_f(cmd);
            }
		}
#else
		if (lanConfig_cursor == 2)
			Net_SlistQW_f(cmd);
		else
			Net_Slist_f(cmd);
#endif
	}

	startrow = not_found_neg1, endrow = not_found_neg1;

	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;

#if 0 // Server list is reentrant, don't mess with cursor.
//	local_cursor = 0;
#endif

	if (menu_state_reenter)
		menu_state_reenter = 0;
}


static void M_ServerList_Draw (void)
{
	int n, statnumplayers, statmaxplayers;
	cachepic_t *p0;
	const char *s;
	char vabuf[1024];

	if (serverlist_viewx_dirty) {
		// Rebuild list
		M_ServerList_Rebuild ();
		serverlist_viewx_dirty = false;
	}

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Background (640, vid_conheight.integer, q_darken_true);

	ServerList_GetPlayerStatistics(&statnumplayers, &statmaxplayers);
	s = va(vabuf, sizeof(vabuf), "%d/%d masters %d/%d servers %d/%d players", 
		masterreplycount, masterquerycount, 
		serverreplycount, serverquerycount, statnumplayers, statmaxplayers);
	M_PrintBronzey((640 - strlen(s) * 8) / 2, 32, s);
	if (*m_return_reason)
		M_Print(16, menu_height - 8, m_return_reason);

	//^7%-21.21s %-19.19s ^%c%-17.17s^7 %-20.20s
	// scroll the list as the cursor moves

	drawcur_y = 60;

	// Baker subtracting off rows for server ip stuff (1) and for player info (4)
	visiblerows = (int)((menu_height - (/*rows*/ (7 + 1 + 1) * 8) - drawcur_y) / 8);

	// Baker: Do it this way because a short list may have more visible rows than the list count
	// so using bound doesn't work.
	if (local_scroll_is_blocked == false) {
		startrow = local_cursor - (visiblerows / 2);
		if (startrow > local_count - visiblerows)
			startrow = local_count - visiblerows;
		if (startrow < 0)
			startrow = 0; // visiblerows can exceed local_count
	}

	endrow = Smallest(startrow + visiblerows, local_count);

	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic((640 - Draw_GetPicWidth(p0)) / 2, 4, "gfx/p_multi", NO_HOTSPOTS_0, NA0, NA0);

	WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache)

	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (!in_range_beyond (0, n, local_count) )
				continue;

			if (n == local_cursor)
				drawsel_idx = (n - startrow) /*relative*/;

			int idx_nova = serverlist_list[n];
			serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);

			Hotspots_Add2 (menu_x + 0, menu_y + drawcur_y, 62 * 8, (8 * 1) + 1, 1,
				hotspottype_button, /*idx*/ n); // PPX DUR
			M_PrintColored(0, drawcur_y, entry_nova->line1);

			drawcur_y += 8;
		} // for
	} // endrow > startrow
	else if (host.realtime - masterquerytime > 10)
	{
		if (masterquerycount)
			M_Print(0, drawcur_y, "No servers found");
		else
			M_Print(0, drawcur_y, "No master servers found (network problem?)");
	}
	else
	{
		if (serverquerycount)
			M_Print(0, drawcur_y, "Querying servers");
		else
			M_Print(0, drawcur_y, "Querying master servers");
	}

drawcolumnheaders:
	drawcur_y = 48;
	int drawcur_x, draw_cur_w, headidx = 0; // draw_cur_h = 8;
	drawcur_x = 2 * 8;

	#define HOTSPOT_DRAW_ADD \
		Hotspots_Add2 (menu_x + drawcur_x, menu_y + drawcur_y, draw_cur_w, (8 * 1) + 1, /*count*/ 1,  hotspottype_listitem, --headidx); \
		drawcur_x += (draw_cur_w + /*space*/ 1 * 8) // Ender

	float underline_color3[3] = {0.32, 0.32, 0.32}; // White

	// Players sort is reversed (ascending is descending, descending is ascending)
	if (slist_sort_by == SLIST_SORTBY_PLAYERS_1 && slist_sort_ascending) {
		underline_color3[1] = 0;
		underline_color3[2] = 0;
	} else if (slist_sort_by != SLIST_SORTBY_PLAYERS_1 && !slist_sort_ascending) {
		underline_color3[1] = 0;
		underline_color3[2] = 0;
	}

	#define DRAW_UNDERLINE_COLUMN \
		DrawQ_Fill(menu_x + drawcur_x, menu_y + drawcur_y + 9, draw_cur_w, 1 /*draw_cur_h*/, VECTOR3_SEND(underline_color3),  \
			alpha_1_0, DRAWFLAG_NORMAL_0) // Ender

	draw_cur_w = 4 /*chars*/ * 8;

	// DRAW COLUMN HEADERS
	if (slist_sort_by == SLIST_SORTBY_PING_0) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Ping");	HOTSPOT_DRAW_ADD;
		draw_cur_w = 7 /*chars*/ * 8; // column width
	if (slist_sort_by == SLIST_SORTBY_PLAYERS_1) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Players");	HOTSPOT_DRAW_ADD;
		drawcur_x += (/*1 spaces*/ 1 * 8);
	draw_cur_w = 4 /*chars*/ * 8; // column width
	if (slist_sort_by == SLIST_SORTBY_NAME_2) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Name");	HOTSPOT_DRAW_ADD;
		drawcur_x += (/*19 spaces*/ 20 * 8);
	draw_cur_w = 4 /*chars*/ * 8;
	if (slist_sort_by == SLIST_SORTBY_GAME_3) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Game");	HOTSPOT_DRAW_ADD;
		drawcur_x += (/*19 spaces*/ 6 * 8);
	draw_cur_w = 3 /*chars*/ * 8;
	if (slist_sort_by == SLIST_SORTBY_MAP_4) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Map");	HOTSPOT_DRAW_ADD;

	PPX_DrawSel_End (); // Baker: hover highlight drawn

	// Baker: Selected server draw occurs here

draw_selected_server:
	while (local_count) {
		// Baker: Determine if the hover is a column header
		// if so don't use the hover idx for drawing selected server player list
		int is_column_header = false;
		if (hotspotx_hover != not_found_neg1) {
			hotspotx_s *h = &hotspotxs[hotspotx_hover];
			if (h->hotspottype == hotspottype_listitem) {
				// It's a column header - we use this to make sure hover is not used for selected server
				is_column_header = true;
			}
		}

		int idx =
			is_column_header ?					local_cursor :
			hotspotx_hover == not_found_neg1 ?	local_cursor :
													hotspotx_hover + startrow;

		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);

draw_players_on_selected_server:
		drawcur_y = menu_height - (/*rows*/ (1) * 8) - 4;

		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, entry_nova->info.name);
		M_PrintBronzey	( (2 +  0) * 8, drawcur_y, s);

		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, entry_nova->info.cname);
		M_Print			( (2 + 40) * 8 , drawcur_y, s);

		// Players
		drawcur_y = menu_height - (/*rows*/ (7) * 8);

		M_PrintBronzey	( (2 +  0) * 8, drawcur_y - 12, "Players");


		for (int clnum = 0; clnum < server_player_infos_count; clnum ++) {
			server_player_info_t *player = &server_player_infos[clnum];

			WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache)
			unsigned char *c;
			int column	= clnum / 4; // integer division
			int row		= (clnum modulo 4); // remainder
			int x		= (2 + 2 + column * 20) * 8;
			int y		= drawcur_y + row * 10;
			c = palette_rgb_pantsscoreboard[player->top_color];
			DrawQ_Fill (menu_x + x - 4 + 2, menu_y + y + 0 + 0, 28, 5, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			c = palette_rgb_shirtscoreboard[player->bottom_color];
			DrawQ_Fill (menu_x + x - 4 + 2, menu_y + y + 4 + 1, 28, 4, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			M_Print			(x, y, va(vabuf, sizeof(vabuf), "%3d", player->frags));
			x += 4 * 8;
			M_Print			(x, y, player->name);
		} // each player
		break;
	} // while 1
}


static void M_ServerList_Key(cmd_state_t *cmd, int key, int ascii)
{
	local_scroll_is_blocked = false;

	switch (key) {
	case K_MOUSE2: // fall thru
	case K_ESCAPE:
#if 1
		M_Menu_MultiPlayer_f (cmd);
#else
		M_Menu_LanConfig_f(cmd);
#endif
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1)
			break;

		{
			// This detects MOUSE collision with column headers
			// And sets the sort
			hotspotx_s *h = &hotspotxs[hotspotx_hover];
			if (h->hotspottype == hotspottype_listitem) {
				int trueidx = -(h->trueidx + 1);
				int did_click_the_current_sort_index = slist_sort_by == trueidx;
				if (did_click_the_current_sort_index) {
					// If clicked what is already the key, toggle it.
					slist_sort_ascending = !slist_sort_ascending;
				} else if (trueidx == SLIST_SORTBY_PLAYERS_1) {
					// Changed sort key, set to descending unless players
					slist_sort_ascending = 0; // Biggest players first is default for that column
				} else {
					slist_sort_ascending = 1;
				}

				slist_sort_by = trueidx;
				serverlist_viewx_dirty = true;
				break; // HIT A COLUMN HEADER, GET OUT.
			}
		}

		{
			// Baker: The column headers are not part of local_cursor
			// but mouse interact only.
			int new_cursor = hotspotx_hover + startrow;
			int is_new_cursor = new_cursor != local_cursor;

			local_scroll_is_blocked = true; // PROTECT AGAINST AUTOSCROLL

			local_cursor = new_cursor;

			if (is_new_cursor) {
				// GET OUT!  SET FOCUS TO ITEM
				Commit_To_Cname (); // Lock to this name
				break;
			}

			// Same cursor -- double click in effect.
			// fall thru
			double new_click_time = Sys_DirtyTime();
			double click_delta_time = local_click_time ? (new_click_time - local_click_time) : 0;
			local_click_time = new_click_time;

			if (is_new_cursor == false && click_delta_time && click_delta_time < DOUBLE_CLICK_0_5) {
				// Fall through to K_ENTER and connect
			} else {
				// Entry changed or not fast enough do not fall through to K_ENTER
				break;
			}
		} // block


	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			int idx_nova = serverlist_list[local_cursor];
			serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);

			char *s_ipaddy = entry_nova->info.cname;
			char *s_name = entry_nova->info.name;

			// Baker: Issuing a connect will exit the menu
			// So we set the re-entrance state "menu_state_reenter" first
			menu_state_reenter = (lanConfig_cursor == /*qw*/ 2) ? m_slist_qw_28 /*3*/ : m_slist_27;

			// Baker: Prevent annoyingly long console lines
			va_super (connect_string, /*#chars*/ 80, "connect " QUOTED_S " // %s", s_ipaddy, s_name);
			Cbuf_AddTextLine (cmd, connect_string);

			// Baker r0072: Add maps menu map to command history for recall.
			Key_History_Push_String (connect_string);
		} // local_count

		break;

	case K_SPACE: // This is "refresh list"

#if 1
		Net_Slist_Both_f (cmd);
#else
		if (lanConfig_cursor == 2)
			Net_SlistQW_f(cmd);
		else
			Net_Slist_f(cmd);
#endif
		break;

	case K_HOME:
		if (local_count)
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_END:
		if (local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) // PGUP does not wrap, stops at start
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) // K_MWHEELUP does not wrap, stops at start
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) // PGDN does not wrap, stops at end
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_UPARROW:
		// S_LocalSound ("sound/misc/menu1.wav"); // commented out for less noisy
		local_cursor--;
		if (local_cursor < 0) // K_UPARROW wraps around to end EXCEPT ON MEGA LISTS
			local_cursor = 0; // MEGA LIST DOES NOT WRAP, CURSOR STAYS AT START
		Commit_To_Cname ();
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav"); // commented out for less noisy
		local_cursor++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start EXCEPT ON MEGA LISTS
			local_cursor = local_count - 1;// MEGA LIST DOES NOT WRAP, CURSOR STAYS AT END
		Commit_To_Cname ();
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		break;
	} // sw

}

#undef local_count
#undef local_cursor
#undef visiblerows
#undef startrow
#undef endrow
#undef local_click_time
#undef local_scroll_is_blocked
