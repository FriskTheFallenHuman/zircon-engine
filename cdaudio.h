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

#include "qtypes.h"
#include "cvar.h"

extern qbool cdValid;
extern qbool cdPlaying;
extern qbool cdPlayLooping;
extern unsigned char cdPlayTrack;

extern cvar_t cdaudioinitialized;

int CDAudio_Init(void);
void CDAudio_Open(void);
void CDAudio_Close(void);
void CDAudio_Play(int track, qbool looping);
void CDAudio_Play_byName (const char *trackname, qbool looping, qbool tryreal, float startposition);
void CDAudio_Stop(void);
void CDAudio_Pause(void);
void CDAudio_Resume(void);
int CDAudio_Startup(void);
void CDAudio_Shutdown(void);
void CDAudio_Update(void);
float CDAudio_GetPosition(void);
//void CDAudio_StartPlaylist(qbool resume);

// Prototypes of the system dependent functions
void CDAudio_SysEject (void);
void CDAudio_SysCloseDoor (void);
int CDAudio_SysGetAudioDiskInfo (void);
float CDAudio_SysGetVolume (void);
void CDAudio_SysSetVolume (float volume);
int CDAudio_SysPlay (int track);
int CDAudio_SysStop (void);
int CDAudio_SysPause (void);
int CDAudio_SysResume (void);
int CDAudio_SysUpdate (void);
void CDAudio_SysInit (void);
int CDAudio_SysStartup (void);
void CDAudio_SysShutdown (void);
