#include "quakedef.h"
#include "protocol.h"

WARP_X_ (QW_CL_FindModelNumbers qw_monsters)
int QW_Is_Step_ModelIndex_Plus1(int qw_modelindex)
{
	if (!qw_modelindex)
		return false;

//	const char *sm = cl.model_name[qw_modelindex];
	for (int midx = 0; midx < cl.qw_monsters_modelindexes_count; midx ++)  {
		if (qw_modelindex == cl.qw_monsters_modelindexes[midx]) {
			return PLUS1(cl.qw_monsters_modelindexes_qw_monster_id[midx]);
			break;
		}
	} // midx
	return false;
}

static void QW_TranslateEffects(entity_state_t *s, int qweffects)
{
	s->effects = 0;
	s->internaleffects = 0;
	if (qweffects & QW_EF_BRIGHTFIELD_1)
		s->effects |= EF_BRIGHTFIELD_1;
	if (qweffects & QW_EF_MUZZLEFLASH_2)
		s->effects |= EF_MUZZLEFLASH_2;
	if (qweffects & QW_EF_FLAG1_16)
	{
		// mimic FTEQW's interpretation of EF_FLAG1 as EF_NODRAW_16 on non-player entities
		if (s->number > cl.maxclients)
			s->effects |= EF_NODRAW_16;
		else
			s->internaleffects |= INTEF_FLAG1QW_1;
	}
	if (qweffects & QW_EF_FLAG2_32)
	{
		// mimic FTEQW's interpretation of EF_FLAG2 as EF_ADDITIVE on non-player entities
		if (s->number > cl.maxclients)
			s->effects |= EF_ADDITIVE_32;
		else
			s->internaleffects |= INTEF_FLAG2QW_2;
	}
	if (qweffects & QW_EF_RED_128)
	{
		if (qweffects & QW_EF_BLUE_64)
			s->effects |= EF_RED_128 | EF_BLUE_64;
		else
			s->effects |= EF_RED_128;
	}
	else if (qweffects & QW_EF_BLUE_64)
		s->effects |= EF_BLUE_64;
	else if (qweffects & QW_EF_BRIGHTLIGHT_4)
		s->effects |= EF_BRIGHTLIGHT_4;
	else if (qweffects & QW_EF_DIMLIGHT_8)
		s->effects |= EF_DIMLIGHT_8;
}

extern cvar_t cl_rollangle;
extern cvar_t cl_rollspeed;

