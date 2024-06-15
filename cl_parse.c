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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "cdaudio.h"
#include "cl_collision.h"
#include "csprogs.h"
#include "libcurl.h"
#include "utf8lib.h"
#ifdef CONFIG_MENU
#include "menu.h"
#endif
#include "cl_video.h"
#include "float.h"

const char *svc_strings[128] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",	// (DP8) [string] null terminated parting message
	"svc_updatestat",
	"svc_version",		// [int] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value

	"svc_serverinfo",		// [int] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from

	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene",
	"svc_showlmp",	// [string] iconlabel [string] lmpfile [short] x [short] y
	"svc_hidelmp",	// [string] iconlabel
	"svc_skybox", // [string] skyname
	"", // 38
	"", // 39
	"", // 40
	"", // 41
	"", // 42
	"", // 43
	"", // 44
	"", // 45
	"", // 46
	"", // 47
	"", // 48
	"", // 49
	"svc_downloaddata", //				50		// [int] start [short] size [variable length] data
	"svc_updatestatubyte", //			51		// [byte] stat [byte] value
	"svc_effect", //			52		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
	"svc_effect2", //			53		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
	"svc_sound2", //			54		// short soundindex instead of byte
	"svc_spawnbaseline2", //	55		// short modelindex instead of byte
	"svc_spawnstatic2", //		56		// short modelindex instead of byte
	"svc_entities", //			57		// [int] deltaframe [int] thisframe [float vector] eye [variable length] entitydata
	"svc_csqcentities", //		58		// [short] entnum [variable length] entitydata ... [short] 0x0000
	"svc_spawnstaticsound2", //	59		// [coord3] [short] samp [byte] vol [byte] aten
	"svc_trailparticles", //	60		// [short] entnum [short] effectnum [vector] start [vector] end
	"svc_pointparticles", //	61		// [short] effectnum [vector] start [vector] velocity [short] count
	"svc_pointparticles1", //	62		// [short] effectnum [vector] start, same as svc_pointparticles except velocity is zero and count is 1
	"",						// 63 
	"",						// 64
	"",						// 65 
	"",						// 66
	"",						// 67 
	"",						// 68
	"",						// 69 
	"svc_zircon_warp",		// 70
};

const char *qw_svc_strings[128] =
{
	"qw_svc_bad",					// 0
	"qw_svc_nop",					// 1
	"qw_svc_disconnect",			// 2
	"qw_svc_updatestat",			// 3	// [byte] [byte]
	"",								// 4
	"qw_svc_setview",				// 5	// [short] entity number
	"qw_svc_sound",					// 6	// <see code>
	"",								// 7
	"qw_svc_print",					// 8	// [byte] id [string] null terminated string
	"qw_svc_stufftext",				// 9	// [string] stuffed into client's console buffer
	"qw_svc_setangle",				// 10	// [angle3] set the view angle to this absolute value
	"qw_svc_serverdata",			// 11	// [long] protocol ...
	"qw_svc_lightstyle",			// 12	// [byte] [string]
	"",								// 13
	"qw_svc_updatefrags",			// 14	// [byte] [short]
	"",								// 15
	"qw_svc_stopsound",				// 16	// <see code>
	"",								// 17
	"",								// 18
	"qw_svc_damage",				// 19
	"qw_svc_spawnstatic",			// 20
	"",								// 21
	"qw_svc_spawnbaseline",			// 22
	"qw_svc_temp_entity",			// 23	// variable
	"qw_svc_setpause",				// 24	// [byte] on / off
	"",								// 25
	"qw_svc_centerprint",			// 26	// [string] to put in center of the screen
	"qw_svc_killedmonster",			// 27
	"qw_svc_foundsecret",			// 28
	"qw_svc_spawnstaticsound",		// 29	// [coord3] [byte] samp [byte] vol [byte] aten
	"qw_svc_intermission",			// 30		// [vec3_t] origin [vec3_t] angle
	"qw_svc_finale",				// 31		// [string] text
	"qw_svc_cdtrack",				// 32		// [byte] track
	"qw_svc_sellscreen",			// 33
	"qw_svc_smallkick",				// 34		// set client punchangle to 2
	"qw_svc_bigkick",				// 35		// set client punchangle to 4
	"qw_svc_updateping",			// 36		// [byte] [short]
	"qw_svc_updateentertime",		// 37		// [byte] [float]
	"qw_svc_updatestatlong",		// 38		// [byte] [long]
	"qw_svc_muzzleflash",			// 39		// [short] entity
	"qw_svc_updateuserinfo",		// 40		// [byte] slot [long] uid
	"qw_svc_download",				// 41		// [short] size [size bytes]
	"qw_svc_playerinfo",			// 42		// variable
	"qw_svc_nails",					// 43		// [byte] num [48 bits] xyzpy 12 12 12 4 8
	"qw_svc_chokecount",			// 44		// [byte] packets choked
	"qw_svc_modellist",				// 45		// [strings]
	"qw_svc_soundlist",				// 46		// [strings]
	"qw_svc_packetentities",		// 47		// [...]
	"qw_svc_deltapacketentities",	// 48		// [...]
	"qw_svc_maxspeed",				// 49		// maxspeed change, for prediction
	"qw_svc_entgravity",			// 50		// gravity change, for prediction
	"qw_svc_setinfo",				// 51		// setinfo on a client
	"qw_svc_serverinfo",			// 52		// serverinfo
	"qw_svc_updatepl",				// 53		// [byte] [byte]
};

//=============================================================================

cvar_t cl_worldmessage = {CF_CLIENT | CF_READONLY, "cl_worldmessage", "", "title of current level"};
cvar_t cl_worldname = {CF_CLIENT | CF_READONLY, "cl_worldname", "", "name of current worldmodel"};
cvar_t cl_worldnamenoextension = {CF_CLIENT | CF_READONLY, "cl_worldnamenoextension", "", "name of current worldmodel without extension"};
cvar_t cl_worldbasename = {CF_CLIENT | CF_READONLY, "cl_worldbasename", "", "name of current worldmodel without maps/ prefix or extension"};

cvar_t developer_networkentities = {CF_CLIENT, "developer_networkentities", "0", "prints received entities, value is 0-10 (higher for more info, 10 being the most verbose)"};
cvar_t cl_gameplayfix_soundsmovewithentities = {CF_CLIENT, "cl_gameplayfix_soundsmovewithentities", "1", "causes sounds made by lifts, players, projectiles, and any other entities, to move with the entity, so for example a rocket noise follows the rocket rather than staying at the starting position"};
cvar_t cl_sound_wizardhit = {CF_CLIENT, "cl_sound_wizardhit", "wizard/hit.wav", "sound to play during TE_WIZSPIKE (empty cvar disables sound)"};
cvar_t cl_sound_hknighthit = {CF_CLIENT, "cl_sound_hknighthit", "hknight/hit.wav", "sound to play during TE_KNIGHTSPIKE (empty cvar disables sound)"};
cvar_t cl_sound_tink1 = {CF_CLIENT, "cl_sound_tink1", "weapons/tink1.wav", "sound to play with 80% chance during TE_SPIKE/TE_SUPERSPIKE (empty cvar disables sound)"};
cvar_t cl_sound_ric1 = {CF_CLIENT, "cl_sound_ric1", "weapons/ric1.wav", "sound to play with 5% chance during TE_SPIKE/TE_SUPERSPIKE (empty cvar disables sound)"};
cvar_t cl_sound_ric2 = {CF_CLIENT, "cl_sound_ric2", "weapons/ric2.wav", "sound to play with 5% chance during TE_SPIKE/TE_SUPERSPIKE (empty cvar disables sound)"};
cvar_t cl_sound_ric3 = {CF_CLIENT, "cl_sound_ric3", "weapons/ric3.wav", "sound to play with 10% chance during TE_SPIKE/TE_SUPERSPIKE (empty cvar disables sound)"};
cvar_t cl_readpicture_force = {CF_CLIENT, "cl_readpicture_force", "0", "when enabled, the low quality pictures read by ReadPicture() are preferred over the high quality pictures on the file system"};

#define RIC_GUNSHOT		1
#define RIC_GUNSHOTQUAD	2
cvar_t cl_sound_ric_gunshot = {CF_CLIENT, "cl_sound_ric_gunshot", "0", "specifies if and when the related cl_sound_ric and cl_sound_tink sounds apply to TE_GUNSHOT/TE_GUNSHOTQUAD, 0 = no sound, 1 = TE_GUNSHOT, 2 = TE_GUNSHOTQUAD, 3 = TE_GUNSHOT and TE_GUNSHOTQUAD"};
cvar_t cl_sound_r_exp3 = {CF_CLIENT, "cl_sound_r_exp3", "weapons/r_exp3.wav", "sound to play during TE_EXPLOSION and related effects (empty cvar disables sound)"};
cvar_t snd_cdautopause = {CF_CLIENT | CF_ARCHIVE, "snd_cdautopause", "1", "pause the CD track while the game is paused"};

cvar_t cl_serverextension_download = {CF_CLIENT, "cl_serverextension_download", "0", "indicates whether the server supports the download command.  Baker: a DarkPlaces server sends cl_serverextension_download 2 to clients"};
cvar_t cl_joinbeforedownloadsfinish = {CF_CLIENT | CF_ARCHIVE, "cl_joinbeforedownloadsfinish", "1", "if non-zero the game will begin after the map is loaded before other downloads finish"};
cvar_t cl_nettimesyncfactor = {CF_CLIENT | CF_ARCHIVE, "cl_nettimesyncfactor", "0", "rate at which client time adapts to match server time, 1 = instantly, 0.125 = slowly, 0 = not at all (only applied in bound modes 0, 1, 2, 3)"};
cvar_t cl_nettimesyncboundmode = {CF_CLIENT | CF_ARCHIVE, "cl_nettimesyncboundmode", "6", "method of restricting client time to valid values, 0 = no correction, 1 = tight bounding (jerky with packet loss), 2 = loose bounding (corrects it if out of bounds), 3 = leniant bounding (ignores temporary errors due to varying framerate), 4 = slow adjustment method from Quake3, 5 = slightly nicer version of Quake3 method, 6 = tight bounding + mode 5, 7 = jitter compensated dynamic adjustment rate"};
cvar_t cl_nettimesyncboundtolerance = {CF_CLIENT | CF_ARCHIVE, "cl_nettimesyncboundtolerance", "0.25", "how much error is tolerated by bounding check, as a fraction of frametime, 0.25 = up to 25% margin of error tolerated, 1 = use only new time, 0 = use only old time (same effect as setting cl_nettimesyncfactor to 1) (only affects bound modes 2 and 3)"};
cvar_t cl_iplog_name = {CF_CLIENT | CF_ARCHIVE, "cl_iplog_name", "darkplaces_iplog.txt", "name of iplog file containing player addresses for iplog_list command and automatic ip logging when parsing status command"};

cvar_t cl_pext = {CF_CLIENT | CF_PERSISTENT, "cl_pext", "1", "allow/disallow protocol extensions [Zircon]"};
//cvar_t cl_pext_qw_coloredtext = {CF_CLIENT | CF_PERSISTENT, "cl_pext_qw_coloredtext", "1", "Zircon will look for &c and &r color codes in Quakeworld qw_svc_print and translate [Zircon]"};
cvar_t cl_pext_qw_256packetentities = {CF_CLIENT | CF_PERSISTENT, "cl_pext_qw_256packetentities", "1", "Specifies that the client suppports FTE 256 visible entities, above the Quakeworld vanilla limit of 64 [Zircon]"};
cvar_t cl_pext_qw_limits = {CF_CLIENT | CF_PERSISTENT, "cl_pext_qw_limits", "1", "Large entity limits above the Quakeworld vanilla limit of 512 [Zircon]"};
cvar_t cl_pext_chunkeddownloads = {CF_CLIENT | CF_PERSISTENT, "cl_pext_chunkeddownloads", "1", "Specifies that the client supports chunked downloads [Zircon]"};
cvar_t cl_chunksperframe = {CF_CLIENT | CF_PERSISTENT, "cl_chunksperframe", "30", "Chunks per frame for FTE chunked download [Zircon]"};
//cvar_t cl_pext_qw_limits = {CF_CLIENT, "cl_pext_qw_limits", "1", "FTE enable Quakeworld enhanced protocol limits [Zircon]"};

static qbool QW_CL_CheckOrDownloadFile(const char *filename);
static void QW_CL_RequestNextDownload(void);
static void QW_CL_NextUpload_f(cmd_state_t *cmd);
//static qbool QW_CL_IsUploading(void);
static void QW_CL_StopUpload_f(cmd_state_t *cmd);

