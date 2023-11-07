/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "darkplaces.h"
#include "cdaudio.h"
#include "image.h"
#include "progsvm.h"

#include "mprogdefs.h"

#define TYPE_DEMO 1
#define TYPE_GAME 2
#define TYPE_BOTH 3

static cvar_t forceqmenu = { 0, "forceqmenu", "0", "enables the quake menu instead of the quakec menu.dat (if present)" };
static cvar_t menu_progs = { 0, "menu_progs", "menu.dat", "name of quakec menu.dat file" };
static cvar_t menu_options_colorcontrol_correctionvalue = {0, "menu_options_colorcontrol_correctionvalue", "0.5", "intensity value that matches up to white/black dither pattern, should be 0.5 for linear color"};

static int rotc(int count, int val, int dir)
{
	int j = val + dir;
	     if (j >= count)	j = 0;
	else if (j < 0)			j = count - 1;
	return j;
}

extern int iszirc;


static int NehGameType;
void menu_state_set_nova (int ee);
static qbool m_missingdata = false;

int menu_state_reenter = 0; // To renter at maps menu

static void M_Options_Draw (void);
static void M_VideoNova_Draw (void);
static void M_VideoNova_Key (int key, int ascii);
static void M_Options_Key (int k, int ascii);

static void M_Maps_Key (int k, int ascii);
static void M_Maps_Draw (void);

extern cvar_t gl_picmip;
extern cvar_t cl_bobmodel_speed;
extern cvar_t con_textsize;
extern cvar_t con_notifysize;
extern cvar_t slowmo;
extern cvar_t gl_texture_anisotropy;
extern cvar_t r_textshadow;
extern cvar_t r_hdr_scenebrightness;
extern cvar_t r_nearest_conchars;

static int menuplyr_width, menuplyr_height, menuplyr_top, menuplyr_bottom, menuplyr_load;
static unsigned char *menuplyr_pixels;

enum m_state_e m_state;
char m_return_reason[128];

static void M_NoSave_Key (int key, int ascii);
static void M_NoSave_Draw (void);

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
	void M_Menu_OptionsNova_f (void);
	void M_Menu_Options_Classic_f (void);
	void M_Menu_Options_Effects_f (void);
	void M_Menu_Options_Graphics_f (void);
	void M_Menu_Options_ColorControl_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Reset_f (void);
		void M_Menu_Video_f (void);
		void M_Menu_VideoNova_f (void);
	void M_Menu_Help_f (void);
		void M_Menu_Maps_f (void);
	void M_Menu_Credits_f (void);
	void M_Menu_Quit_f (void);
	void M_Menu_NoSave_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_ServerList_f (void);
void M_Menu_ModList_f (void);

static void M_Main_Draw (void);
	static void M_SinglePlayer_Draw (void);
		static void M_Load_Draw (void);
		static void M_Save_Draw (void);
	static void M_MultiPlayer_Draw (void);
		static void M_Setup_Draw (void);
	static void M_OptionsNova_Draw (void);
	static void M_Options_Effects_Draw (void);
	static void M_Options_Graphics_Draw (void);
	static void M_Options_ColorControl_Draw (void);
		static void M_Keys_Draw (void);
		static void M_Reset_Draw (void);
		static void M_Video_Draw (void);
	static void M_Help_Draw (void);
	static void M_Credits_Draw (void);
	static void M_Quit_Draw (void);
static void M_LanConfig_Draw (void);
static void M_GameOptions_Draw (void);
static void M_ServerList_Draw (void);
static void M_ModList_Draw (void);


static void M_Main_Key (int key, int ascii);
	static void M_SinglePlayer_Key (int key, int ascii);
		static void M_Load_Key (int key, int ascii);
		static void M_Save_Key (int key, int ascii);
	static void M_MultiPlayer_Key (int key, int ascii);
		static void M_Setup_Key (int key, int ascii);
	static void M_OptionsNova_Key (int key, int ascii);
	static void M_Options_Effects_Key (int key, int ascii);
	static void M_Options_Graphics_Key (int key, int ascii);
	static void M_Options_ColorControl_Key (int key, int ascii);
		static void M_Keys_Key (int key, int ascii);
		static void M_Reset_Key (int key, int ascii);
		static void M_Video_Key (int key, int ascii);
	static void M_Help_Key (int key, int ascii);
	static void M_Credits_Key (int key, int ascii);
	static void M_Quit_Key (int key, int ascii);