WARP_X_ (qw_svc_playerinfo)
void EntityStateQW_ReadPlayerUpdate (void)
{
	int slot = MSG_ReadByte(&cl_message);
	int enumber = slot + 1;
	int weaponframe;
	int msec;
	int playerflags;
	int bits;
	entity_state_t *s;
	// look up the entity
	entity_t *ent = cl.entities + enumber;
	vec3_t viewangles;
	vec3_t velocity;

	// slide the current state into the previous
	ent->state_previous = ent->state_current;

	// read the update
	s = &ent->state_current;
	*s = defaultstate;
	s->active = ACTIVE_NETWORK;
	s->number = enumber;
	s->colormap = enumber;
	playerflags = MSG_ReadShort(&cl_message);
	MSG_ReadVector(&cl_message, s->origin, cls.protocol);
	s->frame = MSG_ReadByte(&cl_message);

	VectorClear(viewangles);
	VectorClear(velocity);

	if (playerflags & QW_PF_MSEC)
	{
		// time difference between last update this player sent to the server,
		// and last input we sent to the server (this packet is in response to
		// our input, so msec is how long ago the last update of this player
		// entity occurred, compared to our input being received)
		msec = MSG_ReadByte(&cl_message);
	}
	else
		msec = 0;
	if (playerflags & QW_PF_COMMAND)
	{
		bits = MSG_ReadByte(&cl_message);
		if (bits & QW_CM_ANGLE1)
			viewangles[0] = MSG_ReadAngle16i(&cl_message); // cmd->angles[0]
		if (bits & QW_CM_ANGLE2)
			viewangles[1] = MSG_ReadAngle16i(&cl_message); // cmd->angles[1]
		if (bits & QW_CM_ANGLE3)
			viewangles[2] = MSG_ReadAngle16i(&cl_message); // cmd->angles[2]
		if (bits & QW_CM_FORWARD)
			MSG_ReadShort(&cl_message); // cmd->forwardmove
		if (bits & QW_CM_SIDE)
			MSG_ReadShort(&cl_message); // cmd->sidemove
		if (bits & QW_CM_UP)
			MSG_ReadShort(&cl_message); // cmd->upmove
		if (bits & QW_CM_BUTTONS)
			(void) MSG_ReadByte(&cl_message); // cmd->buttons
		if (bits & QW_CM_IMPULSE)
			(void) MSG_ReadByte(&cl_message); // cmd->impulse
		(void) MSG_ReadByte(&cl_message); // cmd->msec
	}
	if (playerflags & QW_PF_VELOCITY1)
		velocity[0] = MSG_ReadShort(&cl_message);
	if (playerflags & QW_PF_VELOCITY2)
		velocity[1] = MSG_ReadShort(&cl_message);
	if (playerflags & QW_PF_VELOCITY3)
		velocity[2] = MSG_ReadShort(&cl_message);
	if (playerflags & QW_PF_MODEL)
		s->modelindex = MSG_ReadByte(&cl_message); // Baker: Player model
	else
		s->modelindex = cl.qw_modelindex_player;
	if (s->sflags)
		s->sflags = 0;
	if (playerflags & QW_PF_SKINNUM)
		s->skin = MSG_ReadByte(&cl_message);
	if (playerflags & QW_PF_EFFECTS)
		QW_TranslateEffects(s, MSG_ReadByte(&cl_message));
	if (playerflags & QW_PF_WEAPONFRAME)
		weaponframe = MSG_ReadByte(&cl_message);
	else
		weaponframe = 0;

	if (enumber == cl.playerentity) {
		// if this is an update on our player, update the angles
		VectorCopy(cl.viewangles, viewangles);
	}

	// calculate the entity angles from the viewangles
	s->angles[0] = viewangles[0] * -0.0333;
	s->angles[1] = viewangles[1];
	s->angles[2] = 0;
	s->angles[2] = Com_CalcRoll(s->angles, velocity, cl_rollangle.value, cl_rollspeed.value)*4;

	// if this is an update on our player, update interpolation state
	if (enumber == cl.playerentity) {
		VectorCopy (cl.mpunchangle[0], cl.mpunchangle[1]);
		VectorCopy (cl.mpunchvector[0], cl.mpunchvector[1]);
		VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
		cl.mviewzoom[1] = cl.mviewzoom[0];

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

		VectorCopy(velocity, cl.mvelocity[0]);
		cl.stats[STAT_WEAPONFRAME] = weaponframe;
		if (playerflags & QW_PF_GIB)
			cl.stats[STAT_VIEWHEIGHT] = 8;
		else if (playerflags & QW_PF_DEAD)
			cl.stats[STAT_VIEWHEIGHT] = -16;
		else
			cl.stats[STAT_VIEWHEIGHT] = 22;
	}

	// set the cl.entities_active flag
	cl.entities_active[enumber] = (s->active == ACTIVE_NETWORK);
	// set the update time
	s->time = cl.mtime[0] - msec * 0.001; // qw has no clock
	// check if we need to update the lerp stuff
	if (s->active == ACTIVE_NETWORK)
		CL_MoveLerpEntityStates(&cl.entities[enumber]);
}

