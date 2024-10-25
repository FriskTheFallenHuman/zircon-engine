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
// cl_input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"
#include "csprogs.h"
#include "thread.h"

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_jump, in_attack, in_use;
kbutton_t	in_up, in_down;
// LadyHavoc: added 6 new buttons
kbutton_t	in_button3, in_button4, in_button5, in_button6, in_button7, in_button8;
//even more
kbutton_t	in_button9, in_button10, in_button11, in_button12, in_button13, in_button14, in_button15, in_button16;

int			in_impulse;


static void KeyDown (cmd_state_t *cmd, kbutton_t *b)
{
	int k;
	const char *c;

	c = Cmd_Argv(cmd, 1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_PrintLinef ("Three keys down for a button!");
		return;
	}

	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

static void KeyUp (cmd_state_t *cmd, kbutton_t *b)
{
	int k;
	const char *c;

	c = Cmd_Argv(cmd, 1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)
	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

static void IN_KLookDown(cmd_state_t *cmd) {KeyDown(cmd, &in_klook);}
static void IN_KLookUp(cmd_state_t *cmd) {KeyUp(cmd, &in_klook);}
static void IN_MLookDown(cmd_state_t *cmd) {KeyDown(cmd, &in_mlook);}
static void IN_MLookUp(cmd_state_t *cmd)
{
	KeyUp(cmd, &in_mlook);
	if ( !(in_mlook.state&1) && lookspring.value)
		V_StartPitchDrift_f(cmd);
}
static void IN_UpDown(cmd_state_t *cmd) {KeyDown(cmd, &in_up);}
static void IN_UpUp(cmd_state_t *cmd) {KeyUp(cmd, &in_up);}
static void IN_DownDown(cmd_state_t *cmd) {KeyDown(cmd, &in_down);}
static void IN_DownUp(cmd_state_t *cmd) {KeyUp(cmd, &in_down);}
static void IN_LeftDown(cmd_state_t *cmd) {
	KeyDown(cmd, &in_left);
}
static void IN_LeftUp(cmd_state_t *cmd)  {
	KeyUp(cmd, &in_left);
}
static void IN_RightDown(cmd_state_t *cmd) {
	KeyDown(cmd, &in_right);
}
static void IN_RightUp(cmd_state_t *cmd) {
	KeyUp(cmd, &in_right);
}
static void IN_ForwardDown(cmd_state_t *cmd) {KeyDown(cmd, &in_forward);}
static void IN_ForwardUp(cmd_state_t *cmd) {KeyUp(cmd, &in_forward);}
static void IN_BackDown(cmd_state_t *cmd) {KeyDown(cmd, &in_back);}
static void IN_BackUp(cmd_state_t *cmd) {KeyUp(cmd, &in_back);}
static void IN_LookupDown(cmd_state_t *cmd) {KeyDown(cmd, &in_lookup);}
static void IN_LookupUp(cmd_state_t *cmd) {KeyUp(cmd, &in_lookup);}
static void IN_LookdownDown(cmd_state_t *cmd) {KeyDown(cmd, &in_lookdown);}
static void IN_LookdownUp(cmd_state_t *cmd) {KeyUp(cmd, &in_lookdown);}
static void IN_MoveleftDown(cmd_state_t *cmd) {KeyDown(cmd, &in_moveleft);}
static void IN_MoveleftUp(cmd_state_t *cmd) {KeyUp(cmd, &in_moveleft);}
static void IN_MoverightDown(cmd_state_t *cmd) {KeyDown(cmd, &in_moveright);}
static void IN_MoverightUp(cmd_state_t *cmd) {
	KeyUp(cmd, &in_moveright);
}

static void IN_SpeedDown(cmd_state_t *cmd) {KeyDown(cmd, &in_speed);}
static void IN_SpeedUp(cmd_state_t *cmd) {KeyUp(cmd, &in_speed);}
static void IN_StrafeDown(cmd_state_t *cmd) {KeyDown(cmd, &in_strafe);}
static void IN_StrafeUp(cmd_state_t *cmd) {KeyUp(cmd, &in_strafe);}

static void IN_AttackDown(cmd_state_t *cmd) {KeyDown(cmd, &in_attack);}
static void IN_AttackUp(cmd_state_t *cmd) {KeyUp(cmd, &in_attack);}

static void IN_UseDown(cmd_state_t *cmd) {KeyDown(cmd, &in_use);}
static void IN_UseUp(cmd_state_t *cmd) {KeyUp(cmd, &in_use);}

// LadyHavoc: added 6 new buttons
static void IN_Button3Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button3);}
static void IN_Button3Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button3);}
static void IN_Button4Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button4);}
static void IN_Button4Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button4);}
static void IN_Button5Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button5);}
static void IN_Button5Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button5);}
static void IN_Button6Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button6);}
static void IN_Button6Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button6);}
static void IN_Button7Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button7);}
static void IN_Button7Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button7);}
static void IN_Button8Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button8);}
static void IN_Button8Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button8);}

static void IN_Button9Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button9);}
static void IN_Button9Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button9);}
static void IN_Button10Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button10);}
static void IN_Button10Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button10);}
static void IN_Button11Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button11);}
static void IN_Button11Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button11);}
static void IN_Button12Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button12);}
static void IN_Button12Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button12);}
static void IN_Button13Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button13);}
static void IN_Button13Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button13);}
static void IN_Button14Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button14);}
static void IN_Button14Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button14);}
static void IN_Button15Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button15);}
static void IN_Button15Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button15);}
static void IN_Button16Down(cmd_state_t *cmd) {KeyDown(cmd, &in_button16);}
static void IN_Button16Up(cmd_state_t *cmd) {KeyUp(cmd, &in_button16);}

static void IN_JumpDown(cmd_state_t *cmd) {KeyDown(cmd, &in_jump);}
static void IN_JumpUp(cmd_state_t *cmd) {KeyUp(cmd, &in_jump);}

static void IN_Impulse(cmd_state_t *cmd) {
	in_impulse=atoi(Cmd_Argv(cmd, 1));
}

in_bestweapon_info_t in_bestweapon_info[IN_BESTWEAPON_MAX];

static void IN_BestWeapon_Register(const char *name, int impulse, int weaponbit, int activeweaponcode, int ammostat, int ammomin)
{
	int i;
	for(i = 0; i < IN_BESTWEAPON_MAX && in_bestweapon_info[i].impulse; ++i)
		if (in_bestweapon_info[i].impulse == impulse)
			break;
	if (i >= IN_BESTWEAPON_MAX)
	{
		Con_PrintLinef ("no slot left for weapon definition; increase IN_BESTWEAPON_MAX");
		return; // sorry
	}
	c_strlcpy (in_bestweapon_info[i].name, name);
	in_bestweapon_info[i].impulse = impulse;
	if (weaponbit != -1)
		in_bestweapon_info[i].weaponbit = weaponbit;
	if (activeweaponcode != -1)
		in_bestweapon_info[i].activeweaponcode = activeweaponcode;
	if (ammostat != -1)
		in_bestweapon_info[i].ammostat = ammostat;
	if (ammomin != -1)
		in_bestweapon_info[i].ammomin = ammomin;
}

void IN_BestWeapon_ResetData (void)
{
	memset(in_bestweapon_info, 0, sizeof(in_bestweapon_info));
	IN_BestWeapon_Register("1", 1, IT_AXE, IT_AXE, STAT_SHELLS, 0);
	IN_BestWeapon_Register("2", 2, IT_SHOTGUN, IT_SHOTGUN, STAT_SHELLS, 1);
	IN_BestWeapon_Register("3", 3, IT_SUPER_SHOTGUN, IT_SUPER_SHOTGUN, STAT_SHELLS, 1);
	IN_BestWeapon_Register("4", 4, IT_NAILGUN, IT_NAILGUN, STAT_NAILS, 1);
	IN_BestWeapon_Register("5", 5, IT_SUPER_NAILGUN, IT_SUPER_NAILGUN, STAT_NAILS, 1);
	IN_BestWeapon_Register("6", 6, IT_GRENADE_LAUNCHER, IT_GRENADE_LAUNCHER, STAT_ROCKETS, 1);
	IN_BestWeapon_Register("7", 7, IT_ROCKET_LAUNCHER, IT_ROCKET_LAUNCHER, STAT_ROCKETS, 1);
	IN_BestWeapon_Register("8", 8, IT_LIGHTNING, IT_LIGHTNING, STAT_CELLS, 1);
	IN_BestWeapon_Register("9", 9, 128, 128, STAT_CELLS, 1); // generic energy weapon for mods
	IN_BestWeapon_Register("p", 209, 128, 128, STAT_CELLS, 1); // dpmod plasma gun
	IN_BestWeapon_Register("w", 210, 8388608, 8388608, STAT_CELLS, 1); // dpmod plasma wave cannon
	IN_BestWeapon_Register("l", 225, HIT_LASER_CANNON, HIT_LASER_CANNON, STAT_CELLS, 1); // hipnotic laser cannon
	IN_BestWeapon_Register("h", 226, HIT_MJOLNIR, HIT_MJOLNIR, STAT_CELLS, 0); // hipnotic mjolnir hammer
}

static void IN_BestWeapon_Register_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) == 7)
	{
		IN_BestWeapon_Register(
			Cmd_Argv(cmd, 1),
			atoi(Cmd_Argv(cmd, 2)),
			atoi(Cmd_Argv(cmd, 3)),
			atoi(Cmd_Argv(cmd, 4)),
			atoi(Cmd_Argv(cmd, 5)),
			atoi(Cmd_Argv(cmd, 6))
		);
	}
	else if (Cmd_Argc(cmd) == 2 && String_Match(Cmd_Argv(cmd, 1), "clear"))
	{
		memset(in_bestweapon_info, 0, sizeof(in_bestweapon_info));
	}
	else if (Cmd_Argc(cmd) == 2 && String_Match(Cmd_Argv(cmd, 1), "quake"))
	{
		IN_BestWeapon_ResetData();
	}
	else
	{
		Con_Printf ("Usage: %s weaponshortname impulse itemcode activeweaponcode ammostat ammomin; %s clear; %s quake\n", Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0));
	}
}