/*
==================
CL_ParseStartSoundPacket
==================
*/
static void CL_ParseStartSoundPacket(int is_largesoundindex)
{
	vec3_t  pos;
	int 	channel, ent;
	int 	sound_num;
	int 	nvolume;
	int 	field_mask;
	float 	attenuation;
	float	speed;
	int		fflags = CHANNELFLAG_NONE;

	if (cls.protocol == PROTOCOL_QUAKEWORLD) {
		channel = MSG_ReadShort(&cl_message);

		if (channel & (1<<15))
			nvolume = MSG_ReadByte(&cl_message);
		else
			nvolume = DEFAULT_SOUND_PACKET_VOLUME;

		if (channel & (1<<14))
			attenuation = MSG_ReadByte(&cl_message) / 64.0;
		else
			attenuation = DEFAULT_SOUND_PACKET_ATTENUATION_1_0;

		speed = 1.0f;

		ent = (channel>>3)&1023;
		channel &= 7;

		sound_num = MSG_ReadByte(&cl_message);
	} else {
		field_mask = MSG_ReadByte(&cl_message);

		if (Have_Flag (field_mask, SND_VOLUME_1))
			nvolume = MSG_ReadByte(&cl_message);
		else
			nvolume = DEFAULT_SOUND_PACKET_VOLUME;

		// PROTOCOL_FITZQUAKE looks ok
		if (Have_Flag (field_mask, SND_ATTENUATION_2))
			attenuation = MSG_ReadByte(&cl_message) / 64.0;
		else
			attenuation = DEFAULT_SOUND_PACKET_ATTENUATION_1_0;

		// PROTOCOL_FITZQUAKE looks ok
		if (Have_Flag (field_mask, SND_SPEEDUSHORT4000_32))
			speed = ((unsigned short)MSG_ReadShort(&cl_message)) / 4000.0f;
		else
			speed = 1.0f;

		// PROTOCOL_FITZQUAKE looks ok
		if (Have_Flag (field_mask, SND_LARGEENTITY_8))
		{
			ent = (unsigned short) MSG_ReadShort(&cl_message);
			channel = MSG_ReadChar(&cl_message);
		}
		else
		{
			channel = (unsigned short) MSG_ReadShort(&cl_message);
			ent = channel >> 3;
			channel &= 7;
		}

		if (is_largesoundindex || Have_Flag(field_mask, SND_LARGESOUND_16) ||
			isin2 (cls.protocol, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3))
			sound_num = (unsigned short) MSG_ReadShort(&cl_message);
		else
			sound_num = MSG_ReadByte(&cl_message);

		//if (isin2(cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		//	if (Have_Flag (field_mask, SND_LARGESOUND_16))
		//		sound_num = (unsigned short) MSG_ReadShort ();
		//	else
		//		sound_num = MSG_ReadByte ();
		//	//johnfitz

		//	//johnfitz -- check soundnum
		//	//johnfitz
		//	// Readcoord
		//}

		if (isin2(cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999))
			if (sound_num >= FITZQUAKE_MAX_SOUNDS_2048)
				Host_Error_Line ("CL_ParseStartSoundPacket: %d > FITZQUAKE_MAX_SOUNDS_2048", sound_num);

	} // if ! Quakeworld

	MSG_ReadVector(&cl_message, pos, cls.protocol);

	if (sound_num < 0 || sound_num >= MAX_SOUNDS_4096) {
		Con_PrintLinef ("CL_ParseStartSoundPacket: sound_num (%d) >= MAX_SOUNDS_4096 (%d)", sound_num, MAX_SOUNDS_4096);
		return;
	}

	if (ent >= MAX_EDICTS_32768) {
		Con_PrintLinef ("CL_ParseStartSoundPacket: ent = %d", ent);
		return;
	}

	if (ent >= cl.max_entities)
		CL_ExpandEntities(ent);

	if ( !CL_VM_Event_Sound(sound_num, nvolume / 255.0f, channel, attenuation, ent, pos, fflags, speed) )
		S_StartSound_StartPosition_Flags (ent, channel, cl.sound_precache[sound_num], pos, nvolume/255.0f, attenuation, 0, fflags, speed);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/

static unsigned char olddata[NET_MAXMESSAGE_65536];
WARP_X_CALLERS_ (All over the place even model load plus texture load)
void CL_KeepaliveMessage (qbool readmessages)
{
	static double lastdirtytime = 0;
	static qbool recursive = false;
	double dirtytime;
	double deltatime;
	static double countdownmsg = 0;
	static double countdownupdate = 0;
	sizebuf_t old;

	qbool thisrecursive;

	thisrecursive = recursive;
	recursive = true;

	dirtytime = Sys_DirtyTime();
	deltatime = dirtytime - lastdirtytime;
	lastdirtytime = dirtytime;
	if (deltatime <= 0 || deltatime >= 1800.0)
		return;

	countdownmsg -= deltatime;
	countdownupdate -= deltatime;

	if (!thisrecursive) {
		if (cls.state != ca_dedicated) {
			if (countdownupdate <= 0) { // check if time stepped backwards
				countdownupdate = 2;
			}
		}
	}

	// no need if server is local and definitely not if this is a demo
	if (sv.active || !cls.netcon || cls.protocol == PROTOCOL_QUAKEWORLD || cls.signon >= SIGNONS_4)
	{
		recursive = thisrecursive;
		return;
	}

	if (readmessages)
	{
		// read messages from server, should just be nops
		old = cl_message;
		memcpy(olddata, cl_message.data, cl_message.cursize);

		NetConn_ClientFrame();

		cl_message = old;
		memcpy(cl_message.data, olddata, cl_message.cursize);
	}

	if (cls.netcon && countdownmsg <= 0) { // check if time stepped backwards
		sizebuf_t	msg;
		unsigned char		buf[4];
		countdownmsg = 5;
		// write out a nop
		// LadyHavoc: must use unreliable because reliable could kill the sigon message!
		Con_PrintLinef ("--> client to server keepalive");
		memset(&msg, 0, sizeof(msg));
		msg.data = buf;
		msg.maxsize = sizeof(buf);
		MSG_WriteChar(&msg, clc_nop);
		NetConn_SendUnreliableMessage(cls.netcon, &msg, cls.protocol, q_net_rate_10000, q_net_burstrate_0, q_net_suppress_reliables_false);
	}

	recursive = thisrecursive;
}

void CL_ParseEntityLump(char *entdata)
{
	qbool loadedsky = false;
	const char *data;
	char key[128], value[MAX_INPUTLINE_16384];
	FOG_clear(); // LadyHavoc: no fog until set
	// LadyHavoc: default to the map's sky (q3 shader parsing sets this)
	R_SetSkyBox(cl.worldmodel->brush.skybox);
	data = entdata;
	if (!data)
		return;
	if (!COM_ParseToken_Simple(&data, false, false, true))
		return; // error
	if (com_token[0] != '{')
		return; // error
	while (1)
	{
		if (!COM_ParseToken_Simple(&data, false, false, true))
			return; // error
		if (com_token[0] == '}')
			break; // end of worldspawn
		if (com_token[0] == '_')
			c_strlcpy (key, com_token + 1);
		else
			strlcpy (key, com_token, sizeof (key));
		while (key[strlen(key)-1] == ' ') // remove trailing spaces
			key[strlen(key)-1] = 0;
		if (!COM_ParseToken_Simple(&data, false, false, true))
			return; // error
		c_strlcpy (value, com_token);
		if (String_Match("sky", key)) {
			loadedsky = true;
			R_SetSkyBox(value);
		}
		else if (String_Match("skyname", key)) { // non-standard, introduced by QuakeForge... sigh.
			loadedsky = true;
			R_SetSkyBox(value);
		}
		else if (String_Match("qlsky", key)) { // non-standard, introduced by QuakeLives (EEK)
			loadedsky = true;
			R_SetSkyBox(value);
		}
		else if (String_Match("fog", key)) {
			FOG_clear(); // so missing values get good defaults
			r_refdef.fog_start = 0;
			r_refdef.fog_alpha = 1;
			r_refdef.fog_end = 16384;
			r_refdef.fog_height = 1<<30;
			r_refdef.fog_fadedepth = 128;
#if _MSC_VER >= 1400
	#define sscanf sscanf_s
#endif
			// Baker r1201: FitzQuake r_skyfog
			r_refdef.fog_alpha = 0;

			sscanf(value, "%f %f %f %f %f %f %f %f %f",
				&r_refdef.fog_density0,
				&r_refdef.fog_red,
				&r_refdef.fog_green,
				&r_refdef.fog_blue,
				&r_refdef.fog_alpha,
				&r_refdef.fog_start,
				&r_refdef.fog_end,
				&r_refdef.fog_height,
				&r_refdef.fog_fadedepth); // Baker r1201: FitzQuake r_skyfog
			if (r_refdef.fog_alpha == 0 && r_skyfog.value) {
				float a = bound(0.0, r_skyfog.value, 1.0);
				r_refdef.fog_density = r_refdef.fog_density0 / (a * a);
				r_refdef.fog_alpha = a;
			} else {
				r_refdef.fog_density = r_refdef.fog_density0;
			}
		}
		else if (String_Match("fog_density", key))
			r_refdef.fog_density = atof(value);
		else if (String_Match("fog_red", key))
			r_refdef.fog_red = atof(value);
		else if (String_Match("fog_green", key))
			r_refdef.fog_green = atof(value);
		else if (String_Match("fog_blue", key))
			r_refdef.fog_blue = atof(value);
		else if (String_Match("fog_alpha", key))
			r_refdef.fog_alpha = atof(value);
		else if (String_Match("fog_start", key))
			r_refdef.fog_start = atof(value);
		else if (String_Match("fog_end", key))
			r_refdef.fog_end = atof(value);
		else if (String_Match("fog_height", key))
			r_refdef.fog_height = atof(value);
		else if (String_Match("fog_fadedepth", key))
			r_refdef.fog_fadedepth = atof(value);
		else if (String_Match("fog_heighttexture", key))
		{
			FOG_clear(); // so missing values get good defaults
#if _MSC_VER >= 1400
			sscanf_s(value, "%f %f %f %f %f %f %f %f %f %s", &r_refdef.fog_density0, &r_refdef.fog_red, &r_refdef.fog_green, &r_refdef.fog_blue, &r_refdef.fog_alpha, &r_refdef.fog_start, &r_refdef.fog_end, &r_refdef.fog_height, &r_refdef.fog_fadedepth, r_refdef.fog_height_texturename, (unsigned int)sizeof(r_refdef.fog_height_texturename)); // Baker r1201: FitzQuake r_skyfog
#else
			sscanf(value, "%f %f %f %f %f %f %f %f %f %63s", &r_refdef.fog_density0, &r_refdef.fog_red, &r_refdef.fog_green, &r_refdef.fog_blue, &r_refdef.fog_alpha, &r_refdef.fog_start, &r_refdef.fog_end, &r_refdef.fog_height, &r_refdef.fog_fadedepth, r_refdef.fog_height_texturename); // Baker r1201: FitzQuake r_skyfog
#endif

			r_refdef.fog_height_texturename[63] = 0;
		}
	}

	if (!loadedsky && cl.worldmodel->brush.isq2bsp)
		R_SetSkyBox("unit1_");
}

extern cvar_t con_chatsound_team_file;
static const vec3_t defaultmins = {-4096, -4096, -4096};
static const vec3_t defaultmaxs = {4096, 4096, 4096};

WARP_X_CALLERS_ (CL_BeginDownloads_DP QW_CL_RequestNextDownload)
static void CL_SetupWorldModel(void)
{
	prvm_prog_t *prog = CLVM_prog;

#if 1
	WARP_X_ (CL_RESETNEWMAP_0)

	extern cvar_t r_lockpvs;
	extern cvar_t r_lockvisibility;

	Cvar_SetValueQuick	(&r_lockpvs, 0);			// Baker: June 6 2024, this was crashing us.
	Cvar_SetValueQuick	(&r_lockvisibility, 0);		// Baker: June 6 2024, I bet this is nearly the same
	Cvar_SetValueQuick	(&tool_inspector, 0);		// Baker r0106: tool inspector
	Cvar_SetValueQuick	(&tool_marker, 0);			// Baker r0109: tool marker
	
	// Baker r7084 - gamecommand clear cl
	extern cvar_t prvm_cl_gamecommands;
	extern cvar_t prvm_cl_progfields;

	Cvar_SetQuick(&prvm_cl_gamecommands, ""); // newmap
	Cvar_SetQuick(&prvm_cl_progfields, ""); // newmap
#endif

	// update the world model
	cl.entities[0].render.model = cl.worldmodel = CL_GetModelByIndex(1);

#if 1 // LEVELSHOTS
	va_super (levelshotname_tga, MAX_QPATHX2_256, "levelshots/%s.tga", cl.worldbasename);
	int have_levelshot = FS_FileExists (levelshotname_tga);
	if (have_levelshot) {
		c_strlcpy (cl.levelshotsname, levelshotname_tga);
	} else {
		va_super (levelshotname_jpg, MAX_QPATHX2_256, "levelshots/%s.jpg", cl.worldbasename);
		have_levelshot = FS_FileExists (levelshotname_jpg);
		if (have_levelshot) {
			c_strlcpy (cl.levelshotsname, levelshotname_jpg);
		}
	}
#endif

	CL_UpdateRenderEntity(&cl.entities[0].render);

	// make sure the cl.worldname and related cvars are set up now that we know the world model name
	// set up csqc world for collision culling
	if (cl.worldmodel) {
		c_strlcpy (cl.worldname, cl.worldmodel->model_name);
		FS_StripExtension(cl.worldname, cl.worldnamenoextension, sizeof(cl.worldnamenoextension));
		c_strlcpy (cl.worldbasename, String_Starts_With_PRE (cl.worldnamenoextension, "maps/") ? 
			cl.worldnamenoextension + 5 : cl.worldnamenoextension);
		Cvar_SetQuick(&cl_worldmessage, cl.worldmessage);
		Cvar_SetQuick(&cl_worldname, cl.worldname);
		Cvar_SetQuick(&cl_worldnamenoextension, cl.worldnamenoextension);
		Cvar_SetQuick(&cl_worldbasename, cl.worldbasename);
		World_SetSize(&cl.world, cl.worldname, cl.worldmodel->normalmins, cl.worldmodel->normalmaxs, prog);
	}
	else
	{
		Cvar_SetQuick(&cl_worldmessage, cl.worldmessage);
		Cvar_SetQuick(&cl_worldnamenoextension, "");
		Cvar_SetQuick(&cl_worldbasename, "");

		World_SetSize(&cl.world, "", defaultmins, defaultmaxs, prog);
	}
	World_Start(&cl.world);

	// load or reload .loc file for team chat messages
	CL_Locs_Reload_f(cmd_local);

	// make sure we send enough keepalives
	CL_KeepaliveMessage(false);

	// reset particles and other per-level things
	R_Modules_NewMap();

	// make sure we send enough keepalives
	CL_KeepaliveMessage(false);

	// load the team chat beep if possible
	cl.foundteamchatsound = FS_FileExists(con_chatsound_team_file.string);

	// check memory integrity
	Mem_CheckSentinelsGlobal();

#ifdef CONFIG_MENU
	// make menu know
	MR_NewMap();
#endif

	// load the csqc now
	if (cl.loadcsqc)
	{
		cl.loadcsqc = false;

		CL_VM_Init();
	}
}

WARP_X_CALLERS_ (QW_CL_RequestNextDownload ezQuake equiv CL_CheckOrDownloadFile)
static qbool QW_CL_CheckOrDownloadFile(const char *filename)
{
	qfile_t *file;

	// see if the file already exists
	file = FS_OpenVirtualFile(filename, fs_quiet_true);
	if (file) {
		FS_Close(file);
		return true;
	}

	// download messages in a demo would be bad
	if (cls.demorecording) {
		Con_PrintLinef ("Unable to download " QUOTED_S " when recording.", filename);
		return true;
	}

	// don't try to download when playing a demo
	if (!cls.netcon)
		return true;

	int was_lit = cls.asking_for_lit == 1;


	if (was_lit == false && String_Ends_With_Caseless (filename, ".bsp")) {
		// We will ask for lit first
		// ezQuake #d0
		// Baker_CL_Download_Start_QW_SetName
		cls.asking_for_lit = 1; // Ask for lit

		cls.qw_downloadname[0] = 0;
		c_strlcpy (cls.qw_downloadname, filename);
		File_URL_Edit_Remove_Extension(cls.qw_downloadname);
		c_strlcat (cls.qw_downloadname, ".lit");
			//filename = cls.qw_downloadname;
		//cls.qw_got_lit = -1;



	//	if (developer_qw.integer)
	//		Con_PrintLinef ("QW_CL_CheckOrDownloadFile: Temp file is %s", cls.qw_downloadtempname);

		cls.qw_downloadmethod = DL_QW_1; // Gets changed later
		cls.qw_downloadstarttime = Sys_DirtyTime();

		Con_PrintLinef ("Downloading %s", cls.qw_downloadname);

		if (!cls.qw_downloadmemory) {
			cls.qw_downloadmemory = NULL;
			cls.qw_downloadmemorycursize = 0;
			cls.qw_downloadmemorymaxsize = 1024*1024; // start out with a 1MB buffer
		}

		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd, "download %s", cls.qw_downloadname);

		//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
		//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "download %s", filename));
		if (developer_qw.integer)
			Con_PrintLinef ("QW_CL_CheckOrDownloadFile: Asking for file " QUOTED_S, cls.qw_downloadname);

		//cls.qw_downloadnumber++;
		cls.qw_downloadpercent = 0;
		cls.qw_downloadmemorycursize = 0;


		return false;

	}

	cls.asking_for_lit = 0;

	// ezQuake #d0
	// Baker_CL_Download_Start_QW_SetName
	cls.qw_downloadname[0] = 0;
	c_strlcpy (cls.qw_downloadname, filename);


//	if (developer_qw.integer)
//		Con_PrintLinef ("QW_CL_CheckOrDownloadFile: Temp file is %s", cls.qw_downloadtempname);

	cls.qw_downloadmethod = DL_QW_1; // Gets changed later
	cls.qw_downloadstarttime = Sys_DirtyTime();

	Con_PrintLinef ("Downloading %s", filename);

	if (!cls.qw_downloadmemory) {
		cls.qw_downloadmemory = NULL;
		cls.qw_downloadmemorycursize = 0;
		cls.qw_downloadmemorymaxsize = 1024*1024; // start out with a 1MB buffer
	}

	Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd, "download %s", filename);

	//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
	//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "download %s", filename));
	if (developer_qw.integer)
		Con_PrintLinef ("QW_CL_CheckOrDownloadFile: Asking for file " QUOTED_S, filename);

	cls.qw_downloadnumber++;
	cls.qw_downloadpercent = 0;
	cls.qw_downloadmemorycursize = 0;


	return false;
}

static void QW_CL_ProcessUserInfo(int slot);

typedef struct _deadster_t_s {
	int	our_qw_monster_id;
	int	dying_low;
	int dying_high;
	int extra_dead_frame1;
	int extra_dead_frame2;
	int extra_dead_frame3;
	int extra_dead_frame4;
} deadster_t;

char *qw_monsters[] = {
	"progs/soldier.mdl",	// 0
	"progs/dog.mdl",		// 1
	"progs/demon.mdl",		// 2
	"progs/ogre.mdl",		// 3
	"progs/shambler.mdl",	// 4
	"progs/knight.mdl",		// 5
	"progs/zombie.mdl",		// 6
	"progs/wizard.mdl",		// 7
	"progs/enforcer.mdl",	// 8
	"progs/fish.mdl",		// 9
	"progs/hknight.mdl",	// 10
	"progs/shalrath.mdl",	// 11
	"progs/tarbaby",		// 12
};


deadster_t deadinfos[] = {
// index					dying0	dying1	extra dead frame
	{ 0,		/*soldier*/		8,		28,		0,		0,	0,	}, // dead frames 17 28
	{ 1,		/*dog*/			8,		25,		0,		0,	0,	}, // dead frames 16 25
	{ 2,		/*fiend*/		45,		53,		0,		0,	0,	}, // dead frames 53
	{ 3,		/*ogre*/		112,	135,	0,		0,	0,	}, // dead frames 125 135
	{ 4,		/*shambler*/	83,		93,		102,	0,	0,	}, // dead frames 84 92 102
	{ 5,		/*knight*/		76,		96,		0,		0,	0,	}, // dead frames 85 96
	{ 6,		/*zombie*/		91,		197,	0,		0,	0,	}, // you have to gib them
	{ 7,		/*scrag*/		46,		53,		0,		0,	0,	}, // dead frames 53
	{ 8,		/*enforcer*/	41,		65,		0,		0,	0,	}, // dead frames 54 65
	{ 9,		/*fish*/		18,		38,		0,		0,	0,	}, // dead frames 38
	{ 10,		/*hknight*/		42,		62,		0,		0,	0,	}, // dead frames 53 62
	{ 11,		/*vore*/		16,		22,		0,		0,	0,	}, // dead frames 22
};

int deadinfos_count = ARRAY_COUNT(deadinfos);

WARP_X_ (QW_CL_FindModelNumbers EntityFrameQW_AllocDatabase right above)

qbool QW_Is_Step_Dead (int qw_monster_id, int frame)
{
	if (in_range (0, qw_monster_id, (int)ARRAY_COUNT(deadinfos)) == false)
		Host_Error_Line ("QW Monster id %d not in range 0 to %d", qw_monster_id, (int)ARRAY_COUNT(deadinfos));

	deadster_t *p_monster = &deadinfos[qw_monster_id];
	if (in_range (p_monster->dying_low, frame, p_monster->dying_high))
		return true; // It's dying or dead

	if (p_monster->extra_dead_frame1 && frame == p_monster->extra_dead_frame1)
		return true; // It's dead

	return false; // It's alive
}

WARP_X_ (QW_Is_Step_ModelIndex, QW_Is_Step_Dead)
void QW_CL_FindModelNumbers (void)
{
	//memset (cl.qw_monsters_modelindexes, 0, sizeof(qw_monsters_modelindexes) );
	cl.qw_monsters_modelindexes_count = 0;

	for (int my_model_idx = 0; my_model_idx < MAX_MODELS_8192; my_model_idx ++)  {
		char *s_this = cl.model_name[my_model_idx];

		for (int monster_idx = 0; monster_idx < (int)ARRAY_COUNT (qw_monsters); monster_idx ++)  {
			char *s_this_monster = qw_monsters[monster_idx];
			if (false == String_Match (s_this, s_this_monster))
				continue;

			// Add to indexes
			cl.qw_monsters_modelindexes[cl.qw_monsters_modelindexes_count] = my_model_idx;
			cl.qw_monsters_modelindexes_qw_monster_id[cl.qw_monsters_modelindexes_count] = monster_idx;
			cl.qw_monsters_modelindexes_count ++;

			if (cl.qw_monsters_modelindexes_count == (int)ARRAY_COUNT(cl.qw_monsters_modelindexes)) {
				// FULL!
				goto filled_it;
			}
		} // monster
	} // model

filled_it:
	; // Baker: obligatory null statement :(
}


static void QW_CL_RequestNextDownload(void)
{
	int j;
	char vabuf[1024];

	if (developer_qw.integer)
		Con_PrintLinef ("QW_CL_RequestNextDownload");

	// clear name of file that just finished
	// Baker_CL_Download_Clear_Name_QW
	cls.qw_downloadname[0] = 0;

	// skip the download fragment if playing a demo
	if (!cls.netcon) {
		return;
	}

	switch (cls.qw_downloadtype) {
	case dl_single:
		break;
	case dl_skin:
		if (cls.qw_downloadnumber == 0)
			Con_PrintLinef ("Checking skins...");
		for (; cls.qw_downloadnumber < cl.maxclients;cls.qw_downloadnumber++) {
			if (!cl.scores[cls.qw_downloadnumber].name[0])
				continue;
			// check if we need to download the file, and return if so
			if (!QW_CL_CheckOrDownloadFile(va(vabuf, sizeof(vabuf), "skins/%s.pcx", cl.scores[cls.qw_downloadnumber].qw_skin)))
				return;
		} // for

		cls.qw_downloadtype = dl_none;

		// load any newly downloaded skins
		for (j = 0;j < cl.maxclients;j++)
			QW_CL_ProcessUserInfo(j);

		// if we're still in signon stages, request the next one
		if (cls.signon != SIGNONS_4) {
			cls.signon = SIGNON_3; // QUAKEWORLD -- SIGNONS_4 - 1;
			// we'll go to SIGNONS_4 when the first entity update is received
			Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd, "begin %d", cl.qw_servercount);
			//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
			//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "begin %d", cl.qw_servercount));
		}
		break;
	case dl_model:
		if (cls.qw_downloadnumber == 0)
		{
			Con_PrintLinef ("Checking models...");
			cls.qw_downloadnumber = 1;
		}

		for (;cls.qw_downloadnumber < MAX_MODELS_8192 && cl.model_name[cls.qw_downloadnumber][0];cls.qw_downloadnumber++)
		{
			// skip submodels
			if (cl.model_name[cls.qw_downloadnumber][0] == '*')
				continue;
			if (String_Match(cl.model_name[cls.qw_downloadnumber], "progs/spike.mdl"))
				cl.qw_modelindex_spike = cls.qw_downloadnumber;
			if (String_Match(cl.model_name[cls.qw_downloadnumber], "progs/player.mdl"))
				cl.qw_modelindex_player = cls.qw_downloadnumber;
			if (String_Match(cl.model_name[cls.qw_downloadnumber], "progs/flag.mdl"))
				cl.qw_modelindex_flag = cls.qw_downloadnumber;
			if (String_Match(cl.model_name[cls.qw_downloadnumber], "progs/s_explod.spr"))
				cl.qw_modelindex_s_explod = cls.qw_downloadnumber;
			// check if we need to download the file, and return if so
			if (!QW_CL_CheckOrDownloadFile(cl.model_name[cls.qw_downloadnumber]))
				return;
		}

		cls.qw_downloadtype = dl_none;

		// touch all of the precached models that are still loaded so we can free
		// anything that isn't needed
		if (!sv.active)
			Mod_ClearUsed();

		for (j = 1; j < MAX_MODELS_8192 && cl.model_name[j][0]; j++)
			Mod_FindName(cl.model_name[j], cl.model_name[j][0] == '*' ? cl.model_name[1] : NULL);
		// precache any models used by the client (this also marks them used)
		cl.model_bolt = Mod_ForName("progs/bolt.mdl", false, false, NULL);
		cl.model_bolt2 = Mod_ForName("progs/bolt2.mdl", false, false, NULL);
		cl.model_bolt3 = Mod_ForName("progs/bolt3.mdl", false, false, NULL);

		cl.model_beam = Mod_ForName("progs/beam.mdl", false, false, NULL);

		// we purge the models and sounds later in CL_SignonReply
		//Mod_PurgeUnused();

		// now we try to load everything that is new

		// world model
		cl.model_precache[1] = Mod_ForName(cl.model_name[1], false, false, NULL);
		if (cl.model_precache[1]->Draw == NULL)
			Con_PrintLinef ("Map %s could not be found or downloaded", cl.model_name[1]);

		// normal models
		for (j = 2;j < MAX_MODELS_8192 && cl.model_name[j][0];j++)
			if ((cl.model_precache[j] = Mod_ForName(cl.model_name[j], false, false, cl.model_name[j][0] == '*' ? cl.model_name[1] : NULL))->Draw == NULL)
				Con_PrintLinef ("Model %s could not be found or downloaded", cl.model_name[j]);

		// check memory integrity
		Mem_CheckSentinelsGlobal();

		// now that we have a world model, set up the world entity, renderer
		// modules and csqc
		CL_SetupWorldModel();

		{
			QW_CL_FindModelNumbers ();
		}

		// add pmodel/emodel CRCs to userinfo
		CL_SetInfo("pmodel", va(vabuf, sizeof(vabuf), "%d", FS_CRCFile("progs/player.mdl", NULL)), true, true, true, true);
		CL_SetInfo("emodel", va(vabuf, sizeof(vabuf), "%d", FS_CRCFile("progs/eyes.mdl", NULL)), true, true, true, true);

		// done checking sounds and models, send a prespawn command now
		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd,
			"prespawn %d 0 %d", cl.qw_servercount, cl.model_precache[1]->brush.qw_md4sum2);

		if (cls.qw_downloadmemory)
		{
			Mem_Free(cls.qw_downloadmemory);
			cls.qw_downloadmemory = NULL;
		}

		// done loading
		cl.loadfinished = true;
		break;
	case dl_sound:
		if (cls.qw_downloadnumber == 0) {
			Con_PrintLinef ("Checking sounds...");
			cls.qw_downloadnumber = 1;
		}

		for (;cl.sound_name[cls.qw_downloadnumber][0];cls.qw_downloadnumber++)
		{
			// check if we need to download the file, and return if so
			if (!QW_CL_CheckOrDownloadFile(va(vabuf, sizeof(vabuf), "sound/%s", cl.sound_name[cls.qw_downloadnumber])))
				return;
		}

		cls.qw_downloadtype = dl_none;

		// clear sound usage flags for purging of unused sounds
		S_ClearUsed();

		// precache any sounds used by the client
		cl.sfx_wizhit = S_PrecacheSound(cl_sound_wizardhit.string, false, true);
		cl.sfx_knighthit = S_PrecacheSound(cl_sound_hknighthit.string, false, true);
		cl.sfx_tink1 = S_PrecacheSound(cl_sound_tink1.string, false, true);
		cl.sfx_ric1 = S_PrecacheSound(cl_sound_ric1.string, false, true);
		cl.sfx_ric2 = S_PrecacheSound(cl_sound_ric2.string, false, true);
		cl.sfx_ric3 = S_PrecacheSound(cl_sound_ric3.string, false, true);
		cl.sfx_r_exp3 = S_PrecacheSound(cl_sound_r_exp3.string, false, true);

		// sounds used by the game
		for (j = 1;j < MAX_SOUNDS_4096 && cl.sound_name[j][0];j++)
			cl.sound_precache[j] = S_PrecacheSound(cl.sound_name[j], true, true);

		// we purge the models and sounds later in CL_SignonReply
		//S_PurgeUnused();

		// check memory integrity
		Mem_CheckSentinelsGlobal();

		// done with sound downloads, next we check models
		Msg_WriteByte_WriteStringf(&cls.netcon->message, qw_clc_stringcmd, "modellist %d %d", cl.qw_servercount, 0);
		//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
		//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "modellist %d %d", cl.qw_servercount, 0));
		break;
	case dl_none:
	default:
		Con_PrintLinef ("Unknown download type.");
	}
}

