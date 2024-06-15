// qdefs.h

#ifndef QDEFS_H
#define QDEFS_H

#if defined (__GNUC__) || defined (__clang__) || defined (__TINYC__)
#define DP_GCC_COMPATIBLE
#endif

#if (__GNUC__ > 2) || defined (__clang__) || (__TINYC__)
#define DP_FUNC_PRINTF(n) __attribute__ ((format (printf, n, n+1)))
#define DP_FUNC_PURE      __attribute__ ((pure))
#define DP_FUNC_NORETURN  __attribute__ ((noreturn))
#define DP_FUNC_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define DP_FUNC_PRINTF(n)
#define DP_FUNC_PURE
#define DP_FUNC_NORETURN
# if defined (_MSC_VER)
# define DP_FUNC_ALWAYS_INLINE __forceinline
# else
# define DP_FUNC_ALWAYS_INLINE inline
# endif
#endif

#define MAX_NUM_ARGVS_50			50		// Baker: Windows and reconstitute the command line for the cmdline externally visible cvar

#define	MAX_INPUTLINE_16384			16384	///< maximum length of console commandline, QuakeC strings, and many other text processing buffers
#define	CON_TEXTSIZE_1_MB			1048576	///< max scrollback buffer characters in console
#define	CON_MAXLINES_16384			16384	///< max scrollback buffer lines in console
#define	HIST_TEXTSIZE				262144	///< max command history buffer characters in console
#define	HIST_MAXLINES_4096			4096	///< max command history buffer lines in console
#define	MAX_ALIAS_NAME_32			32

// Baker: 131072 * 5 = 655360 .. not sure what is special about that
#define	CMDBUFSIZE_0_6_MB			655360	///< maximum script size that can be loaded by the exec command (8192 in Quake)
#define	MAX_ARGS_80					80		///< maximum number of parameters to a console command or alias

#define FITZQUAKE_MAX_SOUNDS_2048	2048
#define	MAX_SOUNDS_QUAKE_256		256		///< max number of sounds loaded at once

#define QW_MAX_CLIENTS_32			32

#define	NET_MAXMESSAGE_65536		65536	///< max reliable packet size (sent as multiple fragments of MAX_PACKETFRAGMENT)
#define	MAX_PACKETFRAGMENT			1024	///< max length of packet fragment
#define	MAX_EDICTS_32768			32768	///< max number of objects in game world at once (32768 protocol limit)
#define	MAX_MODELS_8192				8192	///< max number of models loaded at once (including during level transitions)
#define	MAX_SOUNDS_4096				4096	///< max number of sounds loaded at once
#define	MAX_LIGHTSTYLES_256			256		///< max flickering light styles in level (note: affects savegame format)
#define	MAX_STYLESTRING_64			64		///< max length of flicker pattern for light style
#define	MAX_SCOREBOARD_255			255		///< max number of players in game at once (255 protocol limit)
#define	MAX_SCOREBOARDNAME_128		128		///< max length of player name in game
#define	MAX_USERINFO_STRING_1280	1280	///< max length of infostring for PROTOCOL_QUAKEWORLD (196 in QuakeWorld)
#define	MAX_SERVERINFO_STRING_1280	1280	///< max length of server infostring for PROTOCOL_QUAKEWORLD (512 in QuakeWorld)
#define	MAX_LOCALINFO_STRING		32768	///< max length of server-local infostring for PROTOCOL_QUAKEWORLD (32768 in QuakeWorld)
#define	CL_MAX_USERCMDS_128			128		///< max number of predicted input packets in queue
#define	CVAR_HASHSIZE_65536			65536	///< number of hash buckets for accelerating cvar name lookups
#define	M_MAX_EDICTS_32768			32768	///< max objects in menu vm
#define	MAX_DEMOS_8					8		///< max demos provided to demos command
#define	MAX_DEMONAME_32				16		///< max demo name length for demos command
#define	MAX_SAVEGAMES_20			20		///< max savegames listed in savegame menu   //Baker r1241: MAX_SAVEGAMES_20 to 20 mirroring other Quake engines.
#define	SAVEGAME_COMMENT_LENGTH_39	39		///< max comment length of savegame in menu
#define	MAX_CLIENTNETWORKEYES		16		///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_LEVELNETWORKEYES		512		///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_OCCLUSION_QUERIES		4096	///< max number of query objects that can be used in one frame (GL_ARB_occlusion_query)

