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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"
#include "cdaudio.h"
#include "sound.h"

// used by menu to ghost CD audio slider
cvar_t cdaudioinitialized = {CF_CLIENT | CF_READONLY,"cdaudioinitialized","0","indicates if CD Audio system is active"};

#define MAX_PLAYLISTS 10
int music_playlist_active = -1;
int music_playlist_playing = 0; // 0 = not playing, 1 = playing, -1 = tried and failed

cvar_t music_playlist_index = {CF_CLIENT, "music_playlist_index", "-1", "selects which of the music_playlist_ variables is the active one, -1 disables playlists"};
cvar_t music_playlist_list[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_list0", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list1", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list2", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list3", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list4", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list5", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list6", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list7", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list8", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list9", "", "list of tracks to play"}
};
cvar_t music_playlist_current[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_current0", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current1", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current2", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current3", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current4", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current5", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current6", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current7", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current8", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current9", "0", "current track index to play in list"},
};
cvar_t music_playlist_random[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_random0", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random1", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random2", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random3", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random4", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random5", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random6", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random7", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random8", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random9", "0", "enables random play order if 1, 0 is sequential play"},
};
cvar_t music_playlist_sampleposition[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_sampleposition0", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition1", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition2", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition3", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition4", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition5", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition6", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition7", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition8", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition9", "-1", "resume position for track, -1 restarts every time"},
};

static qbool wasPlaying = false;
static qbool initialized = false;
static qbool enabled = false;
static float cdvolume;
typedef char filename_t[MAX_QPATH_128];
#ifdef MAXTRACKS_256
static filename_t remap[MAXTRACKS_256];
#endif
static int faketrack = -1;

static qbool cdPlaying = false;
static qbool cdPlayLooping = false;
static unsigned char cdPlayTrack;

static void CDAudio_Eject (void)
{
}


static void CDAudio_CloseDoor (void)
{
}

static int CDAudio_GetAudioDiskInfo (void)
{
	return -1;
}

// Helper for CDAudio_Play, the "cd" command, and the music_playlist system.
// Does _not_ act as NOP when a playlist is active, simply because this is used
// _by_ playlist code. So beware when calling this.

#pragma message ("Is start position reliable seconds into track?")

// Baker: My guess is "tryreal" means use the actual cd player versus play the ogg
// trackname example is "20" or "3"
// Baker: startposition does "work", is it accurate?
// Can we get the position?

static void CDAudio_Play_byName (const char *trackname, qbool looping, qbool tryreal, float startposition)
{
	unsigned int track;
	sfx_t *sfx;
	char filename[MAX_QPATH_128];

	CL_StartVideo();

	if (!enabled)
		return;

	// Baker: strspn - Returns the length of the initial portion of str1 which consists only of 
	// characters that are part of str2
	// "gives the length of the initial span in string consisting of characters not in "chars"
	// span .. strspn is a too clever way to find the offset
	// strspn(trackname, "0123456789") probably returns 0 to 9
	if (tryreal && strspn(trackname, "0123456789") == strlen(trackname))
	{
		track = (unsigned int) atoi(trackname);
#ifdef MAXTRACKS_256
		if (track > 0 && track < MAXTRACKS_256 && *remap[track])
			trackname = remap[track];
#endif
	}

	if (tryreal && strspn(trackname, "0123456789") == strlen(trackname)) {
		track = (unsigned int) atoi(trackname);
		if (track < 1) {
			Con_DPrintLinef ("CDAudio: Bad track number %u.", track);
			return;
		}
	}
	else
		track = 0;

	// div0: I assume this code was intentionally there. Maybe turn it into a cvar?
	if (cdPlaying && cdPlayTrack == track && faketrack == -1)
		return;
	CDAudio_Stop ();

	// Try playing a fake track (sound file) first
	if (track >= 1) {
		                              dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%03u.wav", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%03u.ogg", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/track%03u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/track%03u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%02u.wav", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%02u.ogg", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/track%02u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/track%02u.ogg", track);// added by motorsep
	}
	else
	{
		                              dpsnprintf(filename, sizeof(filename), "%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/%s.ogg", trackname); // added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/%s.ogg", trackname); // added by motorsep
	}

	if (FS_FileExists(filename) && (sfx = S_PrecacheSound (filename, q_tx_complain_false, q_levelsound_false))) {
		Con_DPrintLinef ("CD track %s", filename);
		c_strlcpy (cl.cdtrackname, filename);
		faketrack = S_StartSound_StartPosition_Flags (
			ENT_NONE_NEG1, 
			ENT_CHANNEL_0, 
			sfx, 
			vec3_origin, 
			cdvolume, 
			ATTENUATION_0, 
			startposition, 
			(looping ? CHANNELFLAG_FORCELOOP : 0) | CHANNELFLAG_FULLVOLUME | CHANNELFLAG_LOCALSOUND, 
			SND_SPEED_NORMAL_1_0);
		if (faketrack != -1)
		{
			if (track >= 1)
				Con_DPrintLinef ("BGM track %u playing...", track);
			else
				Con_DPrintLinef ("BGM track %s playing...", trackname);
		}
	}

	if (faketrack == -1) {
		if (track >= 1)
			Con_DPrintLinef ("Could not load BGM track %u.", track);
		else
			Con_DPrintLinef ("Could not load BGM track %s.", trackname);
		return;
	}

	cdPlayLooping = looping;
	cdPlayTrack = track;
	cdPlaying = true;

	if (cdvolume == 0.0 || bgmvolume.value == 0)
		CDAudio_Pause ();
}

