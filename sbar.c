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
// sbar.c -- status bar code

#include "quakedef.h"
#include <time.h>
#include "cl_collision.h"
#include "csprogs.h"

cachepic_t *sb_disc;

#define STAT_MINUS 10 // num frame for '-' stats digit
cachepic_t *sb_nums[2][11];
cachepic_t *sb_colon, *sb_slash;
cachepic_t *sb_ibar;
cachepic_t *sb_sbar;
cachepic_t *sb_scorebar;
cachepic_t *sb_backtile; // Baker r1081 Quake backtile
// AK only used by NEX
cachepic_t *sb_sbar_minimal;
cachepic_t *sb_sbar_overlay;

// AK changed the bound to 9
cachepic_t *sb_weapons[7][9]; // 0 is active, 1 is owned, 2-5 are flashes
cachepic_t *sb_ammo[4];
cachepic_t *sb_sigil[4];
cachepic_t *sb_armor[3];
cachepic_t *sb_items[32];

// 0-4 are based on health (in 20 increments)
// 0 is static, 1 is temporary animation
cachepic_t *sb_faces[5][2];
cachepic_t *sb_health; // GAME_NEXUIZ

cachepic_t *sb_face_invis;
cachepic_t *sb_face_quad;
cachepic_t *sb_face_invuln;
cachepic_t *sb_face_invis_invuln;

qbool sb_showscores;

int sb_lines;			// scan lines to draw

cachepic_t *rsb_invbar[2];
cachepic_t *rsb_weapons[5];
cachepic_t *rsb_items[2];
cachepic_t *rsb_ammo[3];
cachepic_t *rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
cachepic_t *hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
int hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
cachepic_t *hsb_items[2];

cachepic_t *sb_ranking;
cachepic_t *sb_complete;
cachepic_t *sb_inter;
cachepic_t *sb_finale;

cvar_t showfps = {CF_CLIENT | CF_ARCHIVE, "showfps", "0", "shows your rendered fps (frames per second)"};
cvar_t showsound = {CF_CLIENT | CF_ARCHIVE, "showsound", "0", "shows number of active sound sources, sound latency, and other statistics"};
cvar_t showblur = {CF_CLIENT | CF_ARCHIVE, "showblur", "0", "shows the current alpha level of motionblur"};
cvar_t showspeed = {CF_CLIENT | CF_ARCHIVE, "showspeed", "0", "shows your current speed (qu per second); number selects unit: 1 = qu/s, 2 = m/s, 3 = km/h, 4 = mph, 5 = knots"};
cvar_t showspeed_factor = {CF_CLIENT | CF_ARCHIVE, "showspeed_factor", "2.54", "multiplier of the centimeter for showspeed. 1 unit = 1 inch in Quake, so this should be 2.54 for Quake, etc"};
cvar_t showtopspeed = {CF_CLIENT | CF_ARCHIVE, "showtopspeed", "0", "shows your top speed (kept on screen for max 3 seconds); value -1 takes over the unit from showspeed, otherwise it's an unit number just like in cl_showspeed"};
cvar_t showtime = {CF_CLIENT | CF_ARCHIVE, "showtime", "0", "shows current time of day (useful on screenshots)"};
cvar_t showtime_format = {CF_CLIENT | CF_ARCHIVE, "showtime_format", "%H:%M:%S", "format string for time of day"};
cvar_t showdate = {CF_CLIENT | CF_ARCHIVE, "showdate", "0", "shows current date (useful on screenshots)"};
cvar_t showdate_format = {CF_CLIENT | CF_ARCHIVE, "showdate_format", "%Y-%m-%d", "format string for date"};
cvar_t showtex = { CF_CLIENT, "showtex", "0", "shows the name of the texture on the crosshair but not for translucent surfaces (for map debugging), showtex 2 will collide with translucent surfaces and shows entity number also [Zircon change]" }; // Baker r7081 showtex 2 for entity information
cvar_t showzmove = { CF_CLIENT, "showzmove", "0", "shows zmove status [Zircon]" }; // Baker r7081 showtex 2 for entity information

cvar_t showpos = { CF_CLIENT, "showpos", "0", "shows player origin [Zircon]" }; // Baker r7082 showpos showangles
cvar_t showangles = { CF_CLIENT, "showangles", "0", "shows player angles [Zircon]" }; // Baker r7082 showpos showangles

cvar_t sbar_alpha_bg = {CF_CLIENT | CF_ARCHIVE, "sbar_alpha_bg", "0.4", "opacity value of the statusbar background image"};
cvar_t sbar_alpha_fg = {CF_CLIENT | CF_ARCHIVE, "sbar_alpha_fg", "1", "opacity value of the statusbar weapon/item icons and numbers"};
cvar_t sbar_hudselector = {CF_CLIENT | CF_ARCHIVE, "sbar_hudselector", "0", "selects which of the builtin hud layouts to use (meaning is somewhat dependent on gamemode, so nexuiz has a very different set of hud layouts than quake for example)"};
cvar_t scr_clock = {CF_CLIENT | CF_ARCHIVE, "scr_clock", "0", "shows game time in status bar [Zircon]"}; // Baker r8085: ProQuake looking clock option
cvar_t sbar_showprotocol = {CF_CLIENT | CF_ARCHIVE, "sbar_showprotocol", "1", "shows client protocol in status bar [Zircon]"}; // Baker

cvar_t sbar_miniscoreboard_size = {CF_CLIENT | CF_ARCHIVE, "sbar_miniscoreboard_size", "-1", "sets the size of the mini deathmatch overlay in items, or disables it when set to 0, or sets it to a sane default when set to -1"};
cvar_t sbar_flagstatus_right = {CF_CLIENT | CF_ARCHIVE, "sbar_flagstatus_right", "0", "moves Nexuiz flag status icons to the right"};
cvar_t sbar_flagstatus_pos = {CF_CLIENT | CF_ARCHIVE, "sbar_flagstatus_pos", "115", "pixel position of the Nexuiz flag status icons, from the bottom"};
cvar_t sbar_quake = {CF_CLIENT | CF_ARCHIVE, "sbar_quake", "0", "1: Draw Quake sbar backtile, 2: Quake Remaster/Quake 64 look [Zircon]"}; // Baker r0081 r1082 status bar 
cvar_t sbar_info_pos = {CF_CLIENT | CF_ARCHIVE, "sbar_info_pos", "0", "pixel position of the info strings (such as showfps), from the bottom"};

cvar_t cl_deathscoreboard = {CF_CLIENT, "cl_deathscoreboard", "1", "shows scoreboard (+showscores) while dead"};

cvar_t crosshair_color_red = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_red", "0.5", "customizable crosshair color [Zircon default]"};  // Baker r8084 crosshair default gray
cvar_t crosshair_color_green = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_green", "0.5", "customizable crosshair color [Zircon default]"};  // Baker r8084 crosshair default gray
cvar_t crosshair_color_blue = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_blue", "0.5", "customizable crosshair color [Zircon default]"};  // Baker r8084 crosshair default gray
cvar_t crosshair_color_alpha = {CF_CLIENT | CF_ARCHIVE, "crosshair_color_alpha", "0.9", "how opaque the crosshair should be [Zircon default]"};  // Baker r8084 crosshair default gray
cvar_t crosshair_size = {CF_CLIENT | CF_ARCHIVE, "crosshair_size", "0.5", "adjusts size of the crosshair on the screen [Zircon default]"};  // Baker r8084 crosshair default gray

static void Sbar_DeathmatchOverlay (void);
static void Sbar_IntermissionOverlay (void);
static void Sbar_FinaleOverlay (void);



/*
===============
Sbar_ShowScores

Tab key down
===============
*/
static void Sbar_ShowScores_f(cmd_state_t *cmd)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	CL_VM_UpdateShowingScoresState(sb_showscores);
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
static void Sbar_DontShowScores_f(cmd_state_t *cmd)
{
	sb_showscores = false;
	CL_VM_UpdateShowingScoresState(sb_showscores);
}