void CL_Models_Query (feed_fn_t myfeed_shall_stop)
{
	if (!r_refdef.scene.worldmodel)
		return;

	for (int j = 1; j < MAX_MODELS_8192 && cl.model_name[j][0]; j ++) {

		qbool shall_stop = myfeed_shall_stop (-1, cl.model_name[j], "", NULL, NULL, NULL, j, 1, 2);
		if (shall_stop)
			return;
	} // for

	//cl.model_precache
	//model_t		*m = r_refdef.scene.worldmodel;
	//texture_t	*t;
	//int j;
	//for (j = 0, t = m->data_textures; j < m->num_textures; j++, t++) {
	//	if (!t->name[0]) continue;
	//	if (String_Contains_Caseless(t->name, "NO TEXTURE FOUND")) continue;
		
}
 
WARP_X_ ()
void CL_Sounds_Query (feed_fn_t myfeed_shall_stop)
{
	if (!r_refdef.scene.worldmodel)
		return;

	for (int j = 1; j < MAX_SOUNDS_4096 && cl.sound_name[j][0]; j ++) {

		qbool shall_stop = myfeed_shall_stop (-1, cl.sound_name[j], "", NULL, NULL, NULL, j, 1, 2);
		if (shall_stop)
			return;
	} // for
		
}

#include "cl_parse_chunked.c.h"

WARP_X_ (qw_svc_download oob is false)
void QW_CL_ParseDownload(int q_is_oob)
{
	// ezQuake start chunk

	if (developer_qw.integer)
		Con_PrintLinef ("qw_svc_download -> QW_CL_ParseDownload");

	if (Have_Flag (cls.fteprotocolextensions, PEXT_CHUNKEDDOWNLOADS)) {
		if (developer_qw.integer)
			Con_PrintLinef ("QW_CL_ParseDownload: Download is Chunked");
		QW_CL_ParseChunkedDownload(q_is_oob);
		return;
	}

	int size = (signed short)MSG_ReadShort(&cl_message);
	int percent_of_all = MSG_ReadByte(&cl_message);

	//Con_Printf ("download %d %d%% (%d/%d)\n", size, percent, cls.qw_downloadmemorycursize, cls.qw_downloadmemorymaxsize);

	if (developer_qw.integer)
		Con_PrintLinef ("QW_CL_ParseDownload: Download is Not chunked");

	// skip the download fragment if playing a demo
	if (!cls.netcon) {
		if (size > 0)
			cl_message.readcount += size;
		return;
	}

	if (size == -1) {
		Con_PrintLinef ("QW_CL_ParseDownload: File not found size -1.");
		QW_CL_RequestNextDownload();
		return;
	}

	if (cl_message.readcount + (unsigned short)size > cl_message.cursize)
		Host_Error_Line ("corrupt download message");

#if 1


	Baker_CL_Download_During_ReadMsg_Size_Enlarge_QW (size);
#else
	// make sure the buffer is big enough to include this new fragment
	if (!cls.qw_downloadmemory || cls.qw_downloadmemorymaxsize < cls.qw_downloadmemorycursize + size) {
		unsigned char *old;
		while (cls.qw_downloadmemorymaxsize < cls.qw_downloadmemorycursize + size)
			cls.qw_downloadmemorymaxsize *= 2;
		old = cls.qw_downloadmemory;
		cls.qw_downloadmemory = (unsigned char *)Mem_Alloc(cls.permanentmempool, cls.qw_downloadmemorymaxsize);
		if (old)
		{
			memcpy(cls.qw_downloadmemory, old, cls.qw_downloadmemorycursize);
			Mem_Free(old);
		}
	}

	// read the fragment out of the packet
	MSG_ReadBytes(&cl_message, size, cls.qw_downloadmemory + cls.qw_downloadmemorycursize);
	cls.qw_downloadmemorycursize += size;
	cls.qw_downloadspeedcount += size;
#endif
	cls.qw_downloadpercent = percent_of_all;

	if (percent_of_all != 100) {
		// request next fragment
		// Baker: I think there is where next chunked occurs
		// ezQuake I think there is where next chunked occurs

		if (developer_qw.integer)
			Con_PrintLinef ("QW_CL_CheckOrDownloadFile: Asking for next download via " QUOTED_S, "nextdl");
		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd, "nextdl");
		//MSG_WriteString(&cls.netcon->message, "nextdl");
	}
	else
	{
		// finished file

#if 1

		Baker_CL_Download_FWrite (cls.qw_downloadmemorycursize, -1);
#else
		Con_PrintLinef ("Downloaded " QUOTED_S, cls.qw_downloadname);
		FS_WriteFile (cls.qw_downloadname, cls.qw_downloadmemory, cls.qw_downloadmemorycursize);


#endif
		cls.qw_downloadpercent = 0;

		// start downloading the next file (or join the game)
		QW_CL_RequestNextDownload();
	}
}

static void QW_CL_ParseModelList(int is_double_width)
{
	int n;
	int nummodels = is_double_width ? MSG_ReadShort (&cl_message) : MSG_ReadByte(&cl_message);
	char *str;

	// parse model precache list
	while (1) {
		str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		if (!str[0])
			break;

		nummodels++;
		if (nummodels == MAX_MODELS_8192)
			Host_Error_Line ("Server sent too many model precaches");

		if (strlen(str) >= MAX_QPATH_128)
			Host_Error_Line ("Server sent a precache name of %d characters (max %d)", (int)strlen(str), MAX_QPATH_128 - 1);

		c_strlcpy (cl.model_name[nummodels], str);
	}

	n = MSG_ReadByte(&cl_message);
	if (n) {
#if 1 // Baker: Hmmm ... purpose?
		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd,
			"modellist %d %d", cl.qw_servercount, (nummodels & 0xff00) + n);
#else
		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd,
			"modellist %d %d", cl.qw_servercount, n);

#endif
		//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
		//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "modellist %d %d", cl.qw_servercount, n));
		return;
	}

	cls.signon = SIGNON_2; // QUAKEWORLD
	cls.qw_downloadnumber = 0;
	cls.qw_downloadtype = dl_model;
	QW_CL_RequestNextDownload();
}

static void QW_CL_ParseSoundList(void)
{
	int n;
	int numsounds = MSG_ReadByte(&cl_message);
	char *str;

	// parse sound precache list
	for (;;)
	{
		str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		if (!str[0])
			break;
		numsounds++;
		if (numsounds==MAX_SOUNDS_4096)
			Host_Error_Line ("Server sent too many sound precaches");
		if (strlen(str) >= MAX_QPATH_128)
			Host_Error_Line ("Server sent a precache name of %d characters (max %d)", (int)strlen(str), MAX_QPATH_128 - 1);
		strlcpy(cl.sound_name[numsounds], str, sizeof (cl.sound_name[numsounds]));
	}

	n = MSG_ReadByte(&cl_message);

	if (n) {
		Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd,
			"soundlist %d %d", cl.qw_servercount, n);
		//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
		//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "soundlist %d %d", cl.qw_servercount, n));
		return;
	}

	cls.signon = SIGNON_2; // QUAKEWORLD
	cls.qw_downloadnumber = 0;
	cls.qw_downloadtype = dl_sound;
	QW_CL_RequestNextDownload();
}

static void QW_CL_Skins_f(cmd_state_t *cmd)
{
	cls.qw_downloadnumber = 0;
	cls.qw_downloadtype = dl_skin;
	QW_CL_RequestNextDownload();
}

static void QW_CL_Changing_f(cmd_state_t *cmd)
{
	if (cls.qw_downloadmemory)  // don't change when downloading
		return;

	S_StopAllSounds();
	cl.intermission = 0;
	cls.signon = SIGNON_1;  // QUAKEWORLD -- not active anymore, but not disconnected
	Con_PrintLinef (NEWLINE "Changing map...");
}

void QW_CL_NextUpload_f(cmd_state_t *cmd)
{
	int r, percent, size;

	if (!cls.qw_uploaddata)
		return;

	r = cls.qw_uploadsize - cls.qw_uploadpos;
	if (r > 768)
		r = 768;
	size = min(1, cls.qw_uploadsize);
	percent = (cls.qw_uploadpos+r)*100/size;

	MSG_WriteByte(&cls.netcon->message, qw_clc_upload);
	MSG_WriteShort(&cls.netcon->message, r);
	MSG_WriteByte(&cls.netcon->message, percent);
	SZ_Write(&cls.netcon->message, cls.qw_uploaddata + cls.qw_uploadpos, r);

	Con_DPrintLinef ("UPLOAD: %6d: %d written", cls.qw_uploadpos, r);

	cls.qw_uploadpos += r;

	if (cls.qw_uploadpos < cls.qw_uploadsize)
		return;

	Con_PrintLinef ("Upload completed");

	QW_CL_StopUpload_f(cmd);
}

void QW_CL_StartUpload(unsigned char *data, int size)
{
	// do nothing in demos or if not connected
	if (!cls.netcon)
		return;

	// abort existing upload if in progress
	QW_CL_StopUpload_f(cmd_local);

	Con_DPrintLinef ("Starting upload of %d bytes...", size);

	cls.qw_uploaddata = (unsigned char *)Mem_Alloc(cls.permanentmempool, size);
	memcpy(cls.qw_uploaddata, data, size);
	cls.qw_uploadsize = size;
	cls.qw_uploadpos = 0;

	QW_CL_NextUpload_f(cmd_local);
}

#if 0
qbool QW_CL_IsUploading(void)
{
	return cls.qw_uploaddata != NULL;
}
#endif

void QW_CL_StopUpload_f(cmd_state_t *cmd)
{
	if (cls.qw_uploaddata)
		Mem_Free(cls.qw_uploaddata);
	cls.qw_uploaddata = NULL;
	cls.qw_uploadsize = 0;
	cls.qw_uploadpos = 0;
}

WARP_X_ (qw_svc_setinfo			QW_CL_SetInfo)
WARP_X_ (qw_svc_updateuserinfo	QW_CL_UpdateUserInfo)
WARP_X_ (QW_CL_RequestNextDownload)

static void QW_CL_ProcessUserInfo(int slot)
{
	int topcolor, bottomcolor;
	char temp[2048];
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "name", cl.scores[slot].name, sizeof(cl.scores[slot].name));
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "topcolor", temp, sizeof(temp));topcolor = atoi(temp);
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "bottomcolor", temp, sizeof(temp));bottomcolor = atoi(temp);
	cl.scores[slot].colors = topcolor * 16 + bottomcolor;
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "*spectator", temp, sizeof(temp));
	cl.scores[slot].qw_spectator = temp[0] != 0;
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "team", cl.scores[slot].qw_team, sizeof(cl.scores[slot].qw_team));
	InfoString_GetValue(cl.scores[slot].qw_userinfo, "skin", cl.scores[slot].qw_skin, sizeof(cl.scores[slot].qw_skin));
	if (!cl.scores[slot].qw_skin[0])
		strlcpy(cl.scores[slot].qw_skin, "base", sizeof(cl.scores[slot].qw_skin));
	// TODO: skin cache
}

static void QW_CL_UpdateUserInfo(void)
{
	int slot;
	slot = MSG_ReadByte(&cl_message);
	if (slot >= cl.maxclients) {
		Con_PrintLinef ("svc_updateuserinfo >= cl.maxclients (%d >= %d)", slot, cl.maxclients);
		MSG_ReadLong(&cl_message);
		MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		return;
	}
	cl.scores[slot].qw_userid = MSG_ReadLong(&cl_message);
	strlcpy(cl.scores[slot].qw_userinfo, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(cl.scores[slot].qw_userinfo));

	QW_CL_ProcessUserInfo(slot);
}

static void QW_CL_SetInfo(void)
{
	int slot;
	char key[2048];
	char value[2048];
	slot = MSG_ReadByte(&cl_message);
	strlcpy(key, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(key));
	strlcpy(value, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(value));
	if (slot >= cl.maxclients) {
		Con_PrintLinef ("svc_setinfo >= cl.maxclients  (%d >= %d)", slot, cl.maxclients);
		return;
	}
	InfoString_SetValue(cl.scores[slot].qw_userinfo, sizeof(cl.scores[slot].qw_userinfo), key, value);

	QW_CL_ProcessUserInfo(slot);
}

WARP_X_ (QW_CL_ServerInfo qw_svc_serverinfo cl.serverinfo )
void QW_CL_ProcessServerInfo (void)
{
	char temp[2048];
	cl.qw_z_ext = 0;
	// Get the server's ZQuake extension bits
	char *s_val = InfoString_GetValue (cl.qw_serverinfo, "*z_ext", temp, sizeof(temp));

	if (s_val)
		cl.qw_z_ext = atoi(s_val);
	//Info_ValueForKey(cl.qw_serverinfo, "*z_ext"));

}

WARP_X_ (svc_serverinfo)
static void QW_CL_ServerInfo(void)
{
	char key[2048];
	char value[2048];
	char temp[32];
	strlcpy(key, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(key));
	strlcpy(value, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(value));
	Con_DPrintLinef ("SERVERINFO: %s=%s", key, value);
	InfoString_SetValue(cl.qw_serverinfo, sizeof(cl.qw_serverinfo), key, value);
	InfoString_GetValue(cl.qw_serverinfo, "teamplay", temp, sizeof(temp));
	cl.qw_teamplay = atoi(temp);


	QW_CL_ProcessServerInfo ();
}

static void QW_CL_ParseNails(void)
{
	int i, j;
	int numnails = MSG_ReadByte(&cl_message);
	vec_t *v;
	unsigned char bits[6];
	for (i = 0;i < numnails;i++)
	{
		for (j = 0;j < 6;j++)
			bits[j] = MSG_ReadByte(&cl_message);
		if (cl.qw_num_nails >= 255)
			continue;
		v = cl.qw_nails[cl.qw_num_nails++];
		v[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		v[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		v[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		v[3] = -360*(bits[4]>>4)/16;
		v[4] = 360*bits[5]/256;
		v[5] = 0;
	}
}

static void CL_UpdateItemsAndWeapon(void)
{
	int j;
	// check for important changes

	// set flash times
	if (cl.olditems != cl.stats[STAT_ITEMS])
		for (j = 0;j < 32;j++)
			if ((cl.stats[STAT_ITEMS] & (1<<j)) && !(cl.olditems & (1<<j)))
				cl.item_gettime[j] = cl.time;
	cl.olditems = cl.stats[STAT_ITEMS];

	// GAME_NEXUIZ hud needs weapon change time
	if (cl.activeweapon != cl.stats[STAT_ACTIVEWEAPON])
		cl.weapontime = cl.time;
	cl.activeweapon = cl.stats[STAT_ACTIVEWEAPON];
}

#define LOADPROGRESSWEIGHT_SOUND            1.0
#define LOADPROGRESSWEIGHT_MODEL            4.0
#define LOADPROGRESSWEIGHT_WORLDMODEL      30.0
#define LOADPROGRESSWEIGHT_WORLDMODEL_INIT  2.0

#include "cl_parse_download.c.h"

extern cvar_t cl_topcolor;
extern cvar_t cl_bottomcolor;
static void CL_SignonReply_SIGNON_1_SendPlayerInfo(void)
{ // CROSSX
	char vabuf[1024];
	MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
	MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "name " QUOTED_S, cl_name.string));

	MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
	MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "color %d %d", cl_topcolor.integer, cl_bottomcolor.integer));

	MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
	MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "rate %d", cl_rate.integer));

	MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
	MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "rate_burstsize %d", cl_rate_burstsize.integer));

	if (cl_pmodel.integer) {
		MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
		MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "pmodel %d", cl_pmodel.integer));
	}
	if (*cl_playermodel.string) {
		MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
		MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "playermodel %s", cl_playermodel.string));
	}
	if (*cl_playerskin.string) {
		MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
		MSG_WriteString (&cls.netcon->message, va(vabuf, sizeof(vabuf), "playerskin %s", cl_playerskin.string));
	}
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
static void CL_SignonReply (void)
{
	switch (cls.signon) {
	case SIGNON_1:
		if (cls.netcon) {
			// send player info before we begin downloads
			// (so that the server can see the player name while downloading)
			CL_SignonReply_SIGNON_1_SendPlayerInfo();

			// execute cl_begindownloads next frame
			// (after any commands added by svc_stufftext have been executed)
			// when done with downloads the "prespawn" will be sent
			WARP_X_ (CL_BeginDownloads_DP_f)
			Cbuf_AddTextLine (cmd_local, NEWLINE "cl_begindownloads" NEWLINE);

			//MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
			//MSG_WriteString (&cls.netcon->message, "prespawn");
		}
		else // playing a demo...  make sure loading occurs as soon as possible
			CL_BeginDownloads_DP (q_is_aborted_download_false);
		break;

	case SIGNON_2:
		if (cls.netcon) {
			// LadyHavoc: quake sent the player info here but due to downloads
			// it is sent earlier instead
			// CL_SignonReply_SIGNON_1_SendPlayerInfo();

			// LadyHavoc: changed to begin a loading stage and issue this when done
			MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
			MSG_WriteString (&cls.netcon->message, "spawn");
		}
		break;

	case SIGNON_3:
		if (cls.netcon)
		{
			MSG_WriteByte (&cls.netcon->message, clc_stringcmd);
			MSG_WriteString (&cls.netcon->message, "begin");
		}
		break;

	case SIGNONS_4:
		// after the level has been loaded, we shouldn't need the shaders, and
		// if they are needed again they will be automatically loaded...
		// we also don't need the unused models or sounds from the last level
		Mod_FreeQ3Shaders();
		Mod_PurgeUnused();
		S_PurgeUnused();

		Con_ClearNotify();
		if (Sys_CheckParm("-profilegameonly"))
			Sys_AllowProfiling(true);
		break;
	}
}

