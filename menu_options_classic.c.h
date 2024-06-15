// menu_options_classic.c.h

// Frame cursor is set to a copy of the real cursor during draw
// and must be m_optcursor if it uses M_Menu_Options_AdjustSliders or other draw functions here
// because these functions check m_optcursor variable to see if mouse hit.
// Only 

#define		local_count		moc_count_32
#define		local_cursor	options_cursor
#define 	visiblerows 	m_optc_visiblerows


static int options_cursor;
int m_optc_visiblerows;

extern cvar_t crosshair_color_red;
extern cvar_t crosshair_color_green;
extern cvar_t crosshair_color_blue;
extern cvar_t crosshair_color_alpha;
extern cvar_t crosshair_size;

typedef enum {
	moc_customize_0				= 0,
	moc_console_1				= 1,
	moc_reset_2					= 2,
	moc_vidmode_3				= 3,
	moc_crosshair_4				= 4,
	moc_Mouse_Speed_5			= 5,
	moc_Invert_Mouse_6			= 6,
	moc_fov_7					= 7,
	moc_Always_Run_8			= 8,
	moc_Show_framerate_9		= 9,
	moc_Show_datetime_10		= 10,
	moc_cust_brite_11			= 11,
	moc_game_brite_12			= 12,
	moc_brite_13				= 13, // trast
	moc_gamma_14				= 14,
	moc_Sound_Volume_15			= 15,
	moc_Music_Volume_16			= 16,
	moc_Customize_Effects_17	= 17,
	moc_Effects_Quake_18		= 18,
	moc_Effects_Normal_19		= 19,
	moc_Effects_High_20			= 20,
	moc_Customize_Lighting_21	= 21,
	moc_Lighting_Flares_22		= 22,
	moc_Lighting_Normal_23		= 23,
	moc_Lighting_High_24		= 24,
	moc_Lighting_Full_25		= 25,
	moc_Browse_Mods_26			= 26,

	moc_crosshair_red_27		= 27,
	moc_crosshair_green_28		= 28,
	moc_crosshair_blue_29		= 29,
	moc_crosshair_alpha_30		= 30,
	moc_crosshair_size_31		= 31,
	moc_count_32				= 32,
} moc_e;

void M_Menu_Options_Classic_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_options_classic);
	m_entersound = true;
}

static void M_Menu_Options_AdjustSliders (int dir)
{
	double f;

	S_LocalSound ("sound/misc/menu3.wav");

	switch (local_cursor) {
	default:
	case_break moc_crosshair_4:			Cvar_SetValueQuick (&crosshair, bound(0, crosshair.integer + dir, 7));
	case_break moc_Mouse_Speed_5:		Cvar_SetValueQuick (&sensitivity, bound(1, sensitivity.value + dir * 0.5, 50));
	case_break moc_Invert_Mouse_6:		Cvar_SetValueQuick (&m_pitch, -m_pitch.value);
	case_break moc_fov_7:				Cvar_SetValueQuick (&scr_fov, bound(1, scr_fov.integer + dir * 1, 170));
	case_break moc_Always_Run_8:		{
											int spd = (cl_forwardspeed.value > 200) ? 200 : 400;
											Cvar_SetValueQuick (&cl_forwardspeed, spd);
											Cvar_SetValueQuick (&cl_backspeed, spd);
										}
	case_break moc_Show_framerate_9:	{	int c = 3; int x = get_showfps5_rot(); int j = rotc(c, x ,dir); set_showfps5 (j);}
	case_break moc_Show_datetime_10:	{f = !(showdate.integer && showtime.integer);Cvar_SetValueQuick(&showdate, f);Cvar_SetValueQuick(&showtime, f);}
	
	case_break moc_game_brite_12:		Cvar_SetValueQuick (&r_hdr_scenebrightness, bound(1, r_hdr_scenebrightness.value + dir * 0.0625, 4));
	case_break moc_brite_13:			Cvar_SetValueQuick (&v_contrast, bound(1, v_contrast.value + dir * 0.0625, 4));
	case_break moc_gamma_14:			Cvar_SetValueQuick (&v_gamma, bound(0.5, v_gamma.value + dir * 0.0625, 3));
	case_break moc_Sound_Volume_15:		Cvar_SetValueQuick (&volume, bound(0, volume.value + dir * 0.0625, 1));
	case_break moc_Music_Volume_16:		Cvar_SetValueQuick (&bgmvolume, bound(0, bgmvolume.value + dir * 0.0625, 1));

	case_break moc_crosshair_red_27:	Cvar_SetValueQuick (&crosshair_color_red, bound(0, crosshair_color_red.value + dir * 0.0625, 1));
	case_break moc_crosshair_green_28:	Cvar_SetValueQuick (&crosshair_color_green, bound(0, crosshair_color_green.value + dir * 0.0625, 1));
	case_break moc_crosshair_blue_29:	Cvar_SetValueQuick (&crosshair_color_blue, bound(0, crosshair_color_blue.value + dir * 0.0625, 1));
	case_break moc_crosshair_alpha_30:	Cvar_SetValueQuick (&crosshair_color_alpha, bound(0, crosshair_color_alpha.value + dir * 0.0625, 1));
	case_break moc_crosshair_size_31:	Cvar_SetValueQuick (&crosshair_size, bound(0.25, crosshair_size.value + dir * 0.25, 8));
	} // sw
}


