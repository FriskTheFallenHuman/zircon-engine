// menu_options_nova.c.h

#define		frame_cursor	m_optcursorx
#define		local_count		moa_count_22
#define		local_cursor	options_cursorx
#define 	visiblerows 	m_optn_visiblerows

extern cvar_t con_textsize;
static int options_cursorx;

int m_optn_visiblerows;

#include "menu_options_nova_extra.c.h"

typedef enum {
	moa_Key_Setup_0				= 0,
	moa_console_1				= 1,
	moa_Scale_2					= 2,
	moa_Console_Scale_3			= 3,
	
	moa_Scene_Brightness_4		= 4,
	moa_Gamma_5					= 5,
	moa_Contrast_6				= 6,
	moa_Mouse_Speed_7			= 7,
	moa_Sound_Volume_8			= 8,
	moa_Music_Volume_9			= 9,
	moa_Always_Run_10			= 10,
	moa_Invert_Mouse_11			= 11,
	moa_Show_framerate_12		= 12,
	moa_Status_Bar_13			= 13,
	moa_Gun_Position_14			= 14,
	moa_Bobbing_15				= 15,
	moa_OverbrightModels_16		= 16,
	moa_SharpText_17			= 17,

	moa_Effects_18				= 18,
	
	moa_Video_Mode_19			= 19,
	moa_Reset_to_defaults_20	= 20,
	moa_Classic_Menu_21			= 21,
	moa_count_22/*local_count*/	= 22,
} moa_e;



void M_Menu_Options_Nova_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_options_nova);
	m_entersound = true;
}

static void M_Menu_OptionsNova_AdjustSliders (int dir)
{
	cvar_t *pcvar;

	S_LocalSound ("sound/misc/menu3.wav");

	switch (local_cursor) {
	default:
	case_break moa_Key_Setup_0:				
	case_break moa_console_1:				
	case_break moa_Scale_2:				/*cvar_t */ pcvar  = vid.fullscreen ? &vid_fullscreen_conscale : &vid_window_conscale;
										Cvar_SetValueQuick (pcvar, bound( 0.5, pcvar->value + dir * (1 / 16.0) , 2) );
	case_break moa_Console_Scale_3:
		Cvar_SetValueQuick (&con_textsize, bound(2, con_textsize.value + dir * 2, 32));
	
	case_break moa_Scene_Brightness_4:  Cvar_SetValueQuick (&r_hdr_scenebrightness, bound(0.5, r_hdr_scenebrightness.value + dir * 0.0625, 3));
	case_break moa_Gamma_5:				Cvar_SetValueQuick (&v_gamma, bound(0.5, v_gamma.value + dir * 0.0625, 3));
	case_break moa_Contrast_6:			Cvar_SetValueQuick (&v_contrast, bound(0.5, v_contrast.value + dir * 0.0625, 4));
	case_break moa_Mouse_Speed_7:		Cvar_SetValueQuick (&sensitivity, bound(1, sensitivity.value + dir * 0.5, 50));
	case_break moa_Sound_Volume_8:		Cvar_SetValueQuick (&volume, bound(0, volume.value + dir * 0.0625, 1));
	case_break moa_Music_Volume_9:		Cvar_SetValueQuick (&bgmvolume, bound(0, bgmvolume.value + dir * 0.0625, 1));
	
	case_break moa_Always_Run_10:		{int spd = (cl_forwardspeed.value > 200) ? 200 : 400;
										Cvar_SetValueQuick (&cl_forwardspeed, spd);
										Cvar_SetValueQuick (&cl_backspeed, spd);}
										

	case_break moa_Invert_Mouse_11:		Cvar_SetValueQuick (&m_pitch, -m_pitch.value);
	case_break moa_Show_framerate_12:	{	int c = 3; int x = get_showfps5_rot(); int j = rotc(c, x ,dir); set_showfps5 (j);}
	case_break moa_Status_Bar_13:		{	int c = 3; int x = get_statusbar4_rot(); int j = rotc(c, x ,dir); set_statusbar4 (j);}
	case_break moa_Gun_Position_14:		{	int c = 2; int x = get_gunpos3_rot(); int j = rotc(c, x ,dir); set_gunpos3 (j);}
	case_break moa_Bobbing_15:			{   int c = 3; int x = get_bobbing2_rot(); int j = rotc(c, x ,dir); set_bobbing2 (j);}
	case_break moa_OverbrightModels_16:	{   int c = 3; int x = get_overbright4_rot(); int j = rotc(c, x ,dir); set_overbright4 (j);}
	case_break moa_SharpText_17:		Cvar_SetValueQuick (&r_nearest_conchars, !r_nearest_conchars.value);
	
	case_break moa_Effects_18:			{	int c = 6; int x = get_effects0_rot();  int j = rotc(c, x ,dir); set_effects0 (j);}
	
	case_break moa_Video_Mode_19:
	case_break moa_Reset_to_defaults_20:
	case_break moa_Classic_Menu_21:		break;
	} // sw
}	


