/*
Copyright (C) 2006-2021 DarkPlaces contributors

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

#ifndef CSPROGS_H
#define CSPROGS_H

// LadyHavoc: changed to match MAX_EDICTS_32768
#define CL_MAX_EDICTS MAX_EDICTS_32768

#define ENTMASK_ENGINE				1
#define ENTMASK_ENGINEVIEWMODELS	2
#define ENTMASK_NORMAL				4

#define VF_MIN			1	//(vector)
#define VF_MIN_X		2	//(float)
#define VF_MIN_Y		3	//(float)
#define VF_SIZE			4	//(vector) (viewport size)
#define VF_SIZE_X		5	//(float)
#define VF_SIZE_Y		6	//(float)
#define VF_VIEWPORT		7	//(vector, vector)
#define VF_FOV			8	//(vector)
#define VF_FOVX			9	//(float)
#define VF_FOVY			10	//(float)
#define VF_ORIGIN		11	//(vector)
#define VF_ORIGIN_X		12	//(float)
#define VF_ORIGIN_Y		13	//(float)
#define VF_ORIGIN_Z		14	//(float)
#define VF_ANGLES		15	//(vector)
#define VF_ANGLES_X		16	//(float)
#define VF_ANGLES_Y		17	//(float)
#define VF_ANGLES_Z		18	//(float)

#define VF_DRAWWORLD		19	//(float)	//actually world model and sky
#define VF_DRAWENGINESBAR	20	//(float)
#define VF_DRAWCROSSHAIR	21	//(float)

#define VF_CL_VIEWANGLES	33	//(vector)	//sweet thing for RPGs/...
#define VF_CL_VIEWANGLES_X	34	//(float)
#define VF_CL_VIEWANGLES_Y	35	//(float)
#define VF_CL_VIEWANGLES_Z	36	//(float)

// FTEQW's extension range
#define VF_PERSPECTIVE		200 //(float)

// what is this doing here? This is a DP extension introduced by Black, should be in 4xx range
#define VF_CLEARSCREEN		201 //(float)

// what is this doing here? This is a DP extension introduced by VorteX, should be in 4xx range
#define VF_FOG_DENSITY		202 //(float)
#define VF_FOG_COLOR		203 //(vector)
#define VF_FOG_COLOR_R		204 //(float)
#define VF_FOG_COLOR_G		205 //(float)
#define VF_FOG_COLOR_B		206 //(float)
#define VF_FOG_ALPHA		207 //(float)
#define VF_FOG_START		208 //(float)
#define VF_FOG_END   		209 //(float)
#define VF_FOG_HEIGHT		210 //(float)
#define VF_FOG_FADEDEPTH	211 //(float)

// DP's extension range
#define VF_MAINVIEW		400 //(float)
#define VF_MINFPS_QUALITY	401 //(float)

#define RF_VIEWMODEL		1	// The entity is never drawn in mirrors. In engines with realtime lighting, it casts no shadows.
#define RF_EXTERNALMODEL	2	// The entity is appears in mirrors but not in the normal view. It does still cast shadows in engines with realtime lighting.
#define RF_DEPTHHACK		4	// The entity appears closer to the view than normal, either by scaling it wierdly or by just using a depthrange. This will usually be found in conjunction with RF_VIEWMODEL
#define RF_ADDITIVE			8	// Add the entity acording to it's alpha values instead of the normal blend
#define RF_USEAXIS			16	// When set, the entity will use the v_forward, v_right and v_up globals instead of it's angles field for orientation. Angles will be ignored compleatly.
								// Note that to use this properly, you'll NEED to use the predraw function to set the globals.
//#define RF_DOUBLESIDED		32
#define RF_USETRANSPARENTOFFSET 64   // Allows QC to customize origin used for transparent sorting via transparent_origin global, helps to fix transparent sorting bugs on a very large entities
#define RF_WORLDOBJECT          128  // for large outdoor entities that should not be culled
#define RF_FULLBRIGHT			256
#define RF_NOSHADOW				512
#define RF_MODELLIGHT           4096 // CSQC-set model light
#define RF_DYNAMICMODELLIGHT    8192 // origin-dependent model light


#define MONSTER_QW_UNKNOWN_NEG_2	-2	// Attempted to identify and failed.

#define MONSTER_QW_ARMY_0			0
#define MONSTER_QW_DOG_1			1
#define MONSTER_QW_DEMON_2			2
#define MONSTER_QW_OGRE_3			3
#define MONSTER_QW_SHAMBLER_4		4
#define MONSTER_QW_KNIGHT_5			5
#define MONSTER_QW_ZOMBIE_6			6
#define MONSTER_QW_WIZARD_7			7
#define MONSTER_QW_ENFORCER_8		8
#define MONSTER_QW_FISH_9			9
#define MONSTER_QW_HELL_KNIGHT_10	10
#define MONSTER_QW_SHALRATH_11		11
#define MONSTER_QW_TARBABY_12		12

// Baker: Made similar to JoeQuake and ezQuake stuff ...
//cl_modelnames[mi_monster1] = "progs/soldier.mdl";
//cl_modelnames[mi_m2] = "progs/dog.mdl";
//cl_modelnames[mi_m3] = "progs/demon.mdl";
//cl_modelnames[mi_m4] = "progs/ogre.mdl";
//cl_modelnames[mi_m5] = "progs/shambler.mdl";
//cl_modelnames[mi_m6] = "progs/knight.mdl";
//cl_modelnames[mi_m7] = "progs/zombie.mdl";
//cl_modelnames[mi_m8] = "progs/wizard.mdl";
//cl_modelnames[mi_m9] = "progs/enforcer.mdl";
//cl_modelnames[mi_m10] = "progs/fish.mdl";
//cl_modelnames[mi_m11] = "progs/hknight.mdl";
//cl_modelnames[mi_m12] = "progs/shalrath.mdl";
//cl_modelnames[mi_m13] = "progs/tarbaby";


#define MONSTER_QW_255_NOT_FOUND	255



WARP_X_ (RENDER_STEP)

#define MONSTER_INFO_PLAYER_1			1
#define MONSTER_INFO_UNKNOWN_MONSTER_2	2
#define MONSTER_INFO_ARMY_10			10
#define MONSTER_INFO_BOSS_11			11
#define MONSTER_INFO_DEMON_12			12
#define MONSTER_INFO_DOG_13				13
#define MONSTER_INFO_ENFORCER_14		14
#define MONSTER_INFO_FISH_15			15
#define MONSTER_INFO_HELL_KNIGHT_16		16
#define MONSTER_INFO_KNIGHT_17			17
#define MONSTER_INFO_OGRE_18			18
#define MONSTER_INFO_OLDONE_19			19
#define MONSTER_INFO_SHALRATH_20		20
#define MONSTER_INFO_SHAMBLER_21		21
#define MONSTER_INFO_TARBABY_22			22
#define MONSTER_INFO_WIZARD_23			23
#define MONSTER_INFO_ZOMBIE_24			24

// Baker: DarkPlaces supports 0 to 16 (in theory)
#define GE_HEALTH_500					500		// if (is_monster) Server 
#define GE_MAX_HEALTH_501				501		// if (is_monster)
#define GE_MODELNAME_502				502		// Baker: Take modelindex .. look in cl precache
#define GE_MONSTERTYPEHACK_503			503		// Baker: Take modelindex .. look in cl precache
#define GE_RENDERFLAGS_504				504		// Baker: Take modelindex .. look in cl precache
#define GE_MONSTERTYPEQW_505			505		// Baker: Take modelindex .. look in cl precache
#define GE_ENTITYBOX2D_506				506		// Returns 4 corners of model 2D projection 
												// based on mins/maxs.

// Return values is maxs (BOTTOM RIGHT), (LEFT TOP) mins is returned in v_up (UP = TOP ... get it?)



#define GE_MONSTERCHAINFIND_512			512		// Baker: Finds chain of monsters that are active.  Returns first.

#define	GE_MONSTERCHAIN_OPEN_513		513		// Limit 256, sorts by zdepth.
// float count = getentity(NULL, GE_MONSTERCHAIN_OPEN_513);
// Finds all ACTIVE monsters (RENDER_STEP) and Z depth sorts them.
// Returns count

#define	GE_MONSTERCHAIN_GET_514			514		// Limit 256, sorts by zdepth.
// for (float idx; idx < count; idx ++) {
//     float entnum = getentity(idx, GE_MONSTERCHAIN_GET_514);
//		// GE_MONSTERCHAIN_GET_514 sets
//		// v_forward	= origin
//		// v_right		= 2d origin
//		// v_up			= 2d origin above the monster (origin + bbox->maxz)
//		// trace_dphitconstants	= QW monster type id
//									 MONSTER_QW_UNKNOWN_NEG_2 (identification failed)
//									 MONSTER_QW_SHAMBLER_4
// }
// reply is entity number
// sets: v_forward = origin
// sets: 

// PP is .map pre-processor
#define	GE_PP_SECRETCHAIN_OPEN_520		520		// Limit 256, sorts by zdepth.
// float num_secrets_in_bsp_ent = getentity(NULL, GE_PP_SECRETCHAIN_OPEN_520);
// Finds all "trigger_secret" in .bsp entities segment. Q1/Q3.
// Returns number found.

#define	GE_PP_SECRETCHAIN_GET_521		521		// Limit 256, sorts by zdepth.
// Returns 10 if the model was *10, 50 is the model was *50.
// for (float idx; idx < num_secrets_in_bsp_ent; idx ++) {
//     float star_brush_number = getentity(idx, GE_PP_SECRETCHAIN_GET_521);
//		// v_forward	= origin -- center of the "trigger_secret" brush.
//		// v_up			= size 3d
// }

#define GE_MAXENTS_NEG1				-1	/* Valid for getentity, ignores the entity argument. Returns the maximum number of entities which may be valid, to avoid having to poll 65k when only 100 are used. */
#define GE_ACTIVE_0					0	/* Valid for getentity. Returns whether this entity is known to the client or not. */
#define GE_ORIGIN_1					1	/* Valid for getentity. Returns the interpolated .origin. */
#define GE_FORWARD_2				2	/* Valid for getentity. Returns the interpolated forward vector. */
#define GE_RIGHT_3					3	/* Valid for getentity. Returns the entity's right vector. */
#define GE_UP_4						4	/* Valid for getentity. Returns the entity's up vector. */
#define GE_SCALE_5					5	/* Valid for getentity. Returns the entity .scale. */
#define GE_ORIGIN5ANDVECTORS_6  	6	/* Valid for getentity. Returns interpolated .origin, but also sets v_forward, v_right, and v_up accordingly. Use vectoangles(v_forward,v_up) to determine the angles. */
#define GE_ALPHA_7					7	/* Valid for getentity. Returns the entity alpha. */
#define GE_COLORMOD_8				8	/* Valid for getentity. Returns the colormod vector. */
#define GE_PANTSCOLOR_9				9	/* Valid for getentity. Returns the entity's lower color (from .colormap), as a palette range value. */
#define GE_SHIRTCOLOR_10			10	/* Valid for getentity. Returns the entity's lower color (from .colormap), as a palette range value. */
#define GE_SKIN_11					11	/* Valid for getentity. Returns the entity's .skin index. */
#define GE_MINS_12					12	/* Valid for getentity. Guesses the entity's .min vector. */
#define GE_MAXS_13					13	/* Valid for getentity. Guesses the entity's .max vector. */
#define GE_ABSMIN_14				14	/* Valid for getentity. Guesses the entity's .absmin vector. */
#define GE_ABSMAX_15				15	/* Valid for getentity. Guesses the entity's .absmax vector. */

