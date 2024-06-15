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

#include "quakedef.h"
#include "utf8lib.h"
#include "server.h"
#include "sv_demo.h"

int current_skill;

#ifdef CONFIG_MENU
	// CLIENT
	cvar_t sv_cheats = {CF_SERVER | CF_NOTIFY, "sv_cheats", "1", "enables cheat commands in any game, and cheat impulses in dpmod [Zircon default]"}; // Baker r1401 - sv_cheats defaults 1
#else
	// DEDICATED SERVER - DEFAULTS 0
	cvar_t sv_cheats = {CF_SERVER | CF_NOTIFY, "sv_cheats", "0", "enables cheat commands in any game, and cheat impulses in dpmod [Zircon default]"}; // Baker r1401 - sv_cheats defaults 1
#endif

cvar_t sv_adminnick = {CF_SERVER | CF_ARCHIVE, "sv_adminnick", "", "nick name to use for admin messages instead of host name"};
cvar_t sv_status_privacy = {CF_SERVER | CF_ARCHIVE, "sv_status_privacy", "0", "do not show IP addresses in 'status' replies to clients"};
cvar_t sv_status_show_qcstatus = {CF_SERVER | CF_ARCHIVE, "sv_status_show_qcstatus", "0", "show the 'qcstatus' field in status replies, not the 'frags' field. Turn this on if your mod uses this field, and the 'frags' field on the other hand has no meaningful value."};
cvar_t sv_namechangetimer = {CF_SERVER | CF_ARCHIVE, "sv_namechangetimer", "5", "how often to allow name changes, in seconds (prevents people from using animated names and other tricks"};

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/

