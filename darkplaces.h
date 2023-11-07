/*
Copyright (C) 2020 Ashley Rose Hale (LadyHavoc)
Copyright (C) 2020 David Knapp (Cloudwalk)

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

/* darkplaces.h - Master definitions file for Darkplaces engine */

#ifndef DARKPLACES_H
#define DARKPLACES_H

extern const char *buildstring;
extern const char *buildstringshort;
extern char engineversion[128];
extern char engineversionshort[128];

#ifdef __APPLE__
	# include <TargetConditionals.h>
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
	#define DP_FUNC_PRINTF(n) __attribute__ ((format (printf, n, n+1)))
	#define DP_FUNC_PURE      __attribute__ ((pure))
	#define DP_FUNC_NORETURN  __attribute__ ((noreturn))
#else
	#define DP_FUNC_PRINTF(n)
	#define DP_FUNC_PURE
	#define DP_FUNC_NORETURN
#endif

#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <setjmp.h>

#include "qtypes.h"

#ifdef SUPPORTDIRECTX
#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined
#endif


#define MAX_NUM_ARGVS	50				// Baker: Windows and reconstitute the command line for the cmdline externally visible cvar


#define	MAX_INPUTLINE			16384 ///< maximum length of console commandline, QuakeC strings, and many other text processing buffers
#define	CON_TEXTSIZE			1048576 ///< max scrollback buffer characters in console
#define	CON_MAXLINES			16384 ///< max scrollback buffer lines in console
#define	HIST_TEXTSIZE			262144 ///< max command history buffer characters in console
#define	HIST_MAXLINES			4096 ///< max command history buffer lines in console
#define	MAX_ALIAS_NAME			32
#define	CMDBUFSIZE				655360 ///< maximum script size that can be loaded by the exec command (8192 in Quake)
#define	MAX_ARGS				80 ///< maximum number of parameters to a console command or alias

#define	NET_MAXMESSAGE			65536 ///< max reliable packet size (sent as multiple fragments of MAX_PACKETFRAGMENT)
#define	MAX_PACKETFRAGMENT		1024 ///< max length of packet fragment
#define	MAX_EDICTS				32768 ///< max number of objects in game world at once (32768 protocol limit)
#define	MAX_MODELS				8192 ///< max number of models loaded at once (including during level transitions)
#define	MAX_SOUNDS				4096 ///< max number of sounds loaded at once
#define	MAX_LIGHTSTYLES			256 ///< max flickering light styles in level (note: affects savegame format)
#define	MAX_STYLESTRING			64 ///< max length of flicker pattern for light style
#define	MAX_SCOREBOARD			255 ///< max number of players in game at once (255 protocol limit)
#define	MAX_SCOREBOARDNAME		128 ///< max length of player name in game
#define	MAX_USERINFO_STRING		1280 ///< max length of infostring for PROTOCOL_QUAKEWORLD (196 in QuakeWorld)
#define	MAX_SERVERINFO_STRING	1280 ///< max length of server infostring for PROTOCOL_QUAKEWORLD (512 in QuakeWorld)
#define	MAX_LOCALINFO_STRING	32768 ///< max length of server-local infostring for PROTOCOL_QUAKEWORLD (32768 in QuakeWorld)
#define	CL_MAX_USERCMDS			128 ///< max number of predicted input packets in queue
#define	CVAR_HASHSIZE			65536 ///< number of hash buckets for accelerating cvar name lookups
#define	M_MAX_EDICTS			32768 ///< max objects in menu vm
#define	MAX_DEMOS				8 ///< max demos provided to demos command
#define	MAX_DEMONAME			16 ///< max demo name length for demos command
#define	MAX_SAVEGAMES			20 ///< max savegames listed in savegame menu
#define	SAVEGAME_COMMENT_LENGTH	39 ///< max comment length of savegame in menu
#define	MAX_CLIENTNETWORKEYES	16 ///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_LEVELNETWORKEYES	512 ///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_OCCLUSION_QUERIES	4096 ///< max number of GL_ARB_occlusion_query objects that can be used in one frame

#define CRYPTO_HOSTKEY_HASHSIZE 8192 ///< number of hash buckets for accelerating host key lookups
#define MAX_NETWM_ICON 352822 // 16x16, 22x22, 24x24, 32x32, 48x48, 64x64, 128x128, 256x256, 512x512

