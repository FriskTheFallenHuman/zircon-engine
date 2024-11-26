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
// sv_user.c -- server code for moving users

#include "quakedef.h"
#include "sv_demo.h"
#define DEBUGMOVES 0

static usercmd_t usercmd;
extern cvar_t sv_autodemo_perclient;
extern cvar_t sv_rollangle;
extern cvar_t sv_rollspeed;

/*
==================
SV_PreSpawn_f
==================
*/

// Baker: Zircon sends extensions with prespawn if protcool is DP7
WARP_X_ (CL_BeginDownloads_Prespawn_Zircon_Extensions_Send HINT_MESSAGE_PREFIX SV_InsertHints)
static void SV_Zircon_Extensions_Send (client_t *client, sizebuf_t *sb)
{
	// Baker: Send even if client does not support protocol extensions.
	// We will send "//hint zircon_ext 0
	char vabuf[1024];
	const char *sv_hint_string;
	
	if (sv.protocol == PROTOCOL_DARKPLACES7) {
		WARP_X_ (CLIENT_SUPPORTED_ZIRCON_EXT)
		//unsigned int sv_ext = sv.zirconprotcolextensions_sv; //sv_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0;
		unsigned int shared_flags = client->cl_zirconprotocolextensions & sv.zirconprotcolextensions_sv;

		if (developer_zext.integer)
			Con_PrintLinef ("SV: Client " QUOTED_S " is reporting extensions %d", client->name, client->cl_zirconprotocolextensions); // This has newline
		if (developer_zext.integer)
			Con_PrintLinef ("SV: Server supports extensions %d", sv.zirconprotcolextensions_sv); // This has newline
		client->cl_zirconprotocolextensions = shared_flags; // zirconprotocolextensions
		sv_hint_string = va(vabuf, sizeof(vabuf), HINT_MESSAGE_PREFIX "zircon_ext %d" NEWLINE, shared_flags);
		
		Con_DPrintLinef	("Sending to " QUOTED_S ": "  QUOTED_S, client->name, sv_hint_string); // No newline, hint_string already has one
		MSG_WriteByte	(sb, svc_stufftext);  // ZIRCON_PEXT
		MSG_WriteString (sb, sv_hint_string);
	}
}

void SV_PreSpawn_f(cmd_state_t *cmd)
{
	if (host_client->prespawned) {
		Con_PrintLinef ("prespawn not valid -- already prespawned");
		return;
	}
	host_client->prespawned = true;

	if (Cmd_Argc (cmd) > 1) {
		host_client->cl_zirconprotocolextensions = atoi(Cmd_Argv(cmd,1));
		if (developer_zext.integer)
			Con_PrintLinef ("Client is reporting extensions %d", host_client->cl_zirconprotocolextensions); // ZIRCON_PEXT
	}

	if (host_client->netconnection) {
		SV_Zircon_Extensions_Send (host_client, &host_client->netconnection->message);
		SZ_Write (&host_client->netconnection->message, sv.signon.data, sv.signon.cursize);
		MSG_WriteByte (&host_client->netconnection->message, svc_signonnum);
		MSG_WriteByte (&host_client->netconnection->message, SIGNON_2);
		host_client->sendsignon = 0;		// enable unlimited sends again
	}

	// reset the name change timer because the client will send name soon
	host_client->nametime = 0;
}

/*
==================
SV_Spawn_f
==================
*/
void SV_Spawn_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	client_t *client;
	int stats[MAX_CL_STATS_256];

	if (!host_client->prespawned) {
		Con_PrintLinef ("Spawn not valid -- not yet prespawned");
		return;
	}

	if (host_client->spawned) {
		Con_PrintLinef ("Spawn not valid -- already spawned");
		return;
	}

	host_client->spawned = true;

	// reset name change timer again because they might want to change name
	// again in the first 5 seconds after connecting
	host_client->nametime = 0;

	// LadyHavoc: moved this above the QC calls at FrikaC's request
	// LadyHavoc: commented this out
	//if (host_client->netconnection)
	//	SZ_Clear (&host_client->netconnection->message);

	// run the entrance script
	if (sv.loadgame) { // WALDO!!!! .siv ok
		// loaded games are fully initialized already
		if (PRVM_serverfunction(RestoreGame)) {
			Con_DPrintLinef ("Calling RestoreGame");
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
			prog->ExecuteProgram(prog, PRVM_serverfunction(RestoreGame), "QC function RestoreGame is missing");
		}
	}
	else
	{
		//Con_Printf ("SV_Spawn_f: host_client->edict->netname = %s, host_client->edict->netname = %s, host_client->name = %s\n", PRVM_GetString(PRVM_serveredictstring(host_client->edict, netname)), PRVM_GetString(PRVM_serveredictstring(host_client->edict, netname)), host_client->name);

		// copy spawn parms out of the client_t
		for (i = 0 ; i < NUM_SPAWN_PARMS_16; i ++)
			(&PRVM_serverglobalfloat(parm1))[i] = host_client->spawn_parms[i];

		// call the spawn function
		host_client->clientconnectcalled = true;
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(ClientConnect), "QC function ClientConnect is missing");

		Con_PrintLinef ("%s connected", host_client->name);

		PRVM_serverglobalfloat(time) = sv.time;
		prog->ExecuteProgram(prog, PRVM_serverfunction(PutClientInServer), "QC function PutClientInServer is missing");

#if 1
		if (Have_Zircon_Ext_Flag_SV_Hard (ZIRCON_EXT_WALKTHROUGH_PLAYERS_IS_ACTIVE_128) )
			PRVM_serveredictfloat(host_client->edict, clipgroup) = 1;
