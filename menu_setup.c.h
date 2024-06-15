// menu_setup.c.h

#define 	local_count		NUM_SETUP_CMDS
#define		local_cursor	setup_cursor

//=============================================================================
/* SETUP MENU */

static int		setup_cursor = 4;
static int		setup_cursor_table[] = {40, 64, 88, 124, 140};

static char	setup_myname[MAX_SCOREBOARDNAME_128];
static int		setup_oldtop;
static int		setup_oldbottom;
static int		setup_top;
static int		setup_bottom;
static int		setup_rate;
static int		setup_oldrate;

#define	NUM_SETUP_CMDS	5

extern cvar_t cl_topcolor;
extern cvar_t cl_bottomcolor;

void M_Menu_Setup_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_setup);
	m_entersound = true;
	strlcpy(setup_myname, cl_name.string, sizeof(setup_myname));

#if 1
	setup_top = setup_oldtop = cl_color.integer >> 4;
	setup_bottom = setup_oldbottom = cl_color.integer & 15;
#else
	setup_top = setup_oldtop = cl_topcolor.integer;
	setup_bottom = setup_oldbottom = cl_bottomcolor.integer ;
#endif
	setup_rate = cl_rate.integer;
}


typedef struct ratetable_s
{
	int rate;
	const char *name;
}
ratetable_t;

#define RATES ((int)(sizeof(setup_ratetable)/sizeof(setup_ratetable[0])))
static ratetable_t setup_ratetable[] =
{
	{1000, "28.8 bad"},
	{1500, "28.8 mediocre"},
	{2000, "28.8 good"},
	{2500, "33.6 mediocre"},
	{3000, "33.6 good"},
	{3500, "56k bad"},
	{4000, "56k mediocre"},
	{4500, "56k adequate"},
	{5000, "56k good"},
	{7000, "64k ISDN"},
	{15000, "128k ISDN"},
	{25000, "broadband"},
	{30000, "broadband default"},
	{9999999, "broadband ++"}
};

static int setup_rateindex(int rate)
{
	int i;
	for (i = 0;i < RATES;i++)
		if (setup_ratetable[i].rate > setup_rate)
			break;
	return bound(1, i, RATES) - 1;
}