/*
======================
SV_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
static void SV_Map_f(cmd_state_t *cmd)
{
	char startspot[MAX_QPATH_128] = {0};
	char level[MAX_QPATH_128];

	// Baker r1202: "map" command with no params says map name
	if (isin4(Cmd_Argc(cmd),1, 2, 3,4) == false) {
		Con_PrintLinef ("map <levelname> : start a new game (kicks off all players)");
		return;
	}

	if (Cmd_Argc(cmd) < 2) {
		// Baker: If we are on a map, print the name of the map5

		if (cls.state == ca_connected && cls.signon == SIGNONS_4) {
			Con_PrintLinef ("map is %s" NEWLINE, cl.worldbasename);

			if (cl.worldmodel) {
				Con_PrintLinef (S_FMT_LEFT_PAD_20 " %s", "supportwateralpha?", cl.worldmodel->brush.supportwateralpha ? "Yes" : "No");
				Con_PrintLinef (S_FMT_LEFT_PAD_20 " %s", "vis data?", cl.worldmodel->brush.num_pvsclusters != 0 ? "Yes" : "No");

				//if (developer.integer) {
					// Con_PrintLinef (S_FMT_LEFT_PAD_20 " %d", "num brushes", cl.worldmodel->brush.num_brushes);
					Con_PrintLinef (S_FMT_LEFT_PAD_20 " %d", "num triangles", cl.worldmodel->surfmesh.num_triangles);
					Con_PrintLinef (S_FMT_LEFT_PAD_20 " %d", "num vertices", cl.worldmodel->surfmesh.num_vertices);
					Con_PrintLinef (S_FMT_LEFT_PAD_20 " " VECTOR3_5d1F, "normalmins", VECTOR3_SEND(cl.worldmodel->normalmins));
					Con_PrintLinef (S_FMT_LEFT_PAD_20 " " VECTOR3_5d1F, "normalmaxs", VECTOR3_SEND(cl.worldmodel->normalmaxs));
					vec3_t size3;
					VectorSubtract (cl.worldmodel->normalmaxs, cl.worldmodel->normalmins, size3);
					Con_PrintLinef (S_FMT_LEFT_PAD_20 " " VECTOR3_5d1F, "size", VECTOR3_SEND(size3));
				//}

				Con_PrintLinef (NEWLINE "Keys" NEWLINE);

				char *entities, entname[MAX_QPATH_128];
				dpsnprintf (entname, sizeof(entname), "%s.ent", cl.worldnamenoextension);
				if ((entities = (char *)FS_LoadFile(entname, tempmempool, fs_quiet_true, fs_size_ptr_null))) {
					String_Worldspawn_Value_For_Key_Con_PrintLine (entities);
					Mem_Free(entities);
				} else {
					String_Worldspawn_Value_For_Key_Con_PrintLine (cl.worldmodel->brush.entities);
				}
			} // worldmodel
		} else if (sv.active) {
			Con_PrintLinef ("map is %s", sv.name);
		}

		return;
	}

	// 0   1    2      3
	// map e1m1 sfind sreplace
	if (Cmd_Argc(cmd) == 4) {
		Z_StrDup_Realloc_ (cls.s_find_za, Cmd_Argv(cmd, 2));
		Z_StrDup_Realloc_ (cls.s_replace_za, Cmd_Argv(cmd, 3));
	}


	if (Cmd_Argc(cmd) == 3) {
		c_strlcpy (startspot, Cmd_Argv(cmd, 2));
	}

	Con_CloseConsole_If_Client ();

	// Baker: I have no evidence this mod ever was released.
	// GAME_DELUXEQUAKE - clear warpmark (used by QC)
	//if (gamemode == GAME_DELUXEQUAKE)
	//	Cvar_Set(&cvars_all, "warpmark", "");

	if (host.hook.Disconnect)
		host.hook.Disconnect(q_is_kicked_false, q_disconnect_message_NULL);

	SV_Shutdown();

	if (svs.maxclients != svs.maxclients_next) {
		svs.maxclients = svs.maxclients_next;
		if (svs.clients)
			Mem_Free(svs.clients);
		svs.clients = (client_t *)Mem_Alloc(sv_mempool, sizeof(client_t) * svs.maxclients);
	}

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	svs.serverflags = 0;			// haven't completed an episode yet
	c_strlcpy (level, Cmd_Argv(cmd, 1));

	cl_signon_start_time = Sys_DirtyTime (); // map

	// Baker: Reset the .siv list
	if (sv_intermap_siv_list.numstrings)
		stringlistfreecontents (&sv_intermap_siv_list); // .SIV clear "map"

	vec3_t zero_origin = {0,0,0};
	SV_SpawnServer (level, q_s_loadgame_NULL, startspot[0] ? startspot : q_s_startspot_EmptyString, zero_origin, /*totaltimeatstart*/ 0);

	if (sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

/*
==================
SV_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
WARP_X_ (SV_Loadgame_from SV_Changelevel_f SV_Restart_f VM_changelevel SV_SpawnServer SV_SaveSpawnparms) // Q2X
static void SV_Changelevel2_f (cmd_state_t *cmd) // Q2X
{
	char level_base_name[MAX_QPATH_128];
	const char *s_level = Cmd_Argv(cmd, 1);

	// Baker: supertime is level_totaltimeatstart + sv.time
	float supertime = (sv.time - sv.intermap_surplustime) + sv.intermap_totaltimeatstart; // No sv.time is unreliable
	float level_totaltimeatlastexit = supertime;//sv.intermap_totaltimeatlastexit + sv.time;
	float level_totaltimeatstart = supertime; // For new level
	

	if (Cmd_Argc(cmd) != 3) {
		Con_PrintLinef ("changelevel2 <levelname> <startspot> : continue game on a new level");
		return;
	}

	if (!sv.active) {
		Con_PrintLinef ("You must be running a server to changelevel. Use 'map %s' instead", Cmd_Argv(cmd, 1));
		return;
	}

	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	prvm_prog_t *prog = SVVM_prog;
	func_t foff = PRVM_ED_FindFunctionOffset(prog, "BeforeExitMap");
	if (foff) {
		Con_DPrintLinef ("Calling BeforeExitMap");
		// PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(PRVM_EDICT_NUM(0));//PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, foff, "QC function BeforeExitMap is missing");
	}

	SV_SaveSpawnparms ();

	c_strlcpy (level_base_name, s_level);

	const char *s_startspot = Cmd_Argv(cmd, 2);
	// SaveGamestate
	{
		// Baker: Server issued the changelevel, it already had the opportunity
		// to clean up entities.

		// Baker: Add to the .siv list or update the .siv list
#if 1
		int siv_idx = stringlist_find_index (&sv_intermap_siv_list, sv.worldbasename);
		char *s_siv_alloc = NULL;
		if (siv_idx == not_found_neg1) {
			// ADD - we have not been to this map
			stringlistappend (&sv_intermap_siv_list, sv.worldbasename); // .SIV ADD NEW WORLDNAME (changelevel2)
			
		} else {
			// UPDATE / STOMP
			stringlist_replace_at_index (&sv_intermap_siv_list, siv_idx, sv.worldbasename); // .SIV REPLACE WORLDNAME (changelevel2)
		}
		SV_Savegame_to (SVVM_prog, &s_siv_alloc, q_savefile_NULL, q_is_siv_write_true, level_totaltimeatlastexit);

		if (siv_idx == not_found_neg1) {
			// ADD - we have not been to this map
			stringlistappend (&sv_intermap_siv_list, s_siv_alloc); // .SIV ADD NEW WORLD DATA changelevel2
			
		} else {
			// UPDATE / STOMP
			stringlist_replace_at_index (&sv_intermap_siv_list, siv_idx + 1, s_siv_alloc); // .SIV REPLACE WORLD DATA changelevel2
		}
		Z_Free (s_siv_alloc);

#else
		char	name_with_dot_siv[MAX_QPATH_128];
		c_dpsnprintf2 (name_with_dot_siv, "intermap/%s_%s.siv", s_startspot, sv.worldbasename); // SAVE

		SV_Savegame_to (SVVM_prog, q_savestring_NULL, name_with_dot_siv, q_is_intermap_true);
#endif
	}

	// Load game state
	{
		WARP_X_ (SV_Loadgame_f)
#if 1
		int siv_idx = stringlist_find_index (&sv_intermap_siv_list, level_base_name);
		//char *s_siv_alloc = NULL;
		//c_strlcpy (sv.intermap_startspot, s_startspot); // .CROSS startspot SV_SpawnServer_Intermap_SIV_Is_Ok

		if (siv_idx == not_found_neg1) {
			// ADD - we have not been to this map
			vec3_t zero_origin = {0,0,0};
			SV_SpawnServer (level_base_name, q_s_loadgame_NULL, s_startspot, zero_origin, level_totaltimeatstart);
		} else {
			// UPDATE / STOMP
			char *s_data = sv_intermap_siv_list.strings[siv_idx + 1];
			vec3_t zero_origin = {0,0,0};
			int is_ok = SV_SpawnServer_Intermap_SIV_Is_Ok (level_base_name, s_data, s_startspot, zero_origin, level_totaltimeatstart); // .SIV USE CHANGELEVEL2
			if (is_ok == false) {
				// Baker: If no save for the area, first time entering load from bsp
				Con_PrintLinef ("SIV spawn map error");
			}

		}

#else
		char	name_with_dot_siv[MAX_QPATH_128];
		c_dpsnprintf2 (name_with_dot_siv, "intermap/%s_%s.siv", s_startspot, level_base_name); // LOAD
		int is_ok = SV_SpawnServer_Intermap_SIV_Is_Ok (level_base_name, name_with_dot_siv, s_startspot);
		if (is_ok == false) {
			// Baker: If no save for the area, first time entering load from bsp
			SV_SpawnServer (level_base_name, q_s_loadgame_NULL, s_startspot);
		}
#endif

	}

	if (sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

static void SV_Changelevel_f (cmd_state_t *cmd) // Q2X
{
	char level[MAX_QPATH_128];
	const char *s_level = Cmd_Argv(cmd, 1);

	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("changelevel <levelname> : continue game on a new level");
		return;
	}

	if (!sv.active) {
		Con_PrintLinef ("You must be running a server to changelevel. Use 'map %s' instead", Cmd_Argv(cmd, 1));
		return;
	}

	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	SV_SaveSpawnparms ();
	c_strlcpy (level, s_level);

	int have_intermap = false;
	prvm_prog_t *prog = SVVM_prog;
	int v_enable_intermap_offset = PRVM_ED_FindGlobalOffset(prog, "enable_intermap");
	if (v_enable_intermap_offset >= 0) {
		float val = PRVM_GLOBALFIELDFLOAT(v_enable_intermap_offset);
		if (val != 0)
			have_intermap = true;
	} // if
	if (have_intermap || sv_intermap_siv_list.numstrings) {
		// Regular changelevel does not save the map state <-- I think this is ok
		// Regular changelevel does not load the .siv data (?) <-- I'm not sure this is ok.
		// #pragma message ("This does not print ... error instead?")
		// Con_PrintLinef (CON_RED, "Warning .SIV data destroyed with regular changelevel");
		stringlistfreecontents (&sv_intermap_siv_list); // .SIV clear regular "changelevel"
	}

	vec3_t zero_origin = {0,0,0};
	// Baker: At the moment, we expect changelevel2 everywhere if that is used.
	// I don't think mixing and matching is a good idea,
	// but if there is mix and match --- the idea would be 
	// some maps are fresh and loaded from bsp everytime.
	SV_SpawnServer (level, q_s_loadgame_NULL, q_s_startspot_EmptyString, zero_origin, /*totaltimeatstart*/ 0);

	if (sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

/*
==================
SV_Restart_f

Restarts the current server for a dead player
==================
*/
WARP_X_ (SV_Changelevel_f SV_Changelevel2_f SV_Restart_f VM_changelevel SV_SpawnServer SV_SaveSpawnparms) // Q2X
static void SV_Restart_f (cmd_state_t *cmd)
{
	// Baker: How does this work for a loadgame?
	char level_base_name[MAX_QPATH_128];
	char level_startspot[64] = {0};
	vec3_t level_startorigin = {0,0,0};
	float level_totaltimeatstart = 0;
	float level_totaltimeatlastexit = 0;
	float level_surplustime = 0;

#if 1 // Baker: Where/how do other engines do this?
	r_refdef.viewblend[0] = r_refdef.viewblend[1] = r_refdef.viewblend[2] = r_refdef.viewblend[3] = 0;  
#endif

	if (Cmd_Argc(cmd) != 1) {
		Con_PrintLinef ("restart : restart current level");
		return;
	}

	if (!sv.active) {
		Con_PrintLinef ("Only the server may restart");
		return;
	}

	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	c_strlcpy (level_base_name, sv.name); 
	c_strlcpy (level_startspot, sv.intermap_startspot); // .CROSS startspot "restart"
	VectorCopy (sv.intermap_startorigin, level_startorigin); // .CROSS startorigin "restart"
	level_totaltimeatstart = sv.intermap_totaltimeatstart;
	level_totaltimeatlastexit = sv.intermap_totaltimeatlastexit;
	level_surplustime = sv.intermap_surplustime;


	// Baker: Extra considerations here.
	// Was it an intermap load game?
	if (sv.was_intermap_loaded_from_siv) {
		WARP_X_ (SV_Changelevel2_f)
		int siv_idx = stringlist_find_index (&sv_intermap_siv_list, level_base_name);
		//char *s_siv_alloc = NULL;
		if (siv_idx == not_found_neg1) {
			// ADD - we have not been to this map
			Con_PrintLinef ("SIV restart map error - somehow did not find .siv data for a map loaded from .siv");
			SV_SpawnServer (level_base_name, q_s_loadgame_NULL, level_startspot, level_startorigin, level_totaltimeatstart);			
		} else {
			// UPDATE / STOMP
			char *s_data = sv_intermap_siv_list.strings[siv_idx + 1];
			int is_ok = SV_SpawnServer_Intermap_SIV_Is_Ok (level_base_name, s_data, level_startspot, level_startorigin, level_totaltimeatstart); // .SIV USE RESTART
			if (is_ok == false) {
				// Baker: If no save for the area, first time entering load from bsp
				Con_PrintLinef ("SIV restart map error");
			}

		}

	} else
		SV_SpawnServer (level_base_name, q_s_loadgame_NULL, level_startspot, level_startorigin, level_totaltimeatstart);

	if (sv.active && host.hook.ConnectLocal != NULL)
		host.hook.ConnectLocal();
}

//===========================================================================

// Disable cheats if sv_cheats is turned off
static void SV_DisableCheats_c(cvar_t *var)
{
	prvm_prog_t *prog = SVVM_prog;

	if (prog->loaded && var->value == 0) {
		// bones - sv_cheats: fix two segfaults when setting it to 0
		for (int i = 0; i < svs.maxclients; i ++) {
			int newflags;
			if (Have_Flag((int)PRVM_serveredictfloat(svs.clients[i].edict, flags), FL_GODMODE)) {
				 newflags = PRVM_serveredictfloat(svs.clients[i].edict, flags);
				 Flag_Remove_From (newflags, FL_GODMODE);
				 PRVM_serveredictfloat(svs.clients[i].edict, flags) = newflags;
			}

			if (Have_Flag((int)PRVM_serveredictfloat(svs.clients[i].edict, flags), FL_NOTARGET)) {
				 newflags = PRVM_serveredictfloat(svs.clients[i].edict, flags);
				 Flag_Remove_From (newflags, FL_NOTARGET);
				 PRVM_serveredictfloat(svs.clients[i].edict, flags) = newflags;
			}

			if (isin2 (PRVM_serveredictfloat(svs.clients[i].edict, movetype), MOVETYPE_NOCLIP_8, MOVETYPE_FLY_5)) {
				noclip_anglehack = false;
				PRVM_serveredictfloat(svs.clients[i].edict, movetype) = MOVETYPE_WALK_3;
			}
		} // for
	} // if
}

/*
==================
SV_God_f

Sets client to godmode
==================
*/
// Baker r1244: FitzQuake explicit option "god 1" forces god mode on
static void SV_God_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int flagz = PRVM_serveredictfloat(host_client->edict, flags);
	int wants_on = 0;
	int was_on = Have_Flag (flagz, FL_GODMODE);
	if (Cmd_Argc (cmd) > 1)	{ wants_on = atof(Cmd_Argv (cmd, 1)) != 0; }
	else					{ wants_on = !was_on; }

	if (wants_on)	{ Flag_Add_To (flagz, FL_GODMODE);		}
	else			{ Flag_Remove_From (flagz, FL_GODMODE); }

	PRVM_serveredictfloat(host_client->edict, flags) = flagz;

	if (!((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_GODMODE) )
		SV_ClientPrint("godmode OFF" NEWLINE);
	else
		SV_ClientPrint("godmode ON" NEWLINE);
}

qbool noclip_anglehack;

// Baker r1244: FitzQuake explicit option "god 1" forces god mode on
static void SV_Noclip_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int wants_on = 0;
	int was_on = PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_NOCLIP_8;
	if (Cmd_Argc (cmd) > 1)	{ wants_on = atof(Cmd_Argv (cmd, 1)) != 0; }
	else					{ wants_on = !was_on; }

	if (wants_on) {
		noclip_anglehack = true;
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_NOCLIP_8;
		SV_ClientPrint("noclip ON" NEWLINE);
	}
	else
	{
		noclip_anglehack = false;
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_WALK_3;
		SV_ClientPrint("noclip OFF" NEWLINE);
	}
}