static void M_LanConfig_Key (int key, int ascii);
static void M_GameOptions_Key (int key, int ascii);
static void M_ServerList_Key (int key, int ascii);
static void M_ModList_Key (int key, int ascii);

static qbool	m_entersound;		///< play after drawing a frame, so caching won't disrupt the sound

// Baker: These are windowed modes and focused on 4:3 aspect ratio
// In modern times, 16:9 or 8:5 is more what we want.
video_resolution_t video_resolutions_hardcoded[] =
{
{"WideScreen 16x9"           ,  640, 360, 640, 360, 1     },
{"WideScreen 16x9"           ,  683, 384, 683, 384, 1     },
{"Zircon 2:1"                ,  900, 450, 900, 450, 1     },
{"WideScreen 16x9"           ,  960, 540, 640, 360, 1     },
{"Zircon 2:1"                , 1200, 600, 1200, 600, 1    },
{"WideScreen 16x9"           , 1280, 720, 640, 360, 1     },
{"WideScreen 16x9"           , 1360, 768, 680, 384, 1     },
{"WideScreen 16x9"           , 1366, 768, 683, 384, 1     },
{"WideScreen 16x9"           , 1920,1080, 640, 360, 1     },
{"WideScreen 16x9"           , 2560,1440, 640, 360, 1     },
{"WideScreen 16x9"           , 3840,2160, 640, 360, 1     },
{NULL, 0, 0, 0, 0, 0}
};
// this is the number of the default mode (640x480) in the list above
int video_resolutions_hardcoded_count = sizeof(video_resolutions_hardcoded) / sizeof(*video_resolutions_hardcoded) - 1;

static qbool menu_video_resolutions_forfullscreen;

static void M_Menu_Video_FindResolution(int w, int h, float a);


void M_Update_Return_Reason(const char *s)
{
	strlcpy(m_return_reason, s, sizeof(m_return_reason));
	if (s)
		Con_DPrintf("%s\n", s);
}

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)

// Nehahra
#define NumberOfNehahraDemos 34
typedef struct nehahrademonames_s
{
	const char *name;
	const char *desc;
} nehahrademonames_t;

static nehahrademonames_t NehahraDemos[NumberOfNehahraDemos] =
{
	{"intro", "Prologue"},
	{"genf", "The Beginning"},
	{"genlab", "A Doomed Project"},
	{"nehcre", "The New Recruits"},
	{"maxneh", "Breakthrough"},
	{"maxchar", "Renewal and Duty"},
	{"crisis", "Worlds Collide"},
	{"postcris", "Darkening Skies"},
	{"hearing", "The Hearing"},
	{"getjack", "On a Mexican Radio"},
	{"prelude", "Honor and Justice"},
	{"abase", "A Message Sent"},
	{"effect", "The Other Side"},
	{"uhoh", "Missing in Action"},
	{"prepare", "The Response"},
	{"vision", "Farsighted Eyes"},
	{"maxturns", "Enter the Immortal"},
	{"backlot", "Separate Ways"},
	{"maxside", "The Ancient Runes"},
	{"counter", "The New Initiative"},
	{"warprep", "Ghosts to the World"},
	{"counter1", "A Fate Worse Than Death"},
	{"counter2", "Friendly Fire"},
	{"counter3", "Minor Setback"},
	{"madmax", "Scores to Settle"},
	{"quake", "One Man"},
	{"cthmm", "Shattered Masks"},
	{"shades", "Deal with the Dead"},
	{"gophil", "An Unlikely Hero"},
	{"cstrike", "War in Hell"},
	{"shubset", "The Conspiracy"},
	{"shubdie", "Even Death May Die"},
	{"newranks", "An Empty Throne"},
	{"seal", "The Seal is Broken"}
};

