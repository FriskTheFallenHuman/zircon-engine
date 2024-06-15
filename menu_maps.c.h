// menu_maps.c.h

#define		local_count					m_maplist_count
#define		local_cursor				m_maplist_cursor
#define 	visiblerows 				m_maplist_visiblerows
#define 	startrow 					m_maplist_startrow
#define 	endrow 						m_maplist_endrow
#define		local_click_time			m_maplist_click_time
#define		local_scroll_is_blocked		m_maplist_scroll_is_blocked

float	m_maplist_click_time;
int		m_maplist_scroll_is_blocked;
int		m_maplist_cursor;
int		m_maplist_visiblerows;
int		m_maplist_startrow;
int		m_maplist_endrow;

#if 1

saveo_t mapeo;
int mapeo_oldmaplistcursor = -1;


// DYNX: 0


// DYNX: Change - very specific
WARP_X_CALLERS_ (M_Load2_Draw)
WARP_X_CALLERS_ (SavegameTextureChange)
#define saveo NOT_FOOL
// Q: WHY NOT DELETE CURRENT CURRENT TEXTURE (HOW)
// AND CREATE A NEW ONE ...
// Answer: We still need same name.
//ccs *supername,
// , int superw, int superh
static void MAPX_TextureChange (saveo_t *super, ccs *q3mapname_or_null)
{
	extern int image_width, image_height;

	if (q3mapname_or_null == NULL) {
		Mem_FreeNull_ (super->savif_bgra_pels);
		return;
	}

	// Baker: This either finds it in list (nothing is done) or creates a texture.
	va_super (levelshots_map_tga, MAX_QPATHX2_256, "levelshots/%s.tga", q3mapname_or_null);

	Mem_FreeNull_ (super->savif_bgra_pels);

	bgra4 *screenshotpels_za = (bgra4 *)loadimagepixelsbgra (
		levelshots_map_tga,
		q_tx_complain_false,
		q_tx_allowfixtrans_false,
		q_tx_convertsrgb_false,
		q_tx_miplevel_null
	);

	// TRANSFER
	if (screenshotpels_za == NULL) {
		return; // We already freed, just get out.  Make sure to not render.
	}

	super->superw = image_width;
	super->superh = image_height;
	super->aspecthdivw = image_height / (float)image_width;
	super->aspectheight = 320.0 * super->aspecthdivw;

//	if (image_width != image_height)
//		int k = 4;

	WARP_X_ (DrawQ_SuperPic_Video)

	dynamic_baker_texture_t king = {0};
	super->savif_bgra_pels = screenshotpels_za; // ACQUIRED
	Dynamic_Baker_Texture2D_Prep (&king, q_is_dirty_true, super->supername,
		super->savif_bgra_pels, super->superw, super->superh);

}

//WARP_X_ (UnlinkVideoTexture)
//WARP_X_CALLERS_ (M_Load2_Draw) // like SAVEGAME_PIC_NAME
//static void DynamicTexturePurge (ccs *dynamic_pic_name, bgra4 *image_pels, int width, int height) // Only called when there are no save games.
//{
//	// free the texture (this does not destroy the cachepic_t, which is external)
//	// CPIFX: Baker: I think this is called to clear the buff free old pic
//	dynamic_baker_texture_t king = {0};
//	Dynamic_Baker_Texture2D_Prep (&king, q_is_dirty_false, SAVEGAME_PIC_NAME, image_pels, SAVEGAME_PIC_WIDTH_512, SAVEGAME_PIC_HEIGHT_320);
//	Draw_FreePic	(SAVEGAME_PIC_NAME); // doesn't free the pic, runs R_SkinFrame_PurgeSkinFrame
//	king.bpic->skinframe = NULL; // ? Q1SKY
//}
//

#endif

WARP_X_ (M_Menu_ServerList_f)

// maps/levelshots/whatever.png ...
void M_Menu_Maps_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu);
	menu_state_set_nova (m_maps_26);
	m_entersound = true;

	if (m_maplist_count == 0)
		GetMapList("", NULL, 0, /*is_menu_fill*/ true, /*autocompl*/ false, /*suppress*/ false);

#define LEVELSHOTS_PIC_NAME		"levelshotspic"

	if (!mapeo.supername[0]) {
		c_strlcpy (mapeo.supername, "levelshotspic");
		mapeo.superw = SAVEGAME_PIC_WIDTH_512;
		mapeo.superh = SAVEGAME_PIC_HEIGHT_320;
		mapeo.is_fixed_size = false;
	}
	//mapeo_oldmaplistcursor = -1;//

	startrow = not_found_neg1, endrow = not_found_neg1;

#if 0 // RE-ENTRANT SO DON'T SET CURSOR
	local_cursor = 0;
#endif

	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;

	menu_state_reenter = 0; local_scroll_is_blocked = false;
}

