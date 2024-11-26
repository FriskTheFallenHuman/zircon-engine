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
// server.h

#ifndef SERVER_H
#define SERVER_H

typedef struct server_static_s{
	int				maxclients, maxclients_next;				/// number of svs.clients slots (updated by maxplayers command)
	struct client_s	*clients;									/// client slots
	int				serverflags;								/// episode completion information
	qbool			changelevel_issued;							/// cleared when at SV_SpawnServer
	char			serverinfo[MAX_SERVERINFO_STRING_1280];		/// server infostring

	WARP_X_ (PEXT_CHUNKEDDOWNLOADS)


	// csqc stuff
	unsigned char	*csqc_progdata;
	size_t			csqc_progsize_deflated;
	unsigned char	*csqc_progdata_deflated;

	// independent server thread (when running client)
	qbool			threaded; // true if server is running on separate thread
	qbool volatile	threadstop;
	void			*threadmutex;
	void			*thread;
} server_static_t;

//=============================================================================

typedef enum server_state_e {ss_loading, ss_active} server_state_t;

#define MAX_CONNECTFLOODADDRESSES 16
#define MAX_GETSTATUSFLOODADDRESSES 128
typedef struct server_floodaddress_s
{
	double lasttime;
	lhnetaddress_t address;
}
server_floodaddress_t;

#define AUTOSAVE_INTERVAL_90		90.0
#define AUTOSAVE_NAME_AUTOSAVE0_SAV	"_auto_save_0.sav"