static float menu_x, menu_y, menu_width, menu_height;

static void M_Background(int width, int height)
{
	menu_width = bound(1.0f, (float)width, vid_conwidth.value);
	menu_height = bound(1.0f, (float)height, vid_conheight.value);
	menu_x = (vid_conwidth.integer - menu_width) * 0.5;
	menu_y = (vid_conheight.integer - menu_height) * 0.5;
	//DrawQ_Fill(menu_x, menu_y, menu_width, menu_height, 0, 0, 0, 0.5, 0);
	DrawQ_Fill(0, 0, vid_conwidth.integer, vid_conheight.integer, 0, 0, 0, 0.5, 0);
}

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
static void M_DrawCharacter (float cx, float cy, int num)
{
	char temp[2];
	temp[0] = num;
	temp[1] = 0;
	DrawQ_String(menu_x + cx, menu_y + cy, temp, 1, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintColored(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, false, FONT_MENU);
}

//
static void M_Print(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintBronzey(float cx, float cy, const char *str)
{ // 84 64 26 // 0.8, 0.5, 0.2 // 84/255.0, 64/255.0, 26/255.0
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, /*rgb*/ 0.8, 0.5, 0.2, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintRed(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 0, 0, 1, 0, NULL, true, FONT_MENU);
}

static void M_ItemPrint(float cx, float cy, const char *str, int unghosted)
{
	if (unghosted)
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	else
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 0.4, 0.4, 0.4, 1, 0, NULL, true, FONT_MENU);
}

static void M_ItemPrint2(float cx, float cy, const char *str, int unghosted)
{
	if (unghosted)
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, /*ignore color codes*/ false, FONT_MENU);
	else
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 0.4, 0.4, 0.4, 1, 0, NULL, /*ignore color codes*/ false, FONT_MENU);
}

#define NO_HOTSPOTS_0		0
#define USE_IMAGE_SIZE_NEG1	-1
#define USE_IMAGE_SIZE_NEG1	-1
#define NA0					0

typedef enum {
	hotspottype_none_0 = 0,
	hotspottype_inert,			// It's a hotspot.  It's entirely inert.  Used to fill to keep indexes static.
	hotspottype_toggle,			// More than 2 choices.		Responds to left and right.
	hotspottype_slider,			// Slider.					Responds to left and right.
	hotspottype_button,			// Execute.					Example: Single Player, Reset Defaults
	hotspottype_button_line,	// Execute.					Namemaker.  Same as button but little extra trim of what is highlighted for clarity.
//  hotspottype_emitkey,		// Emitter.					Fake key emission
	hotspottype_listitem,		// A list item				Demos and levels and serverlist.
	hotspottype_text,			// Text						// ?
	hotspottype_textbutton,		// Text button.  Enter fires.
	hotspottype_screen,			// Help uses this for next.  Remember, it is not acceptable for a K_MOUSE1 with no hotspot to do something.
	hotspottype_vscroll,		//
	hotspottype_hscroll,		//
} hotspottype_e;


typedef struct {
		float	left, top, width, height;
} crectf_s;

typedef struct {
	crectf_s r;
	int				idx;				// Our number (cursor).
	int				trueidx;
	hotspottype_e	hotspottype;

} hotspotx_s;


hotspotx_s hotspotxs[128];
int hotspotx_count;
int hotspotx_hover;
hotspotx_s *hotspotx_hover_hs;

static int drawidx;
static int drawcur_y;


// in_windowmouse_x && in_windowmouse_y
//Con_PrintLinef ("%d %d", (int)in_windowmouse_x, (int)in_windowmouse_y);

int Hotspots_Hit (void)
{
	int n; for (n = 0; n < hotspotx_count; n ++) {
		hotspotx_s *h = &hotspotxs[n];
		int is_hit = RECT_HIT (h->r, in_windowmouse_x, in_windowmouse_y);
		if (is_hit) {
			return n;
		} // hit
	} // for
	return not_found_neg1;
}