static void EntityStateQW_CL_ReadFrame_ReadEntityUpdate (entity_state_t *s, int snum, int bits)
{
	int qweffects = 0;
	s->active = ACTIVE_NETWORK;
	s->number = bits & 511;

	bits &= ~511;
	if (Have_Flag (bits, QW_U_MOREBITS))
		bits |= MSG_ReadByte(&cl_message);

	int morebits = 0;
	if (Have_Flag (bits, QW_FTE_U_EVENMORE_S7) && Have_Flag (cls.fteprotocolextensions, PEXT_ENTITYDBL)) {
		// Fte extensions for huge entity counts
		morebits	= MSG_ReadByte(&cl_message); // Read even more
		//if (Have_Flag (morebits, QW_FTE_U_YETMORE)) {
		//	morebits |= MSG_ReadByte(&cl_message)<<8; // Read yetmore
		if (Have_Flag (morebits, QW_FTE_U_ENTITYDBL_S5))	s->number += 512;
		if (Have_Flag (morebits, QW_FTE_U_ENTITYDBL2_S6))	s->number += 1024;
		//}
	}

	// store the QW_U_SOLID bit here?
	if (bits & QW_U_MODEL) {
		s->modelindex = MSG_ReadByte(&cl_message); // Monster modelindex
	}

	if (bits & QW_U_FRAME) {
		s->frame = MSG_ReadByte(&cl_message);
	}
	if (bits & QW_U_COLORMAP)
		s->colormap = MSG_ReadByte(&cl_message);
	if (bits & QW_U_SKIN)
		s->skin = MSG_ReadByte(&cl_message);
	if (bits & QW_U_EFFECTS)
		QW_TranslateEffects(s, qweffects = MSG_ReadByte(&cl_message));
	if (bits & QW_U_ORIGIN1)
		s->origin[0] = MSG_ReadCoord13i(&cl_message);
	if (bits & QW_U_ANGLE1)
		s->angles[0] = MSG_ReadAngle8i(&cl_message);
	if (bits & QW_U_ORIGIN2)
		s->origin[1] = MSG_ReadCoord13i(&cl_message);
	if (bits & QW_U_ANGLE2)
		s->angles[1] = MSG_ReadAngle8i(&cl_message);
	if (bits & QW_U_ORIGIN3)
		s->origin[2] = MSG_ReadCoord13i(&cl_message);
	if (bits & QW_U_ANGLE3)
		s->angles[2] = MSG_ReadAngle8i(&cl_message);

	if (developer_networkentities.integer >= 2) {
		Con_Printf ("ReadFields e%d", s->number);
		if (bits & QW_U_MODEL)
			Con_Printf (" U_MODEL %d", s->modelindex);
		if (bits & QW_U_FRAME)
			Con_Printf (" U_FRAME %d", s->frame);
		if (bits & QW_U_COLORMAP)
			Con_Printf (" U_COLORMAP %d", s->colormap);
		if (bits & QW_U_SKIN)
			Con_Printf (" U_SKIN %d", s->skin);
		if (bits & QW_U_EFFECTS)
			Con_Printf (" U_EFFECTS %d", qweffects);
		if (bits & QW_U_ORIGIN1)
			Con_Printf (" U_ORIGIN1 %f", s->origin[0]);
		if (bits & QW_U_ANGLE1)
			Con_Printf (" U_ANGLE1 %f", s->angles[0]);
		if (bits & QW_U_ORIGIN2)
			Con_Printf (" U_ORIGIN2 %f", s->origin[1]);
		if (bits & QW_U_ANGLE2)
			Con_Printf (" U_ANGLE2 %f", s->angles[1]);
		if (bits & QW_U_ORIGIN3)
			Con_Printf (" U_ORIGIN3 %f", s->origin[2]);
		if (bits & QW_U_ANGLE3)
			Con_Printf (" U_ANGLE3 %f", s->angles[2]);
		if (bits & QW_U_SOLID)
			Con_Printf (" U_SOLID");
		Con_Print("\n");
	}

	int qw_monster_id_plus1 = QW_Is_Step_ModelIndex_Plus1(s->modelindex);
	if (qw_monster_id_plus1) {
		qbool QW_Is_Step_Dead (int qw_monster_id, int frame);
		int is_dead = QW_Is_Step_Dead (UNPLUS1(qw_monster_id_plus1), s->frame);
		if (is_dead) {
			Flag_Remove_From (s->sflags, RENDER_STEP);
			//Flag_Add_To (s->effects, EF_RED_128);
		} else {
			Flag_Add_To (s->sflags, RENDER_STEP);
			//Flag_Remove_From (s->effects, EF_RED_128);
		}
	} // monster model index

}

