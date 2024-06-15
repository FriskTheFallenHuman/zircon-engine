/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2021 DarkPlaces contributors

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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

#include <time.h>
#include "libcurl.h"
#include "taskqueue.h"
#include "utf8lib.h"

/*

A server can always be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

host_static_t host;

// pretend frames take this amount of time (in seconds), 0 = realtime
cvar_t host_framerate = {CF_CLIENT | CF_SERVER, "host_framerate","0", "locks frame timing to this value in seconds, 0.05 is 20fps for example, note that this can easily run too fast, use cl_maxfps if you want to limit your framerate instead, or sys_ticrate to limit server speed"};
// shows time used by certain subsystems
cvar_t host_speeds = {CF_CLIENT | CF_SERVER, "host_speeds","0", "reports how much time is used in server/graphics/sound"};
cvar_t host_maxwait = {CF_CLIENT | CF_SERVER, "host_maxwait","1000", "maximum sleep time requested from the operating system in millisecond. Larger sleeps will be done using multiple host_maxwait length sleeps. Lowering this value will increase CPU load, but may help working around problems with accuracy of sleep times."};

cvar_t developer = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "developer","0", "shows debugging messages and information (recommended for all developers and level designers); the value -1 also suppresses buffering and logging these messages"};
cvar_t developer_stuffcmd = {CF_CLIENT, "developer_stuffcmd", "0", "prints stuffcmd text to the console for debugging [Zircon]"};
cvar_t developer_keycode = {CF_CLIENT, "developer_keycode", "0", "prints key scancode information for debugging [Zircon]"};
cvar_t developer_zext = {CF_SHARED, "developer_zext", "0", "prints Zircon extension information for debugging [Zircon]"};

cvar_t developer_svc = {CF_CLIENT, "developer_svc", "0", "prints svc messages received ignoring common ones, 2 prints them all [Zircon]"};

cvar_t developer_extra = {CF_CLIENT | CF_SERVER, "developer_extra", "0", "prints additional debugging messages, often very verbose!"};
cvar_t developer_insane = {CF_CLIENT | CF_SERVER, "developer_insane", "0", "prints huge streams of information about internal workings, entire contents of files being read/written, etc.  Not recommended!"};
cvar_t developer_loadingfile_fs = {CF_CLIENT | CF_SERVER, "developer_loadingfile_fs","0", "prints name and size of every file loaded via the FS_LoadFile function (which is almost everything)"};
cvar_t developer_loading = {CF_CLIENT | CF_SERVER, "developer_loading","0", "prints information about files as they are loaded or unloaded successfully"};
cvar_t developer_spawnfunction_warnings = {CF_CLIENT | CF_SERVER, "developer_spawnfunction_warnings","-3", "whether to print no spawn function warnings 0 = never, 1 = always, -3 = yes, but not for non-Quake map formats [Zircon]"}; // Baker: -3 means yes except for Quake 3 maps because Quake 3 maps almost always have entities that no QC would have a spawn function for
cvar_t developer_entityparsing = {CF_CLIENT, "developer_entityparsing", "0", "prints detailed network entities information each time a packet is received"};
cvar_t developer_execstring = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "developer_execstring", "0", "prints detailed on command executing to debug console [Zircon]"};


cvar_t timestamps = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timestamps", "0", "prints timestamps on console messages"};
cvar_t timeformat = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timeformat", "[%Y-%m-%d %H:%M:%S] ", "time format to use on timestamped console messages"};

cvar_t sessionid = {CF_CLIENT | CF_SERVER | CF_READONLY, "sessionid", "", "ID of the current session (use the -sessionid parameter to set it); this is always either empty or begins with a dot (.)"};
cvar_t locksession = {CF_CLIENT | CF_SERVER, "locksession", "0", "Lock the session? 0 = no, 1 = yes and abort on failure, 2 = yes and continue on failure"};

cvar_t host_isclient = {CF_SHARED | CF_READONLY, "_host_isclient", "0", "If 1, clientside is active."};

/*
================
Host_AbortCurrentFrame

aborts the current host frame and goes on with the next one
================
*/
void Host_AbortCurrentFrame(void)
{
	// in case we were previously nice, make us mean again
	Sys_MakeProcessMean();

	longjmp (host.abortframe, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error_Line (const char *error, ...)
{
	static char hosterrorstring1[MAX_INPUTLINE_16384]; // THREAD UNSAFE
	static char hosterrorstring2[MAX_INPUTLINE_16384]; // THREAD UNSAFE
	static qbool hosterror = false;
	va_list argptr;

	// turn off rcon redirect if it was active when the crash occurred
	// to prevent loops when it is a networking problem
	Con_Rcon_Redirect_Abort();

	va_start (argptr,error);
	dpvsnprintf (hosterrorstring1,sizeof(hosterrorstring1),error,argptr);
	va_end (argptr);

	Con_PrintLinef (CON_ERROR "Host_Error: %s", hosterrorstring1);

	// LadyHavoc: if crashing very early, or currently shutting down, do
	// Sys_Error instead
	if (host.superframecount < 3 || host.state == host_shutdown)
		Sys_Error ("Host_Error: %s", hosterrorstring1);

	if (hosterror)
		Sys_Error ("Host_Error: recursively entered (original error was: %s    new error is: %s)", hosterrorstring2, hosterrorstring1);
	hosterror = true;

	strlcpy(hosterrorstring2, hosterrorstring1, sizeof(hosterrorstring2));

	CL_Parse_DumpPacket();

	CL_Parse_ErrorCleanUp();

	//PR_Crash();

	// print out where the crash happened, if it was caused by QC (and do a cleanup)
	PRVM_Crash(SVVM_prog);
	PRVM_Crash(CLVM_prog);
#ifdef CONFIG_MENU
	PRVM_Crash(MVM_prog);
#endif

	Cvar_SetValueQuick(&csqc_progcrc, -1);
	Cvar_SetValueQuick(&csqc_progsize, -1);

	if (host.hook.SV_Shutdown)
		host.hook.SV_Shutdown();

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s",hosterrorstring2);	// dedicated servers exit

	CL_Disconnect();
	cls.demonum = -1;
	if (cls.levelshots_index_plus1) {
		void levels_shot_decide_map_and_act (int after_this_idx0);
		levels_shot_decide_map_and_act (UNPLUS1(cls.levelshots_index_plus1));
	}

	hosterror = false;

	Host_AbortCurrentFrame();
}

/*
==================
Host_Quit_f
==================
*/
static void Host_Quit_f(cmd_state_t *cmd)
{
	if (host.state == host_shutdown)
		Con_PrintLinef ("shutting down already!");
	else
#if 1 // Baker: To allow "zircon.exe +quit"
		Sys_Quit(0);
#else
		host.state = host_shutdown;
#endif
}

static void Host_Version_f(cmd_state_t *cmd)
{
	Con_PrintLinef ("Version: %s build %s", gamename, buildstring);
}

static void Host_Framerate_c(cvar_t *var)
{
	if (var->value < 0.00001 && var->value != 0)
		Cvar_SetValueQuick(var, 0);
}

// TODO: Find a better home for this.
static void SendCvar_f(cmd_state_t *cmd)
{
	if (cmd->source == src_local && host.hook.SV_SendCvar)
	{
		host.hook.SV_SendCvar(cmd);
		return;
	}
	if (cmd->source == src_client && host.hook.CL_SendCvar)
	{
		host.hook.CL_SendCvar(cmd);
		return;
	}
}

/*
===============
Host_SaveConfig_f

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_SaveConfig(const char *file)
{
	qfile_t *f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	// LadyHavoc: don't save a config if it crashed in startup
	if (host.superframecount >= 3 && cls.state != ca_dedicated && !Sys_CheckParm("-benchmark") && !Sys_CheckParm("-capturedemo"))
	{
		// Baker 1024: Prevent stale values of vid_width or vid_height writing to config
		Cvar_SetValueQuick (&vid_width, vid.width);
		Cvar_SetValueQuick (&vid_height, vid.height);
		Cvar_SetValueQuick (&vid_fullscreen, vid.fullscreen);

		f = FS_OpenRealFile(file, "wb", fs_quiet_FALSE);  // WRITE-EON save config
		if (!f)
		{
			Con_PrintLinef (CON_ERROR "Couldn't write %s.", file);
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (&cvars_all, f);

		FS_Close (f);
	} // if not ignoring save config
}

// Baker r1411: "saveconfig" takes an argument so "saveconfig mine" is possible.
static void Host_SaveConfig_f(cmd_state_t *cmd)
{
	char vabuf[1024];
	const char *s_file = CONFIGFILENAME;
	c_strlcpy (vabuf, s_file);

	if (Cmd_Argc(cmd) > 1) {
		s_file = Cmd_Argv(cmd, 1);
		c_strlcpy (vabuf, s_file);

		// If does not end with .cfg, default it.
		if (String_Ends_With(vabuf, ".cfg") == false) {
			c_strlcat (vabuf, ".cfg");
		}
	} // if argc > 1

	Con_PrintLinef ("Saving to %s", vabuf);

	Host_SaveConfig(vabuf);
}

void Host_WriteConfig_All_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("writeconfig_all <filename> requires a config name to save as");
		return;
	}

	char vabuf[1024];
	const char *s_file = Cmd_Argv(cmd, 1);
	c_strlcpy (vabuf, s_file);

	// If does not end with .cfg, default it.
	if (String_Ends_With (vabuf, ".cfg") == 0) {
		c_strlcat (vabuf, ".cfg");
	}
	Con_PrintLinef ("Saving to %s", vabuf);

	qfile_t *f = FS_OpenRealFile (vabuf, "wb", fs_quiet_FALSE); // WRITE-EON write config
	if (!f) {
		Con_PrintLinef (CON_ERROR "Couldn't write %s.", vabuf);
		return;
	}

	Cvar_WriteVariables_All (&cvars_all, f);
	FS_Close (f);
}

void Host_WriteConfig_All_Changed_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("writeconfig_all_changed <filename> requires a config name to save as");
		return;
	}

	char vabuf[1024];
	const char *s_file = Cmd_Argv(cmd, 1);
	c_strlcpy (vabuf, s_file);

	// If does not end with .cfg, default it.
	if (String_Ends_With (vabuf, ".cfg") == 0) {
		c_strlcat (vabuf, ".cfg");
	}
	Con_PrintLinef ("Saving to %s", vabuf);

	qfile_t *f = FS_OpenRealFile (vabuf, "wb", fs_quiet_FALSE); // WRITE-EON write config
	if (!f) {
		Con_PrintLinef (CON_ERROR "Couldn't write %s.", vabuf);
		return;
	}

	Cvar_WriteVariables_All_Changed (&cvars_all, f);
	FS_Close (f);
}

WARP_X_CALLERS_ (Host_InitOnce, Host_LoadConfig_f)
static void Host_AddConfigText(cmd_state_t *cmd)
{
	// set up the default startmap_sp and startmap_dm aliases (mods can
	// override these) and then execute the quake.rc startup script

#ifdef CONFIG_MENU
	if (cls.state != ca_dedicated) {
		Cbuf_InsertText(cmd, "alias +zoom " QUOTED_STR("set _saved_fov $fov; fov 20") "; alias -zoom " QUOTED_STR("fov $_saved_fov") NEWLINE);
		Cbuf_InsertText(cmd, "set _saved_fov $fov" NEWLINE);
	}
#endif // CONFIG_MENU

	if (gamemode == GAME_NEHAHRA)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map nehstart\"\nalias startmap_dm \"map nehstart\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TRANSFUSION)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map e1m1\"\n""alias startmap_dm \"map bb1\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TEU)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec teu.rc\n");
	else
		Cbuf_InsertText(cmd, "alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec " STARTCONFIGFILENAME "\n");

	Cbuf_Execute(cmd->cbuf);

	
}

/*
===============
Host_LoadConfig_f

Resets key bindings and cvars to defaults and then reloads scripts
===============
*/
int is_in_loadconfig; // Baker: To not print certain "command not found messages" like "gamma" during gamedir change
static void Host_LoadConfig_f(cmd_state_t *cmd)
{
	is_in_loadconfig = true;

	// reset all cvars, commands and aliases to init values
	Cmd_RestoreInitState();
#ifdef CONFIG_MENU
	// prepend a menu restart command to execute after the config
	Cbuf_InsertText(cmd_local, "\nmenu_restart\n");
#endif
	// reset cvars to their defaults, and then exec startup scripts again
	Host_AddConfigText(cmd_local);

	is_in_loadconfig = false;
}

/*
=======================
Host_InitOnceLocal
======================
*/
extern cvar_t r_texture_jpeg_fastpicmip;
static void Host_InitOnceLocal (void)
{
	Cmd_AddCommand(CF_SHARED, "quit", Host_Quit_f, "quit the game");
	Cmd_AddCommand(CF_SHARED, "version", Host_Version_f, "print engine version");
	Cmd_AddCommand(CF_SHARED, "saveconfig", Host_SaveConfig_f, "save settings to config.cfg (or a specified filename) immediately (also automatic when quitting)");
	Cmd_AddCommand(CF_SHARED, "loadconfig", Host_LoadConfig_f, "reset everything and reload configs");
	Cmd_AddCommand(CF_SHARED, "sendcvar", SendCvar_f, "sends the value of a cvar to the server as a sentcvar command, for use by QuakeC");

	Cmd_AddCommand(CF_SHARED, "writeconfig", Host_SaveConfig_f, "save settings to config.cfg (or a specified filename) immediately (also automatic when quitting) [Zircon]"); // Baker r1243: writeconfig

	Cmd_AddCommand(CF_SHARED, "writeconfig_all", Host_WriteConfig_All_f, "write all cvars to a specified filename [Zircon]");
	Cmd_AddCommand(CF_SHARED, "writeconfig_all_changed", Host_WriteConfig_All_Changed_f, "write all changed cvars to a specified filename [Zircon]");

	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterCallback (&host_framerate, Host_Framerate_c);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&host_maxwait);
	Cvar_RegisterVariable (&host_isclient);

	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&developer_stuffcmd);
	Cvar_RegisterVariable (&developer_keycode);
	Cvar_RegisterVariable (&developer_svc);

	Cvar_RegisterVariable (&developer_movement);
	Cvar_RegisterVariable (&developer_zext);

	Cvar_RegisterVariable (&developer_extra);
	Cvar_RegisterVariable (&developer_insane);
	Cvar_RegisterVariable (&developer_loadingfile_fs);
	Cvar_RegisterVariable (&developer_loading);
	Cvar_RegisterVariable (&developer_entityparsing);
	Cvar_RegisterVariable (&developer_execstring);
	Cvar_RegisterVariable (&developer_spawnfunction_warnings);

	Cvar_RegisterVariable (&timestamps);
	Cvar_RegisterVariable (&timeformat);

	Cvar_RegisterVariable (&r_texture_jpeg_fastpicmip);
}