typedef struct server_s
{
	/// false if only a net client
	qbool active;

	qbool paused;
	double pausedstart;
	/// handle connections specially
	qbool loadgame;

	/// one of the PROTOCOL_ values
	protocolversion_t protocol;

	double time;
	double frametime;

	unsigned int spawnframe; // signals SV_Frame() to reset its timers
		// Baker: looks like: tell SV_Frame() to reset its timers  sv.spawnframe = host.superframecount;


	// performance data
	float perf_cpuload;
	float perf_lost;
	float perf_offset_avg;
	float perf_offset_max;
	float perf_offset_sdev;
	// temporary performance data accumulators
	float perf_acc_realtime;
	float perf_acc_sleeptime;
	float perf_acc_lost;
	float perf_acc_offset;
	float perf_acc_offset_squared;
	float perf_acc_offset_max;
	int perf_acc_offset_samples;

	// used by PF_checkclient
	int lastcheck; // Baker: Last player edictnumber returns by checkclient (monster finds a player)
	double lastchecktime;

	// crc of clientside progs at time of level start
	int csqc_progcrc; // -1 = no progs
	int csqc_progsize; // -1 = no progs
	char csqc_progname[MAX_QPATH_128]; // copied from csqc_progname at level start

	/// collision culling data
	world_t world;

	/// map name
	char name[64]; // %s followed by entrance name

	WARP_X_ (enable_intermap sv_siv_virtual_files_list) // This means save games use intermap
	qbool		was_intermap_loaded_from_siv;	// If the map was re-entered, "restart" uses .siv
	char		intermap_startspot[64];			// intermap "startspot" -- QuakeC can use or not.
												// Will set global "startspot" if present in the QuakeC
	vec3_t		intermap_startorigin;			// intermap "coordinates" -- QuakeC can use or not.
	float		intermap_totaltimeatstart;		// intermap totaltime in world accumulated at changelevel (totaltime + sv.time = total game time)
	float		intermap_totaltimeatlastexit;	// intermap totaltime in a map when it was last exited
	float		intermap_surplustime;			// intermap totaltime in world accumulated at changelevel (totaltime + sv.time = total game time)

	float		cdtrack_seconds_into;
	float		sv_autosave_time;
	WARP_X_ (sv_siv_list)
	//char		intermap_map_set[MAX_INPUTLINE_16384]; // Every .siv saved updates this
											// When saving we do not do the map we are on.
	//char		intermap_sessionid[32];		// We use a float like "27397.900391", QuakeC assigns.
	

	int is_qex; // AURA 9.0
	// variants of map name
	char worldmessage[40]; // map title (not related to filename)
	char worldbasename[MAX_QPATH_128]; // %s
	char worldname[MAX_QPATH_128]; // maps/%s.bsp
	char worldnamenoextension[MAX_QPATH_128]; // maps/%s
	//char		startspot[64];		// Q2X
	struct model_s *worldmodel;
	// NULL terminated
	// LadyHavoc: precaches are now MAX_QPATH_128 rather than a pointer
	// updated by SV_ModelIndex
	char model_precache[MAX_MODELS_8192][MAX_QPATH_128];
	struct model_s *models[MAX_MODELS_8192];
	// NULL terminated
	// LadyHavoc: precaches are now MAX_QPATH_128 rather than a pointer
	// updated by SV_SoundIndex
	char sound_precache[MAX_SOUNDS_4096][MAX_QPATH_128];
	char lightstyles[MAX_LIGHTSTYLES_256][64];
	/// some actions are only valid during load
	server_state_t state;

	sizebuf_t datagram;
	unsigned char datagram_buf[NET_MAXMESSAGE_65536];

	// copied to all clients at end of frame
	sizebuf_t reliable_datagram;
	unsigned char reliable_datagram_buf[NET_MAXMESSAGE_65536];

	sizebuf_t signon;
	/// LadyHavoc: increased signon message buffer from 8192
	unsigned char signon_buf[NET_MAXMESSAGE_65536];

	/// connection flood blocking
	/// note this is in server_t rather than server_static_t so that it is
	/// reset on each map command (such as New Game in singleplayer)
	server_floodaddress_t connectfloodaddresses[MAX_CONNECTFLOODADDRESSES];
	server_floodaddress_t getstatusfloodaddresses[MAX_GETSTATUSFLOODADDRESSES];

	qbool particleeffectnamesloaded;
	char particleeffectname[MAX_PARTICLEEFFECTNAME_4096][MAX_QPATH_128];

	int writeentitiestoclient_stats_culled_pvs;
	int writeentitiestoclient_stats_culled_trace;
	int writeentitiestoclient_stats_visibleentities;
	int writeentitiestoclient_stats_totalentities;
	int writeentitiestoclient_cliententitynumber;
	int writeentitiestoclient_clientnumber;
	sizebuf_t *writeentitiestoclient_msg;
	vec3_t writeentitiestoclient_eyes[MAX_CLIENTNETWORKEYES];
	int writeentitiestoclient_numeyes;
#if 1 // June 2
	unsigned char *writeentitiestoclient_pvs;
#else
	int writeentitiestoclient_pvsbytes;
	unsigned char writeentitiestoclient_pvs[MAX_MAP_LEAFS_65536/8];
#endif
	const entity_state_t *writeentitiestoclient_sendstates[MAX_EDICTS_32768];
	unsigned short writeentitiestoclient_csqcsendstates[MAX_EDICTS_32768];

	int numsendentities;
	entity_state_t sendentities[MAX_EDICTS_32768];
	entity_state_t *sendentitiesindex[MAX_EDICTS_32768];

	int sententitiesmark;
	int sententities[MAX_EDICTS_32768];
	int sententitiesconsideration[MAX_EDICTS_32768];

	/// legacy support for self.Version based csqc entity networking
	unsigned char csqcentityversion[MAX_EDICTS_32768]; // legacy

	unsigned int zirconprotcolextensions_sv;
	int zircon_jumpflag_fieldoffset;
} server_t;

#define NUM_CSQCENTITIES_PER_FRAME 256
typedef struct csqcentityframedb_s
{
	int framenum;
	int num;
	unsigned short entno[NUM_CSQCENTITIES_PER_FRAME];
	int sendflags[NUM_CSQCENTITIES_PER_FRAME];
} csqcentityframedb_t;

#define NUM_SPAWN_PARMS_16 16

