
#ifndef CL_COLLISION_H
#define CL_COLLISION_H

float CL_SelectTraceLine(const vec3_t start, const vec3_t end, vec3_t impact, vec3_t normal, int *hitent, entity_render_t *ignoreent);
void CL_FindNonSolidLocation(const vec3_t in, vec3_t out, vec_t radius);

model_t *CL_GetModelByIndex(int modelindex);
model_t *CL_GetModelFromEdict(prvm_edict_t *ed);

void CL_LinkEdict(prvm_edict_t *ent);
int CL_GenericHitSuperContentsMask(const prvm_edict_t *edict);

// Baker: Alternate collision prediction
#define HITT_NOPLAYERS_0_NOTAMOVE				0
#define HITT_NOPLAYERS_0						0
#define HITT_PLAYERS_1							1
#define HITT_PLAYERS_1_NOTAMOVE					1
#define HITT_PLAYERS_PLUS_SOLIDS_2				2	// Zircon
#define HITT_PLAYERS_PLUS_ONLY_MONSTERS_QW_3	3	// Quakeworld
#define HITT_PLAYERS_DONT_HIT_PLAYERS_FLAG_4	4	// 
#define HITT_PLAYERS_PLUS_SOLIDS_NO_PLAYERS_6	6	// Walk through players option

trace_t CL_TraceBox(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, int hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities);
trace_t CL_TraceLine(const vec3_t start, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, int hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities, qbool hitsurfaces);
trace_t CL_TracePoint(const vec3_t start, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, qbool hitnetworkbrushmodels, int hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities);
trace_t CL_Cache_TraceLineSurfaces(const vec3_t start, const vec3_t end, int type, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
#define CL_PointSuperContents(point) (CL_TracePoint((point), sv_gameplayfix_swiminbmodels.integer ? MOVE_NOMONSTERS_1 : MOVE_WORLDONLY_3, NULL, 0, 0, 0, true, HITT_NOPLAYERS_0_NOTAMOVE, NULL, false).startsupercontents)

#endif // ! CL_COLLISION_H