#endif
	}

	if (!host_client->netconnection)
		return;

	// send time of update
	MSG_WriteByte (&host_client->netconnection->message, svc_time);
	MSG_WriteFloat (&host_client->netconnection->message, sv.time);

	// send all current names, colors, and frag counts
	for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++) {
		if (!client->active)
			continue;
		MSG_WriteByte	(&host_client->netconnection->message, svc_updatename);
		MSG_WriteByte	(&host_client->netconnection->message, i);
		MSG_WriteString (&host_client->netconnection->message, client->name);
		MSG_WriteByte	(&host_client->netconnection->message, svc_updatefrags);
		MSG_WriteByte	(&host_client->netconnection->message, i);
		MSG_WriteShort	(&host_client->netconnection->message, client->frags);
		MSG_WriteByte	(&host_client->netconnection->message, svc_updatecolors);
		MSG_WriteByte	(&host_client->netconnection->message, i);
		MSG_WriteByte	(&host_client->netconnection->message, client->colors);
	}

	// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES_256 ; i++)
	{
		if (sv.lightstyles[i][0])
		{
			MSG_WriteByte	(&host_client->netconnection->message, svc_lightstyle);
			MSG_WriteByte	(&host_client->netconnection->message, (char)i);
			MSG_WriteString (&host_client->netconnection->message, sv.lightstyles[i]);
		}
	}

	// send some stats
	MSG_WriteByte (&host_client->netconnection->message, svc_updatestat);
	MSG_WriteByte (&host_client->netconnection->message, STAT_TOTALSECRETS);
	MSG_WriteLong (&host_client->netconnection->message, (int)PRVM_serverglobalfloat(total_secrets));

	MSG_WriteByte (&host_client->netconnection->message, svc_updatestat);
	MSG_WriteByte (&host_client->netconnection->message, STAT_TOTALMONSTERS);
	MSG_WriteLong (&host_client->netconnection->message, (int)PRVM_serverglobalfloat(total_monsters));

	MSG_WriteByte (&host_client->netconnection->message, svc_updatestat);
	MSG_WriteByte (&host_client->netconnection->message, STAT_SECRETS);
	MSG_WriteLong (&host_client->netconnection->message, (int)PRVM_serverglobalfloat(found_secrets));

	MSG_WriteByte (&host_client->netconnection->message, svc_updatestat);
	MSG_WriteByte (&host_client->netconnection->message, STAT_MONSTERS);
	MSG_WriteLong (&host_client->netconnection->message, (int)PRVM_serverglobalfloat(killed_monsters));

	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	if (sv.loadgame) { // .siv ok
		MSG_WriteByte  (&host_client->netconnection->message, svc_setangle);
		MSG_WriteAngle (&host_client->netconnection->message, PRVM_serveredictvector(host_client->edict, v_angle)[0], sv.protocol);
		MSG_WriteAngle (&host_client->netconnection->message, PRVM_serveredictvector(host_client->edict, v_angle)[1], sv.protocol);
		MSG_WriteAngle (&host_client->netconnection->message, 0, sv.protocol);
	}
	else
	{
		MSG_WriteByte (&host_client->netconnection->message, svc_setangle);
		MSG_WriteAngle (&host_client->netconnection->message, PRVM_serveredictvector(host_client->edict, angles)[0], sv.protocol);
		MSG_WriteAngle (&host_client->netconnection->message, PRVM_serveredictvector(host_client->edict, angles)[1], sv.protocol);
		MSG_WriteAngle (&host_client->netconnection->message, 0, sv.protocol);
	}

	SV_WriteClientdataToMessage (host_client, host_client->edict, &host_client->netconnection->message, stats);

	MSG_WriteByte (&host_client->netconnection->message, svc_signonnum);
	MSG_WriteByte (&host_client->netconnection->message, 3);
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (cmd_state_t *cmd)
{
	if (!host_client->spawned)
	{
		Con_Print("Begin not valid -- not yet spawned\n");
		return;
	}
	if (host_client->begun)
	{
		Con_Print("Begin not valid -- already begun\n");
		return;
	}
	host_client->begun = true;


	// LadyHavoc: note: this code also exists in SV_DropClient
	if (sv.loadgame) { // .siv ok
		int i;
		for (i = 0; i < svs.maxclients; i ++)
			if (svs.clients[i].active && !svs.clients[i].spawned)
				break;

		// Baker: Clients send "begin" one at a time
		if (i == svs.maxclients) {
			Con_Printf ("Loaded game, everyone rejoined - unpausing\n");
			sv.paused = sv.loadgame = false; // we're basically done with loading now
		}
	}
}

