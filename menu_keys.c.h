// menu_keys.c.h

#define		m_local_cursor		keys_cursor


//=============================================================================
/* KEYS MENU */

static const char *quakebindnames[][2] =
{
{"+attack", 		"attack"},
{"+jump", 			"jump"},
{"+forward", 		"move forward"},
{"+back", 			"move back"},
{"+moveleft", 		"move left"},
{"+moveright", 		"move right"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"impulse 10", 		"next weapon"},
{"impulse 12", 		"previous weapon"},
{"+speed", 			"run"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+strafe", 		"sidestep"},
{"centerview", 		"center view"},
};


static const char *quake3_quake1bindnames[][2] =
{
{"+attack", 		"attack"},
{"+search", 		"search"},
{"+jump", 			"jump"},
{"+zoom", 			"zoom"},
//{"+drop", 		"drop"},
{"nextweapon", 		"next weapon"},
{"prevweapon", 		"previous weapon"},

{"", 				""},
{"+forward", 		"move forward"},
{"+back", 			"move back"},
{"+moveleft", 		"move left"},
{"+moveright", 		"move right"},
{"", 				""},

{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"+run", 			"run"},

{"", 				""},
{"flashlight", 		"flashlight"},
{"nightvision", 	"nightvision"},
{"+hook", 			"grapple"},
{"messagemode", 	"talk"},
};

static const char *zirconbindnames[][2] =
{
//{"", 				""},
{"+attack", 		"attack"},
{"+search", 		"use"},
{"+jump", 			"jump"},
{"+zoom", 			"zoom"},
{"+drop", 			"drop"},
{"nextweapon", 		"next weapon"},
{"prevweapon", 		"previous weapon"},

{"", 				""},
{"+forward", 		"move forward"},
{"+back", 			"move back"},
{"+moveleft", 		"move left"},
{"+moveright", 		"move right"},
{"", 				""},

{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"+run", 			"run"},

{"", 				""},
{"flashlight", 		"flashlight"},
{"nightvision", 	"nightvision"},
{"+hook", 			"grapple"},
{"messagemode", 	"talk"},
};

static int numcommands;
static const char *(*bindnames)[2];

static int		keys_cursor;
static int		bind_grab;

static int		m_keys_prevstate;
void M_Menu_Keys_f (void)
{
	key_dest = key_menu_grabbed;
	m_keys_prevstate = m_state;
	menu_state_set_nova (m_keys);
	m_entersound = true;

	if (gamemode == GAME_QUAKE3_QUAKE1) {
		numcommands = ARRAY_COUNT(quake3_quake1bindnames);
		bindnames = quake3_quake1bindnames;
	}
	else if (iszirc) {
		numcommands = ARRAY_COUNT(zirconbindnames);
		bindnames = zirconbindnames;
	} else  {
		numcommands = ARRAY_COUNT(quakebindnames);
		bindnames = quakebindnames;
	}

	// Make sure "keys_cursor" doesn't start on a section in the binding list
	keys_cursor = 0;
	while (bindnames[keys_cursor][0][0] == '\0') {
		keys_cursor++;

		// Only sections? There may be a problem somewhere...
		if (keys_cursor >= numcommands)
			Sys_Error ("M_Init: The key binding list only contains sections");
	}
}

#define NUMKEYS 5

static void M_UnbindCommand (const char *command)
{
	int		j;
	const char	*b;

	for (j = 0; j < (int)sizeof (keybindings[0]) / (int)sizeof (keybindings[0][0]); j++) {
		b = keybindings[0][j];
		if (!b)
			continue;
		if (String_Does_Match (b, command))
			Key_SetBinding (j, 0, "");
	} // j
}


static void M_Keys_Draw (void)
{
	int		i, j;
	int		keys[NUMKEYS];
	int		y;
	cachepic_t	*p0;
	char	keystring[MAX_INPUTLINE];

	M_Background(320, 48 + 8 * numcommands);

	p0 = Draw_CachePic ("gfx/ttl_cstm");
	M_DrawPic ( (320-p0->width)/2, 4, p0 /*"gfx/ttl_cstm"*/, NO_HOTSPOTS_0, NA0, NA0);

	if (bind_grab)
		M_Print(12, 32, "Press a key or button for this action");
	else
		M_Print(18, 32, "Enter to change, backspace to clear");

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

// search for known bindings
	for (i = 0 ; i < numcommands ; i++) {
		y = 48 + 8*i;

		// If there's no command, it's just a section
		if (bindnames[i][0][0] == '\0' && bindnames[i][1][0] == '\0') {
			Hotspots_Add (menu_x + 16, menu_y + y, /*wh*/ 0, 0, 1, hotspottype_slider); // Au 15
			continue;
		}
		else if (bindnames[i][0][0] == '\0') {
			M_PrintRed (4, y, "\x0D");  // #13 is the little arrow pointing to the right
			M_PrintRed (16, y, bindnames[i][1]);
			Hotspots_Add (menu_x + 16, menu_y + y, /*wh*/ 0, 0, 1, hotspottype_slider); // Au 15
			continue;
		}
		else
			M_Print(16, y, bindnames[i][1]);

		Key_FindKeysForCommand (bindnames[i][0], keys, NUMKEYS, 0);

		// LadyHavoc: redesigned to print more than 2 keys, inspired by Tomaz's MiniRacer
		if (keys[0] == -1) {
			strlcpy(keystring, "", sizeof(keystring));
		} else {
			char tinystr[2];
			keystring[0] = 0;
			for (j = 0; j < NUMKEYS; j ++) {
				if (keys[j] != -1) {
					const char *s = Key_KeynumToString (keys[j], tinystr, sizeof(tinystr));
					if (iszirc && s == tinystr) {
						tinystr[0] = toupper(tinystr[0]);
					}
					if (j > 0)
						strlcat(keystring, " or ", sizeof(keystring));
					strlcat(keystring, s, sizeof(keystring));
				}
			}
		}
		Hotspots_Add (menu_x + 16, menu_y + y, (40 * 8) /*360*/, 8, 1, hotspottype_slider);
		M_Print(150, y, keystring);
	}

	if (bind_grab)
		M_DrawCharacter (140, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (140, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));

	PPX_DrawSel_End ();
}


static void M_Keys_Key (int k, int ascii)
{
	char	cmd[80];
	int		keys[NUMKEYS];
	char	tinystr[2];

	if (bind_grab) {
		// defining a key
		S_LocalSound ("sound/misc/menu1.wav");
		if (k == K_ESCAPE) {
			bind_grab = false;
		}
		else //if (k != '`')
		{
			dpsnprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString (k, tinystr, sizeof(tinystr)), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { m_local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		if (m_keys_prevstate == m_options_classic) {
			M_Menu_Options_Classic_f ();
		} else {
			M_Menu_OptionsNova_f ();
		}
		break;

	case K_LEFTARROW:
	case K_HOME:
		keys_cursor = 0;
		break;

	case K_END:
		keys_cursor = numcommands - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		do {
			keys_cursor--;
			if (keys_cursor < 0)
				keys_cursor = numcommands - 1;
		}
		while (bindnames[keys_cursor][0][0] == '\0');  // skip sections
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		do {
			keys_cursor++;
			if (keys_cursor >= numcommands)
				keys_cursor = 0;
		}
		while (bindnames[keys_cursor][0][0] == '\0');  // skip sections
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else keys_cursor = hotspotx_hover; // fall thru

	case K_ENTER:		// go into bind mode
		Key_FindKeysForCommand (bindnames[keys_cursor][0], keys, NUMKEYS, 0);
		S_LocalSound ("sound/misc/menu2.wav");
		if (keys[NUMKEYS - 1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DELETE:			// delete bindings
leftus:
		S_LocalSound ("sound/misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}


#undef	m_local_cursor