static void sbar_start(void)
{
	char vabuf[1024];
	int i;

	if (gamemode == GAME_BLOODOMNICIDE) {
	}
	else if (IS_OLDNEXUIZ_DERIVED(gamemode))
	{
		for (i = 0;i < 10;i++)
			sb_nums[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/num_%d",i), CACHEPICFLAG_QUIET);
		sb_nums[0][10] = Draw_CachePic_Flags ("gfx/num_minus", CACHEPICFLAG_QUIET);
		sb_colon = Draw_CachePic_Flags ("gfx/num_colon", CACHEPICFLAG_QUIET | CACHEPICFLAG_FAILONMISSING_256);

		sb_ammo[0] = Draw_CachePic_Flags ("gfx/sb_shells", CACHEPICFLAG_QUIET);
		sb_ammo[1] = Draw_CachePic_Flags ("gfx/sb_bullets", CACHEPICFLAG_QUIET);
		sb_ammo[2] = Draw_CachePic_Flags ("gfx/sb_rocket", CACHEPICFLAG_QUIET);
		sb_ammo[3] = Draw_CachePic_Flags ("gfx/sb_cells", CACHEPICFLAG_QUIET);

		sb_armor[0] = Draw_CachePic_Flags ("gfx/sb_armor", CACHEPICFLAG_QUIET);
		sb_armor[1] = NULL;
		sb_armor[2] = NULL;

		sb_health = Draw_CachePic_Flags ("gfx/sb_health", CACHEPICFLAG_QUIET);

		sb_items[2] = Draw_CachePic_Flags ("gfx/sb_slowmo", CACHEPICFLAG_QUIET);
		sb_items[3] = Draw_CachePic_Flags ("gfx/sb_invinc", CACHEPICFLAG_QUIET);
		sb_items[4] = Draw_CachePic_Flags ("gfx/sb_energy", CACHEPICFLAG_QUIET);
		sb_items[5] = Draw_CachePic_Flags ("gfx/sb_str", CACHEPICFLAG_QUIET);

		sb_items[11] = Draw_CachePic_Flags ("gfx/sb_flag_red_taken", CACHEPICFLAG_QUIET);
		sb_items[12] = Draw_CachePic_Flags ("gfx/sb_flag_red_lost", CACHEPICFLAG_QUIET);
		sb_items[13] = Draw_CachePic_Flags ("gfx/sb_flag_red_carrying", CACHEPICFLAG_QUIET);
		sb_items[14] = Draw_CachePic_Flags ("gfx/sb_key_carrying", CACHEPICFLAG_QUIET);
		sb_items[15] = Draw_CachePic_Flags ("gfx/sb_flag_blue_taken", CACHEPICFLAG_QUIET);
		sb_items[16] = Draw_CachePic_Flags ("gfx/sb_flag_blue_lost", CACHEPICFLAG_QUIET);
		sb_items[17] = Draw_CachePic_Flags ("gfx/sb_flag_blue_carrying", CACHEPICFLAG_QUIET);

		sb_sbar = Draw_CachePic_Flags ("gfx/sbar", CACHEPICFLAG_QUIET);
		sb_sbar_minimal = Draw_CachePic_Flags ("gfx/sbar_minimal", CACHEPICFLAG_QUIET);
		sb_sbar_overlay = Draw_CachePic_Flags ("gfx/sbar_overlay", CACHEPICFLAG_QUIET);

		for(i = 0; i < 9;i++)
			sb_weapons[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inv_weapon%d",i), CACHEPICFLAG_QUIET);
	}
	else
	{
		sb_disc = Draw_CachePic_Flags ("gfx/disc", CACHEPICFLAG_QUIET);

		for (i = 0;i < 10;i++)
		{
			sb_nums[0][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/num_%d",i), CACHEPICFLAG_QUIET);
			sb_nums[1][i] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/anum_%d",i), CACHEPICFLAG_QUIET);
		}

		sb_nums[0][10] = Draw_CachePic_Flags ("gfx/num_minus", CACHEPICFLAG_QUIET);
		sb_nums[1][10] = Draw_CachePic_Flags ("gfx/anum_minus", CACHEPICFLAG_QUIET);

		sb_colon = Draw_CachePic_Flags ("gfx/num_colon", CACHEPICFLAG_QUIET | CACHEPICFLAG_FAILONMISSING_256);
		sb_slash = Draw_CachePic_Flags ("gfx/num_slash", CACHEPICFLAG_QUIET);

		sb_weapons[0][0] = Draw_CachePic_Flags ("gfx/inv_shotgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][1] = Draw_CachePic_Flags ("gfx/inv_sshotgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][2] = Draw_CachePic_Flags ("gfx/inv_nailgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][3] = Draw_CachePic_Flags ("gfx/inv_snailgun", CACHEPICFLAG_QUIET);
		sb_weapons[0][4] = Draw_CachePic_Flags ("gfx/inv_rlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[0][5] = Draw_CachePic_Flags ("gfx/inv_srlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[0][6] = Draw_CachePic_Flags ("gfx/inv_lightng", CACHEPICFLAG_QUIET);

		sb_weapons[1][0] = Draw_CachePic_Flags ("gfx/inv2_shotgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][1] = Draw_CachePic_Flags ("gfx/inv2_sshotgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][2] = Draw_CachePic_Flags ("gfx/inv2_nailgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][3] = Draw_CachePic_Flags ("gfx/inv2_snailgun", CACHEPICFLAG_QUIET);
		sb_weapons[1][4] = Draw_CachePic_Flags ("gfx/inv2_rlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[1][5] = Draw_CachePic_Flags ("gfx/inv2_srlaunch", CACHEPICFLAG_QUIET);
		sb_weapons[1][6] = Draw_CachePic_Flags ("gfx/inv2_lightng", CACHEPICFLAG_QUIET);

		for (i = 0;i < 5;i++)
		{
			sb_weapons[2+i][0] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_shotgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][1] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_sshotgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][2] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_nailgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][3] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_snailgun",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][4] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_rlaunch",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][5] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_srlaunch",i+1), CACHEPICFLAG_QUIET);
			sb_weapons[2+i][6] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_lightng",i+1), CACHEPICFLAG_QUIET);
		}

		sb_ammo[0] = Draw_CachePic_Flags ("gfx/sb_shells", CACHEPICFLAG_QUIET);
		sb_ammo[1] = Draw_CachePic_Flags ("gfx/sb_nails", CACHEPICFLAG_QUIET);
		sb_ammo[2] = Draw_CachePic_Flags ("gfx/sb_rocket", CACHEPICFLAG_QUIET);
		sb_ammo[3] = Draw_CachePic_Flags ("gfx/sb_cells", CACHEPICFLAG_QUIET);

		sb_armor[0] = Draw_CachePic_Flags ("gfx/sb_armor1", CACHEPICFLAG_QUIET);
		sb_armor[1] = Draw_CachePic_Flags ("gfx/sb_armor2", CACHEPICFLAG_QUIET);
		sb_armor[2] = Draw_CachePic_Flags ("gfx/sb_armor3", CACHEPICFLAG_QUIET);

		sb_items[0] = Draw_CachePic_Flags ("gfx/sb_key1", CACHEPICFLAG_QUIET);
		sb_items[1] = Draw_CachePic_Flags ("gfx/sb_key2", CACHEPICFLAG_QUIET);
		sb_items[2] = Draw_CachePic_Flags ("gfx/sb_invis", CACHEPICFLAG_QUIET);
		sb_items[3] = Draw_CachePic_Flags ("gfx/sb_invuln", CACHEPICFLAG_QUIET);
		sb_items[4] = Draw_CachePic_Flags ("gfx/sb_suit", CACHEPICFLAG_QUIET);
		sb_items[5] = Draw_CachePic_Flags ("gfx/sb_quad", CACHEPICFLAG_QUIET);

		sb_sigil[0] = Draw_CachePic_Flags ("gfx/sb_sigil1", CACHEPICFLAG_QUIET);
		sb_sigil[1] = Draw_CachePic_Flags ("gfx/sb_sigil2", CACHEPICFLAG_QUIET);
		sb_sigil[2] = Draw_CachePic_Flags ("gfx/sb_sigil3", CACHEPICFLAG_QUIET);
		sb_sigil[3] = Draw_CachePic_Flags ("gfx/sb_sigil4", CACHEPICFLAG_QUIET);

		sb_faces[4][0] = Draw_CachePic_Flags ("gfx/face1", CACHEPICFLAG_QUIET);
		sb_faces[4][1] = Draw_CachePic_Flags ("gfx/face_p1", CACHEPICFLAG_QUIET);
		sb_faces[3][0] = Draw_CachePic_Flags ("gfx/face2", CACHEPICFLAG_QUIET);
		sb_faces[3][1] = Draw_CachePic_Flags ("gfx/face_p2", CACHEPICFLAG_QUIET);
		sb_faces[2][0] = Draw_CachePic_Flags ("gfx/face3", CACHEPICFLAG_QUIET);
		sb_faces[2][1] = Draw_CachePic_Flags ("gfx/face_p3", CACHEPICFLAG_QUIET);
		sb_faces[1][0] = Draw_CachePic_Flags ("gfx/face4", CACHEPICFLAG_QUIET);
		sb_faces[1][1] = Draw_CachePic_Flags ("gfx/face_p4", CACHEPICFLAG_QUIET);
		sb_faces[0][0] = Draw_CachePic_Flags ("gfx/face5", CACHEPICFLAG_QUIET);
		sb_faces[0][1] = Draw_CachePic_Flags ("gfx/face_p5", CACHEPICFLAG_QUIET);

		sb_face_invis = Draw_CachePic_Flags ("gfx/face_invis", CACHEPICFLAG_QUIET);
		sb_face_invuln = Draw_CachePic_Flags ("gfx/face_invul2", CACHEPICFLAG_QUIET);
		sb_face_invis_invuln = Draw_CachePic_Flags ("gfx/face_inv2", CACHEPICFLAG_QUIET);
		sb_face_quad = Draw_CachePic_Flags ("gfx/face_quad", CACHEPICFLAG_QUIET);

		sb_sbar = Draw_CachePic_Flags ("gfx/sbar", CACHEPICFLAG_QUIET);
		sb_ibar = Draw_CachePic_Flags ("gfx/ibar", CACHEPICFLAG_QUIET);

		sb_backtile = Draw_CachePic_Flags ("gfx/backtile", CACHEPICFLAG_QUIET); // Baker r1081: backtile option
		sb_scorebar = Draw_CachePic_Flags ("gfx/scorebar", CACHEPICFLAG_QUIET);

	//MED 01/04/97 added new hipnotic weapons
		if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH)
		{
			hsb_weapons[0][0] = Draw_CachePic_Flags ("gfx/inv_laser", CACHEPICFLAG_QUIET);
			hsb_weapons[0][1] = Draw_CachePic_Flags ("gfx/inv_mjolnir", CACHEPICFLAG_QUIET);
			hsb_weapons[0][2] = Draw_CachePic_Flags ("gfx/inv_gren_prox", CACHEPICFLAG_QUIET);
			hsb_weapons[0][3] = Draw_CachePic_Flags ("gfx/inv_prox_gren", CACHEPICFLAG_QUIET);
			hsb_weapons[0][4] = Draw_CachePic_Flags ("gfx/inv_prox", CACHEPICFLAG_QUIET);

			hsb_weapons[1][0] = Draw_CachePic_Flags ("gfx/inv2_laser", CACHEPICFLAG_QUIET);
			hsb_weapons[1][1] = Draw_CachePic_Flags ("gfx/inv2_mjolnir", CACHEPICFLAG_QUIET);
			hsb_weapons[1][2] = Draw_CachePic_Flags ("gfx/inv2_gren_prox", CACHEPICFLAG_QUIET);
			hsb_weapons[1][3] = Draw_CachePic_Flags ("gfx/inv2_prox_gren", CACHEPICFLAG_QUIET);
			hsb_weapons[1][4] = Draw_CachePic_Flags ("gfx/inv2_prox", CACHEPICFLAG_QUIET);

			for (i = 0;i < 5;i++)
			{
				hsb_weapons[2+i][0] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_laser",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][1] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_mjolnir",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][2] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_gren_prox",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][3] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_prox_gren",i+1), CACHEPICFLAG_QUIET);
				hsb_weapons[2+i][4] = Draw_CachePic_Flags (va(vabuf, sizeof(vabuf), "gfx/inva%d_prox",i+1), CACHEPICFLAG_QUIET);
			}

			hsb_items[0] = Draw_CachePic_Flags ("gfx/sb_wsuit", CACHEPICFLAG_QUIET);
			hsb_items[1] = Draw_CachePic_Flags ("gfx/sb_eshld", CACHEPICFLAG_QUIET);
		}
		else if (gamemode == GAME_ROGUE)
		{
			rsb_invbar[0] = Draw_CachePic_Flags ("gfx/r_invbar1", CACHEPICFLAG_QUIET);
			rsb_invbar[1] = Draw_CachePic_Flags ("gfx/r_invbar2", CACHEPICFLAG_QUIET);

			rsb_weapons[0] = Draw_CachePic_Flags ("gfx/r_lava", CACHEPICFLAG_QUIET);
			rsb_weapons[1] = Draw_CachePic_Flags ("gfx/r_superlava", CACHEPICFLAG_QUIET);
			rsb_weapons[2] = Draw_CachePic_Flags ("gfx/r_gren", CACHEPICFLAG_QUIET);
			rsb_weapons[3] = Draw_CachePic_Flags ("gfx/r_multirock", CACHEPICFLAG_QUIET);
			rsb_weapons[4] = Draw_CachePic_Flags ("gfx/r_plasma", CACHEPICFLAG_QUIET);

			rsb_items[0] = Draw_CachePic_Flags ("gfx/r_shield1", CACHEPICFLAG_QUIET);
			rsb_items[1] = Draw_CachePic_Flags ("gfx/r_agrav1", CACHEPICFLAG_QUIET);

	// PGM 01/19/97 - team color border
			rsb_teambord = Draw_CachePic_Flags ("gfx/r_teambord", CACHEPICFLAG_QUIET);
	// PGM 01/19/97 - team color border

			rsb_ammo[0] = Draw_CachePic_Flags ("gfx/r_ammolava", CACHEPICFLAG_QUIET);
			rsb_ammo[1] = Draw_CachePic_Flags ("gfx/r_ammomulti", CACHEPICFLAG_QUIET);
			rsb_ammo[2] = Draw_CachePic_Flags ("gfx/r_ammoplasma", CACHEPICFLAG_QUIET);
		}
	}

	sb_ranking = Draw_CachePic_Flags ("gfx/ranking", CACHEPICFLAG_QUIET);
	sb_complete = Draw_CachePic_Flags ("gfx/complete", CACHEPICFLAG_QUIET);
	sb_inter = Draw_CachePic_Flags ("gfx/inter", CACHEPICFLAG_QUIET);
	sb_finale = Draw_CachePic_Flags ("gfx/finale", CACHEPICFLAG_QUIET);
}

