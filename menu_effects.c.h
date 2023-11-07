// menu_effects.c.h - 100%

#define		msel_cursoric	m_optcursor		// frame cursor
#define		m_local_cursor		options_effects_cursor

extern cvar_t r_textshadow;
extern cvar_t r_hdr_scenebrightness;

#define	OPTIONS_EFFECTS_ITEMS	36

static int options_effects_cursor;

void M_Menu_Options_Effects_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_options_effects);
	m_entersound = true;
}


extern cvar_t cl_stainmaps;
extern cvar_t cl_stainmaps_clearonload;
extern cvar_t r_explosionclip;
extern cvar_t r_coronas;
extern cvar_t gl_flashblend;
extern cvar_t cl_beams_polygons;
extern cvar_t cl_beams_quakepositionhack;
extern cvar_t cl_beams_instantaimhack;
extern cvar_t cl_beams_lightatend;
extern cvar_t r_lightningbeam_thickness;
extern cvar_t r_lightningbeam_scroll;
extern cvar_t r_lightningbeam_repeatdistance;
extern cvar_t r_lightningbeam_color_red;
extern cvar_t r_lightningbeam_color_green;
extern cvar_t r_lightningbeam_color_blue;
extern cvar_t r_lightningbeam_qmbtexture;

static void M_Menu_Options_Effects_AdjustSliders (int dir)
{
	int optnum;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 0;
	     if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles, !cl_particles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_quake, !cl_particles_quake.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_quality, bound(1, cl_particles_quality.value + dir * 0.5, 4));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_explosions_shell, !cl_particles_explosions_shell.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_explosionclip, !r_explosionclip.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_stainmaps, !cl_stainmaps.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_stainmaps_clearonload, !cl_stainmaps_clearonload.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_decals, !cl_decals.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_bulletimpacts, !cl_particles_bulletimpacts.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_smoke, !cl_particles_smoke.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_sparks, !cl_particles_sparks.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_bubbles, !cl_particles_bubbles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood, !cl_particles_blood.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood_alpha, bound(0.2, cl_particles_blood_alpha.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood_bloodhack, !cl_particles_blood_bloodhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_polygons, !cl_beams_polygons.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_instantaimhack, !cl_beams_instantaimhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_quakepositionhack, !cl_beams_quakepositionhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_lightatend, !cl_beams_lightatend.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_thickness, bound(1, r_lightningbeam_thickness.integer + dir, 10));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_scroll, bound(0, r_lightningbeam_scroll.integer + dir, 10));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_repeatdistance, bound(64, r_lightningbeam_repeatdistance.integer + dir * 64, 1024));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_red, bound(0, r_lightningbeam_color_red.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_green, bound(0, r_lightningbeam_color_green.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_blue, bound(0, r_lightningbeam_color_blue.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_qmbtexture, !r_lightningbeam_qmbtexture.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerpmodels, !r_lerpmodels.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerpsprites, !r_lerpsprites.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerplightstyles, !r_lerplightstyles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&gl_polyblend, bound(0, gl_polyblend.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_skyscroll1, bound(-8, r_skyscroll1.value + dir * 0.1, 8));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_skyscroll2, bound(-8, r_skyscroll2.value + dir * 0.1, 8));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_waterwarp, bound(0, (int)(r_waterwarp.value + dir) , 2));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_wateralpha, bound(0, r_wateralpha.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_waterdeform, bound(0, (int)(r_waterdeform.value + dir) , 2));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_waterscroll, bound(0, r_waterscroll.value + dir * 0.5, 10));
}

int visiblerows03;
#define visiblerows visiblerows03