static void M_Maps_Draw (void)
{
	int n;
	cachepic_t *p0;
	const char *s_available = "Maps";

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Background(640, vid_conheight.integer, q_darken_true);

	M_Print (48 + 32, 32, s_available);

	// scroll the list as the cursor moves

	drawcur_y = 8 * 6;
	visiblerows = (int)((menu_height - (2 * 8) - drawcur_y) / 8) - 8;

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

	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((640 - Draw_GetPicWidth(p0)) / 2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (!in_range_beyond (0, n, local_count))
				continue;

			if (n == local_cursor)
				drawsel_idx = (n - startrow) /*relative*/;

			maplist_s *mx = &m_maplist[n];

			Hotspots_Add2	(menu_x + (8 * 9), menu_y + drawcur_y, (55 * 8) /*360*/, 8, 1, hotspottype_button, n);
			M_ItemPrint		((8 +  0) * 10, drawcur_y, (const char *)mx->s_name_trunc_16_a, true);
			M_ItemPrint		((8 + 18) * 10, drawcur_y, (const char *)mx->s_bsp_code, true);
			M_ItemPrint2	((8 + 21) * 10, drawcur_y, (const char *)mx->s_map_title_trunc_28_a, true);


			drawcur_y +=8;
		} // for

		// Print current map
		if (1) {
			const char *s = "(disconnected)";
			if (cls.state == ca_connected && cls.signon == SIGNONS_4) // MAPS MENU
				s = cl.worldbasename;
			int slen = (int)strlen(s);
			M_Print        (640 - (11   * 8) - 20, 40, "current map");
			M_PrintBronzey (640 - (slen * 8) - 20, 48, s);
		}
	} // endrow > startrow
	else
	{
		M_Print(80, drawcur_y, "No Maps found");
	}

	PPX_DrawSel_End ();

// NEW:
	int effective_cursor = local_cursor;

	if (effective_cursor != mapeo_oldmaplistcursor && local_count != 0) {
		maplist_s *mx = &m_maplist[local_cursor];
		ccs *q3mapname = (ccs *)mx->s_name_after_maps_folder_a;

		MAPX_TextureChange (&mapeo, q3mapname);

		mapeo_oldmaplistcursor = effective_cursor;
	} // while 1

	if (mapeo.savif_bgra_pels) {
		int col6 = menu_x + (8 + 21) * 10 + 28 * 8 + 8;
		int remain = vid_conwidth.integer - col6;
		int w = remain - 2;
		int height = mapeo.aspecthdivw == 1 ? w * 3/4 : w * mapeo.aspecthdivw;

		p0 = Draw_CachePic_Flags (mapeo.supername, CACHEPICFLAG_NOTPERSISTENT);
		DrawQ_Pic (
			vid_conwidth.integer - remain - 2,
			(vid_conheight.integer - height) / 2,
			p0,
			w /*remain*/ ,
			height /*mapeo.aspectheight*/,
			q_rgba_solid_white_4_parms,
			DRAWFLAG_NORMAL_0
		);

	}
}


static void M_Maps_Key(cmd_state_t *cmd, int key, int ascii)
{
	int lcase_ascii;

	local_scroll_is_blocked = false;

	switch (key) {
	case K_MOUSE2: // fall thru to K_ESCAPE and then exit
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1)
			break;

		local_cursor = hotspotx_hover + startrow; // fall thru
		{
			int new_cursor = hotspotx_hover + startrow;
			int is_new_cursor = new_cursor != local_cursor;

			local_scroll_is_blocked = true; // PROTECT AGAINST AUTOSCROLL

			local_cursor = new_cursor;

			if (is_new_cursor) {
				// GET OUT!  SET FOCUS TO ITEM
				// Commit_To_Cname ();
				break;
			}

			// Same cursor -- double click in effect.
			// fall thru
			double new_click_time = Sys_DirtyTime();
			double click_delta_time = local_click_time ? (new_click_time - local_click_time) : 0;
			local_click_time = new_click_time;

			if (is_new_cursor == false && click_delta_time && click_delta_time < DOUBLE_CLICK_0_5) {
				// Fall through and load the map
			} else {
				// Entry changed or not fast enough
				break;
			}
		} // block

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			maplist_s *mx = &m_maplist[local_cursor];

			va_super (tmp, 1024, "map %s // %s", mx->s_name_after_maps_folder_a, mx->s_map_title_trunc_28_a);

			Cbuf_AddTextLine (cmd, tmp);

			// Baker r0072: Add maps menu map to command history for recall.
			Key_History_Push_String (tmp);

			// Maps is re-entrant
			menu_state_reenter = m_maps_26;
			KeyDest_Set (key_game); // key_dest = key_game;
			menu_state_set_nova (m_none);
		}

		break;

	case K_SPACE:
		Con_PrintLinef ("Refreshing maps menu list ...");
		GetMapList("", NULL, 0, /*is_menu_fill*/ true, /*autocompl*/ false, /*suppress*/ false);
		break;

	case K_HOME:
		if (local_count)
			local_cursor = 0;
		break;

	case K_END:
		if (local_count)
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
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0) // K_UPARROW wraps around to end EXCEPT ON MEGA LISTS
			local_cursor = 0; // MEGA EXCEPTION
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor ++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start EXCEPT ON MEGA LISTS
			local_cursor = local_count - 1 ; // MEGA EXCEPTION
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		lcase_ascii = tolower(ascii);
		if (in_range ('a', lcase_ascii, 'z')) {
			// Baker: This is a wraparound seek
			// Find the next item starting with 'a'
			// or whatever key someone pressed
			int startx = local_cursor;
			char sprefix[2] = { (char)lcase_ascii, 0 };

			for (int iters = 0; iters < local_count; iters ++) {
				startx ++;
				if (startx >= local_count) {
					startx = 0;
				}

				maplist_s *mx = &m_maplist[startx];

				if (String_Starts_With_Caseless ((char *)mx->s_name_after_maps_folder_a, sprefix)) {
					local_cursor = startx;
					break;
				} // if
			} // iters


		} // if a-z
		break;
	} // sw

}

#undef	local_count
#undef	local_cursor
#undef 	visiblerows
#undef 	startrow
#undef 	endrow
#undef 	local_scroll_is_blocked