char engineversion[128];
char engineversionshort[128]; // Baker r8002: Zircon console name

qbool sys_nostdout = false;

static qfile_t *locksession_fh = NULL;
static qbool locksession_run = false;
static void Host_InitOnce_Session(void)
{
	int i;
	char *buf;
	Cvar_RegisterVariable(&sessionid);
	Cvar_RegisterVariable(&locksession);

	// load the session ID into the read-only cvar
	if ((i = Sys_CheckParm("-sessionid")) && (i + 1 < sys.argc))
	{
		if (sys.argv[i+1][0] == '.')
			Cvar_SetQuick(&sessionid, sys.argv[i+1]);
		else
		{
			buf = (char *)Z_Malloc(strlen(sys.argv[i+1]) + 2);
			dpsnprintf(buf, sizeof(buf), ".%s", sys.argv[i+1]);
			Cvar_SetQuick(&sessionid, buf);
		}
	}
}

void Host_LockSession(void)
{
	if (locksession_run)
		return;
	locksession_run = true;
	if (locksession.integer != 0 && !Sys_CheckParm("-readonly"))
	{
		char vabuf[1024];
		char *p = va(vabuf, sizeof(vabuf), "%slock%s", *fs_userdir ? fs_userdir : fs_basedir, sessionid.string);
		FS_CreatePath(p);
		locksession_fh = FS_SysOpen(p, "wl", fs_nonblocking_false);
		// TODO maybe write the pid into the lockfile, while we are at it? may help server management tools
		if (!locksession_fh)
		{
			if (locksession.integer == 2)
			{
				Con_Printf (CON_WARN "WARNING: session lock %s could not be acquired. Please run with -sessionid and an unique session name. Continuing anyway.\n", p);
			}
			else
			{
				Sys_Error ("session lock %s could not be acquired. Please run with -sessionid and an unique session name.\n", p);
			}
		}
	}
}