int Hotspots_DidHit_Slider (void)
{
	hotspotx_hover = Hotspots_Hit();
	if (hotspotx_hover != not_found_neg1) {
		hotspotx_hover_hs = &hotspotxs[hotspotx_hover];
		if (hotspotx_hover_hs->hotspottype == hotspottype_slider)
			return true;

	}
	return false;
}

int Hotspots_DidHit (void)
{
	hotspotx_hover = Hotspots_Hit();
	if (hotspotx_hover != not_found_neg1) {
		return true;
	}
	return false;
}

int Hotspots_GetIdx (int hidx)
{
	if (in_range_beyond (0,  hidx, hotspotx_count )) {
		hotspotx_s *h = &hotspotxs[hidx];
		if (h->trueidx != not_found_neg1)
			return h->trueidx;

		return hidx;
	}
	return not_found_neg1;
}


hotspotx_s *Hotspots_Add (float left, float top, float width, float height, int count_, hotspottype_e hotspottype)
{
	// convert to screen pixels
	float xmag = vid.width / vid_conwidth.value; // 1300 / 240
	float ymag = vid.height / vid_conheight.value;
	left = left * xmag;
	top = top * ymag;
	width = width * xmag;
	height = height * ymag;


	hotspotx_s *h = NULL;
	int count	= count_ ? count_ : 1;
	int nheight = count_ ? height / count : height;

	int n; for (n = 0; n < count; n ++) {
		int idx = hotspotx_count;
		h = &hotspotxs[idx];
		h->idx			= idx;
		h->trueidx 		= not_found_neg1;
		h->hotspottype	= hotspottype;
		RECT_SET (h->r, left, top, width, nheight);
		// Con_PrintLinef ("Hotspot %d = %d, %d %d x %d", hotspot_menu_item[idx].idx, hotspot_menu_item[idx].rect.left, hotspot_menu_item[idx].rect.top, hotspot_menu_item[idx].rect.width, hotspot_menu_item[idx].rect.height);
		top += nheight;
		hotspotx_count ++;
	} // for


	return h;
}

// Anything that can scroll must come here instead
void Hotspots_Add2 (float left, float top, float width, float height, int count_, hotspottype_e hotspottype, int trueidx)
{
	hotspotx_s *h = Hotspots_Add (left, top, width, height, count_, hotspottype);
	h->trueidx=trueidx;
}

#define PPX_Start(realcursor, frameselcursor) drawidx = 0; drawsel_idx = not_found_neg1; frameselcursor = realcursor

int drawsel_idx;
int draw_idx;
// Sets menu state
void menu_state_set_nova (int ee)
{
	m_state = (enum m_state_e)ee;

	hotspotx_hover = not_found_neg1;
	hotspotx_count = 0;

	drawsel_idx = not_found_neg1;

}




static void PPX_DrawSel_End (void)
{
	if (drawsel_idx!= not_found_neg1) { // PPX SEL
		crectf_s r	= hotspotxs[drawsel_idx].r;
		float xmag	= vid.width / vid_conwidth.value; // 1300 / 240
		float ymag	= vid.height / vid_conheight.value;

		float left		= r.left	/ xmag;
		float top		= r.top		/ ymag;
		float width		= r.width	/ xmag;
		float height	= r.height	/ ymag;

		float redx = 0.5 + 0.2 * sin(realtime * M_PI);
		DrawQ_Fill(left, top, width, height, /*rgb*/ redx, 0, 0, /*a*/ 0.5, DRAWFLAG_ADDITIVE);
	} // if
}
#define CPC(x) Draw_CachePic (x)


