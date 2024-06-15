#include "quakedef.h"
#include "protocol.h"

WARP_X_ (CL_ParseServerMessage EntityFrame5_CL_ReadFrame EntityFrameQW_CL_ReadFrame)
// Baker: Where does the origin come in?
WARP_X_ (defaultstate)
static void EntityFrame5_CL_ReadFrame_ReadUpdate (entity_state_t *s, int number)
{
	int bits;
	int startoffset = cl_message.readcount;
	int bytes = 0;
	bits = MSG_ReadByte(&cl_message);

	if (bits & E5_EXTEND1) {
		bits |= MSG_ReadByte(&cl_message) << 8;
		if (bits & E5_EXTEND2) {
			bits |= MSG_ReadByte(&cl_message) << 16;
			if (bits & E5_EXTEND3)
				bits |= MSG_ReadByte(&cl_message) << 24;
		}
	}

	if (bits & E5_FULLUPDATE) {
		*s = defaultstate; // Baker: This is a global that is initialized on "engine start"
		s->active = ACTIVE_NETWORK;
	}

	if (bits & E5_FLAGS) {
		if (Have_Zircon_Ext_Flag_CLS(ZIRCON_EXT_32BIT_RENDER_FLAGS_16)) {
			s->sflags = MSG_ReadLong(&cl_message);
			if (cl_movement_collide_nonsolid_red.value) {
				if (Have_Flag (s->sflags, RENDER_SOLID_NOT_BAKER_256))
					Flag_Add_To (s->effects, EF_RED_128);
				else 
					Flag_Remove_From (s->effects, EF_RED_128);
			}
		} else {
			s->sflags = MSG_ReadByte(&cl_message);
		}
	}

	// Baker: Server should not send E5_BBOX_S27 if CL does not support
	if (Have_Flag (bits, E5_BBOX_S27)) {
		s->bbx_mins[0] = MSG_ReadCoord13i(&cl_message);
		s->bbx_mins[1] = MSG_ReadCoord13i(&cl_message);
		s->bbx_mins[2] = MSG_ReadCoord13i(&cl_message);
		s->bbx_maxs[0] = MSG_ReadCoord13i(&cl_message);
		s->bbx_maxs[1] = MSG_ReadCoord13i(&cl_message);
		s->bbx_maxs[2] = MSG_ReadCoord13i(&cl_message);
	}

	if (bits & E5_ORIGIN) {
		if (bits & E5_ORIGIN32) {
			s->origin[0] = MSG_ReadCoord32f(&cl_message); // Baker: entity_state_t ->origin
			s->origin[1] = MSG_ReadCoord32f(&cl_message);
			s->origin[2] = MSG_ReadCoord32f(&cl_message);
		}
		else
		{
			s->origin[0] = MSG_ReadCoord13i(&cl_message);
			s->origin[1] = MSG_ReadCoord13i(&cl_message);
			s->origin[2] = MSG_ReadCoord13i(&cl_message);
		}
	}

	if (bits & E5_ANGLES) {
		if (bits & E5_ANGLES16) {
			s->angles[0] = MSG_ReadAngle16i(&cl_message);
			s->angles[1] = MSG_ReadAngle16i(&cl_message);
			s->angles[2] = MSG_ReadAngle16i(&cl_message);
		}
		else
		{
			s->angles[0] = MSG_ReadAngle8i(&cl_message);
			s->angles[1] = MSG_ReadAngle8i(&cl_message);
			s->angles[2] = MSG_ReadAngle8i(&cl_message);
		}
	}

	if (bits & E5_MODEL) {
		if (bits & E5_MODEL16)
			s->modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->modelindex = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_FRAME) {
		if (bits & E5_FRAME16)
			s->frame = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->frame = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_SKIN)
		s->skin = MSG_ReadByte(&cl_message);

	if (bits & E5_EFFECTS) {
		if (bits & E5_EFFECTS32)
			s->effects = (unsigned int) MSG_ReadLong(&cl_message);
		else if (bits & E5_EFFECTS16)
			s->effects = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->effects = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_ALPHA)
		s->alpha = MSG_ReadByte(&cl_message);

	if (bits & E5_SCALE)
		s->scale = MSG_ReadByte(&cl_message);

	if (bits & E5_COLORMAP)
		s->colormap = MSG_ReadByte(&cl_message);

	if (bits & E5_ATTACHMENT) { // TAGX
		s->tagxentity = (unsigned short) MSG_ReadShort(&cl_message);
		s->tagindex = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_LIGHT) {
		s->light[0] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[1] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[2] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[3] = (unsigned short) MSG_ReadShort(&cl_message);
		s->lightstyle = MSG_ReadByte(&cl_message);
		s->lightpflags = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_GLOW) {
		s->glowsize = MSG_ReadByte(&cl_message);
		s->glowcolor = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_COLORMOD) {
		s->colormod[0] = MSG_ReadByte(&cl_message);
		s->colormod[1] = MSG_ReadByte(&cl_message);
		s->colormod[2] = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_GLOWMOD) {
		s->glowmod[0] = MSG_ReadByte(&cl_message);
		s->glowmod[1] = MSG_ReadByte(&cl_message);
		s->glowmod[2] = MSG_ReadByte(&cl_message);
	}

	if (bits & E5_COMPLEXANIMATION) {
		skeleton_t *skeleton;
		const model_t *model;
		int modelindex;
		int type;
		int bonenum;
		int numbones;
		short pose7s[7];
		type = MSG_ReadByte(&cl_message);
		switch(type) {
		case 0:
			s->framegroupblend[0].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].fb_frame = 0;
			s->framegroupblend[2].fb_frame = 0;
			s->framegroupblend[3].fb_frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = 0;
			s->framegroupblend[2].start = 0;
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = 1;
			s->framegroupblend[1].lerp = 0;
			s->framegroupblend[2].lerp = 0;
			s->framegroupblend[3].lerp = 0;
			break;
		case 1:
			s->framegroupblend[0].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].fb_frame = 0;
			s->framegroupblend[3].fb_frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = 0;
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = 0;
			s->framegroupblend[3].lerp = 0;
			break;
		case 2:
			s->framegroupblend[0].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[3].fb_frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[3].lerp = 0;
			break;
		case 3:
			s->framegroupblend[0].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[3].fb_frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[3].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[3].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			break;
		case 4:
			if (!cl.engineskeletonobjects)
				cl.engineskeletonobjects = (skeleton_t *) Mem_Alloc(cls.levelmempool, sizeof(*cl.engineskeletonobjects) * MAX_EDICTS_32768);
			skeleton = &cl.engineskeletonobjects[number];
			modelindex = MSG_ReadShort(&cl_message);
			model = CL_GetModelByIndex(modelindex);
			numbones = MSG_ReadByte(&cl_message);
			if (model && numbones != model->num_bones)
				Host_Error_Line ("E5_COMPLEXANIMATION: model has different number of bones than network packet describes");
			if (!skeleton->relativetransforms || skeleton->model != model)
			{
				skeleton->model = model;
				skeleton->relativetransforms = (matrix4x4_t *) Mem_Realloc(cls.levelmempool, skeleton->relativetransforms, sizeof(*skeleton->relativetransforms) * numbones);
				for (bonenum = 0;bonenum < numbones;bonenum++)
					skeleton->relativetransforms[bonenum] = identitymatrix;
			}
			for (bonenum = 0;bonenum < numbones;bonenum++)
			{
				pose7s[0] = (short)MSG_ReadShort(&cl_message);
				pose7s[1] = (short)MSG_ReadShort(&cl_message);
				pose7s[2] = (short)MSG_ReadShort(&cl_message);
				pose7s[3] = (short)MSG_ReadShort(&cl_message);
				pose7s[4] = (short)MSG_ReadShort(&cl_message);
				pose7s[5] = (short)MSG_ReadShort(&cl_message);
				pose7s[6] = (short)MSG_ReadShort(&cl_message);
				Matrix4x4_FromBonePose7s(skeleton->relativetransforms + bonenum, 1.0f / 64.0f, pose7s);
			}
			s->skeletonobject = *skeleton;
			break;
		default:
			Host_Error_Line ("E5_COMPLEXANIMATION: Parse error - unknown type %d", type);
			break;
		}
	}
	if (bits & E5_TRAILEFFECTNUM)
		s->traileffectnum = (unsigned short) MSG_ReadShort(&cl_message);


	bytes = cl_message.readcount - startoffset;
	if (developer_networkentities.integer >= 2) {
		Con_Printf ("ReadFields e%d (%d bytes)", number, bytes);

		if (bits & E5_ORIGIN)
			Con_Printf (" E5_ORIGIN %f %f %f", s->origin[0], s->origin[1], s->origin[2]);
		if (bits & E5_ANGLES)
			Con_Printf (" E5_ANGLES %f %f %f", s->angles[0], s->angles[1], s->angles[2]);
		if (bits & E5_MODEL)
			Con_Printf (" E5_MODEL %d", s->modelindex);
		if (bits & E5_FRAME)
			Con_Printf (" E5_FRAME %d", s->frame);
		if (bits & E5_SKIN)
			Con_Printf (" E5_SKIN %d", s->skin);
		if (bits & E5_EFFECTS)
			Con_Printf (" E5_EFFECTS %d", s->effects);
		if (bits & E5_FLAGS) {
			Con_Printf (" E5_FLAGS %d (", s->sflags);
			if (s->sflags & RENDER_STEP)
				Con_Print(" STEP");
			if (s->sflags & RENDER_GLOWTRAIL)
				Con_Print(" GLOWTRAIL");
			if (s->sflags & RENDER_VIEWMODEL)
				Con_Print(" VIEWMODEL");
			if (s->sflags & RENDER_EXTERIORMODEL)
				Con_Print(" EXTERIORMODEL");
			if (s->sflags & RENDER_LOWPRECISION)
				Con_Print(" LOWPRECISION");
			if (s->sflags & RENDER_COLORMAPPED)
				Con_Print(" COLORMAPPED");
			if (s->sflags & RENDER_SHADOW)
				Con_Print(" SHADOW");
			if (s->sflags & RENDER_LIGHT)
				Con_Print(" LIGHT");
			if (s->sflags & RENDER_NOSELFSHADOW)
				Con_Print(" NOSELFSHADOW");
			Con_Print(")");
		}
		if (bits & E5_ALPHA)
			Con_Printf (" E5_ALPHA %f", s->alpha / 255.0f);
		if (bits & E5_SCALE)
			Con_Printf (" E5_SCALE %f", s->scale / 16.0f);
		if (bits & E5_COLORMAP)
			Con_Printf (" E5_COLORMAP %d", s->colormap);
		if (bits & E5_ATTACHMENT)
			Con_Printf (" E5_ATTACHMENT e%d:%d", s->tagxentity, s->tagindex);
		if (bits & E5_LIGHT)
			Con_Printf (" E5_LIGHT %d:%d:%d:%d %d:%d", s->light[0], s->light[1], s->light[2], s->light[3], s->lightstyle, s->lightpflags);
		if (bits & E5_GLOW)
			Con_Printf (" E5_GLOW %d:%d", s->glowsize * 4, s->glowcolor);
		if (bits & E5_COLORMOD)
			Con_Printf (" E5_COLORMOD %f:%f:%f", s->colormod[0] / 32.0f, s->colormod[1] / 32.0f, s->colormod[2] / 32.0f);
		if (bits & E5_GLOWMOD)
			Con_Printf (" E5_GLOWMOD %f:%f:%f", s->glowmod[0] / 32.0f, s->glowmod[1] / 32.0f, s->glowmod[2] / 32.0f);
		if (bits & E5_COMPLEXANIMATION)
			Con_Printf (" E5_COMPLEXANIMATION");
		if (bits & E5_TRAILEFFECTNUM)
			Con_Printf (" E5_TRAILEFFECTNUM %d", s->traileffectnum);
		Con_Print("\n");
	}
}