#define CRYPTO_HOSTKEY_HASHSIZE_8192 8192	///< number of hash buckets for accelerating host key lookups

#if 0 // Baker: I'm not seeing this used
#define MAX_NETWM_ICON 352822 // 16x16, 22x22, 24x24, 32x32, 48x48, 64x64, 128x128, 256x256, 512x512
#endif

#define	MAX_WATERPLANES_16			16		///< max number of water planes visible (each one causes additional view renders)
#define	MAX_CUBEMAPS_1024			1024	///< max number of cubemap textures loaded for light filters
#define	MAX_EXPLOSIONS_64			64		///< max number of explosion shell effects active at once (not particle related)
#define	MAX_DLIGHTS_256				256		///< max number of dynamic lights (rocket flashes, etc) in scene at once
#define	MAX_CACHED_PICS_2048		2048	///< max number of 2D pics loaded at once
#define	CACHEPICHASHSIZE_256		256		///< number of hash buckets for accelerating 2D pic name lookups
#define	MAX_PARTICLEEFFECTNAME_4096	4096	///< maximum number of unique names of particle effects (for particleeffectnum)
#define	MAX_PARTICLEEFFECTINFO_8192	8192	///< maximum number of unique particle effects (each name may associate with several of these)
#define	MAX_PARTICLETEXTURES_256	256		///< maximum number of unique particle textures in the particle font
#define	MAXCLVIDEOS_65				65		///< maximum number of video streams being played back at once (1 is reserved for the playvideo command)

// Baker: MAX_DYNAMIC_TEXTURES_COUNT does not exist in source code
#define	MAX_DYNAMIC_TEXTURES_64		64		///< maximum number of dynamic textures (web browsers, playvideo, etc)
#if 1 // June 2
//#define	MAX_MAP_LEAFS			65536 ///< maximum number of BSP leafs in world (8192 in Quake), 
// now dynamically allocated
#else
#define	MAX_MAP_LEAFS_65536			65536	///< maximum number of BSP leafs in world (8192 in Quake)
#endif

#define	MAXTRACKS_256				256		///< max CD track index

// 0 to NUM_AMBIENTS - 1 = water, etc
// NUM_AMBIENTS to NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS_512 - 1 = normal entity sounds
// NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS_512 to total_channels = static sounds

#define	MAX_DYNAMIC_CHANNELS_512	512
#define	MAX_CHANNELS_8196			(8192 + 4)
#define	MODLIST_MAX_ENTRIES_256		256
#define	MAX_FAVORITESERVERS			256
#define	MAX_DECALSYSTEM_QUEUE_1024	1024
#define	PAINTBUFFER_SIZE_2048		2048
#define	MAX_BINDMAPS_8				8
#define	MAX_PARTICLES_INITIAL_8192	8192	///< initial allocation for cl.particles
#define	MAX_PARTICLES				1048576	///< upper limit on cl.particles size
#define	MAX_ENTITIES_INITIAL_256	256		///< initial size of cl.entities
#define	MAX_STATICENTITIES_4096		4096	///< limit on size of cl.static_entities

#define	MAX_EFFECTS_256				256		///< limit on size of cl.effects
#define	MAX_BEAMS_256				256		///< limit on size of cl.beams
#define	MAX_TEMPENTITIES_4096		4096	///< max number of temporary models visible per frame (certain sprite effects, certain types of CSQC entities also use this)
#define SERVERLIST_TOTALSIZE_2048	2048	///< max servers in the server list
#define SERVERLIST_ANDMASKCOUNT		16		///< max items in server list AND mask
#define SERVERLIST_ORMASKCOUNT		16		///< max items in server list OR mask

#define CMD_TOKENIZELENGTH (MAX_INPUTLINE_16384 + MAX_ARGS_80) ///< maximum tokenizable commandline length (counting trailing 0)


#define	MAX_QPATH_128				128		///< max length of a quake game pathname