/*
==================
SV_Give_f
==================
*/
// Baker r1245: FitzQuake give armor "give a 200"
static void SV_Give_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	const char *t;
	int v;
	int flagsz;

	t = Cmd_Argv(cmd, 1);
	v = atoi (Cmd_Argv(cmd, 2));

	switch (t[0]) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// MED 01/04/97 added hipnotic give stuff
		if (gamemode == GAME_HIPNOTIC || gamemode == GAME_QUOTH) {
			if (t[0] == '6') {
				if (t[1] == 'a')	PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_PROXIMITY_GUN;
				else				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | IT_GRENADE_LAUNCHER;
			}
			else if (t[0] == '9')	PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_LASER_CANNON;
			else if (t[0] == '0')	PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | HIT_MJOLNIR;
			else if (t[0] >= '2')	PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | (IT_SHOTGUN << (t[0] - '2'));
		} else {
			if (t[0] >= '2')
				PRVM_serveredictfloat(host_client->edict, items) = (int)PRVM_serveredictfloat(host_client->edict, items) | (IT_SHOTGUN << (t[0] - '2'));
		}
		break;

	case 's':
		if (gamemode == GAME_ROGUE)
			PRVM_serveredictfloat(host_client->edict, ammo_shells1) = v;

		PRVM_serveredictfloat(host_client->edict, ammo_shells) = v;
		break;
	case 'n':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_nails1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		} else {
			PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		}
		break;
	case 'l':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_lava_nails) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_nails) = v;
		}
		break;
	case 'r':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_rockets1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		} else {
			PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		}
		break;
	case 'm':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_multi_rockets) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_rockets) = v;
		}
		break;
	case 'h':
		PRVM_serveredictfloat(host_client->edict, health) = v;
		break;
	case 'c':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_cells1) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) <= IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		} else {
			PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		}
		break;
	case 'p':
		if (gamemode == GAME_ROGUE) {
			PRVM_serveredictfloat(host_client->edict, ammo_plasma) = v;
			if (PRVM_serveredictfloat(host_client->edict, weapon) > IT_LIGHTNING)
				PRVM_serveredictfloat(host_client->edict, ammo_cells) = v;
		}
		break;

    case 'a':
		flagsz = PRVM_serveredictfloat(host_client->edict, items);
		PRVM_serveredictfloat(host_client->edict, armorvalue) = v;
		if (gamemode == GAME_ROGUE)	{	Flag_Remove_From	(flagsz, RIT_ARMOR1 | RIT_ARMOR2 | RIT_ARMOR3 ); }
		else						{	Flag_Remove_From	(flagsz, IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 );	}

		if (v > 150) {
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.8;
			if (gamemode == GAME_ROGUE)	{	Flag_Add_To			(flagsz, IT_ARMOR3); }
			else						{Flag_Add_To			(flagsz, IT_ARMOR3); }
		} else if (v > 100) {
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.6;
			if (gamemode == GAME_ROGUE)	{	Flag_Add_To			(flagsz, IT_ARMOR2); }
			else						{Flag_Add_To			(flagsz, IT_ARMOR2); }
		} else if (v >= 0) {
			PRVM_serveredictfloat(host_client->edict, armortype) = 0.3;
			if (gamemode == GAME_ROGUE)	{	Flag_Add_To			(flagsz, IT_ARMOR1); }
			else						{Flag_Add_To			(flagsz, IT_ARMOR1); }
		}
		PRVM_serveredictfloat(host_client->edict, items) = flagsz;
		break;

	}
}

/*
==================
SV_Fly_f

Sets client to flymode
==================
*/
// Baker r1244: FitzQuake explicit option "god 1" forces god mode on
static void SV_Fly_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int wants_on = 0;
	int was_on = PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_FLY_5;
	if (Cmd_Argc (cmd) > 1)	{ wants_on = atof(Cmd_Argv (cmd, 1)) != 0; }
	else					{ wants_on = !was_on; }

	if (wants_on) {
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_FLY_5;
		SV_ClientPrint("flymode ON\n");
	} else {
		PRVM_serveredictfloat(host_client->edict, movetype) = MOVETYPE_WALK_3;
		SV_ClientPrint("flymode OFF\n");
	}
}

// Baker r1244: FitzQuake explicit option "god 1" forces god mode on
static void SV_Notarget_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int flagz = PRVM_serveredictfloat(host_client->edict, flags);
	int wants_on = 0;
	int was_on = Have_Flag (flagz, FL_NOTARGET);
	if (Cmd_Argc (cmd) > 1)	{ wants_on = atof(Cmd_Argv (cmd, 1)) != 0; }
	else					{ wants_on = !was_on; }

	if (wants_on)	{ Flag_Add_To (flagz, FL_NOTARGET);		}
	else			{ Flag_Remove_From (flagz, FL_NOTARGET); }

	PRVM_serveredictfloat(host_client->edict, flags) = flagz;

	if (!((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_NOTARGET) )
		SV_ClientPrint("notarget OFF" NEWLINE);
	else
		SV_ClientPrint("notarget ON" NEWLINE);
}

/*
==================
SV_Kill_f
==================
*/
static void SV_Kill_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	if (PRVM_serveredictfloat(host_client->edict, health) <= 0)
	{
		SV_ClientPrint("Can't suicide -- already dead!" NEWLINE);
		return;
	}

	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
	prog->ExecuteProgram(prog, PRVM_serverfunction(ClientKill), "QC function ClientKill is missing");

	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.
}

/*
==================
SV_SetPos_f
==================
*/
// Baker r3174: "setpos x y z [pitch] [yaw] [roll]"
static void SV_SetPos_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *player_ed = host_client->edict;

	vec3_t origin = {0}, angles = {0};
	int do_angles = false;

	if (Cmd_Argc(cmd) == 7) {
		origin[0] = atof(Cmd_Argv(cmd, 1));
		origin[1] = atof(Cmd_Argv(cmd, 2));
		origin[2] = atof(Cmd_Argv(cmd, 3));
		angles[0] = atof(Cmd_Argv(cmd, 4));
		angles[1] = atof(Cmd_Argv(cmd, 5));
		angles[2] = atof(Cmd_Argv(cmd, 6));
		do_angles = true;
	} else if (Cmd_Argc(cmd) == 4) {
		origin[0] = atof(Cmd_Argv(cmd, 1));
		origin[1] = atof(Cmd_Argv(cmd, 2));
		origin[2] = atof(Cmd_Argv(cmd, 3));
//		VectorCopy (cl.viewangles, angles);
	} else {
		SV_ClientPrint("Can't setpos -- need origin" NEWLINE);
		return;
	}

	VectorClear (PRVM_serveredictvector(player_ed, velocity));
	VectorCopy	(origin, PRVM_serveredictvector(player_ed, origin));
	if (do_angles) {
		VectorCopy	(angles, PRVM_serveredictvector(player_ed, angles));
	}

	// Baker: simulate teleport
	PRVM_serveredictfloat(player_ed, fixangle) =
		(int)true;

	int flagz = PRVM_serveredictfloat(player_ed, flags);
	if (Have_Flag (flagz, FL_ONGROUND)) {
		Flag_Remove_From (flagz, FL_ONGROUND);
		PRVM_serveredictfloat(player_ed, flags) = (int)flagz;
	}
}


/*
==================
SV_Pause_f
==================
*/
static void SV_Pause_f(cmd_state_t *cmd)
{
	void (*print) (const char *fmt, ...);
	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!pausable.integer && cmd->source == src_client && LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
	{
		print("Pause not allowed." NEWLINE);
		return;
	}

	sv.paused ^= 1;
	if (cmd->source != src_local)
		SV_BroadcastPrintf("%s %spaused the game\n", host_client->name, sv.paused ? "" : "un");
	else if (*(sv_adminnick.string))
		SV_BroadcastPrintf("%s %spaused the game\n", sv_adminnick.string, sv.paused ? "" : "un");
	else
		SV_BroadcastPrintf("%s %spaused the game\n", hostname.string, sv.paused ? "" : "un");
	// send notification to all clients
	MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
	MSG_WriteByte(&sv.reliable_datagram, sv.paused);
}