static void IN_BestWeapon_f(cmd_state_t *cmd)
{
	int i, n;
	const char *t;
	if (Cmd_Argc(cmd) < 2)
	{
		Con_Printf ("bestweapon requires 1 or more parameters\n");
		return;
	}
	for (i = 1;i < Cmd_Argc(cmd);i++)
	{
		t = Cmd_Argv(cmd, i);
		// figure out which weapon this character refers to
		for (n = 0;n < IN_BESTWEAPON_MAX && in_bestweapon_info[n].impulse;n++)
		{
			if (String_Match(in_bestweapon_info[n].name, t))
			{
				// we found out what weapon this character refers to
				// check if the inventory contains the weapon and enough ammo
				if ((cl.stats[STAT_ITEMS] & in_bestweapon_info[n].weaponbit) && (cl.stats[in_bestweapon_info[n].ammostat] >= in_bestweapon_info[n].ammomin))
				{
					// we found one of the weapons the player wanted
					// send an impulse to switch to it
					in_impulse = in_bestweapon_info[n].impulse;
					return;
				}
				break;
			}
		}
		// if we couldn't identify the weapon we just ignore it and continue checking for other weapons
	}
	// if we couldn't find any of the weapons, there's nothing more we can do...
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	qbool	impulsedown, impulseup, down;

	impulsedown = (key->state & 2) != 0;
	impulseup = (key->state & 4) != 0;
	down = (key->state & 1) != 0;
	val = 0;

	if (impulsedown && !impulseup)
	{
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}
	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}
	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}
	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state &= 1;		// clear impulses

	return val;
}




//==========================================================================

cvar_t cl_upspeed = {CF_CLIENT | CF_ARCHIVE, "cl_upspeed","400","vertical movement speed (while swimming or flying)"};
cvar_t cl_forwardspeed = {CF_CLIENT | CF_ARCHIVE, "cl_forwardspeed","400","forward movement speed"};
cvar_t cl_backspeed = {CF_CLIENT | CF_ARCHIVE, "cl_backspeed","400","backward movement speed"};
cvar_t cl_sidespeed = {CF_CLIENT | CF_ARCHIVE, "cl_sidespeed","350","strafe movement speed"};

cvar_t cl_movespeedkey = {CF_CLIENT | CF_ARCHIVE, "cl_movespeedkey","2.0","how much +speed multiplies keyboard movement speed"};
cvar_t cl_movecliptokeyboard = {CF_CLIENT, "cl_movecliptokeyboard", "0", "if set to 1, any move is clipped to the nine keyboard states; if set to 2, only the direction is clipped, not the amount"};

cvar_t cl_yawspeed = {CF_CLIENT | CF_ARCHIVE, "cl_yawspeed","140","keyboard yaw turning speed"};
cvar_t cl_pitchspeed = {CF_CLIENT | CF_ARCHIVE, "cl_pitchspeed","150","keyboard pitch turning speed"};

cvar_t cl_anglespeedkey = {CF_CLIENT | CF_ARCHIVE, "cl_anglespeedkey","1.5","how much +speed multiplies keyboard turning speed"};

cvar_t developer_movement = {CF_SHARED, "developer_movement", "0", "developer movement messages [Zircon]"};

cvar_t cl_movement = {CF_CLIENT | CF_ARCHIVE, "cl_movement", "1", "enables clientside prediction of your player movement on DP servers but not in singleplayer, 2 even in single player (use cl_nopred for QWSV servers) [Zircon]"};
cvar_t cl_zircon_move = {CF_CLIENT| CF_ARCHIVE, "cl_zircon_move", "1", "enables Zircon/Kleskby free movement if available, requires cl_movement 2 in single player [Zircon]"};
cvar_t cl_movement_collide_nonsolid_red = {CF_CLIENT, "cl_movement_collide_nonsolid_red", "0", "prediction non-solids get EF_RED glow effect to help identify [Zircon]"};
cvar_t cl_movement_collide_models = {CF_CLIENT, "cl_movement_collide_models", "1", "enables collision against monsters and non-solid for clientside prediction of your player movement [Zircon]"};

cvar_t cl_movement_replay = {CF_CLIENT, "cl_movement_replay", "1", "use engine prediction"};
cvar_t cl_movement_nettimeout = {CF_CLIENT | CF_ARCHIVE, "cl_movement_nettimeout", "0.3", "stops predicting moves when server is lagging badly (avoids major performance problems), timeout in seconds"};
cvar_t cl_movement_minping = {CF_CLIENT | CF_ARCHIVE, "cl_movement_minping", "0", "whether to use prediction when ping is lower than this value in milliseconds[Zircon]" }; // [Zircon] 75
cvar_t cl_movement_track_canjump = {CF_CLIENT | CF_ARCHIVE, "cl_movement_track_canjump", "1", "track if the player released the jump key between two jumps to decide if he is able to jump or not; when off, this causes some \"sliding\" slightly above the floor when the jump key is held too long; if the mod allows repeated jumping by holding space all the time, this has to be set to zero too"};
cvar_t cl_movement_maxspeed = {CF_CLIENT, "cl_movement_maxspeed", "320", "how fast you can move (should match sv_maxspeed)"};
cvar_t cl_movement_maxairspeed = {CF_CLIENT, "cl_movement_maxairspeed", "30", "how fast you can move while in the air (should match sv_maxairspeed)"};
cvar_t cl_movement_stopspeed = {CF_CLIENT, "cl_movement_stopspeed", "100", "speed below which you will be slowed rapidly to a stop rather than sliding endlessly (should match sv_stopspeed)"};
cvar_t cl_movement_friction = {CF_CLIENT, "cl_movement_friction", "4", "how fast you slow down (should match sv_friction)"};
cvar_t cl_movement_wallfriction = {CF_CLIENT, "cl_movement_wallfriction", "1", "how fast you slow down while sliding along a wall (should match sv_wallfriction)"};
cvar_t cl_movement_waterfriction = {CF_CLIENT, "cl_movement_waterfriction", "-1", "how fast you slow down (should match sv_waterfriction), if less than 0 the cl_movement_friction variable is used instead"};
cvar_t cl_movement_edgefriction = {CF_CLIENT, "cl_movement_edgefriction", "1", "how much to slow down when you may be about to fall off a ledge (should match edgefriction)"};
cvar_t cl_movement_stepheight = {CF_CLIENT, "cl_movement_stepheight", "18", "how tall a step you can step in one instant (should match sv_stepheight)"};
cvar_t cl_movement_accelerate = {CF_CLIENT, "cl_movement_accelerate", "10", "how fast you accelerate (should match sv_accelerate)"};
cvar_t cl_movement_airaccelerate = {CF_CLIENT, "cl_movement_airaccelerate", "-1", "how fast you accelerate while in the air (should match sv_airaccelerate), if less than 0 the cl_movement_accelerate variable is used instead"};
cvar_t cl_movement_wateraccelerate = {CF_CLIENT, "cl_movement_wateraccelerate", "-1", "how fast you accelerate while in water (should match sv_wateraccelerate), if less than 0 the cl_movement_accelerate variable is used instead"};
cvar_t cl_movement_jumpvelocity = {CF_CLIENT, "cl_movement_jumpvelocity", "270", "how fast you move upward when you begin a jump (should match the quakec code)"};
cvar_t cl_movement_airaccel_qw = {CF_CLIENT, "cl_movement_airaccel_qw", "1", "ratio of QW-style air control as opposed to simple acceleration (reduces speed gain when zigzagging) (should match sv_airaccel_qw); when < 0, the speed is clamped against the maximum allowed forward speed after the move"};
cvar_t cl_movement_airaccel_sideways_friction = {CF_CLIENT, "cl_movement_airaccel_sideways_friction", "0", "anti-sideways movement stabilization (should match sv_airaccel_sideways_friction); when < 0, only so much friction is applied that braking (by accelerating backwards) cannot be stronger"};
cvar_t cl_nopred = {CF_CLIENT | CF_ARCHIVE, "cl_nopred", "0", "(QWSV only) disables player movement prediction when playing on QWSV servers (this setting is separate from cl_movement because player expectations are different when playing on DP vs QW servers)"};

cvar_t in_pitch_min = {CF_CLIENT, "in_pitch_min", "-90", "how far you can aim upward (quake used -70)"};
cvar_t in_pitch_max = {CF_CLIENT, "in_pitch_max", "90", "how far you can aim downward (quake used 80)"};

cvar_t m_filter = {CF_CLIENT | CF_ARCHIVE, "m_filter","0", "smoothes mouse movement, less responsive but smoother aiming"};
cvar_t m_accelerate = {CF_CLIENT | CF_ARCHIVE, "m_accelerate","1", "linear mouse acceleration factor (set to 1 to disable the linear acceleration and use only the power or natural acceleration; set to 0 to disable all acceleration)"};
cvar_t m_accelerate_minspeed = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_minspeed","5000", "below this speed in px/s, no acceleration is done, with a linear slope between (applied only on linear acceleration)"};
cvar_t m_accelerate_maxspeed = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_maxspeed","10000", "above this speed in px/s, full acceleration is done, with a linear slope between (applied only on linear acceleration)"};
cvar_t m_accelerate_filter = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_filter","0", "linear mouse acceleration factor filtering lowpass constant in seconds (set to 0 for no filtering)"};
cvar_t m_accelerate_power_offset = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_offset","0", "below this speed in px/ms, no power acceleration is done"};
cvar_t m_accelerate_power = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power","2", "acceleration power (must be above 1 to be useful)"};
cvar_t m_accelerate_power_senscap = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_senscap", "0", "maximum acceleration factor generated by power acceleration; use 0 for unbounded"};
cvar_t m_accelerate_power_strength = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_power_strength", "0", "strength of the power mouse acceleration effect"};
cvar_t m_accelerate_natural_strength = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_strength", "0", "How quickly the accelsensitivity approaches the m_accelerate_natural_accelsenscap, values are compressed between 0 and 1 but higher numbers are allowed"};
cvar_t m_accelerate_natural_accelsenscap = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_accelsenscap", "0", "Horizontal asymptote that sets the maximum value for the natural mouse acceleration curve, value 2, for example, means that the maximum sensitivity is 2 times the base sensitivity"};
cvar_t m_accelerate_natural_offset = {CF_CLIENT | CF_ARCHIVE, "m_accelerate_natural_offset", "0", "below this speed in px/ms, no natural acceleration is done"};

