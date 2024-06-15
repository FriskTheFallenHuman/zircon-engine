// menu_video_classic.c.h

#define 	local_count		VIDEO_ITEMS
#define		local_cursor	video_cursor

#define VIDEO_ITEMS 10
static int video_cursor = 0;
static int video_cursor_table[VIDEO_ITEMS] = {68, 88, 96, 104, 112, 120, 128, 136, 144, 152};
static int menu_video_resolution;

video_resolution_t *video_resolutions;
int video_resolutions_count;

static video_resolution_t *menu_video_resolutions;
static int menu_video_resolutions_count;

static void M_Menu_Video_FindResolution(int w, int h, float a)
{
	int i;

	if (menu_video_resolutions_forfullscreen)
	{
		menu_video_resolutions = video_resolutions;
		menu_video_resolutions_count = video_resolutions_count;
	}
	else
	{
		menu_video_resolutions = video_resolutions_hardcoded;
		menu_video_resolutions_count = video_resolutions_hardcoded_count;
	}

	// Look for the closest match to the current resolution
	menu_video_resolution = 0;
	for (i = 1;i < menu_video_resolutions_count;i++) {
		// if the new mode would be a worse match in width, skip it
		if (abs(menu_video_resolutions[i].width - w) > abs(menu_video_resolutions[menu_video_resolution].width - w))
			continue;
		// if it is equal in width, check height
		if (menu_video_resolutions[i].width == w && menu_video_resolutions[menu_video_resolution].width == w)
		{
			// if the new mode would be a worse match in height, skip it
			if (abs(menu_video_resolutions[i].height - h) > abs(menu_video_resolutions[menu_video_resolution].height - h))
				continue;
			// if it is equal in width and height, check pixel aspect
			if (menu_video_resolutions[i].height == h && menu_video_resolutions[menu_video_resolution].height == h)
			{
				// if the new mode would be a worse match in pixel aspect, skip it
				if (fabs(menu_video_resolutions[i].pixelheight - a) > fabs(menu_video_resolutions[menu_video_resolution].pixelheight - a))
					continue;
				// if it is equal in everything, skip it (prefer earlier modes)
				if (menu_video_resolutions[i].pixelheight == a && menu_video_resolutions[menu_video_resolution].pixelheight == a)
					continue;
				// better match for width, height, and pixel aspect
				menu_video_resolution = i;
			}
			else // better match for width and height
				menu_video_resolution = i;
		}
		else // better match for width
			menu_video_resolution = i;
	}
}

void M_Menu_Video_Classic_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_video_classic);
	m_entersound = true;

	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);
}