static void M_DrawPic(float cx, float cy, cachepic_t *pico, int count, int _colsize, int _rowsize)
{
	DrawQ_Pic(menu_x + cx, menu_y + cy, pico, 0, 0, 1, 1, 1, 1, 0);
	if (count) {
		int width  = (_colsize == USE_IMAGE_SIZE_NEG1) ? pico->width  : _colsize * count;
		int height = (_rowsize == USE_IMAGE_SIZE_NEG1) ? pico->height : _rowsize * count;
		Hotspots_Add (menu_x + cx, menu_y + cy, width, height, count, hotspottype_button);
	}
}

static void M_DrawTextBox(float x, float y, float width, float height)
{
	int n;
	float cx, cy;

	// draw left side
	cx = x;
	cy = y;
	M_DrawPic (cx, cy, CPC("gfx/box_tl"), NO_HOTSPOTS_0, NA0, NA0);
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, CPC("gfx/box_ml"), NO_HOTSPOTS_0, NA0, NA0);
	}
	M_DrawPic (cx, cy+8, CPC("gfx/box_bl"), NO_HOTSPOTS_0, NA0, NA0);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		M_DrawPic (cx, cy, CPC("gfx/box_tm"), NO_HOTSPOTS_0, NA0, NA0);
		for (n = 0; n < height; n++)
		{
			cy += 8;
			if (n >= 1)
				M_DrawPic (cx, cy, CPC("gfx/box_mm2"), NO_HOTSPOTS_0, NA0, NA0);
			else
				M_DrawPic (cx, cy, CPC("gfx/box_mm"), NO_HOTSPOTS_0, NA0, NA0);
		}
		M_DrawPic (cx, cy+8, CPC("gfx/box_bm"), NO_HOTSPOTS_0, NA0, NA0);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	M_DrawPic (cx, cy, CPC("gfx/box_tr"), NO_HOTSPOTS_0, NA0, NA0);
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, CPC("gfx/box_mr"), NO_HOTSPOTS_0, NA0, NA0);
	}
	M_DrawPic (cx, cy+8, CPC("gfx/box_br"), NO_HOTSPOTS_0, NA0, NA0);
}

//=============================================================================

//int m_save_demonum;

/*
================
M_ToggleMenu
================
*/
static void M_ToggleMenu(int mode)
{
	m_entersound = true;

	if ((key_dest != key_menu && key_dest != key_menu_grabbed) || m_state != m_main)
	{
		if(mode == 0) {
			return; // the menu is off, and we want it off
		} else {
			if (menu_state_reenter) {
				menu_state_reenter = 0;
				M_Menu_Maps_f ();
			} else {
				M_Menu_Main_f ();
			}
		}
	}
	else
	{
		if(mode == 1)
			return; // the menu is on, and we want it on
		key_dest = key_game;
		menu_state_set_nova (m_none);
	}
}


static int demo_cursor;
static void M_Demo_Draw (void)
{
	int i;

	M_Background(320, 200);

	for (i = 0;i < NumberOfNehahraDemos;i++)
		M_Print(16, 16 + 8*i, NehahraDemos[i].desc);

	// line cursor
	M_DrawCharacter (8, 16 + demo_cursor*8, 12+((int)(realtime*4)&1));
}


static void M_Menu_Demos_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_demo);
	m_entersound = true;
}


static void M_Demo_Key (int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_ESCAPE:	case K_MOUSE2:
		M_Menu_Main_f ();
		break;

	//case K_MOUSE1:
	//	if (hotspotx_hover >= 0) {
	//		_cursor = hotspotx_hover;
	//		M_O_Key_Enter ();
	//	} // if
	//	break;

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		menu_state_set_nova (m_none);
		key_dest = key_game;
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "playdemo %s", NehahraDemos[demo_cursor].name));
		return;

	case K_HOME:
		demo_cursor = 0;
		break;

	case K_END:
		demo_cursor = NumberOfNehahraDemos-1;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		demo_cursor--;
		if (demo_cursor < 0)
			demo_cursor = NumberOfNehahraDemos-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		demo_cursor++;
		if (demo_cursor >= NumberOfNehahraDemos)
			demo_cursor = 0;
		break;
	}
}



//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

