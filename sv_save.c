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
#include "prvm_cmds.h"

/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5

void SV_Savegame_to(prvm_prog_t *prog, const char *name)
{
	qfile_t	*f;
	int		i, k, l, numbuffers, lightstyles = 64;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	char	line[MAX_INPUTLINE_16384];
	qbool isserver;
	char	*s;

	// first we have to figure out if this can be saved in 64 lightstyles
	// (for Quake compatibility)
	for (i=64 ; i<MAX_LIGHTSTYLES_256 ; i++)
		if (sv.lightstyles[i][0])
			lightstyles = i+1;

	isserver = prog == SVVM_prog;

	Con_PrintLinef ("Saving game to %s...", name);
	f = FS_OpenRealFile(name, "wb", false);
	if (!f)
	{
		Con_PrintLinef ("ERROR: couldn't open.");
		return;
	}

	FS_Printf (f, "%d" NEWLINE, SAVEGAME_VERSION);

	memset(comment, 0, sizeof(comment));
	if (isserver)
		dpsnprintf (comment, sizeof(comment), "%-21.21s kills:%3d/%3d", PRVM_GetString(prog, PRVM_serveredictstring(prog->edicts, message)), (int)PRVM_serverglobalfloat(killed_monsters), (int)PRVM_serverglobalfloat(total_monsters));
	else
		dpsnprintf (comment, sizeof(comment), "(crash dump of %s progs)", prog->name);
	// convert space to _ to make stdio happy
	// LadyHavoc: convert control characters to _ as well
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (ISWHITESPACEORCONTROL(comment[i]))
			comment[i] = '_';
	comment[SAVEGAME_COMMENT_LENGTH] = '\0';

	FS_Printf(f, "%s\n", comment);
	if (isserver)
	{
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			FS_Printf(f, "%f" NEWLINE, svs.clients[0].spawn_parms[i]);
		FS_Printf(f, "%d" NEWLINE, current_skill);
		FS_Printf(f, "%s" NEWLINE, sv.name);
		FS_Printf(f, "%f" NEWLINE,sv.time);
	}
	else
	{
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			FS_Printf(f, "(dummy)" NEWLINE);
		FS_Printf (f, "%d" NEWLINE, 0);
		FS_Printf (f, "%s" NEWLINE, "(dummy)");
		FS_Printf (f, "%f" NEWLINE, host.realtime);
	}

	// write the light styles
	for (i=0 ; i<lightstyles ; i++)
	{
		if (isserver && sv.lightstyles[i][0])
			FS_Printf(f, "%s" NEWLINE, sv.lightstyles[i]);
		else
			FS_Print (f,"m" NEWLINE);
	}

	PRVM_ED_WriteGlobals (prog, f);
	for (i=0 ; i<prog->num_edicts ; i++)
	{
		FS_Printf (f,"// edict %d" NEWLINE, i);
		//Con_Printf ("edict %d...\n", i);
		PRVM_ED_Write (prog, f, PRVM_EDICT_NUM(i));
	}

#if 1
	FS_Printf (f,"/*" NEWLINE);
	FS_Printf (f,"// DarkPlaces extended savegame" NEWLINE);
	// darkplaces extension - extra lightstyles, support for color lightstyles
	for (i=0 ; i<MAX_LIGHTSTYLES_256 ; i++)
		if (isserver && sv.lightstyles[i][0])
			FS_Printf (f, "sv.lightstyles %d %s" NEWLINE, i, sv.lightstyles[i]);

	// darkplaces extension - model precaches
	for (i=1 ; i<MAX_MODELS_8192 ; i++)
		if (sv.model_precache[i][0])
			FS_Printf (f,"sv.model_precache %d %s" NEWLINE, i, sv.model_precache[i]);

	// darkplaces extension - sound precaches
	for (i=1 ; i<MAX_SOUNDS_4096 ; i++)
		if (sv.sound_precache[i][0])
			FS_Printf (f,"sv.sound_precache %d %s" NEWLINE, i, sv.sound_precache[i]);

	// darkplaces extension - save buffers
	numbuffers = (int)Mem_ExpandableArray_IndexRange(&prog->stringbuffersarray);
	for (i = 0; i < numbuffers; i++) {
		prvm_stringbuffer_t *stringbuffer = (prvm_stringbuffer_t*) Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, i);
		if (stringbuffer && (stringbuffer->flags & STRINGBUFFER_SAVED)) {
			FS_Printf (f,"sv.buffer %d %d \"string\"" NEWLINE, i, 
				stringbuffer->flags & STRINGBUFFER_QCFLAGS);
			for(k = 0; k < stringbuffer->num_strings; k++)
			{
				if (!stringbuffer->strings[k])
					continue;
				// Parse the string a bit to turn special characters
				// (like newline, specifically) into escape codes
				s = stringbuffer->strings[k];
				for (l = 0;l < (int)sizeof(line) - 2 && *s;)
				{	
					if (*s == '\n')
					{
						line[l++] = '\\';
						line[l++] = 'n';
					}
					else if (*s == '\r')
					{
						line[l++] = '\\';
						line[l++] = 'r';
					}
					else if (*s == '\\')
					{
						line[l++] = '\\';
						line[l++] = '\\';
					}
					else if (*s == '"')
					{
						line[l++] = '\\';
						line[l++] = '"';
					}
					else
						line[l++] = *s;
					s++;
				}
				line[l] = '\0';
				FS_Printf (f,"sv.bufstr %d %d " QUOTED_S NEWLINE, i, k, line);
			}
		}
	}
	FS_Printf(f,"*/" NEWLINE);
