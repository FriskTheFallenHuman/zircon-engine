// menu_color.c.h - 100%

#define		msel_cursoric	m_optcursor		// frame cursor
#define		m_local_cursor		options_colorcontrol_cursor


#define	OPTIONS_COLORCONTROL_ITEMS	18

static int		options_colorcontrol_cursor;

// intensity value to match up to 50% dither to 'correct' quake


void M_Menu_Options_ColorControl_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_options_colorcontrol);
	m_entersound = true;
}


static void M_Menu_Options_ColorControl_AdjustSliders (int dir)
{
	int optnum;
	float f;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 1;
	if (options_colorcontrol_cursor == optnum++)
		Cvar_SetValueQuick (&v_hwgamma, !v_hwgamma.integer);
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_gamma, bound(1, v_gamma.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_contrast, bound(1, v_contrast.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_brightness, bound(0, v_brightness.value + dir * 0.05, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, !v_color_enable.integer);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_r, bound(0, v_color_black_r.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_g, bound(0, v_color_black_g.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_b, bound(0, v_color_black_b.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3 + dir * 0.0125, 0.8);
		Cvar_SetValueQuick (&v_color_black_r, f);
		Cvar_SetValueQuick (&v_color_black_g, f);
		Cvar_SetValueQuick (&v_color_black_b, f);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_r, bound(0, v_color_grey_r.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_g, bound(0, v_color_grey_g.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_b, bound(0, v_color_grey_b.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3 + dir * 0.0125, 0.95);
		Cvar_SetValueQuick (&v_color_grey_r, f);
		Cvar_SetValueQuick (&v_color_grey_g, f);
		Cvar_SetValueQuick (&v_color_grey_b, f);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_r, bound(1, v_color_white_r.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_g, bound(1, v_color_white_g.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_b, bound(1, v_color_white_b.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(1, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3 + dir * 0.125, 5);
		Cvar_SetValueQuick (&v_color_white_r, f);
		Cvar_SetValueQuick (&v_color_white_g, f);
		Cvar_SetValueQuick (&v_color_white_b, f);
	}
}

int visiblerows02;
#define visiblerows visiblerows02


static void M_Options_ColorControl_Draw (void)
{
	//int visiblerows;
	float x, c, s, t, u, v;
	cachepic_t	*p0, *dither;

	PPX_Start(/*realcursor*/ m_local_cursor, /*frame select draw cursor might be changed*/ msel_cursoric); // PPX FRAME

	dither = Draw_CachePic_Flags ("gfx/colorcontrol/ditherpattern", CACHEPICFLAG_NOCLAMP);

	M_Background(320, 256);

	M_DrawPic(16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-p0->width)/2, 4, p0 /*"gfx/p_option"*/, NO_HOTSPOTS_0, NA0, NA0);

	
	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, m_local_cursor - (visiblerows >> 1), max(0, OPTIONS_COLORCONTROL_ITEMS - visiblerows)) * 8;
	//drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Options_PrintCommand( "     Reset to defaults", true);
	M_Options_PrintCheckbox("Hardware Gamma Control", vid_hardwaregammasupported.integer, v_hwgamma.integer);
	M_Options_PrintSlider(  "                 Gamma", !v_color_enable.integer && vid_hardwaregammasupported.integer && v_hwgamma.integer, v_gamma.value, 1, 5);
	M_Options_PrintSlider(  "              Contrast", !v_color_enable.integer, v_contrast.value, 1, 5);
	M_Options_PrintSlider(  "            Brightness", !v_color_enable.integer, v_brightness.value, 0, 0.8);
	M_Options_PrintCheckbox("  Color Level Controls", true, v_color_enable.integer);
	M_Options_PrintSlider(  "          Black: Red  ", v_color_enable.integer, v_color_black_r.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Green", v_color_enable.integer, v_color_black_g.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Blue ", v_color_enable.integer, v_color_black_b.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Grey ", v_color_enable.integer, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3, 0, 0.8);
	M_Options_PrintSlider(  "           Grey: Red  ", v_color_enable.integer && vid_hardwaregammasupported.integer && v_hwgamma.integer, v_color_grey_r.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Green", v_color_enable.integer && vid_hardwaregammasupported.integer && v_hwgamma.integer, v_color_grey_g.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Blue ", v_color_enable.integer && vid_hardwaregammasupported.integer && v_hwgamma.integer, v_color_grey_b.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Grey ", v_color_enable.integer && vid_hardwaregammasupported.integer && v_hwgamma.integer, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3, 0, 0.95);
	M_Options_PrintSlider(  "          White: Red  ", v_color_enable.integer, v_color_white_r.value, 1, 5);
	M_Options_PrintSlider(  "          White: Green", v_color_enable.integer, v_color_white_g.value, 1, 5);
	M_Options_PrintSlider(  "          White: Blue ", v_color_enable.integer, v_color_white_b.value, 1, 5);
	M_Options_PrintSlider(  "          White: Grey ", v_color_enable.integer, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3, 1, 5);

	drawcur_y += 4;
	DrawQ_Fill(menu_x, menu_y + drawcur_y, 320, 4 + 64 + 8 + 64 + 4, 0, 0, 0, 1, 0);drawcur_y += 4;
	s = (float) 312 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 4 / 2 * vid.height / vid_conheight.integer;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,0,0,1, 0,1, 0,0,0,1, 1,1, 1,0,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,1,0,1, 0,1, 0,0,0,1, 1,1, 0,1,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,0,1,1, 0,1, 0,0,0,1, 1,1, 0,0,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,1,1,1, 0,1, 0,0,0,1, 1,1, 1,1,1,1, 0);drawcur_y += 4;

	c = menu_options_colorcontrol_correctionvalue.value; // intensity value that should be matched up to a 50% dither to 'correct' quake
	s = (float) 48 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 48 / 2 * vid.height / vid_conheight.integer;
	u = s * 0.5;
	v = t * 0.5;
	drawcur_y += 8;
	x = 4;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, c, 0, 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,0,0,1, u,0, 1,0,0,1, 0,v, 1,0,0,1, u,v, 1,0,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, 0, c, 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,1,0,1, u,0, 0,1,0,1, 0,v, 0,1,0,1, u,v, 0,1,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, 0, 0, c, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,0,1,1, u,0, 0,0,1,1, 0,v, 0,0,1,1, u,v, 0,0,1,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, c, c, c, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,1,1,1, u,0, 1,1,1,1, 0,v, 1,1,1,1, u,v, 1,1,1,1, 0);

	PPX_DrawSel_End ();
}


static void M_Options_ColorControl_Key (int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { m_local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		M_Menu_Options_Classic_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else options_colorcontrol_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;
		switch (options_colorcontrol_cursor)
		{
		case 0:
			Cvar_SetValueQuick(&v_hwgamma, 1);
			Cvar_SetValueQuick(&v_gamma, 1);
			Cvar_SetValueQuick(&v_contrast, 1);
			Cvar_SetValueQuick(&v_brightness, 0);
			Cvar_SetValueQuick(&v_color_enable, 0);
			Cvar_SetValueQuick(&v_color_black_r, 0);
			Cvar_SetValueQuick(&v_color_black_g, 0);
			Cvar_SetValueQuick(&v_color_black_b, 0);
			Cvar_SetValueQuick(&v_color_grey_r, 0);
			Cvar_SetValueQuick(&v_color_grey_g, 0);
			Cvar_SetValueQuick(&v_color_grey_b, 0);
			Cvar_SetValueQuick(&v_color_white_r, 1);
			Cvar_SetValueQuick(&v_color_white_g, 1);
			Cvar_SetValueQuick(&v_color_white_b, 1);
			break;
		default:
			M_Menu_Options_ColorControl_AdjustSliders (1);
			break;
		}
		return;

	case K_HOME: 
		options_colorcontrol_cursor = 0;
		break;

	case K_END:
		options_colorcontrol_cursor = OPTIONS_COLORCONTROL_ITEMS - 1;
		break;

	case K_PGUP:
		options_colorcontrol_cursor -= visiblerows / 2;
		if (options_colorcontrol_cursor < 0) options_colorcontrol_cursor = 0;
		break;

	case K_MWHEELUP:
		options_colorcontrol_cursor -= visiblerows / 4;
		if (options_colorcontrol_cursor < 0) options_colorcontrol_cursor = 0;
		break;

	case K_PGDN:
		options_colorcontrol_cursor += visiblerows / 2;
		if (options_colorcontrol_cursor > OPTIONS_COLORCONTROL_ITEMS - 1) options_colorcontrol_cursor = OPTIONS_COLORCONTROL_ITEMS - 1;
		break;

	case K_MWHEELDOWN:
		options_colorcontrol_cursor += visiblerows / 4;
		if (options_colorcontrol_cursor > OPTIONS_COLORCONTROL_ITEMS - 1) options_colorcontrol_cursor = OPTIONS_COLORCONTROL_ITEMS - 1;
		break;


	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		options_colorcontrol_cursor--;
		if (options_colorcontrol_cursor < 0)
			options_colorcontrol_cursor = OPTIONS_COLORCONTROL_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		options_colorcontrol_cursor++;
		if (options_colorcontrol_cursor >= OPTIONS_COLORCONTROL_ITEMS)
			options_colorcontrol_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		M_Menu_Options_ColorControl_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_ColorControl_AdjustSliders (1);
		break;
	}
}

#undef msel_cursoric
#undef m_local_cursor
#undef visiblerows