/*
==================
CL_ParseServerInfo
==================
*/
WARP_X_ (svc_serverinfo ) // Baker: svc_serverinfo and qw_svc_serverdata are both 11
WARP_X_ (qw_svc_serverdata)
static void CL_ParseServerInfo (int is_qw)
{
	char *str;
	int j;
	protocolversion_t protocol;
	int nummodels, numsounds;

	// if we start loading a level and a video is still playing, stop it
	CL_VideoStop (REASON_NEW_MAP_1);

	Con_DPrintLinef ("Serverinfo packet received.");
	Collision_Cache_Reset(true);

#if 1
	if (is_qw && cls.demorecording) {
		Con_PrintLinef ("Unable to record Quakeworld demos, stopping record.");
		CL_Stop_f (cmd_local);
	}
#endif

	// if server is active, we already began a loading plaque
	if (!sv.active) {
		SCR_BeginLoadingPlaque(false);
		S_StopAllSounds();
		// prevent dlcache assets from the previous map from interfering with this one
		FS_UnloadPacks_dlcache();
		// free q3 shaders so that any newly downloaded shaders will be active
		// bones_was_here: we free the q3 shaders later in CL_SignonReply
		//Mod_FreeQ3Shaders();
	}

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	// clear cl_serverextension cvars
	Cvar_SetValueQuick(&cl_serverextension_download, 0);

//
// wipe the client_state_t struct
//
	CL_ClearState ();
	cls.fteprotocolextensions = 0;  cls.zirconprotocolextensions = 0; cl.qw_z_ext = 0;


// parse protocol version number
	if (!is_qw) {
		// Baker: cls.signon is 0
		j = MSG_ReadLong(&cl_message);
		goto quakeworld_skip;
	}

	// ezQuake read fte sv_extensions
#if 1
	while (1) {
		int protover = MSG_ReadLong (&cl_message);
		if (protover == PROTOCOL_VERSION_FTE1) {
			int sv_extensions = MSG_ReadLong(&cl_message);
			cls.fteprotocolextensions = sv_extensions;
			Con_PrintLinef ("Server: Using FTE extensions 0x%x", cls.fteprotocolextensions);
			continue;
		}
		//if (protover == PROTOCOL_VERSION_EZQUAKE1) {
		//	int sv_extensions = MSG_ReadLong(&cl_message);
		//	cls.mvdprotocolextensions1 = sv_extensions;
		//	Con_PrintLinef ("Server: Using ezQuake extensions 0x%x", cls.mvdprotocolextensions1);
		//	continue;
		//}

		if (protover == PROTOCOL_VERSION_QW_28) //this ends the version info
			break;
		Host_Error_Line ("Server returned version %d, not %d" NEWLINE "You probably need to upgrade.", protover, 28 /*PROTOCOL_VERSION*/);
	} // while
	j = PROTOCOL_VERSION_QW_28;
#endif

quakeworld_skip:

	protocol = Protocol_EnumForNumber(j);


	if (protocol == PROTOCOL_UNKNOWN_0) {
		Host_Error_Line ("CL_ParseServerInfo: Server is unrecognized protocol number (%d)", j);
		return;
	}
	// hack for unmarked Nehahra movie demos which had a custom protocol
	if (protocol == PROTOCOL_QUAKEDP && cls.demoplayback
		&& gamemode == GAME_NEHAHRA)
		protocol = PROTOCOL_NEHAHRAMOVIE;
	cls.protocol = protocol;

	// Baker: For reasons unclear, this happens late in single player and early with demos
	// or at least it seems to.
	Con_DPrintLinef ("Server protocol is %s", Protocol_NameForEnum(cls.protocol));

	cls.protocol_flags_rmq = 0;
	if (isin1(cls.protocol, PROTOCOL_FITZQUAKE999)) {
		const unsigned int supportedflags = (PRFL_RMQ_SHORTANGLE_USED | PRFL_RMQ_INT32COORD_USED);
			//PRFL_RMQ_FLOATANGLE_UNUSED | PRFL_RMQ_24BITCOORD_UNUSED | PRFL_RMQ_FLOATCOORD_UNUSED | PRFL_RMQ_EDICTSCALE_UNUSED |
			//PRFL_RMQ_INT32COORD_USED);

		// mh - read protocol flags from server so that we know what protocol features to expect
		cls.protocol_flags_rmq = (unsigned int) MSG_ReadLong (&cl_message);

		if (0 != (cls.protocol_flags_rmq & (~supportedflags))) {
			Con_PrintLinef (CON_WARN "PROTOCOL_RMQ protocol_flags_rmq %d contains unsupported flags", cls.protocol_flags_rmq);
		}
	} // if rmq

	cl.num_entities = 1;

	if (protocol == PROTOCOL_QUAKEWORLD) {
		char gamedir[1][MAX_QPATH_128];

		cl.qw_servercount = MSG_ReadLong(&cl_message);

		str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		Con_PrintLinef ("server gamedir is %s", str);
		c_strlcpy(gamedir[0], str);
gamedir_change:
		// change gamedir if needed
		if (!cl.islocalgame)
		if (!FS_ChangeGameDirs(1, gamedir, q_tx_complain_true, q_fail_on_missing_false))
			Host_Error_Line ("CL_ParseServerInfo: unable to switch to server specified gamedir");

		cl.gametype = GAME_DEATHMATCH;
		cl.maxclients = QW_MAX_CLIENTS_32;

		// parse player number
		j = MSG_ReadByte(&cl_message);
		// cl.qw_spectator is an unneeded flag, cl.scores[cl.playerentity].qw_spectator works better (it can be updated by the server during the game)
		//int is_cl_qw_spectator_on_connect = (j & 128) != 0;
		cl.realplayerentity = cl.playerentity = cl.viewentity = (j & 127) + 1;
		cl.scores = (scoreboard_t *)Mem_Alloc(cls.levelmempool, cl.maxclients*sizeof(*cl.scores));
		//cl.scores[cl.playerentity-1].qw_spectator = is_cl_qw_spectator_on_connect;

		// get the full level name
		str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		strlcpy (cl.worldmessage, str, sizeof(cl.worldmessage));

		// get the movevars that are defined in the qw protocol
		cl.movevars_gravity            = MSG_ReadFloat(&cl_message);
		cl.movevars_stopspeed          = MSG_ReadFloat(&cl_message);
		cl.movevars_maxspeed           = MSG_ReadFloat(&cl_message);
		cl.movevars_spectatormaxspeed  = MSG_ReadFloat(&cl_message);
		cl.movevars_accelerate         = MSG_ReadFloat(&cl_message);
		cl.movevars_airaccelerate      = MSG_ReadFloat(&cl_message);
		cl.movevars_wateraccelerate    = MSG_ReadFloat(&cl_message);
		cl.movevars_friction           = MSG_ReadFloat(&cl_message);
		cl.movevars_waterfriction      = MSG_ReadFloat(&cl_message);
		cl.movevars_entgravity         = MSG_ReadFloat(&cl_message);

		// other movevars not in the protocol...
		cl.movevars_wallfriction = 0;
		cl.movevars_timescale = 1;
		cl.movevars_jumpvelocity = 270;
		cl.movevars_edgefriction = 1;
		cl.movevars_maxairspeed = 30;
		cl.movevars_stepheight = 18;
		cl.movevars_airaccel_qw = 1;
		cl.movevars_airaccel_sideways_friction = 0;

		// seperate the printfs so the server message can have a color
		//Con_Printf ("\n\n<===================================>\n\n\2%s\n", str);

		// Baker r1431: Less console spam.
		Con_PrintLinef (NEWLINE NEWLINE NEWLINE NEWLINE "\2%s", str);

		// check memory integrity
		Mem_CheckSentinelsGlobal();

		if (cls.netcon) {
			Msg_WriteByte_WriteStringf (&cls.netcon->message, qw_clc_stringcmd,
				"soundlist %d %d", cl.qw_servercount, 0);
			//MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
			//MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "soundlist %d %d", cl.qw_servercount, 0));
		}

		cl.loadbegun = false;
		cl.loadfinished = false;

		cls.state = ca_connected;
		cls.signon = SIGNON_1; // QUAKEWORLD

		// note: on QW protocol we can't set up the gameworld until after
		// downloads finish...
		// (we don't even know the name of the map yet)
		// this also means cl_autodemo does not work on QW protocol...

		strlcpy(cl.worldname, "", sizeof(cl.worldname));
		strlcpy(cl.worldnamenoextension, "", sizeof(cl.worldnamenoextension));
		strlcpy(cl.worldbasename, "qw", sizeof(cl.worldbasename));
		Cvar_SetQuick(&cl_worldname, cl.worldname);
		Cvar_SetQuick(&cl_worldnamenoextension, cl.worldnamenoextension);
		Cvar_SetQuick(&cl_worldbasename, cl.worldbasename);

		// check memory integrity
		Mem_CheckSentinelsGlobal();
	}
	else
	{
	// parse maxclients
		cl.maxclients = MSG_ReadByte(&cl_message);
		if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD_255) {
			Host_Error_Line ("Bad maxclients (%u) from server", cl.maxclients);
			return;
		}
		cl.scores = (scoreboard_t *)Mem_Alloc(cls.levelmempool, cl.maxclients*sizeof(*cl.scores));

	// parse gametype
		cl.gametype = MSG_ReadByte(&cl_message);
		cls.fteprotocolextensions = 0; cls.zirconprotocolextensions = 0; cl.qw_z_ext = 0; // Hmmm
		// the original id singleplayer demos are bugged and contain
		// GAME_DEATHMATCH even for singleplayer
		if (cl.maxclients == 1 && cls.protocol == PROTOCOL_QUAKE)
			cl.gametype = GAME_COOP;

	// parse signon message
		str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
		c_strlcpy (cl.worldmessage, str);

	// seperate the printfs so the server message can have a color
		if (cls.protocol != PROTOCOL_NEHAHRAMOVIE) { // no messages when playing the Nehahra movie
// Baker r1431: Less console spam.
			Con_PrintLinef (NEWLINE NEWLINE NEWLINE "\2%s", str);
		}

		// check memory integrity
		Mem_CheckSentinelsGlobal();

		// parse model precache list
		for (nummodels=1 ; ; nummodels++)
		{
			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			if (!str[0])
				break;
			if (nummodels==MAX_MODELS_8192)
				Host_Error_Line ("Server sent too many model precaches");
			if (strlen(str) >= MAX_QPATH_128)
				Host_Error_Line ("Server sent a precache name of %d characters (max %d)", (int)strlen(str), MAX_QPATH_128 - 1);
			c_strlcpy (cl.model_name[nummodels], str);
		}
		// parse sound precache list
		for (numsounds=1 ; ; numsounds++)
		{
			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			if (!str[0])
				break;
			if (numsounds==MAX_SOUNDS_4096)
				Host_Error_Line ("Server sent too many sound precaches");
			if (strlen(str) >= MAX_QPATH_128)
				Host_Error_Line ("Server sent a precache name of %d characters (max %d)", (int)strlen(str), MAX_QPATH_128 - 1);
			c_strlcpy (cl.sound_name[numsounds], str);
		}

		// set the base name for level-specific things...  this gets updated again by CL_SetupWorldModel later
		c_strlcpy(cl.worldname, cl.model_name[1]);
		FS_StripExtension(cl.worldname, cl.worldnamenoextension, sizeof(cl.worldnamenoextension));
		c_strlcpy(cl.worldbasename,
			String_Starts_With_PRE(cl.worldnamenoextension, "maps/") ? cl.worldnamenoextension + 5 : cl.worldnamenoextension);
		Cvar_SetQuick(&cl_worldmessage, cl.worldmessage);
		Cvar_SetQuick(&cl_worldname, cl.worldname);
		Cvar_SetQuick(&cl_worldnamenoextension, cl.worldnamenoextension);
		Cvar_SetQuick(&cl_worldbasename, cl.worldbasename);

		// touch all of the precached models that are still loaded so we can free
		// anything that isn't needed
		if (!sv.active)
			Mod_ClearUsed();
		for (j = 1;j < nummodels;j++)
			Mod_FindName(cl.model_name[j], cl.model_name[j][0] == '*' ? cl.model_name[1] : NULL);
		// precache any models used by the client (this also marks them used)
		cl.model_bolt = Mod_ForName("progs/bolt.mdl", /*crash checkdisk parent name*/ false, false, NULL);
		cl.model_bolt2 = Mod_ForName("progs/bolt2.mdl", false, false, NULL);
		cl.model_bolt3 = Mod_ForName("progs/bolt3.mdl", false, false, NULL);
		cl.model_beam = Mod_ForName("progs/beam.mdl", false, false, NULL);

		// we purge the models and sounds later in CL_SignonReply
		//Mod_PurgeUnused();
		//S_PurgeUnused();

		// clear sound usage flags for purging of unused sounds
		S_ClearUsed();

		// precache any sounds used by the client
		cl.sfx_wizhit = S_PrecacheSound(cl_sound_wizardhit.string, false, true);
		cl.sfx_knighthit = S_PrecacheSound(cl_sound_hknighthit.string, false, true);
		cl.sfx_tink1 = S_PrecacheSound(cl_sound_tink1.string, false, true);
		cl.sfx_ric1 = S_PrecacheSound(cl_sound_ric1.string, false, true);
		cl.sfx_ric2 = S_PrecacheSound(cl_sound_ric2.string, false, true);
		cl.sfx_ric3 = S_PrecacheSound(cl_sound_ric3.string, false, true);
		cl.sfx_r_exp3 = S_PrecacheSound(cl_sound_r_exp3.string, false, true);

		// sounds used by the game
		for (j = 1;j < MAX_SOUNDS_4096 && cl.sound_name[j][0];j++)
			cl.sound_precache[j] = S_PrecacheSound(cl.sound_name[j], /*complain levelsound*/ true, true);

		// now we try to load everything that is new
		cl.loadmodel_current = 1;
		cl.downloadmodel_current = 1;
		cl.loadmodel_total = nummodels;
		cl.loadsound_current = 1;
		cl.downloadsound_current = 1;
		cl.loadsound_total = numsounds;
		cl.downloadcsqc = true;
		cl.loadbegun = false;
		cl.loadfinished = false;
		cl.loadcsqc = true;

		// check memory integrity
		Mem_CheckSentinelsGlobal();

	// if cl_autodemo is set, automatically start recording a demo if one isn't being recorded already
		if (cl_autodemo.integer && cls.netcon && cls.protocol != PROTOCOL_QUAKEWORLD) {
			char demofile[MAX_OSPATH];

			if (cls.demorecording) {
				// finish the previous level's demo file
				CL_Stop_f(cmd_local);
			}

			// start a new demo file
			dpsnprintf (demofile, sizeof(demofile), "%s_%s.dem",
				Sys_TimeString (cl_autodemo_nameformat.string), cl.worldbasename);

			Con_PrintLinef ("Auto-recording to %s.", demofile);

			// Reset bit 0 for every new demo
			Cvar_SetValueQuick(&cl_autodemo_delete,
				(cl_autodemo_delete.integer & ~0x1)
				|
				((cl_autodemo_delete.integer & 0x2) ? 0x1 : 0)
			);

			cls.demofile = FS_OpenRealFile(demofile, "wb", fs_quiet_FALSE); // WRITE-EON - demo reocrd for autodemo
			if (cls.demofile) {
				cls.forcetrack = -1;
				FS_Printf (cls.demofile, "%d" NEWLINE, cls.forcetrack);
				cls.demorecording = true;
				strlcpy(cls.demoname, demofile, sizeof(cls.demoname));
				cls.demo_lastcsprogssize = -1;
				cls.demo_lastcsprogscrc = -1;
			}
			else
				Con_PrintLinef (CON_ERROR "ERROR: couldn't open %s", demofile );
		}
	}
	cl.islocalgame = NetConn_IsLocalGame();
}

void CL_ValidateState(entity_state_t *s)
{
	model_t *model;

	if (!s->active)
		return;

	if (s->modelindex >= MAX_MODELS_8192)
		Host_Error_Line ("CL_ValidateState: modelindex (%d) >= MAX_MODELS_8192 (%d)", s->modelindex, MAX_MODELS_8192);

	// these warnings are only warnings, no corrections are made to the state
	// because states are often copied for decoding, which otherwise would
	// propogate some of the corrections accidentally
	// (this used to happen, sometimes affecting skin and frame)

	// colormap is client index + 1
	if (!(s->sflags & RENDER_COLORMAPPED) && s->colormap > cl.maxclients)
		Con_DPrintLinef ("CL_ValidateState: colormap (%d) > cl.maxclients (%d)", s->colormap, cl.maxclients);

	if (developer_extra.integer)
	{
		model = CL_GetModelByIndex(s->modelindex);
		if (model && model->type && s->frame >= model->numframes)
			Con_DPrintLinef ("CL_ValidateState: no such frame %d in " QUOTED_S " (which has %d frames)", s->frame, model->model_name, model->numframes);
		if (model && model->type && s->skin > 0 && s->skin >= model->numskins && !(s->lightpflags & PFLAGS_FULLDYNAMIC_128))
			Con_DPrintLinef ("CL_ValidateState: no such skin %d in " QUOTED_S " (which has %d skins)", s->skin, model->model_name, model->numskins);
	}
}