/*
===============
SV_SetIdealPitch
===============
*/
#define	MAX_FORWARD	6
void SV_SetIdealPitch (void)
{
	prvm_prog_t *prog = SVVM_prog;
	float	angleval, sinval, cosval, step, dir;
	trace_t	tr;
	vec3_t	top, bottom;
	float	z[MAX_FORWARD];
	int		i, j;
	int		steps;

	if (!((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_ONGROUND_512))
		return;

	angleval = PRVM_serveredictvector(host_client->edict, angles)[YAW] * M_PI*2 / 360;
	sinval = sin(angleval);
	cosval = cos(angleval);

	for (i=0 ; i<MAX_FORWARD ; i++)
	{
		top[0] = PRVM_serveredictvector(host_client->edict, origin)[0] + cosval*(i+3)*12;
		top[1] = PRVM_serveredictvector(host_client->edict, origin)[1] + sinval*(i+3)*12;
		top[2] = PRVM_serveredictvector(host_client->edict, origin)[2] + PRVM_serveredictvector(host_client->edict, view_ofs)[2];

		bottom[0] = top[0];
		bottom[1] = top[1];
		bottom[2] = top[2] - 160;

		tr = SV_TraceLine(top, bottom, MOVE_NOMONSTERS_1, host_client->edict, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value);
		// if looking at a wall, leave ideal the way is was
		if (tr.startsolid)
			return;

		// near a dropoff
		if (tr.fraction == 1)
			return;

		z[i] = top[2] + tr.fraction*(bottom[2]-top[2]);
	}

	dir = 0;
	steps = 0;
	for (j=1 ; j<i ; j++)
	{
		step = z[j] - z[j-1];
		if (step > -ON_EPSILON_0_1F && step < ON_EPSILON_0_1F)
			continue;

		// mixed changes
		if (dir && ( step-dir > ON_EPSILON_0_1F || step-dir < -ON_EPSILON_0_1F ) )
			return;

		steps++;
		dir = step;
	}

	if (!dir)
	{
		PRVM_serveredictfloat(host_client->edict, idealpitch) = 0;
		return;
	}

	if (steps < 2)
		return;
	PRVM_serveredictfloat(host_client->edict, idealpitch) = -dir * sv_idealpitchscale.value;
}

static vec3_t wishdir, forward, right, up;
static float wishspeed;

static qbool onground;

/*
==================
SV_UserFriction

==================
*/
static void SV_UserFriction (void)
{
	prvm_prog_t *prog = SVVM_prog;
	float speed, newspeed, control, friction;
	vec3_t start, stop;
	trace_t trace;

	speed = sqrt(PRVM_serveredictvector(host_client->edict, velocity)[0]*PRVM_serveredictvector(host_client->edict, velocity)[0]+PRVM_serveredictvector(host_client->edict, velocity)[1]*PRVM_serveredictvector(host_client->edict, velocity)[1]);
	if (!speed)
		return;

	// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = PRVM_serveredictvector(host_client->edict, origin)[0] + PRVM_serveredictvector(host_client->edict, velocity)[0]/speed*16;
	start[1] = stop[1] = PRVM_serveredictvector(host_client->edict, origin)[1] + PRVM_serveredictvector(host_client->edict, velocity)[1]/speed*16;
	start[2] = PRVM_serveredictvector(host_client->edict, origin)[2] + PRVM_serveredictvector(host_client->edict, mins)[2];
	stop[2] = start[2] - 34;

	trace = SV_TraceLine(start, stop, MOVE_NOMONSTERS_1, host_client->edict, SV_GenericHitSuperContentsMask(host_client->edict), 0, 0, collision_extendmovelength.value);

	if (trace.fraction == 1.0)
		friction = sv_friction.value*sv_edgefriction.value;
	else
		friction = sv_friction.value;

	// apply friction
	control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
	newspeed = speed - sv.frametime*control*friction;

	if (newspeed < 0)
		newspeed = 0;
	else
		newspeed /= speed;

	VectorScale(PRVM_serveredictvector(host_client->edict, velocity), newspeed, PRVM_serveredictvector(host_client->edict, velocity));
}

/*
==============
SV_Accelerate
==============
*/
static void SV_Accelerate (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (PRVM_serveredictvector(host_client->edict, velocity), wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = sv_accelerate.value*sv.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		PRVM_serveredictvector(host_client->edict, velocity)[i] += accelspeed*wishdir[i];
}

extern cvar_t sv_gameplayfix_q2airaccelerate;
static void SV_AirAccelerate (vec3_t wishveloc)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	float addspeed, wishspd, accelspeed, currentspeed;

	wishspd = VectorNormalizeLength (wishveloc);
	if (wishspd > sv_maxairspeed.value)
		wishspd = sv_maxairspeed.value;
	currentspeed = DotProduct (PRVM_serveredictvector(host_client->edict, velocity), wishveloc);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = (sv_airaccelerate.value < 0 ? sv_accelerate.value : sv_airaccelerate.value)*(sv_gameplayfix_q2airaccelerate.integer ? wishspd : wishspeed) * sv.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		PRVM_serveredictvector(host_client->edict, velocity)[i] += accelspeed*wishveloc[i];
}


static void DropPunchAngle (void)
{
	prvm_prog_t *prog = SVVM_prog;
	vec_t len;
	vec3_t punchangle, punchvector;

	VectorCopy(PRVM_serveredictvector(host_client->edict, punchangle), punchangle);
	VectorCopy(PRVM_serveredictvector(host_client->edict, punchvector), punchvector);

	len = VectorNormalizeLength(punchangle);
	if (len > 0)
	{
		len -= 10*sv.frametime;
		if (len < 0)
			len = 0;
		VectorScale(punchangle, len, punchangle);
	}

	len = VectorNormalizeLength(punchvector);
	if (len > 0)
	{
		len -= 20*sv.frametime;
		if (len < 0)
			len = 0;
		VectorScale(punchvector, len, punchvector);
	}

	VectorCopy(punchangle, PRVM_serveredictvector(host_client->edict, punchangle));
	VectorCopy(punchvector, PRVM_serveredictvector(host_client->edict, punchvector));
}

/*
===================
SV_WaterMove

===================
*/
// Baker r0085: FitzQuake noclipping
void SV_NoClip_Move (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	vec3_t wishvel, v_angle;
	vec_t fwishspeed, temp;

	// user intentions
	VectorCopy(PRVM_serveredictvector(host_client->edict, v_angle), v_angle);
	AngleVectors(v_angle, forward, right, up);

	for (i = 0; i < 3 ; i ++) {
		wishvel[i] = forward[i] * usercmd.clx_forwardmove + right[i] * usercmd.clx_sidemove;
	}

    wishvel[2] += usercmd.clx_upmove;

	fwishspeed = VectorLength(wishvel);
	if (fwishspeed > sv_maxspeed.value)
	{
		temp = sv_maxspeed.value/fwishspeed;
		VectorScale (wishvel, temp, wishvel);
		fwishspeed = sv_maxspeed.value;
	}

	for (i=0 ; i<3 ; i++)
		PRVM_serveredictvector(host_client->edict, velocity)[i] = wishvel[i];
}

static void SV_WaterMove (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	vec3_t wishvel, v_angle;
	vec_t speed, newspeed, fwishspeed, addspeed, accelspeed, temp;

	// user intentions
	VectorCopy(PRVM_serveredictvector(host_client->edict, v_angle), v_angle);
	AngleVectors(v_angle, forward, right, up);

	for (i = 0; i < 3 ; i ++)
		wishvel[i] = forward[i]*usercmd.clx_forwardmove + right[i]*usercmd.clx_sidemove;

	if (!usercmd.clx_forwardmove && !usercmd.clx_sidemove && !usercmd.clx_upmove) {
		// The usual
		wishvel[2] -= 60;		// drift towards bottom (BOAT)
	}
	else
		wishvel[2] += usercmd.clx_upmove;

	fwishspeed = VectorLength(wishvel);
	if (fwishspeed > sv_maxspeed.value) {
		temp = sv_maxspeed.value/fwishspeed;
		VectorScale (wishvel, temp, wishvel);
		fwishspeed = sv_maxspeed.value;
	}
	fwishspeed *= 0.7;

	// water friction
	speed = VectorLength(PRVM_serveredictvector(host_client->edict, velocity));
	if (speed) {
		newspeed = speed - sv.frametime * speed * (sv_waterfriction.value < 0 /*d: -1*/ ? sv_friction.value /*d: 4*/ : sv_waterfriction.value /*d: -1*/);
		if (newspeed < 0)
			newspeed = 0;
		temp = newspeed/speed;
		VectorScale(PRVM_serveredictvector(host_client->edict, velocity), temp, PRVM_serveredictvector(host_client->edict, velocity));
	}
	else
		newspeed = 0;

	// water acceleration
	if (!fwishspeed)
		return;

	addspeed = fwishspeed - newspeed;
	if (addspeed <= 0)
		return;

	VectorNormalize (wishvel);
	accelspeed = (sv_wateraccelerate.value < 0 ? sv_accelerate.value : sv_wateraccelerate.value) * fwishspeed * sv.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		PRVM_serveredictvector(host_client->edict, velocity)[i] += accelspeed * wishvel[i];
}

static void SV_WaterJump (void)
{
	prvm_prog_t *prog = SVVM_prog;
	if (sv.time > PRVM_serveredictfloat(host_client->edict, teleport_time) || !PRVM_serveredictfloat(host_client->edict, waterlevel) /*WATERLEVEL_NONE_0*/ )
	{
		PRVM_serveredictfloat(host_client->edict, flags) = (int)PRVM_serveredictfloat(host_client->edict, flags) & ~FL_WATERJUMP;
		PRVM_serveredictfloat(host_client->edict, teleport_time) = 0;
	}
	PRVM_serveredictvector(host_client->edict, velocity)[0] = PRVM_serveredictvector(host_client->edict, movedir)[0];
	PRVM_serveredictvector(host_client->edict, velocity)[1] = PRVM_serveredictvector(host_client->edict, movedir)[1];
}


/*
===================
SV_AirMove

===================
*/
static void SV_AirMove (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	vec3_t wishvel;
	float fmove, smove, temp;

	// LadyHavoc: correct quake movement speed bug when looking up/down
	wishvel[0] = wishvel[2] = 0;
	wishvel[1] = PRVM_serveredictvector(host_client->edict, angles)[1];
	AngleVectors (wishvel, forward, right, up);

	fmove = usercmd.clx_forwardmove;
	smove = usercmd.clx_sidemove;

// hack to not let you back into teleporter
	if (sv.time < PRVM_serveredictfloat(host_client->edict, teleport_time) && fmove < 0)
		fmove = 0;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*fmove + right[i]*smove;

	if ((int)PRVM_serveredictfloat(host_client->edict, movetype) != MOVETYPE_WALK_3)
		wishvel[2] += usercmd.clx_upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);
	if (wishspeed > sv_maxspeed.value)
	{
		temp = sv_maxspeed.value/wishspeed;
		VectorScale (wishvel, temp, wishvel);
		wishspeed = sv_maxspeed.value;
	}

	if (PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_NOCLIP_8)
	{
		// noclip
		VectorCopy (wishvel, PRVM_serveredictvector(host_client->edict, velocity));
	}
	else if (onground)
	{
		SV_UserFriction ();
		SV_Accelerate ();
	}
	else
	{
		// not on ground, so little effect on velocity
		SV_AirAccelerate (wishvel);
	}
}