#define	MAX_WATERPLANES			16 ///< max number of water planes visible (each one causes additional view renders)
#define	MAX_CUBEMAPS			1024 ///< max number of cubemap textures loaded for light filters
#define	MAX_EXPLOSIONS			64 ///< max number of explosion shell effects active at once (not particle related)
#define	MAX_DLIGHTS				256 ///< max number of dynamic lights (rocket flashes, etc) in scene at once
#define	MAX_CACHED_PICS			2048 ///< max number of 2D pics loaded at once .. 04242023 DP mirror
#define	CACHEPICHASHSIZE		256 ///< number of hash buckets for accelerating 2D pic name lookups
#define	MAX_PARTICLEEFFECTNAME	4096 ///< maximum number of unique names of particle effects (for particleeffectnum)
#define	MAX_PARTICLEEFFECTINFO	8192 ///< maximum number of unique particle effects (each name may associate with several of these)
#define	MAX_PARTICLETEXTURES	256 ///< maximum number of unique particle textures in the particle font
#define	MAXCLVIDEOS				65 ///< maximum number of video streams being played back at once (1 is reserved for the playvideo command)
#define	MAX_DYNAMIC_TEXTURE_COUNT	64 ///< maximum number of dynamic textures (web browsers, playvideo, etc)
#define	MAX_MAP_LEAFS			65536 ///< maximum number of BSP leafs in world (8192 in Quake)

#define	MAXTRACKS				256 ///< max CD track index
// 0 to NUM_AMBIENTS - 1 = water, etc
// NUM_AMBIENTS to NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS - 1 = normal entity sounds
// NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS to total_channels = static sounds
#define	MAX_DYNAMIC_CHANNELS	512
#define	MAX_CHANNELS			(8192 + 4)
#define	MODLIST_TOTALSIZE		256
#define	MAX_FAVORITESERVERS		256
#define	MAX_DECALSYSTEM_QUEUE	1024
#define	PAINTBUFFER_SIZE		2048
#define	MAX_BINDMAPS			8
#define	MAX_PARTICLES_INITIAL	8192 ///< initial allocation for cl.particles
#define	MAX_PARTICLES			1048576 ///< upper limit on cl.particles size
#define	MAX_DECALS_INITIAL		8192 ///< initial allocation for cl.decals
#define	MAX_DECALS				1048576 ///< upper limit on cl.decals size
#define	MAX_ENITIES_INITIAL		256 ///< initial size of cl.entities
#define	MAX_STATICENTITIES		4096 ///< limit on size of cl.static_entities  .. 04242023 DP mirror
#define	MAX_EFFECTS				256 ///< limit on size of cl.effects
#define	MAX_BEAMS				256 ///< limit on size of cl.beams
#define	MAX_TEMPENTITIES		4096 ///< max number of temporary models visible per frame (certain sprite effects, certain types of CSQC entities also use this)
#define SERVERLIST_TOTALSIZE		2048 ///< max servers in the server list
#define SERVERLIST_ANDMASKCOUNT		16 ///< max items in server list AND mask
#define SERVERLIST_ORMASKCOUNT		16 ///< max items in server list OR mask



#define CMD_TOKENIZELENGTH (MAX_INPUTLINE + MAX_ARGS) ///< maximum tokenizable commandline length (counting trailing 0)


#define	MAX_QPATH		128			///< max length of a quake game pathname
#ifdef PATH_MAX
	#define	MAX_OSPATH	PATH_MAX
#elif MAX_PATH
	#define	MAX_OSPATH	MAX_PATH
#else
	#define	MAX_OSPATH	1024		///< max length of a filesystem pathname
#endif

#define	ON_EPSILON		0.1			///< point on plane side epsilon

#define	NET_MINRATE		1000 ///< limits "rate" and "sv_maxrate" cvars

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		256
#define	STAT_HEALTH			0
//#define	STAT_FRAGS		1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		///< bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		///< bumped by svc_killedmonster
#define STAT_ITEMS			15 ///< FTE, DP
#define STAT_VIEWHEIGHT		16 ///< FTE, DP
//#define STAT_TIME			17 ///< FTE
//#define STAT_VIEW2		20 ///< FTE
#define STAT_VIEWZOOM		21 ///< DP