#ifdef PATH_MAX
	#define	MAX_OSPATH		PATH_MAX
#elif MAX_PATH
	#define	MAX_OSPATH		MAX_PATH
#else
	#define	MAX_OSPATH		1024		///< max length of a filesystem pathname
#endif

#define	ON_EPSILON_0_1F				0.1			///< point on plane side epsilon

#define	NET_MINRATE_1000			1000 ///< limits "rate" and "sv_maxrate" cvars

// In Quake, any char in 0..32 counts as whitespace
//#define ISWHITESPACE(ch) ((unsigned char) ch <= (unsigned char) ' ')
#define ISWHITESPACE(ch) (!(ch) || (ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')
#define ISCOMMENT(ch, pos) ch[pos] == '/' && ch[pos + 1] == '/' && (pos == 0 || ISWHITESPACE(ch[pos - 1]))
// This also includes extended characters, and ALL control chars
#define ISWHITESPACEORCONTROL(ch) ((signed char) (ch) <= (signed char) ' ')

#define DOUBLE_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFFFFFFFFFF) // also match "negative zero" doubles of value 0x8000000000000000
#define DOUBLE_LOSSLESS_FORMAT "%.17g"
#define DOUBLE_VECTOR_LOSSLESS_FORMAT "%.17g %.17g %.17g"

#define FLOAT_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFF) // also match "negative zero" floats of value 0x80000000
#define FLOAT_LOSSLESS_FORMAT "%.9g"
#define FLOAT_VECTOR_LOSSLESS_FORMAT "%.9g %.9g %.9g"

// originally this was _MSC_VER
// but here we want to test the system libc, which on win32 is borked, and NOT the compiler
#ifdef _WIN32
#define INT_LOSSLESS_FORMAT_SIZE "I64"	// Baker: Sole use is in VM_sprintf
#define INT_LOSSLESS_FORMAT_CONVERT_S(x) ((__int64)(x))
#define INT_LOSSLESS_FORMAT_CONVERT_U(x) ((unsigned __int64)(x))
#else
#define INT_LOSSLESS_FORMAT_SIZE "j"	// Baker: Sole use is in VM_sprintf
#define INT_LOSSLESS_FORMAT_CONVERT_S(x) ((intmax_t)(x))
#define INT_LOSSLESS_FORMAT_CONVERT_U(x) ((uintmax_t)(x))
#endif

// simple safe library to handle integer overflows when doing buffer size calculations
// Usage:
//   - calculate data size using INTOVERFLOW_??? macros
//   - compare: calculated-size <= INTOVERFLOW_NORMALIZE(buffersize)
// Functionality:
//   - all overflows (values > INTOVERFLOW_MAX) and errors are mapped to INTOVERFLOW_MAX
//   - if any input of an operation is INTOVERFLOW_MAX, INTOVERFLOW_MAX will be returned
//   - otherwise, regular arithmetics apply

#define INTOVERFLOW_MAX 2147483647

#define INTOVERFLOW_ADD(a,b) (((a) < INTOVERFLOW_MAX && (b) < INTOVERFLOW_MAX && (a) < INTOVERFLOW_MAX - (b)) ? ((a) + (b)) : INTOVERFLOW_MAX)
#define INTOVERFLOW_SUB(a,b) (((a) < INTOVERFLOW_MAX && (b) < INTOVERFLOW_MAX && (b) <= (a))                  ? ((a) - (b)) : INTOVERFLOW_MAX)
#define INTOVERFLOW_MUL(a,b) (((a) < INTOVERFLOW_MAX && (b) < INTOVERFLOW_MAX && (a) < INTOVERFLOW_MAX / (b)) ? ((a) * (b)) : INTOVERFLOW_MAX)
#define INTOVERFLOW_DIV(a,b) (((a) < INTOVERFLOW_MAX && (b) < INTOVERFLOW_MAX && (b) > 0)                     ? ((a) / (b)) : INTOVERFLOW_MAX)

#define INTOVERFLOW_NORMALIZE(a) (((a) < INTOVERFLOW_MAX) ? (a) : (INTOVERFLOW_MAX - 1))

#endif