typedef struct client_s
{
	/// false = empty client slot
	qbool active;
	/// false = don't do ClientDisconnect on drop
	qbool clientconnectcalled;
	/// false = don't allow spawn
	qbool prespawned;
	/// false = don't allow begin
	qbool spawned;
	/// false = don't send datagrams
	qbool begun;
	/// 1 = send svc_serverinfo and advance to 2, 2 doesn't send, then advances to 0 (allowing unlimited sending) when prespawn is received
	int sendsignon;

	/// requested rate in bytes per second
	int rate;

	// temporarily exceed rate by this amount of bytes
	int rate_burstsize;

	// realtime this client connected
	double connecttime;

	// keepalive messages must be sent periodically during signon
	double keepalivetime;

	// communications handle
	netconn_t *netconnection;

	unsigned int movesequence;
	signed char movement_count[NETGRAPH_PACKETS_256];
	unsigned int movement_highestsequence_seen; // not the same as movesequence if prediction is off
	/// movement
	usercmd_t cmd;
	/// intended motion calced from cmd
	vec3_t wishdir;

	/// PRVM_EDICT_NUM(clientnum+1)
	prvm_edict_t *edict;

	/// LadyHavoc: can be used for prediction or whatever...
	float ping;

	/// this is used by sv_clmovement_minping code
	double clmovement_disabletimeout;
	/// this is used by sv_clmovement_inputtimeout code
	float clmovement_inputtimeout;

// SERVER:
#if 5432
	int last_wanted_is_zircon_free_move;

	// And we treat clc_zircon_move as clc_move
	// Until client responds with (byte)clc_zircon_fixangle_ack (LONG) fixangle_lastmovesequence  
	WARP_X_ (clc_move clc_zircon_move clc_zircon_warp_ack svc_zircon_warp)
	unsigned int zircon_warp_sequence; // Send svc_zircon_warp every frame until client responds with clc_zircon_warp_ack
	int zircon_warp_sequence_cleared; // Send svc_zircon_warp every frame until client responds with clc_zircon_warp_ack
#endif

/// spawn parms are carried from level to level
	prvm_vec_t spawn_parms[NUM_SPAWN_PARMS_16];

	// properties that are sent across the network only when changed
	char name[MAX_SCOREBOARDNAME_128], old_name[MAX_SCOREBOARDNAME_128];
	int colors, old_colors;
	int frags, old_frags;
	char playermodel[MAX_QPATH_128], old_model[MAX_QPATH_128];
	char playerskin[MAX_QPATH_128], old_skin[MAX_QPATH_128];

	/// netaddress support
	char netaddress[MAX_QPATH_128];

	/// visibility state
	float visibletime[MAX_EDICTS_32768];

	// scope is whether an entity is currently being networked to this client
	// sendflags is what properties have changed on the entity since the last
	// update that was sent
	int csqcnumedicts;
	unsigned char csqcentityscope[MAX_EDICTS_32768];
	unsigned int csqcentitysendflags[MAX_EDICTS_32768];

#define NUM_CSQCENTITYDB_FRAMES 256
	csqcentityframedb_t csqcentityframehistory[NUM_CSQCENTITYDB_FRAMES];
	int csqcentityframehistory_next;
	int csqcentityframe_lastreset;

	/// prevent animated names
	float nametime;

	/// latest received clc_ackframe (used to detect packet loss)
	int latestframenum; // Baker: CL last acknowledged framenum Entity Database (PHYSICAL)

	/// cache weaponmodel name lookups
	char weaponmodel[MAX_QPATH_128];
	int weaponmodelindex;

	/// clientcamera (entity to use as camera)
	int clientcamera;

	entityframe_database_t *entitydatabase;
	entityframe4_database_t *entitydatabase4;
	entityframe5_database_t *entitydatabase5;

	// delta compression of stats
	unsigned char statsdeltabits[(MAX_CL_STATS_256+7)/8];
	int stats[MAX_CL_STATS_256];

	unsigned char unreliablemsg_data[NET_MAXMESSAGE_65536];
	sizebuf_t unreliablemsg;
	int unreliablemsg_splitpoints;
	int unreliablemsg_splitpoint[NET_MAXMESSAGE_65536/16];

	// information on an active download if any
	qfile_t *download_file;
	int download_expectedposition; ///< next position the client should ack
	qbool download_started;
	char download_name[MAX_QPATH_128];
	qbool download_deflate;
	qbool download_chunked;

	// fixangle data
	qbool fixangle_angles_set; // Baker: This is pre-existing DarkPlaces but relevant to ZMOVE
	vec3_t fixangle_angles;

	/// demo recording
	qfile_t *sv_demo_file;

	// number of skipped entity frames
	// if it exceeds a limit, an empty entity frame is sent
	int num_skippedentityframes;

	// last sent move sequence
	// if the move sequence changed, an empty entity frame is sent
	unsigned int	lastmovesequence;

	unsigned int 	cl_zirconprotocolextensions;
	int				download_chunks_perframe;
} client_t; // sv client


