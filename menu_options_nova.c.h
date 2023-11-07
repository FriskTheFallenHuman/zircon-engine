// menu_options_nova.c.h

#define		msel_cursoric	m_optcursorx	// frame cursor
#define		m_local_cursor		options_cursorx


static int m_optcursorx;
static int options_cursorx;

#include "menu_options_nova_extra.c.h"

typedef enum {
	moa_Key_Setup_0				= 0,
	moa_console_1				= 1,
	moa_Scale_2					= 2,
	moa_Console_Scale_3			= 3,
	
	moa_Gamma_4					= 4,
	moa_Contrast_5				= 5,
	moa_Mouse_Speed_6			= 6,
	moa_Sound_Volume_7			= 7,
	moa_Music_Volume_8			= 8,
	moa_Always_Run_9			= 9,
	moa_Invert_Mouse_10			= 10,
	moa_Show_framerate_11		= 11,
	moa_Status_Bar_12			= 12,
	moa_Gun_Position_13			= 13,
	moa_Bobbing_14				= 14,
	moa_SharpText_15			= 15,

	moa_Effects_16				= 16,
	
	moa_Video_Mode_17			= 17,
	moa_Reset_to_defaults_18	= 18,
	moa_Classic_Menu_19			= 19,
	moa_count_20				= 20,
} moa_e;



void M_Menu_OptionsNova_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_options);
	m_entersound = true;
}





		
static void M_Menu_OptionsNova_AdjustSliders (int dir)
{
	
	cvar_t *pcvar;

	S_LocalSound ("sound/misc/menu3.wav");

	
	switch (m_local_cursor) {
	default:
	case_break moa_Key_Setup_0:				
	case_break moa_console_1:				
	case_break moa_Scale_2:				/*cvar_t */ pcvar  = vid.fullscreen ? &vid_fullscreen_conscale : &vid_window_conscale;
										Cvar_SetValueQuick (pcvar, bound( 0.5, pcvar->value + dir * (1 / 16.0) , 2) );
	case_break moa_Console_Scale_3:		Cvar_SetValueQuick (&con_textsize, bound(2, con_textsize.value + dir * 2, 64));
	
	case_break moa_Gamma_4:				Cvar_SetValueQuick (&v_gamma, bound(0.5, v_gamma.value + dir * 0.0625, 3));
	case_break moa_Contrast_5:			Cvar_SetValueQuick (&v_contrast, bound(0.5, v_contrast.value + dir * 0.0625, 4));
	case_break moa_Mouse_Speed_6:		Cvar_SetValueQuick (&sensitivity, bound(1, sensitivity.value + dir * 0.5, 50));
	case_break moa_Sound_Volume_7:		Cvar_SetValueQuick (&volume, bound(0, volume.value + dir * 0.0625, 1));
	case_break moa_Music_Volume_8:		Cvar_SetValueQuick (&bgmvolume, bound(0, bgmvolume.value + dir * 0.0625, 1));
	
	case_break moa_Always_Run_9:		{int spd = (cl_forwardspeed.value > 200) ? 200 : 400;
										Cvar_SetValueQuick (&cl_forwardspeed, spd);
										Cvar_SetValueQuick (&cl_backspeed, spd);}
										

	case_break moa_Invert_Mouse_10:		Cvar_SetValueQuick (&m_pitch, -m_pitch.value);
	case_break moa_Show_framerate_11:	{	int c = 3; int x = getef5(); int j = rotc(c, x ,dir); setlevel5 (j);}
	case_break moa_Status_Bar_12:		{	int c = 3; int x = getef4(); int j = rotc(c, x ,dir); setlevel4 (j);}
	case_break moa_Gun_Position_13:		{	int c = 2; int x = getef3(); int j = rotc(c, x ,dir); setlevel3 (j);}
	case_break moa_Bobbing_14:			{   int c = 3; int x = getef2(); int j = rotc(c, x ,dir); setlevel2 (j);}
	case_break moa_SharpText_15:		Cvar_SetValueQuick (&r_nearest_conchars, !r_nearest_conchars.value);
	
	case_break moa_Effects_16:			{	int c = 6; int x = getef();  int j = rotc(c, x ,dir); setlevel (j);}
	
	case_break moa_Video_Mode_17:
	case_break moa_Reset_to_defaults_18:
	case_break moa_Classic_Menu_19:		break;
	} // sw

}	