void Host_UnlockSession(void)
{
	if (!locksession_run)
		return;
	locksession_run = false;

	if (locksession_fh)
	{
		FS_Close(locksession_fh);
		// NOTE: we can NOT unlink the lock here, as doing so would
		// create a race condition if another process created it
		// between our close and our unlink
		locksession_fh = NULL;
	}
}

/*
================
LOC_LoadFile
================
*/

// AURA 4.0
void sfake_start (char *s);
int sfake_is_eof ();
char *sfake_getlin ();
void sfake_close ();

//typedef struct stringlist_s
//{
//	/// maxstrings changes as needed, causing reallocation of strings[] array
//	int maxstrings;
//	int numstrings;
//	char **strings;
//} stringlist_t;

stringlist_t	rerelease_loc;

void LOC_LoadFile (void)
{
	char *enlocal_pure_sa;	// _sa means string allocated
	char *enlocal_sa;
	const char *sfile = "localization/loc_english.txt";

	enlocal_pure_sa = (char *)FS_LoadFile (sfile, tempmempool, fs_quiet_true, fs_size_ptr_null);

	if (!enlocal_pure_sa) {
		Con_PrintLinef  ("Language initialization not found");
		return;
	}

	Con_PrintLinef  ("Language initialization: %s", sfile);

	// Remove carriage returns if found
	enlocal_sa = String_Replace_Malloc (enlocal_pure_sa, "\r", "");

	Mem_Free (enlocal_pure_sa); // done with that

	sfake_start (enlocal_sa);

	stringlistfreecontents (&rerelease_loc);

	#define MAX_NUM_Q_ARGVS_50	50


	while (sfake_is_eof() == false) {
		char *sthis_line = sfake_getlin();
		char scopy_line[16384];

		if (!sthis_line)
			break;	// Some sort of EOF condition

		String_Edit_Trim (sthis_line);

		if (!sthis_line[0])
			continue; // it was all white space

		if (String_Starts_With (sthis_line, "//"))
			continue; // comment

		c_strlcpy (scopy_line, sthis_line);

		int rfake_argc = 0;
		char *rfake_argv[MAX_NUM_Q_ARGVS_50] = {0};

		// tokenize console
		String_Command_String_To_Argv (/*destructive edit*/ scopy_line, &rfake_argc, rfake_argv, MAX_NUM_Q_ARGVS_50);

		Con_DPrintLinef ("args %d", rfake_argc);

		if (rfake_argc == 3 && String_Match_Caseless (rfake_argv[1], "=" ) ) {
			char sdecode_arg2[16384];
			stringlistappend (&rerelease_loc, rfake_argv[0]);

			// Decode newlines
			c_strlcpy (sdecode_arg2, rfake_argv[2]);
			String_Edit_Replace (sdecode_arg2, sizeof(sdecode_arg2), "\\n", NEWLINE);

			stringlistappend (&rerelease_loc, sdecode_arg2);

			Con_DPrintLinef ("%s " NEWLINE QUOTED_S " " QUOTED_S, sthis_line, rfake_argv[0], sdecode_arg2);
		} else {
			//Con_PrintLinef ("%s " NEWLINE QUOTED_S " " QUOTED_S, sthis_line, rfake_argv[0], sdecode_arg2[2]);
		}


	} // each line

	// cleanup
	sfake_close ();
	freenull_ (enlocal_sa);
}