static void M_Video_Classic_Draw (void)
{
	int t;
	cachepic_t	*p0;
	char vabuf[1024];

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
	t = 0;

	// Current and Proposed Resolution
	M_Print(16, video_cursor_table[t] - 12, "    Current Resolution");
	if (vid_supportrefreshrate && vid.userefreshrate && vid.fullscreen)
		M_Print(220, video_cursor_table[t] - 12, va(vabuf, sizeof(vabuf), "%dx%d %.2fhz", vid.width, vid.height, vid.refreshrate));
	else
		M_Print(220, video_cursor_table[t] - 12, va(vabuf, sizeof(vabuf), "%dx%d", vid.width, vid.height));

	M_Print(16, video_cursor_table[t], "        New Resolution");
	M_Print(220, video_cursor_table[t], va(vabuf, sizeof(vabuf), "%dx%d", menu_video_resolutions[menu_video_resolution].width, menu_video_resolutions[menu_video_resolution].height));
	M_Print(96, video_cursor_table[t] + 8, va(vabuf, sizeof(vabuf), "Type: %s", menu_video_resolutions[menu_video_resolution].type));
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// Antialiasing
	M_Print(16, video_cursor_table[t], "          Antialiasing");
	M_DrawSlider(220, video_cursor_table[t], vid_samples.value, 1, 32);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// Refresh Rate
	M_ItemPrint(16, video_cursor_table[t], "      Use Refresh Rate", vid_supportrefreshrate);
	M_DrawCheckbox(220, video_cursor_table[t], vid_userefreshrate.integer);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// Refresh Rate
	M_ItemPrint(16, video_cursor_table[t], "          Refresh Rate", vid_supportrefreshrate && vid_userefreshrate.integer);
	M_DrawSlider(220, video_cursor_table[t], vid_refreshrate.value, 50, 150);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// Fullscreen
	M_Print(16, video_cursor_table[t], "            Fullscreen");
	M_DrawCheckbox(220, video_cursor_table[t], vid_fullscreen.integer);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// Vertical Sync
	M_ItemPrint(16, video_cursor_table[t], "         Vertical Sync", true);
	M_DrawCheckbox(220, video_cursor_table[t], vid_vsync.integer);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "    Anisotropic Filter", vid.support.ext_texture_filter_anisotropic);
	M_DrawSlider(220, video_cursor_table[t], gl_texture_anisotropy.integer, 1, vid.max_anisotropy);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "       Texture Quality", true);
	M_DrawSlider(220, video_cursor_table[t], gl_picmip.value, 3, 0);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "   Texture Compression", true);
	M_DrawCheckbox(220, video_cursor_table[t], gl_texturecompression.integer);
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	t++;

	// "Apply" button
	M_Print(220, video_cursor_table[t], "Apply");
	Hotspots_Add (menu_x + 48, menu_y + video_cursor_table[t], 272, 8 + 1, 1, hotspottype_button, q_force_scale_0);
	t++;

	// Cursor
	M_DrawCharacter(200, video_cursor_table[local_cursor], 12+((int)(host.realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Menu_Video_AdjustSliders (int dir)
{
	int t;

	S_LocalSound ("sound/misc/menu3.wav");

	t = 0;
	if (local_cursor == t++) // 1
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
			if (menu_video_resolutions[menu_video_resolution].width >= vid_minwidth.integer && menu_video_resolutions[menu_video_resolution].height >= vid_minheight.integer)
				break;
		}
	}
	else if (local_cursor == t++) // 2
		Cvar_SetValueQuick (&vid_samples, bound(1, vid_samples.value * (dir > 0 ? 2 : 0.5), 32));
	else if (local_cursor == t++) // 3
		Cvar_SetValueQuick (&vid_userefreshrate, !vid_userefreshrate.integer);
	else if (local_cursor == t++) // 4
		Cvar_SetValueQuick (&vid_refreshrate, bound(50, vid_refreshrate.value + dir, 150));
	else if (local_cursor == t++) // 5
		Cvar_SetValueQuick (&vid_fullscreen, !vid_fullscreen.integer);
	else if (local_cursor == t++) // 6
		Cvar_SetValueQuick (&vid_vsync, !vid_vsync.integer);
	else if (local_cursor == t++) // 7
		Cvar_SetValueQuick (&gl_texture_anisotropy, bound(1, gl_texture_anisotropy.value * (dir < 0 ? 0.5 : 2.0), vid.max_anisotropy));
	else if (local_cursor == t++) // 8
		Cvar_SetValueQuick (&gl_picmip, bound(0, gl_picmip.value - dir, 3));
	else if (local_cursor == t++) // 9
		Cvar_SetValueQuick (&gl_texturecompression, !gl_texturecompression.integer);
}

int m_video_prevstate;
static void M_Video_Classic_Key(cmd_state_t *cmd, int key, int ascii)
{
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
		if (vid_supportrefreshrate)
			Cvar_SetValueQuick(&vid_refreshrate, vid.refreshrate);
		Cvar_SetValueQuick(&vid_userefreshrate, vid.userefreshrate);

//		S_LocalSound ("sound/misc/menu1.wav");
		if (m_video_prevstate == m_video_nova)
			M_Menu_Video_Nova_f (cmd);
		else M_Menu_Options_Classic_f(cmd);
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else local_cursor = hotspotx_hover; // fall thru

		case K_ENTER:
			m_entersound = true;

			switch (local_cursor) {
				case (local_count - 1):
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
					M_Menu_Options_Classic_f(cmd);
					break;

				default:
					M_Menu_Video_AdjustSliders (1);
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

			M_Menu_Video_AdjustSliders (-1);
			break;

		case K_RIGHTARROW:
			M_Menu_Video_AdjustSliders (1);
			break;
	} // sw
}

#undef local_count
#undef local_cursor