static void M_OptionsNova_PrintCommand(const char *s, int enabled)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_button); // PPX DUR
	if (drawcur_y >= 32) {
		int issel = drawidx == msel_cursoric; if (issel) { drawsel_idx = msel_cursoric;	 }
		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
	}
	
	drawcur_y += 8;
	drawidx++;
}


static void M_OptionsNova_PrintSS(const char *s, int enabled, const char *s2)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider);
	if (drawcur_y >= 32) {
		int issel = drawidx == msel_cursoric; if (issel) { drawsel_idx = msel_cursoric;	 }
		M_ItemPrint(0 + 80,                          drawcur_y, s, enabled);
		M_ItemPrint(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, s2, enabled);
	}
	
	drawcur_y += 8;
	drawidx++;
}


static void M_OptionsNova_PrintCheckbox(const char *s, int enabled, int yes)
{
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, 1, hotspottype_slider);
	if (drawcur_y >= 32) {
		int issel = drawidx == msel_cursoric; if (issel) { drawsel_idx = msel_cursoric;	}
		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
		M_DrawCheckbox(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, yes);
	}
	
	drawcur_y += 8;
	drawidx++;
}

static void M_OptionsNova_PrintSlider(const char *s, int enabled, float value, float minvalue, float maxvalue)
{ 
	Hotspots_Add (menu_x + 80 - 16, menu_y + drawcur_y, 320, 8 + 1, /*count*/ 1, hotspottype_slider);
	if (drawcur_y >= 32) {
		int issel = drawidx == msel_cursoric;  if (issel) { drawsel_idx = msel_cursoric; }	 // PPX 2.2
		M_ItemPrint(0 + 80, drawcur_y, s, enabled);
		M_DrawSlider(0 + 80 + (int)strlen(s) * 8 + 8, drawcur_y, value, minvalue, maxvalue);
	}
	
	drawcur_y += 8;
	drawidx++;
}

WARP_X_ (M_OptionsNova_Key)
WARP_X_CALLERS_ (M_Menu_Help_f etc)

int visiblerows06;
#define visiblerows visiblerows06