/*
================
LOC_GetString

Returns localized string if available, or input string otherwise
================
*/

const char *LOC_GetDecodeOld (const char *s_token)
{
	int j;
	for (j = 0; j < rerelease_loc.numstrings; j += 2) {
		char *sthis = rerelease_loc.strings[j];
		if (String_Match (sthis, s_token)) {
			return rerelease_loc.strings[j + 1]; //c_strlcpy (s_decode, rerelease_loc.strings[j + 1]);
		}
	} // for
	return NULL;
}

int LOC_GetDecode (const char *s)
{
	int j;
	int slen = (int)strlen(s);
	int best_idx = not_found_neg1;
	int best_len = -1;


	for (j = 0; j < rerelease_loc.numstrings; j += 2) {
		char *sthis = rerelease_loc.strings[j];
		if (String_Starts_With (s, sthis)) {
			int slen2 = strlen(sthis);
			if (slen == slen2)
				return j; // rerelease_loc.strings[j + 1]; //c_strlcpy (s_decode, rerelease_loc.strings[j + 1]);
			if (best_len == -1 || slen2 > best_len) {
				best_idx = j;
				best_len = slen2;

			}
		}
	} // for
	return best_idx; //not_found_neg1;
}

char s_decodebuf[16384];
#define ASSIGN(x) (x)