void CL_MoveLerpEntityStates(entity_t *ent)
{
	float odelta[3], adelta[3];
	VectorSubtract(ent->state_current.origin, ent->persistent.neworigin, odelta);
	VectorSubtract(ent->state_current.angles, ent->persistent.newangles, adelta);
	if (!ent->state_previous.active || ent->state_previous.modelindex != ent->state_current.modelindex) {
		// reset all persistent stuff if this is a new entity
		ent->persistent.lerpdeltatime = 0;
		ent->persistent.lerpstarttime = cl.mtime[1];
		VectorCopy(ent->state_current.origin, ent->persistent.oldorigin);
		VectorCopy(ent->state_current.angles, ent->persistent.oldangles);
		VectorCopy(ent->state_current.origin, ent->persistent.neworigin);
		VectorCopy(ent->state_current.angles, ent->persistent.newangles);
		// reset animation interpolation as well
		ent->render.framegroupblend[0].fb_frame = ent->render.framegroupblend[1].fb_frame = ent->state_current.frame;
		ent->render.framegroupblend[0].start = ent->render.framegroupblend[1].start = cl.time;
		ent->render.framegroupblend[0].lerp = 1;ent->render.framegroupblend[1].lerp = 0;
		ent->render.shadertime = cl.time;
		// reset various persistent stuff
		ent->persistent.muzzleflash = 0;
		ent->persistent.trail_allowed = false;
	}
	else if ((ent->state_previous.effects & EF_TELEPORT_BIT) != (ent->state_current.effects & EF_TELEPORT_BIT))
	{
		// don't interpolate the move
		ent->persistent.lerpdeltatime = 0;
		ent->persistent.lerpstarttime = cl.mtime[1];
		VectorCopy(ent->state_current.origin, ent->persistent.oldorigin);
		VectorCopy(ent->state_current.angles, ent->persistent.oldangles);
		VectorCopy(ent->state_current.origin, ent->persistent.neworigin);
		VectorCopy(ent->state_current.angles, ent->persistent.newangles);
		ent->persistent.trail_allowed = false;

		// if (ent->state_current.frame != ent->state_previous.frame)
		// do this even if we did change the frame
		// teleport bit is only used if an animation restart, or a jump, is necessary
		// so it should be always harmless to do this
		{
			ent->render.framegroupblend[0].fb_frame = ent->render.framegroupblend[1].fb_frame = ent->state_current.frame;
			ent->render.framegroupblend[0].start = ent->render.framegroupblend[1].start = cl.time;
			ent->render.framegroupblend[0].lerp = 1;ent->render.framegroupblend[1].lerp = 0;
		}

		// note that this case must do everything the following case does too
	}
	else if ((ent->state_previous.effects & EF_RESTARTANIM_BIT)
					!= (ent->state_current.effects & EF_RESTARTANIM_BIT))
	{
		ent->render.framegroupblend[1] = ent->render.framegroupblend[0];
		ent->render.framegroupblend[1].lerp = 1;
		ent->render.framegroupblend[0].fb_frame = ent->state_current.frame;
		ent->render.framegroupblend[0].start = cl.time;
		ent->render.framegroupblend[0].lerp = 0;
	}
	else if (DotProduct(odelta, odelta) > 1000*1000
		|| (cl.fixangle[0] && !cl.fixangle[1])
		|| (ent->state_previous.tagindex != ent->state_current.tagindex)
		|| (ent->state_previous.tagxentity != ent->state_current.tagxentity)) // TAGX CL PARSE LERP
	{
		// don't interpolate the move
		// (the fixangle[] check detects teleports, but not constant fixangles
		//  such as when spectating)
		ent->persistent.lerpdeltatime = 0;
		ent->persistent.lerpstarttime = cl.mtime[1];
		VectorCopy(ent->state_current.origin, ent->persistent.oldorigin);
		VectorCopy(ent->state_current.angles, ent->persistent.oldangles);
		VectorCopy(ent->state_current.origin, ent->persistent.neworigin);
		VectorCopy(ent->state_current.angles, ent->persistent.newangles);
		ent->persistent.trail_allowed = false;
	}
	else if (ent->state_current.sflags & RENDER_STEP) {
		// Baker: Does this ever hit and does this do it right?
		// monster interpolation
		if (DotProduct(odelta, odelta) + DotProduct(adelta, adelta) > 0.01) {
			ent->persistent.lerpdeltatime = bound(0, cl.mtime[1] - ent->persistent.lerpstarttime, 0.1);
			ent->persistent.lerpstarttime = cl.mtime[1];
			VectorCopy(ent->persistent.neworigin, ent->persistent.oldorigin);
			VectorCopy(ent->persistent.newangles, ent->persistent.oldangles);
			VectorCopy(ent->state_current.origin, ent->persistent.neworigin);
			VectorCopy(ent->state_current.angles, ent->persistent.newangles);
		}
	}
	else {
		// not a monster
		ent->persistent.lerpstarttime = ent->state_previous.time;
		ent->persistent.lerpdeltatime = bound(0, ent->state_current.time - ent->state_previous.time, 0.1);
		VectorCopy(ent->persistent.neworigin, ent->persistent.oldorigin);
		VectorCopy(ent->persistent.newangles, ent->persistent.oldangles);
		VectorCopy(ent->state_current.origin, ent->persistent.neworigin);
		VectorCopy(ent->state_current.angles, ent->persistent.newangles);
	}
	// trigger muzzleflash effect if necessary
	if (Have_Flag (ent->state_current.effects, EF_MUZZLEFLASH_2))
		ent->persistent.muzzleflash = 1;

	// restart animation bit
	if ((ent->state_previous.effects & EF_RESTARTANIM_BIT) != (ent->state_current.effects & EF_RESTARTANIM_BIT)) {
		ent->render.framegroupblend[1] = ent->render.framegroupblend[0];
		ent->render.framegroupblend[1].lerp = 1;
		ent->render.framegroupblend[0].fb_frame = ent->state_current.frame;
		ent->render.framegroupblend[0].start = cl.time;
		ent->render.framegroupblend[0].lerp = 0;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
WARP_X_ (svcfitz_spawnbaseline2)
static void CL_ParseBaseline (entity_t *ent, int is_large_model_index, int fitz_version, int is_static)
{ // DPD 999 - Stuff goes here
	int j;
	int fitz_bits = 0;

	ent->state_baseline = defaultstate;
	// FIXME: set ent->state_baseline.number?
	ent->state_baseline.active = true;

	// Baker: large model index
	if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		fitz_bits = (fitz_version == 2) ? MSG_ReadByte(&cl_message) : 0;
		ent->state_baseline.modelindex = Have_Flag(fitz_bits, B_FITZ_LARGEMODEL_1) ?
			MSG_ReadShort(&cl_message) :
			MSG_ReadByte(&cl_message);
		ent->state_baseline.frame = Have_Flag(fitz_bits, B_FITZ_LARGEFRAME_2) ?
			MSG_ReadShort(&cl_message) :
			MSG_ReadByte(&cl_message);
	}
	else
	if (is_large_model_index) {
		ent->state_baseline.modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		ent->state_baseline.frame = (unsigned short) MSG_ReadShort(&cl_message);
	}
	else if (isin3 (cls.protocol, PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3)) {
		ent->state_baseline.modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		ent->state_baseline.frame = MSG_ReadByte(&cl_message);
	}
	else
	{
		ent->state_baseline.modelindex = MSG_ReadByte(&cl_message);
		ent->state_baseline.frame = MSG_ReadByte(&cl_message);
	}

	// Baker: Quakeworld comes here, but render flags does not occur here.
	//if (cls.protocol == PROTOCOL_QUAKEWORLD) {
	//	qbool QW_Is_Step_ModelIndex(int qw_modelindex);
	//	if (QW_Is_Step_ModelIndex(ent->state_baseline.modelindex)) {
	//		ent->render |= RENDER_STEP;
	//	}
	//}

	ent->state_baseline.colormap = MSG_ReadByte(&cl_message);
	ent->state_baseline.skin = MSG_ReadByte(&cl_message);
	for (j = 0; j < 3; j++) {
		ent->state_baseline.origin[j] = MSG_ReadCoord(&cl_message, cls.protocol);
		ent->state_baseline.angles[j] = MSG_ReadAngle(&cl_message, cls.protocol);
	}

	WARP_X_ (VM_SV_makestatic)

	// Baker: This is too early for protocol extensions read from the server.
	if (isin1 (cls.protocol, PROTOCOL_DARKPLACES7) &&
		Have_Zircon_Ext_Flag_CLS (ZIRCON_EXT_STATIC_ENT_ALPHA_COLORMOD_SCALE_32)
		&& cls.storr[0]) {
		unsigned char cs_effects_additive1_fullbright2;

		ent->state_baseline.alpha = cls.storr[1];
		ent->state_baseline.colormod[0] = cls.storr[2];
		ent->state_baseline.colormod[1] = cls.storr[3];
		ent->state_baseline.colormod[2] = cls.storr[4];

		cs_effects_additive1_fullbright2 = cls.storr[5];
		ent->state_baseline.scale = cls.storr[6];

		if (Have_Flag (cs_effects_additive1_fullbright2, EF_SHORTY_ADDITIVE_1))
			Flag_Add_To (ent->state_baseline.effects, EF_ADDITIVE_32);
		if (Have_Flag (cs_effects_additive1_fullbright2, EF_SHORTY_FULLBRIGHT_2))
			Flag_Add_To (ent->state_baseline.effects, EF_FULLBRIGHT);
		if (Have_Flag (cs_effects_additive1_fullbright2, EF_SHORTY_NOSHADOW_4))
			Flag_Add_To (ent->state_baseline.effects, EF_NOSHADOW);
		cls.storr[0] = 0; // Clear the store immediately.
	} // Baker: ZIRCON_EXT_STATIC_ENT_ALPHA_COLORMOD_SCALE_32

	if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		ent->state_baseline.alpha = Have_Flag (fitz_bits, B_FITZ_ALPHA_4) ?
			MSG_ReadByte(&cl_message) : 255;//FITZ_ENTALPHA_DEFAULT_0; //johnfitz -- PROTOCOL_FITZQUAKE
		if (Have_Flag (fitz_bits, B_FITZ_SCALE_8)) {
			// Baker: I'm not convinced DarkPlaces and FitzQuake scales are compatible
			// However, AFAIK ... Quakespasm does not use scale at all?
			ent->state_baseline.scale = Have_Flag (fitz_bits, B_FITZ_SCALE_8) ? MSG_ReadByte(&cl_message) :
				FITZ_ENTSCALE_DEFAULT_16;
		} else {
			ent->state_baseline.scale = FITZ_ENTSCALE_DEFAULT_16;
		}
	} // fitz alpha and scale

	ent->state_previous = ent->state_current = ent->state_baseline;
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/

WARP_X_ (svc_clientdata)
static void CL_ParseClientdata (void)
{
	int j, bits;

	VectorCopy (cl.mpunchangle[0], cl.mpunchangle[1]);
	VectorCopy (cl.mpunchvector[0], cl.mpunchvector[1]);
	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	cl.mviewzoom[1] = cl.mviewzoom[0];

	// Baker: These protocols don't support viewzoom
	if (isin13 (cls.protocol,	PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,	PROTOCOL_QUAKE,
								PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,	PROTOCOL_NEHAHRABJP,
								PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,	PROTOCOL_DARKPLACES1,
								PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,	PROTOCOL_DARKPLACES4,
								PROTOCOL_DARKPLACES5)) {
		cl.stats[STAT_VIEWHEIGHT] = DEFAULT_VIEWHEIGHT;
		cl.stats[STAT_ITEMS] = 0;
		cl.stats[STAT_VIEWZOOM_21] = 255;
	}
	cl.idealpitch = 0;
	cl.mpunchangle[0][0] = 0;
	cl.mpunchangle[0][1] = 0;
	cl.mpunchangle[0][2] = 0;
	cl.mpunchvector[0][0] = 0;
	cl.mpunchvector[0][1] = 0;
	cl.mpunchvector[0][2] = 0;
	cl.mvelocity[0][0] = 0;
	cl.mvelocity[0][1] = 0;
	cl.mvelocity[0][2] = 0;
	cl.mviewzoom[0] = 1;

	bits = (unsigned short) MSG_ReadShort(&cl_message);

	// Baker: Fitz Ok
	if (bits & SU_EXTEND1_S15)
		bits |= (MSG_ReadByte(&cl_message) << 16);
	if (bits & SU_EXTEND2_S23)
		bits |= (MSG_ReadByte(&cl_message) << 24);

	if (bits & SU_VIEWHEIGHT)
		cl.stats[STAT_VIEWHEIGHT] = MSG_ReadChar(&cl_message);

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar(&cl_message);

	for (j = 0;j < 3;j++) {
		if (bits & (SU_PUNCH1 << j) ) {
			if (isin8 (cls.protocol,
						PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,	 PROTOCOL_QUAKE,
						PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,	PROTOCOL_NEHAHRABJP,
						PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3))
				cl.mpunchangle[0][j] = MSG_ReadChar(&cl_message);
			else
				cl.mpunchangle[0][j] = MSG_ReadAngle16i(&cl_message);
		}
		// Baker: fitz collides with SU_FITZ_WEAPON2_S16
		if ( (bits & (SU_PUNCHVEC1_S16 << j)) &&
			false == isin2(cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
			if (isin4 (cls.protocol, PROTOCOL_DARKPLACES1, PROTOCOL_DARKPLACES2, PROTOCOL_DARKPLACES3, PROTOCOL_DARKPLACES4))
				cl.mpunchvector[0][j] = MSG_ReadCoord16i(&cl_message);
			else
				cl.mpunchvector[0][j] = MSG_ReadCoord32f(&cl_message);
		}
		if (bits & (SU_VELOCITY1<<j) )
		{
			if (isin12 (cls.protocol,
						PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,	PROTOCOL_QUAKE,
						PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,	PROTOCOL_NEHAHRABJP,
						PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,	PROTOCOL_DARKPLACES1,
						PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,	PROTOCOL_DARKPLACES4))
				cl.mvelocity[0][j] = MSG_ReadChar(&cl_message)*16;
			else
				cl.mvelocity[0][j] = MSG_ReadCoord32f(&cl_message);
		}
	}

	// LadyHavoc: hipnotic demos don't have this bit set but should
	if (Have_Flag (bits, SU_ITEMS) ||
		isin13 (cls.protocol,
				PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,	PROTOCOL_QUAKE,
				PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,	PROTOCOL_NEHAHRABJP,
				PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,	PROTOCOL_DARKPLACES1,
				PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,	PROTOCOL_DARKPLACES4,
				PROTOCOL_DARKPLACES5) )
		cl.stats[STAT_ITEMS] = MSG_ReadLong(&cl_message);

	SET___ cl.onground = (bits & SU_ONGROUND) != 0; // Baker: svc_clientdata
	cl.inwater = (bits & SU_INWATER) != 0;

	if (cls.protocol == PROTOCOL_DARKPLACES5) {
		cl.stats[STAT_WEAPONFRAME] = (bits & SU_WEAPONFRAME) ? MSG_ReadShort(&cl_message) : 0;
		cl.stats[STAT_ARMOR] = (bits & SU_ARMOR) ? MSG_ReadShort(&cl_message) : 0;
		cl.stats[STAT_WEAPON] = (bits & SU_WEAPON) ? MSG_ReadShort(&cl_message) : 0;
		cl.stats[STAT_HEALTH] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_AMMO] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_SHELLS] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_NAILS] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_ROCKETS] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_CELLS] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_ACTIVEWEAPON] = (unsigned short) MSG_ReadShort(&cl_message);
	}
	else if (isin12 (cls.protocol, PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,
				PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,
				PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,
				PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
				PROTOCOL_DARKPLACES4)) {
		cl.stats[STAT_WEAPONFRAME] = (bits & SU_WEAPONFRAME) ? MSG_ReadByte(&cl_message) : 0;
		cl.stats[STAT_ARMOR] = (bits & SU_ARMOR) ? MSG_ReadByte(&cl_message) : 0;
		if (isin3 (cls.protocol, PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3))
			cl.stats[STAT_WEAPON] = (bits & SU_WEAPON) ? (unsigned short)MSG_ReadShort(&cl_message) : 0;
		else
			cl.stats[STAT_WEAPON] = (bits & SU_WEAPON) ? MSG_ReadByte(&cl_message) : 0;
		cl.stats[STAT_HEALTH] = MSG_ReadShort(&cl_message);
		cl.stats[STAT_AMMO] = MSG_ReadByte(&cl_message);
		cl.stats[STAT_SHELLS] = MSG_ReadByte(&cl_message);
		cl.stats[STAT_NAILS] = MSG_ReadByte(&cl_message);
		cl.stats[STAT_ROCKETS] = MSG_ReadByte(&cl_message);
		cl.stats[STAT_CELLS] = MSG_ReadByte(&cl_message);
		if (isin3 (gamemode, GAME_HIPNOTIC, GAME_ROGUE, GAME_QUOTH) || IS_OLDNEXUIZ_DERIVED(gamemode) )
			cl.stats[STAT_ACTIVEWEAPON] = (1<<MSG_ReadByte(&cl_message));
		else
			cl.stats[STAT_ACTIVEWEAPON] = MSG_ReadByte(&cl_message);

		if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999)) {
			//johnfitz -- PROTOCOL_FITZQUAKE
 			// DPD 999 CL_ParseClientdata 3
			if (bits & SU_FITZ_WEAPON2_S16)
				cl.stats[STAT_WEAPON] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_ARMOR2_S17)
				cl.stats[STAT_ARMOR] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_AMMO2_S18)
				cl.stats[STAT_AMMO] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_SHELLS2_S19)
				cl.stats[STAT_SHELLS] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_NAILS2_S20)
				cl.stats[STAT_NAILS] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_ROCKETS2_S21)
				cl.stats[STAT_ROCKETS] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_CELLS2_S22)
				cl.stats[STAT_CELLS] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_WEAPONFRAME2_24)
				cl.stats[STAT_WEAPONFRAME] |= (MSG_ReadByte(&cl_message) << 8);
			if (bits & SU_FITZ_WEAPONALPHA_S25) {
				//int gunalpha =
					MSG_ReadByte(&cl_message);
				//cl.viewent.alpha = gunalpha; // FITZ TODO
			}
			else {
				//cl.viewent.alpha = FITZ_ENTALPHA_DEFAULT_0;
			}
			//johnfitz

		}
	}

	// Baker: SU_VIEWZOOM_S19 collides with SU_FITZ_SHELLS2_S19
	if (Have_Flag (bits, SU_VIEWZOOM_S19) &&
		false == isin2(cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		if (isin3 (cls.protocol, PROTOCOL_DARKPLACES2, PROTOCOL_DARKPLACES3, PROTOCOL_DARKPLACES4) )
			cl.stats[STAT_VIEWZOOM_21] = MSG_ReadByte(&cl_message);
		else {
			if (developer_qw.integer)
				Con_PrintLinef ("STAT_VIEWZOOM_21");
			cl.stats[STAT_VIEWZOOM_21] = (unsigned short) MSG_ReadShort(&cl_message);
	}
	}

	// viewzoom interpolation
	cl.mviewzoom[0] = (float) max(cl.stats[STAT_VIEWZOOM_21], 2) * (1.0f / 255.0f);
}

/*
=====================
CL_ParseStatic
=====================
*/
static void CL_ParseStatic (int is_large_model_index, int fitz_version)
{
	// Baker: Fitz supports static and scale on these.
	entity_t *ent;
// HEREON
	if (cl.num_static_entities >= cl.max_static_entities)
		Host_Error_Line ("Too many static entities");
	ent = &cl.static_entities[cl.num_static_entities++];
	CL_ParseBaseline (ent, is_large_model_index, fitz_version, q_is_static_true);

	if (ent->state_baseline.modelindex == 0) {
		Con_DPrintLinef ("svc_parsestatic: static entity without model at %f %f %f", ent->state_baseline.origin[0], ent->state_baseline.origin[1], ent->state_baseline.origin[2]);
		cl.num_static_entities--;
		// This is definitely a cheesy way to conserve resources...
		return;
	}

// copy it to the current state
	ent->render.model = CL_GetModelByIndex(ent->state_baseline.modelindex);
	ent->render.framegroupblend[0].fb_frame = ent->state_baseline.frame;
	ent->render.framegroupblend[0].lerp = 1;
	// make torchs play out of sync
	ent->render.framegroupblend[0].start = lhrandom(-10, -1);
	ent->render.skinnum = ent->state_baseline.skin;
	ent->render.effects = ent->state_baseline.effects;
	ent->render.ralpha = ent->state_baseline.alpha * (1.0f / 255.0f);

	// Baker: alpha and scale?  At least for FitzQuake?   What about colormap?
	// Have we investigated Flint Ridge and the Hotel?

	//VectorCopy (ent->state_baseline.origin, ent->render.origin);
	//VectorCopy (ent->state_baseline.angles, ent->render.angles);

	Matrix4x4_CreateFromQuakeEntity(&ent->render.matrix, ent->state_baseline.origin[0], ent->state_baseline.origin[1], ent->state_baseline.origin[2], ent->state_baseline.angles[0], ent->state_baseline.angles[1], ent->state_baseline.angles[2], 1);
	ent->render.allowdecals = true;
	CL_UpdateRenderEntity(&ent->render);
}

/*
===================
CL_ParseStaticSound
===================
*/
static void CL_ParseStaticSound (int is_large_soundindex, int fitz_version)
{
	vec3_t		org;
	int			sound_num, vol, atten;

	MSG_ReadVector(&cl_message, org, cls.protocol);
	if (is_large_soundindex || fitz_version == 2)
		sound_num = (unsigned short) MSG_ReadShort(&cl_message);
	else
		sound_num = MSG_ReadByte(&cl_message);

	if (sound_num < 0 || sound_num >= MAX_SOUNDS_4096)
	{
		Con_PrintLinef ("CL_ParseStaticSound: sound_num(%d) >= MAX_SOUNDS_4096 (%d)", sound_num, MAX_SOUNDS_4096);
		return;
	}

	vol = MSG_ReadByte(&cl_message);
	atten = MSG_ReadByte(&cl_message);

	S_StaticSound (cl.sound_precache[sound_num], org, vol/255.0f, atten);
}

static void CL_ParseEffect (void)
{
	vec3_t		org;
	int			modelindex, startframe, framecount, framerate;

	MSG_ReadVector(&cl_message, org, cls.protocol);
	modelindex = MSG_ReadByte(&cl_message);
	startframe = MSG_ReadByte(&cl_message);
	framecount = MSG_ReadByte(&cl_message);
	framerate = MSG_ReadByte(&cl_message);

	CL_Effect(org, CL_GetModelByIndex(modelindex), startframe, framecount, framerate);
}

static void CL_ParseEffect2 (void)
{
	vec3_t		org;
	int			modelindex, startframe, framecount, framerate;

	MSG_ReadVector(&cl_message, org, cls.protocol);
	modelindex = (unsigned short) MSG_ReadShort(&cl_message);
	startframe = (unsigned short) MSG_ReadShort(&cl_message);
	framecount = MSG_ReadByte(&cl_message);
	framerate = MSG_ReadByte(&cl_message);

	CL_Effect(org, CL_GetModelByIndex(modelindex), startframe, framecount, framerate);
}

void CL_NewBeam (int ent, vec3_t start, vec3_t end, model_t *m, int lightning)
{
	int j;
	beam_t *b = NULL;

	if (ent >= MAX_EDICTS_32768)
	{
		Con_PrintLinef ("CL_NewBeam: invalid entity number %d", ent);
		ent = 0;
	}

	if (ent >= cl.max_entities)
		CL_ExpandEntities(ent);

	// override any beam with the same entity
	j = cl.max_beams;
	if (ent)
		for (j = 0, b = cl.beams;j < cl.max_beams;j++, b++)
			if (b->entity == ent)
				break;
	// if the entity was not found then just replace an unused beam
	if (j == cl.max_beams)
		for (j = 0, b = cl.beams;j < cl.max_beams;j++, b++)
			if (!b->model)
				break;
	if (j < cl.max_beams)
	{
		cl.num_beams = max(cl.num_beams, j + 1);
		b->entity = ent;
		b->lightning = lightning;
		b->model = m;
		b->endtime = cl.mtime[0] + 0.2;
		VectorCopy (start, b->start);
		VectorCopy (end, b->end);
	}
	else
		Con_DPrintLinef ("beam list overflow!");
}

static void CL_ParseBeam (model_t *m, int lightning)
{
	int ent;
	vec3_t start, end;

	ent = (unsigned short) MSG_ReadShort(&cl_message);
	MSG_ReadVector(&cl_message, start, cls.protocol);
	MSG_ReadVector(&cl_message, end, cls.protocol);

	if (ent >= MAX_EDICTS_32768)
	{
		Con_Printf ("CL_ParseBeam: invalid entity number %d\n", ent);
		ent = 0;
	}

	CL_NewBeam(ent, start, end, m, lightning);
}