#define GE_MODELLIGHT_AMBIENT_16	16	// FTE no have 16

#define GE_MODELINDEX_200			200	/*ZIRCON*/ /* Valid for getentity. Guesses the entity's .modelindex float. */
#define GE_MODELINDEX2_201			201	/*I think is a QW thing*/ /* Valid for getentity. Guesses the entity's .vw_index float. */
#define GE_EFFECTS_202				202	/*ZIRCON*/ /* Valid for getentity. Guesses the entity's .effects float. */
#define GE_FRAME_203				203	/*ZIRCON*/ /* Valid for getentity. Guesses the entity's .frame float. */
#define GE_ANGLES_204				204	/*ZIRCON*/ /* Valid for getentity. Guesses the entity's .angles vector. */
#define GE_FATNESS_205				205	/*FTE FEATURE*/ /* Valid for getentity. Guesses the entity's .fatness float. */ // Baker: weight
#define GE_DRAWFLAGS_206			206	/*FTE HEXEN2 ?*/ /* Valid for getentity. Guesses the entity's .drawflags float. */
#define GE_ABSLIGHT_207				207	/*NEED TO SEE WHAT THIS IS*/ /* Valid for getentity. Guesses the entity's .abslight float. */

// Baker: glowmod is a dynamic light ... and a byte palette one, right?
#define GE_GLOWMOD_208				208	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .glowmod vector. */
#define GE_GLOWSIZE_209				209	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .glowsize float. */
#define GE_GLOWCOLOUR_210			210	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .glowcolor float. */