static void M_Options_Effects_Draw (void)
{
	//int visiblerows;
	cachepic_t	*p0;

	M_Background(320, bound(200, 32 + OPTIONS_EFFECTS_ITEMS * 8, vid_conheight.integer));

	M_DrawPic(16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-p0->width)/2, 4, p0 /*"gfx/p_option"*/, NO_HOTSPOTS_0, NA0, NA0);

	PPX_Start(/*realcursor*/ m_local_cursor, /*frame select draw cursor might be changed*/ msel_cursoric); // PPX FRAME

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, m_optcursor - (visiblerows >> 1), max(0, OPTIONS_EFFECTS_ITEMS - visiblerows)) * 8;
	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Options_PrintCheckbox("             Particles", true, cl_particles.integer);
	M_Options_PrintCheckbox(" Quake-style Particles", true, cl_particles_quake.integer);
	M_Options_PrintSlider(  "     Particles Quality", true, cl_particles_quality.value, 1, 4);
	M_Options_PrintCheckbox("       Explosion Shell", true, cl_particles_explosions_shell.integer);
	M_Options_PrintCheckbox("  Explosion Shell Clip", true, r_explosionclip.integer);
	M_Options_PrintCheckbox("             Stainmaps", true, cl_stainmaps.integer);
	M_Options_PrintCheckbox("Onload Clear Stainmaps", true, cl_stainmaps_clearonload.integer);
	M_Options_PrintCheckbox("                Decals", true, cl_decals.integer);
	M_Options_PrintCheckbox("        Bullet Impacts", true, cl_particles_bulletimpacts.integer);
	M_Options_PrintCheckbox("                 Smoke", true, cl_particles_smoke.integer);
	M_Options_PrintCheckbox("                Sparks", true, cl_particles_sparks.integer);
	M_Options_PrintCheckbox("               Bubbles", true, cl_particles_bubbles.integer);
	M_Options_PrintCheckbox("                 Blood", true, cl_particles_blood.integer);
	M_Options_PrintSlider(  "         Blood Opacity", true, cl_particles_blood_alpha.value, 0.2, 1);
	M_Options_PrintCheckbox("Force New Blood Effect", true, cl_particles_blood_bloodhack.integer);
	M_Options_PrintCheckbox("     Polygon Lightning", true, cl_beams_polygons.integer);
	M_Options_PrintCheckbox("Smooth Sweep Lightning", true, cl_beams_instantaimhack.integer);
	M_Options_PrintCheckbox(" Waist-level Lightning", true, cl_beams_quakepositionhack.integer);
	M_Options_PrintCheckbox("   Lightning End Light", true, cl_beams_lightatend.integer);
	M_Options_PrintSlider(  "   Lightning Thickness", cl_beams_polygons.integer, r_lightningbeam_thickness.integer, 1, 10);
	M_Options_PrintSlider(  "      Lightning Scroll", cl_beams_polygons.integer, r_lightningbeam_scroll.integer, 0, 10);
	M_Options_PrintSlider(  " Lightning Repeat Dist", cl_beams_polygons.integer, r_lightningbeam_repeatdistance.integer, 64, 1024);
	M_Options_PrintSlider(  "   Lightning Color Red", cl_beams_polygons.integer, r_lightningbeam_color_red.value, 0, 1);
	M_Options_PrintSlider(  " Lightning Color Green", cl_beams_polygons.integer, r_lightningbeam_color_green.value, 0, 1);
	M_Options_PrintSlider(  "  Lightning Color Blue", cl_beams_polygons.integer, r_lightningbeam_color_blue.value, 0, 1);
	M_Options_PrintCheckbox(" Lightning QMB Texture", cl_beams_polygons.integer, r_lightningbeam_qmbtexture.integer);
	M_Options_PrintCheckbox("   Model Interpolation", true, r_lerpmodels.integer);
	M_Options_PrintCheckbox("  Sprite Interpolation", true, r_lerpsprites.integer);
	M_Options_PrintCheckbox(" Flicker Interpolation", true, r_lerplightstyles.integer);
	M_Options_PrintSlider(  "            View Blend", true, gl_polyblend.value, 0, 1);
	M_Options_PrintSlider(  "Upper Sky Scroll Speed", true, r_skyscroll1.value, -8, 8);
	M_Options_PrintSlider(  "Lower Sky Scroll Speed", true, r_skyscroll2.value, -8, 8);
	M_Options_PrintSS(      "  Underwater View Warp", true, efwarp(getefwaterwarp()) );
	M_Options_PrintSlider(  " Water Alpha (opacity)", true, r_wateralpha.value, 0, 1);
	M_Options_PrintSS(      "          Water Deform", true, efdistort(getefdistort()) );
	M_Options_PrintSlider(  "        Water Movement", true, r_waterscroll.value, 0, 10);

	PPX_DrawSel_End ();
}


static void M_Options_Effects_Key (int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: if (hotspotx_hover != not_found_neg1) { options_effects_cursor = hotspotx_hover; goto leftus; } // fall thru
	case K_ESCAPE:	
		M_Menu_Options_Classic_f ();
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else options_effects_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		M_Menu_Options_Effects_AdjustSliders (1);
		break;

	case K_HOME: 
		options_effects_cursor = 0;
		break;

	case K_END:
		options_effects_cursor = OPTIONS_EFFECTS_ITEMS - 1;
		break;

	case K_PGUP:
		options_effects_cursor -= visiblerows / 2;
		if (options_effects_cursor < 0) options_effects_cursor = 0;
		break;

	case K_MWHEELUP:
		options_effects_cursor -= visiblerows / 4;
		if (options_effects_cursor < 0) options_effects_cursor = 0;
		break;

	case K_PGDN:
		options_effects_cursor += visiblerows / 2;
		if (options_effects_cursor > OPTIONS_EFFECTS_ITEMS - 1) options_effects_cursor = OPTIONS_EFFECTS_ITEMS - 1;
		break;

	case K_MWHEELDOWN:
		options_effects_cursor += visiblerows / 4;
		if (options_effects_cursor > OPTIONS_EFFECTS_ITEMS - 1) options_effects_cursor = OPTIONS_EFFECTS_ITEMS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		options_effects_cursor--;
		if (options_effects_cursor < 0)
			options_effects_cursor = OPTIONS_EFFECTS_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		options_effects_cursor++;
		if (options_effects_cursor >= OPTIONS_EFFECTS_ITEMS)
			options_effects_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		M_Menu_Options_Effects_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_Effects_AdjustSliders (1);
		break;
	}
}

#undef msel_cursoric
#undef m_local_cursor
#undef visiblerows