/*
===================
SV_PlayerPhysics

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void SV_PlayerPhysics (void)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t v_angle, angles, velocity;

	//Con_Printf ("clientthink for %dms\n", (int) (sv.frametime * 1000));

	SV_PlayerPhysics_ApplyClientMove();
	// make sure the velocity is sane (not a NaN)
	SV_CheckVelocity(host_client->edict);

	// LadyHavoc: QuakeC replacement for SV_PlayerPhysics (player movement)
	if (PRVM_serverfunction(SV_PlayerPhysics) && sv_playerphysicsqc.integer)
	{
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SV_PlayerPhysics), "QC function SV_PlayerPhysics is missing");
		SV_CheckVelocity(host_client->edict);
		return;
	}

	if (PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_NONE_0)
		return;

	onground = ((int)PRVM_serveredictfloat(host_client->edict, flags) & FL_ONGROUND_512) != 0;

	DropPunchAngle ();

	// if dead, behave differently
	if (PRVM_serveredictfloat(host_client->edict, health) <= 0)
		return;

	usercmd = host_client->cmd;

	// angles
	// show 1/3 the pitch angle and all the roll angle
	VectorAdd (PRVM_serveredictvector(host_client->edict, v_angle), PRVM_serveredictvector(host_client->edict, punchangle), v_angle);
	VectorCopy(PRVM_serveredictvector(host_client->edict, angles), angles);
	VectorCopy(PRVM_serveredictvector(host_client->edict, velocity), velocity);
	PRVM_serveredictvector(host_client->edict, angles)[ROLL] = Com_CalcRoll (angles, velocity, sv_rollangle.value, sv_rollspeed.value)*4;
	if (!PRVM_serveredictfloat(host_client->edict, fixangle))
	{
		PRVM_serveredictvector(host_client->edict, angles)[PITCH] = -v_angle[PITCH]/3;
		PRVM_serveredictvector(host_client->edict, angles)[YAW] = v_angle[YAW];
	}

	if ( (int)PRVM_serveredictfloat(host_client->edict, flags) & FL_WATERJUMP )
	{
		SV_WaterJump ();
		SV_CheckVelocity(host_client->edict);
		return;
	}

	// walk
#if 1
	if (PRVM_serveredictfloat(host_client->edict, movetype) != MOVETYPE_NOCLIP_8) {
		if (PRVM_serveredictfloat(host_client->edict, waterlevel) >= WATER_LEVEL_WAIST_2) {
			SV_WaterMove ();
			SV_CheckVelocity(host_client->edict);
			return;
		} else if (Have_Flag ((int)PRVM_serveredictfloat(host_client->edict, flags), FL_SWIM_2)) {
			// boat/swim more lax water check
			if (PRVM_serveredictfloat(host_client->edict, waterlevel) >= WATER_LEVEL_FEET_1) {
				SV_WaterMove ();
				SV_CheckVelocity(host_client->edict);
				return;
			}
		}
	}
#else
	if ((PRVM_serveredictfloat(host_client->edict, waterlevel) >= WATER_LEVEL_WAIST_2) && 
		(PRVM_serveredictfloat(host_client->edict, movetype) != MOVETYPE_NOCLIP_8))
	{
		SV_WaterMove ();
		SV_CheckVelocity(host_client->edict);
		return;
	}
#endif

	// Baker r0085: FitzQuake noclipping
	if ((PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_NOCLIP_8) && sv_altnoclipmove.integer) {
		SV_NoClip_Move ();
		SV_CheckVelocity(host_client->edict);
		return;
	}

	SV_AirMove ();
	SV_CheckVelocity(host_client->edict);

#if 0 // PHYSICAL
	if (sv.protocol == PROTOCOL_DARKPLACES8) {
		PRVM_serveredictvector(host_client->edict, origin)[0] = v_angle[YAW];
	}
#endif
}

/*
===================
SV_ReadClientMessage_ReadClientMove
===================
*/
int sv_numreadmoves = 0;
usercmd_t sv_readmoves[CL_MAX_USERCMDS_128];
static void SV_ReadClientMessage_ReadClientMove (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	usercmd_t newmove;
	usercmd_t *move = &newmove;

	memset(move, 0, sizeof(*move));

	if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);

	// read ping time - Baker: DP7 and Quakeworld
	if (false == isin14 (sv.protocol,				PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999, 
							PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE, 
							PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,
							PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
							PROTOCOL_DARKPLACES4,	PROTOCOL_DARKPLACES5,	PROTOCOL_DARKPLACES6)) {
		move->clx_sequence = MSG_ReadLong(&sv_message);

	} // if DP7 / Quakeworld
	move->clx_time = MSG_ReadFloat(&sv_message);
	if (sv_message.badread) Con_PrintLinef ("SV_ReadClientMessage: badread at %s:%d", __FILE__, __LINE__);
	move->clx_receivetime = (float)sv.time;

#if DEBUGMOVES
	Con_Printf ("%s move%d #%u %dms (%dms) %d %d '%d %d %d' '%d %d %d'\n", move->time > move->receivetime ? "^3read future" : "^4read normal", sv_numreadmoves + 1, move->sequence, (int)floor((move->time - host_client->cmd.time) * 1000.0 + 0.5), (int)floor(move->time * 1000.0 + 0.5), move->impulse, move->buttons, (int)move->viewangles[0], (int)move->viewangles[1], (int)move->viewangles[2], (int)move->forwardmove, (int)move->sidemove, (int)move->upmove);
#endif
	// limit reported time to current time
	// (incase the client is trying to cheat)
	if (move->clx_time > sv.time + sv.frametime) {
		// Baker: No evidence this future move hits PHYSICAL
		// Baker: This hits sometimes when the console is opened on listen server hosting player
		// Con_PrintLinef ("Future move time %f vs max %f", move->clx_time, sv.time + sv.frametime); // PHYSICAL
	}
	move->clx_time = Smallest(move->clx_time, sv.time + sv.frametime);

	// read current angles
	for (i = 0;i < 3;i++) {
		// sv999#1 - FitzQuake 666 client sends short angles always so we MSG_ReadAngle16
		if (isin2(sv.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999))
			move->viewangles[i] = MSG_ReadAngle16f(&sv_message);
		else 
		if (isin6(sv.protocol, PROTOCOL_QUAKE, 
							PROTOCOL_QUAKEDP, PROTOCOL_NEHAHRAMOVIE,
							PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3))
			move->viewangles[i] = MSG_ReadAngle8i(&sv_message);
		else if (sv.protocol == PROTOCOL_DARKPLACES1)
			move->viewangles[i] = MSG_ReadAngle16i(&sv_message);
		else if (isin2 (sv.protocol, PROTOCOL_DARKPLACES2, PROTOCOL_DARKPLACES3))
			move->viewangles[i] = MSG_ReadAngle32f(&sv_message);
		else
			move->viewangles[i] = MSG_ReadAngle16i(&sv_message);
	}

#if 0 // PHYSICAL
	if (sv.protocol == PROTOCOL_DARKPLACES8) {
		for (i = 0;i < 3;i++)
		{
			move->player_org[i] = MSG_ReadAngle16i(&sv_message);
		}
	}