//=============================================================================

#define WATER_LEVEL_NOT_0			0		// self.waterlevel
#define WATER_LEVEL_FEET_1			1
#define WATER_LEVEL_WAIST_2			2
#define WATER_LEVEL_EYES_MAX_3		3


// edict->movetype values
#define	MOVETYPE_NONE_0				0		///< never moves
#define	MOVETYPE_ANGLENOCLIP_1		1
#define	MOVETYPE_ANGLECLIP_2		2
#define	MOVETYPE_WALK_3				3		///< gravity
#define	MOVETYPE_STEP_4				4		///< gravity, special edge handling
#define	MOVETYPE_FLY_5				5
#define	MOVETYPE_TOSS_6				6		///< gravity
#define	MOVETYPE_PUSH_7				7		///< no clip to world, push and crush
#define	MOVETYPE_NOCLIP_8			8
#define	MOVETYPE_FLYMISSILE_9		9		///< extra size to monsters
#define	MOVETYPE_BOUNCE_10			10
#define	MOVETYPE_GIB_FIGHTS_BOUNCEMISSILE_11	11		// 2021 rerelease gibs // AURA 9.1
#define MOVETYPE_BOUNCEMISSILE_11	11		///< bounce w/o gravity
#define MOVETYPE_FOLLOW_12			12		///< track movement of aiment
#define MOVETYPE_FAKEPUSH_13		13		///< tenebrae's push that doesn't push
#define MOVETYPE_PHYSICS_32			32		///< indicates this object is physics controlled
#define MOVETYPE_FLY_WORLDONLY_33	33		///< like MOVETYPE_FLY_5, but uses MOVE_WORLDONLY_3 for all its traces; objects of this movetype better be SOLID_NOT_0 or SOLID_TRIGGER_1 please, or else...

#define MOVETYPE_BOAT_34			34		// Experimental for boats

#define MOVETYPE_USER_FIRST_128		128		///< user defined movetypes
#define MOVETYPE_USER_LAST_191		191

// edict->solid values
#define	SOLID_NOT_0				0		///< no interaction with other objects
#define	SOLID_TRIGGER_1			1		///< touch on edge, but not blocking
#define	SOLID_BBOX_2			2		///< touch on edge, block
#define	SOLID_SLIDEBOX_3		3		///< touch on edge, but not an onground
#define	SOLID_BSP_4				4		///< bsp clip, touch on edge, block
// LadyHavoc: corpse code
#define	SOLID_CORPSE_5			5		///< same as SOLID_BBOX_2, except it behaves as SOLID_NOT_0 against SOLID_SLIDEBOX_3 objects (players/monsters)
// LadyHavoc: physics
// VorteX: now these fields are deprecated, as geomtype is more flexible
#define	SOLID_PHYSICS_BOX		32		///< physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_SPHERE	33		///< physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_CAPSULE	34		///< physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_TRIMESH	35		///< physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_CYLINDER	36		///< physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)