#endif

	FS_Close (f);
	Con_PrintLinef ("done.");
}

static qbool SV_CanSave(void)
{
	prvm_prog_t *prog = SVVM_prog;
	if (SV_IsLocalServer() == 1)
	{
		// singleplayer checks
		// FIXME: This only checks if the first player is dead?
		if ((svs.clients[0].active && PRVM_serveredictfloat(svs.clients[0].edict, deadflag)))
		{
			Con_PrintLinef ("Can't savegame with a dead player");
			return false;
		}

		if (host.hook.CL_Intermission && host.hook.CL_Intermission())
		{
			Con_PrintLinef ("Can't save in intermission.");
			return false;
		}
	}
	else
		Con_PrintLinef (CON_WARN "Warning: saving a multiplayer game may have strange results when restored (to properly resume, all players must join in the same player slots and then the game can be reloaded).");
	return true;
}

/*
===============
SV_Savegame_f
===============
*/
void SV_Savegame_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char	name[MAX_QPATH_128];

	if (!sv.active)
	{
		Con_PrintLinef ("Can't save - no server running.");
		return;
	}

	// Baker: This does dead player check, intermission
	// And multiplayer save warning
	if (!SV_CanSave())
		return;

	if (Cmd_Argc(cmd) != 2)
	{
		Con_PrintLinef ("save <savename> : save a game");
		return;
	}

	if (strstr(Cmd_Argv(cmd, 1), ".."))
	{
		Con_PrintLinef ("Relative pathnames are not allowed.");
		return;
	}

	c_strlcpy (name, Cmd_Argv(cmd, 1));
	FS_DefaultExtension (name, ".sav", sizeof (name));

	SV_Savegame_to(prog, name);
}