static void M_Setup_Draw (void)
{
	int i, j;
	cachepic_t	*p0;
	char vabuf[1024];

	M_Background(320, 200, q_darken_true);

	M_DrawPic (16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	
	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_DrawPic ( (320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_multi", NO_HOTSPOTS_0, NA0, NA0);

	Hotspots_Add (menu_x + 48, menu_y + 40, 320, 8 + 1, 1, hotspottype_button, q_force_scale_0);

	M_Print(64, 40, "Your name");
	M_DrawTextBox (160, 32, 16, 1);
	M_PrintColored(168, 40, setup_myname);

	M_Print(64, 64, "Shirt color");
	Hotspots_Add (menu_x + 48, menu_y + 64, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0);

	M_Print(64, 88, "Pants color");
	Hotspots_Add (menu_x + 48, menu_y + 88, 320, 8 + 1, 1, hotspottype_slider, q_force_scale_0);

	M_Print(64, 124-8, "Network speed limit");
	M_Print(168, 124, va(vabuf, sizeof(vabuf), "%d (%s)", setup_rate, setup_ratetable[setup_rateindex(setup_rate)].name));
	Hotspots_Add (menu_x + 48, menu_y + 116, 320, 8 + 8 + 1, 1, hotspottype_slider, q_force_scale_0);


	M_DrawTextBox (64, 140-8, 14, 1);
	M_Print(72, 140, "Accept Changes");
	Hotspots_Add (menu_x + 48, menu_y + 140, 320, 8 + 1, 1, hotspottype_button, q_force_scale_0);


	// LadyHavoc: rewrote this code greatly
	if (menuplyr_load) {
		unsigned char *f;
		fs_offset_t filesize;
		menuplyr_load = false;
		menuplyr_top = -1;
		menuplyr_bottom = -1;
		f = FS_LoadFile("gfx/menuplyr.lmp", tempmempool, fs_quiet_true, &filesize);
		if (f && filesize >= 9)
		{
			int width, height;
			width = f[0] + f[1] * 256 + f[2] * 65536 + f[3] * 16777216;
			height = f[4] + f[5] * 256 + f[6] * 65536 + f[7] * 16777216;
			if (filesize >= 8 + width * height)
			{
				menuplyr_width = width;
				menuplyr_height = height;
				menuplyr_pixels = (unsigned char *)Mem_Alloc(cls.permanentmempool, width * height);
				menuplyr_translated = (unsigned int *)Mem_Alloc(cls.permanentmempool, width * height * 4);
				memcpy(menuplyr_pixels, f + 8, width * height);
			}
		}
		if (f)
			Mem_Free(f);
	}

	if (menuplyr_pixels)
	{
		if (menuplyr_top != setup_top || menuplyr_bottom != setup_bottom)
		{
			menuplyr_top = setup_top;
			menuplyr_bottom = setup_bottom;

			for (i = 0;i < menuplyr_width * menuplyr_height;i++)
			{
				j = menuplyr_pixels[i];
				if (j >= TOP_RANGE && j < TOP_RANGE + 16)
				{
					if (menuplyr_top < 8 || menuplyr_top == 14)
						j = menuplyr_top * 16 + (j - TOP_RANGE);
					else
						j = menuplyr_top * 16 + 15-(j - TOP_RANGE);
				}
				else if (j >= BOTTOM_RANGE && j < BOTTOM_RANGE + 16)
				{
					if (menuplyr_bottom < 8 || menuplyr_bottom == 14)
						j = menuplyr_bottom * 16 + (j - BOTTOM_RANGE);
					else
						j = menuplyr_bottom * 16 + 15-(j - BOTTOM_RANGE);
				}
				menuplyr_translated[i] = palette_bgra_transparent[j];
			}
#pragma message ("KEEP ME - MENU_SETUP - The player colors don't change .. not sure why yet")
			Draw_NewPic("gfx/menuplyr", menuplyr_width, menuplyr_height, (unsigned char *)menuplyr_translated, TEXTYPE_BGRA, TEXF_CLAMP);
		}
		M_DrawPic(160, 48, "gfx/bigbox", NO_HOTSPOTS_0, NA0, NA0);
		M_DrawPic(172, 56, "gfx/menuplyr", NO_HOTSPOTS_0, NA0, NA0);
	}

	if (local_cursor == 0)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [local_cursor], 10+((int)(host.realtime*4)&1));
	else
		M_DrawCharacter (56, setup_cursor_table [local_cursor], 12+((int)(host.realtime*4)&1));

	drawsel_idx = local_cursor;

	PPX_DrawSel_End ();
}


static void M_Setup_Key(cmd_state_t *cmd, int key, int ascii)
{
	int			slen;
	char vabuf[1024];

	switch (key) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		M_Menu_MultiPlayer_f(cmd);
		break;

	case K_MOUSE1: if (!Hotspots_DidHit () ) { return;	}  local_cursor = hotspotx_hover; // PPX Key2 fall thru 

	case K_ENTER:
		if (local_cursor == 0)
			return;

		if (local_cursor == 1 || local_cursor == 2 || local_cursor == 3)
			goto forward;

		// local_cursor == 4 (Accept changes)
		if (strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "name " QUOTED_S, setup_myname) );
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "color %d %d", setup_top, setup_bottom) );
		if (setup_rate != setup_oldrate)
			Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "rate %d", setup_rate));

		m_entersound = true;
		M_Menu_MultiPlayer_f(cmd);
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

	case K_LEFTARROW:
leftus:
		if (local_cursor < 1)
			return;
		S_LocalSound ("sound/misc/menu3.wav");
		if (local_cursor == 1)
			setup_top = setup_top - 1;
		if (local_cursor == 2)
			setup_bottom = setup_bottom - 1;
		if (local_cursor == 3) {
			slen = setup_rateindex(setup_rate) - 1;
			if (slen < 0)
				slen = RATES - 1;
			setup_rate = setup_ratetable[slen].rate;
		}
		break;

	case K_RIGHTARROW:
		if (local_cursor < 1)
			return;
forward:
		S_LocalSound ("sound/misc/menu3.wav");
		if (local_cursor == 1)
			setup_top = setup_top + 1;
		if (local_cursor == 2)
			setup_bottom = setup_bottom + 1;
		if (local_cursor == 3)
		{
			slen = setup_rateindex(setup_rate) + 1;
			if (slen >= RATES)
				slen = 0;
			setup_rate = setup_ratetable[slen].rate;
		}
		break;

	case K_BACKSPACE:
		if (local_cursor == 0)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;

	default:
		if (ascii < 32)
			break;
		if (local_cursor == 0) {
			slen = (int)strlen(setup_myname);
			if (slen < 15)
			{
				setup_myname[slen+1] = 0;
				setup_myname[slen] = ascii;
			}
		}
	} // switch

	if (setup_top > 15)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 15;
	if (setup_bottom > 15)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 15;
}

#undef 	local_count
#undef	local_cursor