void CDAudio_Play (int track, qbool looping)
{
	char buf[20];
	if (music_playlist_index.integer >= 0)
		return;
	dpsnprintf(buf, sizeof(buf), "%d", (int) track);
	CDAudio_Play_byName(buf, looping, q_tryreal_true, STARTPOS_0);
}

float CDAudio_GetPosition (void)
{
	if (faketrack != -1)
		return S_GetChannelPosition(faketrack);
	return -1;
}

static void CDAudio_StopPlaylistTrack(void);

void CDAudio_Stop (void)
{
	if (!enabled)
		return;

	// save the playlist position
	CDAudio_StopPlaylistTrack();

	if (faketrack != -1)
	{
		S_StopChannel (faketrack, true, true);
		faketrack = -1;
	}

	wasPlaying = false;
	cdPlaying = false;
}

void CDAudio_Pause (void)
{
	if (!enabled || !cdPlaying || faketrack == -1)
		return;

	S_SetChannelFlag (faketrack, CHANNELFLAG_PAUSED, true);
	wasPlaying = cdPlaying;
	cdPlaying = false;
}


void CDAudio_Resume (void)
{
	if (!enabled || cdPlaying || !wasPlaying || faketrack == -1)
		return;

	S_SetChannelFlag (faketrack, CHANNELFLAG_PAUSED, false);
	cdPlaying = true;
}

