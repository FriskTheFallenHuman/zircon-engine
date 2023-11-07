// Baker: cl_maxconsole_menu_fps, Prevent stale values of vid_width or vid_height writing to config

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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

#include <time.h>
#include "libcurl.h"
#include "cdaudio.h"
#include "cl_video.h"
#include "progsvm.h"
#include "csprogs.h"
#include "sv_demo.h"
#include "snd_main.h"
#include "thread.h"
#include "utf8lib.h"

/*

A server can always be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

// how many frames have occurred
// (checked by Host_Error and Host_SaveConfig_f)
int host_framecount = 0;
// LadyHavoc: set when quit is executed
qbool host_shuttingdown = false;

// the accumulated mainloop time since application started (with filtering), without any slowmo or clamping
double realtime;
// the main loop wall time for this frame
double host_dirtytime;

// current client
client_t *host_client;

jmp_buf host_abortframe;

// pretend frames take this amount of time (in seconds), 0 = realtime
cvar_t host_framerate = {CF_CLIENT | CF_SERVER, "host_framerate","0", "locks frame timing to this value in seconds, 0.05 is 20fps for example, note that this can easily run too fast, use cl_maxfps if you want to limit your framerate instead, or sys_ticrate to limit server speed"};
// shows time used by certain subsystems
cvar_t host_speeds = {CF_CLIENT | CF_SERVER, "host_speeds","0", "reports how much time is used in server/graphics/sound"};
cvar_t host_maxwait = {CF_CLIENT | CF_SERVER, "host_maxwait","1000", "maximum sleep time requested from the operating system in millisecond. Larger sleeps will be done using multiple host_maxwait length sleeps. Lowering this value will increase CPU load, but may help working around problems with accuracy of sleep times."};
//cvar_t host_path_protection = {0, //"host_path_protection","1", "Preventing wrong .lit or .ent //or .rtlights files for a map by limiting search based on //where the map itself resides.. [Zircon]"};



cvar_t developer = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "developer","0", "shows debugging messages and information (recommended for all developers and level designers); the value -1 also suppresses buffering and logging these messages"};
cvar_t developer_extra = {CF_CLIENT | CF_SERVER, "developer_extra", "0", "prints additional debugging messages, often very verbose!"};
cvar_t developer_insane = {CF_CLIENT | CF_SERVER, "developer_insane", "0", "prints huge streams of information about internal workings, entire contents of files being read/written, etc.  Not recommended!"};
cvar_t developer_loadfile = {CF_CLIENT | CF_SERVER, "developer_loadfile","0", "prints name and size of every file loaded via the FS_LoadFile function (which is almost everything)"};
cvar_t developer_loading = {CF_CLIENT | CF_SERVER, "developer_loading","0", "prints information about files as they are loaded or unloaded successfully"};
cvar_t developer_entityparsing = {CF_CLIENT, "developer_entityparsing", "0", "prints detailed network entities information each time a packet is received"};

cvar_t timestamps = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timestamps", "0", "prints timestamps on console messages"};
cvar_t timeformat = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timeformat", "[%Y-%m-%d %H:%M:%S] ", "time format to use on timestamped console messages"};

cvar_t sessionid = {CF_CLIENT | CF_SERVER | CF_READONLY, "sessionid", "", "ID of the current session (use the -sessionid parameter to set it); this is always either empty or begins with a dot (.)"};
cvar_t locksession = {CF_CLIENT | CF_SERVER, "locksession", "0", "Lock the session? 0 = no, 1 = yes and abort on failure, 2 = yes and continue on failure"};

/*
================
Host_AbortCurrentFrame

aborts the current host frame and goes on with the next one
================
*/
void Host_AbortCurrentFrame(void) DP_FUNC_NORETURN;
void Host_AbortCurrentFrame(void)
{
	// in case we were previously nice, make us mean again
	Sys_MakeProcessMean();

	longjmp (host_abortframe, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (const char *error, ...)
{
	static char hosterrorstring1[MAX_INPUTLINE]; // THREAD UNSAFE
	static char hosterrorstring2[MAX_INPUTLINE]; // THREAD UNSAFE
	static qbool hosterror = false;
	va_list argptr;

	// turn off rcon redirect if it was active when the crash occurred
	// to prevent loops when it is a networking problem
	Con_Rcon_Redirect_Abort();

	va_start (argptr,error);
	dpvsnprintf (hosterrorstring1,sizeof(hosterrorstring1),error,argptr);
	va_end (argptr);

	Con_PrintLinef ("Host_Error: %s", hosterrorstring1);

	// LadyHavoc: if crashing very early, or currently shutting down, do
	// Sys_Error instead
	if (host_framecount < 3 || host_shuttingdown)
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

	cl.csqc_loaded = false;
	Cvar_SetValueQuick(&csqc_progcrc, -1);
	Cvar_SetValueQuick(&csqc_progsize, -1);

	SV_LockThreadMutex();
	Host_ShutdownServer ();
	SV_UnlockThreadMutex();

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s",hosterrorstring2);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	hosterror = false;

	Host_AbortCurrentFrame();
}

static void Host_ServerOptions (void)
{
	int i;

	// general default
	svs.maxclients = 8;

// COMMANDLINEOPTION: Server: -dedicated [playerlimit] starts a dedicated server (with a command console), default playerlimit is 8
// COMMANDLINEOPTION: Server: -listen [playerlimit] starts a multiplayer server with graphical client, like singleplayer but other players can connect, default playerlimit is 8
	// if no client is in the executable or -dedicated is specified on
	// commandline, start a dedicated server
	i = Sys_CheckParm ("-dedicated");
	if (i || !cl_available)
	{
		cls.state = ca_dedicated;
		// check for -dedicated specifying how many players
		if (i && i + 1 < com_argc && atoi (com_argv[i+1]) >= 1)
			svs.maxclients = atoi (com_argv[i+1]);
		if (Sys_CheckParm ("-listen"))
			Con_Printf ("Only one of -dedicated or -listen can be specified\n");
		// default sv_public on for dedicated servers (often hosted by serious administrators), off for listen servers (often hosted by clueless users)
		Cvar_SetValue("sv_public", 1);
	}
	else if (cl_available)
	{
		// client exists and not dedicated, check if -listen is specified
		cls.state = ca_disconnected;
		i = Sys_CheckParm ("-listen");
		if (i)
		{
			// default players unless specified
			if (i + 1 < com_argc && atoi (com_argv[i+1]) >= 1)
				svs.maxclients = atoi (com_argv[i+1]);
		}
		else
		{
			// default players in some games, singleplayer in most
			if (gamemode != GAME_GOODVSBAD2 && !IS_NEXUIZ_DERIVED(gamemode) && gamemode != GAME_BATTLEMECH)
				svs.maxclients = 1;
		}
	}

	svs.maxclients = svs.maxclients_next = bound(1, svs.maxclients, MAX_SCOREBOARD);

	svs.clients = (client_t *)Mem_Alloc(sv_mempool, sizeof(client_t) * svs.maxclients);

	if (svs.maxclients > 1 && !deathmatch.integer && !coop.integer)
		Cvar_SetValueQuick(&deathmatch, 1);
}

/*
=======================
Host_InitLocal
======================
*/
void Host_SaveConfig_f(void);
void Host_LoadConfig_f(void);
extern cvar_t sv_writepicture_quality;
extern cvar_t r_texture_jpeg_fastpicmip;
static void Host_InitLocal (void)
{
	Cmd_AddCommand ("saveconfig", Host_SaveConfig_f, "save settings to config.cfg (or a specified filename) immediately (also automatic when quitting)");
	Cmd_AddCommand ("loadconfig", Host_LoadConfig_f, "reset everything and reload configs");

	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&host_maxwait);
//	Cvar_RegisterVariable (&host_path_protection);


	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&developer_extra);
	Cvar_RegisterVariable (&developer_insane);
	Cvar_RegisterVariable (&developer_loadfile);
	Cvar_RegisterVariable (&developer_loading);
	Cvar_RegisterVariable (&developer_entityparsing);

	Cvar_RegisterVariable (&timestamps);
	Cvar_RegisterVariable (&timeformat);

	Cvar_RegisterVariable (&r_texture_jpeg_fastpicmip);
}