#endif

	if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);

	// read movement
	move->clx_forwardmove = MSG_ReadCoord16i(&sv_message);
	move->clx_sidemove = MSG_ReadCoord16i(&sv_message);
	move->clx_upmove = MSG_ReadCoord16i(&sv_message);
	if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);

	// read buttons
	// be sure to bitwise OR them into the move->buttons because we want to
	// accumulate button presses from multiple packets per actual move
	if (isin13 (sv.protocol,	PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,
		PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,
		PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,
		PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
		PROTOCOL_DARKPLACES4,	PROTOCOL_DARKPLACES5))
		move->buttons = MSG_ReadByte(&sv_message);
	else
		move->buttons = MSG_ReadLong(&sv_message);
	if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);

	// read impulse
	move->impulse = MSG_ReadByte(&sv_message);
	if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);

	// PRYDON_CLIENTCURSOR
	if (false == isin13 (sv.protocol,	PROTOCOL_FITZQUAKE666,	PROTOCOL_FITZQUAKE999,
				PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,
				PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2,	PROTOCOL_NEHAHRABJP3,	
				PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
				PROTOCOL_DARKPLACES4,	PROTOCOL_DARKPLACES5))
	{
		// 30 bytes
		move->cursor_screen[0] = MSG_ReadShort(&sv_message) * (1.0f / 32767.0f);
		move->cursor_screen[1] = MSG_ReadShort(&sv_message) * (1.0f / 32767.0f);
		move->cursor_start[0] = MSG_ReadFloat(&sv_message);
		move->cursor_start[1] = MSG_ReadFloat(&sv_message);
		move->cursor_start[2] = MSG_ReadFloat(&sv_message);
		move->cursor_impact[0] = MSG_ReadFloat(&sv_message);
		move->cursor_impact[1] = MSG_ReadFloat(&sv_message);
		move->cursor_impact[2] = MSG_ReadFloat(&sv_message);
		move->cursor_entitynumber = (unsigned short)MSG_ReadShort(&sv_message);
		if (move->cursor_entitynumber >= prog->max_edicts)
		{
			Con_DPrintf ("SV_ReadClientMessage: client send bad cursor_entitynumber\n");
			move->cursor_entitynumber = 0;
		}
		// as requested by FrikaC, cursor_trace_ent is reset to world if the
		// entity is free at time of receipt
		if (PRVM_EDICT_NUM(move->cursor_entitynumber)->free)
			move->cursor_entitynumber = 0;
		if (sv_message.badread) Con_Printf ("SV_ReadClientMessage: badread at %s:%d\n", __FILE__, __LINE__);
	}

	// if the previous move has not been applied yet, we need to accumulate
	// the impulse/buttons from it
	if (!host_client->cmd.clx_is_applied)
	{
		if (!move->impulse)
			move->impulse = host_client->cmd.impulse;
		move->buttons |= host_client->cmd.buttons;
	}

	// now store this move for later execution
	// (we have to buffer the moves because of old ones being repeated)
	if (sv_numreadmoves < CL_MAX_USERCMDS_128)
		sv_readmoves[sv_numreadmoves++] = *move;

	// movement packet loss tracking
	// bones_was_here: checking begun prevents heavy loss detection right after a map change
	if (move->clx_sequence && host_client->begun) {
		if (move->clx_sequence > host_client->movement_highestsequence_seen) {
			if (host_client->movement_highestsequence_seen) {
				// mark moves in between as lost
				unsigned int delta = move->clx_sequence - host_client->movement_highestsequence_seen - 1;
				if (delta < NETGRAPH_PACKETS_256) {
					unsigned int u;
					for(u = 0; u < delta; ++u)
						host_client->movement_count[(host_client->movement_highestsequence_seen + 1 + u) % NETGRAPH_PACKETS_256] = -1;
				}
				else
					memset(host_client->movement_count, -1, sizeof(host_client->movement_count));
			}
			// mark THIS move as seen for the first time
			host_client->movement_count[move->clx_sequence % NETGRAPH_PACKETS_256] = 1;
			// update highest sequence seen
			host_client->movement_highestsequence_seen = move->clx_sequence;
		}
		else
			if (host_client->movement_count[move->clx_sequence % NETGRAPH_PACKETS_256] >= 0)
				++host_client->movement_count[move->clx_sequence % NETGRAPH_PACKETS_256];
	}
	else
	{
		host_client->movement_highestsequence_seen = 0;
		memset(host_client->movement_count, 0, sizeof(host_client->movement_count));
	}
}