static void CL_ParseTempEntity(void)
{
	int type;
	vec3_t pos, pos2;
	vec3_t vel1, vel2;
	vec3_t dir;
	vec3_t color;
	int rnd;
	int colorStart, colorLength, count;
	float velspeed, radius;
	unsigned char *tempcolor;
	matrix4x4_t tempmatrix;

	if (cls.protocol == PROTOCOL_QUAKEWORLD) {
		type = MSG_ReadByte(&cl_message);
		#include "cl_parse_temp_particles_quakeworld.c.h"
	}
	else
	{
		type = MSG_ReadByte(&cl_message);
		switch (type)
		{
		case TE_WIZSPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_WIZSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_wizhit, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_KNIGHTSPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_KNIGHTSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_knighthit, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_SPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
			}
			break;
		case TE_SPIKEQUAD:
			// quad spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SPIKEQUAD, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
			}
			break;
		case TE_SUPERSPIKE:
			// super spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SUPERSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
			}
			break;
		case TE_SUPERSPIKEQUAD:
			// quad super spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SUPERSPIKEQUAD, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
			}
			break;
			// LadyHavoc: added for improved blood splatters
		case TE_BLOOD:
			// blood puff
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			dir[0] = MSG_ReadChar(&cl_message);
			dir[1] = MSG_ReadChar(&cl_message);
			dir[2] = MSG_ReadChar(&cl_message);
			count = MSG_ReadByte(&cl_message);
			CL_ParticleEffect(EFFECT_TE_BLOOD, count, pos, pos, dir, dir, NULL, 0);
			break;
		case TE_SPARK:
			// spark shower
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			dir[0] = MSG_ReadChar(&cl_message);
			dir[1] = MSG_ReadChar(&cl_message);
			dir[2] = MSG_ReadChar(&cl_message);
			count = MSG_ReadByte(&cl_message);
			CL_ParticleEffect(EFFECT_TE_SPARK, count, pos, pos, dir, dir, NULL, 0);
			break;
		case TE_PLASMABURN:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_PLASMABURN, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;
			// LadyHavoc: added for improved gore
		case TE_BLOODSHOWER:
			// vaporized body
			MSG_ReadVector(&cl_message, pos, cls.protocol); // mins
			MSG_ReadVector(&cl_message, pos2, cls.protocol); // maxs
			velspeed = MSG_ReadCoord(&cl_message, cls.protocol); // speed
			count = (unsigned short) MSG_ReadShort(&cl_message); // number of particles
			vel1[0] = -velspeed;
			vel1[1] = -velspeed;
			vel1[2] = -velspeed;
			vel2[0] = velspeed;
			vel2[1] = velspeed;
			vel2[2] = velspeed;
			CL_ParticleEffect(EFFECT_TE_BLOOD, count, pos, pos2, vel1, vel2, NULL, 0);
			break;

		case TE_PARTICLECUBE:
			// general purpose particle effect
			MSG_ReadVector(&cl_message, pos, cls.protocol); // mins
			MSG_ReadVector(&cl_message, pos2, cls.protocol); // maxs
			MSG_ReadVector(&cl_message, dir, cls.protocol); // dir
			count = (unsigned short) MSG_ReadShort(&cl_message); // number of particles
			colorStart = MSG_ReadByte(&cl_message); // color
			colorLength = MSG_ReadByte(&cl_message); // gravity (1 or 0)
			velspeed = MSG_ReadCoord(&cl_message, cls.protocol); // randomvel
			CL_ParticleCube(pos, pos2, dir, count, colorStart, colorLength != 0, velspeed);
			break;

		case TE_PARTICLERAIN:
			// general purpose particle effect
			MSG_ReadVector(&cl_message, pos, cls.protocol); // mins
			MSG_ReadVector(&cl_message, pos2, cls.protocol); // maxs
			MSG_ReadVector(&cl_message, dir, cls.protocol); // dir
			count = (unsigned short) MSG_ReadShort(&cl_message); // number of particles
			colorStart = MSG_ReadByte(&cl_message); // color
			CL_ParticleRain(pos, pos2, dir, count, colorStart, 0);
			break;

		case TE_PARTICLESNOW:
			// general purpose particle effect
			MSG_ReadVector(&cl_message, pos, cls.protocol); // mins
			MSG_ReadVector(&cl_message, pos2, cls.protocol); // maxs
			MSG_ReadVector(&cl_message, dir, cls.protocol); // dir
			count = (unsigned short) MSG_ReadShort(&cl_message); // number of particles
			colorStart = MSG_ReadByte(&cl_message); // color
			CL_ParticleRain(pos, pos2, dir, count, colorStart, 1);
			break;

		case TE_GUNSHOT:
			// bullet hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_GUNSHOT, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (cl_sound_ric_gunshot.integer & RIC_GUNSHOT)
			{
				if (rand() % 5)
					S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
				else
				{
					rnd = rand() & 3;
					if (rnd == 1)
						S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
					else if (rnd == 2)
						S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
					else
						S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
				}
			}
			break;

		case TE_GUNSHOTQUAD:
			// quad bullet hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_GUNSHOTQUAD, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (cl_sound_ric_gunshot.integer & RIC_GUNSHOTQUAD)
			{
				if (rand() % 5)
					S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1, q_is_forceloop_false);
				else
				{
					rnd = rand() & 3;
					if (rnd == 1)
						S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1, q_is_forceloop_false);
					else if (rnd == 2)
						S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1, q_is_forceloop_false);
					else
						S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1, q_is_forceloop_false);
				}
			}
			break;

		case TE_EXPLOSION:
			// rocket explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_EXPLOSION, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_EXPLOSIONQUAD:
			// quad rocket explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_EXPLOSIONQUAD, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_EXPLOSION3:
			// Nehahra movie colored lighting explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			color[0] = MSG_ReadCoord(&cl_message, cls.protocol) * (2.0f / 1.0f);
			color[1] = MSG_ReadCoord(&cl_message, cls.protocol) * (2.0f / 1.0f);
			color[2] = MSG_ReadCoord(&cl_message, cls.protocol) * (2.0f / 1.0f);
			CL_ParticleExplosion(pos);
			Matrix4x4_CreateTranslate(&tempmatrix, pos[0], pos[1], pos[2]);
			CL_AllocLightFlash(NULL, &tempmatrix, 350, color[0], color[1], color[2], 700, 0.5, NULL, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_EXPLOSIONRGB:
			// colored lighting explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleExplosion(pos);
			color[0] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			color[1] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			color[2] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			Matrix4x4_CreateTranslate(&tempmatrix, pos[0], pos[1], pos[2]);
			CL_AllocLightFlash(NULL, &tempmatrix, 350, color[0], color[1], color[2], 700, 0.5, NULL, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_TAREXPLOSION:
			// tarbaby explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_TAREXPLOSION, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_SMALLFLASH:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_SMALLFLASH, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case TE_CUSTOMFLASH:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			radius = (MSG_ReadByte(&cl_message) + 1) * 8;
			velspeed = (MSG_ReadByte(&cl_message) + 1) * (1.0 / 256.0);
			color[0] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			color[1] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			color[2] = MSG_ReadByte(&cl_message) * (2.0f / 255.0f);
			Matrix4x4_CreateTranslate(&tempmatrix, pos[0], pos[1], pos[2]);
			CL_AllocLightFlash(NULL, &tempmatrix, radius, color[0], color[1], color[2], radius / velspeed, velspeed, NULL, -1, true, 1, 0.25, 1, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
			break;

		case TE_FLAMEJET:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			MSG_ReadVector(&cl_message, dir, cls.protocol);
			count = MSG_ReadByte(&cl_message);
			CL_ParticleEffect(EFFECT_TE_FLAMEJET, count, pos, pos, dir, dir, NULL, 0);
			break;

		case TE_LIGHTNING1:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt, true);
			break;

		case TE_LIGHTNING2:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt2, true);
			break;

		case TE_LIGHTNING3:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt3, false);
			break;

	// PGM 01/21/97
		case TE_BEAM:
			// grappling hook beam
			CL_ParseBeam(cl.model_beam, false);
			break;
	// PGM 01/21/97

	// LadyHavoc: for compatibility with the Nehahra movie...
		case TE_LIGHTNING4NEH:
			CL_ParseBeam(Mod_ForName(MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), true, false, NULL), false);
			break;

		case TE_LAVASPLASH:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_ParticleEffect(EFFECT_TE_LAVASPLASH, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case TE_TELEPORT:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_ParticleEffect(EFFECT_TE_TELEPORT, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case TE_EXPLOSION2:
			// color mapped explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			colorStart = MSG_ReadByte(&cl_message);
			colorLength = MSG_ReadByte(&cl_message);
			if (colorLength == 0)
				colorLength = 1;
			CL_ParticleExplosion2(pos, colorStart, colorLength);
			tempcolor = palette_rgb[(rand()%colorLength) + colorStart];
			color[0] = tempcolor[0] * (2.0f / 255.0f);
			color[1] = tempcolor[1] * (2.0f / 255.0f);
			color[2] = tempcolor[2] * (2.0f / 255.0f);
			Matrix4x4_CreateTranslate(&tempmatrix, pos[0], pos[1], pos[2]);
			CL_AllocLightFlash(NULL, &tempmatrix, 350, color[0], color[1], color[2], 700, 0.5, NULL, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_TEI_G3:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			MSG_ReadVector(&cl_message, pos2, cls.protocol);
			MSG_ReadVector(&cl_message, dir, cls.protocol);
			CL_ParticleTrail(EFFECT_TE_TEI_G3, 1, pos, pos2, dir, dir, NULL, 0, true, true, NULL, NULL, 1);
			break;

		case TE_TEI_SMOKE:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			MSG_ReadVector(&cl_message, dir, cls.protocol);
			count = MSG_ReadByte(&cl_message);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_TEI_SMOKE, count, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case TE_TEI_BIGEXPLOSION:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_TEI_BIGEXPLOSION, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1, q_is_forceloop_false);
			break;

		case TE_TEI_PLASMAHIT:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			MSG_ReadVector(&cl_message, dir, cls.protocol);
			count = MSG_ReadByte(&cl_message);
			CL_FindNonSolidLocation(pos, pos, 5);
			CL_ParticleEffect(EFFECT_TE_TEI_PLASMAHIT, count, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		default:
			Host_Error_Line ("CL_ParseTempEntity: bad type %d (hex %02X)", type, type);
		}
	}
}

static void CL_ParseTrailParticles(void)
{
	int entityindex;
	int effectindex;
	vec3_t start, end;
	entityindex = (unsigned short)MSG_ReadShort(&cl_message);
	if (entityindex >= MAX_EDICTS_32768)
		entityindex = 0;
	if (entityindex >= cl.max_entities)
		CL_ExpandEntities(entityindex);
	effectindex = (unsigned short)MSG_ReadShort(&cl_message);
	MSG_ReadVector(&cl_message, start, cls.protocol);
	MSG_ReadVector(&cl_message, end, cls.protocol);
	CL_ParticleTrail(effectindex, 1, start, end, vec3_origin, vec3_origin, entityindex > 0 ? cl.entities + entityindex : NULL, 0, true, true, NULL, NULL, 1);
}

static void CL_ParsePointParticles(void)
{
	int effectindex, count;
	vec3_t origin, velocity;
	effectindex = (unsigned short)MSG_ReadShort(&cl_message);
	MSG_ReadVector(&cl_message, origin, cls.protocol);
	MSG_ReadVector(&cl_message, velocity, cls.protocol);
	count = (unsigned short)MSG_ReadShort(&cl_message);
	CL_ParticleEffect(effectindex, count, origin, origin, velocity, velocity, NULL, 0);
}

static void CL_ParsePointParticles1(void)
{
	int effectindex;
	vec3_t origin;
	effectindex = (unsigned short)MSG_ReadShort(&cl_message);
	MSG_ReadVector(&cl_message, origin, cls.protocol);
	CL_ParticleEffect(effectindex, 1, origin, origin, vec3_origin, vec3_origin, NULL, 0);
}

typedef struct cl_iplog_item_s
{
	char *address;
	char *name;
}
cl_iplog_item_t;

static qbool cl_iplog_loaded = false;
static int cl_iplog_numitems = 0;
static int cl_iplog_maxitems = 0;
static cl_iplog_item_t *cl_iplog_items;

static void CL_IPLog_Load(void);
static void CL_IPLog_Add(const char *address, const char *name, qbool checkexisting, qbool addtofile)
{
	int j;
	size_t sz_name, sz_address;
	if (!address || !address[0] || !name || !name[0])
		return;
	if (!cl_iplog_loaded)
		CL_IPLog_Load();
	if (developer_extra.integer)
		Con_DPrintf ("CL_IPLog_Add(" QUOTED_S ", " QUOTED_S ", %d, %d);\n", address, name, checkexisting, addtofile);
	// see if it already exists
	if (checkexisting)
	{
		for (j = 0;j < cl_iplog_numitems;j++)
		{
			if (String_Match(cl_iplog_items[j].address, address) && String_Match(cl_iplog_items[j].name, name))
			{
				if (developer_extra.integer)
					Con_DPrintLinef ("... found existing " QUOTED_S " " QUOTED_S, cl_iplog_items[j].address, cl_iplog_items[j].name);
				return;
			}
		}
	}
	// it does not already exist in the iplog, so add it
	if (cl_iplog_maxitems <= cl_iplog_numitems || !cl_iplog_items)
	{
		cl_iplog_item_t *olditems = cl_iplog_items;
		cl_iplog_maxitems = max(1024, cl_iplog_maxitems + 256);
		cl_iplog_items = (cl_iplog_item_t *) Mem_Alloc(cls.permanentmempool, cl_iplog_maxitems * sizeof(cl_iplog_item_t));
		if (olditems)
		{
			if (cl_iplog_numitems)
				memcpy(cl_iplog_items, olditems, cl_iplog_numitems * sizeof(cl_iplog_item_t));
			Mem_Free(olditems);
		}
	}
	sz_address = strlen(address) + 1;
	sz_name = strlen(name) + 1;
	cl_iplog_items[cl_iplog_numitems].address = (char *) Mem_Alloc(cls.permanentmempool, sz_address);
	cl_iplog_items[cl_iplog_numitems].name = (char *) Mem_Alloc(cls.permanentmempool, sz_name);
	strlcpy(cl_iplog_items[cl_iplog_numitems].address, address, sz_address);
	// TODO: maybe it would be better to strip weird characters from name when
	// copying it here rather than using a straight strcpy?
	strlcpy(cl_iplog_items[cl_iplog_numitems].name, name, sz_name);
	cl_iplog_numitems++;
	if (addtofile)
	{
		// add it to the iplog.txt file
		// TODO: this ought to open the one in the userpath version of the base
		// gamedir, not the current gamedir
// not necessary for mobile
#ifndef DP_MOBILETOUCH
		Log_Printf(cl_iplog_name.string, "%s %s\n", address, name);
		if (developer_extra.integer)
			Con_DPrintf ("CL_IPLog_Add: appending this line to %s: %s %s\n", cl_iplog_name.string, address, name);
#endif
	}
}

static void CL_IPLog_Load(void)
{
	int j, len, linenumber;
	char *text, *textend;
	unsigned char *filedata;
	fs_offset_t filesize;
	char line[MAX_INPUTLINE_16384];
	char address[MAX_INPUTLINE_16384];
	cl_iplog_loaded = true;
	// TODO: this ought to open the one in the userpath version of the base
	// gamedir, not the current gamedir
// not necessary for mobile
#ifndef DP_MOBILETOUCH
	filedata = FS_LoadFile(cl_iplog_name.string, tempmempool, fs_quiet_true, &filesize);
#else
	filedata = NULL;
#endif
	if (!filedata)
		return;
	text = (char *)filedata;
	textend = text + filesize;
	for (linenumber = 1;text < textend;linenumber++)
	{
		for (len = 0;text < textend && *text != '\r' && *text != '\n';text++)
			if (len < (int)sizeof(line) - 1)
				line[len++] = *text;
		line[len] = 0;
		if (text < textend && *text == '\r' && text[1] == '\n')
			text++;
		if (text < textend && *text == '\n')
			text++;
		if (line[0] == '/' && line[1] == '/')
			continue; // skip comments if anyone happens to add them
		for (j = 0;j < len && !ISWHITESPACE(line[j]);j++)
			address[j] = line[j];
		address[j] = 0;
		// skip exactly one space character
		j++;
		// address contains the address with termination,
		// line + j contains the name with termination
		if (address[0] && line[j])
			CL_IPLog_Add(address, line + j, false, false);
		else
			Con_PrintLinef ("%s:%d: could not parse address and name:" NEWLINE "%s", cl_iplog_name.string, linenumber, line);
	}
}

static void CL_IPLog_List_f(cmd_state_t *cmd)
{
	int i, j;
	const char *addressprefix;
	if (Cmd_Argc(cmd) > 2)
	{
		Con_Printf ("usage: %s 123.456.789.\n", Cmd_Argv(cmd, 0));
		return;
	}
	addressprefix = "";
	if (Cmd_Argc(cmd) >= 2)
		addressprefix = Cmd_Argv(cmd, 1);
	if (!cl_iplog_loaded)
		CL_IPLog_Load();
	if (addressprefix && addressprefix[0])
		Con_PrintLinef ("Listing iplog addresses beginning with %s", addressprefix);
	else
		Con_PrintLinef ("Listing all iplog entries");
	Con_PrintLinef ("address         name");
	for (i = 0;i < cl_iplog_numitems;i++)
	{
		if (addressprefix && addressprefix[0])
		{
			for (j = 0;addressprefix[j];j++)
				if (addressprefix[j] != cl_iplog_items[i].address[j])
					break;
			// if this address does not begin with the addressprefix string
			// simply omit it from the output
			if (addressprefix[j])
				continue;
		}
		// if name is less than 15 characters, left justify it and pad
		// if name is more than 15 characters, print all of it, not worrying
		// about the fact it will misalign the columns
		if (strlen(cl_iplog_items[i].address) < 15)
			Con_Printf ("%-15s %s\n", cl_iplog_items[i].address, cl_iplog_items[i].name);
		else
			Con_Printf ("%5s %s\n", cl_iplog_items[i].address, cl_iplog_items[i].name);
	}
}

// look for anything interesting like player IP addresses or ping reports
static qbool CL_ExaminePrintString(const char *text)
{
	int len;
	const char *t;
	char temp[MAX_INPUTLINE_16384];
	if (String_Match(text, "Client ping times:\n")) {
		cl.parsingtextmode = CL_PARSETEXTMODE_PING;
		// hide ping reports in demos
		if (cls.demoplayback)
			cl.parsingtextexpectingpingforscores = 1;
		for(cl.parsingtextplayerindex = 0; cl.parsingtextplayerindex < cl.maxclients && !cl.scores[cl.parsingtextplayerindex].name[0]; cl.parsingtextplayerindex++)
			;
		if (cl.parsingtextplayerindex >= cl.maxclients) // should never happen, since the client itself should be in cl.scores
		{
			Con_PrintLinef ("ping reply but empty scoreboard?!?");
			cl.parsingtextmode = CL_PARSETEXTMODE_NONE;
			cl.parsingtextexpectingpingforscores = 0;
		}
		cl.parsingtextexpectingpingforscores = cl.parsingtextexpectingpingforscores ? 2 : 0;
		return !cl.parsingtextexpectingpingforscores;
	}

	if (String_Starts_With(text, "host:    " /*, 9*/)) {
		// cl.parsingtextexpectingpingforscores = false; // really?
		cl.parsingtextmode = CL_PARSETEXTMODE_STATUS;
		cl.parsingtextplayerindex = 0;
		return true;
	}

	if (cl.parsingtextmode == CL_PARSETEXTMODE_PING) {
		// if anything goes wrong, we'll assume this is not a ping report
		qbool expected = cl.parsingtextexpectingpingforscores != 0;
		cl.parsingtextexpectingpingforscores = 0;
		cl.parsingtextmode = CL_PARSETEXTMODE_NONE;
		t = text;

		while (*t == ' ')
			t++;

		if ((*t >= '0' && *t <= '9') || *t == '-') {
			int ping = atoi(t);
			while ((*t >= '0' && *t <= '9') || *t == '-')
				t++;
			if (*t == ' ') {
				int charindex = 0;
				t++;
				if (cl.parsingtextplayerindex < cl.maxclients) {
					for (charindex = 0;cl.scores[cl.parsingtextplayerindex].name[charindex] == t[charindex];charindex++)
						;
					// note: the matching algorithm stops at the end of the player name because some servers append text such as " READY" after the player name in the scoreboard but not in the ping report
					//if (cl.scores[cl.parsingtextplayerindex].name[charindex] == 0 && t[charindex] == '\n')
					if (t[charindex] == '\n') {
						cl.scores[cl.parsingtextplayerindex].qw_ping = bound(0, ping, 9999);
						for (cl.parsingtextplayerindex++;cl.parsingtextplayerindex < cl.maxclients && !cl.scores[cl.parsingtextplayerindex].name[0];cl.parsingtextplayerindex++)
							;
						//if (cl.parsingtextplayerindex < cl.maxclients) // we could still get unconnecteds!
						{
							// we parsed a valid ping entry, so expect another to follow
							cl.parsingtextmode = CL_PARSETEXTMODE_PING;
							cl.parsingtextexpectingpingforscores = expected;
						}
						return !expected;
					}
				}
				if (String_Starts_With_PRE(t, "unconnected\n"/*, 12*/)) {
					// just ignore
					cl.parsingtextmode = CL_PARSETEXTMODE_PING;
					cl.parsingtextexpectingpingforscores = expected;
					return !expected;
				}
				else
					Con_DPrintLinef ("player names '%s' and '%s' didn't match", cl.scores[cl.parsingtextplayerindex].name, t);
			}
		}
	}

	if (cl.parsingtextmode == CL_PARSETEXTMODE_STATUS) {
		if (String_Starts_With_PRE(text, "players: " /*, 9*/)) {
			cl.parsingtextmode = CL_PARSETEXTMODE_STATUS_PLAYERID;
			cl.parsingtextplayerindex = 0;
			return true;
		}
		else if (!strstr(text, ": ")) {
			cl.parsingtextmode = CL_PARSETEXTMODE_NONE; // status report ended
			return true;
		}
	}

	if (cl.parsingtextmode == CL_PARSETEXTMODE_STATUS_PLAYERID) {
		// if anything goes wrong, we'll assume this is not a status report
		cl.parsingtextmode = CL_PARSETEXTMODE_NONE;
		if (text[0] == '#' && text[1] >= '0' && text[1] <= '9') {
			t = text + 1;
			cl.parsingtextplayerindex = atoi(t) - 1;
			while (*t >= '0' && *t <= '9')
				t++;
			if (*t == ' ') {
				cl.parsingtextmode = CL_PARSETEXTMODE_STATUS_PLAYERIP;
				return true;
			}
			// the player name follows here, along with frags and time
		}
	}

	if (cl.parsingtextmode == CL_PARSETEXTMODE_STATUS_PLAYERIP) {
		// if anything goes wrong, we'll assume this is not a status report
		cl.parsingtextmode = CL_PARSETEXTMODE_NONE;
		if (text[0] == ' ') {
			t = text;
			while (*t == ' ')
				t++;
			for (len = 0;*t && *t != '\n';t++)
				if (len < (int)sizeof(temp) - 1)
					temp[len++] = *t;
			temp[len] = 0;
			// botclient is perfectly valid, but we don't care about bots
			// also don't try to look up the name of an invalid player index
			if (String_Match(temp, "botclient") == false   //  strcmp(temp, "botclient")
			 && cl.parsingtextplayerindex >= 0
			 && cl.parsingtextplayerindex < cl.maxclients
			 && cl.scores[cl.parsingtextplayerindex].name[0]) {
				// log the player name and IP address string
				// (this operates entirely on strings to avoid issues with the
				//  nature of a network address)
				CL_IPLog_Add(temp, cl.scores[cl.parsingtextplayerindex].name, true, true);
			}
			cl.parsingtextmode = CL_PARSETEXTMODE_STATUS_PLAYERID;
			return true;
		}
	}
	return true;
}