// edict->deadflag values
#define	DEAD_NO					0
#define	DEAD_DYING				1
#define	DEAD_DEAD				2

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// edict->flags
#define	FL_FLY_1				1
#define	FL_SWIM_2				2
#define	FL_CONVEYOR				4		// Baker: Used? It is not.  FTE?  Don't see it instead commented out FL_GLIMPSE
#define	FL_CLIENT_8				8
#define	FL_INWATER				16
#define	FL_MONSTER_32			32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM_256				256
#define	FL_ONGROUND_512				512
#define	FL_PARTIALGROUND		1024	///< not all corners are valid
#define	FL_WATERJUMP			2048	///< player jumping out of water
#define	FL_JUMPRELEASED			4096	///< for jump debouncing

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

//============================================================================

extern cvar_t coop;
extern cvar_t deathmatch;
extern cvar_t fraglimit;
extern cvar_t gamecfg;
extern cvar_t noexit;
extern cvar_t nomonsters;
extern cvar_t pausable;
extern cvar_t pr_checkextension;
extern cvar_t samelevel;
extern cvar_t saved1;
extern cvar_t saved2;
extern cvar_t saved3;
extern cvar_t saved4;
extern cvar_t savedgamecfg;
extern cvar_t scratch1;
extern cvar_t scratch2;
extern cvar_t scratch3;
extern cvar_t scratch4;
extern cvar_t skill;
extern cvar_t host_timescale;
extern cvar_t sv_accelerate;
extern cvar_t sv_aim;
extern cvar_t sv_airaccel_qw;
extern cvar_t sv_airaccel_sideways_friction;
extern cvar_t sv_airaccelerate;
extern cvar_t sv_airstopaccelerate;
extern cvar_t sv_airstrafeaccelerate;
extern cvar_t sv_maxairstrafespeed;
extern cvar_t sv_airstrafeaccel_qw;
extern cvar_t sv_aircontrol;
extern cvar_t sv_aircontrol_power;
extern cvar_t sv_aircontrol_penalty;
extern cvar_t sv_altnoclipmove; // Baker r0085: FitzQuake noclipping

extern cvar_t sv_airspeedlimit_nonqw;
extern cvar_t sv_allowdownloads;
extern cvar_t sv_allowdownloads_archive;
extern cvar_t sv_allowdownloads_config;
extern cvar_t sv_allowdownloads_dlcache;
extern cvar_t sv_allowdownloads_inarchive;
extern cvar_t sv_areagrid_link_SOLID_NOT;
extern cvar_t sv_areagrid_mingridsize;
extern cvar_t sv_checkforpacketsduringsleep;
extern cvar_t sv_clmovement_enable;
extern cvar_t sv_clmovement_minping;
extern cvar_t sv_clmovement_minping_disabletime;
extern cvar_t sv_clmovement_inputtimeout;
extern cvar_t sv_clmovement_maxnetfps;
extern cvar_t sv_cullentities_nevercullbmodels;
extern cvar_t sv_clmovement_soundreliable;
extern cvar_t sv_cullentities_pvs;
extern cvar_t sv_cullentities_stats;
extern cvar_t sv_cullentities_trace;
extern cvar_t sv_cullentities_trace_delay;
extern cvar_t sv_cullentities_trace_enlarge;
extern cvar_t sv_cullentities_trace_prediction;
extern cvar_t sv_cullentities_trace_samples;
extern cvar_t sv_cullentities_trace_samples_extra;
extern cvar_t sv_debugmove;
extern cvar_t sv_echobprint;
extern cvar_t sv_edgefriction;
extern cvar_t sv_entpatch;
extern cvar_t sv_freezenonclients;
extern cvar_t sv_friction;
extern cvar_t sv_gameplayfix_blowupfallenzombies;
extern cvar_t sv_gameplayfix_consistentplayerprethink;
extern cvar_t sv_gameplayfix_delayprojectiles;
extern cvar_t sv_gameplayfix_droptofloorstartsolid;
extern cvar_t sv_gameplayfix_droptofloorstartsolid_nudgetocorrect;
extern cvar_t sv_gameplayfix_easierwaterjump;
extern cvar_t sv_gameplayfix_findradiusdistancetobox;
extern cvar_t sv_gameplayfix_gravityunaffectedbyticrate;
extern cvar_t sv_gameplayfix_grenadebouncedownslopes;
extern cvar_t sv_gameplayfix_multiplethinksperframe;
extern cvar_t sv_gameplayfix_noairborncorpse;
extern cvar_t sv_gameplayfix_noairborncorpse_allowsuspendeditems;
extern cvar_t sv_gameplayfix_nudgeoutofsolid;
extern cvar_t sv_gameplayfix_nudgeoutofsolid_separation;
extern cvar_t sv_gameplayfix_q2airaccelerate;
extern cvar_t sv_gameplayfix_nogravityonground;
extern cvar_t sv_gameplayfix_setmodelrealbox;
extern cvar_t sv_gameplayfix_slidemoveprojectiles;
extern cvar_t sv_gameplayfix_fiendjumpfix;
extern cvar_t sv_gameplayfix_nosquashentities;

