#include "quakedef.h"
#include "protocol.h"

WARP_X_ (SV_WriteEntitiesToClient EntityFrameQuake_ReadEntity)
void EntityFrameQuake_WriteFrame_FitzQuake (const entity_state_t *s, entity_state_t *pbaseline, sizebuf_t *pbuf)
{
	prvm_prog_t *prog = SVVM_prog;
	//int is_rmq	= isin1(sv.protocol, PROTOCOL_FITZQUAKE999);

	int fitz_bits = 0;

fitz_skip_1:

	// LadyHavoc: old stuff, but rewritten to have more exact tolerances
	(*pbaseline) = prog->edicts[s->number].priv.server->baseline;
	if (pbaseline->origin[0] != s->origin[0])
												fitz_bits |= U_ORIGIN1;
	if (pbaseline->origin[1] != s->origin[1])
												fitz_bits |= U_ORIGIN2;
	if (pbaseline->origin[2] != s->origin[2])
												fitz_bits |= U_ORIGIN3;
	if (pbaseline->angles[0] != s->angles[0])
												fitz_bits |= U_ANGLE1;
	if (pbaseline->angles[1] != s->angles[1])
												fitz_bits |= U_ANGLE2;
	if (pbaseline->angles[2] != s->angles[2])
												fitz_bits |= U_ANGLE3;

	if (s->sflags & RENDER_STEP)
												fitz_bits |= U_STEP;

	if (pbaseline->colormap != s->colormap)
												fitz_bits |= U_COLORMAP;
	if (pbaseline->skin != s->skin)
												fitz_bits |= U_SKIN;

	if (pbaseline->frame != s->frame)
												fitz_bits |= U_FRAME;

	// Efforts to mask for Remaster support should go here?
	if ( (pbaseline->effects ^ s->effects) & QUAKESPASM_MAX_EFFECTS_15 )
												fitz_bits |= U_EFFECTS;

	if (pbaseline->modelindex != s->modelindex)
												fitz_bits |= U_MODEL;

	// FITZQUAKE ALPHA/SCALE GO HERE

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (1 /*sv.protocol != PROTOCOL_NETQUAKE*/)
	{
#if 0
		if (pbaseline->alpha != s->alpha) bits |= U_FITZALPHA_S16;
		if (pbaseline->scale != s->scale) bits |= U_RMQ_SCALE_S20;
#endif
		if (fitz_bits & U_FRAME && (int)s->frame & 0xFF00) fitz_bits |= U_FITZFRAME2_S17;
		if (fitz_bits & U_MODEL && (int)s->modelindex & 0xFF00) fitz_bits |= U_FITZMODEL2_S18;
#if 0
		if (ent->sendinterval) bits |= U_LERPFINISH;
#endif
		if (fitz_bits >= 65536) fitz_bits |= U_EXTEND1_S15;
		if (fitz_bits >= 16777216) fitz_bits |= U_EXTEND2_S23;
	}
	//johnfitz

	if (s->number >= 256)
		fitz_bits |= U_LONGENTITY;

	if (fitz_bits >= 256)
		fitz_bits |= U_MOREBITS_1;

write_start:
	// Write #1 fitz_bits
	MSG_WriteByte (pbuf, fitz_bits | U_SIGNAL_128);
	if (fitz_bits & U_MOREBITS_1)
												MSG_WriteByte(pbuf, fitz_bits>>8);

	// Write #2 extend
	if (fitz_bits & U_EXTEND1_S15)
												MSG_WriteByte(pbuf, fitz_bits>>16);
	if (fitz_bits & U_EXTEND2_S23)
												MSG_WriteByte(pbuf, fitz_bits>>24);

	// Write #3 ent number
	if (fitz_bits & U_LONGENTITY)
												MSG_WriteShort(pbuf, s->number);
	else
												MSG_WriteByte(pbuf, s->number);

	// Write #4 model index as a byte
	if (fitz_bits & U_MODEL)
												MSG_WriteByte(pbuf, s->modelindex);

	// Write 5,6,7,8 frame, colormap, skin, effects
	if (fitz_bits & U_FRAME)
												MSG_WriteByte(pbuf, s->frame);
	if (fitz_bits & U_COLORMAP)
												MSG_WriteByte(pbuf, s->colormap);
	if (fitz_bits & U_SKIN)
												MSG_WriteByte(pbuf, s->skin);
	if (fitz_bits & U_EFFECTS)
												MSG_WriteByte(pbuf, s->effects & QUAKESPASM_MAX_EFFECTS_15);

	// Write 10, 11
	if (fitz_bits & U_ORIGIN1)
												MSG_WriteCoord(pbuf, s->origin[0], sv.protocol);
	if (fitz_bits & U_ANGLE1)
												MSG_WriteAngle(pbuf, s->angles[0], sv.protocol);
	if (fitz_bits & U_ORIGIN2)
												MSG_WriteCoord(pbuf, s->origin[1], sv.protocol);
	if (fitz_bits & U_ANGLE2)
												MSG_WriteAngle(pbuf, s->angles[1], sv.protocol);
	if (fitz_bits & U_ORIGIN3)
												MSG_WriteCoord(pbuf, s->origin[2], sv.protocol);
	if (fitz_bits & U_ANGLE3)
												MSG_WriteAngle(pbuf, s->angles[2], sv.protocol);

	// Write 12 alpha, scale, frame2, model2, lerpfinish
	// uscale must not exist for 666
	if (fitz_bits & U_FITZALPHA_S16)
												MSG_WriteByte(pbuf, s->alpha);
	if (fitz_bits & U_RMQ_SCALE_S20)
												MSG_WriteByte(pbuf, s->scale);

	if (fitz_bits & U_FITZFRAME2_S17)
												MSG_WriteByte(pbuf, (int)s->frame >> 8);
	if (fitz_bits & U_FITZMODEL2_S18)
												MSG_WriteByte(pbuf, (int)s->modelindex >> 8);
#if 000
	// Lerp
	MSG_WriteByte(msg, (byte)(Q_rint((ent->v.nextthink-sv.time)*255)));
#endif


}