const char *LOC_GetString (const char *stxt)
{
	int is_nested_next = false;
	int cycles = 0;

	c_strlcpy (s_decodebuf, stxt);
	char *s_this_token;

  	while (ASSIGN (s_this_token = strchr (s_decodebuf, '$') ) /**/ ) {
		//char *s_tokend = &s_this_token[1];
		char *s_token = &s_this_token[1];
		cycles ++;
		int prefixsize =  s_this_token - s_decodebuf;
		int  j = LOC_GetDecode (s_token);
		int is_nested_now = is_nested_next;

		if (j == not_found_neg1) {
			return "(error)";//stxt;
		}

		//char *s_tokenx = rerelease_loc.strings[j + 0];
		char *s_decodx = rerelease_loc.strings[j + 1];

		//Sys_PrintToTerminal2 (va3(QUOTED_S, s_tokenx));
		//Sys_PrintToTerminal2 (va3(QUOTED_S, s_decodx));

		int slen0 = (int)strlen(rerelease_loc.strings[j + 0]);
#if 0 // gcc unused
		int slen1 = (int)strlen(rerelease_loc.strings[j + 1]);
#endif

		char *s_afterx = &s_decodebuf[slen0 + 1 /*for the dollar*/ ];

		char s_decode2[16384];
		int is_nest_replace  = (*s_afterx != 0);

		is_nested_next = 0;

		memcpy		(s_decode2, s_decodebuf, prefixsize);
		s_decode2[prefixsize] = 0;

		if (is_nested_now) {
			// Remove the next token replace into this one.
			//char *s_afterx2 = &s_decodebuf[slen0 + 1 /*for the dollar*/ ];

			//String_Edit_Replace (s_decode2, sizeof(s_decode2), s_token, ""); // Remove
			String_Edit_Replace (s_decode2, sizeof(s_decode2), "{0}", s_decodx); // Expand
			cycles ++;
			goto skip;
		}

		c_strlcat	(s_decode2, s_decodx);

		if (is_nest_replace) {
			char *s_num = s_afterx;
			String_Edit_Replace (s_decode2, sizeof(s_decode2), "{0}", s_num); // Expand
			cycles ++;
			goto skip;
		}

		if (strstr (s_decodx, "{0}")) {
			cycles ++;
			is_nested_next = true;
		}

		c_strlcat (s_decode2, s_afterx);

skip:
		c_strlcpy (s_decodebuf, s_decode2);
		//Sys_PrintToTerminal2 (va3(QUOTED_S, s_decodebuf));
	} // while

	if (cycles > 1 && String_Ends_With(s_decodebuf, NEWLINE) == false) {
		c_strlcat	(s_decodebuf, NEWLINE);
	}

	//Sys_PrintToTerminal2 (va3( QUOTED_S " to " QUOTED_S,  stxt, s_decodebuf ) );

	return s_decodebuf;
}

