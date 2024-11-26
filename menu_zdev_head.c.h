// menu_zdev_head.c.h

WARP_X_ (ZDev_Feed_Shall_Stop_Fn M_ZDev_Draw)



typedef struct {
	qbool			table_is_clean;			// So that is defaults 0 for "dirty"

	int				cursor;
	int				start_row;
	int				beyond_row;
	int				visible_rows;

// Calcs
	int				usable_height;			// 

// Special duty
	double			click_time;				// For double click test, so we can select items with mouse without firing
	qbool			scroll_is_blocked;
	int				previous_cursor_plus1;	// For "just-in-time" texture load.  Plus1 for 0 initialization means none.

	stringlist_t	list;
	int				list_what;				// list_what_e, we don't use enum because PITA

// Options
	int				is_more_detail;

	struct _theme_s	*checkbox_theme;
	float			checkbox_scale;

	struct _theme_s	*tabsel_theme;
	float			tabsel_scale;
} menu_form_s;

menu_form_s _mz, *mz = &_mz;

const char *s_tabnames[] = {
	"Entities",			//	0
	"Models",			//	1
	"Sounds",			//	2
	"Textures",			//	3
	"Shaders",			//	4
	"Globals",			//  5
	"Fields",			//	6
	"Map", 				//  7	Sky, Is water vised
	"Environment", 		//	8	What would this be?
	"CSQC Entities",	//	9
	"CSQC Globals",		//	10
	"CSQC Fields",		//	11
	"Zircon Extras",	//	12
};
int s_tabnames_count = ARRAY_COUNT(s_tabnames);
//#pragma message ("Do a scrollbar")

WARP_X_ (M_ZDev_Draw M_ZDev_Key)
typedef enum {
	lw_entities_0		= 0,
	lw_models_1			= 1,
	lw_sounds_2			= 2,
	lw_textures_3		= 3,
	lw_shaders_4		= 4,
	lw_globals_5		= 5,
	lw_fields_6			= 6,
	lw_map_7			= 7,
	lw_environment_8	= 8,
	lw_csqc_ents_9		= 9,
	lw_csqc_globals_10	= 10,
	lw_csqc_fields_11	= 11,
	//lw_obs_zircon_extras_12	= 12,
	//lw_obs_count_13		= 13,

	lw_quake_rc_12		= 12,
	lw_default_cfg_13	= 13,
	lw_autoexec_cfg_14	= 14,
	lw_config_cfg_15	= 15,
	lw_map_ents_16		= 16,
	lw_rtlights_17		= 17,
	lw_zircfeatures_18	= 18,
	lw_zircdev_19		= 19,
	lw_zircpreproces_20 = 20,
	lw_obs_count_21		= 21,
} lw_e; // list_what

typedef struct _theme_s {
	float			padding_top_pct_of_scale;
	float			padding_bottom_pct_of_top;
	float			margin_pct;
	float			backalpha;
	vec3_t			forecolor;
	vec3_t			backcolor;
	vec3_t			forecolor_selected;
	vec3_t			backcolor_selected;
} theme_s;

theme_s theme_checkbox1 = { 0, 0.25, 1/4.0, 1, { RGBFLOT(255, 255, 255) },  { RGBFLOT(255, 255, 0 ) }, 
										{ RGBFLOT(255, 255, 255) },  { RGBFLOT(255, 255, 0 ) }, };
theme_s theme_tabsel1 = { 1/6.0, 0.25, 1/8.0, 0.5, { RGBFLOT(128, 128, 128) },  { RGBFLOT(0,51,167) }, 
										{ RGBFLOT(255, 255, 255) },  { RGBFLOT(0,90,230) }, };

//#define mz_scale_24	24	// Checkboxes and stuff?

void MZ_Reset (void)
{
	stringlistfreecontents (&mz->list);
	memset (mz, 0, sizeof(*mz));
	mz->checkbox_theme = &theme_checkbox1;
	mz->checkbox_scale = 24;
	mz->tabsel_theme = &theme_tabsel1;
	mz->tabsel_scale = 24;
}