/*
===============
Host_SaveConfig_f

Writes key bindings and archived cvars to config.cfg
===============
*/
static void Host_SaveConfig_to(const char *file)
{
	qfile_t *f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	// LadyHavoc: don't save a config if it crashed in startup
	if (host_framecount >= 3 && cls.state != ca_dedicated && !Sys_CheckParm("-benchmark") && !Sys_CheckParm("-capturedemo"))
	{
		// Baker 1024: Prevent stale values of vid_width or vid_height writing to config
		Cvar_SetValueQuick (&vid_width, vid.width);
		Cvar_SetValueQuick (&vid_height, vid.height);
		Cvar_SetValueQuick (&vid_fullscreen, vid.fullscreen);

		f = FS_OpenRealFile(file, "wb", false);
		if (!f)
		{
			Con_Printf("Couldn't write %s.\n", file);
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		FS_Close (f);
	}
}
void Host_SaveConfig(void)
{
	Host_SaveConfig_to(CONFIGFILENAME);
}


void Host_SaveConfig_f(void)
{
	char vabuf[1024];
	const char *file = CONFIGFILENAME;
	strlcpy (vabuf, file, sizeof(vabuf));
	if(Cmd_Argc() >= 2) {
		file = Cmd_Argv(1);
		if (String_Does_End_With (file, ".cfg") == 0) {
			dpsnprintf (vabuf, sizeof(vabuf), "%s.cfg", file);
		} else {
			strlcpy (vabuf, file, sizeof(vabuf));
		}

		Con_PrintLinef ("Saving to %s", vabuf);
	}
	else {
		Con_PrintLinef ("Saving to %s", vabuf);
	}

	Host_SaveConfig_to (vabuf);
}

static void Host_AddConfigText(void)
{
	// set up the default startmap_sp and startmap_dm aliases (mods can
	// override these) and then execute the quake.rc startup script
	if (gamemode == GAME_NEHAHRA)
		Cbuf_InsertText ("alias startmap_sp \"map nehstart\"\nalias startmap_dm \"map nehstart\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TRANSFUSION)
		Cbuf_InsertText ("alias startmap_sp \"map e1m1\"\n""alias startmap_dm \"map bb1\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TEU)
		Cbuf_InsertText("alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec teu.rc\n");
	else
		Cbuf_InsertText ("alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec " STARTCONFIGFILENAME "\n");
}

/*
===============
Host_LoadConfig_f

Resets key bindings and cvars to defaults and then reloads scripts
===============
*/
void Host_LoadConfig_f(void)
{
	// reset all cvars, commands and aliases to init values
	Cmd_RestoreInitState();
#ifdef CONFIG_MENU
	// prepend a menu restart command to execute after the config
	Cbuf_InsertText("\nmenu_restart\n");
#endif
	// reset cvars to their defaults, and then exec startup scripts again
	Host_AddConfigText();
}


/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;

	Con_DPrintf("Host_ShutdownServer\n");

	if (!sv.active)
		return;

	NetConn_Heartbeat(2);
	NetConn_Heartbeat(2);

// make sure all the clients know we're disconnecting
	World_End(&sv.world);
	if(prog->loaded)
	{
		if(PRVM_serverfunction(SV_Shutdown))
		{
			func_t s = PRVM_serverfunction(SV_Shutdown);
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_serverfunction(SV_Shutdown) = 0; // prevent it from getting called again
			prog->ExecuteProgram(prog, s,"SV_Shutdown() required");
		}
	}
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
		if (host_client->active)
			SV_DropClient(false, /*Baker fill this in*/ ""); // server shutdown

	NetConn_CloseServerPorts();

	sv.active = false;
//
// clear structures
//
	memset(&sv, 0, sizeof(sv));
	memset(svs.clients, 0, svs.maxclients*sizeof(client_t));

	cl.islocalgame = false;
}


//============================================================================

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
static void Host_GetConsoleCommands (void)
{
	char *cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}

/*
==================
Host_TimeReport

Returns a time report string, for example for
==================
*/
const char *SV_TimingReport(char *buf, size_t buflen)
{
	return va(buf, buflen, "%.1f%% CPU, %.2f%% lost, offset avg %.1fms, max %.1fms, sdev %.1fms", svs.perf_cpuload * 100, svs.perf_lost * 100, svs.perf_offset_avg * 1000, svs.perf_offset_max * 1000, svs.perf_offset_sdev * 1000);
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
static void Host_Init(void);
extern cvar_t cl_maxidlefps, cl_maxfps_alwayssleep, cl_maxconsole_menu_fps, cl_maxfps, cl_maxphysicsframesperserverframe;

void Host_Main(void)
{
	double time1 = 0;
	double time2 = 0;
	double time3 = 0;
	double cl_timer = 0, sv_timer = 0;
	double clframetime, deltacleantime, olddirtytime, dirtytime;
	double wait;
	int pass1, pass2, pass3, i;
	char vabuf[1024];
	qbool playing;

	Host_Init();

	realtime = 0;
	host_dirtytime = Sys_DirtyTime();
	for (;;)
	{
		if (setjmp(host_abortframe))
		{
			SCR_ClearLoadingScreen(false);
			continue;			// something bad happened, or the server disconnected
		}

		olddirtytime = host_dirtytime;
		dirtytime = Sys_DirtyTime();
		deltacleantime = dirtytime - olddirtytime;
		if (deltacleantime < 0)
		{
			// warn if it's significant
			if (deltacleantime < -0.01)
				Con_Printf("Host_Mingled: time stepped backwards (went from %f to %f, difference %f)\n", olddirtytime, dirtytime, deltacleantime);
			deltacleantime = 0;
		}
		else if (deltacleantime >= 1800)
		{
			Con_Printf("Host_Mingled: time stepped forward (went from %f to %f, difference %f)\n", olddirtytime, dirtytime, deltacleantime);
			deltacleantime = 0;
		}
		realtime += deltacleantime;
		host_dirtytime = dirtytime;

		cl_timer += deltacleantime;
		sv_timer += deltacleantime;

		if (!svs.threaded)
		{
			svs.perf_acc_realtime += deltacleantime;

			// Look for clients who have spawned
			playing = false;
			for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
				if(host_client->begun)
					if(host_client->netconnection)
						playing = true;
			if(sv.time < 10)
			{
				// don't accumulate time for the first 10 seconds of a match
				// so things can settle
				svs.perf_acc_realtime = svs.perf_acc_sleeptime = svs.perf_acc_lost = svs.perf_acc_offset = svs.perf_acc_offset_squared = svs.perf_acc_offset_max = svs.perf_acc_offset_samples = 0;
			}
			else if(svs.perf_acc_realtime > 5)
			{
				svs.perf_cpuload = 1 - svs.perf_acc_sleeptime / svs.perf_acc_realtime;
				svs.perf_lost = svs.perf_acc_lost / svs.perf_acc_realtime;
				if(svs.perf_acc_offset_samples > 0)
				{
					svs.perf_offset_max = svs.perf_acc_offset_max;
					svs.perf_offset_avg = svs.perf_acc_offset / svs.perf_acc_offset_samples;
					svs.perf_offset_sdev = sqrt(svs.perf_acc_offset_squared / svs.perf_acc_offset_samples - svs.perf_offset_avg * svs.perf_offset_avg);
				}
				if(svs.perf_lost > 0 && developer_extra.integer)
					if(playing) // only complain if anyone is looking
						Con_DPrintf("Server can't keep up: %s\n", SV_TimingReport(vabuf, sizeof(vabuf)));
				svs.perf_acc_realtime = svs.perf_acc_sleeptime = svs.perf_acc_lost = svs.perf_acc_offset = svs.perf_acc_offset_squared = svs.perf_acc_offset_max = svs.perf_acc_offset_samples = 0;
			}
		}

		if (slowmo.value < 0.00001 && slowmo.value != 0)
			Cvar_SetValue("slowmo", 0);
		if (host_framerate.value < 0.00001 && host_framerate.value != 0)
			Cvar_SetValue("host_framerate", 0);

		// keep the random time dependent, but not when playing demos/benchmarking
		if(!*sv_random_seed.string && !cls.demoplayback)
			rand();

		// get new key events
		Key_EventQueue_Unblock();
		SndSys_SendKeyEvents();
		Sys_SendKeyEvents();

		NetConn_UpdateSockets();

		Log_DestBuffer_Flush();

		// receive packets on each main loop iteration, as the main loop may
		// be undersleeping due to select() detecting a new packet
		if (sv.active && !svs.threaded)
			NetConn_ServerFrame();

		Curl_Run();

		// check for commands typed to the host
		Host_GetConsoleCommands();

		// when a server is running we only execute console commands on server frames
		// (this mainly allows frikbot .way config files to work properly by staying in sync with the server qc)
		// otherwise we execute them on client frames
		if (sv.active ? sv_timer > 0 : cl_timer > 0)
		{
			// process console commands
//			R_TimeReport("preconsole");
			CL_VM_PreventInformationLeaks();
			Cbuf_Frame();
//			R_TimeReport("console");
		}

		//Con_Printf("%6.0f %6.0f\n", cl_timer * 1000000.0, sv_timer * 1000000.0);

		// if the accumulators haven't become positive yet, wait a while
		if (cls.state == ca_dedicated)
			wait = sv_timer * -1000000.0;
		else if (!sv.active || svs.threaded)
			wait = cl_timer * -1000000.0;
		else
			wait = max(cl_timer, sv_timer) * -1000000.0;

		if (!cls.timedemo && wait >= 1)
		{
			double time0, delta;

			if(host_maxwait.value <= 0)
				wait = min(wait, 1000000.0);
			else
				wait = min(wait, host_maxwait.value * 1000.0);
			if(wait < 1)
				wait = 1; // because we cast to int

			time0 = Sys_DirtyTime();
			if (sv_checkforpacketsduringsleep.integer && !sys_usenoclockbutbenchmark.integer && !svs.threaded) {
				NetConn_SleepMicroseconds((int)wait);
				if (cls.state != ca_dedicated)
					NetConn_ClientFrame(); // helps server browser get good ping values
				// TODO can we do the same for ServerFrame? Probably not.
			}
			else
				Sys_Sleep((int)wait);
			delta = Sys_DirtyTime() - time0;
			if (delta < 0 || delta >= 1800) delta = 0;
			if (!svs.threaded)
				svs.perf_acc_sleeptime += delta;
//			R_TimeReport("sleep");
			continue;
		}

		// limit the frametime steps to no more than 100ms each
		if (cl_timer > 0.1)
			cl_timer = 0.1;
		if (sv_timer > 0.1)
		{
			if (!svs.threaded)
				svs.perf_acc_lost += (sv_timer - 0.1);
			sv_timer = 0.1;
		}

		R_TimeReport("---");

	//-------------------
	//
	// server operations
	//
	//-------------------

		// limit the frametime steps to no more than 100ms each
		if (sv.active && sv_timer > 0 && !svs.threaded)
		{
			// execute one or more server frames, with an upper limit on how much
			// execution time to spend on server frames to avoid freezing the game if
			// the server is overloaded, this execution time limit means the game will
			// slow down if the server is taking too long.
			int framecount, framelimit = 1;
			double advancetime, aborttime = 0;
			float offset;
			prvm_prog_t *prog = SVVM_prog;

			// run the world state
			// don't allow simulation to run too fast or too slow or logic glitches can occur

			// stop running server frames if the wall time reaches this value
			if (sys_ticrate.value <= 0)
				advancetime = sv_timer;
			else if (cl.islocalgame && !sv_fixedframeratesingleplayer.integer)
			{
				// synchronize to the client frametime, but no less than 10ms and no more than 100ms
				advancetime = bound(0.01, cl_timer, 0.1);
			}
			else
			{
				advancetime = sys_ticrate.value;
				// listen servers can run multiple server frames per client frame
				framelimit = cl_maxphysicsframesperserverframe.integer;
				aborttime = Sys_DirtyTime() + 0.1;
			}
			if(slowmo.value > 0 && slowmo.value < 1)
				advancetime = min(advancetime, 0.1 / slowmo.value);
			else
				advancetime = min(advancetime, 0.1);

			if(advancetime > 0)
			{
				offset = Sys_DirtyTime() - dirtytime;if (offset < 0 || offset >= 1800) offset = 0;
				offset += sv_timer;
				++svs.perf_acc_offset_samples;
				svs.perf_acc_offset += offset;
				svs.perf_acc_offset_squared += offset * offset;
				if(svs.perf_acc_offset_max < offset)
					svs.perf_acc_offset_max = offset;
			}

			// only advance time if not paused
			// the game also pauses in singleplayer when menu or console is used
			sv.frametime = advancetime * slowmo.value;
			if (host_framerate.value)
				sv.frametime = host_framerate.value;
			if (sv.paused || (cl.islocalgame && (key_dest != key_game || key_consoleactive || cl.csqc_paused)))
				sv.frametime = 0;

			for (framecount = 0;framecount < framelimit && sv_timer > 0;framecount++)
			{
				sv_timer -= advancetime;

				// move things around and think unless paused
				if (sv.frametime)
					SV_Physics();

				// if this server frame took too long, break out of the loop
				if (framelimit > 1 && Sys_DirtyTime() >= aborttime)
					break;
			}
			R_TimeReport("serverphysics");

			// send all messages to the clients
			SV_SendClientMessages();

			if (sv.paused == 1 && realtime > sv.pausedstart && sv.pausedstart > 0) {
				prog->globals.fp[OFS_PARM0] = realtime - sv.pausedstart;
				PRVM_serverglobalfloat(time) = sv.time;
				prog->ExecuteProgram(prog, PRVM_serverfunction(SV_PausedTic), "QC function SV_PausedTic is missing");
			}

			// send an heartbeat if enough time has passed since the last one
			NetConn_Heartbeat(0);
			R_TimeReport("servernetwork");
		}
		else if (!svs.threaded)
		{
			// don't let r_speeds display jump around
			R_TimeReport("serverphysics");
			R_TimeReport("servernetwork");
		}

	//-------------------
	//
	// client operations
	//
	//-------------------

		if (cls.state != ca_dedicated && (cl_timer > 0 || cls.timedemo || ((vid_activewindow ? cl_maxfps : cl_maxidlefps).value < 1)))
		{
			R_TimeReport("---");
			Collision_Cache_NewFrame();
			R_TimeReport("photoncache");
			// decide the simulation time
#ifdef CONFIG_VIDEO_CAPTURE
			// decide the simulation time
			if (cls.capturevideo.active)
			{
				//***
				if (cls.capturevideo.realtime)
					clframetime = cl.realframetime = max(cl_timer, 1.0 / cls.capturevideo.framerate);
				else
				{
					clframetime = 1.0 / cls.capturevideo.framerate;
					cl.realframetime = max(cl_timer, clframetime);
				}
			}
			else if (vid_activewindow && cl_maxfps.value >= 1 && !cls.timedemo)
#else
			if (vid_activewindow && cl_maxfps.value >= 1 && !cls.timedemo)
#endif // CONFIG_VIDEO_CAPTURE

			{
				qbool is_active_hosting_server = sv.active && cls.state != ca_disconnected && svs.maxclients > 1;
				if ( (
//#ifdef _WIN32
//					!vid.factive ||
					!vid_activewindow ||
//#endif
						key_consoleactive || key_dest == key_menu) && is_active_hosting_server == false)
					clframetime = cl.realframetime = max(cl_timer, 1.0 / cl_maxconsole_menu_fps.value); // Baker 1017.3
				else
					clframetime = cl.realframetime = max(cl_timer, 1.0 / cl_maxfps.value);
				// when running slow, we need to sleep to keep input responsive
				wait = bound(0, cl_maxfps_alwayssleep.value * 1000, 100000);
				if (wait > 0)
					Sys_Sleep((int)wait);
			}
			else if (!vid_activewindow && cl_maxidlefps.value >= 1 && !cls.timedemo)
				clframetime = cl.realframetime = max(cl_timer, 1.0 / cl_maxidlefps.value);
			else
				clframetime = cl.realframetime = cl_timer;

			// apply slowmo scaling
			clframetime *= cl.movevars_timescale;
			// scale playback speed of demos by slowmo cvar
			if (cls.demoplayback)
			{
				clframetime *= slowmo.value;
				// if demo playback is paused, don't advance time at all
				if (cls.demopaused)
					clframetime = 0;
			}
			else
			{
				// host_framerate overrides all else
				if (host_framerate.value)
					clframetime = host_framerate.value;

				if (cl.paused || (cl.islocalgame && (key_dest != key_game || key_consoleactive || cl.csqc_paused)))
					clframetime = 0;
			}

			if (cls.timedemo)
				clframetime = cl.realframetime = cl_timer;

			// deduct the frame time from the accumulator
			cl_timer -= cl.realframetime;

			cl.oldtime = cl.time;
			cl.time += clframetime;

			// update video
			if (host_speeds.integer)
				time1 = Sys_DirtyTime();
			R_TimeReport("pre-input");

			// Collect input into cmd
			CL_Input();

			R_TimeReport("input");

			// check for new packets
			NetConn_ClientFrame();

			// read a new frame from a demo if needed
			CL_ReadDemoMessage();
			R_TimeReport("clientnetwork");

			// now that packets have been read, send input to server
			CL_SendMove();
			R_TimeReport("sendmove");

			// update client world (interpolate entities, create trails, etc)
			CL_UpdateWorld();
			R_TimeReport("lerpworld");

			CL_Video_Frame();

			R_TimeReport("client");

			CL_UpdateScreen();
			R_TimeReport("render");

			if (host_speeds.integer)
				time2 = Sys_DirtyTime();

			// update audio
			if(cl.csqc_usecsqclistener)
			{
				S_Update(&cl.csqc_listenermatrix);
				cl.csqc_usecsqclistener = false;
			}
			else
				S_Update(&r_refdef.view.matrix);

			CDAudio_Update();
			R_TimeReport("audio");

			// reset gathering of mouse input
			in_mouse_x = in_mouse_y = 0;

			if (host_speeds.integer)
			{
				pass1 = (int)((time1 - time3)*1000000);
				time3 = Sys_DirtyTime();
				pass2 = (int)((time2 - time1)*1000000);
				pass3 = (int)((time3 - time2)*1000000);
				Con_Printf("%6ius total %6ius server %6ius gfx %6ius snd\n",
							pass1+pass2+pass3, pass1, pass2, pass3);
			}
		}

#if MEMPARANOIA
		Mem_CheckSentinelsGlobal();
#else
		if (developer_memorydebug.integer)
			Mem_CheckSentinelsGlobal();
#endif

		// if there is some time remaining from this frame, reset the timers
		if (cl_timer >= 0)
			cl_timer = 0;
		if (sv_timer >= 0)
		{
			if (!svs.threaded)
				svs.perf_acc_lost += sv_timer;
			sv_timer = 0;
		}

		host_framecount++;
	}
}

//============================================================================

qbool vid_opened = false;
void Host_StartVideo(void)
{
	if (!vid_opened && cls.state != ca_dedicated)
	{
		vid_opened = true;
		// make sure we open sockets before opening video because the Windows Firewall "unblock?" dialog can screw up the graphics context on some graphics drivers
		NetConn_UpdateSockets();
		VID_Start();
		CDAudio_Startup();
	}
}

char engineversion[128];
char engineversionshort[128];

qbool sys_nostdout = false;

extern qbool host_stuffcmdsrun;

static qfile_t *locksession_fh = NULL;
static qbool locksession_run = false;
static void Host_InitSession(void)
{
	int i;
	Cvar_RegisterVariable(&sessionid);
	Cvar_RegisterVariable(&locksession);

	// load the session ID into the read-only cvar
	if ((i = Sys_CheckParm("-sessionid")) && (i + 1 < com_argc))
	{
		char vabuf[1024];
		if(com_argv[i+1][0] == '.')
			Cvar_SetQuick(&sessionid, com_argv[i+1]);
		else
			Cvar_SetQuick(&sessionid, va(vabuf, sizeof(vabuf), ".%s", com_argv[i+1]));
	}
}
void Host_LockSession(void)
{
	if(locksession_run)
		return;
	locksession_run = true;
	if(locksession.integer != 0 && !Sys_CheckParm("-readonly"))
	{
		char vabuf[1024];
		char *p = va(vabuf, sizeof(vabuf), "%slock%s", *fs_userdir ? fs_userdir : fs_basedir, sessionid.string);
		FS_CreatePath(p);
		locksession_fh = FS_SysOpen(p, "wl", false);
		// TODO maybe write the pid into the lockfile, while we are at it? may help server management tools
		if(!locksession_fh)
		{
			if(locksession.integer == 2)
			{
				Con_Printf("WARNING: session lock %s could not be acquired. Please run with -sessionid and an unique session name. Continuing anyway.\n", p);
			}
			else
			{
				Sys_Error("session lock %s could not be acquired. Please run with -sessionid and an unique session name.\n", p);
			}
		}
	}
}
void Host_UnlockSession(void)
{
	if(!locksession_run)
		return;
	locksession_run = false;

	if(locksession_fh)
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

	enlocal_pure_sa = (char *)FS_LoadFile (sfile, tempmempool, /*quiet*/ true, /*filesize ptr*/ NULL, NOLOADINFO_IN_NULL, NOLOADINFO_OUT_NULL);

	if (!enlocal_pure_sa) {
		Con_PrintLinef  ("Language initialization not found");
		return;
	}

	Con_PrintLinef  ("Language initialization: %s", sfile);

	// Remove carriage returns if found
	enlocal_sa = String_Replace_Alloc (enlocal_pure_sa, "\r", "");

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

		if (String_Does_Start_With (sthis_line, "//"))
			continue; // comment

		c_strlcpy (scopy_line, sthis_line);

		int rfake_argc = 0;
		char *rfake_argv[MAX_NUM_Q_ARGVS_50] = {0};

		// tokenize console
		String_Command_String_To_Argv (/*destructive edit*/ scopy_line, &rfake_argc, rfake_argv, MAX_NUM_Q_ARGVS_50);

		Con_DPrintLinef ("args %d", rfake_argc);

		if (rfake_argc == 3 && String_Does_Match_Caseless (rfake_argv[1], "=" ) ) {
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
	freenull3_ (enlocal_sa)
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
		if (String_Does_Match (sthis, s_token)) {
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
		if (String_Does_Start_With (s, sthis)) {
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

		char *s_tokenx = rerelease_loc.strings[j + 0];
		char *s_decodx = rerelease_loc.strings[j + 1];

		//Sys_PrintToTerminal2 (va3(QUOTED_S, s_tokenx));
		//Sys_PrintToTerminal2 (va3(QUOTED_S, s_decodx));

		int slen0 = (int)strlen(rerelease_loc.strings[j + 0]);
		int slen1 = (int)strlen(rerelease_loc.strings[j + 1]);

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

	if (cycles > 1 && String_Does_End_With(s_decodebuf, NEWLINE) == false) {
		c_strlcat	(s_decodebuf, NEWLINE);
	}

	//Sys_PrintToTerminal2 (va3( QUOTED_S " to " QUOTED_S,  stxt, s_decodebuf ) );

	return s_decodebuf;
}



/*
====================
Host_Init
====================
*/
static void Host_Init (void)
{
	int i;
	const char* os;
	char vabuf[1024];

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
	if (Sys_CheckParm("-developer"))
	{
		developer.value = developer.integer = 1;
		developer.string = "1";
	}

	if (Sys_CheckParm("-developer2") || Sys_CheckParm("-developer3"))
	{
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

	if (Sys_CheckParm("-developer3"))
	{
		gl_paranoid.integer = 1;gl_paranoid.string = "1";
		gl_printcheckerror.integer = 1;gl_printcheckerror.string = "1";
	}

// COMMANDLINEOPTION: Console: -nostdout disables text output to the terminal the game was launched from
	if (Sys_CheckParm("-nostdout"))
		sys_nostdout = 1;

	// used by everything
	Memory_Init();

	// initialize console command/cvar/alias/command execution systems
	Cmd_Init();

	// initialize memory subsystem cvars/commands
	Memory_Init_Commands();

	// initialize console and logging and its cvars/commands
	Con_Init();

	// initialize various cvars that could not be initialized earlier
	u8_Init();
	Curl_Init_Commands();
	Cmd_Init_Commands();
	Sys_Init_Commands();
	COM_Init_Commands();
	FS_Init_Commands();

	// initialize console window (only used by sys_win.c)
	Sys_InitConsole();

	// initialize the self-pack (must be before COM_InitGameType as it may add command line options)
	FS_Init_SelfPack();

	// detect gamemode from commandline options or executable name
	COM_InitGameType();

	// AURA TARGET LOC_Init

	// construct a version string for the corner of the console
	os = DP_OS_NAME;
	//dpsnprintf (engineversion, sizeof (engineversion), "%s %s %s", gamename, os, buildstring);
	dpsnprintf (engineversion, sizeof (engineversion), "%s %s %s", gamename, os, buildstring);
	//dpsnprintf (engineversionshort, sizeof (engineversionshort), "Zircon - %s", buildstringshort);

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

	Con_PrintLinef ("%s", engineversion);

	// initialize process nice level
	Sys_InitProcessNice();

	// initialize ixtable
	Mathlib_Init();

	// initialize filesystem (including fs_basedir, fs_gamedir, -game, scr_screenshot_name)
	FS_Init();

#ifdef __ANDROID__
	void FS_Path_f (void);
	FS_Path_f ();
#endif // __ANDROID__

	// register the cvars for session locking
	Host_InitSession();

	// must be after FS_Init
	Crypto_Init();
	Crypto_Init_Commands();

	NetConn_Init();
	Curl_Init();
	PRVM_Init();
	Mod_Init();
	World_Init();
	SV_Init();
	V_Init(); // some cvars needed by server player physics (cl_rollangle etc)
	Host_InitCommands();
	Host_InitLocal();
	Host_ServerOptions();

	Thread_Init();

	if (cls.state == ca_dedicated)
		Cmd_AddCommand ("disconnect", CL_Disconnect_f, "disconnect from server (or disconnect all clients if running a server)");
	else
	{
		Con_DPrintf("Initializing client\n");

		R_Modules_Init();
		Palette_Init();
#ifdef CONFIG_MENU
		MR_Init_Commands();
#endif
		VID_Shared_Init();
		VID_Init();
		Render_Init();
		S_Init();

		CDAudio_Init();

		Key_Init();
		CL_Init();
	}

	// save off current state of aliases, commands and cvars for later restore if FS_GameDir_f is called
	// NOTE: menu commands are freed by Cmd_RestoreInitState
	Cmd_SaveInitState();

	// FIXME: put this into some neat design, but the menu should be allowed to crash
	// without crashing the whole game, so this should just be a short-time solution

	// here comes the not so critical stuff
	if (setjmp(host_abortframe)) {
		return;
	}

	Host_AddConfigText();
	Cbuf_Execute();

	// if stuffcmds wasn't run, then quake.rc is probably missing, use default
	if (!host_stuffcmdsrun)
	{
		Cbuf_AddTextLine ("exec default.cfg" "exec " CONFIGFILENAME NEWLINE "exec autoexec.cfg" NEWLINE "stuffcmds");
		Cbuf_Execute();
	}

	// put up the loading image so the user doesn't stare at a black screen...
	SCR_BeginLoadingPlaque(true); // Vid here (Sep262023)
#ifdef CONFIG_MENU
	if (cls.state != ca_dedicated)
	{
		MR_Init();
	}
#endif

	// check for special benchmark mode
// COMMANDLINEOPTION: Client: -benchmark <demoname> runs a timedemo and quits, results of any timedemo can be found in gamedir/benchmark.log (for example id1/benchmark.log)
	i = Sys_CheckParm("-benchmark");
	if (i && i + 1 < com_argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "timedemo %s", com_argv[i + 1]));
		Cbuf_Execute();
	}

	// check for special demo mode
// COMMANDLINEOPTION: Client: -demo <demoname> runs a playdemo and quits
	i = Sys_CheckParm("-demo");
	if (i && i + 1 < com_argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "playdemo %s", com_argv[i + 1]));
		Cbuf_Execute();
	}

#ifdef CONFIG_VIDEO_CAPTURE
// COMMANDLINEOPTION: Client: -capturedemo <demoname> captures a playdemo and quits
	i = Sys_CheckParm("-capturedemo");
	if (i && i + 1 < com_argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "playdemo %s" NEWLINE "cl_capturevideo 1", com_argv[i + 1]));
		Cbuf_Execute();
	}
#endif // CONFIG_VIDEO_CAPTURE

	if (cls.state == ca_dedicated || Sys_CheckParm("-listen"))
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddTextLine ("startmap_dm");
		Cbuf_Execute();
	}

	if (!sv.active && !cls.demoplayback && !cls.connect_trying) {
#ifdef CONFIG_MENU
		extern cvar_t nostartdemos;
		if (!nostartdemos.value)
			Cbuf_AddTextLine ("togglemenu 1");
#endif
		Cbuf_Execute();
	}

	Con_DPrint("========Initialized=========\n");

	//Host_StartVideo();

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
		Con_Print("recursive shutdown\n");
		return;
	}
	if (setjmp(host_abortframe))
	{
		Con_Print("aborted the quitting frame?!?\n");
		return;
	}
	isdown = true;

	// be quiet while shutting down
	S_StopAllSounds();

	// end the server thread
	if (svs.threaded)
		SV_StopThread();

	// disconnect client from server if active
	CL_Disconnect();

	// shut down local server if active
	SV_LockThreadMutex();
	Host_ShutdownServer ();
	SV_UnlockThreadMutex();

#ifdef CONFIG_MENU
	// Shutdown menu
	if(MR_Shutdown)
		MR_Shutdown();
#endif

	// AK shutdown PRVM
	// AK hmm, no PRVM_Shutdown(); yet

	CL_Video_Shutdown();

	Host_SaveConfig();

	CDAudio_Shutdown ();

	S_Terminate ();
	Curl_Shutdown ();
	NetConn_Shutdown ();
	//PR_Shutdown ();

	if (cls.state != ca_dedicated)
	{
		R_Modules_Shutdown();
		VID_Shutdown();
	}

	SV_StopThread();
	Thread_Shutdown();
	Cmd_Shutdown();
	Key_Shutdown();
	CL_Shutdown();
	Sys_Shutdown();
	Log_Close();
	Crypto_Shutdown();

	Host_UnlockSession();

	S_Shutdown();
	Con_Shutdown();
	Memory_Shutdown();
}