/*
===============
SV_Loadgame_f
===============
*/
void SV_Loadgame_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	char filename[MAX_QPATH_128];
	char mapname[MAX_QPATH_128];
	float time;
	const char *start;
	const char *end;
	const char *t;
	char *text;
	prvm_edict_t *ent;
	int i, k, numbuffers;
	int entnum;
	int version;
	float spawn_parms[NUM_SPAWN_PARMS];
	prvm_stringbuffer_t *stringbuffer;

	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("load <savename> : load a game");
		return;
	}

	strlcpy (filename, Cmd_Argv(cmd, 1), sizeof(filename));
	FS_DefaultExtension (filename, ".sav", sizeof (filename));

	Con_PrintLinef ("Loading game from %s...", filename);

	if (host.hook.Disconnect)
		host.hook.Disconnect(false, NULL);

	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();

	cls.demonum = -1;		// stop demo loop in case this fails

	t = text = (char *)FS_LoadFile (filename, tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
	if (!text) {
		Con_PrintLinef ("ERROR: couldn't open.");
		return;
	}

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: loading version");

	// version
	COM_ParseToken_Simple(&t, false, false, true);
	version = atoi(com_token);
	if (version != SAVEGAME_VERSION) {
		Mem_Free(text);
		Con_PrintLinef ("Savegame is version %d, not %d", version, SAVEGAME_VERSION);
		return;
	}

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: loading description");

	// description
	COM_ParseToken_Simple(&t, false, false, true);

	for (i = 0;i < NUM_SPAWN_PARMS;i++) {
		COM_ParseToken_Simple(&t, false, false, true);
		spawn_parms[i] = atof(com_token);
	}
	// skill
	COM_ParseToken_Simple(&t, false, false, true);
// this silliness is so we can load 1.06 save files, which have float skill values
	current_skill = (int)(atof(com_token) + 0.5);
	Cvar_SetValue (&cvars_all, "skill", (float)current_skill);

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: loading mapname");

	// mapname
	COM_ParseToken_Simple(&t, false, false, true);
	strlcpy (mapname, com_token, sizeof(mapname));

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: loading time");

	// time
	COM_ParseToken_Simple(&t, false, false, true);
	time = atof(com_token);

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: spawning server");

	SV_SpawnServer (mapname, filename); // Baker r9067: loadgame precaches "precache at any time models and sounds"
	if (!sv.active)
	{
		Mem_Free(text);
		Con_PrintLinef ("Couldn't load map");
		return;
	}
	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: loading light styles");

// load the light styles

	// -1 is the globals
	entnum = -1;

	for (i = 0; i < MAX_LIGHTSTYLES_256; i++) {
		// light style
		start = t;
		COM_ParseToken_Simple(&t, false, false, true);
		// if this is a 64 lightstyle savegame produced by Quake, stop now
		// we have to check this because darkplaces may save more than 64
		if (com_token[0] == '{') {
			t = start;
			break;
		}
		strlcpy(sv.lightstyles[i], com_token, sizeof(sv.lightstyles[i]));
	}

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: skipping until globals");

	// now skip everything before the first opening brace
	// (this is for forward compatibility, so that older versions (at
	// least ones with this fix) can load savegames with extra data before the
	// first brace, as might be produced by a later engine version)
	for (;;) {
		start = t;
		if (!COM_ParseToken_Simple(&t, false, false, true))
			break;
		if (com_token[0] == '{')
		{
			t = start;
			break;
		}
	}

	// unlink all entities
	World_UnlinkAll(&sv.world);

// load the edicts out of the savegame file
	end = t;
	for (;;)
	{
		start = t;
		while (COM_ParseToken_Simple(&t, false, false, true))
			if (String_Does_Match(com_token, "}"))
				break;
		if (!COM_ParseToken_Simple(&start, false, false, true)) {
			// end of file
			break;
		}
		if (strcmp(com_token,"{")) {
			Mem_Free(text);
			Host_Error_Line ("First token isn't a brace");
		}

		if (entnum == -1) {
			if (developer_entityparsing.integer)
				Con_PrintLinef ("SV_Loadgame_f: loading globals");

			// parse the global vars
			PRVM_ED_ParseGlobals (prog, start);

			// restore the autocvar globals
			Cvar_UpdateAllAutoCvars(prog->console_cmd->cvars);
		}
		else
		{
			// parse an edict
			if (entnum >= MAX_EDICTS_32768) {
				Mem_Free(text);
				Host_Error_Line ("Host_PerformLoadGame: too many edicts in save file (reached MAX_EDICTS_32768 %d)", MAX_EDICTS_32768);
			}
			while (entnum >= prog->max_edicts)
				PRVM_MEM_IncreaseEdicts(prog);
			ent = PRVM_EDICT_NUM(entnum);
			memset(ent->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
			ent->free = false;

			if (developer_entityparsing.integer)
				Con_PrintLinef ("SV_Loadgame_f: loading edict %d", entnum);

			PRVM_ED_ParseEdict (prog, start, ent);

			// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict(ent);
		}

		end = t;
		entnum++;
	}

	prog->num_edicts = entnum;
	sv.time = time;

	for (i = 0;i < NUM_SPAWN_PARMS;i++)
		svs.clients[0].spawn_parms[i] = spawn_parms[i];

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: skipping until extended data");

	// read extended data if present
	// the extended data is stored inside a /* */ comment block, which the
	// parser intentionally skips, so we have to check for it manually here
	if (end) {
		while (*end == '\r' || *end == '\n')
			end++;
		// /*
		// // DarkPlaces extended savegame
		// sv.lightstyles 0 m
		// sv.lightstyles 63 a
		// sv.model_precache 1 maps/z2m1_somewhere.bsp
		// sv.sound_precache 1 weapons/r_exp3.wav

		if (end[0] == '/' && end[1] == '*' && (end[2] == '\r' || end[2] == '\n')) {
			if (developer_entityparsing.integer)
				Con_PrintLinef ("SV_Loadgame_f: loading extended data");

			Con_PrintLinef ("Loading extended DarkPlaces savegame");
			t = end + 2;
			memset(sv.lightstyles[0], 0, sizeof(sv.lightstyles));
			memset(sv.model_precache[0], 0, sizeof(sv.model_precache));
			memset(sv.sound_precache[0], 0, sizeof(sv.sound_precache));
			BufStr_Flush(prog);

			// t is ptr
			while (COM_ParseToken_Simple(&t, false, false, true)) {
				if (String_Does_Match(com_token, "sv.lightstyles")) {
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_LIGHTSTYLES_256)
						strlcpy(sv.lightstyles[i], com_token, sizeof(sv.lightstyles[i]));
					else
						Con_PrintLinef ("unsupported lightstyle %d " QUOTED_S, i, com_token);
				}
				else if (String_Does_Match(com_token, "sv.model_precache")) {
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_MODELS_8192) {
						strlcpy(sv.model_precache[i], com_token, sizeof(sv.model_precache[i]));
						sv.models[i] = Mod_ForName (sv.model_precache[i], true, false, sv.model_precache[i][0] == '*' ? sv.worldname : NULL);
					}
					else
						Con_PrintLinef ("unsupported model %d " QUOTED_S, i, com_token);
				}
				else if (String_Does_Match(com_token, "sv.sound_precache")) {
					COM_ParseToken_Simple(&t, false, false, true);
					i = atoi(com_token);
					COM_ParseToken_Simple(&t, false, false, true);
					if (i >= 0 && i < MAX_SOUNDS_4096)
						strlcpy(sv.sound_precache[i], com_token, sizeof(sv.sound_precache[i]));
					else
						Con_PrintLinef ("unsupported sound %d " QUOTED_S, i, com_token);
				}
				else if (String_Does_Match(com_token, "sv.buffer")) {
					if (COM_ParseToken_Simple(&t, false, false, true)) {
						i = atoi(com_token);
						if (i >= 0) {
							k = STRINGBUFFER_SAVED;
							if (COM_ParseToken_Simple(&t, false, false, true))
								k |= atoi(com_token);
							if (!BufStr_FindCreateReplace(prog, i, k, "string"))
								Con_PrintLinef (CON_ERROR "failed to create stringbuffer %d", i);
						}
						else
							Con_PrintLinef ("unsupported stringbuffer index %d " QUOTED_S, i, com_token);
					}
					else
						Con_PrintLinef ("unexpected end of line when parsing sv.buffer (expected buffer index)");
				}
				else if (String_Does_Match(com_token, "sv.bufstr"))
				{
					if (!COM_ParseToken_Simple(&t, false, false, true))
						Con_PrintLinef ("unexpected end of line when parsing sv.bufstr");
					else
					{
						i = atoi(com_token);
						stringbuffer = BufStr_FindCreateReplace(prog, i, STRINGBUFFER_SAVED, "string");
						if (stringbuffer)
						{
							if (COM_ParseToken_Simple(&t, false, false, true))
							{
								k = atoi(com_token);
								if (COM_ParseToken_Simple(&t, false, false, true))
									BufStr_Set(prog, stringbuffer, k, com_token);
								else
									Con_PrintLinef ("unexpected end of line when parsing sv.bufstr (expected string)");
							}
							else
								Con_PrintLinef ("unexpected end of line when parsing sv.bufstr (expected strindex)");
						}
						else
							Con_PrintLinef (CON_ERROR "failed to create stringbuffer %d " QUOTED_S, i, com_token);
					}
				}	
				// skip any trailing text or unrecognized commands
				while (COM_ParseToken_Simple(&t, true, false, true) && strcmp(com_token, "\n"))
					;
			}
		}
	}
	// Success
	Mem_Free(text); // AXX1 END

	// remove all temporary flagged string buffers (ones created with BufStr_FindCreateReplace)
	numbuffers = (int)Mem_ExpandableArray_IndexRange(&prog->stringbuffersarray);
	for (i = 0; i < numbuffers; i++) {
		if ( (stringbuffer = (prvm_stringbuffer_t *)Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, i)) )
			if (stringbuffer->flags & STRINGBUFFER_TEMP)
				BufStr_Del(prog, stringbuffer);
	}

	if (developer_entityparsing.integer)
		Con_PrintLinef ("SV_Loadgame_f: finished");

	// make sure we're connected to loopback
	if (sv.active && host.hook.ConnectLocal)
		host.hook.ConnectLocal();
}