static void SV_ReadClientMessage_ExecuteClientMoves (vec_t *zircon_move_final_pos, int zircon_move_final_ground)
{
	vec_t *v;
	prvm_prog_t *prog = SVVM_prog;
	int moveindex;
	double moveframetime;
	double oldframetime;
	double oldframetime2;

	if (sv_numreadmoves < 1)
		return;
	// only start accepting input once the player is spawned
	if (!host_client->begun)
		return;

			// Baker: This is for elevators and lifts
			int ground_ent = PRVM_serveredictedict(host_client->edict, groundentity);
			if (ground_ent) {
				SV_PhysicsX_Zircon_Warp_Start (host_client, "onground_entity");
			}
			int health = PRVM_serveredictfloat(host_client->edict, health);
			if (health <= 0 && health != NEXUIZ_OBS_NEG_666) {
				//SV_PhysicsX_Zircon_Warp_Start ("execmove_health");
			}


#if DEBUGMOVES
	Con_Printf ("SV_ExecuteClientMoves: read %d moves at sv.time %f\n", sv_numreadmoves, (float)sv.time);
#endif
	// disable clientside movement prediction in some cases
	if (ceil(max(sv_readmoves[sv_numreadmoves-1].clx_receivetime - sv_readmoves[sv_numreadmoves-1].clx_time, 0) * 1000.0) < sv_clmovement_minping.integer)
		host_client->clmovement_disabletimeout = host.realtime + sv_clmovement_minping_disabletime.value / 1000.0;
	// several conditions govern whether clientside movement prediction is allowed
	if (sv_readmoves[sv_numreadmoves-1].clx_sequence && 
		sv_clmovement_enable.integer && 
		sv_clmovement_inputtimeout.value /*0.1*/ > 0 && 
		host_client->clmovement_disabletimeout <= host.realtime && 
		(PRVM_serveredictfloat(host_client->edict, disableclientprediction) == -1 || 
		(PRVM_serveredictfloat(host_client->edict, movetype) == MOVETYPE_WALK_3 && 
		(!PRVM_serveredictfloat(host_client->edict, disableclientprediction)))))
	{
		// process the moves in order and ignore old ones
		// but always trust the latest move
		// (this deals with bogus initial move sequences after level change,
		//  where the client will eventually catch up with the level change
		//  and reset its move sequence)
		for (moveindex = 0; moveindex < sv_numreadmoves; moveindex++) {
			usercmd_t *move = sv_readmoves + moveindex;
			if (host_client->movesequence < move->clx_sequence || moveindex == sv_numreadmoves - 1) {
				// Baker: float clx_time_in = move->clx_time;
#if DEBUGMOVES
				Con_Printf ("%smove #%u %dms (%dms) %d %d '%d %d %d' '%d %d %d'\n", (move->time - host_client->cmd.time) > sv.frametime * 1.01 ? "^1" : "^2", 
					move->sequence, (int)floor((move->time - host_client->cmd.time) * 
					1000.0 + 0.5), (int)floor(move->time * 1000.0 + 0.5), 
					move->impulse, move->buttons, 
					(int)move->viewangles[0], (int)move->viewangles[1], 
					(int)move->viewangles[2], (int)move->forwardmove, 
					(int)move->sidemove, (int)move->upmove);
#endif
				// this is a new move - (PHYSICAL - PRIME TARGET - SO WHICH ONE HITS, WHAT ARE VALUES?)
				move->clx_time = bound(sv.time - 1, move->clx_time, sv.time); // prevent slowhack/speedhack combos
				move->clx_time = Largest(move->clx_time, host_client->cmd.clx_time); // prevent backstepping of time
				// bones_was_here: limit moveframetime to a multiple of sv.frametime to match inputtimeout behaviour
				moveframetime = Smallest(move->clx_time - host_client->cmd.clx_time, 
					min(0.1, sys_ticrate.value /*d: 1/72 or 0.013*/ > 0.0 && 
						sv.frametime > 0.0 ? 
							sv.frametime * ceil(sv_clmovement_inputtimeout.value /*d: 0.1*/ / sv.frametime) : 
							sv_clmovement_inputtimeout.value));

				// discard (treat like lost) moves with too low distance from
				// the previous one to prevent hacks using float inaccuracy
				// clients will see this as packet loss in the netgraph
				// this should also apply if a move cannot get
				// executed because it came too late and
				// already was performed serverside
				if (moveframetime < 0.0005) { 
					// count the move as LOST if we don't
					// execute it but it has higher
					// sequence count
					if (host_client->movesequence)
						if (move->clx_sequence > host_client->movesequence)
							host_client->movement_count[(move->clx_sequence) % NETGRAPH_PACKETS_256] = -1;

					//if (developer_movement.integer) { // OLD
					//	Con_PrintLinef ("Rejected move due to timing svframetime is %f", sv.frametime);
					//	Con_PrintLinef ("clx_time in was        : %f", clx_time_in);
					//	Con_PrintLinef ("min allowed sv_time - 1: %f", sv.time - 1);
					//	Con_PrintLinef ("actual      sv_time - 0: %f", sv.time);
					//	Con_PrintLinef ("move->clx_time         : %f", move->clx_time);
					//	Con_PrintLinef ("host_client->clx_time  : %f", host_client->cmd.clx_time);
					//	Con_PrintLinef ("moveframetime          : %f", moveframetime);

					//}
					continue; // REJECT MOVE SLICING
				}

				//Con_Printf ("movesequence = %d (%d lost), moveframetime = %f\n", move->sequence, move->sequence ? move->sequence - host_client->movesequence - 1 : 0, moveframetime);
				host_client->cmd = *move;
				host_client->movesequence = move->clx_sequence;

				// if using prediction, we need to perform moves when packets are
				// received, even if multiple occur in one frame
				// (they can't go beyond the current time so there is no cheat issue
				//  with this approach, and if they don't send input for a while they
				//  start moving anyway, so the longest 'lagaport' possible is
				//  determined by the sv_clmovement_inputtimeout cvar)
				oldframetime = PRVM_serverglobalfloat(frametime);
				oldframetime2 = sv.frametime;
				// update ping time for qc to see while executing this move
				host_client->ping = host_client->cmd.clx_receivetime - host_client->cmd.clx_time;
				// the server and qc frametime values must be changed temporarily
				PRVM_serverglobalfloat(frametime) = sv.frametime = moveframetime;
				// if move is more than 50ms, split it into two moves (this matches QWSV 
				// behavior and the client prediction)
				if (sv.frametime > 0.05) { // Baker: ezQuake chop up very long command 
					PRVM_serverglobalfloat(frametime) = sv.frametime = moveframetime * 0.5f;
					SV_Physics_ClientMove(); // PREDICTED MOVE
				}
				SV_Physics_ClientMove(); // PREDICTED MOVE
				sv.frametime = oldframetime2;
				PRVM_serverglobalfloat(frametime) = oldframetime;
				host_client->clmovement_inputtimeout = min(0.1, sv_clmovement_inputtimeout.value /*d:0.1*/);
			}
		}
	}
	else
	{
		// Baker: This happens on level start
		// try to gather button bits from old moves, but only if their time is
		// advancing (ones with the same timestamp can't be trusted)
		for (moveindex = 0;moveindex < sv_numreadmoves-1;moveindex++) {
			usercmd_t *move = sv_readmoves + moveindex;
			if (host_client->cmd.clx_time < move->clx_time)
			{
				sv_readmoves[sv_numreadmoves-1].buttons |= move->buttons;
				if (move->impulse)
					sv_readmoves[sv_numreadmoves-1].impulse = move->impulse;
			}
		}
		// now copy the new move
		host_client->cmd = sv_readmoves[sv_numreadmoves-1];
		host_client->cmd.clx_time = max(host_client->cmd.clx_time, sv.time);
			// physics will run up to sv.time, so allow no predicted moves
			// before that otherwise, there is a speedhack by turning
			// prediction on and off repeatedly on client side because the
			// engine would run BOTH client and server physics for the same
			// time
		host_client->movesequence = 0;
		// make sure that normal physics takes over immediately
		host_client->clmovement_inputtimeout = 0;
		// update ping time
		host_client->ping = host_client->cmd.clx_receivetime - sv_readmoves[sv_numreadmoves-1].clx_time;
	}

	health = PRVM_serveredictfloat(host_client->edict, health);
	if (health <= 0 && health != NEXUIZ_OBS_NEG_666) {
		//SV_PhysicsX_Zircon_Warp_Start ("execmoves_health0");
	}

	if (host_client->last_wanted_is_zircon_free_move) {
		v = PRVM_serveredictvector(host_client->edict, origin);
		VectorCopy (zircon_move_final_pos, v);
		int on_ground_sv = Have_Flag ((int)PRVM_serveredictfloat(host_client->edict, flags), FL_ONGROUND_512);
		if (on_ground_sv == false && sv.zircon_jumpflag_fieldoffset) {
			// Baker: Mild reduction in jump_flag to avoid weird damage
			// from freemove's slightly higher jumps
			//int ofs = PRVM_ED_FindFieldOffset (prog, "jump_flag");
			float varjumpflag = PRVM_EDICTFIELDFLOAT(host_client->edict, sv.zircon_jumpflag_fieldoffset);

			//Con_PrintLinef ("%dis ground jump flag = %d  z vel %f", 
			//	Have_Flag ((int)PRVM_serveredictfloat(host_client->edict, flags), FL_ONGROUND_512),
			//	(int)varjumpflag,
			//	v[2]
			//);
			PRVM_EDICTFIELDFLOAT(host_client->edict, sv.zircon_jumpflag_fieldoffset) = varjumpflag* 0.84375;
			//varjumpflag = PRVM_EDICTFIELDFLOAT(host_client->edict, sv.zircon_jumpflag_fieldoffset);
			//Con_PrintLinef ("After %d", (int)varjumpflag);
//			if (varjumpflag)
//				v[2] *= 0.875;

		} else


		if (zircon_move_final_ground) { // Used to reduce velocity
			v = PRVM_serveredictvector(host_client->edict, velocity);
			if (v[2] < 0)
				v[2] = -3.0; // Clear downwards move
		}
		host_client->last_wanted_is_zircon_free_move = false; // TURN IT OFF, SINCE WE ARE DONE EXECUTING MOVES
	}

}

