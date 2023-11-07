// menu_video_nova.c.h


#define		m_local_cursor		video2_cursor


extern cvar_t cl_maxfps;

#define VIDEO2_ITEMS 6
static int video2_cursor = 0;

static int video2_cursor_table[VIDEO2_ITEMS + 1] = {
	68,
	80,
	96,
	112,
	144,
	160,
	172
};

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

void M_Menu_VideoNova_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_videonova);
	m_entersound = true;
	video2_cursor = 0;

	// output is menu_video_resolution
	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);
}



#define fps_rates_count ARRAY_COUNT(fps_rates)

static void M_VideoNova_Draw (void)
{
	int t;
	cachepic_t	*p0;
	char vabuf[1024];
	char *s;

	if(!!vid_fullscreen.integer != menu_video_resolutions_forfullscreen) {
		video_resolution_t *res = &menu_video_resolutions[menu_video_resolution];
		menu_video_resolutions_forfullscreen = !!vid_fullscreen.integer;
		M_Menu_Video_FindResolution(res->width, res->height, res->pixelheight);
	}

	M_Background(320, 200);

	M_DrawPic(16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/vidmodes");
	M_DrawPic((320-p0->width)/2, 4, p0 /*"gfx/vidmodes"*/, NO_HOTSPOTS_0, NA0, NA0);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	t = 0;

	// Current and Proposed Resolution
	M_PrintBronzey(16, video2_cursor_table[t] - 24, "    Current Resolution");
	if (vid_supportrefreshrate && vid.userefreshrate && vid.fullscreen)
		M_PrintBronzey(220, video2_cursor_table[t] - 24, va(vabuf, sizeof(vabuf), "%dx%d %.2fhz", vid.width, vid.height, vid.refreshrate));
	else
		M_PrintBronzey(220, video2_cursor_table[t] - 24, va(vabuf, sizeof(vabuf), "%dx%d", vid.width, vid.height));

	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider);
	M_Print(16, video2_cursor_table[t], "        New Resolution");
	M_Print(220, video2_cursor_table[t], va(vabuf, sizeof(vabuf), "%dx%d", menu_video_resolutions[menu_video_resolution].width, menu_video_resolutions[menu_video_resolution].height));
	//M_Print(96, video_cursor_table[t] + 8, va(vabuf, sizeof(vabuf), "Type: %s", menu_video_resolutions[menu_video_resolution].type));
	t++;


	// Fullscreen
	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider);
	M_Print(16, video2_cursor_table[t], "            Fullscreen");
	M_DrawCheckbox(220, video2_cursor_table[t], vid_fullscreen.integer);
	t++;

	// Vertical Sync
	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider);
	M_ItemPrint(16, video2_cursor_table[t], "         Vertical Sync", true);
	M_DrawCheckbox(220, video2_cursor_table[t], vid_vsync.integer);
	t++;

	// Maximum fps
	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider);
	M_ItemPrint(16, video2_cursor_table[t], "       Max Frames/Sec", true);
	//M_DrawSlider(220, video2_cursor_table[t], cl_maxfps.integer, 0, 288 * 2);


	va(vabuf, sizeof(vabuf), "%d %s", cl_maxfps.integer, cl_maxfps.integer ? "" : " (no limit, choppy?)" );
	DrawQ_String(menu_x + 220, menu_y + video2_cursor_table[t], vabuf, 0, /*w*/ 8, 8, /*rgba*/ 1, 1, 1, 1, 0, /*outcolor*/ NULL, true, FONT_MENU);
	t++;


	// "Apply" button
	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_button);
	M_PrintRed(220, video2_cursor_table[t], "Apply");
	t++;

	// "Go Advanced" button
	Hotspots_Add (menu_x + 48, menu_y + video2_cursor_table[t], 272, 8 + 1, 1, hotspottype_button);
	M_Print(220, video2_cursor_table[t], "Go Advanced");
	t++;


	// "Apply" button
	if (vid.fullscreen) {
		s = va(vabuf, sizeof(vabuf), "ALT-ENTER: Quick Set %dx%d windowed", (int)vid_window_width.value, (int)vid_window_height.value);
	} else {
		s = va(vabuf, sizeof(vabuf), "ALT-ENTER: Quick Set %dx%d fullscreen", (int)vid_fullscreen_width.value, (int)vid_fullscreen_height.value);
	}
	M_PrintBronzey (24, video2_cursor_table[t], s);
	t++;

	// Cursor
	M_DrawCharacter(200, video2_cursor_table[video2_cursor], 12+((int)(realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Menu_VideoNova_AdjustSliders (int dir)
{
	int t;

	S_LocalSound ("sound/misc/menu3.wav");

	t = 0;
	if (video2_cursor == t++) // 1
	{
		// Resolution
		int r;
		for(r = 0;r < menu_video_resolutions_count;r++)
		{
			menu_video_resolution += dir;
			if (menu_video_resolution >= menu_video_resolutions_count)
				menu_video_resolution = 0;
			if (menu_video_resolution < 0)
				menu_video_resolution = menu_video_resolutions_count - 1;
			if (menu_video_resolutions[menu_video_resolution].width < 600)
				continue;
#if (defined(_WIN32) && !defined(CORE_SDL)) || defined(MACOSX)
			if (menu_video_resolutions[menu_video_resolution].width > vid.desktop_width)
				continue;
#endif // defined(_WIN32) && !defined(CORE_SDL)
			if (menu_video_resolutions[menu_video_resolution].width >= vid_minwidth.integer && menu_video_resolutions[menu_video_resolution].height >= vid_minheight.integer)
				break;
		}
	}
	else if (video2_cursor == t++) // 2
		Cvar_SetValueQuick (&vid_fullscreen, !vid_fullscreen.integer);
	else if (video2_cursor == t++) // 3
		Cvar_SetValueQuick (&vid_vsync, !vid_vsync.integer);
}


static void M_VideoNova_Key (int key, int ascii)
{
	int j;
	int fpsidxup = 0;
	int fpsidxdw = fps_rates_count  - 1;


	switch (key) {
	//case K_MOUSE2: // Fall
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { m_local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		// vid_shared.c has a copy of the current video config. We restore it
		Cvar_SetValueQuick(&vid_fullscreen, vid.fullscreen);
		Cvar_SetValueQuick(&vid_bitsperpixel, vid.bitsperpixel);
		Cvar_SetValueQuick(&vid_samples, vid.samples);
		if (vid_supportrefreshrate)
			Cvar_SetValueQuick(&vid_refreshrate, vid.refreshrate);
		Cvar_SetValueQuick(&vid_userefreshrate, vid.userefreshrate);

		//S_LocalSound ("sound/misc/menu1.wav");
		M_Menu_OptionsNova_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else video2_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;

		switch (video2_cursor) {

		case (VIDEO2_ITEMS - 3): // fps
			// increase get greater idx, if none use 0
			for (j = 0; j < (int)fps_rates_count; j ++) {
				if (fps_rates[j].rate > cl_maxfps.integer) {
					fpsidxup = j;
					break;
				} // if
			} // for
			Cvar_SetValueQuick (&cl_maxfps, fps_rates[fpsidxup].rate);

			break;

		case (VIDEO2_ITEMS - 2): // APPLY

#ifdef __ANDROID__
	Con_PrintLinef ("vid_restart not supported for this build");
	return;
#endif // __ANDROID__

			Cvar_SetValueQuick (&vid_width, menu_video_resolutions[menu_video_resolution].width);
			Cvar_SetValueQuick (&vid_height, menu_video_resolutions[menu_video_resolution].height);
			Cvar_SetValueQuick (&vid_conwidth, menu_video_resolutions[menu_video_resolution].conwidth);
			Cvar_SetValueQuick (&vid_conheight, menu_video_resolutions[menu_video_resolution].conheight);
			Cvar_SetValueQuick (&vid_pixelheight, menu_video_resolutions[menu_video_resolution].pixelheight);
			Cbuf_AddTextLine ("vid_restart");
			//M_Menu_OptionsNova_f ();
			break;
		case (VIDEO2_ITEMS - 1): // ADVANCED
			m_video_prevstate = m_videonova;
			M_Menu_Video_f ();
			break;

		default:
			M_Menu_VideoNova_AdjustSliders (1);
		} // sw
		break;


	case K_HOME:
		video2_cursor = 0;
		break;

	case K_END:
		video2_cursor = VIDEO2_ITEMS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		video2_cursor--;
		if (video2_cursor < 0)
			video2_cursor = VIDEO2_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		video2_cursor++;
		if (video2_cursor >= VIDEO2_ITEMS)
			video2_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		// decrease get lesser idx
		if (video2_cursor == VIDEO2_ITEMS - 3) {
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
		if (video2_cursor == VIDEO2_ITEMS - 3) {
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


void VID_ListModes_f(void)
{
	int j;

	if (vid.desktop_width) {
		Con_PrintLinef ("Desktop: %d x %d ", vid.desktop_width, vid.desktop_height);
	} else {
		Con_PrintLinef ("Desktop: not queried");
	}

	for (j = 0; j < menu_video_resolutions_count; j++) {
		video_resolution_t *mm = &menu_video_resolutions[j];
		//if (menu_video_resolutions[i].width < 600)
		//	continue;

		//if (menu_video_resolutions[i].width > vid.desktop_width)
		//	continue;

		// if the new mode would be a worse match in width, skip it
		Con_PrintLinef ("%03d: %d x %d ", j, mm->width, mm->height);

	}
        
    Con_PrintLinef ("Current mode:    vid.width vid.height vid.fullscreen " NEWLINE " ... %d x %d fs %d ", vid.width, vid.height, vid.fullscreen);
    Con_PrintLinef ("Current mode dot!:vid.width vid.height vid.fullscreen " NEWLINE " ... %d x %d fs %d ", vid.mode.width, vid.mode.height, vid.mode.fullscreen);
    Con_PrintLinef ("Current cvars:   vid_width vid_height vid_fullscreen " NEWLINE " ... %d x %d fs %d ", vid_width.integer, vid_height.integer, vid_fullscreen.integer);
    Con_PrintLinef ("Current scale:   fullscreen %f windowed %f", vid_fullscreen_conscale.value, vid_window_conscale.value);
    Con_PrintLinef ("Current mag 360: %f", yfactors);
    Con_PrintLinef ("Current mag      %f", yfactor_mag_360);
}

#undef m_local_cursor