cvar_t cl_netfps = {CF_CLIENT | CF_ARCHIVE, "cl_netfps","72", "how many input packets to send to server each second"};
cvar_t cl_netrepeatinput = {CF_CLIENT | CF_ARCHIVE, "cl_netrepeatinput", "1", "how many packets in a row can be lost without movement issues when using cl_movement (technically how many input messages to repeat in each packet that have not yet been acknowledged by the server), only affects DP7 and later servers (Quake uses 0, QuakeWorld uses 2, and just for comparison Quake3 uses 1)"};
cvar_t cl_netimmediatebuttons = {CF_CLIENT | CF_ARCHIVE, "cl_netimmediatebuttons", "1", "sends extra packets whenever your buttons change or an impulse is used (basically: whenever you click fire or change weapon)"};

cvar_t cl_nodelta = {CF_CLIENT, "cl_nodelta", "0", "disables delta compression of non-player entities in QW network protocol"};

cvar_t cl_csqc_generatemousemoveevents = {CF_CLIENT, "cl_csqc_generatemousemoveevents", "1", "enables calls to CSQC_InputEvent with type 2, for compliance with EXT_CSQC spec"};

extern cvar_t v_flipped;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (in_speed.state & 1)
		speed = cl.realframetime * cl_anglespeedkey.value;
	else
		speed = cl.realframetime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed.value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed.value*CL_KeyState (&in_left);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed.value * CL_KeyState (&in_back);
	}

	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed.value * down;

	if (up || down)
		V_StopPitchDrift ();

	cl.viewangles[YAW] = ANGLEMOD(cl.viewangles[YAW]);
	cl.viewangles[PITCH] = ANGLEMOD(cl.viewangles[PITCH]);
	if (cl.viewangles[YAW] >= 180)
		cl.viewangles[YAW] -= 360;
	if (cl.viewangles[PITCH] >= 180)
		cl.viewangles[PITCH] -= 360;
        // TODO: honor serverinfo minpitch and maxpitch values in PROTOCOL_QUAKEWORLD
        // TODO: honor proquake pq_fullpitch cvar when playing on proquake server (server stuffcmd's this to 0 usually)
	cl.viewangles[PITCH] = bound(in_pitch_min.value, cl.viewangles[PITCH], in_pitch_max.value);
	cl.viewangles[ROLL] = bound(-180, cl.viewangles[ROLL], 180);
}

int cl_ignoremousemoves = 2;

/*
================
CL_Input

Send the intended movement message to the server
================
*/
void CL_Input (void)
{
	float mx, my;
	static float old_mouse_x = 0, old_mouse_y = 0;

	// clamp before the move to prevent starting with bad angles
	CL_AdjustAngles ();

	if (v_flipped.integer)
		cl.viewangles[YAW] = -cl.viewangles[YAW];

	// reset some of the command fields
	cl.mcmd.clx_forwardmove = 0;
	cl.mcmd.clx_sidemove = 0;
	cl.mcmd.clx_upmove = 0;

	// get basic movement from keyboard
	if (in_strafe.state & 1) {
		cl.mcmd.clx_sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
		cl.mcmd.clx_sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
	}

	cl.mcmd.clx_sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
	cl.mcmd.clx_sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);

	cl.mcmd.clx_upmove += cl_upspeed.value * CL_KeyState (&in_up);
	cl.mcmd.clx_upmove -= cl_upspeed.value * CL_KeyState (&in_down);

	if (! (in_klook.state & 1) ) {
		cl.mcmd.clx_forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
		cl.mcmd.clx_forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
	}

	// adjust for speed key
	if (in_speed.state & 1) {
		cl.mcmd.clx_forwardmove *= cl_movespeedkey.value;
		cl.mcmd.clx_sidemove *= cl_movespeedkey.value;
		cl.mcmd.clx_upmove *= cl_movespeedkey.value;
	}

	// allow mice or other external controllers to add to the move
	IN_Move ();

	// send mouse move to csqc
	if (cl.csqc_loaded && cl_csqc_generatemousemoveevents.integer /*d: 1*/) {
		if (cl.csqc_wantsmousemove) {
			// event type 3 is a DP_CSQC thing
			static int oldwindowmouse[2];
			if (oldwindowmouse[0] != in_windowmouse_x || oldwindowmouse[1] != in_windowmouse_y) {
				CL_VM_InputEvent(3, in_windowmouse_x * vid_conwidth.value / vid.width, in_windowmouse_y * vid_conheight.value / vid.height);
				oldwindowmouse[0] = in_windowmouse_x;
				oldwindowmouse[1] = in_windowmouse_y;
			}
		}
		else
		{
			if (in_mouse_x || in_mouse_y)
				CL_VM_InputEvent(2, in_mouse_x, in_mouse_y);
		}
	}

	// apply m_accelerate if it is on
	if (m_accelerate.value /*d: 1*/ > 0) {
		float mouse_deltadist = sqrtf(in_mouse_x * in_mouse_x + in_mouse_y * in_mouse_y);
		float speed = mouse_deltadist / cl.realframetime;
		static float averagespeed = 0;
		float f, mi, ma;
		if (m_accelerate_filter.value > 0)
			f = bound(0, cl.realframetime / m_accelerate_filter.value, 1);
		else
			f = 1;
		averagespeed = speed * f + averagespeed * (1 - f);

		// Note: this check is technically unnecessary, as everything in here cancels out if it is zero.
		if (m_accelerate.value /*d: 1*/ != 1.0f) {
			// First do linear slope acceleration which was ripped "in
			// spirit" from many classic mouse driver implementations.
			// If m_accelerate.value == 1, this code does nothing at all.

			mi = max(1, m_accelerate_minspeed.value);
			ma = max(m_accelerate_minspeed.value + 1, m_accelerate_maxspeed.value);

			if (averagespeed <= mi)
			{
				f = 1;
			}
			else if (averagespeed >= ma)
			{
				f = m_accelerate.value;
			}
			else
			{
				f = averagespeed;
				f = (f - mi) / (ma - mi) * (m_accelerate.value - 1) + 1;
			}

			in_mouse_x *= f;
			in_mouse_y *= f;
		}

		// Note: this check is technically unnecessary, as everything in here cancels out if it is zero.
		if (m_accelerate_power_strength.value /*d: 0*/ != 0.0f) {
			// Then do Quake Live-style power acceleration.
			// Note that this behavior REPLACES the usual
			// sensitivity, so we apply it but then divide by
			// sensitivity.value so that the later multiplication
			// restores it again.
			float accelsens = 1.0f;
			float adjusted_speed_pxms = (averagespeed * 0.001f - m_accelerate_power_offset.value) * m_accelerate_power_strength.value;
			float inv_sensitivity = 1.0f / sensitivity.value;
			if (adjusted_speed_pxms > 0) {
				if (m_accelerate_power.value > 1.0f) {
					// TODO: How does this interact with sensitivity changes? Is this intended?
					// Currently: more sensitivity = less acceleration at same pixel speed.
					accelsens += expf((m_accelerate_power.value - 1.0f) * logf(adjusted_speed_pxms)) * inv_sensitivity;
				}
				else
				{
					// The limit of the then-branch for m_accelerate_power -> 1.
					accelsens += inv_sensitivity;
					// Note: QL had just accelsens = 1.0f.
					// This is mathematically wrong though.
				}
			}
			else
			{
				// The limit of the then-branch for adjusted_speed -> 0.
				// accelsens += 0.0f;
			}
			if (m_accelerate_power_senscap.value > 0.0f && accelsens > m_accelerate_power_senscap.value * inv_sensitivity)
			{
				// TODO: How does this interact with sensitivity changes? Is this intended?
				// Currently: senscap is in absolute sensitivity units, so if senscap < sensitivity, it overrides.
				accelsens = m_accelerate_power_senscap.value * inv_sensitivity;
			}

			in_mouse_x *= accelsens;
			in_mouse_y *= accelsens;
		}

		if (m_accelerate_natural_strength.value /*d: 0*/ > 0.0f && m_accelerate_natural_accelsenscap.value /*d: 0*/ >= 0.0f)
		{
			float accelsens = 1.0f;
			float adjusted_speed_pxms = (averagespeed * 0.001f - m_accelerate_natural_offset.value);

			if (adjusted_speed_pxms > 0 && m_accelerate_natural_accelsenscap.value != 1.0f)
			{
				float adjusted_accelsenscap = m_accelerate_natural_accelsenscap.value - 1.0f;
				// This equation is made to multiply the sensitivity for a factor between 1 and m_accelerate_natural_accelsenscap
				// this means there is no need to divide it for the sensitivity.value as the whole
				// expression needs to be multiplied by the sensitivity at the end instead of only having the sens multiplied
				accelsens += (adjusted_accelsenscap - adjusted_accelsenscap * exp( - ((adjusted_speed_pxms * m_accelerate_natural_strength.value) / fabs(adjusted_accelsenscap) )));
			}

			in_mouse_x *= accelsens;
			in_mouse_y *= accelsens;
		}
	}

	// apply m_filter if it is on
	mx = in_mouse_x;
	my = in_mouse_y;
	if (m_filter.integer /*d: 0*/) {
		in_mouse_x = (mx + old_mouse_x) * 0.5;
		in_mouse_y = (my + old_mouse_y) * 0.5;
	}
	old_mouse_x = mx;
	old_mouse_y = my;

	// ignore a mouse move if mouse was activated/deactivated this frame
	if (cl_ignoremousemoves) {
		cl_ignoremousemoves--;
		in_mouse_x = old_mouse_x = 0;
		in_mouse_y = old_mouse_y = 0;
	}

	// if not in menu, apply mouse move to viewangles/movement
	if (!key_consoleactive && key_dest == key_game && !cl.csqc_wantsmousemove && cl_prydoncursor.integer <= 0) {
		float modulatedsensitivity = sensitivity.value * cl.sensitivityscale;
		if (in_strafe.state & 1)
		{
			// strafing mode, all looking is movement
			V_StopPitchDrift();
			cl.mcmd.clx_sidemove += m_side.value * in_mouse_x * modulatedsensitivity;
			if (noclip_anglehack)
				cl.mcmd.clx_upmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
			else
				cl.mcmd.clx_forwardmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
		}
		else if ((in_mlook.state & 1) || freelook.integer)
		{
			// mouselook, lookstrafe causes turning to become strafing
			V_StopPitchDrift();
			if (lookstrafe.integer)
				cl.mcmd.clx_sidemove += m_side.value * in_mouse_x * modulatedsensitivity;
			else
				cl.viewangles[YAW] -= m_yaw.value * in_mouse_x * modulatedsensitivity * cl.viewzoom;
			cl.viewangles[PITCH] += m_pitch.value * in_mouse_y * modulatedsensitivity * cl.viewzoom;
		}
		else
		{
			// non-mouselook, yaw turning and forward/back movement
			cl.viewangles[YAW] -= m_yaw.value * in_mouse_x * modulatedsensitivity * cl.viewzoom;
			cl.mcmd.clx_forwardmove -= m_forward.value * in_mouse_y * modulatedsensitivity;
		}
	}
	else // don't pitch drift when csqc is controlling the mouse
	{
		// mouse interacting with the scene, mostly stationary view
		V_StopPitchDrift();
		// update prydon cursor
		cl.mcmd.cursor_screen[0] = in_windowmouse_x * 2.0 / vid.width - 1.0;
		cl.mcmd.cursor_screen[1] = in_windowmouse_y * 2.0 / vid.height - 1.0;
	}

	if (v_flipped.integer /*d: 0*/)
	{
		cl.viewangles[YAW] = -cl.viewangles[YAW];
		cl.mcmd.clx_sidemove = -cl.mcmd.clx_sidemove;
	}

	// clamp after the move to prevent rendering with bad angles
	CL_AdjustAngles ();

	if (cl_movecliptokeyboard.integer /*d: 0*/) {
		vec_t f = 1;
		if (in_speed.state & 1)
			f *= cl_movespeedkey.value;
		if (cl_movecliptokeyboard.integer == 2) {
			// digital direction, analog amount
			vec_t wishvel_x, wishvel_y;
			wishvel_x = fabs(cl.mcmd.clx_forwardmove);
			wishvel_y = fabs(cl.mcmd.clx_sidemove);
			if (wishvel_x != 0 && wishvel_y != 0 && wishvel_x != wishvel_y)
			{
				vec_t wishspeed = sqrt(wishvel_x * wishvel_x + wishvel_y * wishvel_y);
				if (wishvel_x >= 2 * wishvel_y)
				{
					// pure X motion
					if (cl.mcmd.clx_forwardmove > 0)
						cl.mcmd.clx_forwardmove = wishspeed;
					else
						cl.mcmd.clx_forwardmove = -wishspeed;
					cl.mcmd.clx_sidemove = 0;
				}
				else if (wishvel_y >= 2 * wishvel_x)
				{
					// pure Y motion
					cl.mcmd.clx_forwardmove = 0;
					if (cl.mcmd.clx_sidemove > 0)
						cl.mcmd.clx_sidemove = wishspeed;
					else
						cl.mcmd.clx_sidemove = -wishspeed;
				}
				else
				{
					// diagonal
					if (cl.mcmd.clx_forwardmove > 0)
						cl.mcmd.clx_forwardmove = 0.70710678118654752440 * wishspeed;
					else
						cl.mcmd.clx_forwardmove = -0.70710678118654752440 * wishspeed;
					if (cl.mcmd.clx_sidemove > 0)
						cl.mcmd.clx_sidemove = 0.70710678118654752440 * wishspeed;
					else
						cl.mcmd.clx_sidemove = -0.70710678118654752440 * wishspeed;
				}
			}
		}
		else if (cl_movecliptokeyboard.integer /*d:0*/)
		{
			// digital direction, digital amount
			if (cl.mcmd.clx_sidemove >= cl_sidespeed.value * f * 0.5)
				cl.mcmd.clx_sidemove = cl_sidespeed.value * f;
			else if (cl.mcmd.clx_sidemove <= -cl_sidespeed.value * f * 0.5)
				cl.mcmd.clx_sidemove = -cl_sidespeed.value * f;
			else
				cl.mcmd.clx_sidemove = 0;
			if (cl.mcmd.clx_forwardmove >= cl_forwardspeed.value * f * 0.5)
				cl.mcmd.clx_forwardmove = cl_forwardspeed.value * f;
			else if (cl.mcmd.clx_forwardmove <= -cl_backspeed.value * f * 0.5)
				cl.mcmd.clx_forwardmove = -cl_backspeed.value * f;
			else
				cl.mcmd.clx_forwardmove = 0;
		}
	}
}