static void SV_Say(cmd_state_t *cmd, qbool teamonly)
{
	prvm_prog_t *prog = SVVM_prog;
	client_t *save;
	int j, quoted;
	const char *p1;
	char *p2;
	// LadyHavoc: long say messages
	char text[1024];
	qbool fromServer = false;

	if (cmd->source == src_local)
	{
		fromServer = true;
		teamonly = false;
	}

	if (Cmd_Argc (cmd) < 2)
		return;

	if (!teamplay.integer)
		teamonly = false;

	p1 = Cmd_Args(cmd);
	quoted = false;
	if (*p1 == '\"')
	{
		quoted = true;
		p1++;
	}
	// note this uses the chat prefix \001
	if (!fromServer && !teamonly)
		dpsnprintf (text, sizeof(text), "\001%s: %s", host_client->name, p1);
	else if (!fromServer && teamonly)
		dpsnprintf (text, sizeof(text), "\001(%s): %s", host_client->name, p1);
	else if (*(sv_adminnick.string))
		dpsnprintf (text, sizeof(text), "\001<%s> %s", sv_adminnick.string, p1);
	else
		dpsnprintf (text, sizeof(text), "\001<%s> %s", hostname.string, p1);
	p2 = text + strlen(text);
	while ((const char *)p2 > (const char *)text && (p2[-1] == '\r' || p2[-1] == '\n' || (p2[-1] == '\"' && quoted)))
	{
		if (p2[-1] == '\"' && quoted)
			quoted = false;
		p2[-1] = 0;
		p2--;
	}
	strlcat(text, "\n", sizeof(text));

	// note: save is not a valid edict if fromServer is true
	save = host_client;
	for (j = 0, host_client = svs.clients;j < svs.maxclients;j++, host_client++)
		if (host_client->active && (!teamonly || PRVM_serveredictfloat(host_client->edict, team) == PRVM_serveredictfloat(save->edict, team)))
			SV_ClientPrint(text);
	host_client = save;

	if (!host_isclient.integer)
		Con_Print(&text[1]);
}

static void SV_Say_f(cmd_state_t *cmd)
{
	SV_Say(cmd, false);
}

static void SV_Say_Team_f(cmd_state_t *cmd)
{
	SV_Say(cmd, true);
}

static void SV_Tell_f(cmd_state_t *cmd)
{
	const char *playername_start = NULL;
	size_t playername_length = 0;
	int playernumber = 0;
	client_t *save;
	int j;
	const char *p1, *p2;
	char text[MAX_INPUTLINE_16384]; // LadyHavoc: FIXME: temporary buffer overflow fix (was 64)
	qbool fromServer = false;

	if (cmd->source == src_local)
		fromServer = true;

	if (Cmd_Argc (cmd) < 2)
		return;

	// note this uses the chat prefix \001
	if (!fromServer)
		dpsnprintf (text, sizeof(text), "\001%s tells you: ", host_client->name);
	else if (*(sv_adminnick.string))
		dpsnprintf (text, sizeof(text), "\001<%s tells you> ", sv_adminnick.string);
	else
		dpsnprintf (text, sizeof(text), "\001<%s tells you> ", hostname.string);

	p1 = Cmd_Args(cmd);
	p2 = p1 + strlen(p1);
	// remove the target name
	while (p1 < p2 && *p1 == ' ')
		p1++;
	if (*p1 == '#')
	{
		++p1;
		while (p1 < p2 && *p1 == ' ')
			p1++;
		while (p1 < p2 && isdigit(*p1))
		{
			playernumber = playernumber * 10 + (*p1 - '0');
			p1++;
		}
		--playernumber;
	}
	else if (*p1 == '"')
	{
		++p1;
		playername_start = p1;
		while (p1 < p2 && *p1 != '"')
			p1++;
		playername_length = p1 - playername_start;
		if (p1 < p2)
			p1++;
	}
	else
	{
		playername_start = p1;
		while (p1 < p2 && *p1 != ' ')
			p1++;
		playername_length = p1 - playername_start;
	}
	while (p1 < p2 && *p1 == ' ')
		p1++;
	if (playername_start)
	{
		// set playernumber to the right client
		char namebuf[128];
		if (playername_length >= sizeof(namebuf))
		{
			if (fromServer)
				Con_Print("Host_Tell: too long player name/ID\n");
			else
				SV_ClientPrint("Host_Tell: too long player name/ID\n");
			return;
		}
		memcpy(namebuf, playername_start, playername_length);
		namebuf[playername_length] = 0;
		for (playernumber = 0; playernumber < svs.maxclients; playernumber++)
		{
			if (!svs.clients[playernumber].active)
				continue;
			if (strcasecmp(svs.clients[playernumber].name, namebuf) == 0)
				break;
		}
	}
	if (playernumber < 0 || playernumber >= svs.maxclients || !(svs.clients[playernumber].active))
	{
		if (fromServer)
			Con_Print("Host_Tell: invalid player name/ID\n");
		else
			SV_ClientPrint("Host_Tell: invalid player name/ID\n");
		return;
	}
	// remove trailing newlines
	while (p2 > p1 && (p2[-1] == '\n' || p2[-1] == '\r'))
		p2--;
	// remove quotes if present
	if (*p1 == '"')
	{
		p1++;
		if (p2[-1] == '"')
			p2--;
		else if (fromServer)
			Con_Print("Host_Tell: missing end quote\n");
		else
			SV_ClientPrint("Host_Tell: missing end quote\n");
	}
	while (p2 > p1 && (p2[-1] == '\n' || p2[-1] == '\r'))
		p2--;
	if (p1 == p2)
		return; // empty say
	for (j = (int)strlen(text);j < (int)(sizeof(text) - 2) && p1 < p2;)
		text[j++] = *p1++;
	text[j++] = '\n';
	text[j++] = 0;

	save = host_client;
	host_client = svs.clients + playernumber;
	SV_ClientPrint(text);
	host_client = save;
}

/*
==================
SV_Ping_f

==================
*/
static void SV_Ping_f(cmd_state_t *cmd)
{
	int i;
	client_t *client;
	void (*print) (const char *fmt, ...);

	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!sv.active)
		return;

	print("Client ping times:\n");
	for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
	{
		if (!client->active)
			continue;
		print("%4i %s\n", bound(0, (int)floor(client->ping*1000+0.5), 9999), client->name);
	}
}

/*
====================
SV_Pings_f

Send back ping and packet loss update for all current players to this player
====================
*/
static void SV_Pings_f(cmd_state_t *cmd)
{
	int		i, j, ping, packetloss, movementloss;
	char temp[128];

	if (!host_client->netconnection)
		return;

	if (sv.protocol != PROTOCOL_QUAKEWORLD)
	{
		MSG_WriteByte(&host_client->netconnection->message, svc_stufftext);
		MSG_WriteUnterminatedString(&host_client->netconnection->message, "pingplreport");
	}
	for (i = 0;i < svs.maxclients;i++)
	{
		packetloss = 0;
		movementloss = 0;
		if (svs.clients[i].netconnection)
		{
			for (j = 0;j < NETGRAPH_PACKETS_256;j++)
				if (svs.clients[i].netconnection->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET_NEG1)
					packetloss++;
			for (j = 0;j < NETGRAPH_PACKETS_256;j++)
				if (svs.clients[i].movement_count[j] < 0)
					movementloss++;
		}
		packetloss = (packetloss * 100 + NETGRAPH_PACKETS_256 - 1) / NETGRAPH_PACKETS_256;
		movementloss = (movementloss * 100 + NETGRAPH_PACKETS_256 - 1) / NETGRAPH_PACKETS_256;
		ping = (int)floor(svs.clients[i].ping*1000+0.5);
		ping = bound(0, ping, 9999);
		if (sv.protocol == PROTOCOL_QUAKEWORLD)
		{
			// send qw_svc_updateping and qw_svc_updatepl messages
			MSG_WriteByte(&host_client->netconnection->message, qw_svc_updateping);
			MSG_WriteShort(&host_client->netconnection->message, ping);
			MSG_WriteByte(&host_client->netconnection->message, qw_svc_updatepl);
			MSG_WriteByte(&host_client->netconnection->message, packetloss);
		}
		else
		{
			// write the string into the packet as multiple unterminated strings to avoid needing a local buffer
			if (movementloss)
				dpsnprintf(temp, sizeof(temp), " %d %d,%d", ping, packetloss, movementloss);
			else
				dpsnprintf(temp, sizeof(temp), " %d %d", ping, packetloss);
			MSG_WriteUnterminatedString(&host_client->netconnection->message, temp);
		}
	}
	if (sv.protocol != PROTOCOL_QUAKEWORLD)
		MSG_WriteString(&host_client->netconnection->message, "\n");
}

/*
==================
SV_Status_f
==================
*/