static void M_OptionsNova_PrintCommand(const char *s, int enabled)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_button, q_force_scale_0); // PPX DUR
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor; 
		if (issel) { 
			drawsel_idx = g_draw_frame_cursor;	 
		}
		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_OptionsNova_PrintSS(const char *s, int enabled, const char *s2)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor; 
		if (issel)
			drawsel_idx = g_draw_frame_cursor;	 
		
		M_ItemPrint(0 + 80,                          drawcur_y, s, enabled);
		M_ItemPrint(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, s2, enabled);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_OptionsNova_PrintCheckbox(const char *s, int enabled, int yes)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0);
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor;
		if (issel) 
			drawsel_idx = g_draw_frame_cursor;

		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
		M_DrawCheckbox(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, yes);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_OptionsNova_PrintSlider(const char *s, int enabled, float value, float minvalue, float maxvalue)
{ 
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, /*count*/ 1, hotspottype_slider, q_force_scale_0);
	if (drawcur_y >= 32) {
		int issel = drawidx == g_draw_frame_cursor;  
		if (issel) 
			drawsel_idx = g_draw_frame_cursor;
		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
		M_DrawSlider(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, value, minvalue, maxvalue);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_Options_Nova_Draw (void) 
{
	char vabuf[1024];
	int calcy = 200;//32 + local_count * 8; // Baker: if this exceeds 200, Quake pic shifts down a little
	M_Background(320, bound(200, calcy, vid_conheight.integer), q_darken_true);
	PPX_Start (local_cursor);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	cachepic_t *p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	visiblerows = (int)((menu_height - 32) / 8) + 1; // Baker: Increased by 1 to prevent scrolling
	drawcur_y = 32 - bound(0, local_cursor - (visiblerows / 2), 
			max(0, local_count - visiblerows)) * 8;

	cvar_t *pcvar;
	pcvar = vid.fullscreen ? &vid_fullscreen_conscale : &vid_window_conscale;

	M_OptionsNova_PrintCommand	("   Keyboard Setup ", true);											// 0
	M_OptionsNova_PrintCommand	("    Go to console ", true);											// 1
	M_OptionsNova_PrintSlider	("         2D Scale ", true, pcvar->value, 0.5, 2.0);					// 2
	M_OptionsNova_PrintSlider	("    Console Scale ", true, con_textsize.value / 8, 0.125, 4);			// 3
	drawcur_y += 8;
	M_OptionsNova_PrintSlider	(" Scene Brightness ", true, r_hdr_scenebrightness.value, 0.5, 3);		// 4			// 5
	M_OptionsNova_PrintSlider	("            Gamma ", true, v_gamma.value, 0.5, 3);					// 5
	M_OptionsNova_PrintSlider	("         Contrast ", true, v_contrast.value, 0.5, 2);					// 6
	M_OptionsNova_PrintSlider	("      Mouse Speed ", true, sensitivity.value, 1, 50);					// 7
	M_OptionsNova_PrintSlider	("     Sound Volume ", snd_initialized.integer, volume.value, 0, 1);	// 8
	M_OptionsNova_PrintSlider	("     Music Volume ", cdaudioinitialized.integer, bgmvolume.value, 0, 1); // 9
	M_OptionsNova_PrintCheckbox	("       Always Run ", true, cl_forwardspeed.value > 200);				// 10
	M_OptionsNova_PrintCheckbox	("     Invert Mouse ", true, m_pitch.value < 0);						// 11
	M_OptionsNova_PrintSS		("   Show Framerate ", true, get_showfps5_text(get_showfps5_rot()) );							// 12
	drawcur_y += 8; // Baker 20240404 - if we do this ...?

	M_OptionsNova_PrintSS		("       Status Bar ", true, get_statusbar4_text(get_statusbar4_rot()) ) ;							// 13
	M_OptionsNova_PrintSS		("     Gun Position ", true, get_gunpos3_text(get_gunpos3_rot()) ) ;							// 14
	M_OptionsNova_PrintSS		("          Bobbing ", true, get_bobbing2_text(get_bobbing2_rot()) ) ;							// 15
	M_OptionsNova_PrintSS		("Overbright Models ", true, get_overbright4_text(get_overbright4_rot()) ) ;							// 15
	M_OptionsNova_PrintCheckbox ("       Sharp Text ", true, !!r_nearest_conchars.value);				// 17
	drawcur_y += 8;
	M_OptionsNova_PrintSS       ("          Effects ", true, get_effects0_text(get_effects0_rot()));								// 16
	drawcur_y += 8;
	M_OptionsNova_PrintCommand	(va(vabuf, sizeof(vabuf), "Change Video Mode %d x %d", vid.width, vid.height), true); // 18
	drawcur_y += 8;
	M_OptionsNova_PrintCommand	("Reset to defaults ", true);											// 19
	M_OptionsNova_PrintCommand	("     Classic Menu ", true);											// 20

	PPX_DrawSel_End ();
}

static void M_Options_Nova_Key (cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: 
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		}
		// fall thru

	case K_ESCAPE:
		M_Menu_Main_f (cmd);
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit () ) { return;	}  
		local_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:	
		m_entersound = true;
		switch (local_cursor) {
		case moa_Key_Setup_0:
			M_Menu_Keys_f (cmd);	
			break;

		case moa_console_1:
			menu_state_set_nova (m_none);
			KeyDest_Set (key_game); // key_dest = key_game;
			// Baker: The idea here is to open the console
			Con_ToggleConsole ();
			break;

		case moa_Video_Mode_19:
			M_Menu_Video_Nova_f (cmd);
			break;
		
		case moa_Reset_to_defaults_20:
			M_Menu_Reset_f (cmd);
			break;

		case moa_Classic_Menu_21:
			M_Menu_Options_Classic_f (cmd);
			break;
						
		default:
			M_Menu_OptionsNova_AdjustSliders (1);
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
		M_Menu_OptionsNova_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_OptionsNova_AdjustSliders (1);
		break;
	} // sw
}

#undef local_count
#undef local_cursor
#undef visiblerows