#include "cl_collision.h"

static void CL_UpdatePrydonCursor(void)
{
	vec3_t temp;

	if (cl_prydoncursor.integer <= 0)
		VectorClear(cl.mcmd.cursor_screen);

	/*
	if (cl.mcmd.cursor_screen[0] < -1)
	{
		cl.viewangles[YAW] -= m_yaw.value * (cl.mcmd.cursor_screen[0] - -1) * vid.width * sensitivity.value * cl.viewzoom;
		cl.mcmd.cursor_screen[0] = -1;
	}
	if (cl.mcmd.cursor_screen[0] > 1)
	{
		cl.viewangles[YAW] -= m_yaw.value * (cl.mcmd.cursor_screen[0] - 1) * vid.width * sensitivity.value * cl.viewzoom;
		cl.mcmd.cursor_screen[0] = 1;
	}
	if (cl.mcmd.cursor_screen[1] < -1)
	{
		cl.viewangles[PITCH] += m_pitch.value * (cl.mcmd.cursor_screen[1] - -1) * vid.height * sensitivity.value * cl.viewzoom;
		cl.mcmd.cursor_screen[1] = -1;
	}
	if (cl.mcmd.cursor_screen[1] > 1)
	{
		cl.viewangles[PITCH] += m_pitch.value * (cl.mcmd.cursor_screen[1] - 1) * vid.height * sensitivity.value * cl.viewzoom;
		cl.mcmd.cursor_screen[1] = 1;
	}
	*/
	cl.mcmd.cursor_screen[0] = bound(-1, cl.mcmd.cursor_screen[0], 1);
	cl.mcmd.cursor_screen[1] = bound(-1, cl.mcmd.cursor_screen[1], 1);
	cl.mcmd.cursor_screen[2] = 1;

	// calculate current view matrix
	Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, cl.mcmd.cursor_start);
	// calculate direction vector of cursor in viewspace by using frustum slopes
	VectorSet(temp, cl.mcmd.cursor_screen[2] * 1000000, (v_flipped.integer ? -1 : 1) * cl.mcmd.cursor_screen[0] * -r_refdef.view.frustum_x * 1000000, cl.mcmd.cursor_screen[1] * -r_refdef.view.frustum_y * 1000000);
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, cl.mcmd.cursor_end);
	// trace from view origin to the cursor
	if (cl_prydoncursor_notrace.integer /*zircon default 1*/ ) {
		cl.mcmd.cursor_fraction = 1.0f;
		VectorCopy(cl.mcmd.cursor_end, cl.mcmd.cursor_impact);
		VectorClear(cl.mcmd.cursor_normal);
		cl.mcmd.cursor_entitynumber = 0;
	}
	else {
		cl.mcmd.cursor_fraction = CL_SelectTraceLine(cl.mcmd.cursor_start, 
			cl.mcmd.cursor_end, 
			cl.mcmd.cursor_impact, // vec3
			cl.mcmd.cursor_normal, 
			&cl.mcmd.cursor_entitynumber, 
			// ignore ent ...
			(chase_active.integer || cl.intermission) ? &cl.entities[cl.playerentity].render : NULL);
		//Con_PrintLinef ("Prydon : %d", cl.mcmd.cursor_entitynumber);
	}
}

#define NUMOFFSETS_27 27
static vec3_t offsets[NUMOFFSETS_27] =
{
// 1 no nudge (just return the original if this test passes)
	{ 0.000,  0.000,  0.000},
// 6 simple nudges
	{ 0.000,  0.000,  0.125}, { 0.000,  0.000, -0.125},
	{-0.125,  0.000,  0.000}, { 0.125,  0.000,  0.000},
	{ 0.000, -0.125,  0.000}, { 0.000,  0.125,  0.000},
// 4 diagonal flat nudges
	{-0.125, -0.125,  0.000}, { 0.125, -0.125,  0.000},
	{-0.125,  0.125,  0.000}, { 0.125,  0.125,  0.000},
// 8 diagonal upward nudges
	{-0.125,  0.000,  0.125}, { 0.125,  0.000,  0.125},
	{ 0.000, -0.125,  0.125}, { 0.000,  0.125,  0.125},
	{-0.125, -0.125,  0.125}, { 0.125, -0.125,  0.125},
	{-0.125,  0.125,  0.125}, { 0.125,  0.125,  0.125},
// 8 diagonal downward nudges
	{-0.125,  0.000, -0.125}, { 0.125,  0.000, -0.125},
	{ 0.000, -0.125, -0.125}, { 0.000,  0.125, -0.125},
	{-0.125, -0.125, -0.125}, { 0.125, -0.125, -0.125},
	{-0.125,  0.125, -0.125}, { 0.125,  0.125, -0.125},
};

WARP_X_CALLERS_ (CL_ClientMovement_UpdateStatus)
static qbool CL_ClientMovement_UpdateStatus_Unstick (cl_clientmovement_state_t *s, int collide_type)
{
	int i;
	vec3_t neworigin;
	for (i = 0;i < NUMOFFSETS_27;i++) {
		VectorAdd(offsets[i], s->origin, neworigin);
		if (!CL_TraceBox(neworigin, cl.playercrouchmins, cl.playercrouchmaxs, neworigin, 
			MOVE_NORMAL, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_PLAYERCLIP, 
			0, 0, collision_extendmovelength.value /*d: 16*/, true, 
			/*HITT_PLAYERS_1*/ collide_type, NULL, true).startsolid)
		{
			// Baker: Assume that CL_TraceBox returns NULL if move failed?
			// Baker: If on ground, this hits every frame and hits #0 "no nudge".
			//if (i > 0 && developer_movement.integer) {
			//	Con_PrintLinef ("Nudged player axis idx is %d", i);
			//}
			VectorCopy (neworigin, s->origin);
			return true;
		}
	} // for
	// if all offsets failed, give up
	return false;
}