//void MZ_Button_Hit (int idx)
//{
//	if (mz->list_what == list_what_globals_1)
//		mz->list_what = list_what_shaders_0;
//	else mz->list_what = list_what_globals_1;
//	
//	Toggle_Var (mz->is_more_detail);
//
//	mz->table_is_clean = false;
//}

void DEV_Category_Rotate (int dir)
{
	mz->list_what += dir;
	if (mz->list_what >= lw_obs_count_21)
		mz->list_what = 0;
	else if (mz->list_what < 0)
		mz->list_what = lw_obs_count_21 - 1;

	mz->table_is_clean = false;	
}

void DEV_Category_Click (int idx)
{
	if (mz->list_what == idx)
		return;

	mz->list_what = idx;
	mz->table_is_clean = false;	
}

void DEV_Detail_Click (int idx)
{
	Toggle_Var (mz->is_more_detail);

	mz->table_is_clean = false;	
}



void MZ_CheckBox (float x, float y, ccs *s_title, int value, theme_s *theme, float scale, int *pbutton_id, fire_fn_t fire_fn)
{
	ccs *spic = value ? "engine/bicons/checked" :  "engine/bicons/unchecked";
	frect r	= { x, y, scale, scale }; 

	cachepic_t *pic = Draw_CachePic_Flags (spic, CACHEPICFLAG_NOTPERSISTENT);// pw = Draw_GetPicWidth(pic);

	DrawQ_Pic		(RECT_SEND_XY(r), pic, r.width, r.height, RGBFLOT(255,255,255), alpha_1_0, DRAWFLAG_NORMAL_0);

	frect r2 = { RECT_RIGHTOF(r) + theme->margin_pct * r.width, r.top, r.width, r.height };

	float x_after = DrawQ_String_Scale (RECT_SEND_XY(r2), s_title, maxlen_0, r2.width, r2.height, scale_1_0, scale_1_0, 
		RGBFLOT(255,255,255), alpha_1_0, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, 
		ignorecolorcodes_true, zdev_dpfont
	);

	frect r3 = { r.left, r.top, x_after - r.left, r.height };
	Hotspots_Add3	(RECT_SEND(r3), count_1, hotspottype_listitem, (*pbutton_id), fire_fn, /*fire_idx*/ 0 );

	(*pbutton_id) --;
}

void MZ_DrawTabSelect (float x, float y, const char **names, size_t names_count, int value, theme_s *theme, 
					   float scale, int *pbutton_id, fire_fn_t fire_fn)
{
	float scale_top = scale * theme->padding_top_pct_of_scale;
	float scale_bottom = scale_top * theme->padding_bottom_pct_of_top;

	for (int n = 0; n < (int)names_count; n ++) {
		const char *s = names[n];

		frect r	= { x, y, scale * 6, scale }; 
		frect r2 = { r.left - scale_top, r.top - scale_top, r.width + scale_top * 2, 
			r.height + scale_top + scale_bottom };

		int is_selected = (n == value); //  // RGBFLOT(0,90,230), RGBFLOT(0,51,167), 
		vec_t *backcolor = is_selected ? theme->backcolor_selected : theme->backcolor;
		vec_t *forecolor = is_selected ? theme->forecolor_selected : theme->forecolor;
	
		DrawQ_Fill (RECT_SEND(r2), VECTOR3_SEND(backcolor), theme->backalpha, DRAWFLAG_NORMAL_0);
			
		DrawQ_String_Scale (RECT_SEND_XY(r), s, maxlen_0, /*xy*/ scale, scale, scale_1_0, scale_1_0, 
			VECTOR3_SEND(forecolor), alpha_1_0, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, 
			ignorecolorcodes_true, zdev_dpfont
		);

		Hotspots_Add3	(RECT_SEND(r2), count_1, hotspottype_listitem, (*pbutton_id), fire_fn, /*fire_idx*/ n );

		y += (r2.height + theme->margin_pct * scale);
		(*pbutton_id) --;
	} // for
}	

#include "menu_zdev_feed.c.h"

WARP_X_ (M_ZDev_Draw M_ZDev_Key)
WARP_X_ (ZDev_Globals_Feed_Shall_Stop_Fn MZ_Reset)