// Baker: The format here has to conform to specification for qstat and other tools to work.
static void SV_Status_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char qcstatus[256];
	client_t *client;
	int seconds = 0, minutes = 0, hours = 0, i, j, k, in, players, ping = 0, packetloss = 0;
	void (*print) (const char *fmt, ...);
	char ip_48[48]; // can contain a full length v6 address with [] and a port
	int frags;
	char vabuf[1024];

	if (cmd->source == src_local)
		print = Con_Printf;
	else
		print = SV_ClientPrintf;

	if (!sv.active)
		return;

	in = 0;
	// status 1
	// status 2
	if (Cmd_Argc(cmd) == 2) {
		if (String_Match(Cmd_Argv(cmd, 1), "1"))
			in = 1;
		else if (String_Match(Cmd_Argv(cmd, 1), "2"))
			in = 2;
	}

	for (players = 0, i = 0;i < svs.maxclients;i++)
		if (svs.clients[i].active)
			players++;
	print ("host:     %s" NEWLINE, Cvar_VariableString (&cvars_all, "hostname", CF_SERVER));
	print ("version:  %s build %s (gamename %s) (extensions %x) (zmove on? %d)" NEWLINE,
		gamename, buildstring, gamenetworkfiltername, sv.zirconprotcolextensions_sv, Have_Zircon_Ext_Flag_SV_Hard(ZIRCON_EXT_FREEMOVE_4)
		);
	print ("protocol: %d (%s) %s" NEWLINE, Protocol_NumberForEnum(sv.protocol), Protocol_NameForEnum(sv.protocol), sv.is_qex ? "(remaster)" : "" ); // AURA 5.0
	print ("map:      %s" NEWLINE, sv.name);
	print ("timing:   %s" NEWLINE, SV_TimingReport(vabuf, sizeof(vabuf)));
	print ("players:  %d active (%d max)" NEWLINE NEWLINE, players, svs.maxclients);

	if (in == 1)
		print (CON_GREEN "IP                                             %%pl ping  time   frags  no   name" NEWLINE);
	else if (in == 2)
		print (CON_CYAN "IP                                              no   name" NEWLINE);

	for (i = 0, k = 0, client = svs.clients;i < svs.maxclients;i++, client++) {
		if (!client->active)
			continue;

		++k;

		if (in == 0 || in == 1) {
			seconds = (int)(host.realtime - client->connecttime);
			minutes = seconds / 60;
			if (minutes) {
				seconds -= (minutes * 60);
				hours = minutes / 60;
				if (hours)
					minutes -= (hours * 60);
			}
			else
				hours = 0;

			packetloss = 0;
			if (client->netconnection)
				for (j = 0;j < NETGRAPH_PACKETS_256;j++)
					if (client->netconnection->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET_NEG1)
						packetloss++;
			packetloss = (packetloss * 100 + NETGRAPH_PACKETS_256 - 1) / NETGRAPH_PACKETS_256;
			ping = bound(0, (int)floor(client->ping*1000+0.5), 9999);
		}

		if (sv_status_privacy.integer && cmd->source != src_local && LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
			c_strlcpy(ip_48, client->netconnection ? "hidden" : "botclient");
		else
			c_strlcpy(ip_48, (client->netconnection && *client->netconnection->address) ? client->netconnection->address : "botclient");

		frags = client->frags;

		if (sv_status_show_qcstatus.integer) {
			prvm_edict_t *ed = PRVM_EDICT_NUM(i + 1);
			const char *str = PRVM_GetString(prog, PRVM_serveredictstring(ed, clientstatus));
			if (str && *str) {
				char *p;
				const char *q;
				p = qcstatus;
				for(q = str; *q && p != qcstatus + sizeof(qcstatus) - 1; ++q)
					if (*q != '\\' && *q != '"' && !ISWHITESPACE(*q))
						*p++ = *q;
				*p = 0;
				if (*qcstatus)
					frags = atoi(qcstatus);
			}
		}

		if (in == 0) // default layout
		{
			if (sv.protocol == PROTOCOL_QUAKE && svs.maxclients <= 99)
			{
				// LadyHavoc: this is very touchy because we must maintain ProQuake compatible status output
				print ("#%-2u %-16.16s  %3d  %2d:%02d:%02d" NEWLINE, i + 1, client->name, frags, hours, minutes, seconds);
				print ("   %s" NEWLINE, ip_48);
			}
			else
			{
				// LadyHavoc: no real restrictions here, not a ProQuake-compatible protocol anyway...
				print ("#%-3u %-16.16s %4d  %2d:%02d:%02d" NEWLINE, i + 1, client->name, frags, hours, minutes, seconds);
				print ("   %s" NEWLINE, ip_48);
			}
		}
		else if (in == 1) // extended layout
		{
			print ("%s%-47s %2d %4d %2d:%02d:%02d %4d  #%-3u ^7%s" NEWLINE, k%2 ? "^3" : "^7", ip_48, packetloss, ping, hours, minutes, seconds, frags, i+1, client->name);
		}
		else if (in == 2) // reduced layout
		{
			print ("%s%-47s #%-3u ^7%s" NEWLINE, k%2 ? "^3" : "^7", ip_48, i+1, client->name);
		}
	}
}

void SV_Name(int clientnum)
{
	prvm_prog_t *prog = SVVM_prog;
	PRVM_serveredictstring(host_client->edict, netname) = PRVM_SetEngineString(prog, host_client->name);
	if (String_NOT_Match(host_client->old_name, host_client->name))
	{
		// Baker: CON_WHITE because name may have color
		if (host_client->begun)
			SV_BroadcastPrintf("\003%s" CON_WHITE " changed name to ^3%s" NEWLINE, host_client->old_name, host_client->name);
		c_strlcpy(host_client->old_name, host_client->name);
		// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
		MSG_WriteByte (&sv.reliable_datagram, clientnum);
		MSG_WriteString (&sv.reliable_datagram, host_client->name);
		SV_WriteNetnameIntoDemo(host_client);
	}
}

/*
======================
SV_Name_f
======================
*/
static void SV_Name_f(cmd_state_t *cmd)
{ // CROSSX
	int i, j;
	qbool valid_colors;
	const char *newNameSource;
	char newName[sizeof(host_client->name)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		newNameSource = Cmd_Argv(cmd, 1);
	else
		newNameSource = Cmd_Args(cmd);

	strlcpy(newName, newNameSource, sizeof(newName));

	if (cmd->source == src_local)
		return;

	if (host.realtime < host_client->nametime && String_NOT_Match (newName, host_client->name)) {
		SV_ClientPrintf ("You can't change name more than once every %.1f seconds!" NEWLINE, max(0.0f, sv_namechangetimer.value));
		return;
	}

	host_client->nametime = host.realtime + max(0.0f, sv_namechangetimer.value);

	// point the string back at updateclient->name to keep it safe
	c_strlcpy (host_client->name, newName);

	// Baker: carriage returns and new lines not allowed.
	for (i = 0, j = 0;host_client->name[i];i++)
		if (host_client->name[i] != '\r' && host_client->name[i] != '\n')
			host_client->name[j++] = host_client->name[i];
	host_client->name[j] = 0;

	if (host_client->name[0] == 1 || host_client->name[0] == 2) {
		// may interfere with chat area, and will needlessly beep; so let's add a ^7
		memmove(host_client->name + 2, host_client->name, sizeof(host_client->name) - 2);
		host_client->name[sizeof(host_client->name) - 1] = 0;
		host_client->name[0] = STRING_COLOR_TAG;
		host_client->name[1] = '0' + STRING_COLOR_DEFAULT;
	}

	u8_COM_StringLengthNoColors(host_client->name, 0, &valid_colors);
	if (!valid_colors) // NOTE: this also proves the string is not empty, as "" is a valid colored string
	{
		size_t l;
		l = strlen(host_client->name);
		if (l < sizeof(host_client->name) - 1)
		{
			// duplicate the color tag to escape it
			host_client->name[i] = STRING_COLOR_TAG;
			host_client->name[i+1] = 0;
			//Con_DPrintf ("abuse detected, adding another trailing color tag\n");
		}
		else
		{
			// remove the last character to fix the color code
			host_client->name[l-1] = 0;
			//Con_DPrintf ("abuse detected, removing a trailing color tag\n");
		}
	}

	// find the last color tag offset and decide if we need to add a reset tag
	for (i = 0, j = -1;host_client->name[i]; i++) {
		if (host_client->name[i] == STRING_COLOR_TAG)
		{
			if (host_client->name[i+1] >= '0' && host_client->name[i+1] <= '9')
			{
				j = i;
				// if this happens to be a reset  tag then we don't need one
				if (host_client->name[i+1] == '0' + STRING_COLOR_DEFAULT)
					j = -1;
				i++;
				continue;
			}
			if (host_client->name[i+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(host_client->name[i+2]) && isxdigit(host_client->name[i+3]) && isxdigit(host_client->name[i+4]))
			{
				j = i;
				i += 4;
				continue;
			}
			if (host_client->name[i+1] == STRING_COLOR_TAG)
			{
				i++;
				continue;
			}
		}
	}
	// does not end in the default color string, so add it
	if (j >= 0 && strlen(host_client->name) < sizeof(host_client->name) - 2)
		memcpy(host_client->name + strlen(host_client->name), STRING_COLOR_DEFAULT_STR, strlen(STRING_COLOR_DEFAULT_STR) + 1);

	SV_Name(host_client - svs.clients);
}

static void SV_Rate_f(cmd_state_t *cmd)
{
	int rate;

	rate = atoi(Cmd_Argv(cmd, 1));

	if (cmd->source == src_local)
		return;

	host_client->rate = rate;
}

static void SV_Rate_BurstSize_f(cmd_state_t *cmd)
{
	int rate_burstsize;

	if (Cmd_Argc(cmd) != 2)
		return;

	rate_burstsize = atoi(Cmd_Argv(cmd, 1));

	host_client->rate_burstsize = rate_burstsize;
}

static void SV_Color_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	int top, bottom, playercolor;

	top = atoi(Cmd_Argv(cmd, 1));
	bottom = atoi(Cmd_Argv(cmd, 2));

	top &= 15;
	bottom &= 15;

	playercolor = top*16 + bottom;

	if (host_client->edict && PRVM_serverfunction(SV_ChangeTeam))
	{
		Con_DPrintLinef ("Calling SV_ChangeTeam");
		prog->globals.fp[OFS_PARM0] = playercolor;
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SV_ChangeTeam), "QC function SV_ChangeTeam is missing");
	}
	else
	{
		if (host_client->edict)
		{
			PRVM_serveredictfloat(host_client->edict, clientcolors) = playercolor;
			PRVM_serveredictfloat(host_client->edict, team) = bottom + 1;
		}
		host_client->colors = playercolor;
		if (host_client->old_colors != host_client->colors)
		{
			host_client->old_colors = host_client->colors;
			// send notification to all clients
			MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
			MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
			MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
		}
	}
}