static void M_DrawSlider (int x, int y, float num, float rangemin, float rangemax)
{
	char text[16];
	int i;
	float range;
	range = bound(0, (num - rangemin) / (rangemax - rangemin), 1);
	M_DrawCharacter (x-8, y, 128);
	for (i = 0;i < SLIDER_RANGE;i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
	if (fabs((int)num - num) < 0.01)
		dpsnprintf(text, sizeof(text), "%i", (int)num);
	else
		dpsnprintf(text, sizeof(text), "%.3f", num);
	M_Print(x + (SLIDER_RANGE+2) * 8, y, text);
}

static void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print(x, y, "on");
	else
		M_Print(x, y, "off");
}


//#define moc_count_31 25 aule was here
//#define moc_count_31 27




int m_resetdef_prevstate;

void M_Menu_Reset_f (void)
{
	key_dest = key_menu;
	m_resetdef_prevstate = m_state;
	menu_state_set_nova (m_reset);
	m_entersound = true;
}


static void M_Reset_Key (int key, int ascii)
{
	char vabuf[1024];
	switch (key) {
	case K_MOUSE1:
	case 'Y':
	case 'y': {
		Cbuf_AddTextLine ("cvar_resettodefaults_all;exec default.cfg");

		// Baker 1024: Prevent stale values of vid_width or vid_height after a reset
		Cbuf_AddTextLine (	va(vabuf, sizeof(vabuf), "vid_width %d", vid_width.integer));
		Cbuf_AddTextLine (	va(vabuf, sizeof(vabuf), "vid_height %d", vid_height.integer));
		Cbuf_AddTextLine (	va(vabuf, sizeof(vabuf), "vid_fullscreen %d", vid_fullscreen.integer));

		// no break here since we also exit the menu
			  }
	case K_ESCAPE:	case K_MOUSE2:
	case 'n':
	case 'N':

		if (m_resetdef_prevstate == m_options) {
			menu_state_set_nova (m_options);
		} else {
			menu_state_set_nova (m_options_classic);
		}
		m_entersound = true;
		break;

	default:
		break;
	}
}

static void M_Reset_Draw (void)
{
	int lines = 2, linelength = 20;
	M_Background(linelength * 8 + 16, lines * 8 + 16);
	M_DrawTextBox(0, 0, linelength, lines);
	M_Print(8 + 4 * (linelength - 19),  8, "Reset all settings?");
	M_Print(8 + 4 * (linelength - 11), 16, "Press y / n");
}

// Baker: Transfusion
//=============================================================================
/* CREDITS MENU */

void M_Menu_Credits_f (void)
{
	key_dest = key_menu;
	menu_state_set_nova (m_credits);
	m_entersound = true;
}



static void M_Credits_Draw (void)
{
	M_Background(640, 480);
	M_DrawPic (0, 0, CPC("gfx/creditsmiddle"), NO_HOTSPOTS_0, NA0, NA0);
	M_Print (640/2 - 14/2*8, 236, "Coming soon...");
	M_DrawPic (0, 0, CPC("gfx/creditstop"), NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (0, 433, CPC("gfx/creditsbottom"), NO_HOTSPOTS_0, NA0, NA0);
}


static void M_Credits_Key (int key, int ascii)
{
		M_Menu_Main_f ();
}





//=============================================================================
/* Menu Subsystem */

static void M_KeyEvent(int key, int ascii, qbool downevent);
static void M_Draw(void);
void M_ToggleMenu(int mode);
static void M_Shutdown(void);

static void M_Init (void)
{
	menuplyr_load = true;
	menuplyr_pixels = NULL;

	Cmd_AddCommand ("menu_main", M_Menu_Main_f, "open the main menu");
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f, "open the singleplayer menu");
	Cmd_AddCommand ("menu_load", M_Menu_Load_f, "open the loadgame menu");
	Cmd_AddCommand ("menu_save", M_Menu_Save_f, "open the savegame menu");
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f, "open the multiplayer menu");
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f, "open the player setup menu");
	Cmd_AddCommand ("menu_options", M_Menu_OptionsNova_f, "open the options menu");
	Cmd_AddCommand ("menu_options_effects", M_Menu_Options_Effects_f, "open the effects options menu");
	Cmd_AddCommand ("menu_options_graphics", M_Menu_Options_Graphics_f, "open the graphics options menu");
	Cmd_AddCommand ("menu_options_colorcontrol", M_Menu_Options_ColorControl_f, "open the color control menu");
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f, "open the key binding menu");
	Cmd_AddCommand ("menu_video", M_Menu_Video_f, "open the video options menu");
	Cmd_AddCommand ("menu_video2", M_Menu_VideoNova_f, "open the video options menu");
	Cmd_AddCommand ("menu_reset", M_Menu_Reset_f, "open the reset to defaults menu");
	Cmd_AddCommand ("menu_mods", M_Menu_ModList_f, "open the mods browser menu");
	Cmd_AddCommand ("menu_maps", M_Menu_Maps_f, "open the maps browser menu");
	Cmd_AddCommand ("help", M_Menu_Help_f, "open the help menu");
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f, "open the quit menu");

	Cmd_AddCommand ("menu_credits", M_Menu_Credits_f, "open the credits menu");
}