extern cvar_t host_timescale;
extern cvar_t cl_lerpexcess;

WARP_X_ (CL_ParseServerMessage svc_time also QW CL_ParseServerMessage)
static void CL_NetworkTimeReceived(double newtime)
{
	cl.mtime[1] = cl.mtime[0];
	cl.mtime[0] = newtime;
	if (cl_nolerp.integer || cls.timedemo || cl.mtime[1] == cl.mtime[0] || cls.signon < SIGNONS_4)
		cl.time = cl.mtime[1] = newtime;
	else if (cls.demoplayback)
	{
		// when time falls behind during demo playback it means the cl.mtime[1] was altered
		// due to a large time gap, so treat it as an instant change in time
		// (this can also happen during heavy packet loss in the demo)
		if (cl.time < newtime - 0.1)
			cl.mtime[1] = cl.time = newtime;
	}
	else if (cls.protocol != PROTOCOL_QUAKEWORLD)
	{
		double timehigh = 0; // hush compiler warning
		cl.mtime[1] = max(cl.mtime[1], cl.mtime[0] - 0.1);

		if (developer_extra.integer && vid_activewindow)
		{
			if (cl.time < cl.mtime[1] - (cl.mtime[0] - cl.mtime[1]))
				Con_DPrintf ("--- cl.time < cl.mtime[1] (%f < %f ... %f)\n", cl.time, cl.mtime[1], cl.mtime[0]);
			else if (cl.time > cl.mtime[0] + (cl.mtime[0] - cl.mtime[1]))
				Con_DPrintf ("--- cl.time > cl.mtime[0] (%f > %f ... %f)\n", cl.time, cl.mtime[1], cl.mtime[0]);
		}

		 // Baker: This is not the norm cl_nettimesyncboundmode defaults 6
		if (cl_nettimesyncboundmode.integer < 4) {
			// doesn't make sense for modes > 3
			cl.time += (cl.mtime[1] - cl.time) * bound(0, cl_nettimesyncfactor.value, 1);
			timehigh = cl.mtime[1] + (cl.mtime[0] - cl.mtime[1]) * cl_nettimesyncboundtolerance.value;
		}

		switch (cl_nettimesyncboundmode.integer)
		{
		case 1:
			cl.time = bound(cl.mtime[1], cl.time, cl.mtime[0]);
			break;

		case 2:
			if (cl.time < cl.mtime[1] || cl.time > timehigh)
				cl.time = cl.mtime[1];
			break;

		case 3:
			if ((cl.time < cl.mtime[1] && cl.oldtime < cl.mtime[1]) || (cl.time > timehigh && cl.oldtime > timehigh))
				cl.time = cl.mtime[1];
			break;

		case 4:
			if (fabs(cl.time - cl.mtime[1]) > 0.5)
				cl.time = cl.mtime[1]; // reset
			else if (fabs(cl.time - cl.mtime[1]) > 0.1)
				cl.time += 0.5 * (cl.mtime[1] - cl.time); // fast
			else if (cl.time > cl.mtime[1])
				cl.time -= 0.002 * cl.movevars_timescale; // fall into the past by 2ms
			else
				cl.time += 0.001 * cl.movevars_timescale; // creep forward 1ms
			break;

		case 5:
			if (fabs(cl.time - cl.mtime[1]) > 0.5)
				cl.time = cl.mtime[1]; // reset
			else if (fabs(cl.time - cl.mtime[1]) > 0.1)
				cl.time += 0.5 * (cl.mtime[1] - cl.time); // fast
			else
				cl.time = bound(cl.time - 0.002 * cl.movevars_timescale, cl.mtime[1], cl.time + 0.001 * cl.movevars_timescale);
			break;

		case 6: // Baker: This is the norm -- cl_nettimesyncboundmode defaults 6
			cl.time = bound(cl.mtime[1], cl.time, cl.mtime[0]);
			cl.time = bound(cl.time - 0.002 * cl.movevars_timescale,
				cl.mtime[1], cl.time + 0.001 * cl.movevars_timescale);
			break;

		case 7:
			/* bones_was_here: this aims to prevent disturbances in the force from affecting cl.time
			 * the rolling harmonic mean gives large time error outliers low significance
			 * correction rate is dynamic and gradual (max 10% of mean error per tic)
			 * time is correct within a few server frames of connect/map start
			 * can achieve microsecond accuracy when cl.realframetime is a multiple of sv.frametime
			 * prevents 0ms move frame times with uncapped fps
			 * smoothest mode esp. for vsynced clients on servers with aggressive inputtimeout
			 */
			{
				unsigned char j;
				float error;
				// in event of packet loss, cl.mtime[1] could be very old, so avoid if possible
				double target = cl.movevars_ticrate ? cl.mtime[0] - cl.movevars_ticrate : cl.mtime[1];
				cl.ts_error_stor[cl.ts_error_num] = 1.0f / max(fabs(cl.time - target), FLT_MIN);
				cl.ts_error_num = (cl.ts_error_num + 1) % NUM_TS_ERRORS;
				for (j = 0, error = 0.0f; j < NUM_TS_ERRORS; j++)
					error += cl.ts_error_stor[j];
				error = 0.1f / (error / NUM_TS_ERRORS);
				cl.time = bound(cl.time - error, target, cl.time + error);
			}
			break;
		}
	}
	// this packet probably contains a player entity update, so we will need
	// to update the prediction
	cl.movement_replay = true;
	// this may get updated later in parsing by svc_clientdata
	SET___ cl.onground = false; // We received a packet
	// if true the cl.viewangles are interpolated from cl.mviewangles[]
	// during this frame
	// (makes spectating players much smoother and prevents mouse movement from turning)
	cl.fixangle[1] = cl.fixangle[0];
	cl.fixangle[0] = false;

	if (!cls.demoplayback) // Baker: Super waldo
		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
	// update the csqc's server timestamps, critical for proper sync
	CSQC_UpdateNetworkTimes (cl.mtime[0], cl.mtime[1]);

#ifdef USEODE
	if (cl.mtime[0] > cl.mtime[1])
		World_Physics_Frame(&cl.world, cl.mtime[0] - cl.mtime[1], cl.movevars_gravity);
#endif

	// only lerp entities that also get an update in this frame, when lerp excess is used

	// Baker: This is not the norm -- cl_lerpexcess defaults 0
	if (cl_lerpexcess.value > 0) {
		int j;
		for (j = 1;j < cl.num_entities;j++) {
			if (cl.entities_active[j]) {
				entity_t *ent = cl.entities + j;
				ent->persistent.lerpdeltatime = 0;
			}
		} // for
	} // if
}

#define SHOWNET(x) if (cl_shownet.integer==2)Con_PrintLinef ("%3d:%s(%d)", cl_message.readcount-1, x, cmd);

WARP_X_CALLERS_ (CL_OBJModelAdjust2_f)


void Tokenize_Console_16384_FreeContents (tokenize_console_s *tcx)
{
	int max_argc2 = (int)ARRAY_COUNT(tcx->tokens_za);
	for (int j = 0; j < max_argc2; j ++) {
		if (tcx->tokens_za[j]) {
			mfreenull_ (tcx->tokens_za[j]);
		}
	} // for

	int sizeof_tcx = sizeof(*tcx);
	memset (tcx, 0, sizeof(*tcx));
}

int Tokenize_Console_16384_Za_Return_Argc (tokenize_console_s *tcx, ccs *s)
{
	Tokenize_Console_16384_FreeContents (tcx);

	c_strlcpy (tcx->tokenize_string, s);
	
	const char *p = tcx->tokenize_string;
	//int max_argc = (int)(sizeof(tokens)/sizeof(tokens[0]));
	int max_argc2 = (int)ARRAY_COUNT(tcx->tokens_za);
	tcx->num_tokens = 0;
	for (;;) {
		if (tcx->num_tokens >= max_argc2)
			break;

		// skip whitespace here to find token start pos
		while(*p && ISWHITESPACE(*p))
			++p;

		tcx->tokens_startpos[tcx->num_tokens] = p - tcx->tokenize_string;
		if (!COM_ParseToken_Console(&p))
			break;
		
		tcx->tokens_endpos[tcx->num_tokens] = p - tcx->tokenize_string;
		tcx->tokens_za[tcx->num_tokens] = Z_StrDup (com_token);
		tcx->num_tokens ++;
	}

	int argc = tcx->num_tokens;
	return argc;
}


static	char	ztoken_string[MAX_INPUTLINE_16384];
static	char *	ztoken_argv[MAX_INPUTLINE_16384 / 2];
static	int		ztoken_argc = 0;

WARP_X_CALLERS_ (CL_ParseHint)
int ztokenize_console_argc (const char *s_yourline)
{
	char *p;

	char *z_startpos[sizeof(ztoken_string) / 2];
	//char *z_endpos[sizeof(ztoken_string) / 2];
	int z_max_count = ARRAY_COUNT(ztoken_argv);

	c_strlcpy (ztoken_string, s_yourline);
	p = ztoken_string;

	ztoken_argc = 0;
	while (1) {
		if (ztoken_argc >= (int)z_max_count)
			break;

		// skip whitespace here to find token start pos
		while (*p && ISWHITESPACE(*p))
			p ++;

		z_startpos[ztoken_argc] = p;// p - ztoken_string;

		// Fills or uses com_token global
		// Q: Does this tear up our string?
		// Q: Is p a pointer to the arg?
		// Q: Is p a pointer to a arg in OUR string?
		// Q: Do we need to null terminate or is this done for us?
		if (!COM_ParseToken_Console( (const char **)&p))
			break;
		//z_endpos[ztoken_argc] = p - ztoken_string;
		ztoken_argv[ztoken_argc] = z_startpos[ztoken_argc];
		*p = 0; // Baker: term
		p++; // Baker: Skip
		//tokens[num_tokens] = PRVM_SetTempString(prog, com_token);
		ztoken_argc ++;
	};

	return ztoken_argc;
}

/*
==================
CL_ParseLocalSound - for 2021 rerelease
==================
*/
void CL_ParseLocalSound (void) // AURA 1.0
{
	int field_mask, sound_num;

	field_mask = MSG_ReadByte(&cl_message);

	sound_num = Have_Flag(field_mask, SND_LARGESOUND_16) ?
		(unsigned short) MSG_ReadShort(&cl_message) :
		MSG_ReadByte(&cl_message);

	if (sound_num >= MAX_SOUNDS_4096) {
		Con_PrintLinef ("CL_ParseLocalSound: sound_num (%d) >= MAX_SOUNDS_4096 (%d)", sound_num, MAX_SOUNDS_4096);
		return;
	}

	S_LocalSound (cl.sound_precache[sound_num]->name);
}

// hint skill 1
// hint game quake3_quake1
// hint qex 1
// We know there is a hint.  str is the text after the hint
void CL_ParseHint (const char *str)
{
	char		textbuf[64];
	char		*scursor;
	const char	*scmd_arg0 = textbuf;

	c_strlcpy (textbuf, str);

	// Kill newlines ...
	scursor = textbuf;
	while (*scursor) {
		if (*scursor == NEWLINE_CHAR_10)
			*scursor = 0;
		scursor ++;
	}

	// find arg
	scursor = textbuf;
	while (*scursor > SPACE_CHAR_32)
		scursor++;

	// hint skill 1
	// hint game quake3_quake1
	// hint qex 1
	if (scursor[0] == SPACE_CHAR_32 && scursor[1] > SPACE_CHAR_32) {
		// Found arg2
		scursor[0] = 0; // null after cmd
		const char	*scmd_arg1 = &scursor[1];

		     if (String_Match (scmd_arg0, "skill"))	{ cl.skill_level_p1 = atoi(scmd_arg1) + 1;	}
		else if (String_Match (scmd_arg0, "qex"))		{ cl.is_qex = atoi(scmd_arg1);				} // AURA 1.1
		else if (String_Match (scmd_arg0, "zircon_ext"))	{ // ZIRCON_PEXT
			WARP_X_ (CLIENT_SUPPORTED_ZIRCON_EXT, SV_Zircon_Extensions_Send,  CL_BeginDownloads_Prespawn_Zircon_Extensions_Send)
			cls.zirconprotocolextensions = atoi (scmd_arg1);
			if (developer_zext.integer)
				Con_PrintLinef ("CL_StuffText: Server reporting shared zirconprotocolextensions %d", cls.zirconprotocolextensions);
		}
		else if (String_Match (scmd_arg0, "stor"))	{
			// ZIRCON_EXT_STATIC_ENT_ALPHA_COLORMOD_SCALE_32
			// stor [number of units max is 14]
			int c = ztokenize_console_argc(scmd_arg1);
			if (c) {
				int num_stor = atoi(ztoken_argv[0]); // 6
				if (num_stor + 1 == c) {
					WARP_X_ (CL_ParseBaseline)
					num_stor = bound (0, num_stor, (int)ARRAY_COUNT(cls.storr) - 1);
					cls.storr[0] = num_stor; // Indicate the count
					for (int xn = 0; xn < num_stor; xn ++) {
						cls.storr[1 + xn] = atoi (ztoken_argv[1 + xn]);
					} // for
					//cls.storr[0] = 0;
				} else {
					Con_PrintLinef ("stor " QUOTED_S " says count of %d + 1 but we count %d", scmd_arg1, num_stor, ztoken_argc);
				}

			} // c
		}
		else if (String_Match (scmd_arg0, "game"))		{
			/* todo */
			char gamedir[1][MAX_QPATH_128];
  			//gamedir_change:
			const char *s_base = fs_numgamedirs ? fs_gamedirs[0] : gamedirname1;
			c_strlcpy(gamedir[0], scmd_arg1);
			if (false == String_Match ( s_base, gamedir[0]) && cls.signon < SIGNONS_4) {
				if (!FS_ChangeGameDirs(1, gamedir /*gamedir*/, q_tx_complain_true, q_fail_on_missing_false))
					Host_Error_Line ("CL_ParseServerInfo: unable to switch to server specified gamedir");
			} // Game change
		}
		else
			return;

		Con_DPrintLinef ("Game hint: " QUOTED_S " to " QUOTED_S, scmd_arg0, scmd_arg1);
	}
}

// Baker: We read but do nothing with it
// We don't expect to find this in the wild.
void Fog_ParseServerMessage (void)
{
	float density, red, green, blue, time;

	density = MSG_ReadByte(&cl_message) / 255.0;
	red = MSG_ReadByte(&cl_message) / 255.0;
	green = MSG_ReadByte(&cl_message) / 255.0;
	blue = MSG_ReadByte(&cl_message) / 255.0;
	time = MSG_ReadShort(&cl_message) / 100.0;
//	if (time < 0.0f) time = 0.0f;

//	Fog_Update (density, red, green, blue, time);
}