/*
==================
SV_Kick_f

Kicks a user off of the server
==================
*/
static void SV_Kick_f(cmd_state_t *cmd)
{
	const char *who;
	const char *message = NULL;
	char reason[512];
	client_t *save;
	int i;
	qbool byNumber = false;

	if (!sv.active)
		return;

	save = host_client;

	if (Cmd_Argc(cmd) > 2 && String_Starts_With(Cmd_Argv(cmd, 1), "#") )
	{
		i = (int)(atof(Cmd_Argv(cmd, 2)) - 1);
		if (i < 0 || i >= svs.maxclients || !(host_client = svs.clients + i)->active)
			return;
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
		{
			if (!host_client->active)
				continue;
			if (String_Match_Caseless(host_client->name, Cmd_Argv(cmd, 1)) )
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd->source == src_local)
		{
			if (!host_isclient.integer)
				who = "Console";
			else
				who = cl_name.string;
		}
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc(cmd) > 2)
		{
			message = Cmd_Args(cmd);
			COM_Parse_Basic (&message);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += strlen(Cmd_Argv(cmd, 2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_DropClient (q_is_leaving_false, va(reason, sizeof(reason), "Kicked by %s: %s", who, message)); // kicked
			//SV_ClientPrintf("Kicked by %s: %s" NEWLINE, who, message);
		else
			//SV_ClientPrintf("Kicked by %s" NEWLINE, who);
			SV_DropClient (q_is_leaving_false, va(reason, sizeof(reason), "Kicked by %s", who)); // kicked
	}

	host_client = save;
}

static void SV_MaxPlayers_f(cmd_state_t *cmd)
{
	int n;

	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef (QUOTED_STR ("maxplayers") " is \"%u\"" NEWLINE, svs.maxclients_next);
		return;
	}

	if (sv.active)
	{
		Con_PrintLinef ("maxplayers can not be changed while a server is running.");
		Con_PrintLinef ("It will be changed on next server startup (\"map\" command).");
	}

	n = atoi(Cmd_Argv(cmd, 1));
	n = bound(1, n, MAX_SCOREBOARD_255);
	Con_PrintLinef (QUOTED_STR ("maxplayers") " set to \"%u\"" NEWLINE, n);

	svs.maxclients_next = n;
	if (n == 1)
		Cvar_Set (&cvars_all, "deathmatch", "0");
	else
		Cvar_Set (&cvars_all, "deathmatch", "1");
}

/*
======================
SV_Playermodel_f
======================
*/
// the old playermodel in cl_main has been renamed to __cl_playermodel
static void SV_Playermodel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, j;
	char newPath[sizeof(host_client->playermodel)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		strlcpy (newPath, Cmd_Argv(cmd, 1), sizeof (newPath));
	else
		strlcpy (newPath, Cmd_Args(cmd), sizeof (newPath));

	for (i = 0, j = 0;newPath[i];i++)
		if (newPath[i] != '\r' && newPath[i] != '\n')
			newPath[j++] = newPath[i];
	newPath[j] = 0;

	/*
	if (host.realtime < host_client->nametime)
	{
		SV_ClientPrintf("You can't change playermodel more than once every 5 seconds!" NEWLINE);
		return;
	}

	host_client->nametime = host.realtime + 5;
	*/

	// point the string back at updateclient->name to keep it safe
	strlcpy (host_client->playermodel, newPath, sizeof (host_client->playermodel));
	PRVM_serveredictstring(host_client->edict, playermodel) = PRVM_SetEngineString(prog, host_client->playermodel);
	if (strcmp(host_client->old_model, host_client->playermodel))
	{
		strlcpy(host_client->old_model, host_client->playermodel, sizeof(host_client->old_model));
		/*// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatepmodel);
		MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
		MSG_WriteString (&sv.reliable_datagram, host_client->playermodel);*/
	}
}

/*
======================
SV_Playerskin_f
======================
*/
static void SV_Playerskin_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, j;
	char newPath[sizeof(host_client->playerskin)];

	if (Cmd_Argc (cmd) == 1)
		return;

	if (Cmd_Argc (cmd) == 2)
		strlcpy (newPath, Cmd_Argv(cmd, 1), sizeof (newPath));
	else
		strlcpy (newPath, Cmd_Args(cmd), sizeof (newPath));

	for (i = 0, j = 0;newPath[i];i++)
		if (newPath[i] != '\r' && newPath[i] != '\n')
			newPath[j++] = newPath[i];
	newPath[j] = 0;

	/*
	if (host.realtime < host_client->nametime)
	{
		SV_ClientPrintf("You can't change playermodel more than once every 5 seconds!" NEWLINE);
		return;
	}

	host_client->nametime = host.realtime + 5;
	*/

	// point the string back at updateclient->name to keep it safe
	strlcpy (host_client->playerskin, newPath, sizeof (host_client->playerskin));
	PRVM_serveredictstring(host_client->edict, playerskin) = PRVM_SetEngineString(prog, host_client->playerskin);
	if (strcmp(host_client->old_skin, host_client->playerskin))
	{
		//if (host_client->begun)
		//	SV_BroadcastPrintf("%s changed skin to %s\n", host_client->name, host_client->playerskin);
		strlcpy(host_client->old_skin, host_client->playerskin, sizeof(host_client->old_skin));
		/*// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatepskin);
		MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
		MSG_WriteString (&sv.reliable_datagram, host_client->playerskin);*/
	}
}

/*
======================
SV_PModel_f
LadyHavoc: only supported for Nehahra, I personally think this is dumb, but Mindcrime won't listen.
LadyHavoc: correction, Mindcrime will be removing pmodel in the future, but it's still stuck here for compatibility.
======================
*/
// Baker: Tried this with Nehahra ... don't see anything happening?
static void SV_PModel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;

	if (Cmd_Argc (cmd) == 1)
		return;

	PRVM_serveredictfloat(host_client->edict, pmodel) = atoi(Cmd_Argv(cmd, 1));
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

static prvm_edict_t	*FindViewthing(prvm_prog_t *prog)
{
	int		i;
	prvm_edict_t	*e;

	for (i=0 ; i<prog->num_edicts ; i++)
	{
		e = PRVM_EDICT_NUM(i);
		if (String_Match (PRVM_GetString(prog, PRVM_serveredictstring(e, classname)), "viewthing"))
			return e;
	}
	Con_Print("No viewthing on map\n");
	return NULL;
}

/*
==================
SV_Viewmodel_f
==================
*/
static void SV_Viewmodel_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = Mod_ForName (Cmd_Argv(cmd, 1), false, true, NULL);
		if (m && m->loaded && m->Draw)
		{
			PRVM_serveredictfloat(e, frame) = 0;
			cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)] = m;
		}
		else
			Con_Printf ("viewmodel: can't load %s\n", Cmd_Argv(cmd, 1));
	}
}

/*
==================
SV_Viewframe_f
==================
*/
static void SV_Viewframe_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	int		f;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		f = atoi(Cmd_Argv(cmd, 1));
		if (f >= m->numframes)
			f = m->numframes-1;

		PRVM_serveredictfloat(e, frame) = f;
	}
}

static void PrintFrameName (model_t *m, int frame)
{
	if (m->animscenes)
		Con_Printf ("frame %d: %s\n", frame, m->animscenes[frame].name);
	else
		Con_Printf ("frame %d\n", frame);
}

/*
==================
SV_Viewnext_f
==================
*/
static void SV_Viewnext_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e)
	{
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		PRVM_serveredictfloat(e, frame) = PRVM_serveredictfloat(e, frame) + 1;
		if (PRVM_serveredictfloat(e, frame) >= m->numframes)
			PRVM_serveredictfloat(e, frame) = m->numframes - 1;

		PrintFrameName (m, (int)PRVM_serveredictfloat(e, frame));
	}
}