void SV_PlayerPhysics_ApplyClientMove (void)
{
	prvm_prog_t *prog = SVVM_prog;
	usercmd_t *move = &host_client->cmd;
	int j, movementloss, packetloss;

	if (!move->clx_receivetime)
		return;

	// note: a move can be applied multiple times if the client packets are
	// not coming as often as the physics is executed, and the move must be
	// applied before running qc each time because the id1 qc had a bug where
	// it clears self.button2 in PlayerJump, causing pogostick behavior if
	// moves are not applied every time before calling qc
	move->clx_is_applied = true;

	// set the edict fields
	PRVM_serveredictfloat(host_client->edict, button0) = move->buttons & 1;
	PRVM_serveredictfloat(host_client->edict, button2) = (move->buttons & 2)>>1;
	if (move->impulse)
		PRVM_serveredictfloat(host_client->edict, impulse) = move->impulse;
	// only send the impulse to qc once
	move->impulse = 0;

	movementloss = packetloss = 0;
	if (host_client->netconnection)
	{
		for (j = 0;j < NETGRAPH_PACKETS_256;j++)
			if (host_client->netconnection->incoming_netgraph[j].unreliablebytes == NETGRAPH_LOSTPACKET_NEG1)
				packetloss++;
		for (j = 0;j < NETGRAPH_PACKETS_256;j++)
			if (host_client->movement_count[j] < 0)
				movementloss++;
	}

	VectorCopy(move->viewangles, PRVM_serveredictvector(host_client->edict, v_angle));
	PRVM_serveredictfloat(host_client->edict, button3) = ((move->buttons >> 2) & 1);
	PRVM_serveredictfloat(host_client->edict, button4) = ((move->buttons >> 3) & 1);
	PRVM_serveredictfloat(host_client->edict, button5) = ((move->buttons >> 4) & 1);
	PRVM_serveredictfloat(host_client->edict, button6) = ((move->buttons >> 5) & 1);
	PRVM_serveredictfloat(host_client->edict, button7) = ((move->buttons >> 6) & 1);
	PRVM_serveredictfloat(host_client->edict, button8) = ((move->buttons >> 7) & 1);
	PRVM_serveredictfloat(host_client->edict, button9) = ((move->buttons >> 11) & 1);
	PRVM_serveredictfloat(host_client->edict, button10) = ((move->buttons >> 12) & 1);
	PRVM_serveredictfloat(host_client->edict, button11) = ((move->buttons >> 13) & 1);
	PRVM_serveredictfloat(host_client->edict, button12) = ((move->buttons >> 14) & 1);
	PRVM_serveredictfloat(host_client->edict, button13) = ((move->buttons >> 15) & 1);
	PRVM_serveredictfloat(host_client->edict, button14) = ((move->buttons >> 16) & 1);
	PRVM_serveredictfloat(host_client->edict, button15) = ((move->buttons >> 17) & 1);
	PRVM_serveredictfloat(host_client->edict, button16) = ((move->buttons >> 18) & 1);
	PRVM_serveredictfloat(host_client->edict, buttonuse) = ((move->buttons >> 8) & 1);
	PRVM_serveredictfloat(host_client->edict, buttonchat) = ((move->buttons >> 9) & 1);
	PRVM_serveredictfloat(host_client->edict, cursor_active) = ((move->buttons >> 10) & 1);
	VectorSet(PRVM_serveredictvector(host_client->edict, movement), move->clx_forwardmove, move->clx_sidemove, move->clx_upmove);
	VectorCopy(move->cursor_screen, PRVM_serveredictvector(host_client->edict, cursor_screen));
	VectorCopy(move->cursor_start, PRVM_serveredictvector(host_client->edict, cursor_trace_start));
	VectorCopy(move->cursor_impact, PRVM_serveredictvector(host_client->edict, cursor_trace_endpos));
	PRVM_serveredictedict(host_client->edict, cursor_trace_ent) = PRVM_EDICT_TO_PROG(PRVM_EDICT_NUM(move->cursor_entitynumber));
	PRVM_serveredictfloat(host_client->edict, ping) = host_client->ping * 1000.0;
	PRVM_serveredictfloat(host_client->edict, ping_packetloss) = packetloss / (float) NETGRAPH_PACKETS_256;
	PRVM_serveredictfloat(host_client->edict, ping_movementloss) = movementloss / (float) NETGRAPH_PACKETS_256;
}

static qbool SV_FrameLost(int framenum)
{
	if (host_client->entitydatabase5)
	{
		if (framenum <= host_client->entitydatabase5->latestframenum)
		{
			EntityFrame5_LostFrame(host_client->entitydatabase5, framenum);
			EntityFrameCSQC_LostFrame(host_client, framenum);
			return true;
		}
	}
	return false;
}

static void SV_FrameAck(int framenum)
{
	if (host_client->entitydatabase)
		EntityFrame_AckFrame(host_client->entitydatabase, framenum);
	else if (host_client->entitydatabase4)
		EntityFrame4_AckFrame(host_client->entitydatabase4, framenum, true);
	else if (host_client->entitydatabase5)
		EntityFrame5_AckFrame(host_client->entitydatabase5, framenum);
}

/*
===================
SV_ReadClientMessage
===================
*/
WARP_X_ (NetConn_ServerParsePacket clc_move takes us to SV_ReadClientMessage_ReadClientMove)
WARP_X_ (we exit we do SV_ReadClientMessage_ExecuteClientMoves)

WARP_X_CALLERS_ (NetConn_ServerParsePacket)

#include "sv_user_zmove.c.h"