static void CD_f(cmd_state_t *cmd)
{
	const char *command;
#ifdef MAXTRACKS_256
	int ret;
	int n;
#endif

	command = Cmd_Argv(cmd, 1);

	if (String_NOT_Match_Caseless (command, "remap"))
		CL_StartVideo();

	if (String_Match_Caseless (command, "on")) {
		enabled = true;
		return;
	}

	if (String_Match_Caseless (command, "off")) {
		CDAudio_Stop();
		enabled = false;
		return;
	}

	if (String_Match_Caseless (command, "reset")) {
		enabled = true;
		CDAudio_Stop();
#ifdef MAXTRACKS_256
		for (n = 0; n < MAXTRACKS_256; n++)
			*remap[n] = 0; // empty string, that is, unremapped
#endif
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (String_Match_Caseless (command, "rescan")) {
		CDAudio_Shutdown();
		CDAudio_Startup();
		return;
	}

	if (String_Match_Caseless (command, "remap")) {
#ifdef MAXTRACKS_256
		ret = Cmd_Argc(cmd) - 2;
		if (ret <= 0)
		{
			for (n = 1; n < MAXTRACKS_256; n++)
				if (*remap[n])
					Con_PrintLinef ("  %u -> %s", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			strlcpy(remap[n], Cmd_Argv(cmd, n+1), sizeof(*remap));
#endif
		return;
	}

	if (String_Match_Caseless (command, "close")) {
		CDAudio_CloseDoor();
		return;
	}

	if (String_Match_Caseless (command, "play")) {
		if (music_playlist_index.integer >= 0)
			return;
		int startposition = (Cmd_Argc(cmd) > 3) ? atof( Cmd_Argv(cmd, 3) ) : STARTPOS_0;
		CDAudio_Play_byName(Cmd_Argv(cmd, 2), /*Baker: why?*/ q_looping_false, q_tryreal_true, startposition);
		return;
	}

	if (String_Match_Caseless (command, "loop")) {
		if (music_playlist_index.integer >= 0)
			return;
		int startposition = (Cmd_Argc(cmd) > 3) ? atof( Cmd_Argv(cmd, 3) ) : STARTPOS_0;
		CDAudio_Play_byName(Cmd_Argv(cmd, 2), q_looping_true, q_tryreal_true, startposition);
		return;
	}

	if (String_Match_Caseless (command, "stop")) {
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Stop();
		return;
	}

	if (String_Match_Caseless (command, "pause")) {
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Pause();
		return;
	}

	if (String_Match_Caseless (command, "resume")) {
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Resume();
		return;
	}

	if (String_Match_Caseless (command, "eject")) {
		if (faketrack == -1)
			CDAudio_Stop();
		CDAudio_Eject();
		return;
	}

	if (String_Match_Caseless (command, "info") == 0) {
		CDAudio_GetAudioDiskInfo ();
		if (cdPlaying)
			Con_PrintLinef ("Currently %s track %u", cdPlayLooping ? "looping" : "playing", cdPlayTrack);
		else if (wasPlaying)
			Con_Printf ("Paused %s track %u", cdPlayLooping ? "looping" : "playing", cdPlayTrack);
		if (cdvolume >= 0)
			Con_PrintLinef ("Volume is %f", cdvolume);
		else
			Con_PrintLinef ("Can't get CD volume");
		return;
	}

	Con_PrintLinef ("CD commands:");
	Con_PrintLinef ("cd on - enables CD audio system");
	Con_PrintLinef ("cd off - stops and disables CD audio system");
	Con_PrintLinef ("cd reset - resets CD audio system (clears track remapping and re-reads disc information)");
	Con_PrintLinef ("cd rescan - rescans disks in drives (to use another disc)");
	Con_PrintLinef ("cd remap <remap1> [remap2] [remap3] [...] - chooses emulated CD tracks to play when a map asks for a particular track, this has many uses");
	Con_PrintLinef ("cd close - closes CD tray");
	Con_PrintLinef ("cd eject - stops playing music and opens CD tray to allow you to change disc");
	Con_PrintLinef ("cd play <tracknumber> <startposition> - plays selected track in remapping table");
	Con_PrintLinef ("cd loop <tracknumber> <startposition> - plays and repeats selected track in remapping table");
	Con_PrintLinef ("cd stop - stops playing current CD track");
	Con_PrintLinef ("cd pause - pauses CD playback");
	Con_PrintLinef ("cd resume - unpauses CD playback");
	Con_PrintLinef ("cd info - prints basic disc information (number of tracks, currently playing track, volume level)");
}

static void CDAudio_SetVolume (float newvol)
{
	// If the volume hasn't changed
	if (newvol == cdvolume)
		return;

	// If the CD has been muted
	if (newvol <= 0.0f)
		CDAudio_Pause ();
	else
	{
		// If the CD has been unmuted
		if (cdvolume <= 0.0f)
			CDAudio_Resume ();

		if (faketrack != -1)
			S_SetChannelVolume (faketrack, newvol);
	}

	cdvolume = newvol;
}

static void CDAudio_StopPlaylistTrack(void)
{
	if (music_playlist_active >= 0 && music_playlist_active < MAX_PLAYLISTS && music_playlist_sampleposition[music_playlist_active].value >= 0)
	{
		// save position for resume
		float position = CDAudio_GetPosition();
		Cvar_SetValueQuick(&music_playlist_sampleposition[music_playlist_active], position >= 0 ? position : 0);
	}
	music_playlist_active = -1;
	music_playlist_playing = 0; // not playing
}

static void CDAudio_StartPlaylist(qbool resume)
{
	const char *list;
	const char *t;
	int index;
	int current;
	int randomplay;
	int count;
	int listindex;
	float position;
	char trackname[MAX_QPATH_128];
	CDAudio_Stop();
	index = music_playlist_index.integer;
	if (index >= 0 && index < MAX_PLAYLISTS && bgmvolume.value > 0)
	{
		list = music_playlist_list[index].string;
		current = music_playlist_current[index].integer;
		randomplay = music_playlist_random[index].integer;
		position = music_playlist_sampleposition[index].value;
		count = 0;
		trackname[0] = 0;
		if (list && list[0])
		{
			for (t = list;;count++)
			{
				if (!COM_ParseToken_Console(&t))
					break;
				// if we don't find the desired track, use the first one
				if (count == 0)
					strlcpy(trackname, com_token, sizeof(trackname));
			}
		}
		if (count > 0)
		{
			// position < 0 means never resume track
			if (position < 0)
				position = 0;
			// advance to next track in playlist if the last one ended
			if (!resume)
			{
				position = 0;
				current++;
				if (randomplay)
					current = (int)lhrandom(0, count);
			}
			// wrap playlist position if needed
			if (current >= count)
				current = 0;
			// set current
			Cvar_SetValueQuick(&music_playlist_current[index], current);
			// get the Nth trackname
			if (current >= 0 && current < count)
			{
				for (listindex = 0, t = list;;listindex++)
				{
					if (!COM_ParseToken_Console(&t))
						break;
					if (listindex == current)
					{
						strlcpy(trackname, com_token, sizeof(trackname));
						break;
					}
				}
			}
			if (trackname[0])
			{
				CDAudio_Play_byName(trackname, false, false, position);
				if (faketrack != -1)
					music_playlist_active = index;
			}
		}
	}
	music_playlist_playing = music_playlist_active >= 0 ? 1 : -1;
}

void CDAudio_Update (void)
{
	static int lastplaylist = -1;
	if (!enabled)
		return;

	CDAudio_SetVolume (bgmvolume.value);
	if (music_playlist_playing > 0 && CDAudio_GetPosition() < 0)
	{
		// this track ended, start a new track from the beginning
		CDAudio_StartPlaylist(false);
		lastplaylist = music_playlist_index.integer;
	}
	else if (lastplaylist != music_playlist_index.integer
	|| (bgmvolume.value > 0 && !music_playlist_playing && music_playlist_index.integer >= 0))
	{
		// active playlist changed, save position and switch track
		CDAudio_StartPlaylist(true);
		lastplaylist = music_playlist_index.integer;
	}
}

int CDAudio_Init (void)
{
	int i;

	if (cls.state == ca_dedicated)
		return -1;

// COMMANDLINEOPTION: Sound: -nocdaudio disables CD audio support
	if (Sys_CheckParm("-nocdaudio"))
		return -1;

#ifdef MAXTRACKS_256
	for (i = 0; i < MAXTRACKS_256; i++)
		*remap[i] = 0;
#endif

	Cvar_RegisterVariable(&cdaudioinitialized);
	Cvar_SetValueQuick(&cdaudioinitialized, true);
	enabled = true;

	Cvar_RegisterVariable(&music_playlist_index);
	for (i = 0;i < MAX_PLAYLISTS;i++)
	{
		Cvar_RegisterVariable(&music_playlist_list[i]);
		Cvar_RegisterVariable(&music_playlist_current[i]);
		Cvar_RegisterVariable(&music_playlist_random[i]);
		Cvar_RegisterVariable(&music_playlist_sampleposition[i]);
	}

	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "cd", CD_f, "execute a CD drive command (cd on/off/reset/remap/close/play/loop/stop/pause/resume/eject/info) - use cd by itself for usage");

	return 0;
}

int CDAudio_Startup (void)
{
	if (Sys_CheckParm("-nocdaudio"))
		return -1;

	initialized = true;

	Con_DPrintLinef ("CD Audio Initialized");

	return 0;
}

void CDAudio_Shutdown (void)
{
	if (!initialized)
		return;

	CDAudio_Stop();
	initialized = false;
}