#include "cl_input_pmove.c.h"

static void QW_MSG_WriteDeltaUsercmd(sizebuf_t *buf, usercmd_t *from, usercmd_t *to)
{
	int bits;

	bits = 0;
	if (to->viewangles[0] != from->viewangles[0])		bits |= QW_CM_ANGLE1;
	if (to->viewangles[1] != from->viewangles[1])		bits |= QW_CM_ANGLE2;
	if (to->viewangles[2] != from->viewangles[2])		bits |= QW_CM_ANGLE3;
	if (to->clx_forwardmove != from->clx_forwardmove)	bits |= QW_CM_FORWARD;
	if (to->clx_sidemove != from->clx_sidemove)			bits |= QW_CM_SIDE;
	if (to->clx_upmove != from->clx_upmove)				bits |= QW_CM_UP;
	if (to->buttons != from->buttons)					bits |= QW_CM_BUTTONS;
	if (to->impulse != from->impulse)					bits |= QW_CM_IMPULSE;

	MSG_WriteByte(buf, bits);
	if (bits & QW_CM_ANGLE1)		MSG_WriteAngle16i(buf, to->viewangles[0]);
	if (bits & QW_CM_ANGLE2)		MSG_WriteAngle16i(buf, to->viewangles[1]);
	if (bits & QW_CM_ANGLE3)		MSG_WriteAngle16i(buf, to->viewangles[2]);
	if (bits & QW_CM_FORWARD)		MSG_WriteShort(buf, (short) to->clx_forwardmove);
	if (bits & QW_CM_SIDE)			MSG_WriteShort(buf, (short) to->clx_sidemove);
	if (bits & QW_CM_UP)			MSG_WriteShort(buf, (short) to->clx_upmove);
	if (bits & QW_CM_BUTTONS)		MSG_WriteByte(buf, to->buttons);
	if (bits & QW_CM_IMPULSE)		MSG_WriteByte(buf, to->impulse);
	MSG_WriteByte(buf, to->clx_msec);
}

void CL_NewFrameReceived(int num)
{
	if (developer_networkentities.integer >= 10)
		Con_Printf ("recv: svc_entities %d\n", num);
	cl.latestframenums[cl.latestframenumsposition] = num;
	cl.latestsendnums[cl.latestframenumsposition] = cl.mcmd.clx_sequence;
	cl.latestframenumsposition = (cl.latestframenumsposition + 1) % LATESTFRAMENUMS_32;
}

// Baker: VM_CL_RotateMoves "Obscure builtin used by GAME_XONOTIC"
void CL_RotateMoves(const matrix4x4_t *m)
{
	// rotate viewangles in all previous moves
	vec3_t v;
	vec3_t f, r, u;
	
	for (int i = 0; i < CL_MAX_USERCMDS_128; i ++) {
		if (cl.movecmd[i].clx_sequence > cls.servermovesequence) {
			usercmd_t *c = &cl.movecmd[i];
			AngleVectors(c->viewangles, f, r, u);
			Matrix4x4_Transform(m, f, v); VectorCopy(v, f);
			Matrix4x4_Transform(m, u, v); VectorCopy(v, u);
			AnglesFromVectors(c->viewangles, f, u, false);
		}
	} // for
}

static qbool CL_SendClientCommand (int is_reliable, int clc_code, char *msg)
{
	if (cls.demoplayback || cls.state == ca_disconnected) {
		return false;	// no point.
	}

	if (developer_qw.integer)
		Con_PrintLinef ("CL -> SV: %s", msg);

	if (is_reliable) {
		MSG_WriteByte	(&cls.netcon->message, clc_code);
		MSG_WriteString	(&cls.netcon->message, msg);
	}
	else {
		if (!cls.netcon->cmdmsg.maxsize) {
			sizebuf_t *sb		= &cls.netcon->cmdmsg;
			sb->data			= cls.netcon->cmdmsg_data;
			sb->maxsize			= sizeof(cls.netcon->cmdmsg_data);
			sb->allowoverflow	= true;
		}

		WARP_X_ (NetConn_SendUnreliableMessage)
		int size_needed = 1 + strlen(msg) + 1;
		sizebuf_t *sb = &cls.netcon->cmdmsg;
		if (sb->cursize + size_needed <= sb->maxsize) {
			MSG_WriteByte	(&cls.netcon->cmdmsg, clc_code);
			MSG_WriteString	(&cls.netcon->cmdmsg, msg);
			
		} else {
			return false;
		}
	} // if -- unreliable segment
	return true;
}

// Equivalent to ezQuake QW_CL_SendClientCommandf
// Use ONLY as that direct equivalent
qbool DP_CL_SendClientCommandf (int is_reliable, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[2048];

	if (cls.demoplayback || cls.state == ca_disconnected) {
		return false;	// no point.
	}

	va_start		(argptr, fmt);
	dpvsnprintf		(msg, sizeof(msg), fmt, argptr);
	va_end			(argptr);

	return CL_SendClientCommand (is_reliable, clc_stringcmd, msg);
}

qbool QW_CL_SendClientCommandf (int is_reliable, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[2048];

	if (cls.demoplayback || cls.state == ca_disconnected) {
		return false;	// no point.
	}

	va_start		(argptr, fmt);
	dpvsnprintf		(msg, sizeof(msg), fmt, argptr);
	va_end			(argptr);

	return CL_SendClientCommand (is_reliable, qw_clc_stringcmd, msg);
}



/*
==============
CL_SendMove
==============
*/
usercmd_t nullcmd; // for delta compression of qw moves