void SV_ReadClientMessage(void)
{
	prvm_prog_t *prog = SVVM_prog;
	int netcmd, num, start;
	char *s, *p, *q;
	
	vec3_t zircon_move_final_pos = {0};
	int zircon_move_final_ground = false; // Used to reduce falling velocity

	if (sv_autodemo_perclient.integer >= 2)
		SV_WriteDemoMessage (host_client, &(host_client->netconnection->message), /*clientoserver?*/ true);

	//MSG_BeginReading ();
	host_client->download_chunks_perframe = 0;
	sv_numreadmoves = 0;

	// Baker: SV_ReadClientMessage This loop exits when?
	// netcmd == -1
	while (1) {
		if (!host_client->active) {
			// a command caused an error
			SV_DropClient (q_is_leaving_false, "Connection closing");
			return;
		}

		if (sv_message.badread) {
			Con_PrintLinef ("SV_ReadClientMessage: badread");
			SV_DropClient (q_is_leaving_false, "An internal server error occurred");
			return;
		}

		netcmd = MSG_ReadByte(&sv_message);

baker_loop_exits_here:
		if (netcmd == -1) {
			// end of message
			SV_ReadClientMessage_ExecuteClientMoves (zircon_move_final_pos, zircon_move_final_ground);
			break;
		}

		switch (netcmd) {
		default:
			Con_PrintLinef ("SV_ReadClientMessage: unknown command char %d (at offset 0x%x)", netcmd, 
				sv_message.readcount);
			if (developer_networking.integer)
				Com_HexDumpToConsole(sv_message.data, sv_message.cursize);
			SV_DropClient (q_is_leaving_false, "Unknown message sent to the server");
			return;

		case clc_nop:
			break;

		case clc_stringcmd:
			// allow reliable messages now as the client is done with initial loading
			if (host_client->sendsignon == 2)
				host_client->sendsignon = 0;
			s = MSG_ReadString (&sv_message, sv_readstring, sizeof(sv_readstring));
			q = NULL;
			for (p = s; *p; ++p) {
				switch(*p) {
				case 10:
				case 13:
					if (!q)
						q = p;
					break;
				default:
					if (q)
						goto clc_stringcmd_invalid; // newline seen, THEN something else -> possible exploit
					break;
				} // switch
			} // for p
			if (q)
				*q = 0;
			if (String_Starts_With_PRE(s, "spawn"/*, 5*/) ||
				String_Starts_With_PRE(s, "begin"/*, 5*/) ||
				String_Starts_With_PRE(s, "prespawn"/*, 8*/) )
				Cmd_ExecuteString (cmd_serverfromclient, s, src_client, /*lock mutex*/ true);
			else if (PRVM_serverfunction(SV_ParseClientCommand))
			{
				int restorevm_tempstringsbuf_cursize;
				restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;
				PRVM_G_INT(OFS_PARM0) = PRVM_SetTempString(prog, s);
				PRVM_serverglobalfloat(time) = sv.time;
				PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);

				prog->ExecuteProgram	(prog, PRVM_serverfunction(SV_ParseClientCommand), "QC function SV_ParseClientCommand is missing");

				prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
			}
			else
				Cmd_ExecuteString (cmd_serverfromclient, s, src_client, /*lockmutex?*/ true);
			break;

clc_stringcmd_invalid:
			Con_PrintLinef ("Received invalid stringcmd from %s", host_client->name);
			if (developer.integer > 0)
				Com_HexDumpToConsole((unsigned char *) s, (int)strlen(s));
			break;

		case clc_disconnect:
			SV_DropClient (q_is_leaving_true, sv.protocol == PROTOCOL_DARKPLACES8
			               ? MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring))
			               : "Disconnect by user"); // client wants to disconnect
			return;

		case clc_move:
			host_client->last_wanted_is_zircon_free_move = false; // ZMOVE 
			SV_ReadClientMessage_ReadClientMove ();
			break;

#if 5432
		case clc_zircon_move:
			// Where is execute .. top of loop
			host_client->last_wanted_is_zircon_free_move = true; // ZMOVE
			if (host_client->last_wanted_is_zircon_free_move) {
				// Baker: Do we need to check if it occurred here?
				// I don't think touch functions can fire here
				
				//if (host_client->zircon_warp_sequence == false) {
				//// Baker: If free move is wanted, we have to read the move even if we deny it.
				zircon_move_final_pos[0] = MSG_ReadFloat(&sv_message); // ZMOVE RECEIVE END MOVE
				zircon_move_final_pos[1] = MSG_ReadFloat(&sv_message); // ZMOVE RECEIVE END MOVE
				zircon_move_final_pos[2] = MSG_ReadFloat(&sv_message); // ZMOVE RECEIVE END MOVE
				zircon_move_final_ground = MSG_ReadByte(&sv_message); // ZMOVE RECEIVE END MOVE
			}
			if (ZMOVE_SV_DENIED) {
				if (Have_Flag (developer_movement.integer, /*SV*/ 2)) // ZMOVE_WARP
					Con_PrintLinef ("SV: ZMOVE denied due to warp %u", host_client->zircon_warp_sequence);
				host_client->last_wanted_is_zircon_free_move = false; // ZMOVE	
			}			
			SV_ReadClientMessage_ReadClientMove ();
			break;

		WARP_X_ (svc_zircon_warp)
		case clc_zircon_warp_ack: // ZMOVE_WARP
			SV_ReadClientMessage_Zircon_Warp_Ack_Read ();
			break;

#endif

		case clc_ackdownloaddata:
			start = MSG_ReadLong(&sv_message);
downloadx_sv_start_download_during_3:
			num = MSG_ReadShort(&sv_message);

			if (host_client->download_file && host_client->download_started) {
				int is_chunked_finish = 
					host_client->download_file && 
					host_client->download_chunked &&
					host_client->download_expectedposition == 0 &&
					start == (int)FS_FileSize(host_client->download_file);

				if (host_client->download_expectedposition == start || is_chunked_finish) {
					int size = (int)FS_FileSize(host_client->download_file);
					// a data block was successfully received by the client,
					// update the expected position on the next data block
					host_client->download_expectedposition = start + num;
					// if this was the last data block of the file, it's done
					if (is_chunked_finish || host_client->download_expectedposition >= 
						FS_FileSize(host_client->download_file) ) {
						// tell the client that the download finished
						// we need to calculate the crc now
						//
						// note: at this point the OS probably has the file
						// entirely in memory, so this is a faster operation
						// now than it was when the download started.
						//
						// it is also preferable to do this at the end of the
						// download rather than the start because it reduces
						// potential for Denial Of Service attacks against the
						// server.

						int crc;
						unsigned char *temp;
						FS_Seek(host_client->download_file, 0, SEEK_SET);
						temp = (unsigned char *) Mem_Alloc(tempmempool, size);
						FS_Read(host_client->download_file, temp, size);
						crc = CRC_Block(temp, size);
						Mem_Free(temp);
						// calculated crc, send the file info to the client
						// (so that it can verify the data)
downloadx_sv_finish_0:
						SV_ClientCommandsf		(NEWLINE 
							"cl_downloadfinished %d %d %s" NEWLINE, 
							size, 
							crc, 
							host_client->download_name);
						Con_DPrintLinef			("Download of %s by %s has finished", host_client->download_name, host_client->name);
						FS_Close(host_client->download_file);

						host_client->download_file = NULL;
						host_client->download_name[0] = 0;
						host_client->download_expectedposition = 0;
						host_client->download_started = false;
					}
				}
				else {
					// a data block was lost, reset to the expected position
					// and resume sending from there
					FS_Seek (host_client->download_file, host_client->download_expectedposition, SEEK_SET);
				}
			}
			break;

		case clc_ackframe:
			if (sv_message.badread) Con_PrintLinef ("SV_ReadClientMessage: badread at %s:%d", __FILE__, __LINE__);
			num = MSG_ReadLong(&sv_message);
			if (sv_message.badread) Con_PrintLinef ("SV_ReadClientMessage: badread at %s:%d", __FILE__, __LINE__);
			if (developer_networkentities.integer >= 10)
				Con_PrintLinef ("recv clc_ackframe %d", num);
			// if the client hasn't progressed through signons yet,
			// ignore any clc_ackframes we get (they're probably from the
			// previous level)
			if (host_client->begun && host_client->latestframenum < num) {
				for (int i = host_client->latestframenum + 1;i < num;i++)
					if (!SV_FrameLost(i))
						break;
				SV_FrameAck(num);
				host_client->latestframenum = num; // PHYSICAL
			} // if
			break;
		} // switch
	} // while
}