static void M_NewMap(void)
{
}

static int M_GetServerListEntryCategory(const serverlist_entry_t *entry)
{
	return 0;
}

void M_Shutdown(void)
{
	// reset key_dest
	key_dest = key_game;
}



WARP_X_ (M_Draw)

void M_KeyEvent (int key, int ascii, qbool downevent)
{
	if (!downevent)
		return;

	switch (m_state) {
	case m_none:																return;
	case m_main:					M_Main_Key (key, ascii);					return;
	case m_demo:					M_Demo_Key (key, ascii);					return;
	case m_singleplayer:			M_SinglePlayer_Key (key, ascii);			return;
	case m_load:					M_Load_Key (key, ascii);					return;
	case m_save:					M_Save_Key (key, ascii);					return;
	case m_multiplayer:				M_MultiPlayer_Key (key, ascii);				return;
	case m_setup:					M_Setup_Key (key, ascii);					return;
	case m_options:					M_OptionsNova_Key (key, ascii);				return;
	case m_options_classic:				M_Options_Key (key, ascii);					return;
	case m_options_effects:			M_Options_Effects_Key (key, ascii);			return;
	case m_options_graphics:		M_Options_Graphics_Key (key, ascii);		return;
	case m_options_colorcontrol:	M_Options_ColorControl_Key (key, ascii);	return;

	case m_keys:					M_Keys_Key (key, ascii);					return;
	case m_reset:					M_Reset_Key (key, ascii);					return;
	case m_video:					M_Video_Key (key, ascii);					return;
	case m_videonova:				M_VideoNova_Key (key, ascii);				return;
	case m_help:					M_Help_Key (key, ascii);					return;
	case m_credits:					M_Credits_Key (key, ascii);					return;
	case m_quit:					M_Quit_Key (key, ascii);					return;
	case m_nosave:					M_NoSave_Key (key, ascii);					return;
	case m_lanconfig:				M_LanConfig_Key (key, ascii);				return;
	case m_gameoptions:				M_GameOptions_Key (key, ascii);				return;
	case m_slist:					M_ServerList_Key (key, ascii);				return;
	case m_modlist:					M_ModList_Key (key, ascii);					return;
	case m_maps:					M_Maps_Key (key, ascii);					return;
	} // sw

}

WARP_X_ (M_Options_Draw M_OptionsNova_Draw M)