extern cvar_t sv_gameplayfix_monsterinterpolate;

extern cvar_t sv_gameplayfix_stepdown;
extern cvar_t sv_gameplayfix_stepmultipletimes;
extern cvar_t sv_gameplayfix_nostepmoveonsteepslopes;
extern cvar_t sv_gameplayfix_swiminbmodels;
extern cvar_t sv_gameplayfix_upwardvelocityclearsongroundflag;
extern cvar_t sv_gameplayfix_downtracesupportsongroundflag;
extern cvar_t sv_gameplayfix_q1bsptracelinereportstexture;
extern cvar_t sv_gameplayfix_unstickplayers;
extern cvar_t sv_gameplayfix_unstickentities;
extern cvar_t sv_gameplayfix_fixedcheckwatertransition;
extern cvar_t sv_gravity;
extern cvar_t sv_idealpitchscale;
extern cvar_t sv_jumpstep;
extern cvar_t sv_jumpvelocity;
extern cvar_t sv_legacy_bbox_expand;
extern cvar_t sv_maxairspeed;
extern cvar_t sv_maxrate;
extern cvar_t sv_maxspeed;
extern cvar_t sv_maxvelocity;
extern cvar_t sv_nostep;
extern cvar_t sv_playerphysicsqc;
extern cvar_t sv_progs;
extern cvar_t sv_protocolname;
extern cvar_t sv_random_seed;
extern cvar_t host_limitlocal;
extern cvar_t sv_pext;
extern cvar_t sv_sound_land;
extern cvar_t sv_sound_watersplash;
extern cvar_t sv_precache_suppress_warning;

extern cvar_t sv_stepheight;
extern cvar_t sv_stopspeed;
extern cvar_t sv_wallfriction;
extern cvar_t sv_wateraccelerate;
extern cvar_t sv_waterfriction;
extern cvar_t sv_areadebug;
extern cvar_t sys_ticrate;
extern cvar_t teamplay;
extern cvar_t temp1;
extern cvar_t timelimit;

#if 0
extern cvar_t sv_gameplayfix_hemebond_pushmove;
#endif

extern mempool_t *sv_mempool;

/// persistant server info
extern server_static_t svs;
/// local server
extern server_t sv;

extern stringlist_t sv_intermap_siv_list;  // /.siv string list
// 0: start
// 1: data blob zipped
// 2: e1m1
// 3: data blob zipped

// Baker: sv_siv_virtual_files_list
// 1. Allocated when?		.SIV		It is not allocated.  It must be freed. When we write a .siv

// map is always a new game, changelevel is always a continued game.
// 2. Cleared when?						// "map" because that is a new game.
										// "changelevel" because preservation is killed
										// "loadgame" can stomp it.
										// "restart" should reload it, although ...
										// "changelevel2" updates it
										// "save" saves it

WARP_X_ (SV_Changelevel_f, SV_Changelevel2_f, SV_Map_f, SV_Restart_f, SV_Loadgame_f)

#define SIV_DECOMPRESS_BUFSIZE_16_MB (16384 * 1024)

extern stringlist_t sv_sivs;
extern client_t *host_client;

//===========================================================

void SV_InitOnce (void);
double SV_Frame(double time);
void SV_Shutdown(void);

int SV_IsLocalServer(void);

