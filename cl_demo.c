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

#ifdef CONFIG_VIDEO_CAPTURE
extern cvar_t cl_capturevideo;
extern cvar_t cl_capturevideo_demo_stop;
#endif
int old_vsync = 0;

cvar_t cl_startdemos = {CF_CLIENT | CF_ARCHIVE , "cl_startdemos", "1", "Play start demos on startup [Zircon change]"}; // Baker r3172: nostartdemos

cvar_t cl_autodemo = {CF_CLIENT | CF_ARCHIVE, "cl_autodemo", "0", "records every game played, using the date/time and map name to name the demo file" };
cvar_t cl_autodemo_nameformat = {CF_CLIENT | CF_ARCHIVE, "cl_autodemo_nameformat", "autodemos/%Y-%m-%d_%H-%M", "The format of the cl_autodemo filename, followed by the map name (the date is encoded using strftime escapes)" };
cvar_t cl_autodemo_delete = {CF_CLIENT, "cl_autodemo_delete", "0", "Delete demos after recording.  This is a bitmask, bit 1 gives the default, bit 0 the value for the current demo.  Thus, the values are: 0 = disabled; 1 = delete current demo only; 2 = delete all demos except the current demo; 3 = delete all demos from now on" };


static void CL_FinishTimeDemo (void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all outgoing network messages are skipped, and
incoming messages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[MAX_INPUTLINE_16384];

	if (cls.demonum == -1)
		return;		// don't play demos

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS_8)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Print("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	dpsnprintf (str, sizeof(str), "playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText(cmd_local, str);
	cls.demonum++;
}

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
// LadyHavoc: now called only by CL_Disconnect
void CL_StopPlayback (void)
{
#ifdef CONFIG_VIDEO_CAPTURE
	if (cl_capturevideo_demo_stop.integer)
		Cvar_Set(&cvars_all, "cl_capturevideo", "0");
#endif

	if (!cls.demoplayback)
		return;

	FS_Close (cls.demofile);
	cls.demoplayback = false;
	cls.demofile = NULL;

	if (cls.timedemo)
		CL_FinishTimeDemo ();

	if (!cls.demostarting) // only quit if not starting another demo
		if (Sys_CheckParm("-demo") || Sys_CheckParm("-capturedemo"))
			host.state = host_shutdown;
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
#====================
*/
void CL_WriteDemoMessage (sizebuf_t *message)
{
	int		len;
	int		i;
	float	f;

	if (cls.demopaused) // LadyHavoc: pausedemo
		return;

	len = LittleLong (message->cursize);
	FS_Write (cls.demofile, &len, 4);
	for (i=0 ; i<3 ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		FS_Write (cls.demofile, &f, 4);
	}
	FS_Write (cls.demofile, message->data, message->cursize);
}

/*
====================
CL_CutDemo

Dumps the current demo to a buffer, and resets the demo to its starting point.
Used to insert csprogs.dat files as a download to the beginning of a demo file.
====================
*/
void CL_CutDemo (unsigned char **buf, fs_offset_t *filesize)
{
	*buf = NULL;
	*filesize = 0;

	FS_Close(cls.demofile);
	*buf = FS_LoadFile(cls.demoname, tempmempool, fs_quiet_FALSE, filesize);

	// restart the demo recording
	cls.demofile = FS_OpenRealFile(cls.demoname, "wb", fs_quiet_FALSE); // WRITE-EON  CL_CutDemo
	if (!cls.demofile)
		Sys_Error ("failed to reopen the demo file");
	FS_Printf(cls.demofile, "%d\n", cls.forcetrack);
}

/*
====================
CL_PasteDemo

Adds the cut stuff back to the demo. Also frees the buffer.
Used to insert csprogs.dat files as a download to the beginning of a demo file.
====================
*/
void CL_PasteDemo (unsigned char **buf, fs_offset_t *filesize)
{
	fs_offset_t startoffset = 0;

	if (!*buf)
		return;

	// skip cdtrack
	while(startoffset < *filesize && ((char *)(*buf))[startoffset] != '\n')
		++startoffset;
	if (startoffset < *filesize)
		++startoffset;

	FS_Write(cls.demofile, *buf + startoffset, *filesize - startoffset);

	Mem_Free(*buf);
	*buf = NULL;
	*filesize = 0;
}

/*
====================
CL_ReadDemoMessage

Handles playback of demos
====================
*/
void CL_ReadDemoMessage(void)
{
	int i;
	float f;

	if (!cls.demoplayback)
		return;

	// LadyHavoc: pausedemo
	if (cls.demopaused)
		return;

	for (;;)
	{
		// decide if it is time to grab the next message
		// always grab until fully connected
		if (cls.signon == SIGNONS_4) { // DEMO READ
			if (cls.timedemo) {
				cls.td_frames++;
				cls.td_onesecondframes++;
				// if this is the first official frame we can now grab the real
				// td_starttime so the bogus time on the first frame doesn't
				// count against the final report
				if (cls.td_frames == 0)
				{
					cls.td_starttime = host.realtime;
					cls.td_onesecondnexttime = cl.time + 1;
					cls.td_onesecondrealtime = host.realtime;
					cls.td_onesecondframes = 0;
					cls.td_onesecondminfps = 0;
					cls.td_onesecondmaxfps = 0;
					cls.td_onesecondavgfps = 0;
					cls.td_onesecondavgcount = 0;
				}
				if (cl.time >= cls.td_onesecondnexttime)
				{
					double fps = cls.td_onesecondframes / (host.realtime - cls.td_onesecondrealtime);
					if (cls.td_onesecondavgcount == 0)
					{
						cls.td_onesecondminfps = fps;
						cls.td_onesecondmaxfps = fps;
					}
					cls.td_onesecondrealtime = host.realtime;
					cls.td_onesecondminfps = min(cls.td_onesecondminfps, fps);
					cls.td_onesecondmaxfps = max(cls.td_onesecondmaxfps, fps);
					cls.td_onesecondavgfps += fps;
					cls.td_onesecondavgcount++;
					cls.td_onesecondframes = 0;
					cls.td_onesecondnexttime++;
				}
			}
			else if (cl.time < cl.mtime[0])
			{
				// don't need another message yet
				return;
			}
		}

		// get the next message
		FS_Read(cls.demofile, &cl_message.cursize, 4);
		cl_message.cursize = LittleLong(cl_message.cursize);
		if (cl_message.cursize & DEMOMSG_CLIENT_TO_SERVER) // This is a client->server message! Ignore for now!
		{
			// skip over demo packet
			FS_Seek(cls.demofile, 12 + (cl_message.cursize & (~DEMOMSG_CLIENT_TO_SERVER)), SEEK_CUR);
			continue;
		}
		if (cl_message.cursize > cl_message.maxsize)
		{
			CL_DisconnectEx(false, "Demo message (%d) > cl_message.maxsize (%d)", cl_message.cursize, cl_message.maxsize);
			cl_message.cursize = 0;
			return;
		}
		VectorCopy(cl.mviewangles[0], cl.mviewangles[1]); // demo
		for (i = 0;i < 3;i++) {
			FS_Read(cls.demofile, &f, 4);
			cl.mviewangles[0][i] = LittleFloat(f);
		}

		if (FS_Read(cls.demofile, cl_message.data, cl_message.cursize) == cl_message.cursize)
		{
			MSG_BeginReading(&cl_message);
			CL_ParseServerMessage();

			if (cls.signon != SIGNONS_4) // DEMO
				Cbuf_Execute((cmd_local)->cbuf); // immediately execute svc_stufftext if in the demo before connect!

			// In case the demo contains a "svc_disconnect" message
			if (!cls.demoplayback)
				return;

			if (cls.timedemo)
				return;
		}
		else
		{
			CL_Disconnect();
			return;
		}
	}
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f(cmd_state_t *cmd)
{
	sizebuf_t buf;
	unsigned char bufdata[64];

	if (!cls.demorecording) {
		Con_PrintLinef ("Not recording a demo.");
		return;
	}

// write a disconnect message to the demo file
	// LadyHavoc: don't replace the cl_message when doing this
	buf.data = bufdata;
	buf.maxsize = sizeof(bufdata);
	SZ_Clear(&buf);
	MSG_WriteByte(&buf, svc_disconnect);
	CL_WriteDemoMessage(&buf);

// finish up
	if (cl_autodemo.integer && (cl_autodemo_delete.integer & 1)) {
		FS_RemoveOnClose(cls.demofile);
		Con_PrintLinef ("Completed and deleted demo");
	}
	else
		Con_PrintLinef ("Completed demo");
	FS_Close (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
int in_record_hack; // Baker r0070: "record" at any time - saves a .sav game or reconnects to a server.
void CL_Record_f(cmd_state_t *cmd)
{
	int c, track;
	char name[MAX_OSPATH];
	char vabuf[1024];

	c = Cmd_Argc(cmd);
	if (c != 2 && c != 3 && c != 4) {
		Con_PrintLinef ("record <demoname> [<map> [cd track]]");
		return;
	}

	if (strstr(Cmd_Argv(cmd, 1), "..")) {
		Con_PrintLinef ("Relative pathnames are not allowed.");
		return;
	}

	WARP_X_ (SV_CanSave)
#if 222 // Baker r0070: These were missing from DarkPlaces Beta
	if (cls.demorecording) {
		Con_PrintLinef ("Already recording a demo.");
		return;
	}

	if (sv.active && cl.islocalgame && cl.intermission) {
		Con_PrintLinef ("Can't record during intermission");
		return;
	}
#endif
	if (cls.state == ca_connected && cls.protocol == PROTOCOL_QUAKEWORLD) {
		Con_PrintLinef ("Cannot record Quakeworld demos");
		return;
	}

	if (c == 2 && cls.state == ca_connected) {
#if 222 // Baker r0070 - create a temp.sav
		if (sv.active && cl.islocalgame && !cl.intermission && in_record_hack == 0) {
			prvm_prog_t *prog = SVVM_prog;
			int deadflag = cl.islocalgame && svs.clients[0].active && PRVM_serveredictfloat(svs.clients[0].edict, deadflag);
			if (deadflag) {
				Con_PrintLinef ("Can't record while dead");
				return;
			}
			in_record_hack = 1;
			Con_DPrintLinef ("Initiating record hack ...");
			Cbuf_AddTextLine (cmd, va (vabuf, sizeof(vabuf), "save temp; disconnect; record %s; load temp", Cmd_Argv(cmd, 1)) );
			return;
		}
		if (!sv.active && cls.state == ca_connected && in_record_hack == 0) {
			in_record_hack = 2;
			Con_DPrintLinef ("Initiating record hack ...");
			Cbuf_AddTextLine (cmd, va (vabuf, sizeof(vabuf), "disconnect; record %s; reconnect", Cmd_Argv(cmd, 1)) );
			return;
		}

#endif
		Con_PrintLinef ("Can not record - already connected to server" NEWLINE "Client demo recording must be started before connecting");
		return;
	}

	if (in_record_hack)
		in_record_hack = 0;

	if (cls.state == ca_connected)
		CL_Disconnect();

	// write the forced cd track number, or -1
	if (c == 4) {
		track = atoi(Cmd_Argv(cmd, 3));
		Con_PrintLinef ("Forcing CD track to %d", cls.forcetrack);
	}
	else
		track = -1;

	// get the demo name
	strlcpy (name, Cmd_Argv(cmd, 1), sizeof (name));
	FS_DefaultExtension (name, ".dem", sizeof (name));

	// start the map up
	if (c > 2)
		Cmd_ExecuteString ( cmd, va(vabuf, sizeof(vabuf), "map %s", Cmd_Argv(cmd, 2)), src_local, false);

	// open the demo file
	Con_PrintLinef ("recording to %s.", name);
	cls.demofile = FS_OpenRealFile(name, "wb", fs_quiet_FALSE); // WRITE-EON record demo
	if (!cls.demofile) {
		Con_PrintLinef (CON_ERROR "ERROR: couldn't open.");
		return;
	}
	strlcpy(cls.demoname, name, sizeof(cls.demoname));

	cls.forcetrack = track;
	FS_Printf(cls.demofile, "%d" NEWLINE, cls.forcetrack);

	cls.demorecording = true;
	cls.demo_lastcsprogssize = -1;
	cls.demo_lastcsprogscrc = -1;
}

void CL_PlayDemo(const char *demo)
{
	char name[MAX_QPATH_128];
	int c;
	qbool neg = false;
	qfile_t *f;

	// open the demo file
	strlcpy (name, demo, sizeof (name));
	FS_DefaultExtension (name, ".dem", sizeof (name));
	f = FS_OpenVirtualFile(name, fs_quiet_FALSE);
	if (!f)
	{
		Con_PrintLinef (CON_ERROR "ERROR: couldn't open %s.", name);
		cls.demonum = -1;		// stop demo loop
		return;
	}

	cls.demostarting = true;

	// disconnect from server
	CL_Disconnect();

	// update networking ports (this is mainly just needed at startup)
	NetConn_UpdateSockets();

	cls.protocol = PROTOCOL_QUAKE; cls.protocol_flags_rmq = 0;

	Con_PrintLinef ("Playing demo %s.", name);
	cls.demofile = f;
	c_strlcpy (cls.demoname, name);

	cls.demoplayback = true;
	cls.state = ca_connected;
	cls.forcetrack = 0;

	while ((c = FS_Getc (cls.demofile)) != '\n')
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');

	if (neg)
		cls.forcetrack = -cls.forcetrack;

	cls.demostarting = false;
}

/*
====================
CL_PlayDemo_f

playdemo [demoname]
====================
*/
void CL_PlayDemo_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("playdemo <demoname> : plays a demo");
		return;
	}

	cl_signon_start_time = Sys_DirtyTime (); // playdemo
	CL_PlayDemo (Cmd_Argv(cmd, 1));
}

typedef struct
{
	int frames;
	double time, totalfpsavg;
	double fpsmin, fpsavg, fpsmax;
}
benchmarkhistory_t;
static size_t doublecmp_offset;
static int doublecmp_withoffset(const void *a_, const void *b_)
{
	const double *a = (const double *) ((const char *) a_ + doublecmp_offset);
	const double *b = (const double *) ((const char *) b_ + doublecmp_offset);
	if (*a > *b)
		return +1;
	if (*a < *b)
		return -1;
	return 0;
}

/*
====================
CL_FinishTimeDemo

====================
*/
static void CL_FinishTimeDemo (void)
{
	int frames;
	int i;
	double time, totalfpsavg;
	double fpsmin, fpsavg, fpsmax; // report min/avg/max fps
	static int benchmark_runs = 0;
	char vabuf[1024];

	cls.timedemo = host.restless = false;

	frames = cls.td_frames;
	time = host.realtime - cls.td_starttime;
	totalfpsavg = time > 0 ? frames / time : 0;
	fpsmin = cls.td_onesecondminfps;
	fpsavg = cls.td_onesecondavgcount ? cls.td_onesecondavgfps / cls.td_onesecondavgcount : 0;
	fpsmax = cls.td_onesecondmaxfps;
	// LadyHavoc: timedemo now prints out 7 digits of fraction, and min/avg/max
	Con_Printf ("%d frames %5.7f seconds %5.7f fps, one-second fps min/avg/max: %.0f %.0f %.0f (%d seconds)\n", frames, time, totalfpsavg, fpsmin, fpsavg, fpsmax, cls.td_onesecondavgcount);
	Log_Printf("benchmark.log", "date %s | enginedate %s | demo %s | commandline %s | run %d | result %d frames %5.7f seconds %5.7f fps, one-second fps min/avg/max: %.0f %.0f %.0f (%d seconds)\n", Sys_TimeString("%Y-%m-%d %H:%M:%S"), buildstring, cls.demoname, cmdline.string, benchmark_runs + 1, frames, time, totalfpsavg, fpsmin, fpsavg, fpsmax, cls.td_onesecondavgcount);
	if (Sys_CheckParm("-benchmark"))
	{
		++benchmark_runs;
		i = Sys_CheckParm("-benchmarkruns");
		if (i && i + 1 < sys.argc) {
			static benchmarkhistory_t *history = NULL;
			if (!history)
				history = (benchmarkhistory_t *)Z_Malloc(sizeof(*history) * atoi(sys.argv[i + 1]));

			history[benchmark_runs - 1].frames = frames;
			history[benchmark_runs - 1].time = time;
			history[benchmark_runs - 1].totalfpsavg = totalfpsavg;
			history[benchmark_runs - 1].fpsmin = fpsmin;
			history[benchmark_runs - 1].fpsavg = fpsavg;
			history[benchmark_runs - 1].fpsmax = fpsmax;

			if (atoi(sys.argv[i + 1]) > benchmark_runs)
			{
				// restart the benchmark
				Cbuf_AddTextLine (cmd_local, va(vabuf, sizeof(vabuf), "timedemo %s", cls.demoname));
				// cannot execute here
			}
			else
			{
				// print statistics
				int first = Sys_CheckParm("-benchmarkruns_skipfirst") ? 1 : 0;
				if (benchmark_runs > first)
				{
#define DO_MIN(f) \
					for(i = first; i < benchmark_runs; ++i) if ((i == first) || (history[i].f < f)) f = history[i].f

#define DO_MAX(f) \
					for(i = first; i < benchmark_runs; ++i) if ((i == first) || (history[i].f > f)) f = history[i].f

#define DO_MED(f) \
					doublecmp_offset = (char *)&history->f - (char *)history; \
					qsort(history + first, benchmark_runs - first, sizeof(*history), doublecmp_withoffset); \
					if ((first + benchmark_runs) & 1) \
						f = history[(first + benchmark_runs - 1) / 2].f; \
					else \
						f = (history[(first + benchmark_runs - 2) / 2].f + history[(first + benchmark_runs) / 2].f) / 2

					DO_MIN(frames);
					DO_MAX(time);
					DO_MIN(totalfpsavg);
					DO_MIN(fpsmin);
					DO_MIN(fpsavg);
					DO_MIN(fpsmax);
					Con_Printf ("MIN: %d frames %5.7f seconds %5.7f fps, one-second fps min/avg/max: %.0f %.0f %.0f (%d seconds)\n", frames, time, totalfpsavg, fpsmin, fpsavg, fpsmax, cls.td_onesecondavgcount);

					DO_MED(frames);
					DO_MED(time);
					DO_MED(totalfpsavg);
					DO_MED(fpsmin);
					DO_MED(fpsavg);
					DO_MED(fpsmax);
					Con_Printf ("MED: %d frames %5.7f seconds %5.7f fps, one-second fps min/avg/max: %.0f %.0f %.0f (%d seconds)\n", frames, time, totalfpsavg, fpsmin, fpsavg, fpsmax, cls.td_onesecondavgcount);

					DO_MAX(frames);
					DO_MIN(time);
					DO_MAX(totalfpsavg);
					DO_MAX(fpsmin);
					DO_MAX(fpsavg);
					DO_MAX(fpsmax);
					Con_Printf ("MAX: %d frames %5.7f seconds %5.7f fps, one-second fps min/avg/max: %.0f %.0f %.0f (%d seconds)\n", frames, time, totalfpsavg, fpsmin, fpsavg, fpsmax, cls.td_onesecondavgcount);
				}
				Z_Free(history);
				history = NULL;
				host.state = host_shutdown;
			}
		}
		else
			host.state = host_shutdown;
	}
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("timedemo <demoname> : gets demo speeds");
		return;
	}

	srand(0); // predictable random sequence for benchmarking

	CL_PlayDemo (Cmd_Argv(cmd, 1));

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	// instantly hide console and deactivate it
	KeyDest_Set (key_game); // key_dest = key_game;
	key_consoleactive = 0;
	scr_con_current = 0;

	cls.timedemo = host.restless = true;
	cls.td_frames = -2;		// skip the first frame
	cls.demonum = -1;		// stop demo loop
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
CL_Startdemos_f
==================
*/
static void CL_Startdemos_f(cmd_state_t *cmd)
{
	int		i, c;

	if (cls.state == ca_dedicated || Sys_CheckParm("-listen") || Sys_CheckParm("-benchmark") || Sys_CheckParm("-demo") || Sys_CheckParm("-capturedemo"))
		return;

	if (cl_startdemos.value == 0) {
		// Baker: What this is trying to do is close the menu and the console
		// and set key_game.  Now .. why are we doing this though?
//		KeyDest_Set (key_game); // key_dest = key_game;
//		menu_state_set_nova (m_none);
//		Con_CloseConsole_If_Client ();
		return;
	}

	c = Cmd_Argc(cmd) - 1;
	if (c > MAX_DEMOS_8) {
		Con_PrintLinef ("Max %d demos in demoloop", MAX_DEMOS_8);
		c = MAX_DEMOS_8;
	}
	Con_DPrintLinef ("%d demo(s) in loop", c);

	for (i = 1 ; i < c + 1; i ++)
		c_strlcpy (cls.demos[i-1], Cmd_Argv(cmd, i));

	// LadyHavoc: clear the remaining slots
	for ( ; i <= MAX_DEMOS_8; i ++)
		cls.demos[i-1][0] = 0;

	cl_signon_start_time = Sys_DirtyTime (); // startdemos
	if (!sv.active && cls.demonum != -1 && !cls.demoplayback) {
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
CL_Demos_f

Return to looping demos
==================
*/
static void CL_Demos_f(cmd_state_t *cmd)
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect ();
	CL_NextDemo();
}

/*
==================
CL_Stopdemo_f

Return to looping demos (Baker: NO!)
==================
*/
static void CL_Stopdemo_f(cmd_state_t *cmd)
{
	if (!cls.demoplayback)
		return;
	CL_Disconnect();
}

// LadyHavoc: pausedemo command
static void CL_PauseDemo_f(cmd_state_t *cmd)
{
	cls.demopaused = !cls.demopaused;
	if (cls.demopaused)
		Con_PrintLinef ("Demo paused");
	else
		Con_PrintLinef ("Demo unpaused");
}

void CL_Demo_Init(void)
{
	Cmd_AddCommand(CF_CLIENT, "record", CL_Record_f, "record a demo");
	Cmd_AddCommand(CF_CLIENT, "stop", CL_Stop_f, "stop recording or playing a demo");
	Cmd_AddCommand(CF_CLIENT, "playdemo", CL_PlayDemo_f, "watch a demo file");
	Cmd_AddCommand(CF_CLIENT, "timedemo", CL_TimeDemo_f, "play back a demo as fast as possible and save statistics to benchmark.log");
	Cmd_AddCommand(CF_CLIENT, "startdemos", CL_Startdemos_f, "start playing back the selected demos sequentially (used at end of startup script)");
	Cmd_AddCommand(CF_CLIENT, "demos", CL_Demos_f, "restart looping demos defined by the last startdemos command");
	Cmd_AddCommand(CF_CLIENT, "stopdemo", CL_Stopdemo_f, "stop playing or recording demo (like stop command) and return to looping demos");
	// LadyHavoc: added pausedemo
	Cmd_AddCommand(CF_CLIENT, "pausedemo", CL_PauseDemo_f, "pause demo playback (can also safely pause demo recording if using QUAKE, QUAKEDP or NEHAHRAMOVIE protocol, useful for making movies)");
	Cvar_RegisterVariable (&cl_autodemo);
	Cvar_RegisterVariable (&cl_autodemo_nameformat);
	Cvar_RegisterVariable (&cl_autodemo_delete);
	Cvar_RegisterVariable (&cl_startdemos);
	
}