/*
====================
Host_InitOnce
====================
*/
float host_hoststartup_unique_num;

static void Host_InitOnce (void)
{
	int i;
	const char *os;
	char vabuf[1024];

	host_hoststartup_unique_num = Sys_DirtyTime ();

	host.hook.ConnectLocal = NULL;
	host.hook.Disconnect = NULL;
	host.hook.ToggleMenu = NULL;
	host.hook.CL_Intermission = NULL;
	host.hook.SV_Shutdown = NULL;

	host.state = host_init;

	if (setjmp(host.abortframe)) // Huh?!
		Sys_Error ("Engine initialization failed. Check the console (if available) for additional information.\n");

	if (Sys_CheckParm("-profilegameonly"))
		Sys_AllowProfiling(false);

	// LadyHavoc: quake never seeded the random number generator before... heh
	if (Sys_CheckParm("-benchmark"))
		srand(0); // predictable random sequence for -benchmark
	else
		srand((unsigned int)time(NULL));

	// FIXME: this is evil, but possibly temporary
	// LadyHavoc: doesn't seem very temporary...
	// LadyHavoc: made this a saved cvar
// COMMANDLINEOPTION: Console: -developer enables warnings and other notices (RECOMMENDED for mod developers)
	if (Sys_CheckParm("-developer")) {
		developer.value = developer.integer = 1;
		developer.string = "1";
	}

	if (Sys_CheckParm("-developer2") || Sys_CheckParm("-developer3")) {
		developer.value = developer.integer = 1;
		developer.string = "1";
		developer_extra.value = developer_extra.integer = 1;
		developer_extra.string = "1";
		developer_insane.value = developer_insane.integer = 1;
		developer_insane.string = "1";
		developer_memory.value = developer_memory.integer = 1;
		developer_memory.string = "1";
		developer_memorydebug.value = developer_memorydebug.integer = 1;
		developer_memorydebug.string = "1";
	}

	if (Sys_CheckParm("-developer3")) {
		gl_paranoid.integer = 1;gl_paranoid.string = "1";
		gl_printcheckerror.integer = 1;gl_printcheckerror.string = "1";
	}

// COMMANDLINEOPTION: Console: -nostdout disables text output to the terminal the game was launched from
	if (Sys_CheckParm("-nostdout"))
		sys_nostdout = 1;

	// -dedicated is checked in SV_ServerOptions() but that's too late for Cvar_RegisterVariable() to skip all the client-only cvars
	if (Sys_CheckParm ("-dedicated") || !cl_available)
		cls.state = ca_dedicated;

	// initialize console command/cvar/alias/command execution systems
	Cmd_InitOnce();

	// initialize memory subsystem cvars/commands
	Memory_InitOnce_Commands();

	// initialize console and logging and its cvars/commands
	Con_InitOnce();

	// initialize various cvars that could not be initialized earlier
	u8_InitOnce();
	Curl_InitOnce_Commands();
	Sys_InitOnce_Commands();
	COM_InitOnce_Commands();

	// initialize filesystem (including fs_basedir, fs_gamedir, -game, scr_screenshot_name)
	FS_InitOnce();

#if defined (_WIN32) && defined(CONFIG_MENU)
	// SLASH DEDICATED (-dedicated)
	if (cls.state == ca_dedicated)
		Sys_Console_InitOnce_WinQuake ();
#endif

	// construct a version string for the corner of the console
	os = DP_OS_NAME;
	c_dpsnprintf3 (engineversion, "%s %s %s", gamename, os, buildstring);
	Con_PrintLinef ("%s\n", engineversion);

	// Baker r8002: Zircon console name
	const char *sfmt = "%s " // ...
		#if defined(_WIN32) && defined(_WIN64)
			"64 "
		#endif // CORE_SDL
		#if !defined(_MSC_VER) && defined(_WIN32)
			"GCC "
		#endif // CORE_SDL
		#ifdef CORE_SDL
			"SDL2 "
		#endif // CORE_SDL
		#if defined (_DEBUG) || defined (DEBUG) // MAC 2nd one
			"(D) "
		#endif // _DEBUG
		"%s";

	c_dpsnprintf2 (engineversionshort, sfmt, gamename, buildstringshort);

	//Con_PrintLinef ("%s", engineversion);

	// initialize process nice level
	Sys_InitOnce_ProcessNice();

	// initialize ixtable
	Mathlib_InitOnce();

	// register the cvars for session locking
	Host_InitOnce_Session();

	// must be after FS_InitOnce
	Crypto_InitOnce();
	Crypto_InitOnce_Commands();

	NetConn_InitOnce();
	Curl_InitOnce();
	PRVM_InitOnce();
	Mod_InitOnce();
	World_InitOnce();
	SV_InitOnce();
	Host_InitOnceLocal();

	Thread_InitOnce();
	TaskQueue_InitOnce();

	CL_InitOnce();

	// save off current state of aliases, commands and cvars for later restore if FS_GameDir_f is called
	// NOTE: menu commands are freed by Cmd_RestoreInitState
	Cmd_Host_Init_SaveInitState();

	// FIXME: put this into some neat design, but the menu should be allowed to crash
	// without crashing the whole game, so this should just be a short-time solution

	// here comes the not so critical stuff

	Host_AddConfigText(cmd_local);

	// if quake.rc is missing, use default
	if (!FS_FileExists("quake.rc"))
	{
		Cbuf_AddTextLine(cmd_local,
			"exec default.cfg" NEWLINE
			"exec " CONFIGFILENAME NEWLINE
			"exec autoexec.cfg");
		Cbuf_Execute(cmd_local->cbuf);
	}

	host.state = host_active;

	CL_StartVideo();

	Log_Start();

	if (cls.state != ca_dedicated)
	{
		// put up the loading image so the user doesn't stare at a black screen...
		SCR_BeginLoadingPlaque(true);
#ifdef CONFIG_MENU
		MR_Init();
#endif
	}

	// check for special benchmark mode
// COMMANDLINEOPTION: Client: -benchmark <demoname> runs a timedemo and quits, results of any timedemo can be found in gamedir/benchmark.log (for example id1/benchmark.log)
	i = Sys_CheckParm("-benchmark");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "timedemo %s\n", sys.argv[i + 1]));
		Cbuf_Execute(cmd_local->cbuf);
	}

	// check for special demo mode