qbool EntityFrameQuake_WriteFrame(sizebuf_t *msg, int maxsize, int numstates, const entity_state_t **states)
{
	prvm_prog_t *prog = SVVM_prog;
	const entity_state_t *s;
	entity_state_t baseline;
	int i, bits;
	sizebuf_t buf;
	unsigned char data[128];
	qbool success = false;
	int is_fitz2 = isin2(sv.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999);

	// prepare the buffer
	memset(&buf, 0, sizeof(buf));
	buf.data = data;
	buf.maxsize = sizeof(data);

	// Baker: numstates is nearly always 1 (attachments or exterior models increases it)
	for (i = 0;i < numstates; i++) {
		s = states[i];
		if (PRVM_serveredictfunction((&prog->edicts[s->number]), SendEntity))
			continue;

		// prepare the buffer
		SZ_Clear(&buf);

// send an update
		bits = 0;
		if (is_fitz2) {
			EntityFrameQuake_WriteFrame_FitzQuake (s, &baseline, &buf);
			goto fitzquake_bypass;
		}
		if (s->number >= 256)
													bits |= U_LONGENTITY;		// Looks ok for Fitz
		if (s->sflags & RENDER_STEP)
													bits |= U_STEP;

		if (s->sflags & RENDER_VIEWMODEL)
													bits |= U_VIEWMODEL;
		if (s->sflags & RENDER_GLOWTRAIL)
													bits |= U_GLOWTRAIL;
		if (s->sflags & RENDER_EXTERIORMODEL)
													bits |= U_EXTERIORMODEL;

		// LadyHavoc: old stuff, but rewritten to have more exact tolerances
		baseline = prog->edicts[s->number].priv.server->baseline;
		if (baseline.origin[0] != s->origin[0])
													bits |= U_ORIGIN1;
		if (baseline.origin[1] != s->origin[1])
													bits |= U_ORIGIN2;
		if (baseline.origin[2] != s->origin[2])
													bits |= U_ORIGIN3;
		if (baseline.angles[0] != s->angles[0])
													bits |= U_ANGLE1;
		if (baseline.angles[1] != s->angles[1])
													bits |= U_ANGLE2;
		if (baseline.angles[2] != s->angles[2])
													bits |= U_ANGLE3;
		if (baseline.colormap != s->colormap)
													bits |= U_COLORMAP;
		if (baseline.skin != s->skin)
													bits |= U_SKIN;

		if (baseline.frame != s->frame) {
			bits |= U_FRAME;
			if (s->frame & 0xFF00) {
				bits |= U_FRAME2_S26;
			}
		} // frame

		if (baseline.effects != s->effects) {
			// Efforts to mask for Remaster support should go here?
			bits |= U_EFFECTS;
			if (s->effects & 0xFF00)
				bits |= U_EFFECTS2_S19;
		} // effects

		if (baseline.modelindex != s->modelindex) {
			bits |= U_MODEL;
			if ((s->modelindex & 0xFF00) && false == isin3 (sv.protocol,
				PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3)) {
					bits |= U_MODEL2_S27;
			}
		} // model

		if (baseline.alpha != s->alpha)
													bits |= U_ALPHA_S17;

		if (baseline.scale != s->scale)
													bits |= U_SCALE_S18;



		if (baseline.glowsize != s->glowsize)
													bits |= U_GLOWSIZE_S20;
		if (baseline.glowcolor != s->glowcolor)
													bits |= U_GLOWCOLOR;

		if (!VectorCompare(baseline.colormod, s->colormod))
			bits |= U_COLORMOD;

		// if extensions are disabled, clear the relevant update flags
		if (isin2 (sv.protocol, PROTOCOL_QUAKE, PROTOCOL_NEHAHRAMOVIE))
			bits &= 0x7FFF;

		if (isin1 (sv.protocol, PROTOCOL_NEHAHRAMOVIE))
			if (s->alpha != 255 || s->effects & EF_FULLBRIGHT)
				bits |= U_EXTEND1_S15;

		// write the message
		if (bits >= 16777216)
													bits |= U_EXTEND2_S23;
		if (bits >= 65536)
													bits |= U_EXTEND1_S15;
		if (bits >= 256)
													bits |= U_MOREBITS_1;
		bits |= U_SIGNAL_128;

		{
			ENTITYSIZEPROFILING_START(msg, states[i]->number, bits);

			// Write #1 bits
			MSG_WriteByte (&buf, bits);
			if (bits & U_MOREBITS_1)
														MSG_WriteByte(&buf, bits>>8);

			// Write #2 extend
			if (sv.protocol != PROTOCOL_NEHAHRAMOVIE) {
				if (bits & U_EXTEND1_S15)
														MSG_WriteByte(&buf, bits>>16);
				if (bits & U_EXTEND2_S23)
														MSG_WriteByte(&buf, bits>>24);
			}

			// Write #3 ent number
			if (bits & U_LONGENTITY)
				MSG_WriteShort(&buf, s->number);
			else
				MSG_WriteByte(&buf, s->number);

			// Write #4 model index as a byte
			if (bits & U_MODEL) {
				if (isin3 (sv.protocol, PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3))
					MSG_WriteShort(&buf, s->modelindex);
				else
					MSG_WriteByte(&buf, s->modelindex);
			}

			// Write 5,6,7,8 frame, colormap, skin, effects
			if (bits & U_FRAME)
														MSG_WriteByte(&buf, s->frame);
			if (bits & U_COLORMAP)
														MSG_WriteByte(&buf, s->colormap);
			if (bits & U_SKIN)
														MSG_WriteByte(&buf, s->skin);
			if (bits & U_EFFECTS)
														MSG_WriteByte(&buf, s->effects);

			// Write 10, 11
			if (bits & U_ORIGIN1)
														MSG_WriteCoord(&buf, s->origin[0], sv.protocol);
			if (bits & U_ANGLE1)
														MSG_WriteAngle(&buf, s->angles[0], sv.protocol);
			if (bits & U_ORIGIN2)
														MSG_WriteCoord(&buf, s->origin[1], sv.protocol);
			if (bits & U_ANGLE2)
														MSG_WriteAngle(&buf, s->angles[1], sv.protocol);
			if (bits & U_ORIGIN3)
														MSG_WriteCoord(&buf, s->origin[2], sv.protocol);
			if (bits & U_ANGLE3)
														MSG_WriteAngle(&buf, s->angles[2], sv.protocol);

			// Write 12 alpha, scale, frame2, model2, lerpfinish
			if (bits & U_ALPHA_S17)
														MSG_WriteByte(&buf, s->alpha);
			if (bits & U_SCALE_S18)
														MSG_WriteByte(&buf, s->scale);
			if (bits & U_EFFECTS2_S19)
														MSG_WriteByte(&buf, s->effects >> 8);
			if (bits & U_GLOWSIZE_S20)
														MSG_WriteByte(&buf, s->glowsize);
			if (bits & U_GLOWCOLOR)
														MSG_WriteByte(&buf, s->glowcolor);
			if (bits & U_COLORMOD)				{
				int c = ((int)bound(0, s->colormod[0] * (7.0f / 32.0f), 7) << 5) | ((int)bound(0, s->colormod[1] * (7.0f / 32.0f), 7) << 2) | ((int)bound(0, s->colormod[2] * (3.0f / 32.0f), 3) << 0);
														MSG_WriteByte(&buf, c);
			}

			if (bits & U_FRAME2_S26)
														MSG_WriteByte(&buf, s->frame >> 8);
			if (bits & U_MODEL2_S27)
														MSG_WriteByte(&buf, s->modelindex >> 8);

			// the nasty protocol
			if (Have_Flag(bits, U_EXTEND1_S15) && sv.protocol == PROTOCOL_NEHAHRAMOVIE) {
				// PROTOCOL_NEHAHRAMOVIE
				if (Have_Flag (s->effects, EF_FULLBRIGHT)) {
					MSG_WriteFloat(&buf, 2); // QSG protocol version
					MSG_WriteFloat(&buf, s->alpha <= 0 ? 0 : (s->alpha >= 255 ? 1 : s->alpha * (1.0f / 255.0f))); // alpha
					MSG_WriteFloat(&buf, 1); // fullbright
				} else {
					MSG_WriteFloat(&buf, 1); // QSG protocol version
					MSG_WriteFloat(&buf, s->alpha <= 0 ? 0 : (s->alpha >= 255 ? 1 : s->alpha * (1.0f / 255.0f))); // alpha
				}
			}
fitzquake_bypass:
			// if the commit is full, we're done this frame
			if (msg->cursize + buf.cursize > maxsize) {
				// next frame we will continue where we left off
				break;
			}
			// write the message to the packet
			SZ_Write(msg, buf.data, buf.cursize);
			success = true;

			if (is_fitz2 == false)
				ENTITYSIZEPROFILING_END(msg, s->number, bits);
		} // block
	} // for i < numstates
	return success;
}
