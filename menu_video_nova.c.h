// menu_video_nova.c.h

#define 	local_count		VID_MENU_COUNT_8
#define		local_cursor	video2_cursor


extern cvar_t cl_maxfps;

static int video2_cursor = 0;


typedef enum res_e_ {
	VID_MENU_NEW_RES_0 = 0,
	VID_MENU_FULLSCREEN_1 = 1,
	VID_MENU_VSYNC_2 = 2,
	VID_MENU_MAXFPS_3 = 3,
	VID_MENU_FORCE_DESKTOP_FULLSCREEN_4 = 4,
	VID_MENU_APPLY_5 = 5,
	VID_MENU_ADVANCED_6 = 6,
	VID_MENU_TEXTURE_FILTERING_7 = 7,
	VID_MENU_COUNT_8
} res_e;

#define MNU_VIDEO_TOP_ROW_36	36
#define MNU_VIDEO_ROW_SIZE_16	16

static ratetable_t fps_rates[] =
{
	{0, " (no limit)"},
	{60, ""},
	{72, ""},
	{120, ""},
	{144, ""},
	{200, ""},
	{240, ""},
	{288, ""},
	{240 * 2, ""},
	{288 * 2, ""},
};

void M_Menu_Video_Nova_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_video_nova);
	m_entersound = true;
	local_cursor = 0;

	// output is menu_video_resolution
	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);
}


#define fps_rates_count ARRAY_COUNT(fps_rates)