void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count);
void SV_StartEffect (vec3_t org, int modelindex, int startframe, int framecount, int framerate);
void SV_StartSound (prvm_edict_t *entity, int channel, const char *sample, int volume, float attenuation, qbool reliable, float speed);
void SV_StartPointSound (vec3_t origin, const char *sample, int volume, float attenuation, float speed);

void SV_ConnectClient (int clientnum, netconn_t *netconnection);
void SV_DropClient (qbool leaving, const char *reason, ... );

void SV_ClientCommandsf(const char *fmt, ...) DP_FUNC_PRINTF(1);

void SV_SendClientMessages(void);

void SV_ReadClientMessage(void);

// precachemode values:
// 0 = fail if not precached,
// 1 = warn if not found and precache if possible
// 2 = precache

#define PRECACHE_MODE_0				0	// fail if not precached Baker: 0 means we are not precaching
#define PRECACHE_MODE_1				1	// warn if not found and precache if possible
#define PRECACHE_MODEL_CMD_MODE_2	2	// precache_model

int SV_ModelIndex(const char *s, int precachemode);
int SV_SoundIndex(const char *s, int precachemode);

int SV_SoundIndex_Count(void);  // Baker r9067: loadgame precaches "precache at any time models and sounds"
int SV_ModelIndex_Count(void);  // Baker r9067: loadgame precaches "precache at any time models and sounds"

int SV_ParticleEffectIndex(const char *name);

model_t *SV_GetModelByIndex(int modelindex);
model_t *SV_GetModelFromEdict(prvm_edict_t *ed);

void SV_SetIdealPitch (void);

void SV_AddUpdates (void);

void SV_PlayerPhysics (void);

void SV_ClientPrint(const char *msg);
void SV_ClientPrintf(const char *fmt, ...) DP_FUNC_PRINTF(1);
void SV_BroadcastPrint(const char *msg);
void SV_BroadcastPrintf(const char *fmt, ...) DP_FUNC_PRINTF(1);

void SV_Physics (void);
void SV_Physics_ClientMove (void);
//void SV_Physics_ClientEntity (prvm_edict_t *ent);

qbool SV_PlayerCheckGround (prvm_edict_t *ent);
qbool SV_CheckBottom (prvm_edict_t *ent);
qbool SV_movestep (prvm_edict_t *ent, vec3_t move, qbool relink, qbool noenemy, qbool settrace);

/*! Needs to be called any time an entity changes origin, mins, maxs, or solid
 * sets ent->v.absmin and ent->v.absmax
 * call TouchAreaGrid as well to fire triggers that overlap the box
 */
void SV_LinkEdict(prvm_edict_t *ent);
void SV_LinkEdict_TouchAreaGrid(prvm_edict_t *ent);
void SV_LinkEdict_TouchAreaGrid_Call(prvm_edict_t *touch, prvm_edict_t *ent); // if we detected a touch from another source

/*! move an entity that is stuck by small amounts in various directions to try to nudge it back into the collision hull
 * returns true if it found a better place
 */
qbool SV_UnstickEntity (prvm_edict_t *ent);
#if 111 // Baker r0061: Classic DarkPlaces physics
/*! move an entity that is stuck out of the surface it is stuck in (can move large amounts)
 * returns true if it found a better place
 */
qbool SV_NudgeOutOfSolid(prvm_edict_t *ent);
#endif

/// calculates hitsupercontentsmask for a generic qc entity
int SV_GenericHitSuperContentsMask(const prvm_edict_t *edict);
/// traces a box move against worldmodel and all entities in the specified area
trace_t SV_TraceBox(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend);
trace_t SV_TraceLine(const vec3_t start, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend);
trace_t SV_TracePoint(const vec3_t start, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
int SV_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int maxedicts, prvm_edict_t **resultedicts);

qbool SV_CanSeeBox(int numsamples, vec_t eyejitter, vec_t enlarge, vec_t entboxexpand, vec3_t eye, vec3_t entboxmins, vec3_t entboxmaxs);

void SV_MarkWriteEntityStateToClient(entity_state_t *s, client_t *client);