static void M_OptionsNova_Draw (void) // 
{
	//int visiblerows;
	char vabuf[1024];
	cachepic_t	*p0;

	M_Background(320, bound(200, 32 + moa_count_20 * 8, vid_conheight.integer));

	M_DrawPic(16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-p0->width)/2, 4, p0 /*"gfx/p_option"*/, NO_HOTSPOTS_0, NA0, NA0);

	PPX_Start(/*realcursor*/ m_local_cursor, /*frame select draw cursor might be changed*/ msel_cursoric); // PPX FRAME

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, msel_cursoric - (visiblerows >> 1), max(0, moa_count_20 - visiblerows)) * 8;

	cvar_t *pcvar;
	pcvar = vid.fullscreen ? &vid_fullscreen_conscale : &vid_window_conscale;

	M_OptionsNova_PrintCommand	("   Keyboard Setup ", true);											// 0
	M_OptionsNova_PrintCommand	("    Go to console ", true);											// 1
	M_OptionsNova_PrintSlider	("         2D Scale ", true, pcvar->value, 0.5, 2.0);					// 2
	M_OptionsNova_PrintSlider	("    Console Scale ", true, con_textsize.value, 2, 64);				// 3
	drawcur_y += 8;
	M_OptionsNova_PrintSlider	("            Gamma ", true, v_gamma.value, 0.5, 3);					// 5
	M_OptionsNova_PrintSlider	("         Contrast ", true, v_contrast.value, 0.5, 2);					// 6
	M_OptionsNova_PrintSlider	("      Mouse Speed ", true, sensitivity.value, 1, 50);					// 7
	M_OptionsNova_PrintSlider	("     Sound Volume ", snd_initialized.integer, volume.value, 0, 1);	// 8
	M_OptionsNova_PrintSlider	("     Music Volume ", cdaudioinitialized.integer, bgmvolume.value, 0, 1); // 9
	M_OptionsNova_PrintCheckbox	("       Always Run ", true, cl_forwardspeed.value > 200);				// 10
	M_OptionsNova_PrintCheckbox	("     Invert Mouse ", true, m_pitch.value < 0);						// 11
	M_OptionsNova_PrintSS		("   Show Framerate ", true, ef5(getef5()) );							// 12
	drawcur_y += 8;

	M_OptionsNova_PrintSS		("       Status Bar ", true, ef4(getef4()) ) ;							// 13
	M_OptionsNova_PrintSS		("     Gun Position ", true, ef3(getef3()) ) ;							// 14
	M_OptionsNova_PrintSS		("          Bobbing ", true, ef2(getef2()) ) ;							// 15
	M_OptionsNova_PrintCheckbox ("       Sharp Text ", true, !!r_nearest_conchars.value);				// 17
	drawcur_y += 8;
	M_OptionsNova_PrintSS       ("          Effects ", true, ef(getef()));								// 16
	drawcur_y += 8;
	M_OptionsNova_PrintCommand	(va(vabuf, sizeof(vabuf), "Change Video Mode %d x %d", vid.width, vid.height), true); // 18
	drawcur_y += 8;
	M_OptionsNova_PrintCommand	("Reset to defaults ", true);											// 19
	M_OptionsNova_PrintCommand	("     Classic Menu ", true);											// 20

	WARP_X_ (M_Draw)
	PPX_DrawSel_End ();
}

WARP_X_ (M_Draw)

static void M_OptionsNova_Key (int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { m_local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
		// PPX Key1
	case K_ESCAPE:	
	
		M_Menu_Main_f ();
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit () ) { return;	}  m_local_cursor = hotspotx_hover; // PPX Key2 fall thru

	case K_ENTER:	
		m_entersound = true;
		switch (m_local_cursor) {
		case moa_Key_Setup_0:			M_Menu_Keys_f ();	break;

		case moa_console_1:				menu_state_set_nova (m_none);
										key_dest = key_game;
										Con_ToggleConsole_f ();
										break;

		case moa_Video_Mode_17:			M_Menu_VideoNova_f ();	break;
		
		case moa_Reset_to_defaults_18:	M_Menu_Reset_f ();	break;
		case moa_Classic_Menu_19:		M_Menu_Options_Classic_f ();	break;
						
		default:
			M_Menu_OptionsNova_AdjustSliders (1);
			break;
		} // sw
		return;

	case K_HOME: 
		m_local_cursor = 0;
		break;

	case K_END:
		m_local_cursor = moa_count_20 - 1;
		break;

	case K_PGUP:
		m_local_cursor -= visiblerows / 2;
		if (m_local_cursor < 0) m_local_cursor = 0;
		break;

	case K_MWHEELUP:
		m_local_cursor -= visiblerows / 4;
		if (m_local_cursor < 0) m_local_cursor = 0;
		break;

	case K_PGDN:
		m_local_cursor += visiblerows / 2;
		if (m_local_cursor > moa_count_20 - 1) m_local_cursor = moa_count_20 - 1;
		break;

	case K_MWHEELDOWN:
		m_local_cursor += visiblerows / 4;
		if (m_local_cursor > moa_count_20 - 1) m_local_cursor = moa_count_20 - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		m_local_cursor--;
		if (m_local_cursor < 0)
			m_local_cursor = moa_count_20 - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		m_local_cursor++;
		if (m_local_cursor >= moa_count_20)
			m_local_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		M_Menu_OptionsNova_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_OptionsNova_AdjustSliders (1);
		break;
	}
}


#undef msel_cursoric
#undef m_local_cursor
#undef visiblerows