static void sbar_shutdown(void)
{
}

static void sbar_newmap(void)
{
}



//=============================================================================

// drawing routines are relative to the status bar location

int sbar_x, sbar_y;

/*
=============
Sbar_DrawPic
=============
*/
//static void Sbar_DrawStretchPic (int x, int y, cachepic_t *pic, float alpha, float overridewidth, float overrideheight)
//{
//	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, overridewidth, overrideheight, 1, 1, 1, alpha, 0);
//}

static void Sbar_DrawPic (int x, int y, cachepic_t *pic)
{
	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, 0, 0, 1, 1, 1, sbar_alpha_fg.value, 0);
}

static void Sbar_DrawAlphaPic (int x, int y, cachepic_t *pic, float alpha)
{
	DrawQ_Pic (sbar_x + x, sbar_y + y, pic, 0, 0, 1, 1, 1, alpha, 0);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
static void Sbar_DrawCharacter (int x, int y, int num)
{
	char vabuf[1024];
	DrawQ_String (sbar_x + x + 4 , sbar_y + y, va(vabuf, sizeof(vabuf), "%c", num), 0, 8, 8, 1, 1, 1, sbar_alpha_fg.value, 0, NULL, true, FONT_SBAR);
}

/*
================
Sbar_DrawString
================
*/
static void Sbar_DrawString (int x, int y, const char *str)
{
	DrawQ_String (sbar_x + x, sbar_y + y, str, 0, 8, 8, 1, 1, 1, sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR);
}

/*
=============
Sbar_DrawNum
=============
*/
static void Sbar_DrawNum (int x, int y, int num, int numdigits, int numscoloridx)
{
	char str[32], *ptr;
	int slen, frame;

	slen = dpsnprintf(str, sizeof(str), "%d", num);
	ptr = str;
	if (slen > numdigits)
		ptr += (slen-numdigits);
	if (slen < numdigits)
		x += (numdigits-slen)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawPic (x, y, sb_nums[numscoloridx][frame]);
		x += 24;

		ptr++;
	}
}

WARP_X_ (SBar_Quake)
static void Sbar_DrawNum2 (int x, int y, int lettersize, int num, int numdigits, int numscoloridx)
{
	char str[32], *sptr;
	int slen, frame;

	slen = c_dpsnprintf1 (str, "%d", num);
	sptr = str;

	if (slen > numdigits)
		sptr += (slen - numdigits);

	if (slen < numdigits)
		x += (numdigits - slen) * lettersize;

	while (*sptr) {
		if (*sptr == '-')
			frame = STAT_MINUS;
		else
			frame = *sptr -'0';

		DrawQ_Pic (
			x,
			y,
			sb_nums[numscoloridx][frame],
			lettersize,								// w
			lettersize,								// h
			1,1,1, sbar_alpha_fg.value,					// color quad r
			DRAWFLAG_NORMAL_0
		);

		x += lettersize;

		sptr++;
	}
}

/*
=============
Sbar_DrawXNum
=============
*/

//static void Sbar_DrawXNum (int x, int y, int num, int numdigits, int lettersize, float r, float g, float b, float a, int flags)
//{
//	char str[32], *sptr;
//	int l, frame;
//
//	if (numdigits < 0)
//	{
//		numdigits = -numdigits;
//		l = dpsnprintf(str, sizeof(str), "%0*d", numdigits, num);
//	}
//	else
//		l = dpsnprintf(str, sizeof(str), "%d", num);
//	sptr = str;
//	if (l > numdigits)
//		sptr += (l-numdigits);
//	if (l < numdigits)
//		x += (numdigits-l) * lettersize;
//
//	while (*sptr) {
//		if (*sptr == '-')
//			frame = STAT_MINUS;
//		else
//			frame = *sptr -'0';
//
//		DrawQ_Pic (
//			sbar_x + x, 
//			sbar_y + y, 
//			sb_nums[0][frame],
//			lettersize, // w
//			lettersize, // h
//			r,g,b,a * sbar_alpha_fg.value, // color quad
//			flags
//		);
//
//		x += lettersize;
//
//		sptr++;
//	}
//}

//=============================================================================


static int Sbar_IsTeammatch(void)
{
	// currently only nexuiz uses the team score board
	return (IS_OLDNEXUIZ_DERIVED(gamemode)
		&& (teamplay.integer > 0));
}

/*
===============
Sbar_SortFrags
===============
*/
static int fragsort[MAX_SCOREBOARD_255];
static int scoreboardlines;

int Sbar_GetSortedPlayerIndex (int index)
{
	return index >= 0 && index < scoreboardlines ? fragsort[index] : -1;
}

static scoreboard_t teams[MAX_SCOREBOARD_255];
static int teamsort[MAX_SCOREBOARD_255];
static int teamlines;
void Sbar_SortFrags (void)
{
	int i, j, k, color;

	// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.maxclients ; i++) {
		if (cl.scores[i].name[0]) {
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.scores[fragsort[j]].frags < cl.scores[fragsort[j+1]].frags) {
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}

	teamlines = 0;
	if (Sbar_IsTeammatch ()) {
		// now sort players by teams.
		for (i=0 ; i<scoreboardlines ; i++) {
			for (j=0 ; j<scoreboardlines-1-i ; j++) {
				if (cl.scores[fragsort[j]].colors < cl.scores[fragsort[j+1]].colors) {
					k = fragsort[j];
					fragsort[j] = fragsort[j+1];
					fragsort[j+1] = k;
				}
			}
		}

		// calculate team scores
		color = -1;
		for (i = 0 ; i<scoreboardlines ; i++) {
			if (color != (cl.scores[fragsort[i]].colors & 15)) {
				const char *teamname;

				color = cl.scores[fragsort[i]].colors & 15;
				teamlines++;

				switch (color) {
					case 4:
						teamname = "^1Red Team";
						break;
					case 13:
						teamname = "^4Blue Team";
						break;
					case 9:
						teamname = "^6Pink Team";
						break;
					case 12:
						teamname = "^3Yellow Team";
						break;
					default:
						teamname = "Total Team Score";
						break;
				}
				strlcpy(teams[teamlines-1].name, teamname, sizeof(teams[teamlines-1].name));

				teams[teamlines-1].frags = 0;
				teams[teamlines-1].colors = color + 16 * color;
			}

			if (cl.scores[fragsort[i]].frags != NEXUIZ_OBS_NEG_666) {
				// do not add spedcators
				// (ugly hack for nexuiz)
				teams[teamlines-1].frags += cl.scores[fragsort[i]].frags;
			}
		}

		// now sort teams by scores.
		for (i=0 ; i<teamlines ; i++)
			teamsort[i] = i;
		for (i=0 ; i<teamlines ; i++) {
			for (j=0 ; j<teamlines-1-i ; j++) {
				if (teams[teamsort[j]].frags < teams[teamsort[j+1]].frags)
				{
					k = teamsort[j];
					teamsort[j] = teamsort[j+1];
					teamsort[j+1] = k;
				}
			}
		}
	}
}

/*
===============
Sbar_SoloScoreboard
===============
*/
static void Sbar_SoloScoreboard (void)
{
	extern cvar_t cl_worldbasename;
	char	str[80];
	char 	vabuf[1024];

	int		max_32 = 32;

mapname:
	if (cl.islocalgame)
		c_dpsnprintf1 (str, "%s", cl.worldmessage);
	else c_dpsnprintf2 (str, "%s: %s", cl_worldbasename.string, cl.worldmessage);

	// if there's a newline character, terminate the string there
	if (strchr(str, NEWLINE_CHAR_10))
		*(strchr(str, NEWLINE_CHAR_10)) = 0;

	// truncate the level name if necessary
	if ((int)strlen(str) >= max_32)
		str[max_32] = 0;

	// print the filename and message
	Sbar_DrawString (8 + 8 * (20 - strlen(str) / 2.0) , 4, str);

levtime:

	if (1 || sb_showscores) {
		int hours = Time_Hours ((int)cl.time);
		int minutes = Time_Minutes_Less_Hours ((int)cl.time);
		int seconds = Time_Seconds ((int)cl.time);
		if (hours) {
			dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d:%02d", hours, minutes, seconds);
		} else {
			dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d", minutes, seconds);
		}

		Sbar_DrawString ((320 - 8 -3 ) - (int)strlen(vabuf) * 8, -24, vabuf);
	}


	if ((cl.islocalgame || cl.skill_level_p1) && cl.gametype == GAME_COOP ) {
		const char *skillstrings[] = {"Easy", "Normal", "Hard", "Nightmare"};
		int skillz = cl.skill_level_p1 ? (cl.skill_level_p1 - 1) : skill.integer; // Baker r8191
		if (in_range (/*easy*/ 0, skillz, /*nightmare*/ 3)) {
			Sbar_DrawString (8 + 8 * (20 - strlen(skillstrings[skillz]) / 2.0), 12, skillstrings[skillz]);
		}
	}

	// monsters and secrets are now both on the top row
	Sbar_DrawString (8, 12, va(vabuf, sizeof(vabuf), "Kills: %d/%d", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]));
	Sbar_DrawString (8 + 26 * 8, 12, va(vabuf, sizeof(vabuf), "Secrets: %d/%d", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]));

	if (sbar_showprotocol.integer && (!cl.islocalgame || cl.movement_predicted) ) {
		// Baker: Nehahra movie is Nehahra demo format
		if (isin2 (cls.protocol, PROTOCOL_QUAKE, PROTOCOL_QUAKEDP)) {
			Sbar_DrawString (8, 4, "Q15");
		} else if (isin1 (cls.protocol, PROTOCOL_DARKPLACES7)) {
			if (cl.movement_predicted == PRED_ZIRCON_MOVE_2) {
				Sbar_DrawString (8, 2, "ZIRCON");
			} else {
				Sbar_DrawString (8, 4, "DP7");
			}
		} else {
			Sbar_DrawString (8, 4, (char *)Protocol_NameForEnum(cls.protocol));
		}
	} else {


	}

}