#define GE_RTSTYLE_211				211	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .style float. */
#define GE_RTPFLAGS_212				212	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .pflags float. */
#define GE_RTCOLOUR_213				213	/*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .color vector. */
#define GE_RTRADIUS_214				214 /*DOABLE LOW PRIORITY*/ /* Valid for getentity. Guesses the entity's .light_lev float. */

#define GE_TAGENTITY_215			215 /*PROBABLY POSSIBLE*/ /* Valid for getentity. Guesses the entity's .tag_entity float. */
#define GE_TAGINDEX_216				216 /*PROBABLY POSSIBLE*/ /* Valid for getentity. Guesses the entity's .tag_index float. */

#define GE_GRAVITYDIR_217			217	/*FTE FEATURE*/ /* Valid for getentity. Guesses the entity's .gravitydir vector. */
#define GE_TRAILEFFECTNUM_218		218	/*PROBABLY POSSIBLE*/ /* Valid for getentity. Guesses the entity's .traileffectnum float. */


extern cvar_t csqc_enable;		// Baker r0101:  csqc_enable
extern cvar_t csqc_progname;	//[515]: csqc crc check and right csprogs name according to progs.dat
extern cvar_t csqc_progcrc;
extern cvar_t csqc_progsize;
extern cvar_t csqc_polygons_defaultmaterial_nocullface;
extern cvar_t csqc_polygons_darkplaces_classic_3d;