WARP_X_ (SV_ReadClientMessage SV_ReadClientMessage_ReadClientMove)
WARP_X_ () // Baker: Where is read from server?
void CL_SendMove(void)
{
	sizebuf_t buf;
	unsigned char send_data_buf_size_1024[1024];

	// if playing a demo, do nothing
	if (!cls.netcon)
		return;

	// ezQuake 1 CL_SendCmd
	QW_CL_SendChunkDownloadReq();

	// we don't que moves during a lag spike (potential network timeout)
	qbool quemove = host.realtime - cl.last_received_message < cl_movement_nettimeout.value;

	// we build up cl.mcmd and then decide whether to send or not
	// we store this into cl.movecmd[0] for prediction each frame even if we
	// do not send, to make sure that prediction is instant
	cl.mcmd.clx_time = cl.time;
	cl.mcmd.clx_sequence = cls.netcon->outgoing_unreliable_sequence;

	// set button button_bits
	// LadyHavoc: added 6 new buttons and use and chat buttons, and prydon cursor active button
	int button_bits = 0;
	if (in_attack.state   & 3) button_bits |=   1;
	if (in_jump.state     & 3) button_bits |=   2;
	if (in_button3.state  & 3) button_bits |=   4;
	if (in_button4.state  & 3) button_bits |=   8;
	if (in_button5.state  & 3) button_bits |=  16;
	if (in_button6.state  & 3) button_bits |=  32;
	if (in_button7.state  & 3) button_bits |=  64;
	if (in_button8.state  & 3) button_bits |= 128;
	if (in_use.state      & 3) button_bits |= 256;
	if (key_dest != key_game || key_consoleactive) button_bits |= 512;
	if (cl_prydoncursor.integer > 0) button_bits |= 1024;
	if (in_button9.state  & 3)  button_bits |=   2048;
	if (in_button10.state  & 3) button_bits |=   4096;
	if (in_button11.state  & 3) button_bits |=   8192;
	if (in_button12.state  & 3) button_bits |=  16384;
	if (in_button13.state  & 3) button_bits |=  32768;
	if (in_button14.state  & 3) button_bits |=  65536;
	if (in_button15.state  & 3) button_bits |= 131072;
	if (in_button16.state  & 3) button_bits |= 262144;
	// button button_bits 19-31 unused currently
	// rotate/zoom view serverside if PRYDON_CLIENTCURSOR cursor is at edge of screen
	if (cl_prydoncursor.integer /*d: 0*/ > 0) {
		if (cl.mcmd.cursor_screen[0] <= -1) button_bits |= 8;
		if (cl.mcmd.cursor_screen[0] >=  1) button_bits |= 16;
		if (cl.mcmd.cursor_screen[1] <= -1) button_bits |= 32;
		if (cl.mcmd.cursor_screen[1] >=  1) button_bits |= 64;
	}

	// set buttons and impulse
	cl.mcmd.buttons = button_bits;
	cl.mcmd.impulse = in_impulse;

	// set viewangles
	VectorCopy (cl.viewangles, cl.mcmd.viewangles);

#if 0 // PHYSICAL
	WARP_X_ (ZIRCON_PEXT ZIRCON_EXT_CHUNKED_2)
	if (cls.protocol == PROTOCOL_DARKPLACES8) {
		// set viewangles
		vec3_t pmove_org;
		Matrix4x4_OriginFromMatrix(&cl.entities[cl.viewentity].render.matrix, pmove_org);
		VectorCopy(pmove_org, cl.mcmd.player_org);

	}
#endif

	// bones_was_here: previously cl.mcmd.frametime was floored to nearest millisec
	// this meant the smoothest async movement required integer millisec
	// client and server frame times (eg 125fps)
	cl.mcmd.clx_frametime = bound(0.0, cl.mcmd.clx_time - cl.movecmd[1].clx_time, 0.255);
	// ridiculous value rejection (matches qw)
	if (cl.mcmd.clx_frametime > 0.25)
		cl.mcmd.clx_frametime = 0.1;
	cl.mcmd.clx_msec = (unsigned char)floor(cl.mcmd.clx_frametime * 1000);

	switch(cls.protocol) {
	case PROTOCOL_QUAKEWORLD:
		// quakeworld uses a different cvar with opposite meaning, for compatibility
		cl.mcmd.clx_is_predicted = cl_nopred.integer == 0;
		break;
	case PROTOCOL_DARKPLACES6:
	case PROTOCOL_DARKPLACES7:
	case PROTOCOL_DARKPLACES8:
		 // Baker: (cl_movement.integer && !sv_active) 
		// We are defaulting cl_movement 1, but want to disable in single player
		cl.mcmd.clx_is_predicted = QW_TREAT_AS_CL_MOVEMENT;
		break;
	default:
		cl.mcmd.clx_is_predicted = false;
		break;
	} // protocol

	// movement is set by input code (forwardmove/sidemove/upmove)
	// always dump the first two moves, because they may contain leftover inputs from the last level
	if (cl.mcmd.clx_sequence <= 2)
		cl.mcmd.clx_forwardmove = cl.mcmd.clx_sidemove = cl.mcmd.clx_upmove = cl.mcmd.impulse = cl.mcmd.buttons = 0;

	cl.mcmd.clx_jump = (cl.mcmd.buttons & 2) != 0;
	cl.mcmd.clx_crouch = 0;
	switch (cls.protocol) {
	case PROTOCOL_FITZQUAKE666:
	case PROTOCOL_FITZQUAKE999:
	case PROTOCOL_QUAKEWORLD:
	case PROTOCOL_QUAKE:
	case PROTOCOL_QUAKEDP:
	case PROTOCOL_NEHAHRAMOVIE:
	case PROTOCOL_NEHAHRABJP:
	case PROTOCOL_NEHAHRABJP2:
	case PROTOCOL_NEHAHRABJP3:
	case PROTOCOL_DARKPLACES1:
	case PROTOCOL_DARKPLACES2:
	case PROTOCOL_DARKPLACES3:
	case PROTOCOL_DARKPLACES4:
	case PROTOCOL_DARKPLACES5:
		break;
	case PROTOCOL_DARKPLACES6:
	case PROTOCOL_DARKPLACES7:
	case PROTOCOL_DARKPLACES8:
		// FIXME: cl.mcmd.buttons & 16 is +button5, Nexuiz/Xonotic specific
		cl.mcmd.clx_crouch = (cl.mcmd.buttons & 16) != 0;
		break;
	case PROTOCOL_UNKNOWN_0:
		break;
	} // sw protocol

	if (quemove)
		cl.movecmd[0] = cl.mcmd;

	// don't predict more than 200fps
	if (cl.timesincepacket >= 0.005)
		cl.movement_replay = true; // redo the prediction

	// now decide whether to actually send this move
	// (otherwise it is only for prediction)

	// do not send 0ms packets because they mess up physics
	if (cl.mcmd.clx_msec == 0 && cl.time > cl.oldtime && (cls.protocol == PROTOCOL_QUAKEWORLD
		|| cls.signon == SIGNONS_4)) {
		return;
	}

	// don't send too often or else network connections can get clogged by a
	// high renderer framerate
	float packettime = 1.0f / bound(10.0f, cl_netfps.value /*d: 72*/, 1000.0f);
	if (cl.movevars_timescale && cl.movevars_ticrate) {
		// try to ensure at least 1 packet per server frame
		// and apply soft limit of 2, hard limit < 4 (packettime reduced further below)
		float maxtic = cl.movevars_ticrate / cl.movevars_timescale;
		packettime = bound(maxtic * 0.5f, packettime, maxtic);
	}
	// bones_was_here: reduce packettime to (largest multiple of realframetime) <= packettime
	// prevents packet rates lower than cl_netfps or server frame rate
	// eg: cl_netfps 60 and cl_maxfps 250 would otherwise send only 50 netfps
	// with this line that config sends 62.5 netfps
	// (this causes it to emit packets at a steady beat)
	packettime = floor(packettime / (float)cl.realframetime) * (float)cl.realframetime;

	// always send if buttons changed or an impulse is pending
	// even if it violates the rate limit!
	// Baker: cl_netimmediatebuttons defaults 1.
	qbool important = (cl.mcmd.impulse || (cl_netimmediatebuttons.integer /*1*/ && cl.mcmd.buttons != cl.movecmd[1].buttons));

	// don't send too often (cl_netfps), allowing a small margin for float error
	// bones_was_here: accumulate realframetime to prevent low packet rates
	// previously with cl_maxfps == cl_netfps it did not send every frame as
	// host.realtime - cl.lastpackettime was often well below (or above) packettime
	if (!important && cl.timesincepacket < packettime * 0.99999f) {
		cl.timesincepacket += cl.realframetime;
		return;
	}

	// don't choke the connection with packets (obey rate limit)
	// it is important that this check be last, because it adds a new
	// frame to the shownetgraph output and any cancelation after this
	// will produce a nasty spike-like look to the netgraph
	// we also still send if it is important
	if (!NetConn_CanSend(cls.netcon) && !important)
		return;

	// reset the packet timing accumulator
	cl.timesincepacket = cl.realframetime;

	buf.maxsize = sizeof(send_data_buf_size_1024);
	buf.cursize = 0;
	buf.data = send_data_buf_size_1024;

#if 1
	if (/* cls.protocol == PROTOCOL_QUAKEWORLD &&*/ cls.netcon->cmdmsg.cursize) {
		//SZ_Init(&buf, data, sizeof(data));

		//SZ_Write(&buf, cls.cmdmsg.data, cls.cmdmsg.cursize);


		//SZ_Clear(&cls.cmdmsg);
		SZ_Write (&buf, cls.netcon->cmdmsg.data, cls.netcon->cmdmsg.cursize);
		if (cls.netcon->cmdmsg.overflowed) {
			Con_PrintLinef ("cls.cmdmsg overflowed");
		}
		SZ_Clear(&cls.netcon->cmdmsg);
	}
#endif

	// send the movement message
	// PROTOCOL_QUAKE        clc_move = 16 bytes total
	// PROTOCOL_QUAKEDP      clc_move = 16 bytes total
	// PROTOCOL_NEHAHRAMOVIE clc_move = 16 bytes total
	// PROTOCOL_DARKPLACES1  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES2  clc_move = 25 bytes total
	// PROTOCOL_DARKPLACES3  clc_move = 25 bytes total
	// PROTOCOL_DARKPLACES4  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES5  clc_move = 19 bytes total
	// PROTOCOL_DARKPLACES6  clc_move = 52 bytes total
	// PROTOCOL_DARKPLACES7  clc_move = 56 bytes total per move (can be up to 16 moves)
	// PROTOCOL_DARKPLACES8  clc_move = 56 bytes total per move (can be up to 16 moves)
	// PROTOCOL_QUAKEWORLD   clc_move = 34 bytes total (typically, but can reach 43 bytes, or even 49 bytes with roll)

	// set prydon cursor info
	CL_UpdatePrydonCursor();

	if (cls.protocol == PROTOCOL_QUAKEWORLD || cls.signon == SIGNONS_4) {
		switch (cls.protocol) {
		case PROTOCOL_QUAKEWORLD:
			{
			MSG_WriteByte(&buf, qw_clc_move);
			// save the position for a checksum byte
			int qw_checksumindex = buf.cursize;
			MSG_WriteByte(&buf, 0);
			// packet loss percentage
			int qw_packetloss = 0;
			for (int qw_j = 0; qw_j < NETGRAPH_PACKETS_256; qw_j ++)
				if (cls.netcon->incoming_netgraph[qw_j].unreliablebytes == NETGRAPH_LOSTPACKET_NEG1)
					qw_packetloss++;
			qw_packetloss = qw_packetloss * 100 / NETGRAPH_PACKETS_256;
			MSG_WriteByte(&buf, qw_packetloss);
			// write most recent 3 moves
			QW_MSG_WriteDeltaUsercmd (&buf, &nullcmd, &cl.movecmd[2]);
			QW_MSG_WriteDeltaUsercmd (&buf, &cl.movecmd[2], &cl.movecmd[1]);
			QW_MSG_WriteDeltaUsercmd (&buf, &cl.movecmd[1], &cl.mcmd);
			// calculate the checksum
			buf.data[qw_checksumindex] = COM_BlockSequenceCRCByteQW (buf.data + qw_checksumindex + 1, buf.cursize - qw_checksumindex - 1, cls.netcon->outgoing_unreliable_sequence);
			// if delta compression history overflows, request no delta
			if (cls.netcon->outgoing_unreliable_sequence - cl.qw_validsequence >= QW_UPDATE_BACKUP_64-1)
				cl.qw_validsequence = 0;
			// request delta compression if appropriate
			if (cl.qw_validsequence && !cl_nodelta.integer && cls.state == ca_connected && !cls.demorecording) {
				cl.qw_deltasequence[cls.netcon->outgoing_unreliable_sequence & QW_UPDATE_MASK_63] = cl.qw_validsequence;
				MSG_WriteByte(&buf, qw_clc_delta);
				MSG_WriteByte(&buf, cl.qw_validsequence & 255);
			}
			else
				cl.qw_deltasequence[cls.netcon->outgoing_unreliable_sequence & QW_UPDATE_MASK_63] = -1;
			break;
			} // PROTOCOL_QUAKEWORLD

		case PROTOCOL_QUAKE:
		case PROTOCOL_QUAKEDP:
		case PROTOCOL_NEHAHRAMOVIE:
		case PROTOCOL_NEHAHRABJP:
		case PROTOCOL_NEHAHRABJP2:
		case PROTOCOL_NEHAHRABJP3:
		case PROTOCOL_FITZQUAKE666:
		case PROTOCOL_FITZQUAKE999:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.mcmd.clx_time); // last server packet time
			// 3 bytes (6 bytes in proquake)
			if (cls.proquake_servermod == 1) { // MOD_PROQUAKE
				for (int i = 0;i < 3; i ++)
					MSG_WriteAngle16i (&buf, cl.mcmd.viewangles[i]);
			} else if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999) ) {
				for (int i = 0;i < 3;i++)
					MSG_WriteAngle16i (&buf, cl.mcmd.viewangles[i]); // Baker DPD 999 same as ProQuake
			} else {
				// NetQuake
				for (int i = 0;i < 3;i++)
					MSG_WriteAngle8i (&buf, cl.mcmd.viewangles[i]);
			}
			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_forwardmove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_sidemove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.mcmd.buttons);
			MSG_WriteByte (&buf, cl.mcmd.impulse);
			break;
		case PROTOCOL_DARKPLACES2:
		case PROTOCOL_DARKPLACES3:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.mcmd.clx_time); // last server packet time
			// 12 bytes
			for (int i = 0; i < 3; i ++)
				MSG_WriteAngle32f (&buf, cl.mcmd.viewangles[i]);
			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_forwardmove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_sidemove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.mcmd.buttons);
			MSG_WriteByte (&buf, cl.mcmd.impulse);
			break;
		case PROTOCOL_DARKPLACES1:
		case PROTOCOL_DARKPLACES4:
		case PROTOCOL_DARKPLACES5:
			// 5 bytes
			MSG_WriteByte (&buf, clc_move);
			MSG_WriteFloat (&buf, cl.mcmd.clx_time); // last server packet time
			// 6 bytes
			for (int i = 0;i < 3;i++)
				MSG_WriteAngle16i (&buf, cl.mcmd.viewangles[i]);

			// 6 bytes
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_forwardmove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_sidemove);
			MSG_WriteCoord16i (&buf, cl.mcmd.clx_upmove);
			// 2 bytes
			MSG_WriteByte (&buf, cl.mcmd.buttons);
			MSG_WriteByte (&buf, cl.mcmd.impulse);
		case PROTOCOL_DARKPLACES6:
		case PROTOCOL_DARKPLACES7:
		case PROTOCOL_DARKPLACES8:
			{
			// set the maxusercmds variable to limit how many should be sent
			int maxusercmds = bound(1, cl_netrepeatinput.integer + 1, min(3, CL_MAX_USERCMDS_128));
			// when movement prediction is off, there's not much point in repeating old input as it will just be ignored
			if (!cl.mcmd.clx_is_predicted)
				maxusercmds = 1;

			// send the latest moves in order, the old ones will be
			// ignored by the server harmlessly, however if the previous
			// packets were lost these moves will be used
			//
			// this reduces packet loss impact on gameplay.
			
			usercmd_t *mseq_cmd;
			int mj;
			for (mj = 0, mseq_cmd = &cl.movecmd[maxusercmds - 1]; mj < maxusercmds; mj++, mseq_cmd --) {
				// don't repeat any stale moves
				if (mseq_cmd->clx_sequence && mseq_cmd->clx_sequence < cls.servermovesequence)
					continue;
				// 5/9 bytes
				
				MSG_WriteByte (&buf, clc_move); // clc_zircon_movestringcmd KLESKBY 1 WHERE IS LOCATION READ?
				if (cls.protocol != PROTOCOL_DARKPLACES6) {
					int is_zircon_move =
						mseq_cmd->clx_is_predicted && 
						!cl.intermission && 
						ZMOVE_IS_ENABLED; // PRED_ZIRCON_MOVE_2 //ZMOVE_WARP

					if (is_zircon_move) {
						
						WARP_X_ (CL_ClientMovement_Replay)
						if (mseq_cmd->zmove_is_move_processed && mseq_cmd->zmove_is_move_processed == mseq_cmd->clx_sequence) {
							buf.data[buf.cursize - ONE_CHAR_1] = clc_zircon_move; // Upgrade clc_move to clc_zircon_move 
							MSG_WriteFloat (&buf, mseq_cmd->zmove_end_origin[0]);
							MSG_WriteFloat (&buf, mseq_cmd->zmove_end_origin[1]);
							MSG_WriteFloat (&buf, mseq_cmd->zmove_end_origin[2]);
							MSG_WriteByte  (&buf, mseq_cmd->zmove_end_onground); // zircon_move_final_ground
// NORM						Con_PrintLinef ("%d: Upgrade to ZMOVE mj = %d", (int)mseq_cmd->clx_sequence, mj);
						} else if (cl.movement_final_origin_set) {
							buf.data[buf.cursize - ONE_CHAR_1] = clc_zircon_move; // Upgrade clc_move to clc_zircon_move 
							MSG_WriteFloat (&buf, cl.movement_final_origin[0]);
							MSG_WriteFloat (&buf, cl.movement_final_origin[1]);
							MSG_WriteFloat (&buf, cl.movement_final_origin[2]);
							MSG_WriteByte (&buf, cl.zircon_replay_save.zmove_end_onground);
							
// NORM						Con_PrintLinef ("%d: New move is unprocessed, using final = %d", (int)mseq_cmd->clx_sequence, mj);
						}
						else {
							if (developer_movement.integer  > 1)
								Con_PrintLinef ("%d: Couldn't origin unprocessed move because not stored mj = %d", (int)mseq_cmd->clx_sequence, mj);
						}
					}
					MSG_WriteLong (&buf, mseq_cmd->clx_is_predicted ? mseq_cmd->clx_sequence : 0);
				}
				MSG_WriteFloat (&buf, mseq_cmd->clx_time); // last server packet time
				// 6 bytes
				for (int ang_j = 0; ang_j < 3; ang_j ++)
					MSG_WriteAngle16i (&buf, mseq_cmd->viewangles[ang_j]);

#if 0 // PHYSICAL
				if (cls.protocol == PROTOCOL_DARKPLACES8) {
					for (i = 0;i < 3;i++)
						MSG_WriteAngle16i (&buf, cl.mcmd.player_org[i]);


				}
#endif
				// 6 bytes
				MSG_WriteCoord16i (&buf, mseq_cmd->clx_forwardmove);
				MSG_WriteCoord16i (&buf, mseq_cmd->clx_sidemove);
				MSG_WriteCoord16i (&buf, mseq_cmd->clx_upmove);
				// 5 bytes
				MSG_WriteLong (&buf, mseq_cmd->buttons);
				MSG_WriteByte (&buf, mseq_cmd->impulse);
				// PRYDON_CLIENTCURSOR
				// 30 bytes
				MSG_WriteShort (&buf, (short)(mseq_cmd->cursor_screen[0] * 32767.0f));
				MSG_WriteShort (&buf, (short)(mseq_cmd->cursor_screen[1] * 32767.0f));
				MSG_WriteFloat (&buf, mseq_cmd->cursor_start[0]); // KLESKBY 1: COPY ORIGIN HERE
				MSG_WriteFloat (&buf, mseq_cmd->cursor_start[1]);
				MSG_WriteFloat (&buf, mseq_cmd->cursor_start[2]);
				MSG_WriteFloat (&buf, mseq_cmd->cursor_impact[0]);
				MSG_WriteFloat (&buf, mseq_cmd->cursor_impact[1]);
				MSG_WriteFloat (&buf, mseq_cmd->cursor_impact[2]);
				MSG_WriteShort (&buf, mseq_cmd->cursor_entitynumber); // KLESKBY 1: COPY ORIGIN HERE
			} // FOR MOVE CMD
			break;
			} // DP7
		case PROTOCOL_UNKNOWN_0:
			break;
		}
	}

	if (cl.zircon_warp_sequence) {
		MSG_WriteByte (&buf, clc_zircon_warp_ack);
		MSG_WriteLong (&buf, cl.zircon_warp_sequence);
		if (Have_Flag (developer_movement.integer, /*CL*/ 1))
			Con_PrintLinef ("CL: clc_zircon_warp_ack %u acknowledge", cl.zircon_warp_sequence);
		cl.zircon_warp_sequence_clock = cl.time + 0.1; // Clock clears AFTER
	}

	if (cls.protocol != PROTOCOL_QUAKEWORLD && buf.cursize) {
		// ack entity frame numbers received since the last input was sent
		// (redundent to improve handling of client->server packet loss)
		// if cl_netrepeatinput is 1 and client framerate matches server
		// framerate, this is 10 bytes, if client framerate is lower this
		// will be more...
		unsigned int oldsequence = cl.mcmd.clx_sequence;
		unsigned int delta = bound(1, cl_netrepeatinput.integer /*d:1*/ + 1, 3);
		if (oldsequence > delta)
			oldsequence = oldsequence - delta;
		else
			oldsequence = 1;
		for (int framenum = 0; framenum < LATESTFRAMENUMS_32; framenum ++) {
			int frame_j = (cl.latestframenumsposition + framenum) % LATESTFRAMENUMS_32;
			if (cl.latestsendnums[frame_j] >= oldsequence) {
				if (developer_networkentities.integer >= 10)
					Con_PrintLinef ("send clc_ackframe %d", cl.latestframenums[frame_j]);
				MSG_WriteByte(&buf, clc_ackframe);
				MSG_WriteLong(&buf, cl.latestframenums[frame_j]);
			}
		} // for
	} // if cls.protocol != PROTOCOL_QUAKEWORLD && buf.cursize

	// PROTOCOL_DARKPLACES6 = 67 bytes per packet
	// PROTOCOL_DARKPLACES7 = 71 bytes per packet

	// acknowledge any recently received data blocks
	// Baker: Server reads this with clc_ackdownloaddata
	for (int dp_download_ack_num = 0; dp_download_ack_num < CL_MAX_DOWNLOADACKS_DP_QW_4 && (cls.dp_downloadack[dp_download_ack_num].start || cls.dp_downloadack[dp_download_ack_num].size);dp_download_ack_num++) {
		MSG_WriteByte		(&buf, clc_ackdownloaddata);
		MSG_WriteLong		(&buf, cls.dp_downloadack[dp_download_ack_num].start);
		MSG_WriteShort		(&buf, cls.dp_downloadack[dp_download_ack_num].size);
		cls.dp_downloadack[dp_download_ack_num].start = 0;
		cls.dp_downloadack[dp_download_ack_num].size = 0;
	}

	// send the reliable message (forwarded commands) if there is one
	if (buf.cursize || cls.netcon->message.cursize)
		NetConn_SendUnreliableMessage(cls.netcon, &buf, cls.protocol, max(20*(buf.cursize+40), cl_rate.integer),
		cl_rate_burstsize.integer, false);

	if (quemove) {
		// update the cl.movecmd array which holds the most recent moves,
		// because we now need a new slot for the next input
		for (int user_cmd_j = CL_MAX_USERCMDS_128 - 1; user_cmd_j >= 1; user_cmd_j --)
			cl.movecmd[user_cmd_j] = cl.movecmd[user_cmd_j - 1]; // Baker: is this struct copy? Yes.
		cl.movecmd[0].clx_msec = 0;
		cl.movecmd[0].clx_frametime = 0;
	}

	// clear button 'click' states
	in_attack.state  &= ~2;
	in_jump.state    &= ~2;
	in_button3.state &= ~2;
	in_button4.state &= ~2;
	in_button5.state &= ~2;
	in_button6.state &= ~2;
	in_button7.state &= ~2;
	in_button8.state &= ~2;
	in_use.state     &= ~2;
	in_button9.state  &= ~2;
	in_button10.state &= ~2;
	in_button11.state &= ~2;
	in_button12.state &= ~2;
	in_button13.state &= ~2;
	in_button14.state &= ~2;
	in_button15.state &= ~2;
	in_button16.state &= ~2;
	// clear impulse
	in_impulse = 0;

	if (cls.netcon->message.overflowed)
		CL_DisconnectEx(q_is_kicked_true, "Lost connection to server");
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand(CF_CLIENT, "+moveup",IN_UpDown, "swim upward");
	Cmd_AddCommand(CF_CLIENT, "-moveup",IN_UpUp, "stop swimming upward");
	Cmd_AddCommand(CF_CLIENT, "+movedown",IN_DownDown, "swim downward");
	Cmd_AddCommand(CF_CLIENT, "-movedown",IN_DownUp, "stop swimming downward");
	Cmd_AddCommand(CF_CLIENT, "+left",IN_LeftDown, "turn left");
	Cmd_AddCommand(CF_CLIENT, "-left",IN_LeftUp, "stop turning left");
	Cmd_AddCommand(CF_CLIENT, "+right",IN_RightDown, "turn right");
	Cmd_AddCommand(CF_CLIENT, "-right",IN_RightUp, "stop turning right");
	Cmd_AddCommand(CF_CLIENT, "+forward",IN_ForwardDown, "move forward");
	Cmd_AddCommand(CF_CLIENT, "-forward",IN_ForwardUp, "stop moving forward");
	Cmd_AddCommand(CF_CLIENT, "+back",IN_BackDown, "move backward");
	Cmd_AddCommand(CF_CLIENT, "-back",IN_BackUp, "stop moving backward");
	Cmd_AddCommand(CF_CLIENT, "+lookup", IN_LookupDown, "look upward");
	Cmd_AddCommand(CF_CLIENT, "-lookup", IN_LookupUp, "stop looking upward");
	Cmd_AddCommand(CF_CLIENT, "+lookdown", IN_LookdownDown, "look downward");
	Cmd_AddCommand(CF_CLIENT, "-lookdown", IN_LookdownUp, "stop looking downward");
	Cmd_AddCommand(CF_CLIENT, "+strafe", IN_StrafeDown, "activate strafing mode (move instead of turn)");
	Cmd_AddCommand(CF_CLIENT, "-strafe", IN_StrafeUp, "deactivate strafing mode");
	Cmd_AddCommand(CF_CLIENT, "+moveleft", IN_MoveleftDown, "strafe left");
	Cmd_AddCommand(CF_CLIENT, "-moveleft", IN_MoveleftUp, "stop strafing left");
	Cmd_AddCommand(CF_CLIENT, "+moveright", IN_MoverightDown, "strafe right");
	Cmd_AddCommand(CF_CLIENT, "-moveright", IN_MoverightUp, "stop strafing right");
	Cmd_AddCommand(CF_CLIENT, "+speed", IN_SpeedDown, "activate run mode (faster movement and turning)");
	Cmd_AddCommand(CF_CLIENT, "-speed", IN_SpeedUp, "deactivate run mode");
	Cmd_AddCommand(CF_CLIENT, "+attack", IN_AttackDown, "begin firing");
	Cmd_AddCommand(CF_CLIENT, "-attack", IN_AttackUp, "stop firing");
	Cmd_AddCommand(CF_CLIENT, "+jump", IN_JumpDown, "jump");
	Cmd_AddCommand(CF_CLIENT, "-jump", IN_JumpUp, "end jump (so you can jump again)");
	Cmd_AddCommand(CF_CLIENT, "impulse", IN_Impulse, "send an impulse number to server (select weapon, use item, etc)");
	Cmd_AddCommand(CF_CLIENT, "+klook", IN_KLookDown, "activate keyboard looking mode, do not recenter view");
	Cmd_AddCommand(CF_CLIENT, "-klook", IN_KLookUp, "deactivate keyboard looking mode");
	Cmd_AddCommand(CF_CLIENT, "+mlook", IN_MLookDown, "activate mouse looking mode, do not recenter view");
	Cmd_AddCommand(CF_CLIENT, "-mlook", IN_MLookUp, "deactivate mouse looking mode");

	// LadyHavoc: added lots of buttons
	Cmd_AddCommand(CF_CLIENT, "+use", IN_UseDown, "use something (may be used by some mods)");
	Cmd_AddCommand(CF_CLIENT, "-use", IN_UseUp, "stop using something");
	Cmd_AddCommand(CF_CLIENT, "+button3", IN_Button3Down, "activate button3 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button3", IN_Button3Up, "deactivate button3");
	Cmd_AddCommand(CF_CLIENT, "+button4", IN_Button4Down, "activate button4 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button4", IN_Button4Up, "deactivate button4");
	Cmd_AddCommand(CF_CLIENT, "+button5", IN_Button5Down, "activate button5 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button5", IN_Button5Up, "deactivate button5");
	Cmd_AddCommand(CF_CLIENT, "+button6", IN_Button6Down, "activate button6 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button6", IN_Button6Up, "deactivate button6");
	Cmd_AddCommand(CF_CLIENT, "+button7", IN_Button7Down, "activate button7 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button7", IN_Button7Up, "deactivate button7");
	Cmd_AddCommand(CF_CLIENT, "+button8", IN_Button8Down, "activate button8 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button8", IN_Button8Up, "deactivate button8");
	Cmd_AddCommand(CF_CLIENT, "+button9", IN_Button9Down, "activate button9 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button9", IN_Button9Up, "deactivate button9");
	Cmd_AddCommand(CF_CLIENT, "+button10", IN_Button10Down, "activate button10 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button10", IN_Button10Up, "deactivate button10");
	Cmd_AddCommand(CF_CLIENT, "+button11", IN_Button11Down, "activate button11 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button11", IN_Button11Up, "deactivate button11");
	Cmd_AddCommand(CF_CLIENT, "+button12", IN_Button12Down, "activate button12 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button12", IN_Button12Up, "deactivate button12");
	Cmd_AddCommand(CF_CLIENT, "+button13", IN_Button13Down, "activate button13 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button13", IN_Button13Up, "deactivate button13");
	Cmd_AddCommand(CF_CLIENT, "+button14", IN_Button14Down, "activate button14 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button14", IN_Button14Up, "deactivate button14");
	Cmd_AddCommand(CF_CLIENT, "+button15", IN_Button15Down, "activate button15 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button15", IN_Button15Up, "deactivate button15");
	Cmd_AddCommand(CF_CLIENT, "+button16", IN_Button16Down, "activate button16 (behavior depends on mod)");
	Cmd_AddCommand(CF_CLIENT, "-button16", IN_Button16Up, "deactivate button16");

	// LadyHavoc: added bestweapon command
	Cmd_AddCommand(CF_CLIENT, "bestweapon", IN_BestWeapon_f, "send an impulse number to server to select the first usable weapon out of several (example: 8 7 6 5 4 3 2 1)");
	Cmd_AddCommand(CF_CLIENT, "register_bestweapon", IN_BestWeapon_Register_f, "(for QC usage only) change weapon parameters to be used by bestweapon; stuffcmd this in ClientConnect");

	

	Cvar_RegisterVariable(&cl_movecliptokeyboard);
	Cvar_RegisterVariable(&cl_movement);
	Cvar_RegisterVariable(&cl_zircon_move);
	Cvar_RegisterVariable(&cl_movement_collide_nonsolid_red);
	
	Cvar_RegisterVariable(&cl_movement_collide_models);
	
	Cvar_RegisterVariable(&cl_movement_replay);
	Cvar_RegisterVariable(&cl_movement_nettimeout);
	Cvar_RegisterVariable(&cl_movement_minping);
	Cvar_RegisterVariable(&cl_movement_track_canjump);
	Cvar_RegisterVariable(&cl_movement_maxspeed);
	Cvar_RegisterVariable(&cl_movement_maxairspeed);
	Cvar_RegisterVariable(&cl_movement_stopspeed);
	Cvar_RegisterVariable(&cl_movement_friction);
	Cvar_RegisterVariable(&cl_movement_wallfriction);
	Cvar_RegisterVariable(&cl_movement_waterfriction);
	Cvar_RegisterVariable(&cl_movement_edgefriction);
	Cvar_RegisterVariable(&cl_movement_stepheight);
	Cvar_RegisterVariable(&cl_movement_accelerate);
	Cvar_RegisterVariable(&cl_movement_airaccelerate);
	Cvar_RegisterVariable(&cl_movement_wateraccelerate);
	Cvar_RegisterVariable(&cl_movement_jumpvelocity);
	Cvar_RegisterVariable(&cl_movement_airaccel_qw);
	Cvar_RegisterVariable(&cl_movement_airaccel_sideways_friction);
	Cvar_RegisterVariable(&cl_nopred);

	Cvar_RegisterVariable(&in_pitch_min);
	Cvar_RegisterVariable(&in_pitch_max);
	Cvar_RegisterVariable(&m_filter);
	Cvar_RegisterVariable(&m_accelerate);
	Cvar_RegisterVariable(&m_accelerate_minspeed);
	Cvar_RegisterVariable(&m_accelerate_maxspeed);
	Cvar_RegisterVariable(&m_accelerate_filter);
	Cvar_RegisterVariable(&m_accelerate_power);
	Cvar_RegisterVariable(&m_accelerate_power_offset);
	Cvar_RegisterVariable(&m_accelerate_power_senscap);
	Cvar_RegisterVariable(&m_accelerate_power_strength);
	Cvar_RegisterVariable(&m_accelerate_natural_offset);
	Cvar_RegisterVariable(&m_accelerate_natural_accelsenscap);
	Cvar_RegisterVariable(&m_accelerate_natural_strength);

	Cvar_RegisterVariable(&cl_netfps);
	Cvar_RegisterVariable(&cl_netrepeatinput);
	Cvar_RegisterVariable(&cl_netimmediatebuttons);

	Cvar_RegisterVariable(&cl_nodelta);

	Cvar_RegisterVariable(&cl_csqc_generatemousemoveevents);
}