static void M_Options_PrintCommand(const char *s, int enabled)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_button, q_force_scale_0); // PPX DUR
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor;  
		if (issel)
			drawsel_idx = drawidx; 

		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
	}

	drawcur_y += 8;
	drawidx++;
}

static void M_Options_PrintSS(const char *s, int enabled, const char *s2)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor; 
		if (issel)
			drawsel_idx = g_draw_frame_cursor;

		M_ItemPrint		(0 + 80,                           drawcur_y, s, enabled);
		M_ItemPrint		(0 + 80 + 8 + (int)strlen(s) * 8 + 8, drawcur_y, s2, enabled);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_Options_PrintCheckbox(const char *s, int enabled, int yes)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0); // PPX DUR
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor;
		if (issel)
			drawsel_idx = g_draw_frame_cursor; 
		
		M_ItemPrint		(0 + 80, drawcur_y, s, enabled);
		M_DrawCheckbox	(0 + 80 + 8 + (int)strlen(s) * 8 + 8, drawcur_y, yes);
	}

	drawcur_y += 8;
	drawidx++;
}

static void M_Options_PrintSlider(const char *s, int enabled, float value, float minvalue, float maxvalue)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, /*count*/ 1, hotspottype_slider, q_force_scale_0);
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor;
		if (issel) 	
			drawsel_idx = g_draw_frame_cursor;

		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
		M_DrawSlider(0 + 80 + 8 + (int)strlen(s) * 8 + 8, drawcur_y, value, minvalue, maxvalue);
	}

	drawcur_y += 8;
	drawidx++;
}

static void M_Options_Classic_Draw (void)
{
	M_Background(320, bound(200, 32 + local_count * 8, vid_conheight.integer), q_darken_true);
	PPX_Start (local_cursor);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	cachepic_t *p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, local_cursor - (visiblerows / 2), 
			max(0, local_count - visiblerows)) * 8;

	M_Options_PrintCommand( "    Customize controls", true);
	M_Options_PrintCommand( "         Go to console", true);
	M_Options_PrintCommand( "     Reset to defaults", true);
	M_Options_PrintCommand( "     Change Video Mode", true);
	M_Options_PrintSlider(  "             Crosshair", true, crosshair.value, 0, 7);
	M_Options_PrintSlider(  "           Mouse Speed", true, sensitivity.value, 1, 50);
	M_Options_PrintCheckbox("          Invert Mouse", true, m_pitch.value < 0);
	M_Options_PrintSlider(  "         Field of View", true, scr_fov.integer, 1, 170);
	M_Options_PrintCheckbox("            Always Run", true, cl_forwardspeed.value > 200);
	M_Options_PrintSS	   ("        Show Framerate", true, get_showfps5_text(get_showfps5_rot()) );
	M_Options_PrintCheckbox("    Show Date and Time", true, showdate.integer && showtime.integer);
	M_Options_PrintCommand( "     Custom Brightness", true);
	M_Options_PrintSlider(  "       Game Brightness", true, r_hdr_scenebrightness.value, 1, 4);
	M_Options_PrintSlider(  "            Brightness", true, v_contrast.value, 1, 2);
	M_Options_PrintSlider(  "                 Gamma", true, v_gamma.value, 0.5, 3);
	M_Options_PrintSlider(  "          Sound Volume", snd_initialized.integer, volume.value, 0, 1);
	M_Options_PrintSlider(  "          Music Volume", cdaudioinitialized.integer, bgmvolume.value, 0, 1);
	M_Options_PrintCommand( "     Customize Effects", true);
	M_Options_PrintCommand( "       Effects:  Quake", true);
	M_Options_PrintCommand( "       Effects: Normal", true);
	M_Options_PrintCommand( "       Effects:   High", true);
	M_Options_PrintCommand( "    Customize Lighting", true);
	M_Options_PrintCommand( "      Lighting: Flares", true);
	M_Options_PrintCommand( "      Lighting: Normal", true);
	M_Options_PrintCommand( "      Lighting:   High", true);
	M_Options_PrintCommand( "      Lighting:   Full", true);
	M_Options_PrintCommand( "           Browse Mods", true);
	M_Options_PrintSlider ( "         Crosshair Red", true, crosshair_color_red.value, 0, 1);
	M_Options_PrintSlider ( "       Crosshair Green", true, crosshair_color_green.value, 0, 1);
	M_Options_PrintSlider ( "        Crosshair Blue", true, crosshair_color_blue.value, 0, 1);
	M_Options_PrintSlider ( "       Crosshair Alpha", true, crosshair_color_alpha.value, 0, 1);
	M_Options_PrintSlider ( "        Crosshair Size", true, crosshair_size.value, 0.25, 8);

	PPX_DrawSel_End ();
}