void SV_SendServerinfo(client_t *client);
void SV_WriteEntitiesToClient(client_t *client, prvm_edict_t *clent, sizebuf_t *msg, int maxsize);
void SV_AddCameraEyes(void);

int SV_PointSuperContents(const vec3_t point);

void SV_FlushBroadcastMessages(void);
void SV_WriteClientdataToMessage (client_t *client, prvm_edict_t *ent, sizebuf_t *msg, int *stats);

void VM_SV_MoveToGoal(prvm_prog_t *prog);

void SV_PlayerPhysics_ApplyClientMove (void);
void SV_SaveSpawnparms (void);

//void SV_SpawnServer (const char *mapshortname, const char *s_loadgame, const char *s_startspot);  // Baker r9067: loadgame precaches "precache at any time models and sounds"

void SV_SpawnServer (const char *mapshortname, const char *s_loadgame, const char *s_startspot, const vec3_t startorigin, float totaltimeatstart0);
qbool SV_SpawnServer_Intermap_SIV_Is_Ok (const char *mapshortname, const char *s_loadgame, const char *s_startspot, const vec3_t startorigin, float totaltimeatstart0);

void SV_CheckVelocity (prvm_edict_t *ent);

void SV_SetupVM(void);

const char *SV_TimingReport(char *buf, size_t buflen); ///< for output in SV_Status_f

int SV_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent);
void SV_GetEntityMatrix(prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, qbool viewmatrix);

void SV_StartThread(void);
void SV_StopThread(void);
#define SV_LockThreadMutex() (void)(svs.threaded ? Thread_LockMutex(svs.threadmutex) : 0)
#define SV_UnlockThreadMutex() (void)(svs.threaded ? Thread_UnlockMutex(svs.threadmutex) : 0)

void VM_CustomStats_Clear(void);
void VM_SV_UpdateCustomStats(client_t *client, prvm_edict_t *ent, sizebuf_t *msg, int *stats);
void SV_Name(int clientnum);
void SV_InitOperatorCommands(void);

void SV_Loadgame_from (cmd_state_t *cmd, const char *s_loadgame, int is_intermap);
void SV_Savegame_to (prvm_prog_t *prog, char **p_string, const char *name, int is_intermap_siv_write, float totaltimeatlastexit_to_write);

void SV_Savegame_f(cmd_state_t *cmd);
void SV_Loadgame_f(cmd_state_t *cmd);
void SV_Siv_f     (cmd_state_t *cmd); // Data on intermap .siv files, entity state saves stored

void SV_PreSpawn_f(cmd_state_t *cmd);
void SV_Spawn_f(cmd_state_t *cmd);
void SV_Begin_f(cmd_state_t *cmd);

qbool SV_VM_ConsoleCommand (const char *text);

void SV_UpdateToReliableMessages_Zircon_Warp_Think (void);
void SV_PhysicsX_Zircon_Warp_Start (client_t *hcl, const char *s_reason);
void SV_Begin_f_Zircon_Warp_Initialize (void);
#define ZMOVE_SV_DENIED (host_client->zircon_warp_sequence || host_client->zircon_warp_sequence_cleared || host_client->movesequence == 0) 

extern cvar_t sv_allow_zircon_move;
extern cvar_t sv_players_walk_thru_players;
extern cvar_t sv_save_screenshots;
extern cvar_t halflifebsp, sv_mapformat_is_quake2,  sv_mapformat_is_quake3;
extern cvar_t _last;

extern cvar_t sv_autosave;

//extern cvar_t sv_gameplayfix_swimflag_collides_liquids;

void VMX_SV_precache_model (prvm_prog_t *prog, const char *s_model);
void VMX_SV_setmodel (prvm_prog_t *prog, prvm_edict_t *e, const char *s_model);
int SV_Spawn_Model_At (ccs *s_classname, ccs *s_modelname, ccs *s_origin, ccs *s_angles, ccs *s_frame, ccs *s_scale);

#define	SAVEGAME_VERSION_5	5
int SV_Loadgame_Intermap_Do_Ents (const char *s_load_game_contents);
#endif // ! SERVER_H