void CL_VM_PreventInformationLeaks(void);

qbool MakeDownloadPacket(const char *filename, unsigned char *data, size_t len, int crc, int cnt, sizebuf_t *buf, int protocol);

qbool CL_VM_GetEntitySoundOrigin(int entnum, vec3_t out);

qbool CL_VM_TransformView(int entnum, matrix4x4_t *viewmatrix, mplane_t *clipplane, vec3_t visorigin);

void CL_VM_Init(void);
void CL_VM_ShutDown(void);
void CL_VM_UpdateIntermissionState(int intermission);
void CL_VM_UpdateShowingScoresState(int showingscores);
qbool CL_VM_InputEvent(int eventtype, float x, float y);
qbool CL_VM_ConsoleCommand(const char *text);
void CL_VM_UpdateDmgGlobals(int dmg_take, int dmg_save, vec3_t dmg_origin);
void CL_VM_UpdateIntermissionState(int intermission);
qbool CL_VM_Event_Sound(int sound_num, float volume, int channel, float attenuation, int ent, vec3_t pos, int flags, float speed);
qbool CL_VM_Parse_TempEntity(void);
void CL_VM_Parse_StuffCmd (const char *msg, int is_qw);
void CL_VM_Parse_CenterPrint(const char *msg);
int CL_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent);
int CL_GetTagMatrix(prvm_prog_t *prog, matrix4x4_t *out, prvm_edict_t *ent, int tagindex, prvm_vec_t *shadingorigin);
void CL_GetEntityMatrix(prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, qbool viewmatrix);

#if 123
/* VMs exposing the polygon calls must call this on Init/Reset */
void VM_Polygons_Reset(prvm_prog_t *prog);
#endif // 123

void QW_CL_StartUpload(unsigned char *data, int size);

void CSQC_UpdateNetworkTimes(double newtime, double oldtime);
void CSQC_AddPrintText(const char *msg);
void CSQC_AddPrintTextQWColor (const char *msg);
void CSQC_ReadEntities(void);
void CSQC_RelinkAllEntities(int drawmask);
void CSQC_RelinkCSQCEntities(void);
void CSQC_Predraw(prvm_edict_t *ed);
void CSQC_Think(prvm_edict_t *ed);
qbool CSQC_AddRenderEdict(prvm_edict_t *ed, int edictnum);//csprogs.c
void CSQC_R_RecalcView(void);

model_t *CL_GetModelByIndex(int modelindex);

int CL_VM_GetViewEntity(void);

void SBar2D_PolygonBegin (prvm_prog_t *prog, const char *texname, float drawflags, float isdraw2d);
void SBar2D_PolygonEnd (prvm_prog_t *prog);
void SBar2D_PolygonVertex (prvm_prog_t *prog, float x, float y, float z, float tx, 
						   float ty, float tz, float red, float green, float blue, float alpha);



#endif
