// cl_input_pmove.c.h

WARP_X_CALLERS_ (CL_ClientMovement_Physics_Swim, CL_ClientMovement_Physics_Walk x 2 )

WARP_X_CALLERS_ (CL_ClientMovement_Move plus 3 others)
// Updates client movement state checking crouch, ground, water level
static void CL_ClientMovement_UpdateStatus (cl_clientmovement_state_t *s, int collide_type)
{
	vec_t f;
	vec3_t origin1, origin2;
	trace_t trace;

	// make sure player is not stuck
	CL_ClientMovement_UpdateStatus_Unstick (s, collide_type);

	// set crouched
	if (s->cmd.clx_crouch) {
		// wants to crouch, this always works..
		if (!s->crouched)
			s->crouched = true;
	}
	else
	{
		// wants to stand, if currently crouching we need to check for a
		// low ceiling first
		if (s->crouched) {
			trace = CL_TraceBox (s->origin, cl.playerstandmins, cl.playerstandmaxs, s->origin,
				MOVE_NORMAL_0, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP,
				0, 0, collision_extendmovelength.value, q_hitbrush_true,
				/*HITT_PLAYERS_1*/ collide_type, q_hitnetwork_ent_NULL, q_hitcsqcents_true);
			if (!trace.startsolid)
				s->crouched = false;
		}
	}
	if (s->crouched) {
		VectorCopy(cl.playercrouchmins, s->mdl_mins);
		VectorCopy(cl.playercrouchmaxs, s->mdl_maxs);
	} else {
		VectorCopy(cl.playerstandmins, s->mdl_mins);
		VectorCopy(cl.playerstandmaxs, s->mdl_maxs);
	}

	// set onground
	VectorSet (origin1, s->origin[0], s->origin[1], s->origin[2] + 1);
	VectorSet (origin2, s->origin[0], s->origin[1], s->origin[2] - 1); // -2 causes clientside doublejump bug at above 150fps, raising that to 300fps :)

	int icollide;
	trace = CL_TraceBox(origin1, s->mdl_mins, s->mdl_maxs, origin2, MOVE_NORMAL_0, s->self,
		SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0,
		collision_extendmovelength.value, q_hitbrush_true,
		/*HITT_PLAYERS_1*/ collide_type, &icollide /*q_hitnetwork_ent_NULL*/, q_hitcsqcents_true);
	if (trace.fraction < 1 && trace.plane.normal[2] > 0.7) {
		SET___ s->onground = true; // Baker: trace down

		// this code actually "predicts" an impact; so let's clip velocity first
		f = DotProduct(s->velocity, trace.plane.normal);
		if (f < 0) // only if moving downwards actually
			VectorMA(s->velocity, -f, trace.plane.normal, s->velocity);
	}
	else
		SET___ s->onground = false; // Baker: trace.fraction == 1 on a slight downward trace depth of 1.

	// set watertype/waterlevel
	VectorSet(origin1, s->origin[0], s->origin[1], s->origin[2] + s->mdl_mins[2] + 1);
	s->waterlevel = WATERLEVEL_NONE_0;
	s->watertype = CL_TracePoint(origin1, MOVE_NOMONSTERS_1,
		s->self, 0, 0, 0, q_hitbrush_true, HITT_NOPLAYERS_0, q_hitnetwork_ent_NULL, q_hitcsqcents_false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK;

	if (s->watertype) {
		s->waterlevel = WATERLEVEL_WETFEET_1;
		origin1[2] = s->origin[2] + (s->mdl_mins[2] + s->mdl_maxs[2]) * 0.5f;
		if (CL_TracePoint(origin1, MOVE_NOMONSTERS_1, s->self, 0, 0, 0, q_hitbrush_true, HITT_NOPLAYERS_0, q_hitnetwork_ent_NULL, q_hitcsqcents_false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK) {
			s->waterlevel = WATERLEVEL_SWIMMING_2;
			origin1[2] = s->origin[2] + 22;
			if (CL_TracePoint(origin1, MOVE_NOMONSTERS_1, s->self, 0, 0, 0, q_hitbrush_true, HITT_NOPLAYERS_0, q_hitnetwork_ent_NULL, q_hitcsqcents_false).startsupercontents & SUPERCONTENTS_LIQUIDSMASK)
				s->waterlevel = WATERLEVEL_SUBMERGED_3;
		}
	}

	// water jump prediction
	if (s->onground || s->velocity[2] <= 0 || s->waterjumptime <= 0)
		s->waterjumptime = 0;
}

// Baker: Called by WALK and SWIM
// Baker: This looks like a box move check
static void CL_ClientMovement_Move (cl_clientmovement_state_t *s, int collide_type)
{
	int bump;
	double t;
	vec_t f;
	vec3_t neworigin;
	vec3_t currentorigin2;
	vec3_t neworigin2;
	vec3_t primalvelocity;
	trace_t trace;
	trace_t trace2;
	trace_t trace3;
	CL_ClientMovement_UpdateStatus (s, collide_type);
	VectorCopy(s->velocity, primalvelocity);

	for (bump = 0, t = s->cmd.clx_frametime; bump < 8 && VectorLength2(s->velocity) > 0;bump++) {
		VectorMA(s->origin, t, s->velocity, neworigin);
		int icollide = 0;
		trace = CL_TraceBox(s->origin, s->mdl_mins, s->mdl_maxs, neworigin, MOVE_NORMAL_0,
			s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP,
			0, 0, collision_extendmovelength.value, q_hitbrush_true, /*HITT_PLAYERS_1*/ collide_type,
			&icollide /*q_hitnetwork_ent_NULL*/, q_hitcsqcents_true);

		// Baker: Didn't make it the whole way, and hit wall?
		if (trace.fraction < 1 && trace.plane.normal[2] == 0) {
			// may be a step or wall, try stepping up
			// first move forward at a higher level
			VectorSet(currentorigin2, s->origin[0], s->origin[1], s->origin[2] + cl.movevars_stepheight);
			VectorSet(neworigin2, neworigin[0], neworigin[1], s->origin[2] + cl.movevars_stepheight);
			trace2 = CL_TraceBox(currentorigin2, s->mdl_mins, s->mdl_maxs, neworigin2, MOVE_NORMAL_0,
				s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP,
				SUPERCONTENTS_SKIP_NONE_0, MATERIALFLAG_NONE_0,
				collision_extendmovelength.value, q_hitbrush_true,
				/*HITT_PLAYERS_1*/ collide_type, q_hitnetwork_ent_NULL, q_hitcsqcents_true);

			if (!trace2.startsolid) {
				// then move down from there
				VectorCopy(trace2.endpos, currentorigin2);
				VectorSet(neworigin2, trace2.endpos[0], trace2.endpos[1], s->origin[2]);
				trace3 = CL_TraceBox(currentorigin2, s->mdl_mins, s->mdl_maxs, neworigin2, MOVE_NORMAL_0, s->self,
					SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0,
					collision_extendmovelength.value, q_hitbrush_true,
					/*HITT_PLAYERS_1*/ collide_type, q_hitnetwork_ent_NULL, q_hitcsqcents_true);
				//Con_Printf ("%f %f %f %f : %f %f %f %f : %f %f %f %f\n", trace.fraction, trace.endpos[0], trace.endpos[1], trace.endpos[2], trace2.fraction, trace2.endpos[0], trace2.endpos[1], trace2.endpos[2], trace3.fraction, trace3.endpos[0], trace3.endpos[1], trace3.endpos[2]);
				// accept the new trace if it made some progress
				if (fabs(trace3.endpos[0] - trace.endpos[0]) >= 0.03125 || fabs(trace3.endpos[1] - trace.endpos[1]) >= 0.03125)
				{
					trace = trace2;
					VectorCopy(trace3.endpos, trace.endpos);
				}
			}
		} // if hit wall or step

		// check if it moved at all
		if (trace.fraction >= 0.001)
			VectorCopy (trace.endpos, s->origin);

		// check if it moved all the way
		if (trace.fraction == 1)
			break;

		// this is only really needed for nogravityonground combined with gravityunaffectedbyticrate
		// <LadyHavoc> I'm pretty sure I commented it out solely because it seemed redundant
		// this got commented out in a change that supposedly makes the code match QW better
		// so if this is broken, maybe put it in an if (cls.protocol != PROTOCOL_QUAKEWORLD) block
		if (trace.plane.normal[2] > 0.7)
			SET___ s->onground = true; // Baker:

		t -= t * trace.fraction;

		f = DotProduct(s->velocity, trace.plane.normal);
		VectorMA(s->velocity, -f, trace.plane.normal, s->velocity);
	}
	if (s->waterjumptime > 0)
		VectorCopy(primalvelocity, s->velocity);
}


static void CL_ClientMovement_Physics_Swim (cl_clientmovement_state_t *s, int collide_type)
{
	vec_t wishspeed;
	vec_t f;
	vec3_t wishvel;
	vec3_t wishdir;

	// water jump only in certain situations
	// this mimics quakeworld code
	if (s->cmd.clx_jump && s->waterlevel == WATERLEVEL_SWIMMING_2 /*2*/ && s->velocity[2] >= -180) {
		vec3_t forward;
		vec3_t yawangles;
		vec3_t spot;
		VectorSet(yawangles, 0, s->cmd.viewangles[1], 0);
		AngleVectors(yawangles, forward, NULL, NULL);
		VectorMA(s->origin, 24, forward, spot);
		spot[2] += 8;
		if (CL_TracePoint(spot, MOVE_NOMONSTERS_1, s->self, 0, 0, 0, true, HITT_NOPLAYERS_0, NULL, false).startsolid)
		{
			spot[2] += 24;
			if (!CL_TracePoint(spot, MOVE_NOMONSTERS_1, s->self, 0, 0, 0, true, HITT_NOPLAYERS_0, NULL, false).startsolid) {
				VectorScale(forward, 50, s->velocity);
				s->velocity[2] = 310;
				s->waterjumptime = 2;
				SET___ s->onground = false; // Baker: CL_ClientMovement_Physics_Swim water jump
				s->cmd.clx_canjump = false;
			}
		}
	}

	if (!(s->cmd.clx_forwardmove*s->cmd.clx_forwardmove + s->cmd.clx_sidemove*s->cmd.clx_sidemove + s->cmd.clx_upmove * s->cmd.clx_upmove)) {
		// drift towards bottom
		VectorSet(wishvel, 0, 0, -60);
	}
	else
	{
		// swim
		vec3_t forward;
		vec3_t right;
		vec3_t up;
		// calculate movement vector
		AngleVectors(s->cmd.viewangles, forward, right, up);
		VectorSet(up, 0, 0, 1);
		VectorMAMAM(s->cmd.clx_forwardmove, forward, s->cmd.clx_sidemove, right, s->cmd.clx_upmove, up, wishvel);
	}

	// split wishvel into wishspeed and wishdir
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);
	wishspeed = min(wishspeed, cl.movevars_maxspeed) * 0.7;

	if (s->crouched)
		wishspeed *= 0.5;

	if (s->waterjumptime <= 0) {
		// water friction
		f = 1 - s->cmd.clx_frametime * cl.movevars_waterfriction * (cls.protocol == PROTOCOL_QUAKEWORLD ? s->waterlevel : WATERLEVEL_WETFEET_1 /*1*/);
		f = bound(0, f, 1);
		VectorScale(s->velocity, f, s->velocity);

		// water acceleration
		f = wishspeed - DotProduct(s->velocity, wishdir);
		if (f > 0)
		{
			f = min(cl.movevars_wateraccelerate * s->cmd.clx_frametime * wishspeed, f);
			VectorMA(s->velocity, f, wishdir, s->velocity);
		}

		// holding jump button swims upward slowly
		if (s->cmd.clx_jump)
		{
			if (s->watertype & SUPERCONTENTS_LAVA)
				s->velocity[2] =  50;
			else if (s->watertype & SUPERCONTENTS_SLIME)
				s->velocity[2] =  80;
			else
			{
				if (IS_NEXUIZ_DERIVED(gamemode))
					s->velocity[2] = 200;
				else
					s->velocity[2] = 100;
			}
		}
	}

	CL_ClientMovement_Move (s, collide_type);
}


static vec_t CL_IsMoveInDirection(vec_t forward, vec_t side, vec_t angle)
{
	if (forward == 0 && side == 0)
		return 0; // avoid division by zero
	angle -= RAD2DEG(atan2(side, forward));
	angle = (ANGLEMOD(angle + 180) - 180) / 45;
	if (angle >  1)
		return 0;
	if (angle < -1)
		return 0;
	return 1 - fabs(angle);
}

static vec_t CL_GeomLerp(vec_t a, vec_t lerp, vec_t b)
{
	if (a == 0)
	{
		if (lerp < 1)
			return 0;
		else
			return b;
	}
	if (b == 0)
	{
		if (lerp > 0)
			return 0;
		else
			return a;
	}
	return a * pow(fabs(b / a), lerp);
}

static void CL_ClientMovement_Physics_CPM_PM_Aircontrol(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed)
{
	vec_t zspeed, speed, dot, k;

#if 0
	// this doesn't play well with analog input
	if (s->cmd.forwardmove == 0 || s->cmd.sidemove != 0)
		return;
	k = 32;
#else
	k = 32 * (2 * CL_IsMoveInDirection(s->cmd.clx_forwardmove, s->cmd.clx_sidemove, 0) - 1);
	if (k <= 0)
		return;
#endif

	k *= bound(0, wishspeed / cl.movevars_maxairspeed, 1);

	zspeed = s->velocity[2];
	s->velocity[2] = 0;
	speed = VectorNormalizeLength(s->velocity);

	dot = DotProduct(s->velocity, wishdir);

	if (dot > 0) { // we can't change direction while slowing down
		k *= pow(dot, cl.movevars_aircontrol_power)*s->cmd.clx_frametime;
		speed = max(0, speed - cl.movevars_aircontrol_penalty * sqrt(max(0, 1 - dot*dot)) * k/32);
		k *= cl.movevars_aircontrol;
		VectorMAM(speed, s->velocity, k, wishdir, s->velocity);
		VectorNormalize(s->velocity);
	}

	VectorScale(s->velocity, speed, s->velocity);
	s->velocity[2] = zspeed;
}

static float CL_ClientMovement_Physics_AdjustAirAccelQW(float accelqw, float factor)
{
	return
		(accelqw < 0 ? -1 : +1)
		*
		bound(0.000001, 1 - (1 - fabs(accelqw)) * factor, 1);
}

static void CL_ClientMovement_Physics_PM_Accelerate(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed, vec_t wishspeed0, vec_t accel, vec_t accelqw, vec_t stretchfactor, vec_t sidefric, vec_t speedlimit)
{
	vec_t vel_straight;
	vec_t vel_z;
	vec3_t vel_perpend;
	vec_t step;
	vec3_t vel_xy;
	vec_t vel_xy_current;
	vec_t vel_xy_backward, vel_xy_forward;
	vec_t speedclamp;

	if (stretchfactor > 0)
		speedclamp = stretchfactor;
	else if (accelqw < 0)
		speedclamp = 1;
	else
		speedclamp = -1; // no clamping

	if (accelqw < 0)
		accelqw = -accelqw;

	if (cl.moveflags & MOVEFLAG_Q2AIRACCELERATE)
		wishspeed0 = wishspeed; // don't need to emulate this Q1 bug

	vel_straight = DotProduct(s->velocity, wishdir);
	vel_z = s->velocity[2];
	VectorCopy(s->velocity, vel_xy); vel_xy[2] -= vel_z;
	VectorMA(vel_xy, -vel_straight, wishdir, vel_perpend);

	step = accel * s->cmd.clx_frametime * wishspeed0;

	vel_xy_current  = VectorLength(vel_xy);
	if (speedlimit > 0)
		accelqw = CL_ClientMovement_Physics_AdjustAirAccelQW(accelqw, (speedlimit - bound(wishspeed, vel_xy_current, speedlimit)) / max(1, speedlimit - wishspeed));
	vel_xy_forward  = vel_xy_current + bound(0, wishspeed - vel_xy_current, step) * accelqw + step * (1 - accelqw);
	vel_xy_backward = vel_xy_current - bound(0, wishspeed + vel_xy_current, step) * accelqw - step * (1 - accelqw);
	if (vel_xy_backward < 0)
		vel_xy_backward = 0; // not that it REALLY occurs that this would cause wrong behaviour afterwards

	vel_straight    = vel_straight   + bound(0, wishspeed - vel_straight,   step) * accelqw + step * (1 - accelqw);

	if (sidefric < 0 && VectorLength2(vel_perpend))
		// negative: only apply so much sideways friction to stay below the speed you could get by "braking"
	{
		vec_t f, fmin;
		f = max(0, 1 + s->cmd.clx_frametime * wishspeed * sidefric);
		fmin = (vel_xy_backward*vel_xy_backward - vel_straight*vel_straight) / VectorLength2(vel_perpend);
		// assume: fmin > 1
		// vel_xy_backward*vel_xy_backward - vel_straight*vel_straight > vel_perpend*vel_perpend
		// vel_xy_backward*vel_xy_backward > vel_straight*vel_straight + vel_perpend*vel_perpend
		// vel_xy_backward*vel_xy_backward > vel_xy * vel_xy
		// obviously, this cannot be
		if (fmin <= 0)
			VectorScale(vel_perpend, f, vel_perpend);
		else
		{
			fmin = sqrt(fmin);
			VectorScale(vel_perpend, max(fmin, f), vel_perpend);
		}
	}
	else
		VectorScale(vel_perpend, max(0, 1 - s->cmd.clx_frametime * wishspeed * sidefric), vel_perpend);

	VectorMA(vel_perpend, vel_straight, wishdir, s->velocity);

	if (speedclamp >= 0)
	{
		vec_t vel_xy_preclamp;
		vel_xy_preclamp = VectorLength(s->velocity);
		if (vel_xy_preclamp > 0) // prevent division by zero
		{
			vel_xy_current += (vel_xy_forward - vel_xy_current) * speedclamp;
			if (vel_xy_current < vel_xy_preclamp)
				VectorScale(s->velocity, (vel_xy_current / vel_xy_preclamp), s->velocity);
		}
	}

	s->velocity[2] += vel_z;
}

static void CL_ClientMovement_Physics_PM_AirAccelerate(cl_clientmovement_state_t *s, vec3_t wishdir, vec_t wishspeed)
{
    vec3_t curvel, wishvel, acceldir, curdir;
    float addspeed, accelspeed, curspeed;
    float dot;

    float airforwardaccel = cl.movevars_warsowbunny_airforwardaccel;
    float bunnyaccel = cl.movevars_warsowbunny_accel;
    float bunnytopspeed = cl.movevars_warsowbunny_topspeed;
    float turnaccel = cl.movevars_warsowbunny_turnaccel;
    float backtosideratio = cl.movevars_warsowbunny_backtosideratio;

    if ( !wishspeed )
        return;

    VectorCopy( s->velocity, curvel );
    curvel[2] = 0;
    curspeed = VectorLength( curvel );

    if ( wishspeed > curspeed * 1.01f )
    {
        float faccelspeed = curspeed + airforwardaccel * cl.movevars_maxairspeed * s->cmd.clx_frametime;
        if ( faccelspeed < wishspeed )
            wishspeed = faccelspeed;
    }
    else
    {
        float f = ( bunnytopspeed - curspeed ) / ( bunnytopspeed - cl.movevars_maxairspeed );
        if ( f < 0 )
            f = 0;
        wishspeed = max( curspeed, cl.movevars_maxairspeed ) + bunnyaccel * f * cl.movevars_maxairspeed * s->cmd.clx_frametime;
    }
    VectorScale( wishdir, wishspeed, wishvel );
    VectorSubtract( wishvel, curvel, acceldir );
    addspeed = VectorNormalizeLength( acceldir );

    accelspeed = turnaccel * cl.movevars_maxairspeed /* wishspeed */ * s->cmd.clx_frametime;
    if ( accelspeed > addspeed )
        accelspeed = addspeed;

    if ( backtosideratio < 1.0f )
    {
        VectorNormalize2( curvel, curdir );
        dot = DotProduct( acceldir, curdir );
        if ( dot < 0 )
            VectorMA( acceldir, -( 1.0f - backtosideratio ) * dot, curdir, acceldir );
    }

    VectorMA( s->velocity, accelspeed, acceldir, s->velocity );
}

static void CL_ClientMovement_Physics_CheckJump(cl_clientmovement_state_t *s)
{
	// jump if on ground with jump button pressed but only if it has been
	// released at least once since the last jump
	if (s->cmd.clx_jump) {
		if (s->onground && (s->cmd.clx_canjump || !cl_movement_track_canjump.integer)) {
			s->velocity[2] += cl.movevars_jumpvelocity;
			SET___ s->onground = false; // Baker: CL_ClientMovement_Phystics_CheckJump with s->cmd.jump + onground
			s->cmd.clx_canjump = false;
		}
	}
	else
		s->cmd.clx_canjump = true;
}

static void CL_ClientMovement_Physics_Walk (cl_clientmovement_state_t *s, int collide_type)
{
	vec_t friction;
	vec_t wishspeed;
	vec_t addspeed;
	vec_t accelspeed;
	vec_t speed;
	vec_t gravity;
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	vec3_t wishvel;
	vec3_t wishdir;
	vec3_t yawangles;
	trace_t trace;

	CL_ClientMovement_Physics_CheckJump (s);

	// calculate movement vector
	VectorSet		(yawangles, 0, s->cmd.viewangles[1], 0);
	AngleVectors	(yawangles, forward, right, up);
	VectorMAM		(s->cmd.clx_forwardmove, forward, s->cmd.clx_sidemove, right, wishvel);

	// split wishvel into wishspeed and wishdir
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);

	// check if onground
	if (s->onground) {
		wishspeed = min(wishspeed, cl.movevars_maxspeed);
		if (s->crouched)
			wishspeed *= 0.5;

		// apply edge friction
		speed = VectorLength2(s->velocity);
		if (speed > 0) {
			friction = cl.movevars_friction;
			// Hmmm ... "cl_movement_edgefriction", "1", "how much to slow down when you may be about to fall off a ledge (should match edgefriction)"
			if (cl.movevars_edgefriction != 1) {
				// Baker: So far this is never hitting
				vec3_t neworigin2;
				vec3_t neworigin3;
				// note: QW uses the full player box for the trace, and yet still
				// uses s->origin[2] + s->mdl_mins[2], which is clearly an bug, but
				// this mimics it for compatibility
				VectorSet(neworigin2, s->origin[0] + s->velocity[0]*(16/speed), s->origin[1] + s->velocity[1]*(16/speed), s->origin[2] + s->mdl_mins[2]);
				VectorSet(neworigin3, neworigin2[0], neworigin2[1], neworigin2[2] - 34);
				if (cls.protocol == PROTOCOL_QUAKEWORLD)
					trace = CL_TraceBox (neworigin2, s->mdl_mins, s->mdl_maxs, neworigin3, MOVE_NORMAL_0, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true,
					/*HITT_PLAYERS_1*/ collide_type, q_hitnetwork_ent_NULL, q_hitcsqcents_true);
				else
					trace = CL_TraceLine(neworigin2, neworigin3, MOVE_NORMAL_0, s->self, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP, 0, 0, collision_extendmovelength.value, true,
					/*HITT_PLAYERS_1*/ collide_type, q_hitnetwork_ent_NULL, q_hitcsqcents_true, q_hitsuraces_false);
				if (trace.fraction == 1 && !trace.startsolid)
					friction *= cl.movevars_edgefriction;
			}
			// apply ground friction
			speed = 1 - s->cmd.clx_frametime * friction * ((speed < cl.movevars_stopspeed) ? (cl.movevars_stopspeed / speed) : 1);
			speed = max(speed, 0);
			VectorScale(s->velocity, speed, s->velocity);
		}
		addspeed = wishspeed - DotProduct(s->velocity, wishdir);
		if (addspeed > 0)
		{
			accelspeed = min(cl.movevars_accelerate * s->cmd.clx_frametime * wishspeed, addspeed);
			VectorMA(s->velocity, accelspeed, wishdir, s->velocity);
		}
		gravity = cl.movevars_gravity * cl.movevars_entgravity * s->cmd.clx_frametime;
		if (Have_Flag(cl.moveflags, MOVEFLAG_NOGRAVITYONGROUND) == false) {
			if (cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
				s->velocity[2] -= gravity * 0.5f;
			else
				s->velocity[2] -= gravity;
		}
		// Baker: Setting this for DarkPlaces didn't really help or hurt.
		if (cls.protocol == PROTOCOL_QUAKEWORLD)
			s->velocity[2] = 0;
		if (VectorLength2(s->velocity))
			CL_ClientMovement_Move (s, collide_type);
		if (Have_Flag(cl.moveflags, MOVEFLAG_NOGRAVITYONGROUND) == false || !s->onground) {
			if (Have_Flag (cl.moveflags, MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE))
				s->velocity[2] -= gravity * 0.5f;
		}
	}
	else
	{
		// NOT ON GROUND ...
		if (s->waterjumptime <= 0) {
			// apply air speed limit
			vec_t accel, wishspeed0, wishspeed2, accelqw, strafity;
			qbool accelerating;

			accelqw = cl.movevars_airaccel_qw;
			wishspeed0 = wishspeed;
			wishspeed = min(wishspeed, cl.movevars_maxairspeed);
			if (s->crouched)
				wishspeed *= 0.5;
			accel = cl.movevars_airaccelerate;

			accelerating = (DotProduct(s->velocity, wishdir) > 0);
			wishspeed2 = wishspeed;

			// CPM: air control
			if (cl.movevars_airstopaccelerate != 0) {
				vec3_t curdir;
				curdir[0] = s->velocity[0];
				curdir[1] = s->velocity[1];
				curdir[2] = 0;
				VectorNormalize(curdir);
				accel = accel + (cl.movevars_airstopaccelerate - accel) * max(0, -DotProduct(curdir, wishdir));
			}
			strafity = CL_IsMoveInDirection(s->cmd.clx_forwardmove, s->cmd.clx_sidemove, -90) + CL_IsMoveInDirection(s->cmd.clx_forwardmove, s->cmd.clx_sidemove, +90); // if one is nonzero, other is always zero
			if (cl.movevars_maxairstrafespeed)
				wishspeed = min(wishspeed, CL_GeomLerp(cl.movevars_maxairspeed, strafity, cl.movevars_maxairstrafespeed));
			if (cl.movevars_airstrafeaccelerate)
				accel = CL_GeomLerp(cl.movevars_airaccelerate, strafity, cl.movevars_airstrafeaccelerate);
			if (cl.movevars_airstrafeaccel_qw)
				accelqw =
					(((strafity > 0.5 ? cl.movevars_airstrafeaccel_qw : cl.movevars_airaccel_qw) >= 0) ? +1 : -1)
					*
					(1 - CL_GeomLerp(1 - fabs(cl.movevars_airaccel_qw), strafity, 1 - fabs(cl.movevars_airstrafeaccel_qw)));
			// !CPM

			if (cl.movevars_warsowbunny_turnaccel && accelerating && s->cmd.clx_sidemove == 0 && s->cmd.clx_forwardmove != 0)
				CL_ClientMovement_Physics_PM_AirAccelerate(s, wishdir, wishspeed2);
			else
				CL_ClientMovement_Physics_PM_Accelerate(s, wishdir, wishspeed, wishspeed0, accel, accelqw, cl.movevars_airaccel_qw_stretchfactor, cl.movevars_airaccel_sideways_friction / cl.movevars_maxairspeed, cl.movevars_airspeedlimit_nonqw);

			if (cl.movevars_aircontrol)
				CL_ClientMovement_Physics_CPM_PM_Aircontrol(s, wishdir, wishspeed2);
		}
		gravity = cl.movevars_gravity * cl.movevars_entgravity * s->cmd.clx_frametime;
		if (cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
			s->velocity[2] -= gravity * 0.5f;
		else
			s->velocity[2] -= gravity;
		CL_ClientMovement_Move (s, collide_type);
		if (Have_Flag(cl.moveflags, MOVEFLAG_NOGRAVITYONGROUND) == false || !s->onground)
		{
			if (cl.moveflags & MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE)
				s->velocity[2] -= gravity * 0.5f;
		}
	}
}

#include "cl_input_pmove_spectator.c.h"

WARP_X_ (Calls CL_ClientMovement_UpdateStatus)
static void PM_CheckWaterJump (cl_clientmovement_state_t *s)
{
	if (s->waterjumptime)
		return;

	// don't hop out if we just jumped in
	if (s->velocity[2] < -180)
		return;

	// see if near an edge
	vec3_t pm_forward, pm_right;
	AngleVectors (cl.viewangles, pm_forward, pm_right, NULL);

	vec3_t flatforward = { pm_forward[0], pm_forward[1], 0 };
	VectorNormalize (flatforward);

	vec3_t spot;
	VectorMA (s->origin, 24, flatforward, spot);
	spot[2] += 8;

	int pmove_contents =
		CL_TracePoint(spot, MOVE_NOMONSTERS_1,
		s->self, /*hit skip skip*/ 0, 0, 0, q_hitbrush_true, HITT_NOPLAYERS_0,
		q_hitnetwork_ent_NULL, q_hitcsqcents_false).startsupercontents;// & SUPERCONTENTS_LIQUIDSMASK;


	if (Have_Flag (pmove_contents, SUPERCONTENTS_SOLID) == false)
		return;
	spot[2] += 24;
	pmove_contents = 		CL_TracePoint(spot, MOVE_NOMONSTERS_1,
		s->self, /*hit skip skip*/ 0, 0, 0, q_hitbrush_true, HITT_NOPLAYERS_0,
		q_hitnetwork_ent_NULL, q_hitcsqcents_false).startsupercontents;// & SUPERCONTENTS_LIQUIDSMASK;

	if (pmove_contents != SUPERCONTENTS_SKIP_NONE_0 /*empty*/)
		return;
	// jump out of water
	VectorScale (flatforward, 50, s->velocity);
	s->velocity[2] = 200;
	s->waterjumptime = 2; // safety net
	//s->jpmove.jump_held = true; // don't jump again until released
}

static void CL_ClientMovement_PlayerMove (cl_clientmovement_state_t *s, int collide_type)
{
	//Con_Printf (" %f", frametime);
	if (!s->cmd.clx_jump)
		s->cmd.clx_canjump = true;
	s->waterjumptime -= s->cmd.clx_frametime;
	CL_ClientMovement_UpdateStatus(s, collide_type);

	if (QW_TREAT_AS_SPECTATOR) {
		// Baker: Noclip plus fly
		CL_ClientMovement_Physics_PM_SpectatorMove (s);
		return;
	}

	// Baker: Let's try to make getting out of water easier
#if 1
	if (s->waterlevel == WATERLEVEL_SWIMMING_2)
		PM_CheckWaterJump(s);
#endif

	if (s->waterlevel >= WATERLEVEL_SWIMMING_2)
		CL_ClientMovement_Physics_Swim (s, collide_type);
	else
		CL_ClientMovement_Physics_Walk (s, collide_type);
}

void CL_ClientMovement_PlayerMove_Frame (cl_clientmovement_state_t *s, int collide_type)
{
	// if a move is more than 50ms, do it as two moves (matching qwsv)
	//Con_Printf ("%d ", s.cmd.msec);
	if (s->cmd.clx_frametime > 0.0005) {
		if (s->cmd.clx_frametime > 0.05) {
//			if (developer_movement.integer) // OLD
//				Con_PrintLinef ("Did 2 moves");
			s->cmd.clx_frametime /= 2;
			CL_ClientMovement_PlayerMove (s, collide_type);
		}
		CL_ClientMovement_PlayerMove (s, collide_type);
	}
	else
	{
		// we REALLY need this handling to happen, even if the move is not executed
		if (!s->cmd.clx_jump)
			s->cmd.clx_canjump = true;
	}
}

WARP_X_ (CL_Frame -> CL_UpdateWorld -> )
WARP_X_ (CL_UpdateMoveVars CL_ClientMovement_UpdateStatus)

// Returns zero if nothing is hit, otherwise entity number + 1
int Is_In_Bad_Place_Ent_Plus1 (cl_clientmovement_state_t *s, int collide_type)
{
	trace_t		check_our_player_trace;
	int reply_num = -2;
	// Do we really want water check here?

	check_our_player_trace = CL_TraceBox (s->origin, cl.playerstandmins, cl.playerstandmaxs, s->origin,
		MOVE_NORMAL_0, s->self,
		/*hit these*/  SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP,
		/*skip these*/ SUPERCONTENTS_SKIP_NONE_0,
		/*skip these*/ MATERIALFLAG_NONE_0,
		collision_extendmovelength.value, q_hitbrush_true,
		 /*HITT_PLAYERS_1*/ /*collide_type*/ HITT_NOPLAYERS_0, &reply_num/*q_hitnetwork_ent_NULL*/,
		 q_hitcsqcents_true);
	if (check_our_player_trace.startsolid) {
		if (reply_num != -2)
			return reply_num + 1;
		return -1;
	}

	return 0;
}

// Baker: Take the nudge and apply it to the first move, tell the rest of the moves to recalculate.
void Zircon_Elevator_Apply_Nudge (cl_clientmovement_state_t *plyr, int collide_type, float nudge)
{
	int move_seq;
	for (move_seq = 0; move_seq < CL_MAX_USERCMDS_128; move_seq ++) {
		if (cl.movecmd[move_seq].clx_sequence <= cls.servermovesequence) {
			break;
		}
	} // move_seq

	// Baker: move_seq will be first move greater than server known sequence.
	// now walk them in oldest to newest order
	// Baker: 0 is current move, so subtracting gets closer to the present
	//int is_first = true;
	for (move_seq --; move_seq >= 0; move_seq --) {
		usercmd_t *m = &cl.movecmd[move_seq]; // Baker: Struct copy ..
		m->zmove_start_origin[0] += nudge;
		m->zmove_end_origin[0] += nudge;
//		is_first = false;
	} // for

	// Baker: Hit this too ...
	plyr->origin[2] += nudge;

	// Baker: Can't do this
	//if (plyr->velocity[2] <= 0) {
	//	plyr->velocity[2] += 0.5;
	//}
}

int Zircon_Elevator_Check_Fix_Is_Ok (cl_clientmovement_state_t *plyr, int collide_type)
{
	// We only allow this so often
	if (cl.zircon_step_sequence >= cls.servermovesequence)
		return true;

	// BAD PLACE CHECK
	int hit_ent_plus1 = Is_In_Bad_Place_Ent_Plus1 (plyr, collide_type); // Brush collide only
	if (hit_ent_plus1 == 0)
		return true; // OK!

	// ELEVATOR CALC
	float start_origin_z = plyr->origin[2];

	// We have a startsolid start with a brush model.
	if (Have_Flag (developer_movement.integer, /*CL*/ 1))
		Con_PrintLinef ("CL: Elevator - Start solid in brush!  Attempt elevator fix ...  %d (unplussed = %d)", hit_ent_plus1, UNPLUS1(hit_ent_plus1));

	// Baker: Moving up 20 fixes .. What is the least we can do

	float nudge;
	int hit_ent_20_plus1;
	int did_fix = false;
	for (nudge = 1.25; nudge < 10; nudge += 0.25) {
		plyr->origin[2] = start_origin_z + nudge;
		hit_ent_20_plus1 = Is_In_Bad_Place_Ent_Plus1 (plyr, collide_type);

		if (hit_ent_20_plus1 == 0) {
			did_fix = true;
			cl.zircon_warp_sequence = cls.servermovesequence;
			break; // FIX ACHIEVED
		}
	} // for

	if (did_fix == false) {
		if (Have_Flag (developer_movement.integer, /*CL*/ 1))
			Con_PrintLinef ("CL: Elevator - Can't fix issue with nudge");
		return 2; // There is a problem
	}

	if (Have_Flag (developer_movement.integer, /*CL*/ 1))
		Con_PrintLinef ("CL: Elevator fixed WITH MOVEUP nudge %f!", nudge);

	// Restore origin to original
	plyr->origin[2] = start_origin_z;

	// If nudge, adjust the moves.
	if (nudge)
		Zircon_Elevator_Apply_Nudge (plyr, collide_type, nudge);

	return false; // Elevator did correct
} // Elevator

void CL_ClientMovement_Replay (int collide_type)
{
	if (cl.movement_predicted && !cl.movement_replay)
		return;

	if (!cl_movement_replay.integer /*d: 1*/)
		return;

	// set up starting state for the series of moves
	cl_clientmovement_state_t s;
	memset (&s, 0, sizeof(s));

	// Baker: cls.servermovesequence is the last move the server knew about
	double totalmovemsec = 0; // Baker: Calculated here
	for (int movenum = 0; movenum < CL_MAX_USERCMDS_128; movenum ++) {
		if (cl.movecmd[movenum].clx_sequence > cls.servermovesequence)
			totalmovemsec += cl.movecmd[movenum].clx_msec;
	}

	// A Baker: BIG EQUALS STATEMENT - move along ...
	cl.movement_predicted = /*MOVED this BELOW --> totalmovemsec >= cl_movement_minping.value d: 0 && */
		cls.servermovesequence &&
		(QW_TREAT_AS_CL_MOVEMENT && !cls.demoplayback &&
		cls.signon == SIGNONS_4 &&
			(cl.stats[STAT_HEALTH] > 0 || QW_TREAT_AS_SPECTATOR) &&
			!cl.intermission
		);

#if 4321
	// Baker: cl_movement_minping is incompatible with Zircon Free Move
	// It cannot be allowed if ZMOVE is even possible, because the warping
	// behavior with cl_minping with DP7.
	if (cl.movement_predicted && ZMOVE_IS_ENABLED && !cls.demoplayback  ) {
		cl.movement_predicted = PRED_ZIRCON_MOVE_2;
	} else if (Have_Zircon_Ext_Flag_CLS (ZIRCON_EXT_FREEMOVE_4) == false) {
		int is_ok_minping = (totalmovemsec >= cl_movement_minping.value);
		if (cl.movement_predicted && is_ok_minping == false)
		cl.movement_predicted = false;
	}

	static int glow, ghigh;
	// USE ORIGIN FROM SERVER
	if (cl.movement_predicted == PRED_ZIRCON_MOVE_2 && cl.movement_final_origin_set) {
		// Baker: Try to find the current sequence
		int move_seq;
		for (move_seq = 0; move_seq < CL_MAX_USERCMDS_128; move_seq ++) {
			if (cl.movecmd[move_seq].clx_sequence <= cls.servermovesequence) {
				break;
			}
		} // move_seq

		if (cls.servermovesequence <= 2) {
			if (developer_movement.integer  > 1)
				Con_PrintLinef ("%d: Very Early Move Use Regular Prediction", (int)cls.servermovesequence);
			goto copy_anyway;
		}

		// Baker: If We found this sequence
		if (cl.movecmd[move_seq].clx_sequence == cls.servermovesequence)  {
			usercmd_t *m = &cl.movecmd[move_seq];

			// Baker: Found exact sequence
			if (m->zmove_is_move_processed && m->zmove_is_move_processed == m->clx_sequence) {
				VectorCopy (m->zmove_end_origin, s.origin);
				VectorCopy (m->zmove_end_velocity, s.velocity);
				s.onground = m->zmove_end_onground;
				goto do_not_copy;
			}

			// Baker: Exact sequence but NOT processed.
			if (developer_movement.integer > 3)
				Con_PrintLinef ("CL %u: Not processed %u old low %u old high %u .. selecting recover option ...", cls.servermovesequence, move_seq, glow, ghigh);

			// Baker: Trying to recover by looking at previous move and using end position
			if (cls.servermovesequence && move_seq < CL_MAX_USERCMDS_128 - 1) {
				usercmd_t *mprev = &cl.movecmd[move_seq + 1];
				if (mprev->clx_sequence == cls.servermovesequence - 1 &&
					mprev->zmove_is_move_processed && mprev->zmove_is_move_processed == mprev->clx_sequence) {
					VectorCopy (mprev->zmove_end_origin, s.origin);
					VectorCopy (mprev->zmove_end_velocity, s.velocity);
					s.onground = mprev->zmove_end_onground;
					if (Vector3_IsZeros (s.origin)) {
						if (developer_movement.integer > 3)
							Con_PrintLinef ("CL %u: Zero origin on recover ok", cls.servermovesequence);
					}
					if (developer_movement.integer > 3)
						Con_PrintLinef ("CL %u: Recovered ok using previous move end position", cls.servermovesequence);
					goto do_not_copy;
				} // If previous sequence is good and processed
				if (developer_movement.integer > 3)
					Con_PrintLinef ("CL %u: Can't recover from previous move because it was not processed", cls.servermovesequence);
			} else { // If a previous sequence available.
				if (developer_movement.integer > 3)
					Con_PrintLinef ("CL %u: Can't recover from previous move because queue filled", cls.servermovesequence);
			}
		} else {
			if (developer_movement.integer > 3)
			Con_PrintLinef ("%d: Missing or new sequence, going hard copy", cls.servermovesequence);
		}

		// Baker: Queue filled or previous move was not processed
		// Baker: Recover using saved origin/velocity/
		VectorCopy (cl.zircon_replay_save.zmove_end_origin, s.origin);
		VectorCopy (cl.zircon_replay_save.zmove_end_velocity, s.velocity);
		s.onground = cl.zircon_replay_save.zmove_end_onground;
		if (Vector3_IsZeros (s.origin)) {
			if (developer_movement.integer > 3)
				Con_PrintLinef ("CL %u: Zero origin on recover failed, using normal prediction", cls.servermovesequence);
			goto copy_anyway;
		}
		if (developer_movement.integer > 3)
			Con_PrintLinef ("CL %u: Recovered using SAVE global", cls.servermovesequence);
		goto do_not_copy;
	} // ZMOVE

	// Baker: Not a ZMOVE
	if (Have_Flag (developer_movement.integer, /*CL*/ 1)) // DP PRED
		Con_PrintLinef ("CL: %u: DP7 predicted start", (int)cls.servermovesequence);
#endif
copy_anyway:
	VectorCopy (cl.entities[cl.playerentity].state_current.origin, s.origin); // ZMOVE CL_ClientMovement_Replay
	VectorCopy (cl.mvelocity[0], s.velocity);
	goto dp7_go;

do_not_copy:


dp7_go:

	s.crouched = true; // will be updated on first move
	// Baker: This happens all the time.
	//if (developer_movement.integer >= 2)
	//	Con_PrintLinef ("move replay start at org %5.1f %5.1f %5.1f "
	//		" vel %3.1f %3.1f %3.1f", s.origin[0], s.origin[1], s.origin[2],
	//		s.velocity[0], s.velocity[1], s.velocity[2]);

	//Con_Printf ("%d = %.0f >= %.0f && %u && (%d && %d && %d == %d && %d > 0 && %d\n", cl.movement_predicted, totalmovemsec, cl_movement_minping.value, cls.servermovesequence, cl_movement.integer, !cls.demoplayback, cls.signon, SIGNONS_4, cl.stats[STAT_HEALTH], !cl.intermission);
	if (cl.movement_predicted) {
		//Con_Printf ("%dms\n", cl.movecmd[0].msec);

		// replay the input queue to predict current location
		// note: this relies on the fact there's always one queue item at the end

		if (cl.movement_predicted == PRED_ZIRCON_MOVE_2) {
			Zircon_Elevator_Check_Fix_Is_Ok (&s, collide_type);
		}
		// find how many are still valid
		int move_seq;
		for (move_seq = 0; move_seq < CL_MAX_USERCMDS_128; move_seq ++) {
			if (cl.movecmd[move_seq].clx_sequence <= cls.servermovesequence) {
				break;
			}
		} // move_seq

		// Baker: move_seq will be first move greater than server known sequence.
		glow = cl.movecmd[move_seq - 1].clx_sequence;
		// now walk them in oldest to newest order
		// Baker: 0 is current move, so subtracting gets closer to the present
		for (move_seq --; move_seq >= 0; move_seq --) {
#if 4321
			WARP_X_ (quemove)
			// This appears to be to alter the s.cmd without it affecting the stored version
			usercmd_t *m = &cl.movecmd[move_seq]; // Baker: Struct copy ..
			if (m->zmove_is_move_processed != m->clx_sequence) {
				VectorCopy (s.origin, m->zmove_start_origin);
				VectorCopy (s.velocity, m->zmove_start_velocity);
				//m->zmove_start_onground = s.onground;
			}
			else if (cl.movement_predicted == PRED_ZIRCON_MOVE_2) {
				// PROCESSED ZMOVE - Use it if
				s.cmd = cl.movecmd[move_seq];
				// Baker: Missing crouch and waterlevel, anything else? watertype
				VectorCopy (m->zmove_end_origin, s.origin);
				VectorCopy (m->zmove_end_velocity, s.velocity);
				s.onground = m->zmove_end_onground;
				continue; // DONE WITH THIS MOVE
			}

#endif
			s.cmd = cl.movecmd[move_seq]; // Baker: Struct copy

			// Baker: Check the previous move to inherit canjump if possible.
			if (move_seq < CL_MAX_USERCMDS_128 - 1)
				s.cmd.clx_canjump = cl.movecmd[move_seq + 1].clx_canjump;

			CL_ClientMovement_PlayerMove_Frame (&s, collide_type);

			cl.movecmd[move_seq].clx_canjump = s.cmd.clx_canjump;
#if 4321
			if (m->zmove_is_move_processed != m->clx_sequence) {
				VectorCopy (s.origin, m->zmove_end_origin);
				VectorCopy (s.velocity, m->zmove_end_velocity);
				m->zmove_end_onground = s.onground;
				m->zmove_is_move_processed = m->clx_sequence;
			}
			cl.zircon_replay_high = ghigh = cl.movecmd[move_seq - 1].clx_sequence;
			if (Vector3_IsZeros (s.origin)) {
				if (Have_Flag (developer_movement.integer, /*CL*/ 1))
					Con_PrintLinef ("CL: Wants to save zero s.origin");
			}
			cl.zircon_replay_save = cl.movecmd[move_seq];
#endif
		} // for move_seq

		CL_ClientMovement_UpdateStatus (&s, collide_type); // Baker: Crouch, onground, waterlevel
	}
	else { // !cl.movement_predicted ...
		// get the first movement queue entry to know whether to crouch and such
		s.cmd = cl.movecmd[0]; // Baker: Struct copy
		if (Have_Flag (developer_movement.integer, /*CL*/ 1))
			Con_PrintLinef ("CL: Replay -> This move is not predicted");
	}

	if (!cls.demoplayback) { // for bob, speedometer
		cl.movement_replay = false;
		// update the interpolation target position and velocity
		VectorCopy	(s.origin, cl.movement_origin);
		VectorCopy	(s.velocity, cl.movement_velocity);
#if 4321
		VectorCopy  (s.origin, cl.movement_final_origin);

		VectorCopy	(s.velocity, cl.movement_final_velocity);
		// causes jitter VectorCopy	(s.velocity, cl.mvelocity[0]);
		//if (Vector3_IsZeros (s.velocity)) {
		//	[0] = 0;
		//	cl.mvelocity[0][1] = 0;
		//	cl.mvelocity[0][2] = 0;
		//	//Con_PrintLinef ("Zeroed out mvelocity");
		//}
		cl.movement_final_origin_set = true;
#endif
	}

	// update the onground flag if appropriate
	if (cl.movement_predicted) {
		// when predicted we simply set the flag according to the UpdateStatus
		if (QW_TREAT_AS_SPECTATOR)
			cl.onground = false; // Baker: CL_ClientMovement_Replay cl.movement_predicted == true BUT SPECTATOR
		else
			SET___ cl.onground = s.onground; // Baker: CL_ClientMovement_Replay cl.movement_predicted == true
	}
	else {
		// when not predicted, cl.onground is cleared by cl_parse.c each time
		// an update packet is received, but can be forced on here to hide
		// server inconsistencies in the onground flag
		// (which mostly occur when stepping up stairs at very high framerates
		//  where after the step up the move continues forward and not
		//  downward so the ground is not detected)
		//
		// such onground inconsistencies can cause jittery gun bobbing and
		// stair smoothing, so we set onground if UpdateStatus says so
		if (s.onground)
			SET___ cl.onground = true; // Baker: CL_ClientMovement_Replay cl.movement_predicted == false
	}
}