WARP_X_CALLERS_ (CL_ParseServerMessage)
WARP_X_			(EntityFrame5_WriteFrame svc_entities)
void EntityFrame5_CL_ReadFrame(void)
{
	int n, enumber, framenum;
	entity_t *ent;
	entity_state_t *s;
	// read the number of this frame to echo back in next input packet
	framenum = MSG_ReadLong(&cl_message);
	CL_NewFrameReceived(framenum);
	if (false == isin11 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999,
		PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,
		PROTOCOL_DARKPLACES1,	PROTOCOL_DARKPLACES2,	PROTOCOL_DARKPLACES3,
		PROTOCOL_DARKPLACES4,	PROTOCOL_DARKPLACES5,	PROTOCOL_DARKPLACES6
		)) {
		unsigned int oldsqe = cls.servermovesequence;
		cls.servermovesequence = MSG_ReadLong(&cl_message); // QUAKEWORLD, DP7  POWAR
		if (developer_qw.integer) {
			if (oldsqe == cls.servermovesequence) {
			Con_PrintLinef ("cl seq %d sv_seq %d delta %d", cl.mcmd.clx_sequence, cls.servermovesequence, cl.mcmd.clx_sequence - cls.servermovesequence);

			Con_PrintLinef ("Warp"); // PHYSICAL
			} // WARPED
		} // if dev qw
	} // if protocol
	// read entity numbers until we find a 0x8000
	// (which would be remove world entity, but is actually a terminator)
	while ((n = (unsigned short)MSG_ReadShort(&cl_message)) != 0x8000 && !cl_message.badread) {
		// get the entity number
		enumber = n & 0x7FFF;
		// we may need to expand the array
		if (cl.num_entities <= enumber) {
			cl.num_entities = enumber + 1;
			if (enumber >= cl.max_entities)
				CL_ExpandEntities(enumber);
		}
		// look up the entity
		ent = cl.entities + enumber;
		// slide the current into the previous slot
		ent->state_previous = ent->state_current;
		// read the update
		s = &ent->state_current;
		if (n & 0x8000) {
			// remove entity
			*s = defaultstate;
		} else {
			// update entity
			EntityFrame5_CL_ReadFrame_ReadUpdate(s, enumber);
		}

		// set the cl.entities_active flag
		cl.entities_active[enumber] = (s->active == ACTIVE_NETWORK);
		// set the update time
		s->time = cl.mtime[0];
		// fix the number (it gets wiped occasionally by copying from defaultstate)
		s->number = enumber;
		// check if we need to update the lerp stuff
		if (s->active == ACTIVE_NETWORK)
			CL_MoveLerpEntityStates(&cl.entities[enumber]);
		// print extra messages if desired
		if (developer_networkentities.integer >= 2 && cl.entities[enumber].state_current.active != cl.entities[enumber].state_previous.active) {
			if (cl.entities[enumber].state_current.active == ACTIVE_NETWORK)
				Con_PrintLinef ("entity #%d has become active", enumber);
			else if (cl.entities[enumber].state_previous.active)
				Con_PrintLinef ("entity #%d has become inactive", enumber);
		}
	} // while
}