/*
===============
Sbar_DrawScoreboard
===============
*/
static void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	// LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode
	//if (cl.gametype == GAME_DEATHMATCH)
	if ((cls.demoplayback && cl.gametype == GAME_DEATHMATCH) || 
		(!cl.islocalgame && !cls.demoplayback) && key_dest == key_game)
		Sbar_DeathmatchOverlay ();
}

//=============================================================================


/*
===============
Sbar_DrawInventory
===============
*/
static void Sbar_DrawInventory (void)
{
	int i;
	char num[6];
	float time;
	int flashon;

	if (gamemode == GAME_ROGUE) {
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN ) 
			Sbar_DrawAlphaPic (0, -24, rsb_invbar[0], sbar_alpha_bg.value);
		else
			Sbar_DrawAlphaPic (0, -24, rsb_invbar[1], sbar_alpha_bg.value);
	}
	else
		Sbar_DrawAlphaPic (0, -24, sb_ibar, sbar_alpha_bg.value);

	// weapons
	for (i = 0 ; i < 7 ; i ++) {
		if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN<<i) ) {
			time = cl.item_gettime[i];
			flashon = (int)(max(0, cl.time - time)*10);
			if (flashon >= 10) {
				if ( cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  ) flashon = 1;
				else flashon = 0;
			} else flashon = (flashon%5) + 2;

			Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);
		}
	}

	// MED 01/04/97
	// hipnotic weapons
	if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) {
		int grenadeflashing=0;
		for (i = 0 ; i < 4 ; i ++) {
			if (cl.stats[STAT_ITEMS] & (1<<hipweapons[i]) ) {
				time = max(0, cl.item_gettime[hipweapons[i]]);
				flashon = (int)((cl.time - time)*10);
				if (flashon >= 10) {
					if ( cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i])  ) flashon = 1;
					else flashon = 0;
				}
				else flashon = (flashon%5) + 2;

				// check grenade launcher
				if (i==2) {
					if (cl.stats[STAT_ITEMS] & HIT_PROXIMITY_GUN) {
						if (flashon) {
							grenadeflashing = 1;
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
						}
					}
				}
				else if (i==3) {
					if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN<<4)) {
						if (!grenadeflashing)
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
					}
					else Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
				}
				else Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);
			}
		}
	}

	if (gamemode == GAME_ROGUE) {
		// check for powered up weapon.
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			for (i=0;i<5;i++)
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
	}

	// ammo counts
	for (i=0 ; i<4 ; i++) {
		dpsnprintf (num, sizeof(num), "%4i",cl.stats[STAT_SHELLS+i] );
		if (num[0] != ' ')	Sbar_DrawCharacter ( (6*i+0)*8 - 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')	Sbar_DrawCharacter ( (6*i+1)*8 - 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')	Sbar_DrawCharacter ( (6*i+2)*8 - 2, -24, 18 + num[2] - '0');
		if (num[3] != ' ')	Sbar_DrawCharacter ( (6*i+3)*8 - 2, -24, 18 + num[3] - '0');
	}

	// items
	for (i=0 ; i<6 ; i++) {
		if (cl.stats[STAT_ITEMS] & (1<<(17+i))) {
			//MED 01/04/97 changed keys
			if (!(gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) || (i>1))
				Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
		}
	}

	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) {
		for (i=0 ; i<2 ; i++)
			if (cl.stats[STAT_ITEMS] & (1<<(24+i)))
				Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
	}

	if (gamemode == GAME_ROGUE) {
		// new rogue items
		for (i=0 ; i<2 ; i++)
			if (cl.stats[STAT_ITEMS] & (1<<(29+i)))
				Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
	} else {
		// sigils
		for (i=0 ; i<4 ; i++)
			if (cl.stats[STAT_ITEMS] & (1<<(28+i)))
				Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
static void Sbar_DrawFrags (void)
{
	int i, k, numscorelines, x, f;
	char num[12];
	scoreboard_t *s;
	unsigned char *c;

	Sbar_SortFrags ();

	// draw the text
	numscorelines = Smallest(scoreboardlines, 4);
	if (cl.gametype == GAME_DEATHMATCH && scr_clock.value || sb_showscores || scr_clock.value != 0)
		numscorelines = Smallest (numscorelines, 2);

	x = 23 * 8;

	for (i = 0; i < numscorelines;i++) {
		k = fragsort[i];
		s = &cl.scores[k];

		// draw background
		c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
		DrawQ_Fill (sbar_x + x + 10, sbar_y     - 23, 28, 4, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
		c = palette_rgb_shirtscoreboard[s->colors & 0xf];
		DrawQ_Fill (sbar_x + x + 10, sbar_y + 4 - 23, 28, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);

		// draw number
		f = s->frags;
		dpsnprintf (num, sizeof(num), "%3d",f);

		if (k == cl.viewentity - 1) {  // brackets around scores
			Sbar_DrawCharacter ( x      + 2, -24, 16);
			Sbar_DrawCharacter ( x + 32 - 4, -24, 17);
		}
		Sbar_DrawCharacter (x +  8, -24, num[0]);
		Sbar_DrawCharacter (x + 16, -24, num[1]);
		Sbar_DrawCharacter (x + 24, -24, num[2]);
		x += 32;
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
static void Sbar_DrawFace (void)
{
	int f;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if (gamemode == GAME_ROGUE && !cl.islocalgame && (teamplay.integer > 3) && (teamplay.integer < 7)) {
		char num[12];
		scoreboard_t *s;
		unsigned char *c;

		s = &cl.scores[cl.viewentity - 1];
		// draw background
		Sbar_DrawPic (112, 0, rsb_teambord);
		c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
		DrawQ_Fill (sbar_x + 113, vid_conheight.integer-SBAR_HEIGHT+3, 22, 9, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
		c = palette_rgb_shirtscoreboard[s->colors & 0xf];
		DrawQ_Fill (sbar_x + 113, vid_conheight.integer-SBAR_HEIGHT+12, 22, 9, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);

		// draw number
		f = s->frags;
		dpsnprintf (num, sizeof(num), "%3i",f);

		if ((s->colors & 0xf0)==0) {
			if (num[0] != ' ')	Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
			if (num[1] != ' ')	Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
			if (num[2] != ' ')	Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter ( 109, 3, num[0]);
			Sbar_DrawCharacter ( 116, 3, num[1]);
			Sbar_DrawCharacter ( 123, 3, num[2]);
		}

		return;
	}
// PGM 01/19/97 - team color drawing

	if ( (cl.stats[STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY) ) == (IT_INVISIBILITY | IT_INVULNERABILITY) )
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
	else if (cl.stats[STAT_ITEMS] & IT_QUAD)	Sbar_DrawPic (112, 0, sb_face_quad );
	else if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)	Sbar_DrawPic (112, 0, sb_face_invis );
	else if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)	Sbar_DrawPic (112, 0, sb_face_invuln);
	else {
		f = cl.stats[STAT_HEALTH] / 20;
		f = bound(0, f, 4);
		Sbar_DrawPic (112, 0, sb_faces[f][cl.time <= cl.faceanimtime]);
	}
}

double topspeed = 0;
double topspeedxy = 0;
time_t current_time = 3;
time_t top_time = 0;
time_t topxy_time = 0;

static void get_showspeed_unit(int unitnumber, double *conversion_factor, const char **unit)
{
	if (unitnumber < 0)
		unitnumber = showspeed.integer;
	switch(unitnumber) {
		default:
		case 1:
			*unit = "qu/s";
			*conversion_factor = 1.0;
			break;
		case 2:
			*unit = "m/s";
			*conversion_factor = 1.0 * showspeed_factor.value * 0.01;
			break;
		case 3:
			*unit = "km/h";
			*conversion_factor = 1.0 * (showspeed_factor.value * 0.01) * 3.6;
			break;
		case 4:
			*unit = "mph";
			*conversion_factor = 1.0 * (showspeed_factor.value * 0.01 * 3.6) * 0.6213711922;
			break;
		case 5:
			*unit = "knots";
			*conversion_factor = 1.0 * (showspeed_factor.value * 0.01 * 3.6) * 0.539957;
			break;
	} // sw
}

static double showfps_nexttime = 0, showfps_lasttime = -1;
static double showfps_framerate = 0;
static int showfps_framecount = 0;

void Sbar_ShowFPS_Update(void)
{
	double interval = 1;
	double newtime;
	newtime = host.realtime;
	if (newtime >= showfps_nexttime) {
		showfps_framerate = showfps_framecount / (newtime - showfps_lasttime);
		if (showfps_nexttime < newtime - interval * 1.5)
			showfps_nexttime = newtime;
		showfps_lasttime = newtime;
		showfps_nexttime += interval;
		showfps_framecount = 0;

	}
	showfps_framecount++;
}

void SBar_DrawTexture (char *texstring) {
	WARP_X_ (R_DebugLine)

	model_t *mod = CL_Mesh_UI();
	// int materialflags = MATERIALFLAG_WALL;
	//qbool hascolor = false; // We are doing 1, 1,1, 
	//qbool hasalpha = false; // We are doing alpha 1

	// Baker: Try to load from q1 map
	texture_t *tex = NULL;

	for (int n = 0; n < cl.worldmodel->num_textures; n ++) {
		texture_t *tx = &cl.worldmodel->data_textures[n];

		if (String_Match(tx->name, texstring)) {
			tex = tx;
			break;
		} // if
	} // for

	// If Q1 map texture fails, use shader
	if (!tex) {
		tex = Mod_Mesh_GetTexture(mod, texstring, DRAWFLAG_NORMAL_0, /*texflags*/ 0, 
			MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | 
			MATERIALFLAG_ALPHA | 
			MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW);
	}

	// Baker: Sometimes we can get nothing like looking at water with collision against translucents off
	if (tex) {
		msurface_t *surf = Mod_Mesh_AddSurface(mod, 
			tex, /*batchwithprevsurface*/ false /*true*/);
		int e0 = Mod_Mesh_IndexForVertex(mod, surf, 10, 10, 0, /*normals*/ 0, 0, -1, /*st*/ 0, 0, /*uv*/ 0, 0, /*rgba*/ 1, 1, 1, alpha_1_0);
		int e1 = Mod_Mesh_IndexForVertex(mod, surf, 10 + vid_conwidth.value / 8, 10, 0, /*normals*/ 0, 0, -1, 
			/*st*/ 1, 0, 0, 0, 1, 1, 1, alpha_1_0);
		int e2 = Mod_Mesh_IndexForVertex(mod, surf, 10 + vid_conwidth.value / 8, 10 + vid_conheight.value / 8, 0,/*normals*/ 0, 0, -1, 
			/*st*/ 1, 1, 0, 0, 1, 1, 1, alpha_1_0);
		int e3 = Mod_Mesh_IndexForVertex(mod, surf, 10, 10 + vid_conheight.value / 8, 0, /*normals*/ 0, 0, -1, 
			/*st*/ 0, 1, 0, 0, 1, 1, 1, alpha_1_0);
		Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
		Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
	}

	// DrawQ_Pic(10, 10, tex, vid_conwidth.value / 8, vid_conheight.value / 8, /*rgba*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0);
}

void Sbar_ShowFPS(void)
{
	float fps_x, fps_y, fps_scalex, fps_scaley, fps_strings = 0;
	char soundstring[32];
	char fpsstring[32];
	char timestring[32];
	char datestring[32];
	char posstring[32];
	char angstring[32];
	char timedemostring1[32];
	char timedemostring2[32];
	char speedstring[32];
	char blurstring[32];
	char topspeedstring[48];
	char texstring[MAX_QPATH_128];
	char entstring[32];
	qbool red = false;
	soundstring[0] = 0;

	fpsstring[0] = 0;
	timedemostring1[0] = 0;
	timedemostring2[0] = 0;
	timestring[0] = 0;
	datestring[0] = 0;
	speedstring[0] = 0;
	blurstring[0] = 0;
	texstring[0] = 0;
	topspeedstring[0] = 0;
	posstring[0] = 0;
	angstring[0] = 0;	
	entstring[0] = 0;
	
	if (showfps.integer && showfps_framerate >= 0.00001 && cls.signon == SIGNONS_4) {  // Baker r8083
		red = (showfps_framerate < 1.0f);
		if (showfps.integer == 2)
			dpsnprintf(fpsstring, sizeof(fpsstring), "%7.3f mspf", (1000.0 / showfps_framerate));
		else if (showfps.integer < 0) {
			extern cvar_t cl_maxfps;
			//#pragma message ("Baker: TODO REMOVE SHOWFPS HACK FOR FIRST FRAME")
			if (cl_maxfps.value && showfps_framerate > cl_maxfps.value * 2) {
				// Baker: this is a hack I'll do the right way later
				// The first fps per level is wildly high
				dpsnprintf(fpsstring, sizeof(fpsstring), "%4d", (int)(cl_maxfps.value + 0.5));
			}
			else {
				if (red)	dpsnprintf(fpsstring, sizeof(fpsstring), "%4d", (int)(1.0 / showfps_framerate + 0.5));
				else		dpsnprintf(fpsstring, sizeof(fpsstring), "%4d", (int)(showfps_framerate + 0.5));
			}
		}
		else if (red)
			dpsnprintf(fpsstring, sizeof(fpsstring), "%4i spf", (int)(1.0 / showfps_framerate + 0.5));
		else
			dpsnprintf(fpsstring, sizeof(fpsstring), "%4i fps", (int)(showfps_framerate + 0.5));

		if (showfps.integer > 0)
			fps_strings++;

		if (cls.timedemo)
		{
			c_dpsnprintf2(timedemostring1, "frame%4d %f", cls.td_frames, host.realtime - cls.td_starttime);
			c_dpsnprintf4(timedemostring2, "%d seconds %3.0f/%3.0f/%3.0f fps", 
					cls.td_onesecondavgcount, 
					cls.td_onesecondminfps, 
					cls.td_onesecondavgfps / max(1, cls.td_onesecondavgcount), 
				cls.td_onesecondmaxfps);
			fps_strings++;
			fps_strings++;
		}
	}

	if (showpos.integer) {
		if (cl.entities) {
			c_dpsnprintf3(posstring, "%d %d %d", (int)cl.entities[cl.playerentity].state_current.origin[0], (int)cl.entities[cl.playerentity].state_current.origin[1], (int)cl.entities[cl.playerentity].state_current.origin[2]);
			fps_strings++;
		}
	}
	if (showangles.integer) {
		if (cl.entities) {
			c_dpsnprintf3(angstring, "%d %d %d", (int)cl.entities[cl.playerentity].state_current.angles[0], (int)cl.entities[cl.playerentity].state_current.angles[1], (int)cl.entities[cl.playerentity].state_current.angles[2]);
			fps_strings++;
		}
	}

	if (showtime.integer) {
		c_strlcpy(timestring, Sys_TimeString(showtime_format.string));
		fps_strings++;
	}
	if (showdate.integer) {
		c_strlcpy(datestring, Sys_TimeString(showdate_format.string));
		fps_strings++;
	}
	if (showblur.integer) {
		c_dpsnprintf1(blurstring, "%3d%% blur", (int)(cl.motionbluralpha * 100));
		fps_strings++;
	}
	if (showsound.integer) {
		c_dpsnprintf3(soundstring, "%4d/4%d at %3dms", cls.soundstats.mixedsounds, cls.soundstats.totalsounds, cls.soundstats.latency_milliseconds);
		fps_strings++;
	}
	if (showspeed.integer || showtopspeed.integer) {
		double speed, speedxy, f;
		const char *unit;
		speed = VectorLength(cl.movement_velocity);
		speedxy = sqrt(cl.movement_velocity[0] * cl.movement_velocity[0] + cl.movement_velocity[1] * cl.movement_velocity[1]);
		if (showspeed.integer) {
			get_showspeed_unit(showspeed.integer, &f, &unit);
			dpsnprintf(speedstring, sizeof(speedstring), "%.0f (%.0f) %s", f*speed, f*speedxy, unit);
			fps_strings++;
		}
		if (showtopspeed.integer) {
			qbool topspeed_latched = false, topspeedxy_latched = false;
			get_showspeed_unit(showtopspeed.integer, &f, &unit);
			if (speed >= topspeed || current_time - top_time > 3)
			{
				topspeed = speed;
				time(&top_time);
			}
			else
				topspeed_latched = true;
			if (speedxy >= topspeedxy || current_time - topxy_time > 3)
			{
				topspeedxy = speedxy;
				time(&topxy_time);
			}
			else
				topspeedxy_latched = true;
			dpsnprintf(topspeedstring, sizeof(topspeedstring), "%s%.0f%s (%s%.0f%s) %s",
				topspeed_latched ? "^1" : "^xf88", f*topspeed, "^xf88",
				topspeedxy_latched ? "^1" : "^xf88", f*topspeedxy, "^xf88",
				unit);
			time(&current_time);
			fps_strings++;
		}
	}

	if (showtex.integer) {
		vec3_t org;
		vec3_t dest;
		vec3_t temp;
		trace_t cltrace = {0};
		int hitnetentity = -1;

		Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&r_refdef.view.matrix, temp, dest);
		// clear the traces as we may or may not fill them out, and mark them with an invalid fraction so we know if we did
		memset(&cltrace, 0, sizeof(cltrace));
		cltrace.fraction = 2.0;
		
		cltrace = CL_TraceLine(org, dest, MOVE_HITMODEL_4, NULL, 
				SUPERCONTENTS_SOLID | SUPERCONTENTS_WATER | SUPERCONTENTS_SLIME | 
				SUPERCONTENTS_LAVA | SUPERCONTENTS_SKY
			, 
			0, 
			showtex.integer >= 2 ? MATERIALFLAGMASK_TRANSLUCENT : 0, collision_extendmovelength.value, 
			true, false, &hitnetentity, true, true);
		if (cltrace.hittexture)
			c_strlcpy(texstring, cltrace.hittexture->name);
		else
			c_strlcpy(texstring, "(no texture hit)");
		

		if (cltrace.hittexture) {
			SBar_DrawTexture (texstring);
		} // cltrace.hittexture

		fps_strings++;

		// Baker r7081: Entity information requires showtex 2
		if (showtex.integer >= 2) {
			trace_t svtrace = {0};
			svtrace.fraction = 2.0;
			// ray hits models (even animated ones) and ignores translucent materials
			if (showtex.integer < 5 && sv.active) {
				svtrace = SV_TraceLine(org, dest, MOVE_HITMODEL_4, NULL, SUPERCONTENTS_SOLID, 0, 
				MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value);
			}

			if (svtrace.fraction < cltrace.fraction) {
				if (svtrace.ent != NULL) {
					prvm_prog_t *prog = SVVM_prog;
					c_dpsnprintf1(entstring, "server entity %d", (int)PRVM_EDICT_TO_PROG(svtrace.ent));
				} else {
					c_strlcpy(entstring, "(no entity hit)");
				}
			} else {
				if (cltrace.ent != NULL) {
					prvm_prog_t *prog = CLVM_prog;
					c_dpsnprintf1(entstring, "client entity %d", (int)PRVM_EDICT_TO_PROG(cltrace.ent));
				}
				else if (hitnetentity > 0)
					c_dpsnprintf1(entstring, "network entity %d", hitnetentity);
				else if (hitnetentity == 0)
					c_strlcpy(entstring, "world entity");
				else
					c_strlcpy(entstring, "(no entity hit)");
			}
			fps_strings++;
		} // showtex >= 2
	}

	int is_zmove_ike = showzmove.integer;
	// Zircon move needs removed by server if not DP7
	if (is_zmove_ike) {
		char tbuf[1024];
		fps_scalex = 12;
		fps_scaley = 12;
		const char *s = "NO EXIST";
		if (Have_Zircon_Ext_Flag_CLS (ZIRCON_EXT_FREEMOVE_4)) {
			if (cl.movement_predicted == PRED_ZIRCON_MOVE_2) 
				s = "Z";
			else s = va(tbuf, sizeof(tbuf), "DP7 (WARP @ %u) SV: %u SEQ: %u", cl.zircon_warp_sequence, cls.servermovesequence, cl.mcmd.clx_sequence);
		}
		//float tw = DrawQ_TextWidth(fpsstring, 0, fps_scalex, fps_scaley, true, FONT_CENTERPRINT);
		//fps_x = vid_conwidth.integer - tw - fps_scalex * 2;
		DrawQ_String (fps_scalex, fps_scaley, s, /*maxlen*/ 0, fps_scalex, fps_scaley, /*rgba*/ 1, 1, 1, 1, /*flags*/ 0, NULL, /*ig color codes*/ true, FONT_INFOBAR);
	}

	if (fps_strings || showfps.integer < 0 || is_zmove_ike) {
		fps_scalex = 12;
		fps_scaley = 12;
		//fps_y = vid_conheight.integer - sb_lines; // yes this may draw over the sbar
		//fps_y = bound(0, fps_y, vid_conheight.integer - fps_strings*fps_scaley);
		fps_y = vid_conheight.integer - sbar_info_pos.integer - fps_strings*fps_scaley;
		if (soundstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(soundstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, soundstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (fpsstring[0])
		{
			r_draw2d_force = true;

			// Baker r7083 showfps -1 displays frames per second in top right corner
			if (showfps.integer < 0) {
				if (!key_consoleactive && cl.time > 1) {
					float tw = DrawQ_TextWidth(fpsstring, 0, fps_scalex, fps_scaley, true, FONT_CENTERPRINT);
					fps_x = vid_conwidth.integer - tw - fps_scalex * 2;
					DrawQ_String(fps_x, fps_scaley, fpsstring, /*maxlen*/ 0, fps_scalex, fps_scaley, /*rgba*/ 1, 1, 1, 1, /*flags*/ 0, NULL, /*ig color codes*/ true, FONT_INFOBAR);
				} // console active
			} else {
				fps_x = vid_conwidth.integer - DrawQ_TextWidth(fpsstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);

				DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
				DrawQ_String(fps_x, fps_y, fpsstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR); // Baker r8086: showfps is never red
				fps_y += fps_scaley;
			}

			r_draw2d_force = false;
		}

		if (posstring[0]) {
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(posstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, posstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (angstring[0]) {
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(angstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, angstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (timedemostring1[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timedemostring1, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timedemostring1, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (timedemostring2[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timedemostring2, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timedemostring2, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (timestring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(timestring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, timestring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (datestring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(datestring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, datestring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (speedstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(speedstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, speedstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (topspeedstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(topspeedstring, 0, fps_scalex, fps_scaley, false, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, topspeedstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, false, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (blurstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(blurstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, blurstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (texstring[0])
		{
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(texstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, texstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
		if (entstring[0]) {
			fps_x = vid_conwidth.integer - DrawQ_TextWidth(entstring, 0, fps_scalex, fps_scaley, true, FONT_INFOBAR);
			DrawQ_Fill(fps_x, fps_y, vid_conwidth.integer - fps_x, fps_scaley, 0, 0, 0, 0.5, 0);
			DrawQ_String(fps_x, fps_y, entstring, 0, fps_scalex, fps_scaley, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			fps_y += fps_scaley;
		}
	}
}

#if 0
static void Sbar_DrawGauge(float x, float y, cachepic_t *pic, float width, float height, float rangey, float rangeheight, float c1, float c2, float c1r, float c1g, float c1b, float c1a, float c2r, float c2g, float c2b, float c2a, float c3r, float c3g, float c3b, float c3a, int drawflags)
{
	float r[5];
	c2 = bound(0, c2, 1);
	c1 = bound(0, c1, 1 - c2);
	r[0] = 0;
	r[1] = rangey + rangeheight * (c2 + c1);
	r[2] = rangey + rangeheight * (c2);
	r[3] = rangey;
	r[4] = height;
	if (r[1] > r[0])
		DrawQ_SuperPic(x, y + r[0], pic, width, (r[1] - r[0]), 0,(r[0] / height), c3r,c3g,c3b,c3a, 1,(r[0] / height), c3r,c3g,c3b,c3a, 0,(r[1] / height), c3r,c3g,c3b,c3a, 1,(r[1] / height), c3r,c3g,c3b,c3a, drawflags);
	if (r[2] > r[1])
		DrawQ_SuperPic(x, y + r[1], pic, width, (r[2] - r[1]), 0,(r[1] / height), c1r,c1g,c1b,c1a, 1,(r[1] / height), c1r,c1g,c1b,c1a, 0,(r[2] / height), c1r,c1g,c1b,c1a, 1,(r[2] / height), c1r,c1g,c1b,c1a, drawflags);
	if (r[3] > r[2])
		DrawQ_SuperPic(x, y + r[2], pic, width, (r[3] - r[2]), 0,(r[2] / height), c2r,c2g,c2b,c2a, 1,(r[2] / height), c2r,c2g,c2b,c2a, 0,(r[3] / height), c2r,c2g,c2b,c2a, 1,(r[3] / height), c2r,c2g,c2b,c2a, drawflags);
	if (r[4] > r[3])
		DrawQ_SuperPic(x, y + r[3], pic, width, (r[4] - r[3]), 0,(r[3] / height), c3r,c3g,c3b,c3a, 1,(r[3] / height), c3r,c3g,c3b,c3a, 0,(r[4] / height), c3r,c3g,c3b,c3a, 1,(r[4] / height), c3r,c3g,c3b,c3a, drawflags);
}
#endif

/*
===============
Sbar_Draw
===============
*/
extern float v_dmg_time, v_dmg_roll, v_dmg_pitch;
extern cvar_t v_kicktime;

static void SBar_Quake()
{
			sbar_x = (vid_conwidth.integer - 320)/2;
			sbar_y = vid_conheight.integer - SBAR_HEIGHT;
			// LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode

			if (cl.scores[cl.playerentity-1].qw_spectator) {
				// 39 27 5
					char	*str = "Spectator Mode";

					WARP_X_ (mapname: )
//					Sbar_DrawAlphaPic (0, 0, sb_sbar, alpha_1_0);
					DrawQ_Fill (sbar_x, sbar_y, 320, 16, 39/255.0 * 0.3, 27/255.0* 0.3, 5/255.0* 0.3, /*alpha*/ 0.5, DRAWFLAG_NORMAL_0);

					// print the filename and message
					Sbar_DrawString (8 + 8 * (20 - strlen(str) / 2.0) , 4, str);


			} else if (sbar_quake.integer == 1) {
		float cols = ceil (vid_conwidth.integer / 64.0);

		float col;
		for (col = 0; col < cols; col ++) {
			DrawQ_Pic (col * 64, vid_conheight.integer - sb_lines,
				sb_backtile, 64, 64 , /*rgba*/ 1, 1, 1, /*alpha*/ 1.0, /*flags*/ 0);
		}
	} else if (sbar_quake.integer == 2) {
//
// Q64 start
//
		int sbar_pelz = vid_conwidth.integer / 20;
		float frow1 = vid_conheight.integer - (sbar_pelz * 1.500);
		float frow0 = frow1                 - (sbar_pelz * 1.125);
		float fcol0 = sbar_pelz * 1;
		float fcol1 = fcol0                 + (sbar_pelz * 1.250);
		float rcol1 = vid_conwidth.integer  - sbar_pelz * 2;
		float rcol0 = rcol1 - sbar_pelz * 3.25;


		int fi;
		cachepic_t *iface;
		// Q64 face icon

		iface = NULL;
		     if (Have_Flag_Strict_Bool (cl.stats[STAT_ITEMS], IT_INVISIBILITY | IT_INVULNERABILITY ) )
				 iface = sb_face_invis_invuln;
		else if (Have_Flag (cl.stats[STAT_ITEMS], IT_QUAD) )				iface = sb_face_quad;
		else if (Have_Flag (cl.stats[STAT_ITEMS], IT_INVISIBILITY) )		iface = sb_face_invis;
		else if (Have_Flag (cl.stats[STAT_ITEMS], IT_INVULNERABILITY) )	iface = sb_face_invuln;
		else {
			fi = cl.stats[STAT_HEALTH] / 20;
			fi = bound(0, fi, 4);
			iface = sb_faces[fi][cl.time <= cl.faceanimtime];
		}

		// Sbar_DrawPic (112, 0, iface);
		DrawQ_Pic (
			fcol0,
			frow1,		// y
			iface,
			sbar_pelz * 1,								// w
			sbar_pelz * 1,								// h
			1,1,1, sbar_alpha_fg.value,					// color quad r
			DRAWFLAG_NORMAL_0
		);

		Sbar_DrawNum2 (
			fcol1,										// x
			frow1,										// y
			sbar_pelz,									// lettersize
			cl.stats[STAT_HEALTH],					// num
			3,											// numdigits
			/*coloridx*/ cl.stats[STAT_HEALTH] <= 25	// 0 or 1 is red
		);

		// Q64 face over

		// Q64 ammo icon
		iface = NULL;
		if (gamemode == GAME_ROGUE) {
			     if (cl.stats[STAT_ITEMS] & RIT_SHELLS)				iface = sb_ammo[0];
			else if (cl.stats[STAT_ITEMS] & RIT_NAILS)				iface = sb_ammo[1];
			else if (cl.stats[STAT_ITEMS] & RIT_ROCKETS)				iface = sb_ammo[2];
			else if (cl.stats[STAT_ITEMS] & RIT_CELLS)				iface = sb_ammo[3];
			else if (cl.stats[STAT_ITEMS] & RIT_LAVA_NAILS)			iface = rsb_ammo[0];
			else if (cl.stats[STAT_ITEMS] & RIT_PLASMA_AMMO)			iface = rsb_ammo[1];
			else if (cl.stats[STAT_ITEMS] & RIT_MULTI_ROCKETS)		iface = rsb_ammo[2];
		} else {
			if (cl.stats[STAT_ITEMS] & IT_SHELLS)					iface = sb_ammo[0];
			else if (cl.stats[STAT_ITEMS] & IT_NAILS)				iface = sb_ammo[1];
			else if (cl.stats[STAT_ITEMS] & IT_ROCKETS)				iface = sb_ammo[2];
			else if (cl.stats[STAT_ITEMS] & IT_CELLS)				iface = sb_ammo[3];
		}

		if (!iface) goto no_ammo;
		// Sbar_DrawPic (224, 0, sb_ammo[0]);
		DrawQ_Pic (															// AMMO
			rcol1,		// x
			frow1,		// y
			iface,
			sbar_pelz * 1,								// w
			sbar_pelz * 1,								// h
			1,1,1, sbar_alpha_fg.value,					// color quad r
			DRAWFLAG_NORMAL_0
		);
		Sbar_DrawNum2 (
			rcol0,										// x
			frow1,										// y
			sbar_pelz,									// lettersize
			cl.stats[STAT_AMMO],						// num
			3,											// numdigits
			/*coloridx*/ cl.stats[STAT_AMMO] <= 10	// 0 or 1 is red
		);
no_ammo:
		// Q64 ammo ike over
		// Q64 armor start
		iface = NULL;

		if (Have_Flag (cl.stats[STAT_ITEMS], IT_INVULNERABILITY)) {
			iface = sb_disc;
		} else {
			if (gamemode == GAME_ROGUE) {
				if (Have_Flag (cl.stats[STAT_ITEMS], RIT_ARMOR3))			iface = sb_armor[2];
				else if (Have_Flag (cl.stats[STAT_ITEMS], RIT_ARMOR2))		iface = sb_armor[1];
				else if (Have_Flag (cl.stats[STAT_ITEMS], RIT_ARMOR1))		iface = sb_armor[0];
			} else { // NOT GAME_ROGUE
				//Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, /*numscoloridx*/ cl.stats[STAT_ARMOR] <= 25);
				if (Have_Flag (cl.stats[STAT_ITEMS], IT_ARMOR3))				iface = sb_armor[2];
				else if (Have_Flag (cl.stats[STAT_ITEMS], IT_ARMOR2))		iface = sb_armor[1];
				else if (Have_Flag (cl.stats[STAT_ITEMS], IT_ARMOR1))		iface = sb_armor[0];
			}
		}

		// if pent ... 666 else red
		if (!iface) goto no_armor;
		DrawQ_Pic (
			fcol0,
			frow0,	// y
			iface,
			sbar_pelz * 1,								// w
			sbar_pelz * 1,								// h
			1,1,1, sbar_alpha_fg.value,					// color quad r
			DRAWFLAG_NORMAL_0
		);

		Sbar_DrawNum2 (
			fcol1,										// x
			frow0,										// y
			sbar_pelz,									// lettersize
			iface == sb_disc ? 666 : cl.stats[STAT_ARMOR],					// num
			3,											// numdigits
			/*coloridx*/ cl.stats[STAT_ARMOR] <= 25	// 0 or 1 is red
		);

no_armor:
		// Q64 armor over
		if (cl.scores[cl.playerentity-1].qw_spectator) {
			if (sb_showscores)
				Sbar_DeathmatchOverlay ();
			return;
		} else
		if (sb_showscores) {
			if (sb_lines) {
				//Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
			}

			//Sbar_SoloScoreboard (); // vad wy?
			Sbar_DrawScoreboard ();
		} else if (sb_showscores || (cl.stats[STAT_HEALTH] <= 0 && /*defaults 1*/ cl_deathscoreboard.integer)) {
			// DEAD

			//Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
			Sbar_DrawScoreboard ();
		} else {

			if (scr_clock.value) {
				int hours = Time_Hours ((int)cl.time);
				int minutes = Time_Minutes_Less_Hours ((int)cl.time);
				int seconds = Time_Seconds ((int)cl.time);
				char vabuf[1024];
				if (hours) {
					dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d:%02d", hours, minutes, seconds);
				} else {
					dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d", minutes, seconds);
				}

				Sbar_DrawString ((320 - 8 -3 ) - (int)strlen(vabuf) * 8, -24, vabuf);
			}
		}


		return;
	} // Q64
	//
	//
	// UP
	//
	//
		if (cl.scores[cl.playerentity-1].qw_spectator) {
			if (sb_showscores)
				Sbar_DeathmatchOverlay ();
			return;
		}

			if (sb_lines > 24) {
				Sbar_DrawInventory ();
		if ((cls.demoplayback && cl.gametype == GAME_DEATHMATCH) || (!cl.islocalgame && !cls.demoplayback))
					Sbar_DrawFrags ();
			}

			if (sb_showscores || 
				(cl.stats[STAT_HEALTH] <= 0 && 
cl_deathscoreboard.integer && cl.scores[cl.playerentity-1].qw_spectator == false)) {
		if (sb_lines) {
			Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
		}

		Sbar_DrawScoreboard ();
	} else if (sb_showscores || (cl.stats[STAT_HEALTH] <= 0 && cl_deathscoreboard.integer && cl.scores[cl.playerentity-1].qw_spectator == false)) {
		// DEAD
		Sbar_DrawAlphaPic (0, 0, sb_scorebar, sbar_alpha_bg.value);
				Sbar_DrawScoreboard ();
			} else if (sb_lines) {
				Sbar_DrawAlphaPic (0, 0, sb_sbar, sbar_alpha_bg.value);

				// keys (hipnotic only)
				//MED 01/04/97 moved keys here so they would not be overwritten
				if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) {
					if (cl.stats[STAT_ITEMS] & IT_KEY1)
						Sbar_DrawPic (209, 3, sb_items[0]);
					if (cl.stats[STAT_ITEMS] & IT_KEY2)
						Sbar_DrawPic (209, 12, sb_items[1]);
				}

				// armor
				if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY) {
					Sbar_DrawNum (24, 0, 666, 3, 1);
					Sbar_DrawPic (0, 0, sb_disc);
				} else {
					if (gamemode == GAME_ROGUE) {
						Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
						if (cl.stats[STAT_ITEMS] & RIT_ARMOR3) Sbar_DrawPic (0, 0, sb_armor[2]);
						else if (cl.stats[STAT_ITEMS] & RIT_ARMOR2) Sbar_DrawPic (0, 0, sb_armor[1]);
						else if (cl.stats[STAT_ITEMS] & RIT_ARMOR1) Sbar_DrawPic (0, 0, sb_armor[0]);
					} else { // NOT GAME_ROGUE
						Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
						if (cl.stats[STAT_ITEMS] & IT_ARMOR3) Sbar_DrawPic (0, 0, sb_armor[2]);
						else if (cl.stats[STAT_ITEMS] & IT_ARMOR2) Sbar_DrawPic (0, 0, sb_armor[1]);
						else if (cl.stats[STAT_ITEMS] & IT_ARMOR1) Sbar_DrawPic (0, 0, sb_armor[0]);
					}
				}


				// face
				Sbar_DrawFace ();

				// health
				Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

				// ammo icon
				if (gamemode == GAME_ROGUE) {
					if (cl.stats[STAT_ITEMS] & RIT_SHELLS) Sbar_DrawPic (224, 0, sb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & RIT_NAILS) Sbar_DrawPic (224, 0, sb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & RIT_ROCKETS) Sbar_DrawPic (224, 0, sb_ammo[2]);
					else if (cl.stats[STAT_ITEMS] & RIT_CELLS) Sbar_DrawPic (224, 0, sb_ammo[3]);
					else if (cl.stats[STAT_ITEMS] & RIT_LAVA_NAILS) Sbar_DrawPic (224, 0, rsb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & RIT_PLASMA_AMMO) Sbar_DrawPic (224, 0, rsb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & RIT_MULTI_ROCKETS) Sbar_DrawPic (224, 0, rsb_ammo[2]);
				} else {
					if (cl.stats[STAT_ITEMS] & IT_SHELLS) Sbar_DrawPic (224, 0, sb_ammo[0]);
					else if (cl.stats[STAT_ITEMS] & IT_NAILS) Sbar_DrawPic (224, 0, sb_ammo[1]);
					else if (cl.stats[STAT_ITEMS] & IT_ROCKETS) Sbar_DrawPic (224, 0, sb_ammo[2]);
					else if (cl.stats[STAT_ITEMS] & IT_CELLS) Sbar_DrawPic (224, 0, sb_ammo[3]);
				}

				Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);

		if (scr_clock.value || sb_showscores) {
			int hours = Time_Hours ((int)cl.time);
			int minutes = Time_Minutes_Less_Hours ((int)cl.time);
			int seconds = Time_Seconds ((int)cl.time);
			char vabuf[1024];
			if (hours) {
				dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d:%02d", hours, minutes, seconds);
			} else {
				dpsnprintf (vabuf, sizeof(vabuf), "%d:%02d", minutes, seconds);
			}

			Sbar_DrawString ((320 - 8 -3 ) - (int)strlen(vabuf) * 8, -24, vabuf);
		} // clock

	} // dead
}

extern int cl_videoplaying;
void Sbar_Draw (void)
{
	cachepic_t *pic;
	char vabuf[1024];

	if (cl.csqc_vidvars.drawenginesbar) {	//[515]: csqc drawsbar
		if (sb_showscores && cl.gametype == GAME_DEATHMATCH) {
			SBar_Quake ();
		}
		else if (cl.intermission == 1) {
			if (IS_OLDNEXUIZ_DERIVED(gamemode)) { // display full scoreboard (that is, show scores + map name)
				Sbar_DrawScoreboard();
				return;
			}
			Sbar_IntermissionOverlay();
		}
		else if (cl.intermission == 2) Sbar_FinaleOverlay();
		else if (IS_OLDNEXUIZ_DERIVED(gamemode))	{					} // IS_OLDNEXUIZ_DERIVED
		else										{	SBar_Quake ();	}  // Quake and others
	}

	if (cl.csqc_vidvars.drawcrosshair && crosshair.integer >= 1 && !cl_videoplaying && !cl.intermission && !r_letterbox.value) {
		pic = Draw_CachePic (va(vabuf, sizeof(vabuf), "gfx/crosshair%d", crosshair.integer));
		DrawQ_Pic((vid_conwidth.integer - Draw_GetPicWidth(pic) * crosshair_size.value) * 0.5f, (vid_conheight.integer - Draw_GetPicHeight(pic) * crosshair_size.value) * 0.5f, pic, Draw_GetPicWidth(pic) * crosshair_size.value, Draw_GetPicHeight(pic) * crosshair_size.value, crosshair_color_red.value, crosshair_color_green.value, crosshair_color_blue.value, crosshair_color_alpha.value, 0);
	}

	if (cl_prydoncursor.integer > 0)
		DrawQ_Pic((cl.mcmd.cursor_screen[0] + 1) * 0.5 * vid_conwidth.integer, (cl.mcmd.cursor_screen[1] + 1) * 0.5 * vid_conheight.integer, Draw_CachePic (va(vabuf, sizeof(vabuf), "gfx/prydoncursor%03d", cl_prydoncursor.integer)), 0, 0, 1, 1, 1, 1, 0);
}

//=============================================================================

/*
==================
Sbar_DeathmatchOverlay

==================
*/
static float Sbar_PrintScoreboardItem(scoreboard_t *s, float x, float y)
{
	int minutes;
	qbool myself = false;
	unsigned char *c;
	char vabuf[1024];
	minutes = (int)((cl.intermission ? cl.completed_time - s->qw_entertime : cl.time - s->qw_entertime) / 60.0);

	if ((s - cl.scores) == cl.playerentity - 1)
		myself = true;
	if ((s - teams) >= 0 && (s - teams) < MAX_SCOREBOARD_255)
		if ((s->colors & 15) == (cl.scores[cl.playerentity - 1].colors & 15))
			myself = true;

	if (cls.protocol == PROTOCOL_QUAKEWORLD) {
		if (s->qw_spectator) {
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %4i spectator  %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), minutes, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %4i spectator  %c%s", minutes, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		} else {
			// draw colors behind score
			//
			//
			//
			//
			//
			c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
			DrawQ_Fill(x + 14*8*FONT_SBAR->maxwidth, y+1, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			c = palette_rgb_shirtscoreboard[s->colors & 0xf];
			DrawQ_Fill(x + 14*8*FONT_SBAR->maxwidth, y+4, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			// print the text
			//DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%c%4i %s", myself ? 13 : ' ', (int) s->frags, s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, true, FONT_DEFAULT);
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %4i %5i %-4s %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), minutes,(int) s->frags, cl.qw_teamplay ? s->qw_team : "", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %4i %5i %-4s %c%s", minutes,(int) s->frags, cl.qw_teamplay ? s->qw_team : "", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
	} else {
		if (s->qw_spectator) {
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i spect %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         spect %c%s", myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		} else {
			// draw colors behind score
			c = palette_rgb_pantsscoreboard[(s->colors & 0xf0) >> 4];
			DrawQ_Fill(x + 9*8*FONT_SBAR->maxwidth, y+1, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			c = palette_rgb_shirtscoreboard[s->colors & 0xf];
			DrawQ_Fill(x + 9*8*FONT_SBAR->maxwidth, y+4, 40*FONT_SBAR->maxwidth, 3, c[0] * (1.0f / 255.0f), c[1] * (1.0f / 255.0f), c[2] * (1.0f / 255.0f), sbar_alpha_fg.value, 0);
			// print the text
			//DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%c%4i %s", myself ? 13 : ' ', (int) s->frags, s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, true, FONT_DEFAULT);
			if (s->qw_ping || s->qw_packetloss)
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "%4i %3i %5i %c%s", bound(0, s->qw_ping, 9999), bound(0, s->qw_packetloss, 99), (int) s->frags, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
			else
				DrawQ_String(x, y, va(vabuf, sizeof(vabuf), "         %5i %c%s", (int) s->frags, myself ? 13 : ' ', s->name), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
		}
	}
	return 8;
}

void Sbar_DeathmatchOverlay (void)
{
	int i, y, xmin, xmax, ymin, ymax;
	char vabuf[1024];

	// request new ping times every two second
	if (cl.last_ping_request < host.realtime - 2 && cls.netcon)
	{
		cl.last_ping_request = host.realtime;
		if (cls.protocol == PROTOCOL_QUAKEWORLD) {
			Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd, "pings");
			//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
			//MSG_WriteString(&cls.netcon->message, "pings");
		}
		else if (isin15 (cls.protocol,	PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,
			PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP, 		PROTOCOL_NEHAHRAMOVIE,
			PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,
			PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
			PROTOCOL_DARKPLACES4,	PROTOCOL_DARKPLACES5,	PROTOCOL_DARKPLACES6,
			PROTOCOL_DARKPLACES7
			))
		{
			// these servers usually lack the pings command and so a less efficient "ping" command must be sent, which on modern DP servers will also reply with a pingplreport command after the ping listing
			static int ping_anyway_counter = 0;
			if (cl.parsingtextexpectingpingforscores == 1)
			{
				Con_DPrintf ("want to send ping, but still waiting for other reply\n");
				if (++ping_anyway_counter >= 5)
					cl.parsingtextexpectingpingforscores = 0;
			}
			if (cl.parsingtextexpectingpingforscores != 1)
			{
				ping_anyway_counter = 0;
				cl.parsingtextexpectingpingforscores = 1; // hide the output of the next ping report
				MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
				MSG_WriteString(&cls.netcon->message, "ping");
			}
		}
		else
		{
			// newer server definitely has pings command, so use it for more efficiency, avoids ping reports spamming the console if they are misparsed, and saves a little bandwidth
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, "pings");
		}
	}

	// scores
	Sbar_SortFrags ();

	ymin = 8;
	ymax = (ymin + 32) + 8 + (Sbar_IsTeammatch() ? (teamlines * 8 + 5): 0) + scoreboardlines * 8 - 1;

	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		xmin = (int) (vid_conwidth.integer - (26 + 15) * 8 * FONT_SBAR->maxwidth) / 2; // 26 characters until name, then we assume 15 character names (they can be longer but usually aren't)
	else
		xmin = (int) (vid_conwidth.integer - (16 + 25) * 8 * FONT_SBAR->maxwidth) / 2; // 16 characters until name, then we assume 25 character names (they can be longer but usually aren't)
	xmax = vid_conwidth.integer - xmin;

	// Baker:  If going to draw the clock, cut this back to 2 so nothing draws behind it.

	if (IS_OLDNEXUIZ_DERIVED(gamemode)) {
		DrawQ_Pic (xmin - 8, ymin - 8, 0, xmax-xmin+1 + 2*8, ymax-ymin+1 + 2*8, 0, 0, 0, sbar_alpha_bg.value, 0);
	} else {
		int fully = (ymin + 32) + 8 + 16 * 8 - 1;
		int szy = Largest (ymax-ymin+1 + 2*8, fully);
		int addy = (vid_conheight.value / 2 - (szy * .65) );

		ymin += addy;
		ymax += addy;
		DrawQ_Pic (xmin - 8, ymin - 8, 0, xmax-xmin+1 + 2*8, szy, 0, 0, 0, 0.5, 0);
	}

	DrawQ_Pic ((vid_conwidth.integer - Draw_GetPicWidth(sb_ranking))/2, ymin /*8*/, sb_ranking, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);

	// draw the text
	y = ymin + 32;// 40;
	if (cls.protocol == PROTOCOL_QUAKEWORLD) {
		DrawQ_String(xmin, y, va(vabuf, sizeof(vabuf), "ping pl%% time frags team  name"), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
	} else {

		DrawQ_String(xmin, y, va(vabuf, sizeof(vabuf), "ping pl%% frags  name"), 0, 8, 8, 1, 1, 1, 1 * sbar_alpha_fg.value, 0, NULL, false, FONT_SBAR );
	}
	y += 8;

	if (Sbar_IsTeammatch ()) {
		// show team scores first
		for (i = 0;i < teamlines && y < vid_conheight.integer;i++)
			y += (int)Sbar_PrintScoreboardItem((teams + teamsort[i]), xmin, y);
		y += 5;
	}

	for (i = 0;i < scoreboardlines && y < vid_conheight.integer;i++)
		y += (int)Sbar_PrintScoreboardItem(cl.scores + fragsort[i], xmin, y);
}



/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay (void)
{
	int		dig;
	int		num;

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	sbar_x = (vid_conwidth.integer - 320) >> 1;
	sbar_y = (vid_conheight.integer - 200) >> 1;

	DrawQ_Pic (sbar_x + 64, sbar_y + 24, sb_complete, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);
	DrawQ_Pic (sbar_x + 0, sbar_y + 56, sb_inter, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);

// time
	dig = (int)cl.completed_time / 60;
	Sbar_DrawNum (160, 64, dig, 3, 0);
	num = (int)cl.completed_time - dig*60;
	Sbar_DrawPic (234,64,sb_colon);
	Sbar_DrawPic (246,64,sb_nums[0][num/10]);
	Sbar_DrawPic (266,64,sb_nums[0][num%10]);

// LA: Display as "a" instead of "a/b" if b is 0
	if (1 || cl.stats[STAT_TOTALSECRETS]) // Baker r1082: display total monsters/secrets at all times like Quake
	{
		Sbar_DrawNum (160, 104, cl.stats[STAT_SECRETS], 3, 0);
		if (!IS_OLDNEXUIZ_DERIVED(gamemode))
			Sbar_DrawPic (232, 104, sb_slash);
		Sbar_DrawNum (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);
	}
	else
	{
		Sbar_DrawNum (240, 104, cl.stats[STAT_SECRETS], 3, 0);
	}

	if (1 || cl.stats[STAT_TOTALMONSTERS]) // Baker r1082: display total monsters/secrets at all times like Quake
	{
		Sbar_DrawNum (160, 144, cl.stats[STAT_MONSTERS], 3, 0);
		if (!IS_OLDNEXUIZ_DERIVED(gamemode))
			Sbar_DrawPic (232, 144, sb_slash);
		Sbar_DrawNum (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);
	}
	else
	{
		Sbar_DrawNum (240, 144, cl.stats[STAT_MONSTERS], 3, 0);
	}
}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay (void)
{
	DrawQ_Pic((vid_conwidth.integer - Draw_GetPicWidth(sb_finale))/2, 16, sb_finale, 0, 0, 1, 1, 1, 1 * sbar_alpha_fg.value, 0);
}

void Sbar_Init (void)
{
	Cmd_AddCommand(CF_CLIENT, "+showscores", Sbar_ShowScores_f, "show scoreboard");
	Cmd_AddCommand(CF_CLIENT, "-showscores", Sbar_DontShowScores_f, "hide scoreboard");
	Cvar_RegisterVariable(&showfps);
	Cvar_RegisterVariable(&showsound);
	Cvar_RegisterVariable(&showblur);
	Cvar_RegisterVariable(&showspeed);
	Cvar_RegisterVariable(&showspeed_factor);
	Cvar_RegisterVariable(&showtopspeed);
	Cvar_RegisterVariable(&showtime);
	Cvar_RegisterVariable(&showtime_format);
	Cvar_RegisterVariable(&showdate);
	Cvar_RegisterVariable(&showdate_format);
	Cvar_RegisterVariable(&showtex);
	Cvar_RegisterVariable(&showzmove);

	Cvar_RegisterVariable(&showpos); // Baker r7082 showpos showangles
	Cvar_RegisterVariable(&showangles); // Baker r7082 showpos showangles

	Cvar_RegisterVariable(&sbar_alpha_bg);
	Cvar_RegisterVariable(&sbar_alpha_fg);
	Cvar_RegisterVariable(&sbar_hudselector);
	Cvar_RegisterVariable(&scr_clock);

	Cvar_RegisterVariable(&sbar_showprotocol);

	Cvar_RegisterVariable(&sbar_miniscoreboard_size);
	Cvar_RegisterVariable(&sbar_info_pos);
	Cvar_RegisterVariable(&sbar_quake);

	Cvar_RegisterVariable(&cl_deathscoreboard);

	Cvar_RegisterVariable(&crosshair_color_red);
	Cvar_RegisterVariable(&crosshair_color_green);
	Cvar_RegisterVariable(&crosshair_color_blue);
	Cvar_RegisterVariable(&crosshair_color_alpha);
	Cvar_RegisterVariable(&crosshair_size);

	Cvar_RegisterVariable(&sbar_flagstatus_right); // (GAME_NEXUZI ONLY)
	Cvar_RegisterVariable(&sbar_flagstatus_pos); // (GAME_NEXUIZ ONLY)

	R_RegisterModule("sbar", sbar_start, sbar_shutdown, sbar_newmap, NULL, NULL);
}