void Mod_Shaders_Query (feed_fn_t myfeed_shall_stop);
void R_WorldTextures_Query (feed_fn_t myfeed_shall_stop);
void CL_Models_Query (feed_fn_t myfeed_shall_stop);
void CL_Sounds_Query (feed_fn_t myfeed_shall_stop);


//char *PRVM_Entities_Query_EdictNum_ZAlloc (prvm_prog_t *prog, int edict_num, ccs *fieldname);

WARP_X_CALLERS_ (M_ZDev_Draw)
void Data_Dirty_Check (void)
{
	if (mz->table_is_clean) return;

	if (host.superframecount < 3) return; // Give it some time to wake

	mz->cursor = 0;
	stringlistfreecontents (&mz->list); // Fill from edicts, from textures

	// Run the feed
	switch (mz->list_what) {
	case lw_entities_0:
		
		if (SVVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No server progs loaded (map not running)");
			break;
		}

		PRVM_Entities_Query_Fieldname (SVVM_prog, ZDev_Entities_Feed_Shall_Stop_Fn, "classname");
		break;

	case lw_models_1:
		
		if (!r_refdef.scene.worldmodel) {
			stringlistappend (&mz->list, "No precache models (map not running)");
			break;
		}
		CL_Models_Query (ZDev_Models_Feed_Shall_Stop_Fn);
		stringlistsort (&mz->list, fs_make_unique_false);

		break;

	case lw_sounds_2:
		
		if (!r_refdef.scene.worldmodel) {
			stringlistappend (&mz->list, "No precache sounds (map not running)");
			break;
		}
		CL_Sounds_Query (ZDev_Sounds_Feed_Shall_Stop_Fn);
		stringlistsort (&mz->list, fs_make_unique_false);

		break;


	case lw_textures_3:
		
		if (!r_refdef.scene.worldmodel && cls.state == ca_connected && cls.signon == SIGNONS_4) {
			stringlistappend (&mz->list, "No world textures (map not running)");
			break;
		}

		R_WorldTextures_Query (ZDev_WorldTextures_Feed_Shall_Stop_Fn);
		stringlistsort (&mz->list, fs_make_unique_false);
		break;

	case lw_shaders_4:

		Mod_Shaders_Query (ZDev_Shaders_Feed_Shall_Stop_Fn);
		stringlistsort (&mz->list, fs_make_unique_false);
		break;

	case lw_globals_5:

		if (SVVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No server progs loaded (map not running)");
			break;
		}

		PRVM_Globals_Query (SVVM_prog, ZDev_Globals_Feed_Shall_Stop_Fn);
		break;

	case lw_fields_6: // fields

		if (SVVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No server progs loaded (map not running)");
			break;
		}

		PRVM_Fields_Query (SVVM_prog, ZDev_Fields_Feed_Shall_Stop_Fn);
		stringlistsort (&mz->list, fs_make_unique_false);
		break;

	case lw_map_7:
		stringlistappend (&mz->list, "map is todo");
		break;

	case lw_environment_8:
		stringlistappend (&mz->list, "path and such");
		break;

	case lw_csqc_ents_9:
		if (CLVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No CSQC progs loaded");
			break;
		}

		PRVM_Entities_Query_Fieldname (SVVM_prog, ZDev_Entities_Feed_Shall_Stop_Fn, "classname");
		break;

	case lw_csqc_globals_10:
		if (CLVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No CSQC progs loaded");
			break;
		}

		PRVM_Entities_Query_Fieldname (SVVM_prog, ZDev_Entities_Feed_Shall_Stop_Fn, "classname");
		break;

	case lw_csqc_fields_11:
		if (CLVM_prog->loaded == false) {
			stringlistappend (&mz->list, "No CSQC progs loaded");
			break;
		}

		PRVM_Entities_Query_Fieldname (SVVM_prog, ZDev_Entities_Feed_Shall_Stop_Fn, "classname");
		break;

	default:

		for (int j = 0; j < 100; j ++) {
			stringlistappend (&mz->list, va32("%d", j));
		} // for
	} // sw
	mz->table_is_clean = true; // Up-to-date
}