// Baker r8191
/*
=====================
CL_ParseServerMessage
=====================
*/
int parsingerror = false;
WARP_X_ ()
void CL_ParseServerMessage(void)
{
	int			cmd;
	int			j, ent_num;
	protocolversion_t protocol;
	unsigned char		cmdlog[32];
	const char		*cmdlogname[32], *str;
	int			cmdindex, cmdcount = 0;
	qbool	qwplayerupdatereceived;
	qbool	strip_pqc;
	char vabuf[1024];

	// LadyHavoc: moved demo message writing from before the packet parse to
	// after the packet parse so that CL_Stop_f can be called by cl_autodemo
	// code in CL_ParseServerinfo
	//if (cls.demorecording)
	//	CL_WriteDemoMessage (&cl_message);

	cl.last_received_message = host.realtime;

	CL_KeepaliveMessage(false);

//
// if recording demos, copy the message out
//
	if (cl_shownet.integer == 1)
		Con_PrintLinef ("%f %d", host.realtime, cl_message.cursize);
	else if (cl_shownet.integer == 2)
		Con_Print("------------------\n");

//
// parse the message
//
	//MSG_BeginReading ();

	parsingerror = true;

	if (cls.protocol == PROTOCOL_QUAKEWORLD) {
		#include "cl_parse_quakeworld.c.h"
		goto quakeworld_skip;
	}

	// Baker: How does this loop exit? cmd == -1
	while (1) {
		if (cl_message.badread)
			Host_Error_Line ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte(&cl_message);

		if (cmd == -1) {
//			R_TimeReport("END OF MESSAGE");
			SHOWNET("END OF MESSAGE");
			break;		// end of message
		}

		cmdindex = cmdcount & 31;
		cmdcount++;
		cmdlog[cmdindex] = cmd;

		// if the high bit of the command byte is set, it is a fast update (Baker: and that is what?)
		if (cmd & U_SIGNAL_128) { // just differentiates from other updates
			// Baker: This happens a lot when entities are updated for
			// Quake protocol and FitzQuake protocols.  May not happen with DP7
			// LadyHavoc: fix for bizarre problem in MSVC that I do not understand (if I assign the string pointer directly it ends up storing a NULL pointer)
			str = "entity";
			cmdlogname[cmdindex] = str;
			SHOWNET ("fast update");
			if (cls.signon == SIGNONS_4 - 1) {
				// first update is the final signon stage
				cls.signon = SIGNONS_4; // NORMAL QUAKE
				cls.world_frames = 0; cls.world_start_realtime = 0;
				CL_SignonReply ();
			}
			EntityFrameQuake_ReadEntity (cmd&127); // Baker: DP7 is not supposed to come here.
			continue;
		}

		SHOWNET(svc_strings[cmd]);
		cmdlogname[cmdindex] = svc_strings[cmd];
		if (!cmdlogname[cmdindex]) {
			// LadyHavoc: fix for bizarre problem in MSVC that I do not understand (if I assign the string pointer directly it ends up storing a NULL pointer)
			const char *d = "<unknown>";
			cmdlogname[cmdindex] = d;
		}

		if (developer_svc.value) {
			// Skip certain spammy svcs unless developer_svc is 2 or higher
			if (developer_svc.value >= 2 || false == isin4 (cmd, svc_nop, svc_time, svc_clientdata,svc_entities))
				Con_PrintLinef ("svc: %s", cmdlogname[cmdindex]);
		}

		// other commands
		switch (cmd) {
		default:
			{
				char description[32*64], tempdesc[64];
				int count;
				c_strlcpy (description, "packet dump: ");
				j = cmdcount - 32;
				if (j < 0)
					j = 0;
				count = cmdcount - j;
				j &= 31;
				while(count > 0)
				{
					dpsnprintf (tempdesc, sizeof (tempdesc), "%3d:%s ", cmdlog[j], cmdlogname[j]);
					c_strlcat (description, tempdesc);
					count--;
					j++;
					j &= 31;
				}
				description[strlen(description)-1] = '\n'; // replace the last space with a newline
				Con_Print(description);
				Host_Error_Line ("CL_ParseServerMessage: Illegible server message");
			}
			break;

		case svc_nop:
			if (cls.signon < SIGNONS_4)
				Con_PrintLinef ("<-- server to client keepalive");
			break;

		case svc_time:
			CL_NetworkTimeReceived(MSG_ReadFloat(&cl_message));
			break;

		case svc_clientdata:
			CL_ParseClientdata();
			break;

		case svc_version:
			j = MSG_ReadLong(&cl_message);
			protocol = Protocol_EnumForNumber(j);
			if (protocol == PROTOCOL_UNKNOWN_0)
				Host_Error_Line ("CL_ParseServerMessage: Server is unrecognized protocol number (%d)", j);
			// hack for unmarked Nehahra movie demos which had a custom protocol
			if (protocol == PROTOCOL_QUAKEDP && cls.demoplayback && gamemode == GAME_NEHAHRA)
				protocol = PROTOCOL_NEHAHRAMOVIE;
			cls.protocol = protocol; // Quakespasm does nothing with protocol flags here
			break;

		case svc_disconnect:
			if (cls.demonum != -1)
				CL_NextDemo();
			else
				CL_DisconnectEx(q_is_kicked_true, cls.protocol == PROTOCOL_DARKPLACES8 ? MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)) : "Server disconnected");
			break;

		case svc_print:
			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));

			if (cl.is_qex && str[0] == '$') { // AURA 1.2
				str = LOC_GetString (str);
				CSQC_AddPrintText (str);	//[515]: csqc
			} else
				if (CL_ExaminePrintString(str)) // au21 - look for anything interesting like player IP addresses or ping reports
				CSQC_AddPrintText(str);	//[515]: csqc
			break;

		case svc_centerprint:
			str = MSG_ReadString (&cl_message, cl_readstring, sizeof(cl_readstring));
			if (cl.is_qex && str[0] == '$') { // AURA 1.3
				str = LOC_GetString (str);
			}

			CL_VM_Parse_CenterPrint(str);	//[515]: csqc
			break;

		case svc_stufftext:
			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));

			/* if (utf8_enable.integer)
			{
				strip_pqc = true;
				// we can safely strip and even
				// interpret these in utf8 mode
			}
			else */ switch(cls.protocol)
			{
				case PROTOCOL_QUAKE:
				case PROTOCOL_QUAKEDP:
					// maybe add other protocols if
					// so desired, but not DP7
						strip_pqc = true;
					break;
				case PROTOCOL_DARKPLACES7:
				default:
					// ProQuake does not support
					// these protocols
					strip_pqc = false;
					break;
			}
			if (strip_pqc)
			{
				// skip over ProQuake messages,
				// TODO actually interpret them
				// (they are sbar team score
				// updates), see proquake cl_parse.c
				if (*str == 0x01)
				{
					++str;
					while(*str >= 0x01 && *str <= 0x1F)
						++str;
				}
			}

			// Baker r8191 - cls.signon is what? SIGNON_1
			if (String_Starts_With (str, HINT_MESSAGE_PREFIX)) {
				Con_DPrintLinef ("Received server hint: %s", str);
				str += strlen (HINT_MESSAGE_PREFIX);
				CL_ParseHint (str);
				break; // Do not continue.
			}

			CL_VM_Parse_StuffCmd(str,q_is_quakeworld_false);	//[515]: csqc
			break;

		case svc_damage:
			V_ParseDamage ();
			break;

		case svc_serverinfo:
			CL_ParseServerInfo (q_is_quakeworld_false);
			break;

		case svc_setangle:
			if (developer_qw.integer)
				Con_PrintLinef ("Setangle");
			for (j = 0 ; j < 3 ; j ++)
				cl.viewangles[j] = MSG_ReadAngle(&cl_message, cls.protocol);
			if (!cls.demoplayback) {
				cl.fixangle[0] = true;
				VectorCopy (cl.viewangles, cl.mviewangles[0]);
				// disable interpolation if this is new
				if (!cl.fixangle[1]) {
					if (developer_qw.integer)
						Con_PrintLinef ("Setangle2");
					VectorCopy (cl.viewangles, cl.mviewangles[1]);
				} // if !cl.fixangle[1]
			} // if !cls.demoplayback
			break;

		case svc_zircon_warp: // ZMOVE_WARP - CL FROM SV
			// Baker: We will turn off free move until we stop receiving this
			// We reply to it in cl_input.c CL_SendCmd
			if (Have_Zircon_Ext_Flag_CLS(ZIRCON_EXT_FREEMOVE_4)) {
				cl.zircon_warp_sequence = MSG_ReadLong(&cl_message);
				cl.zircon_warp_sequence_clock = cl.time + 0.2;
				if (cl.zircon_warp_sequence == 0) { // ZMOVE_WARP
					if (Have_Flag (developer_movement.integer, /*CL*/ 1)) // ZMOVE_WARP
						Con_PrintLinef ("CL: svc_zircon_warp 0 Server says warp all clear!");
				}
			} else {
				Con_DPrintLinef (CON_WARN "svc_zircon_warp without freemove -- This can happen in single player");
				Con_DPrintLinef (CON_WARN "extensions %u", cls.zirconprotocolextensions);				
				MSG_ReadLong(&cl_message); // Process the message
			}
			break;

		case svc_setview:
			cl.viewentity = (unsigned short)MSG_ReadShort(&cl_message);
			if (cl.viewentity >= MAX_EDICTS_32768)
				Host_Error_Line ("svc_setview >= MAX_EDICTS_32768");
			if (cl.viewentity >= cl.max_entities)
				CL_ExpandEntities(cl.viewentity);
			// LadyHavoc: assume first setview recieved is the real player entity
			if (!cl.realplayerentity)
				cl.realplayerentity = cl.viewentity;
			// update cl.playerentity to this one if it is a valid player
			if (cl.viewentity >= 1 && cl.viewentity <= cl.maxclients)
				cl.playerentity = cl.viewentity;
			break;

		case svc_lightstyle:
			j = MSG_ReadByte(&cl_message);
			if (j >= cl.max_lightstyle) {
				Con_Printf ("svc_lightstyle >= MAX_LIGHTSTYLES_256");
				break;
			}
			strlcpy (cl.lightstyle[j].map,  MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof (cl.lightstyle[j].map));
			cl.lightstyle[j].map[MAX_STYLESTRING_64 - 1] = 0;
			cl.lightstyle[j].length = (int)strlen(cl.lightstyle[j].map);
			break;

		case svc_sound:
			CL_ParseStartSoundPacket (
				isin2 (cls.protocol, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3) ?
						q_is_large_soundindex_true : q_is_large_soundindex_false
			);
			break;

		case svc_precache:
			if (isin3 (cls.protocol, PROTOCOL_DARKPLACES1, PROTOCOL_DARKPLACES2, PROTOCOL_DARKPLACES3)) {
				// was svc_sound2 in protocols 1, 2, 3, removed in 4, 5, changed to svc_precache in 6
				CL_ParseStartSoundPacket(q_is_large_soundindex_true);
			}
			else
			{
				char *s;
				j = (unsigned short)MSG_ReadShort(&cl_message);
				s = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
				if (j < 32768) {
					if (j >= 1 && j < MAX_MODELS_8192) {
						model_t *model = Mod_ForName(s, false, false, s[0] == '*' ? cl.model_name[1] : NULL);
						if (!model)
							Con_DPrintLinef ("svc_precache: Mod_ForName(" QUOTED_S ") failed", s);
						cl.model_precache[j] = model;
					}
					else
						Con_PrintLinef ("svc_precache: index %d outside range %d...%d", j, 1, MAX_MODELS_8192);
				}
				else
				{
					j -= 32768;
					if (j >= 1 && j < MAX_SOUNDS_4096) {
						sfx_t *sfx = S_PrecacheSound (s, true, true);
						if (!sfx && snd_initialized.integer)
							Con_DPrintLinef ("svc_precache: S_PrecacheSound(" QUOTED_S ") failed", s);
						cl.sound_precache[j] = sfx;
					}
					else
						Con_PrintLinef ("svc_precache: index %d outside range %d...%d", j, 1, MAX_SOUNDS_4096);
				}
			}
			break;

		case svc_stopsound:
			j = (unsigned short) MSG_ReadShort(&cl_message);
			S_StopSound(j>>3, j&7);
			break;

		case svc_updatename:
			j = MSG_ReadByte(&cl_message);
			if (j >= cl.maxclients)
				Host_Error_Line ("CL_ParseServerMessage: svc_updatename >= cl.maxclients");
			strlcpy (cl.scores[j].name, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof (cl.scores[j].name));
			break;

		case svc_updatefrags:
			j = MSG_ReadByte(&cl_message);
			if (j >= cl.maxclients)
				Host_Error_Line ("CL_ParseServerMessage: svc_updatefrags >= cl.maxclients");
			cl.scores[j].frags = (signed short) MSG_ReadShort(&cl_message);
			break;

		case svc_updatecolors:
			j = MSG_ReadByte(&cl_message);
			if (j >= cl.maxclients)
				Host_Error_Line ("CL_ParseServerMessage: svc_updatecolors >= cl.maxclients");
			cl.scores[j].colors = MSG_ReadByte(&cl_message);
			break;

		case svc_particle:
			CL_ParseParticleEffect ();
			break;

		case svc_effect:
			if (cl.is_qex) { // AURA 1.5
				// case  svc_achievement_fights_effect_52 AURA
				str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
				Con_DPrintLinef ("Ignoring svc_achievement (%s)", str);
				break;
			}
			CL_ParseEffect ();
			break;

		case svc_effect2:
			CL_ParseEffect2 ();
			break;

		case svc_spawnbaseline:
			ent_num = (unsigned short) MSG_ReadShort(&cl_message);
			if (ent_num < 0 || ent_num >= MAX_EDICTS_32768)
				Host_Error_Line ("CL_ParseServerMessage: svc_spawnbaseline: invalid entity number %d", ent_num);
			if (ent_num >= cl.max_entities)
				CL_ExpandEntities(ent_num);
			CL_ParseBaseline (cl.entities + ent_num, q_is_large_modelindex_false, q_fitz_version_1, q_is_static_false);
			break;

		case svcfitz_spawnbaseline2: // DPD 999 - Looks compat
			ent_num = (unsigned short) MSG_ReadShort(&cl_message);
			if (ent_num < 0 || ent_num >= MAX_EDICTS_32768)
				Host_Error_Line ("CL_ParseServerMessage: svc_spawnbaseline: invalid entity number %d", ent_num);
			if (ent_num >= cl.max_entities)
				CL_ExpandEntities(ent_num);
			CL_ParseBaseline (cl.entities + ent_num, q_is_large_modelindex_false, q_fitz_version_2, q_is_static_false);
			break;

		case svc_spawnbaseline2: // 55 - Not used by Fitz666
			ent_num = (unsigned short) MSG_ReadShort(&cl_message);
			if (ent_num < 0 || ent_num >= MAX_EDICTS_32768)
				Host_Error_Line ("CL_ParseServerMessage: svc_spawnbaseline2: invalid entity number %d", ent_num);
			if (ent_num >= cl.max_entities)
				CL_ExpandEntities(ent_num);
			CL_ParseBaseline (cl.entities + ent_num, q_is_large_modelindex_true, q_fitz_version_none_0, q_is_static_false);
			break;
		case svc_spawnstatic:
			CL_ParseStatic (q_is_large_modelindex_false, q_fitz_version_1);
			break;

		case svcfitz_spawnstatic2: // DPD 999
			CL_ParseStatic (q_is_large_modelindex_false, q_fitz_version_2);
			break;

		case svc_spawnstatic2:
			if (cl.is_qex) { // AURA 1.6
				// svc_qex_localsound_fights_spawnstatic2_56
				CL_ParseLocalSound();
				break;
			}
svc_spawnstatic2_ugly: // AURA 13.2
			CL_ParseStatic (q_is_large_modelindex_true, q_fitz_version_none_0);
			break;

		case svc_temp_entity:
			if (!CL_VM_Parse_TempEntity())
				CL_ParseTempEntity ();
			break;

		case svc_setpause:
			cl.paused = MSG_ReadByte(&cl_message) != 0;

			if (cl.paused && snd_cdautopause.integer)
				CDAudio_Pause ();
			else if (bgmvolume.value > 0.0f)
				CDAudio_Resume ();

			S_PauseGameSounds (cl.paused);
			break;

		case svc_signonnum:
			j = MSG_ReadByte(&cl_message);
			// LadyHavoc: it's rude to kick off the client if they missed the
			// reconnect somehow, so allow signon 1 even if at signon 1
			if (j <= cls.signon && j != SIGNON_1)
				Host_Error_Line ("Received signon %d when at %d", j, cls.signon);
			cls.signon = j; // NORMAL QUAKE / DARKPLACES
			//if (cls.signon == SIGNON_1) {
			//	// Baker: Save the time here
			//	// we are going to use this to detect "slow loads hopefully".
			//	cls.signon_1_time = Sys_DirtyTime();
			//} else
			if (cls.signon == 3 /*SIGNONS_4*/) {
				// Baker: Slow load end
				// Baker: Demo playback never hits 4
				cl_signon_start_time = 0;
			}
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			j = MSG_ReadByte(&cl_message);
			if (j < 0 || j >= MAX_CL_STATS_256)
				Host_Error_Line ("svc_updatestat: %d is invalid", j);
			cl.stats[j] = MSG_ReadLong(&cl_message);
			break;

		case svc_updatestatubyte:
			j = MSG_ReadByte(&cl_message);
			if (j < 0 || j >= MAX_CL_STATS_256)
				Host_Error_Line ("svc_updatestat: %d is invalid", j);
			cl.stats[j] = MSG_ReadByte(&cl_message);
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound (isin2 (cls.protocol, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3)
				? q_is_large_soundindex_true : q_is_large_soundindex_false
				, q_fitz_version_1
			);
			break;

		case svcfitz_spawnstaticsound2:
			//CL_ParseStaticSound (2);
			CL_ParseStaticSound (q_is_large_soundindex_true, q_fitz_version_2); // DPD 999
			break;

		case svc_spawnstaticsound2:
			CL_ParseStaticSound (q_is_large_soundindex_true, q_fitz_version_none_0);
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte(&cl_message);
			cl.looptrack = MSG_ReadByte(&cl_message);

			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((unsigned char)cls.forcetrack, q_looping_true);
			else
				CDAudio_Play ((unsigned char)cl.cdtrack, q_looping_true);

			break;

		case svc_intermission:
			if (!cl.intermission)
				cl.completed_time = cl.time;
			cl.intermission = 1;
			CL_VM_UpdateIntermissionState(cl.intermission);
			break;

		case svc_finale:
			if (!cl.intermission)
				cl.completed_time = cl.time;
			cl.intermission = 2;
			CL_VM_UpdateIntermissionState(cl.intermission);

			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			if (cl.is_qex && str[0] == '$') { // AURA 1.7
				str = LOC_GetString (str);
			}

			SCR_CenterPrint(str);
			break;

		case svc_cutscene:
			if (!cl.intermission)
				cl.completed_time = cl.time;
			cl.intermission = 3;
			CL_VM_UpdateIntermissionState(cl.intermission);

			str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			if (cl.is_qex && str[0] == '$') { // AURA 1.8
				str = LOC_GetString (str);
			}

			SCR_CenterPrint (str);
			break;

		case svc_sellscreen:
			Cmd_ExecuteString(cmd_local, "help", src_local, true);
			break;

		case svc_hidelmp:
			if (gamemode == GAME_TENEBRAE)
			{
				// repeating particle effect
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				(void) MSG_ReadByte(&cl_message);
				MSG_ReadLong(&cl_message);
				MSG_ReadLong(&cl_message);
				MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			}
			else
				SHOWLMP_decodehide();
			break;
		case svc_showlmp: // 35
			if (cl.is_qex) { // AURA 13.1
				// svc_zirc_qex_svc_spawnstatic2_35
				// Baker: we are using this as svc_spawnstatic2 if sv.is_qex
				// AFAIK GAME_NEHAHRA is sole user of this goofy svc that LadyHavoc describes as junk
				goto svc_spawnstatic2_ugly;
			}
			if (gamemode == GAME_TENEBRAE)
			{
				// particle effect
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				MSG_ReadCoord(&cl_message, cls.protocol);
				(void) MSG_ReadByte(&cl_message);
				MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			}
			else
				SHOWLMP_decodeshow();
			break;

		case svc_skybox:
			R_SetSkyBox(MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
			break;

		// DPD 999 Baker: svc_bf from FitzQuake would need to be sent by a mod
		case svcfitz_bf: // Possibility this does not exist in any mod
			//Cmd_ExecuteString ("bf", src_command);
			break;

		// DPD 999 Baker: svc_fog from FitzQuake would need to be sent by a mod
		case svcfitz_fog: // Possibility this does not exist in any mod
			Fog_ParseServerMessage ();
			break;

		case svc_entities:
			if (cls.signon == SIGNONS_4 - 1)
			{
				// first update is the final signon stage
				cls.signon = SIGNONS_4;
				CL_SignonReply ();
			}
			if (cls.protocol == PROTOCOL_DARKPLACES1 || cls.protocol == PROTOCOL_DARKPLACES2 ||
				cls.protocol == PROTOCOL_DARKPLACES3)
				EntityFrame_CL_ReadFrame_DP1_DP3();
			else if (cls.protocol == PROTOCOL_DARKPLACES4)
				EntityFrame4_CL_ReadFrame();
			else
				EntityFrame5_CL_ReadFrame();
			break;

		case svc_csqcentities:
			CSQC_ReadEntities();
			break;

		case svc_downloaddata:
			CL_ParseDownload_DP();
downloadx_cl_start_download_during_2:
			break;
		case svc_trailparticles:
			CL_ParseTrailParticles();
			break;
		case svc_pointparticles:
			CL_ParsePointParticles();
			break;
		case svc_pointparticles1:
			CL_ParsePointParticles1();
			break;

		} // switch svc
//		R_TimeReport(svc_strings[cmd]);
	} // while 1


quakeworld_skip:
	if (cls.signon == SIGNONS_4)
		CL_UpdateItemsAndWeapon();
//	R_TimeReport("UpdateItems");

	EntityFrameQuake_ISeeDeadEntities();
//	R_TimeReport("ISeeDeadEntities");

	CL_UpdateMoveVars();
//	R_TimeReport("UpdateMoveVars");

	parsingerror = false;

	// LadyHavoc: this was at the start of the function before cl_autodemo was
	// implemented
	if (cls.demorecording)
	{
		CL_WriteDemoMessage (&cl_message);
//		R_TimeReport("WriteDemo");
	}
}

void CL_Parse_DumpPacket(void)
{
	if (!parsingerror)
		return;
	Con_Print("Packet dump:\n");
	SZ_HexDumpToConsole(&cl_message);
	parsingerror = false;
}

void CL_Parse_ErrorCleanUp(void)
{
	CL_StopDownload_DP(0, 0);
	QW_CL_StopUpload_f(cmd_local);
}

void CL_Parse_Init(void)
{
	Cvar_RegisterVariable(&cl_worldmessage);
	Cvar_RegisterVariable(&cl_worldname);
	Cvar_RegisterVariable(&cl_worldnamenoextension);
	Cvar_RegisterVariable(&cl_worldbasename);

	Cvar_RegisterVariable(&developer_networkentities);
	Cvar_RegisterVariable(&cl_gameplayfix_soundsmovewithentities);

	Cvar_RegisterVariable(&cl_sound_wizardhit);
	Cvar_RegisterVariable(&cl_sound_hknighthit);
	Cvar_RegisterVariable(&cl_sound_tink1);
	Cvar_RegisterVariable(&cl_sound_ric1);
	Cvar_RegisterVariable(&cl_sound_ric2);
	Cvar_RegisterVariable(&cl_sound_ric3);
	Cvar_RegisterVariable(&cl_sound_ric_gunshot);
	Cvar_RegisterVariable(&cl_sound_r_exp3);
	Cvar_RegisterVariable(&snd_cdautopause);

	Cvar_RegisterVariable(&cl_joinbeforedownloadsfinish);

	// server extension cvars set by commands issued from the server during connect
	Cvar_RegisterVariable(&cl_serverextension_download);

	Cvar_RegisterVariable(&cl_nettimesyncfactor);
	Cvar_RegisterVariable(&cl_nettimesyncboundmode);
	Cvar_RegisterVariable(&cl_nettimesyncboundtolerance);
	Cvar_RegisterVariable(&cl_iplog_name);
	Cvar_RegisterVariable(&cl_readpicture_force);

	Cmd_AddCommand(CF_CLIENT, "nextul", QW_CL_NextUpload_f, "sends next fragment of current upload buffer (screenshot for example)");
	Cmd_AddCommand(CF_CLIENT, "stopul", QW_CL_StopUpload_f, "aborts current upload (screenshot for example)");
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "skins", QW_CL_Skins_f, "downloads missing qw skins from server");
	Cmd_AddCommand(CF_CLIENT, "changing", QW_CL_Changing_f, "sent by qw servers to tell client to wait for level change");
	Cmd_AddCommand(CF_CLIENT, "cl_begindownloads", CL_BeginDownloads_DP_f, "used internally by darkplaces client while connecting (causes loading of models and sounds or triggers downloads for missing ones)");
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "cl_downloadbegin", CL_DownloadBegin_DP_f, "(networking) informs client of download file information, client replies with sv_startsoundload to begin the transfer");
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "stopdownload", CL_StopDownload_DP_QW_f, "terminates a download");
	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "cl_downloadfinished", CL_DownloadFinished_DP_f, "signals that a download has finished and provides the client with file size and crc to check its integrity");
	Cmd_AddCommand(CF_CLIENT, "iplog_list", CL_IPLog_List_f, "lists names of players whose IP address begins with the supplied text (example: iplog_list 123.456.789)");

	Cvar_RegisterVariable (&cl_pext);
	Cvar_RegisterVariable (&cl_pext_qw_256packetentities);
	Cvar_RegisterVariable (&cl_pext_qw_limits);

	Cvar_RegisterVariable (&cl_pext_chunkeddownloads);
	Cvar_RegisterVariable (&cl_chunksperframe);
//	Cvar_RegisterVariable(&cl_pext_qw_limits);
}

void CL_Parse_Shutdown(void)
{
}