entityframeqw_database_t *EntityFrameQW_AllocDatabase(mempool_t *pool)
{
	entityframeqw_database_t *d;
	d = (entityframeqw_database_t *)Mem_Alloc(pool, sizeof(*d));
	return d;
}

void EntityFrameQW_FreeDatabase(entityframeqw_database_t *d)
{
	Mem_Free(d);
}

WARP_X_ (qw_svc_packetentities qw_svc_deltapacketentities)
void EntityFrameQW_CL_ReadFrame(qbool delta)
{
	qbool invalid = false;
	int number, oldsnapindex, newsnapindex, oldindex, newindex, oldnum, newnum;
	entity_t *ent;
	entityframeqw_database_t *d;
	entityframeqw_snapshot_t *oldsnap, *newsnap;

	if (!cl.entitydatabaseqw)
		cl.entitydatabaseqw = EntityFrameQW_AllocDatabase(cls.levelmempool);
	d = cl.entitydatabaseqw;

	// there is no cls.netcon in demos, so this reading code can't access
	// cls.netcon-> at all...  so cls.qw_incoming_sequence and
	// cls.qw_outgoing_sequence are updated every time the corresponding
	// cls.netcon->qw. variables are updated
	// read the number of this frame to echo back in next input packet
	cl.qw_validsequence = cls.qw_incoming_sequence;
	newsnapindex = cl.qw_validsequence & QW_UPDATE_MASK_63;
	newsnap = d->snapshotz + newsnapindex;
	memset(newsnap, 0, sizeof(*newsnap));
	oldsnap = NULL;
	if (delta)
	{
		number = MSG_ReadByte(&cl_message);
		oldsnapindex = cl.qw_deltasequence[newsnapindex];
		if ((number & QW_UPDATE_MASK_63) != (oldsnapindex & QW_UPDATE_MASK_63))
			Con_DPrintLinef ("WARNING: from mismatch");

		if (oldsnapindex != -1) {
			if (cls.qw_outgoing_sequence - oldsnapindex >= QW_UPDATE_BACKUP_64-1) {
				Con_DPrintLinef ("delta update too old");
				newsnap->invalid = invalid = true; // too old
				delta = false;
			}
			oldsnap = d->snapshotz + (oldsnapindex & QW_UPDATE_MASK_63);
		}
		else
			delta = false;
	}

	// if we can't decode this frame properly, report that to the server
	if (invalid)
		cl.qw_validsequence = 0;

	// read entity numbers until we find a 0x0000
	// (which would be an empty update on world entity, but is actually a terminator)
	newsnap->num_entities = 0;
	oldindex = 0;
	while (1) {
		int word = (unsigned short)MSG_ReadShort(&cl_message);
		if (cl_message.badread)
			return; // just return, the main parser will print an error
		newnum = word == 0 ? 512 : (word & 511);

#if 1
		int morebits = 0;
		int evenmorebits = 0;
		if (word & QW_U_MOREBITS)
		{
			int oldpos = cl_message.readcount;
			morebits = MSG_ReadByte(&cl_message); // read the more bits
			if (morebits & QW_FTE_U_EVENMORE_S7) {
				evenmorebits = MSG_ReadByte(&cl_message); // read the even more
				if (evenmorebits & QW_FTE_U_ENTITYDBL_S5)		newnum += 512;
				if (evenmorebits & QW_FTE_U_ENTITYDBL2_S6)		newnum += 1024;
			}

			cl_message.readcount = oldpos; // undo the read... before the more bits
		}
#endif

		oldnum = delta ? (oldindex >= oldsnap->num_entities ? 9999 : oldsnap->entities[oldindex].number) : 9999;

		// copy unmodified oldsnap entities
		while (newnum > oldnum) { // delta only
			if (developer_networkentities.integer >= 2)
				Con_PrintLinef ("copy %d", oldnum);
			// copy one of the old entities
#if 1
			if (newsnap->num_entities >= QW_MAX_PACKET_ENTITIES_FTE_256) {
				Host_Error_Line ("EntityFrameQW_CL_ReadFrame: newsnap->num_entities == QW_MAX_PACKET_ENTITIES_FTE_256");
			}
#else
			if (newsnap->num_entities >= QW_MAX_PACKET_ENTITIES_64)
				Host_Error_Line ("EntityFrameQW_CL_ReadFrame: newsnap->num_entities == QW_MAX_PACKET_ENTITIES_64");
#endif
			newsnap->entities[newsnap->num_entities] = oldsnap->entities[oldindex++];
			newsnap->num_entities++;
			oldnum = oldindex >= oldsnap->num_entities ? 9999 : oldsnap->entities[oldindex].number;
		} // while newnum > oldnum

		if (word == 0)
			break;

		if (developer_networkentities.integer >= 2) {
			if (word & QW_U_REMOVE)
				Con_PrintLinef ("remove %d", newnum);
			else if (newnum == oldnum)
				Con_PrintLinef ("delta %d", newnum);
			else
				Con_PrintLinef ("baseline %d", newnum);
		}

		if (word & QW_U_REMOVE) {
			// Baker: If remove, now skip the bytes
			if (word & QW_U_MOREBITS)
				if (MSG_ReadByte(&cl_message) & QW_FTE_U_EVENMORE_S7)
					MSG_ReadByte(&cl_message);

			if (newnum != oldnum && !delta && !invalid) {
				cl.qw_validsequence = 0;
				Con_PrintLinef (CON_WARN "WARNING: U_REMOVE %d on full update", newnum);
			}
		} else {
			// This is supposed to be an update?
#if 1
			if (newsnap->num_entities >= QW_MAX_PACKET_ENTITIES_FTE_256)
				Host_Error_Line ("EntityFrameQW_CL_ReadFrame: newsnap->num_entities == QW_MAX_PACKET_ENTITIES_FTE_256");
#else
			if (newsnap->num_entities >= QW_MAX_PACKET_ENTITIES_64)
				Host_Error_Line ("EntityFrameQW_CL_ReadFrame: newsnap->num_entities == QW_MAX_PACKET_ENTITIES_64");
#endif
#if 1 // Baseline expansion
			if (newnum >= cl.max_entities) {
				//int is_delta = newnum == oldnum;
				//if (cl.num_entities <= newnum) {
					cl.num_entities = newnum + 1;
					if (cl.max_entities < newnum + 1)
						CL_ExpandEntities(newnum);
				//}

			}
#endif
			newsnap->entities[newsnap->num_entities] = (newnum == oldnum) ?
				oldsnap->entities[oldindex] : cl.entities[newnum].state_baseline;
			EntityStateQW_CL_ReadFrame_ReadEntityUpdate (newsnap->entities + newsnap->num_entities, newnum, word);
			newsnap->num_entities++;
		}

		if (newnum == oldnum)
			oldindex++;
	} // while

	// expand cl.num_entities to include every entity we've seen this game
	newnum = newsnap->num_entities ? newsnap->entities[newsnap->num_entities - 1].number : 1;
	if (cl.num_entities <= newnum) {
		cl.num_entities = newnum + 1;
		if (cl.max_entities < newnum + 1)
			CL_ExpandEntities(newnum);
	}

	// now update the non-player entities from the snapshot states
	number = cl.maxclients + 1;
	for (newindex = 0;;newindex++) {
		newnum = newindex >= newsnap->num_entities ? cl.num_entities : newsnap->entities[newindex].number;
		// kill any missing entities
		for ( ; number < newnum; number++) {
			if (cl.entities_active[number])
			{
				cl.entities_active[number] = false;
				cl.entities[number].state_current.active = ACTIVE_NOT;
			}
		} // for
		if (number >= cl.num_entities)
			break;
		// update the entity
		ent = &cl.entities[number];
		ent->state_previous = ent->state_current;
		ent->state_current = newsnap->entities[newindex];
		ent->state_current.time = cl.mtime[0];
		CL_MoveLerpEntityStates(ent);
		// the entity lives again...
		cl.entities_active[number] = true;
		number++;
	} // for
}