/*
==================
SV_Viewprev_f
==================
*/
static void SV_Viewprev_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t	*e;
	model_t	*m;

	if (!sv.active)
		return;

	e = FindViewthing(prog);
	if (e) {
		m = cl.model_precache[(int)PRVM_serveredictfloat(e, modelindex)];

		PRVM_serveredictfloat(e, frame) = PRVM_serveredictfloat(e, frame) - 1;
		if (PRVM_serveredictfloat(e, frame) < 0)
			PRVM_serveredictfloat(e, frame) = 0;

		PrintFrameName (m, (int)PRVM_serveredictfloat(e, frame));
	}
}

static void SV_SendCvar_f(cmd_state_t *cmd)
{
	int i;
	const char *cvarname;
	client_t *old;

	if (Cmd_Argc(cmd) != 2)
		return;

	if (!sv.active)// || !PRVM_serverfunction(SV_ParseClientCommand))
		return;

	cvarname = Cmd_Argv(cmd, 1);

	old = host_client;
	if (host_isclient.integer)
		i = 1;
	else
		i = 0;
	for (; i < svs.maxclients; i++) {
		if (svs.clients[i].active && svs.clients[i].netconnection) {
			host_client = &svs.clients[i];
			SV_ClientCommandsf("sendcvar %s" NEWLINE, cvarname);
		}
	}

	host_client = old;
}

void SV_Spawn_Model_At (ccs *s_classname, ccs *s_modelname, ccs *s_origin, ccs *s_angles, ccs *s_frame, ccs *s_scale)
{
	// Baker: Spawn entity.
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ed = PRVM_ED_Alloc (SVVM_prog);
	
	//const char *s_classname = "func_illusionary"; // Cmd_Argv(cmd, 1)

	PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "classname"), s_classname, qp_parse_backslash_false);
	PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "scale"), s_scale, qp_parse_backslash_false);
	PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "frame"), s_frame, qp_parse_backslash_false);
	PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "origin"), s_origin, /*parse backslash*/ false);
	PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "angles"), s_angles, /*parse backslash*/ false);


	VMX_SV_precache_model	(prog, s_modelname);
	VMX_SV_setmodel			(prog, ed, s_modelname);
	// Baker: The origin should be set.

	// Make it appear in the world
	SV_LinkEdict (ed); // Hmmm.
	//PRVM_ED_Free(prog, ed); // Kills it
}
// Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "modelview", SV_ModelView_f, "Shows a model to look at with [model][scale][z_adjust]");
static void SV_ShowModel_f (cmd_state_t *cmd)
{
	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	prvm_prog_t *prog = SVVM_prog;
	//prvm_edict_t *ed;
//	mdef_t *key;
//	int i;
	qbool haveorigin;

	if (!cl.islocalgame) {
		print (	"Requires local game" NEWLINE);
		return;
	}

	if (Cmd_Argc(cmd) < 2) {
		print (	"Usage: showmodel [model] [frame number] [z adjust] [scale]" NEWLINE
				"Show a model by spawning an entity" NEWLINE
				"Example: showmodel progs/player.mdl /*frame*/ 0 /*z adjust*/ -5 /*scale*/ 2" NEWLINE
		);
		return;
	}

	const char *s_modelname		= Cmd_Argc(cmd) < 2 ? "progs/player.mdl" : Cmd_Argv(cmd, 1);
	const char *s_frame			= Cmd_Argc(cmd) < 3 ? "0" : Cmd_Argv(cmd, 2);
	const char *s_zadjust		= Cmd_Argc(cmd) < 4 ? "0" : Cmd_Argv(cmd, 3);
	const char *s_scale			= Cmd_Argc(cmd) < 5 ? "1" : Cmd_Argv(cmd, 4);

	////// Baker: Spawn entity.
	////ed = PRVM_ED_Alloc (SVVM_prog);

	////const char *s_classname = "func_illusionary"; // Cmd_Argv(cmd, 1)
	////PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "classname"), s_classname, /*parse backslash*/ false);
	////PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "scale"), s_scale, /*parse backslash*/ false);
	////PRVM_ED_ParseEpair (prog, ed, PRVM_ED_FindField(prog, "frame"), s_frame, /*parse backslash*/ false);

	char s_origin_buffer[128];
	char s_angles_buffer[128];

	// Spawn where the player is aiming. We need a view matrix first.
	if (cmd->source == src_client) {
		vec3_t org;// temp; //, temp, dest;
		matrix4x4_t view;
		//trace_t trace;

		SV_GetEntityMatrix(prog, host_client->edict, &view, true);

		Matrix4x4_OriginFromMatrix	(&view, org);
		//VectorSet					(temp, 65536, 0, 0);

		vec3_t forward;
		vec3_t yawangles;
		vec3_t spot;
		VectorSet		(yawangles, cl.viewangles[PITCH], cl.viewangles[YAW], cl.viewangles[ROLL]);
		AngleVectors	(yawangles, forward, NULL, NULL);
		VectorMA		(org, 200, forward, spot);

		float z_adjust = atoi(s_zadjust);

		c_dpsnprintf3 (s_origin_buffer, "%g %g %g", spot[0], spot[1], spot[2] + z_adjust);
		c_dpsnprintf3 (s_angles_buffer, "%g %g %g", cl.viewangles[0], -cl.viewangles[1], cl.viewangles[2]);

		////PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "origin"), s_origin_buffer, /*parse backslash*/ false);
		////PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "angles"), s_angles_buffer, /*parse backslash*/ false);

		haveorigin = true;
	}
	// Or spawn at a specified origin.
	else
	{
		print = Con_Printf;
		haveorigin = false;
	}

	if (haveorigin == false) {
		print ("Missing origin" NEWLINE);
		//PRVM_ED_Free(prog, ed);
		return;
	}

	SV_Spawn_Model_At ("func_illusionary", s_modelname, s_origin_buffer, s_angles_buffer, s_frame, s_scale);

	////// precache_model
	////WARP_X_ (VMX_SV_precache_model, VMX_SV_setmodel)
	////void VMX_SV_precache_model (prvm_prog_t *prog, const char *s_model);
	////void VMX_SV_setmodel (prvm_prog_t *prog, prvm_edict_t *e, const char *s_model);

	////VMX_SV_precache_model	(prog, s_modelname);
	////VMX_SV_setmodel			(prog, ed, s_modelname);
	////// Baker: The origin should be set.


	// Make it appear in the world
	////SV_LinkEdict (ed);

	if (cmd->source == src_client)
		Con_Printf ("%s spawned a model " QUOTED_S NEWLINE, host_client->name, s_modelname);
}


static void SV_Ent_Create_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ed;
	mdef_t *key;
	int i;
	qbool haveorigin;

	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	if (!Cmd_Argc(cmd))
		return;

	ed = PRVM_ED_Alloc(SVVM_prog);

	PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "classname"), Cmd_Argv(cmd, 1), false);

	// Spawn where the player is aiming. We need a view matrix first.
	if (cmd->source == src_client)
	{
		vec3_t org, temp, dest;
		matrix4x4_t view;
		trace_t trace;
		char buf[128];

		SV_GetEntityMatrix(prog, host_client->edict, &view, true);

		Matrix4x4_OriginFromMatrix(&view, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&view, temp, dest);

		trace = SV_TraceLine(org, dest, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value);

		dpsnprintf(buf, sizeof(buf), "%g %g %g", trace.endpos[0], trace.endpos[1], trace.endpos[2]);
		PRVM_ED_ParseEpair(prog, ed, PRVM_ED_FindField(prog, "origin"), buf, false);

		haveorigin = true;
	}
	// Or spawn at a specified origin.
	else
	{
		print = Con_Printf;
		haveorigin = false;
	}

	// Allow more than one key/value pair by cycling between expecting either one.
	for(i = 2; i < Cmd_Argc(cmd); i += 2)
	{
		if (!(key = PRVM_ED_FindField(prog, Cmd_Argv(cmd, i))))
		{
			print("Key %s not found!\n", Cmd_Argv(cmd, i));
			PRVM_ED_Free(prog, ed);
			return;
		}

		/*
		 * This is mostly for dedicated server console, but if the
		 * player gave a custom origin, we can ignore the traceline.
		 */
		if (String_Match(Cmd_Argv(cmd, i), "origin"))
			haveorigin = true;

		if (i + 1 < Cmd_Argc(cmd))
			PRVM_ED_ParseEpair(prog, ed, key, Cmd_Argv(cmd, i+1), false);
	}

	if (!haveorigin)
	{
		print ("Missing origin" NEWLINE);
		PRVM_ED_Free(prog, ed);
		return;
	}

	// Spawn it
	PRVM_ED_CallPrespawnFunction(prog, ed);

	if (!PRVM_ED_CallSpawnFunction(prog, ed, NULL, NULL))
	{
		print ("Could not spawn a " QUOTED_S ". No such entity or it has no spawn function" NEWLINE, Cmd_Argv(cmd, 1));
		if (cmd->source == src_client)
			Con_PrintLinef ("%s tried to spawn a " QUOTED_S, host_client->name, Cmd_Argv(cmd, 1));
		// CallSpawnFunction already freed the edict for us.
		return;
	}

	PRVM_ED_CallPostspawnFunction(prog, ed);

	// Make it appear in the world
	SV_LinkEdict(ed);

	if (cmd->source == src_client)
		Con_PrintLinef ("%s spawned a " QUOTED_S, host_client->name, Cmd_Argv(cmd, 1));
}