// COMMANDLINEOPTION: Client: -demo <demoname> runs a playdemo and quits
	i = Sys_CheckParm("-demo");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "playdemo %s\n", sys.argv[i + 1]));
		Cbuf_Execute(cmd_local->cbuf);
	}

#ifdef CONFIG_VIDEO_CAPTURE
// COMMANDLINEOPTION: Client: -capturedemo <demoname> captures a playdemo and quits
	i = Sys_CheckParm("-capturedemo");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "playdemo %s\ncl_capturevideo 1\n", sys.argv[i + 1]));
		Cbuf_Execute((cmd_local)->cbuf);
	}
#endif

	if (cls.state == ca_dedicated || Sys_CheckParm("-listen"))
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddTextLine (cmd_local, "startmap_dm");
		Cbuf_Execute(cmd_local->cbuf);
	}

	if (!sv.active && !cls.demoplayback && !cls.connect_trying) {
#ifdef CONFIG_MENU
		extern cvar_t cl_startdemos;
		if (cl_startdemos.value)
			Cbuf_AddTextLine (cmd_local, "togglemenu 1");
#endif
		Cbuf_Execute(cmd_local->cbuf);
	}

	Con_DPrint("========Initialized=========\n");

	if (cls.state != ca_dedicated)
		SV_StartThread();
}

/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qbool isdown = false;

	if (isdown)
	{
		Con_PrintLinef ("recursive shutdown");
		return;
	}
	if (setjmp(host.abortframe))
	{
		Con_PrintLinef ("aborted the quitting frame?!?");
		return;
	}
	isdown = true;

	if (cls.state != ca_dedicated)
		CL_Shutdown();

	// end the server thread
	if (svs.threaded)
		SV_StopThread();

	// shut down local server if active
	WARP_X_ (SV_Shutdown)
	if (host.hook.SV_Shutdown)
		host.hook.SV_Shutdown();

	// AK shutdown PRVM
	// AK hmm, no PRVM_Shutdown(); yet

	Host_SaveConfig (CONFIGFILENAME);

	Curl_Shutdown ();
	NetConn_Shutdown ();

	SV_StopThread();
	TaskQueue_Shutdown();
	Thread_Shutdown();
	Cmd_Shutdown();
	Sys_Shutdown();
	Log_Close();
	Crypto_Shutdown();

	Host_UnlockSession();

	Con_Shutdown();
	Memory_Shutdown();
}