#define STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR		220 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_PENALTY			221 ///< DP
#define STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW			222 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCEL_QW				223 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_POWER				224 ///< DP
#define STAT_MOVEFLAGS                              225 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL	226 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_ACCEL				227 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED			228 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL			229 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO	230 ///< DP
#define STAT_MOVEVARS_AIRSTOPACCELERATE				231 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCELERATE			232 ///< DP
#define STAT_MOVEVARS_MAXAIRSTRAFESPEED				233 ///< DP
#define STAT_MOVEVARS_AIRCONTROL					234 ///< DP
#define STAT_FRAGLIMIT								235 ///< DP
#define STAT_TIMELIMIT								236 ///< DP
#define STAT_MOVEVARS_WALLFRICTION					237 ///< DP
#define STAT_MOVEVARS_FRICTION						238 ///< DP
#define STAT_MOVEVARS_WATERFRICTION					239 ///< DP
#define STAT_MOVEVARS_TICRATE						240 ///< DP
#define STAT_MOVEVARS_TIMESCALE						241 ///< DP
#define STAT_MOVEVARS_GRAVITY						242 ///< DP
#define STAT_MOVEVARS_STOPSPEED						243 ///< DP
#define STAT_MOVEVARS_MAXSPEED						244 ///< DP
#define STAT_MOVEVARS_SPECTATORMAXSPEED				245 ///< DP
#define STAT_MOVEVARS_ACCELERATE					246 ///< DP
#define STAT_MOVEVARS_AIRACCELERATE					247 ///< DP
#define STAT_MOVEVARS_WATERACCELERATE				248 ///< DP
#define STAT_MOVEVARS_ENTGRAVITY					249 ///< DP
#define STAT_MOVEVARS_JUMPVELOCITY					250 ///< DP
#define STAT_MOVEVARS_EDGEFRICTION					251 ///< DP
#define STAT_MOVEVARS_MAXAIRSPEED					252 ///< DP
#define STAT_MOVEVARS_STEPHEIGHT					253 ///< DP
#define STAT_MOVEVARS_AIRACCEL_QW					254 ///< DP
#define STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION	255 ///< DP



#include "zone.h"
#include "fs.h"
#include "baker.h"
#include "common.h"
#include "cvar.h"
#include "bspfile.h"
#include "sys.h"
#include "vid.h"
#include "mathlib.h"

#include "r_textures.h"

#include "crypto.h"
#include "draw.h"
#include "screen.h"
#include "netconn.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "model_shared.h"
#include "world.h"
#include "client.h"
#include "render.h"
#include "progs.h"
#include "progsvm.h"
#include "server.h"

#include "input.h"
#include "keys.h"
#include "console.h"

#ifdef CONFIG_MENU
	#include "menu.h"
#endif

#include "csprogs.h"


extern cvar_t developer;
extern cvar_t developer_extra;
extern cvar_t developer_insane;
extern cvar_t developer_loadfile;
extern cvar_t developer_loading;

extern cvar_t sessionid;


#include "sys.h"


#include "glquake.h"

#include "palette.h"

/// incremented every frame, never reset
extern int host_framecount;
/// not bounded in any way, changed at start of every frame, never reset
extern double realtime;
/// equal to Sys_DirtyTime() at the start of this host frame
extern double host_dirtytime;

void Host_InitCommands(void);
void Host_Main(void);
void Host_Shutdown(void);
void Host_StartVideo(void);
void Host_Error(const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;
void Host_Quit_f(void);
void Host_ClientCommands(const char *fmt, ...) DP_FUNC_PRINTF(1);
void Host_ShutdownServer(void);
void Host_Reconnect_f(void);
void Host_NoOperation_f(void);
void Host_LockSession(void);
void Host_UnlockSession(void);

void Host_AbortCurrentFrame(void);


void LOC_LoadFile (void); // AURA 8.0
const char *LOC_GetString (const char *s_dollar_key); // AURA 1.2

// In Quake, any char in 0..32 counts as whitespace
//#define ISWHITESPACE(ch) ((unsigned char) ch <= (unsigned char) ' ')
#define ISWHITESPACE(ch) (!(ch) || (ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')

// This also includes extended characters, and ALL control chars
#define ISWHITESPACEORCONTROL(ch) ((signed char) (ch) <= (signed char) ' ')


#ifdef PRVM_64
	#define FLOAT_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFFFFFFFFFF) // also match "negative zero" doubles of value 0x8000000000000000
	#define FLOAT_LOSSLESS_FORMAT "%.17g"
	#define VECTOR_LOSSLESS_FORMAT "%.17g %.17g %.17g"
#else
	#define FLOAT_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFF) // also match "negative zero" floats of value 0x80000000
	#define FLOAT_LOSSLESS_FORMAT "%.9g"
	#define VECTOR_LOSSLESS_FORMAT "%.9g %.9g %.9g"
#endif

// originally this was _MSC_VER
// but here we want to test the system libc, which on win32 is borked, and NOT the compiler
#ifdef _WIN32
	#define INT_LOSSLESS_FORMAT_SIZE			"I64"
	#define INT_LOSSLESS_FORMAT_CONVERT_S(x)	((__int64)(x))
	#define INT_LOSSLESS_FORMAT_CONVERT_U(x)	((unsigned __int64)(x))
#else
	#define INT_LOSSLESS_FORMAT_SIZE			"j"
	#define INT_LOSSLESS_FORMAT_CONVERT_S(x)	((intmax_t)(x))
	#define INT_LOSSLESS_FORMAT_CONVERT_U(x)	((uintmax_t)(x))
#endif // _WIN32


#endif // ! DARKPLACES_H