static void SV_Ent_Remove_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ed;
	int i, ednum = 0;
	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	if (!Cmd_Argc(cmd))
		return;

	// Allow specifying edict by number
	if (Cmd_Argc(cmd) > 1 && Cmd_Argv(cmd, 1))
	{
		ednum = atoi(Cmd_Argv(cmd, 1));
		if (!ednum)
		{
			print("Cannot remove the world\n");
			return;
		}
	}
	// Or trace a line if it's a client who didn't specify one.
	else if (cmd->source == src_client)
	{
		vec3_t org, temp, dest;
		matrix4x4_t view;
		trace_t trace;

		SV_GetEntityMatrix(prog, host_client->edict, &view, true);

		Matrix4x4_OriginFromMatrix(&view, org);
		VectorSet(temp, 65536, 0, 0);
		Matrix4x4_Transform(&view, temp, dest);

		trace = SV_TraceLine(org, dest, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY, 0, 0, collision_extendmovelength.value);

		if (trace.ent)
			ednum = (int)PRVM_EDICT_TO_PROG(trace.ent);
		if (!trace.ent || !ednum)
			// Don't remove the world, but don't annoy players with a print if they miss
			return;
	}
	else
	{
		// Only a dedicated server console should be able to reach this.
		print("No edict given\n");
		return;
	}

	ed = PRVM_EDICT_NUM(ednum);

	if (ed)
	{
		// Skip players
		for (i = 0; i < svs.maxclients; i++)
		{
			if (ed == svs.clients[i].edict)
				return;
		}

		if (!ed->free)
		{
			print ("Removed a " QUOTED_S NEWLINE, PRVM_GetString(prog, PRVM_serveredictstring(ed, classname)));
			PRVM_ED_ClearEdict(prog, ed);
			PRVM_ED_Free(prog, ed);
		}
	}
	else
	{
		// This should only be reachable if an invalid edict number was given
		print ("No such entity" NEWLINE);
		return;
	}
}

static void SV_Ent_Remove_All_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, rmcount;
	prvm_edict_t *ed;
	void (*print)(const char *, ...) = (cmd->source == src_client ? SV_ClientPrintf : Con_Printf);

	for (i = 0, rmcount = 0, ed = PRVM_EDICT_NUM(i); i < prog->num_edicts; i++, ed = PRVM_NEXT_EDICT(ed))
	{
		if (!ed->free && String_Match(PRVM_GetString(prog, PRVM_serveredictstring(ed, classname)), Cmd_Argv(cmd, 1)))
		{
			if (!i)
			{
				print ("Cannot remove the world" NEWLINE);
				return;
			}
			PRVM_ED_ClearEdict(prog, ed);
			PRVM_ED_Free(prog, ed);
			rmcount++;
		}
	}

	if (!rmcount)
		print ("No " QUOTED_S " found" NEWLINE, Cmd_Argv(cmd, 1));
	else
		print ("Removed %d of " QUOTED_S NEWLINE, rmcount, Cmd_Argv(cmd, 1));
}


void SV_InitOperatorCommands(void)
{
	Cvar_RegisterVariable(&sv_cheats);
	Cvar_RegisterCallback(&sv_cheats, SV_DisableCheats_c);
	Cvar_RegisterVariable(&sv_adminnick);
	Cvar_RegisterVariable(&sv_status_privacy);
	Cvar_RegisterVariable(&sv_status_show_qcstatus);
	Cvar_RegisterVariable(&sv_namechangetimer);

	
	Cmd_AddCommand(CF_SERVER | CF_SERVER_FROM_CLIENT, "status", SV_Status_f, "print server status information");
	Cmd_AddCommand(CF_SHARED | CF_CLIENTCLOSECONSOLE, "map", SV_Map_f, "kick everyone off the server and start a new level"); // Baker r1003: close console for map/load/etc.
	Cmd_AddCommand(CF_SHARED | CF_CLIENTCLOSECONSOLE, "restart", SV_Restart_f, "restart current level"); // Baker r1003: close console for map/load/etc.
	Cmd_AddCommand(CF_SHARED | CF_CLIENTCLOSECONSOLE, "changelevel", SV_Changelevel_f, "change to another level, bringing along all connected clients"); // Baker r1003: close console for map/load/etc.
	Cmd_AddCommand(CF_SHARED | CF_CLIENTCLOSECONSOLE, "changelevel2", SV_Changelevel2_f, "change to another level, bringing along all connected clients"); // Baker r1003: close console for map/load/etc.
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "say", SV_Say_f, "send a chat message to everyone on the server");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "say_team", SV_Say_Team_f, "send a chat message to your team on the server");
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "tell", SV_Tell_f, "send a chat message to only one person on the server");
	Cmd_AddCommand(CF_SERVER | CF_SERVER_FROM_CLIENT, "pause", SV_Pause_f, "pause the game (if the server allows pausing)");
	Cmd_AddCommand(CF_SHARED, "kick", SV_Kick_f, "kick a player off the server by number or name (kick name or kick # 2)");
	Cmd_AddCommand(CF_SHARED | CF_SERVER_FROM_CLIENT, "ping", SV_Ping_f, "print ping times of all players on the server");
	Cmd_AddCommand(CF_SHARED | CF_CLIENTCLOSECONSOLE, "load", SV_Loadgame_f, "load a saved game file"); // Baker r1003: close console for map/load/etc.
	Cmd_AddCommand(CF_SHARED, "save", SV_Savegame_f, "save the game to a file");
	Cmd_AddCommand(CF_SHARED, "viewmodel", SV_Viewmodel_f, "change model of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewframe", SV_Viewframe_f, "change animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewnext", SV_Viewnext_f, "change to next animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "viewprev", SV_Viewprev_f, "change to previous animation frame of viewthing entity in current level");
	Cmd_AddCommand(CF_SHARED, "maxplayers", SV_MaxPlayers_f, "sets limit on how many players (or bots) may be connected to the server at once");
	Cmd_AddCommand(CF_SHARED, "siv", SV_Siv_f, "List intermap .siv (entity saves) stored or siv [number] to copy one to the clipboard [Zircon]");
	host.hook.SV_SendCvar = SV_SendCvar_f;

	// commands that do not have automatic forwarding from cmd_local, these are internal details of the network protocol and not of interest to users (if they know what they are doing they can still use a generic "cmd prespawn" or similar)
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "prespawn", SV_PreSpawn_f, "internal use - signon 1 (client acknowledges that server information has been received)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "spawn", SV_Spawn_f, "internal use - signon 2 (client has sent player information, and is asking server to send scoreboard rankings)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "begin", SV_Begin_f, "internal use - signon 3 (client asks server to start sending entities, and will go to signon 4 (playing) when the first entity update is received)");
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT, "pings", SV_Pings_f, "internal use - command sent by clients to request updated ping and packetloss of players on scoreboard (originally from QW, but also used on NQ servers)");

	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "god", SV_God_f, "god mode (invulnerability)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "notarget", SV_Notarget_f, "notarget mode (monsters do not see you)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "fly", SV_Fly_f, "fly mode (flight)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "noclip", SV_Noclip_f, "noclip mode (flight without collisions, move through walls)");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "give", SV_Give_f, "alter inventory");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "setpos", SV_SetPos_f, "teleport/set current origin <angles> [Zircon]"); // Baker r3174
	Cmd_AddCommand(CF_SERVER_FROM_CLIENT | CF_CLIENTCLOSECONSOLE, "kill", SV_Kill_f, "die instantly"); // Baker r1003: close console for map/load/etc.

	Cmd_AddCommand(CF_USERINFO, "color", SV_Color_f, "change your player shirt and pants colors");
	Cmd_AddCommand(CF_USERINFO, "name", SV_Name_f, "change your player name");
	Cmd_AddCommand(CF_USERINFO, "rate", SV_Rate_f, "change your network connection speed");
	Cmd_AddCommand(CF_USERINFO, "rate_burstsize", SV_Rate_BurstSize_f, "change your network connection speed");
	Cmd_AddCommand(CF_USERINFO, "pmodel", SV_PModel_f, "(Nehahra-only) change your player model choice");
	Cmd_AddCommand(CF_USERINFO, "playermodel", SV_Playermodel_f, "change your player model");
	Cmd_AddCommand(CF_USERINFO, "playerskin", SV_Playerskin_f, "change your player skin number");

	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_create", SV_Ent_Create_f, "Creates an entity at the specified coordinate, of the specified classname. If executed from a server, origin has to be specified manually.");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "showmodel", SV_ShowModel_f, "Shows a model to look at with [model][scale][z_adjust]");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_remove_all", SV_Ent_Remove_All_f, "Removes all entities of the specified classname");
	Cmd_AddCommand(CF_CHEAT | CF_SERVER_FROM_CLIENT, "ent_remove", SV_Ent_Remove_f, "Removes an entity by number, or the entity you're aiming at");
}