void M_Draw (void)
{
	//char vabuf[1024];
	if (key_dest != key_menu && key_dest != key_menu_grabbed) {
		menu_state_set_nova (m_none); // Does this happen? During demo play at startup only.
	}

	if (m_state == m_none)
		return;

	// Hover is 1 frame behind, drawn first
	if (hotspotx_count && hotspotx_hover != not_found_neg1) {
		crectf_s r = hotspotxs[hotspotx_hover].r;
		float xmag		= vid.width / vid_conwidth.value; // 1300 / 240
		float ymag		= vid.height / vid_conheight.value;

		float left		= r.left / xmag;
		float top		= r.top / ymag;
		float width		= r.width / xmag;
		float height	= r.height / ymag;

		if (isin4 (m_state, m_maps, m_modlist, m_keys, m_slist)) {
			DrawQ_Fill(left, top, width, height, /*bronzey*/ 0.5, 0.25, 0.1, 0.75, DRAWFLAG_NORMAL);
		} else {
			DrawQ_Fill(left, top, width, height, /*bronzey*/ 0.5, 0.25, 0.1, 0.37, DRAWFLAG_NORMAL);
		}
	} // if

	hotspotx_count = 0;

	switch (m_state)	{
	case m_none:													break;
	case m_main:					M_Main_Draw ();					break;
	case m_demo:					M_Demo_Draw ();					break;
	case m_singleplayer:			M_SinglePlayer_Draw ();			break;
	case m_load:					M_Load_Draw ();					break;
	case m_save:					M_Save_Draw ();					break;
	case m_nosave:					M_NoSave_Draw ();				break;
	case m_multiplayer:				M_MultiPlayer_Draw ();			break;
	case m_setup:					M_Setup_Draw ();				break;
	case m_options:					M_OptionsNova_Draw (); 			break;
	case m_options_classic:			M_Options_Draw ();				break;
	case m_options_effects:			M_Options_Effects_Draw ();		break;
	case m_options_graphics:		M_Options_Graphics_Draw ();		break;
	case m_options_colorcontrol:	M_Options_ColorControl_Draw ();	break;
	case m_keys:					M_Keys_Draw ();					break;
	case m_reset:					M_Reset_Draw ();				break;
	case m_video:					M_Video_Draw ();				break;

	case m_videonova:				M_VideoNova_Draw ();			break;
	case m_help:					M_Help_Draw ();					break;
	case m_credits:					M_Credits_Draw ();				break;
	case m_quit:					M_Quit_Draw ();					break;
	case m_lanconfig:				M_LanConfig_Draw ();			break;
	case m_gameoptions:				M_GameOptions_Draw ();			break;
	case m_slist:					M_ServerList_Draw ();			break;
	case m_modlist:					M_ModList_Draw ();				break;
	case m_maps:					M_Maps_Draw ();					break;
	}
	if (m_entersound) {
		S_LocalSound ("sound/misc/menu2.wav");
		m_entersound = false;
	}

	hotspotx_hover = Hotspots_Hit ();
	S_ExtraUpdate ();
}

#include "menu_qc.c.h"
#include "menu_main.c.h"
#include "menu_main_zirc.c.h"
	#include "menu_si.c.h"
		#include "menu_saveload.c.h"

	#include "menu_mp.c.h"
		#include "menu_lan.c.h"
		#include "menu_setup.c.h"	// Player look?
		#include "menu_slist.c.h"

	#include "menu_options_nova.c.h"
		#include "menu_keys.c.h"
			#include "menu_video_classic.c.h"
		#include "menu_video_nova.c.h"
		#include "menu_options_classic.c.h"


			#include "menu_color.c.h"
			#include "menu_effects.c.h"
			#include "menu_graphics.c.h"
			#include "menu_modlist.c.h"
			#include "menu_game.c.h"
	#include "menu_help.c.h"
		#include "menu_maps.c.h"
	#include "menu_quit.c.h"

void Menu_Resets (void)
{
	m_main_cursor					=
		options_cursor				=
		options_cursorx				=
		m_singleplayer_cursor		=
		load_cursor					=
		m_multiplayer_cursor		=
		mapscursor					=
		options_colorcontrol_cursor =
		options_effects_cursor		=
		options_graphics_cursor		=
		keys_cursor					=
		setup_cursor				=
		slist_cursor				=
		video_cursor				=
		video2_cursor				= 0;
	lanConfig_cursor = -1;

}