static void M_Video_Nova_Draw (void)
{
	cachepic_t	*p0;
	char vabuf[1024];
	char *s;

	if (!!vid_fullscreen.integer != menu_video_resolutions_forfullscreen) {
		video_resolution_t *res = &menu_video_resolutions[menu_video_resolution];
		menu_video_resolutions_forfullscreen = !!vid_fullscreen.integer;
		M_Menu_Video_FindResolution(res->width, res->height, res->pixelheight);
	}

	M_Background(320, 200, q_darken_true);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/vidmodes");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/vidmodes", NO_HOTSPOTS_0, NA0, NA0);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	drawcur_y = MNU_VIDEO_TOP_ROW_36;

	// Baker: The M_ functions add menux + and menuy + to coordinates
	// the non- M_ functions need to add menu_x and menu_y to coordinates
	// So yeah ...

	// Current and Proposed Resolution
	M_PrintBronzey	(16, drawcur_y, "    Current Resolution");
	if (vid_supportrefreshrate && vid.userefreshrate && vid.fullscreen)
		M_PrintBronzey(220, drawcur_y, va(vabuf, sizeof(vabuf), "%dx%d %.2fhz", vid.width, vid.height, vid.refreshrate));
	else
		M_PrintBronzey(220, drawcur_y, va(vabuf, sizeof(vabuf), "%dx%d", vid.width, vid.height));

	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_Print			(16, drawcur_y, "        New Resolution");
	M_Print			(220, drawcur_y, va(vabuf, sizeof(vabuf), "%dx%d", menu_video_resolutions[menu_video_resolution].width, menu_video_resolutions[menu_video_resolution].height));
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// Fullscreen
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_Print			(16, drawcur_y, "            Fullscreen");
	M_DrawCheckbox	(220, drawcur_y, vid_fullscreen.integer);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// Vertical Sync
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_ItemPrint		(16, drawcur_y, "         Vertical Sync", q_enabled_true);
	M_DrawCheckbox	(220, drawcur_y, vid_vsync.integer);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// Maximum fps
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_ItemPrint		(16, drawcur_y, "        Max Frames/Sec", q_enabled_true);
	va(vabuf, sizeof(vabuf), "%d %s", cl_maxfps.integer, cl_maxfps.integer ? "" : " (no limit, choppy?)" );
	DrawQ_String	(menu_x + 220, menu_y + drawcur_y, vabuf, q_text_maxlen_0, /*w*/ 8, 8, /*rgba*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_MENU);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// Force Desktop Resolution
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_ItemPrint		(16, drawcur_y,  "          Always Force", q_enabled_true);
	DrawQ_String	(menu_x + 16, menu_y + drawcur_y + 8,  "    Desktop Resolution", q_text_maxlen_0, /*w*/ 8, 8, /*rgba*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_MENU);
	M_DrawCheckbox	(220, drawcur_y, vid_desktopfullscreen.integer);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// "Apply" button
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_button, q_force_scale_0);
	M_PrintRed		(220, drawcur_y, "Apply");
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// "Go Advanced" button
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_button, q_force_scale_0);
	M_Print			(220, drawcur_y, "Go Advanced");
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// "Texture Filtering" button
	Hotspots_Add	(menu_x + 48, menu_y + drawcur_y, 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	M_ItemPrint		(16, drawcur_y, "        Texture Filter", q_enabled_true);

	DrawQ_String	(menu_x + 220, menu_y + drawcur_y, TexFilterNameForNum(gl_texturemode_cvar.integer), q_text_maxlen_0, /*w*/ 8, 8, /*rgba*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_MENU);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	// "ALT-ENTER" button
	if (vid.fullscreen) {
		s = va(vabuf, sizeof(vabuf), "ALT-ENTER: Quick Set %dx%d windowed", (int)vid_window_width.value, (int)vid_window_height.value);
	} else {
		s = va(vabuf, sizeof(vabuf), "ALT-ENTER: Quick Set %dx%d fullscreen", (int)vid_fullscreen_width.value, (int)vid_fullscreen_height.value);
	}
	M_PrintBronzey	(24, drawcur_y, s);
	drawcur_y += MNU_VIDEO_ROW_SIZE_16;

	drawcur_y += MNU_VIDEO_ROW_SIZE_16;
	if (fs_userdir[0]) {
		va_super (tmp, 1024, "home: %s", fs_userdir);
		M_Print	(24, drawcur_y, tmp);
		drawcur_y += MNU_VIDEO_ROW_SIZE_16;
	}
		va_super (tmp2, 1024, "current directory: %s", menu_current_working_directory);
		M_Print (24, drawcur_y, tmp2);
		drawcur_y += MNU_VIDEO_ROW_SIZE_16;
#if 0
		va_super (tmp3, 1024, "fs_basedir: %s", fs_basedir);
		M_Print (24, drawcur_y, tmp3);
		drawcur_y += MNU_VIDEO_ROW_SIZE_16;
#endif


	// Cursor
	drawcur_y = MNU_VIDEO_TOP_ROW_36 + MNU_VIDEO_ROW_SIZE_16 + local_cursor * MNU_VIDEO_ROW_SIZE_16;
	M_DrawCharacter	(200, drawcur_y, 12 + ((int)(host.realtime * 4) & 1));

	PPX_DrawSel_End ();
}


static void M_Menu_VideoNova_AdjustSliders (int dir)
{
	S_LocalSound ("sound/misc/menu3.wav");

	switch (local_cursor) {
	case VID_MENU_NEW_RES_0:
		// Resolution
		for (int r = 0; r < menu_video_resolutions_count; r ++) {
			menu_video_resolution += dir;
			if (menu_video_resolution >= menu_video_resolutions_count)
				menu_video_resolution = 0;
			if (menu_video_resolution < 0)
				menu_video_resolution = menu_video_resolutions_count - 1;
			if (menu_video_resolutions[menu_video_resolution].width < 600)
				continue;
#if defined(_WIN32) || defined(MACOSX)
			if (menu_video_resolutions[menu_video_resolution].width > vid.desktop_width)
				continue;
#endif // _WIN32 || MACOSX
			if (menu_video_resolutions[menu_video_resolution].width >= vid_minwidth.integer &&
				menu_video_resolutions[menu_video_resolution].height >= vid_minheight.integer)
				break;
		} // for
		break;

	case VID_MENU_FULLSCREEN_1:
		Cvar_SetValueQuick (&vid_fullscreen, !vid_fullscreen.integer);
		break;

	case VID_MENU_VSYNC_2:
		Cvar_SetValueQuick (&vid_vsync, !vid_vsync.integer);
		break;

	case VID_MENU_FORCE_DESKTOP_FULLSCREEN_4:
		Cvar_SetValueQuick (&vid_desktopfullscreen, !vid_desktopfullscreen.integer);
		break;

	case VID_MENU_TEXTURE_FILTERING_7:
		{
			int newval = gl_texturemode_cvar.integer + dir;
			if (newval > 5)
				newval = 0;
			else if (newval < 0)
				newval = 5;

			Cvar_SetValueQuick (&gl_texturemode_cvar, newval);
		}
		break;

	} // sw
}


static void M_Video_Nova_Key (cmd_state_t *cmd, int key, int ascii)
{
	int j;
	int fpsidxup = 0;
	int fpsidxdw = fps_rates_count  - 1;

	switch (key) {
	case K_MOUSE2:
		if (Hotspots_DidHit_Slider()) {
			local_cursor = hotspotx_hover;
			goto leftus;
		}
		// fall thru
	case K_ESCAPE:
		// vid_shared.c has a copy of the current video config. We restore it
		Cvar_SetValueQuick(&vid_fullscreen, vid.fullscreen);
		Cvar_SetValueQuick(&vid_bitsperpixel, vid.bitsperpixel);
		Cvar_SetValueQuick(&vid_samples, vid.samples);
		// Baker: Surprising we aren't doing the cvars vid_width, etc.
		if (vid_supportrefreshrate)
			Cvar_SetValueQuick(&vid_refreshrate, vid.refreshrate);
		Cvar_SetValueQuick(&vid_userefreshrate, vid.userefreshrate);

		//S_LocalSound ("sound/misc/menu1.wav");
		M_Menu_Options_Nova_f (cmd);
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else local_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;

		switch (local_cursor) {
		case VID_MENU_MAXFPS_3:
			 // Baker: fps
			// increase get greater idx, if none use 0
			for (j = 0; j < (int)fps_rates_count; j ++) {
				if (fps_rates[j].rate > cl_maxfps.integer) {
					fpsidxup = j;
					break;
				} // if
			} // for
			Cvar_SetValueQuick (&cl_maxfps, fps_rates[fpsidxup].rate);
			break;

		case VID_MENU_APPLY_5:
			// Baker: APPLY

#ifdef __ANDROID__
	Con_PrintLinef ("vid_restart not supported for this build");
	return;
#endif // __ANDROID__

			Cvar_SetValueQuick (&vid_width, menu_video_resolutions[menu_video_resolution].width);
			Cvar_SetValueQuick (&vid_height, menu_video_resolutions[menu_video_resolution].height);
			Cvar_SetValueQuick (&vid_conwidth, menu_video_resolutions[menu_video_resolution].conwidth);
			Cvar_SetValueQuick (&vid_conheight, menu_video_resolutions[menu_video_resolution].conheight);
			Cvar_SetValueQuick (&vid_pixelheight, menu_video_resolutions[menu_video_resolution].pixelheight);
			Cbuf_AddTextLine (cmd, "vid_restart");
			// We stay put
			break;

		case VID_MENU_ADVANCED_6: // ADVANCED
			m_video_prevstate = m_video_nova;
			M_Menu_Video_Classic_f (cmd);
			break;

		default:
			M_Menu_VideoNova_AdjustSliders (1);
		} // sw
		break;

	case K_HOME:
		local_cursor = 0;
		break;

	case K_END:
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

leftus:
	case K_LEFTARROW:
		if (local_cursor == VID_MENU_MAXFPS_3) {
			// decrease get lesser fps idx
			// if none, use hi
			for (j = fps_rates_count - 1; j >= 0; j --) {
				if (fps_rates[j].rate < cl_maxfps.integer) {
					fpsidxdw = j;
					break;
				} // if
			} // for
			Cvar_SetValueQuick (&cl_maxfps, fps_rates[fpsidxdw].rate);
			break;
		}

		M_Menu_VideoNova_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		if (local_cursor == VID_MENU_MAXFPS_3) {
			// we get greater fps idx
			for (j = 0; j < (int)fps_rates_count; j ++) {
				if (fps_rates[j].rate > cl_maxfps.integer) {
					fpsidxup = j;
					break;
				} // if
			} // for
			Cvar_SetValueQuick (&cl_maxfps, fps_rates[fpsidxup].rate);
			break;
		}

		M_Menu_VideoNova_AdjustSliders (1);
		break;
	} // sw
}


void VID_ListModes_f(cmd_state_t *cmd)
{
	int j;

	/*if (vid.desktop_width) {
		Con_PrintLinef ("Desktop: %d x %d ", vid.desktop_width, vid.desktop_height);
	} else {
		Con_PrintLinef ("Desktop: not queried");
	}*/

	for (j = 0; j < menu_video_resolutions_count; j++) {
		video_resolution_t *mm = &menu_video_resolutions[j];

		// if the new mode would be a worse match in width, skip it
		Con_PrintLinef ("%03d: %d x %d ", j, mm->width, mm->height);

	}

#if 1
    Con_PrintLinef ("Current mode:    vid.width vid.height vid.fullscreen " NEWLINE " ... %d x %d fs %d ", vid.width, vid.height, vid.fullscreen);
    Con_PrintLinef ("Current mode dot!:vid.width vid.height vid.fullscreen " NEWLINE " ... %d x %d fs %d ", vid.mode.width, vid.mode.height, vid.mode.fullscreen);
    Con_PrintLinef ("Current cvars:   vid_width vid_height vid_fullscreen " NEWLINE " ... %d x %d fs %d ", vid_width.integer, vid_height.integer, vid_fullscreen.integer);
    Con_PrintLinef ("Current scale:   fullscreen %f windowed %f", vid_fullscreen_conscale.value, vid_window_conscale.value);
    Con_PrintLinef ("Current mag 360: %f", yfactors);
    Con_PrintLinef ("Current mag      %f", yfactor_mag_360);
#endif
}

#undef local_count
#undef local_cursor