//============================================================================

/*
==================
Host_Frame

Runs all active servers
==================
*/
static double Host_Frame(double time)
{
	double cl_wait, sv_wait;

	TaskQueue_Frame(false);

	// keep the random time dependent, but not when playing demos/benchmarking
	if (!*sv_random_seed.string && !host.restless)
		rand();

	NetConn_UpdateSockets();

	Log_DestBuffer_Flush();

	// Run any downloads
	Curl_Frame();

	// get new SDL events and add commands from keybindings to the cbuf
	Sys_SendKeyEvents();

	WARP_X_ (Sys_PrintfToTerminal)

	// process console commands
	Cbuf_Frame(host.cbuf);

	if (cls.levelshots_index_plus1) {
		// cl.levelshots_index_plus1
		int delta = host.superframecount - cls.cl_first_world_frame_rendered;
		
		switch (delta) {
		case 0:

			break;

		case 10:
			WARP_X_ (R_LevelShot_Here_f)
			Cbuf_AddTextLine (cmd_local, "levelshot_here");
			// Queue levelshot and do next map name
			break;

		default:
			// Waiting for 5 frames or something
			break;
			
		}
	}

	R_TimeReport("---");

	// if the accumulators haven't become positive yet, wait a while
	sv_wait = - SV_Frame(time);
	cl_wait = - CL_Frame(time);

	

	Mem_CheckSentinelsGlobal ();

	if (cls.state == ca_dedicated)
		return sv_wait; // dedicated
	else if (!sv.active || svs.threaded)
		return cl_wait; // connected to server, main menu, or server is on different thread
	else
		return Smallest (cl_wait, sv_wait); // listen server or singleplayer
}

static inline double Host_Sleep(double time)
{
	double delta, time0;

	// convert to microseconds
	time *= 1000000.0;

	if (time < 1 || host.restless)
		return 0; // not sleeping this frame

	if (host_maxwait.value <= 0)
		time = min(time, 1000000.0);
	else
		time = min(time, host_maxwait.value * 1000.0);

	time0 = Sys_DirtyTime();
	if (sv_checkforpacketsduringsleep.integer && !sys_usenoclockbutbenchmark.integer && !svs.threaded) {
		NetConn_SleepMicroseconds((int)time);
		if (cls.state != ca_dedicated)
			NetConn_ClientFrame(); // helps server browser get good ping values
		// TODO can we do the same for ServerFrame? Probably not.
	}
	else
	{
		if (cls.state != ca_dedicated)
			Curl_Select(&time);
		Sys_Sleep((int)time);
	}

	delta = Sys_DirtyTime() - time0;
	if (delta < 0 || delta >= 1800)
		delta = 0;

//	R_TimeReport("sleep");
	return delta;
}

// Cloudwalk: Most overpowered function declaration...
static inline double Host_UpdateTime (double newtime, double oldtime)
{
	double time = newtime - oldtime;

	if (time < 0)
	{
		// warn if it's significant
		if (time < -0.01)
			Con_Printf (CON_WARN "Host_UpdateTime: time stepped backwards (went from %f to %f, difference %f)\n", oldtime, newtime, time);
		time = 0;
	}
	else if (time >= 1800)
	{
		Con_Printf (CON_WARN "Host_UpdateTime: time stepped forward (went from %f to %f, difference %f)\n", oldtime, newtime, time);
		time = 0;
	}

	return time;
}

void Host_Main(void)
{
	double time, oldtime, sleeptime;

	Host_InitOnce(); // Start!

	host.realtime = 0;
	oldtime = Sys_DirtyTime();

	// Main event loop
	while(host.state != host_shutdown)
	{
		// Something bad happened, or the server disconnected
		if (setjmp(host.abortframe))
		{
			host.state = host_active; // In case we were loading
			continue;
		}

		host.dirtytime = Sys_DirtyTime();
		host.realtime += time = Host_UpdateTime(host.dirtytime, oldtime);

		sleeptime = Host_Frame(time);
		oldtime = host.dirtytime;
		host.superframecount ++;

		sleeptime -= Sys_DirtyTime() - host.dirtytime; // execution time
		host.sleeptime = Host_Sleep(sleeptime);
	}

	return;
}