static void M_Options_Classic_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: 
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		} 
		// fall thru

	case K_ESCAPE:						
		M_Menu_Options_Nova_f (cmd);
		break;

	case K_MOUSE1:						
		if (!Hotspots_DidHit () ) { return;	}  
		local_cursor = hotspotx_hover;
		// fall thru

	case K_ENTER:
		m_entersound = true;
		switch (local_cursor) {
		case moc_customize_0: 
			M_Menu_Keys_f(cmd); 
			break;

		case moc_console_1:
			menu_state_set_nova (m_none);
			KeyDest_Set (key_game); // key_dest = key_game;
 			// Baker: The idea here is to open the console
			Con_ToggleConsole ();
			break;

		case moc_reset_2: 
			M_Menu_Reset_f(cmd); 
			break;

		case moc_vidmode_3: 
			m_video_prevstate = m_options_classic; 
			M_Menu_Video_Classic_f(cmd); 
			break;

		case moc_cust_brite_11: 
			M_Menu_Options_ColorControl_f(cmd); 
			break;

		case moc_Customize_Effects_17:
			M_Menu_Options_Effects_f(cmd); 
			break;

		case moc_Effects_Quake_18: 
			Cbuf_AddTextLine(cmd, "cl_particles 1;cl_particles_quake 1;cl_particles_quality 1;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 0;cl_stainmaps_clearonload 1;cl_decals 0;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 0;cl_beams_polygons 0;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1"); 
			break;

		case moc_Effects_Normal_19: Cbuf_AddTextLine(cmd, "cl_particles 1;cl_particles_quake 0;cl_particles_quality 1;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 0;cl_stainmaps_clearonload 1;cl_decals 1;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 1;cl_beams_polygons 1;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1"); 
			break;

		case moc_Effects_High_20: Cbuf_AddTextLine(cmd, "cl_particles 1;cl_particles_quake 0;cl_particles_quality 2;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 1;cl_stainmaps_clearonload 1;cl_decals 1;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 1;cl_beams_polygons 1;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1");
			break;

		case moc_Customize_Lighting_21:
			M_Menu_Options_Graphics_f(cmd);
			break;

		case moc_Lighting_Flares_22: 
			Cbuf_AddTextLine(cmd, "r_coronas 1;gl_flashblend 1;r_shadow_gloss 0;r_shadow_realtime_dlight 0;r_shadow_realtime_dlight_shadows 0;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 0");
			break;

		case moc_Lighting_Normal_23: 
			Cbuf_AddTextLine(cmd, "r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 0;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 0");
			break;

		case moc_Lighting_High_24:
			Cbuf_AddTextLine(cmd, "r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 1;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 1");
			break;

		case moc_Lighting_Full_25: 
			Cbuf_AddTextLine(cmd, "r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 1;r_shadow_realtime_world 1;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 1"); 
			break;

		case moc_Browse_Mods_26: 
			M_Menu_ModList_f(cmd); 
			break;

		default: 
			M_Menu_Options_AdjustSliders (1); 
			break;
		} // sw
		break;

	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
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
		if (local_cursor > local_count - 1) 
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
		M_Menu_Options_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_AdjustSliders (1);
		break;
	} // sw
}

#undef local_count
#undef local_cursor
#undef visiblerows
