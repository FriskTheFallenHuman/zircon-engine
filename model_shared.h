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

#ifndef MODEL_SHARED_H
#define MODEL_SHARED_H

#include <stddef.h>
#include "qdefs.h"
#include "bspfile.h"
#include "r_qshader.h"
#include "matrixlib.h"
struct rtexture_s;
struct mempool_s;
struct skeleton_s;
struct skinframe_s;

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

typedef enum modtype_e {mod_invalid, mod_brushq1, mod_sprite, mod_alias, mod_brushq2, mod_brushq3, mod_obj, mod_null} modtype_t;

typedef struct animscene_s
{
	char name[32]; // for viewthing support
	int firstframe;
	int framecount;
	int loop; // true or false
	float framerate;
}
animscene_t;

struct md3vertex_s;
struct trivertx_s;
typedef struct texvecvertex_s
{
	signed char svec[3];
	signed char tvec[3];
}
texvecvertex_t;

typedef struct blendweights_s
{
	unsigned char index[4];
	unsigned char influence[4];
}
blendweights_t;

typedef struct r_vertexgeneric_s
{
	// 36 bytes
	float vertex3f[3];
	float color4f[4];
	float texcoord2f[2];
}
r_vertexgeneric_t;

typedef struct r_vertexmesh_s
{
	// 88 bytes
	float vertex3f[3];
	float color4f[4];
	float texcoordtexture2f[2];
	float texcoordlightmap2f[2];
	float svector3f[3];
	float tvector3f[3];
	float normal3f[3];
	unsigned char skeletalindex4ub[4];
	unsigned char skeletalweight4ub[4];
}
r_vertexmesh_t;

typedef struct r_meshbuffer_s
{
	int bufferobject; // OpenGL
	void *devicebuffer; // Direct3D
	size_t size;
	qbool isindexbuffer;
	qbool isuniformbuffer;
	qbool isdynamic;
	qbool isindex16;
	char name[MAX_QPATH];
}
r_meshbuffer_t;

// used for mesh lists in q1bsp/q3bsp map models
// (the surfaces reference portions of these meshes)
typedef struct surfmesh_s
{
	// triangle data in system memory
	int num_triangles; // number of triangles in the mesh
	int *data_element3i; // int[tris*3] triangles of the mesh, 3 indices into vertex arrays for each
	r_meshbuffer_t *data_element3i_indexbuffer;
	int data_element3i_bufferoffset;
	unsigned short *data_element3s; // unsigned short[tris*3] triangles of the mesh in unsigned short format (NULL if num_vertices > 65536)
	r_meshbuffer_t *data_element3s_indexbuffer;
	int data_element3s_bufferoffset;
	int *data_neighbor3i; // int[tris*3] neighboring triangle on each edge (-1 if none)
	// vertex data in system memory
	int num_vertices; // number of vertices in the mesh
	float *data_vertex3f; // float[verts*3] vertex locations
	float *data_svector3f; // float[verts*3] direction of 'S' (right) texture axis for each vertex
	float *data_tvector3f; // float[verts*3] direction of 'T' (down) texture axis for each vertex
	float *data_normal3f; // float[verts*3] direction of 'R' (out) texture axis for each vertex
	float *data_texcoordtexture2f; // float[verts*2] texcoords for surface texture
	float *data_texcoordlightmap2f; // float[verts*2] texcoords for lightmap texture
	float *data_lightmapcolor4f;
	unsigned char *data_skeletalindex4ub;
	unsigned char *data_skeletalweight4ub;
	int *data_lightmapoffsets; // index into surface's lightmap samples for vertex lighting
	r_vertexmesh_t *data_vertexmesh; // interleaved arrays for D3D
	// vertex buffer object (stores geometry in video memory)
	r_meshbuffer_t *vbo_vertexbuffer;
	int vbooffset_vertex3f;
	int vbooffset_svector3f;
	int vbooffset_tvector3f;
	int vbooffset_normal3f;
	int vbooffset_texcoordtexture2f;
	int vbooffset_texcoordlightmap2f;
	int vbooffset_lightmapcolor4f;
	int vbooffset_skeletalindex4ub;
	int vbooffset_skeletalweight4ub;
	int vbooffset_vertexmesh;
	// morph blending, these are zero if model is skeletal or static
	int num_morphframes;
	struct md3vertex_s *data_morphmd3vertex;
	struct trivertx_s *data_morphmdlvertex;
	struct texvecvertex_s *data_morphtexvecvertex;
	float *data_morphmd2framesize6f;
	float num_morphmdlframescale[3];
	float num_morphmdlframetranslate[3];
	// skeletal blending, these are NULL if model is morph or static
	struct blendweights_s *data_blendweights;
	int num_blends;
	unsigned short *blends;
	// set if there is some kind of animation on this model
	qbool isanimated;

	// vertex and index buffers for rendering
	r_meshbuffer_t *vertexmesh_vertexbuffer;
}
surfmesh_t;

#define SHADOWMESHVERTEXHASH 1024
typedef struct shadowmeshvertexhash_s
{
	struct shadowmeshvertexhash_s *next;
}
shadowmeshvertexhash_t;

typedef struct shadowmesh_s
{
	// next mesh in chain
	struct shadowmesh_s *next;
	// used for light mesh (NULL on shadow mesh)
	rtexture_t *map_diffuse;
	rtexture_t *map_specular;
	rtexture_t *map_normal;
	// buffer sizes
	int numverts, maxverts;
	int numtriangles, maxtriangles;
	// used always
	float *vertex3f;
	// used for light mesh (NULL on shadow mesh)
	float *svector3f;
	float *tvector3f;
	float *normal3f;
	float *texcoord2f;
	// used always
	int *element3i;
	r_meshbuffer_t *element3i_indexbuffer;
	int element3i_bufferoffset;
	unsigned short *element3s;
	r_meshbuffer_t *element3s_indexbuffer;
	int element3s_bufferoffset;
	// vertex/index buffers for rendering
	// (created by Mod_ShadowMesh_Finish if possible)
	r_vertexmesh_t *vertexmesh; // usually NULL
	// used for shadow mapping cubemap side partitioning
	int sideoffsets[6], sidetotals[6];
	// used for shadow mesh (NULL on light mesh)
	int *neighbor3i;
	// these are NULL after Mod_ShadowMesh_Finish is performed, only used
	// while building meshes
	shadowmeshvertexhash_t **vertexhashtable, *vertexhashentries;
	r_meshbuffer_t *vbo_vertexbuffer;
	int vbooffset_vertex3f;
	int vbooffset_svector3f;
	int vbooffset_tvector3f;
	int vbooffset_normal3f;
	int vbooffset_texcoord2f;
	int vbooffset_vertexmesh;
}
shadowmesh_t;



typedef enum texturelayertype_e
{
	TEXTURELAYERTYPE_INVALID,
	TEXTURELAYERTYPE_LITTEXTURE,
	TEXTURELAYERTYPE_TEXTURE,
	TEXTURELAYERTYPE_FOG
}
texturelayertype_t;

typedef struct texturelayer_s
{
	texturelayertype_t type;
	qbool depthmask;
	int blendfunc1;
	int blendfunc2;
	rtexture_t *texture;
	matrix4x4_t texmatrix;
	vec4_t color;
}
texturelayer_t;

typedef struct texture_s
{
	// q1bsp
	// name
	//char name[16];
	// size
	unsigned int width, height;
	// SURF_ flags
	//unsigned int flags;

	// base material flags
	int basematerialflags;
	// current material flags (updated each bmodel render)
	int currentmaterialflags;
	// base material alpha (used for Q2 materials)
	float basealpha;

	// PolygonOffset values for rendering this material
	// (these are added to the r_refdef values and submodel values)
	float biaspolygonfactor;
	float biaspolygonoffset;

	// textures to use when rendering this material
	skinframe_t *currentskinframe;
	int numskinframes;
	float skinframerate;
	skinframe_t *skinframes[TEXTURE_MAXFRAMES_64];
	// background layer (for terrain texture blending)
	skinframe_t *backgroundcurrentskinframe;
	int backgroundnumskinframes;
	float backgroundskinframerate;
	skinframe_t *backgroundskinframes[TEXTURE_MAXFRAMES_64];

	// total frames in sequence and alternate sequence
	int anim_total[2];
	// direct pointers to each of the frames in the sequences
	// (indexed as [alternate][frame])
	struct texture_s *anim_frames[2][10];
	// 1 = q1bsp animation with anim_total[0] >= 2 (animated) or anim_total[1] >= 1 (alternate frame set)
	// 2 = q2bsp animation with anim_total[0] >= 2 (uses self.frame)
	int animated;

	// renderer checks if this texture needs updating...
	int update_lastrenderframe;
	void *update_lastrenderentity;
	// the current alpha of this texture (may be affected by r_wateralpha, also basealpha, and ent->alpha)
	float currentalpha;
	// the current texture frame in animation
	struct texture_s *currentframe;
	// current texture transform matrix (used for water scrolling)
	matrix4x4_t currenttexmatrix;
	matrix4x4_t currentbackgroundtexmatrix;

	// various q3 shader features
	q3shaderinfo_layer_rgbgen_t rgbgen;
	q3shaderinfo_layer_alphagen_t alphagen;
	q3shaderinfo_layer_tcgen_t tcgen;
	q3shaderinfo_layer_tcmod_t tcmods[Q3MAXTCMODS_8];
	q3shaderinfo_layer_tcmod_t backgroundtcmods[Q3MAXTCMODS_8];
	q3shaderinfo_deform_t deforms[Q3MAXDEFORMS_4];

	qbool colormapping;
	rtexture_t *basetexture; // original texture without pants/shirt/glow
	rtexture_t *pantstexture; // pants only (in greyscale)
	rtexture_t *shirttexture; // shirt only (in greyscale)
	rtexture_t *nmaptexture; // normalmap (bumpmap for dot3)
	rtexture_t *glosstexture; // glossmap (for dot3)
	rtexture_t *glowtexture; // glow only (fullbrights)
	rtexture_t *fogtexture; // alpha of the base texture (if not opaque)
	rtexture_t *reflectmasktexture; // mask for fake reflections
	rtexture_t *reflectcubetexture; // fake reflections cubemap
	rtexture_t *backgroundbasetexture; // original texture without pants/shirt/glow
	rtexture_t *backgroundnmaptexture; // normalmap (bumpmap for dot3)
	rtexture_t *backgroundglosstexture; // glossmap (for dot3)
	rtexture_t *backgroundglowtexture; // glow only (fullbrights)
	float specularscale;
	float specularpower;
	// color tint (colormod * currentalpha) used for rtlighting this material
	float dlightcolor[3];
	// color tint (colormod * 2) used for lightmapped lighting on this material
	// includes alpha as 4th component
	// replaces role of gl_Color in GLSL shader
	float lightmapcolor[4];

	// from q3 shaders
	int customblendfunc[2];

	int currentnumlayers;
	texturelayer_t currentlayers[16];

	// q3bsp
	char name[64];
	int surfaceflags;
	int supercontents;
	int textureflags;

	// q2bsp
	// we have to load the texture multiple times when Q2SURF_ flags differ,
	// though it still shares the skinframe
	int q2flags;
	int q2value;
	int q2contents;

	// reflection
	float reflectmin; // when refraction is used, minimum amount of reflection (when looking straight down)
	float reflectmax; // when refraction is used, maximum amount of reflection (when looking parallel to water)
	float refractfactor; // amount of refraction distort (1.0 = like the cvar specifies)
	vec4_t refractcolor4f; // color tint of refraction (including alpha factor)
	float reflectfactor; // amount of reflection distort (1.0 = like the cvar specifies)
	vec4_t reflectcolor4f; // color tint of reflection (including alpha factor)
	float r_water_wateralpha; // additional wateralpha to apply when r_water is active
	float r_water_waterscroll[2]; // scale and speed
	int tcamera_entity; // entity number for use by cameras

	// offsetmapping
	dpoffsetmapping_technique_t offsetmapping;
	float offsetscale;
	float offsetbias;

	// transparent sort category
	dptransparentsortcategory_t transparentsort;

	// gloss
	float specularscalemod;
	float specularpowermod;

	// diffuse and ambient
	float rtlightambient;
}
 texture_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			textureindex;
	int			q1flags;
	int			q2flags;			// miptex flags + overrides
	int			q2value;			// light emission, etc
	char		q2texture[32];	// texture name (textures/*.wal)
	int			q2nexttexinfo;	// for animations, -1 = end of chain
}
mtexinfo_t;

typedef struct msurface_lightmapinfo_s
{
	// texture mapping properties used by this surface
	mtexinfo_t *texinfo; // q1bsp
	// index into r_refdef.scene.lightstylevalue array, 255 means not used (black)
	unsigned char styles[MAXLIGHTMAPS]; // q1bsp
	// RGB lighting data [numstyles][height][width][3]
	unsigned char *samples; // q1bsp
	// RGB normalmap data [numstyles][height][width][3]
	unsigned char *nmapsamples; // q1bsp
	// stain to apply on lightmap (soot/dirt/blood/whatever)
	unsigned char *stainsamples; // q1bsp
	int texturemins[2]; // q1bsp
	int extents[2]; // q1bsp
	int lightmaporigin[2]; // q1bsp
}
msurface_lightmapinfo_t;

struct q3deffect_s;
typedef struct msurface_s
{
	// bounding box for onscreen checks
	vec3_t mins;
	vec3_t maxs;
	// the texture to use on the surface
	texture_t *texture;
	// the lightmap texture fragment to use on the rendering mesh
	rtexture_t *lightmaptexture;
	// the lighting direction texture fragment to use on the rendering mesh
	rtexture_t *deluxemaptexture;
	// lightmaptexture rebuild information not used in q3bsp
	msurface_lightmapinfo_t *lightmapinfo; // q1bsp
	// fog volume info in q3bsp
	struct q3deffect_s *effect; // q3bsp
	// mesh information for collisions (only used by q3bsp curves)
	int num_firstcollisiontriangle;
	int *deprecatedq3data_collisionelement3i; // q3bsp
	float *deprecatedq3data_collisionvertex3f; // q3bsp
	float *deprecatedq3data_collisionbbox6f; // collision optimization - contains combined bboxes of every data_collisionstride triangles
	float *deprecatedq3data_bbox6f; // collision optimization - contains combined bboxes of every data_collisionstride triangles

	// surfaces own ranges of vertices and triangles in the model->surfmesh
	int num_triangles; // number of triangles
	int num_firsttriangle; // first triangle
	int num_vertices; // number of vertices
	int num_firstvertex; // first vertex

	// shadow volume building information
	int num_firstshadowmeshtriangle; // index into model->brush.shadowmesh

	// mesh information for collisions (only used by q3bsp curves)
	int num_collisiontriangles; // q3bsp
	int num_collisionvertices; // q3bsp
	int deprecatedq3num_collisionbboxstride;
	int deprecatedq3num_bboxstride;
	// FIXME: collisionmarkframe should be kept in a separate array
	int deprecatedq3collisionmarkframe; // q3bsp // don't collide twice in one trace
}
msurface_t;

#include "matrixlib.h"
#include "bih.h"

#include "model_brush.h"
#include "model_q1bsp.h"
#include "model_q2bsp.h"
#include "model_q3bsp.h"
#include "model_vbsp.h"
#include "model_sprite.h"
#include "model_alias.h"

struct trace_s;



struct frameblend_s;
struct skeleton_s;

typedef struct model_s
{
	// name and path of model, for example "progs/player.mdl"
	char			model_name[MAX_QPATH];
	// model needs to be loaded if this is false
	qbool		loaded;
	// set if the model is used in current map, models which are not, are purged
	qbool		used;
	// CRC of the file this model was loaded from, to reload if changed
	unsigned int	crc;
	// mod_brush, mod_alias, mod_sprite
	modtype_t		type;
	// memory pool for allocations
	struct mempool_s		*mempool;
	// all models use textures...
	rtexturepool_t	*texturepool;
	// EF_* flags (translated from the model file's different flags layout)
	int				effects;
	// number of QC accessible frame(group)s in the model
	int				numframes;
	// number of QC accessible skin(group)s in the model
	int				numskins;
	// whether to randomize animated framegroups
	synctype_t		synctype;
	// bounding box at angles '0 0 0'
	vec3_t			normalmins, normalmaxs;
	// bounding box if yaw angle is not 0, but pitch and roll are
	vec3_t			yawmins, yawmaxs;
	// bounding box if pitch or roll are used
	vec3_t			rotatedmins, rotatedmaxs;
	// sphere radius, usable at any angles
	float			radius;
	// squared sphere radius for easier comparisons
	float			radius2;
	// skin animation info
	animscene_t		*skinscenes; // [numskins]
	// skin animation info
	animscene_t		*animscenes; // [numframes]
	// range of surface numbers in this (sub)model
	int				firstmodelsurface;
	int				nummodelsurfaces;
	int				*sortedmodelsurfaces;
	// range of collision brush numbers in this (sub)model
	int				firstmodelbrush;
	int				nummodelbrushes;
	// BIH (Bounding Interval Hierarchy) for this (sub)model
	bih_t			collision_bih;
	bih_t			render_bih; // if not set, use collision_bih instead for rendering purposes too
	// for md3 models
	int				num_tags;
	int				num_tagframes;
	aliastag_t		*data_tags;
	// for skeletal models
	int				num_bones;
	aliasbone_t		*data_bones;
	float			num_posescale; // scaling factor from origin in poses7s format (includes divide by 32767)
	float			num_poseinvscale; // scaling factor to origin in poses7s format (includes multiply by 32767)
	int				num_poses;
	short			*data_poses7s; // origin xyz, quat xyzw, unit length, values normalized to +/-32767 range
	float			*data_baseboneposeinverse;
	// textures of this model
	int				num_textures;
	int				num_texturesperskin;
	texture_t		*data_textures;
	qbool		wantnormals;
	qbool		wanttangents;
	// surfaces of this model
	int				num_surfaces;
	msurface_t		*data_surfaces;
	// optional lightmapinfo data for surface lightmap updates
	msurface_lightmapinfo_t *data_surfaces_lightmapinfo;
	// all surfaces belong to this mesh
	surfmesh_t		surfmesh;
	// data type of model
	const char		*modeldatatypestring;
	// generates vertex data for a given frameblend
	void(*AnimateVertices)(const struct model_s * RESTRICT model, const struct frameblend_s * RESTRICT frameblend, const struct skeleton_s *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f);
	// draw the model's sky polygons (only used by brush models)
	void(*DrawSky)(struct entity_render_s *ent);
	// draw refraction/reflection textures for the model's water polygons (only used by brush models)
	void(*DrawAddWaterPlanes)(struct entity_render_s *ent);
	// draw the model using lightmap/dlight shading
	void(*Draw)(struct entity_render_s *ent);
	// draw the model to the depth buffer (no color rendering at all)
	void(*DrawDepth)(struct entity_render_s *ent);
	// draw any enabled debugging effects on this model (such as showing triangles, normals, collision brushes...)
	void(*DrawDebug)(struct entity_render_s *ent);
	// draw geometry textures for deferred rendering
	void(*DrawPrepass)(struct entity_render_s *ent);
    // compile an optimized shadowmap mesh for the model based on light source
	void(*CompileShadowMap)(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
	// draw depth into a shadowmap
	void(*DrawShadowMap)(int side, struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist, const unsigned char *surfacesides, const vec3_t lightmins, const vec3_t lightmaxs);
	// gathers info on which clusters and surfaces are lit by light, as well as calculating a bounding box
	void(*GetLightInfo)(struct entity_render_s *ent, vec3_t relativelightorigin, float lightradius, vec3_t outmins, vec3_t outmaxs, int *outleaflist, unsigned char *outleafpvs, int *outnumleafspointer, int *outsurfacelist, unsigned char *outsurfacepvs, int *outnumsurfacespointer, unsigned char *outshadowtrispvs, unsigned char *outlighttrispvs, unsigned char *visitingleafpvs, int numfrustumplanes, const mplane_t *frustumplanes);
	// compile a shadow volume for the model based on light source
	void(*CompileShadowVolume)(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
	// draw a shadow volume for the model based on light source
	void(*DrawShadowVolume)(struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist, const vec3_t lightmins, const vec3_t lightmaxs);
	// draw the lighting on a model (through stencil)
	void(*DrawLight)(struct entity_render_s *ent, int numsurfaces, const int *surfacelist, const unsigned char *trispvs);
	// trace a box against this model
	void (*TraceBox)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask);
	void (*TraceBrush)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, struct colbrushf_s *start, struct colbrushf_s *end, int hitsupercontentsmask, int skipsupercontentsmask);
	// trace a box against this model
	void (*TraceLine)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask);
	// trace a point against this model (like PointSuperContents)
	void (*TracePoint)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask);
	// find the supercontents value at a point in this model
	int (*PointSuperContents)(struct model_s *model, int frame, const vec3_t point);
	// trace a line against geometry in this model and report correct texture (used by r_shadow_bouncegrid)
	void (*TraceLineAgainstSurfaces)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask);
	// fields belonging to some types of model
	model_sprite_t	sprite;
	model_brush_t	brush;
	model_brushq1_t	brushq1;
	model_brushq2_t	brushq2;
	model_brushq3_t	brushq3;
	// flags this model for offseting sounds to the model center (used by brush models)
	int soundfromcenter;

	// if set, the model contains light information (lightmap, or vertexlight)
	qbool lit;
	float lightmapscale;

	qbool nolerp; // SEPUS
	loadinfo_s	loadinfox;
}
model_t;

//============================================================================

// model loading
extern model_t *loadmodel;
extern unsigned char *mod_base;
// sky/water subdivision
//extern struct cvar_s gl_subdivide_size;
// texture fullbrights
extern struct cvar_s r_fullbrights;
extern struct cvar_s r_enableshadowvolumes;

void Mod_Init (void);
void Mod_Reload (void);
model_t *Mod_LoadModel(model_t *mod, qbool crash, qbool checkdisk);
model_t *Mod_FindName (const char *name, const char *parentname);
model_t *Mod_ForName (const char *name, qbool crash, qbool checkdisk, const char *parentname);
void Mod_UnloadModel (model_t *mod);

void Mod_ClearUsed(void);
void Mod_PurgeUnused(void);
void Mod_RemoveStaleWorldModels(model_t *skip); // only used during loading!

extern model_t *loadmodel;
extern char loadname[32];	// for hunk tags

int Mod_BuildVertexRemapTableFromElements(int numelements, const int *elements, int numvertices, int *remapvertices);
void Mod_BuildTriangleNeighbors(int *neighbors, const int *elements, int numtriangles);
void Mod_ValidateElements(int *elements, int numtriangles, int firstvertex, int numverts, const char *filename, int fileline);
void Mod_BuildNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const int *elements, float *normal3f, qbool areaweighting);
void Mod_BuildTextureVectorsFromNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const float *texcoord2f, const float *normal3f, const int *elements, float *svector3f, float *tvector3f, qbool areaweighting);

void Mod_AllocSurfMesh(mempool_t *mempool, int numvertices, int numtriangles, qbool lightmapoffsets, qbool vertexcolors, qbool neighbors);
void Mod_MakeSortedSurfaces(model_t *mod);

// called specially by brush model loaders before generating submodels
// automatically called after model loader returns
void Mod_BuildVBOs(void);

shadowmesh_t *Mod_ShadowMesh_Alloc(mempool_t *mempool, int maxverts, int maxtriangles, rtexture_t *map_diffuse, rtexture_t *map_specular, rtexture_t *map_normal, int light, int neighbors, int expandable);
shadowmesh_t *Mod_ShadowMesh_ReAlloc(mempool_t *mempool, shadowmesh_t *oldmesh, int light, int neighbors);
int Mod_ShadowMesh_AddVertex(shadowmesh_t *mesh, float *vertex14f);
void Mod_ShadowMesh_AddTriangle(mempool_t *mempool, shadowmesh_t *mesh, rtexture_t *map_diffuse, rtexture_t *map_specular, rtexture_t *map_normal, float *vertex14f);
void Mod_ShadowMesh_AddMesh(mempool_t *mempool, shadowmesh_t *mesh, rtexture_t *map_diffuse, rtexture_t *map_specular, rtexture_t *map_normal, const float *vertex3f, const float *svector3f, const float *tvector3f, const float *normal3f, const float *texcoord2f, int numtris, const int *element3i);
shadowmesh_t *Mod_ShadowMesh_Begin(mempool_t *mempool, int maxverts, int maxtriangles, rtexture_t *map_diffuse, rtexture_t *map_specular, rtexture_t *map_normal, int light, int neighbors, int expandable);
shadowmesh_t *Mod_ShadowMesh_Finish(mempool_t *mempool, shadowmesh_t *firstmesh, qbool light, qbool neighbors, qbool createvbo);
void Mod_ShadowMesh_CalcBBox(shadowmesh_t *firstmesh, vec3_t mins, vec3_t maxs, vec3_t center, float *radius);
void Mod_ShadowMesh_Free(shadowmesh_t *mesh);

void Mod_CreateCollisionMesh(model_t *mod);

void Mod_FreeQ3Shaders(void);
void Mod_LoadQ3Shaders(void);
q3shaderinfo_t *Mod_LookupQ3Shader(const char *name);
qbool Mod_LoadTextureFromQ3Shader(texture_t *texture, const char *name, qbool warnmissing, qbool fallback, int defaulttexflags);

extern struct cvar_s r_mipskins;
extern struct cvar_s r_mipnormalmaps;

typedef struct skinfileitem_s
{
	struct skinfileitem_s *next;
	char name[MAX_QPATH];
	char replacement[MAX_QPATH];
}
skinfileitem_t;

typedef struct skinfile_s
{
	struct skinfile_s *next;
	skinfileitem_t *items;
}
skinfile_t;

skinfile_t *Mod_LoadSkinFiles(void);
void Mod_FreeSkinFiles(skinfile_t *skinfile);
int Mod_CountSkinFiles(skinfile_t *skinfile);
void Mod_BuildAliasSkinsFromSkinFiles(texture_t *skin, skinfile_t *skinfile, const char *meshname, const char *shadername);

void Mod_SnapVertices(int numcomponents, int numvertices, float *vertices, float snap);
int Mod_RemoveDegenerateTriangles(int numtriangles, const int *inelement3i, int *outelement3i, const float *vertex3f);
void Mod_VertexRangeFromElements(int numelements, const int *elements, int *firstvertexpointer, int *lastvertexpointer);

typedef struct mod_alloclightmap_row_s
{
	int rowY;
	int currentX;
}
mod_alloclightmap_row_t;

typedef struct mod_alloclightmap_state_s
{
	int width;
	int height;
	int currentY;
	mod_alloclightmap_row_t *rows;
}
mod_alloclightmap_state_t;

void Mod_AllocLightmap_Init(mod_alloclightmap_state_t *state, int width, int height);
void Mod_AllocLightmap_Free(mod_alloclightmap_state_t *state);
void Mod_AllocLightmap_Reset(mod_alloclightmap_state_t *state);
qbool Mod_AllocLightmap_Block(mod_alloclightmap_state_t *state, int blockwidth, int blockheight, int *outx, int *outy);

// bsp models
void Mod_BrushInit(void);
// used for talking to the QuakeC mainly
int Mod_Q1BSP_NativeContentsFromSuperContents(struct model_s *model, int supercontents);
int Mod_Q1BSP_SuperContentsFromNativeContents(struct model_s *model, int nativecontents);
// used for loading wal files in Mod_LoadTextureFromQ3Shader
int Mod_Q2BSP_SuperContentsFromNativeContents(model_t *model, int nativecontents);
int Mod_Q2BSP_NativeContentsFromSuperContents(model_t *model, int supercontents);

// a lot of model formats use the Q1BSP code, so here are the prototypes...
struct entity_render_s;
void R_Mod_DrawAddWaterPlanes(struct entity_render_s *ent);
void R_Mod_DrawSky(struct entity_render_s *ent);
void R_Mod_Draw(struct entity_render_s *ent);
void R_Mod_DrawDepth(struct entity_render_s *ent);
void R_Mod_DrawDebug(struct entity_render_s *ent);
void R_Mod_DrawPrepass(struct entity_render_s *ent);
void R_Mod_GetLightInfo(struct entity_render_s *ent, vec3_t relativelightorigin, float lightradius, vec3_t outmins, vec3_t outmaxs, int *outleaflist, unsigned char *outleafpvs, int *outnumleafspointer, int *outsurfacelist, unsigned char *outsurfacepvs, int *outnumsurfacespointer, unsigned char *outshadowtrispvs, unsigned char *outlighttrispvs, unsigned char *visitingleafpvs, int numfrustumplanes, const mplane_t *frustumplanes);
void R_Mod_CompileShadowMap(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
void R_Mod_DrawShadowMap(int side, struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int modelnumsurfaces, const int *modelsurfacelist, const unsigned char *surfacesides, const vec3_t lightmins, const vec3_t lightmaxs);
void R_Mod_CompileShadowVolume(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
void R_Q1BSP_DrawShadowVolume(struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist, const vec3_t lightmins, const vec3_t lightmaxs);
void R_Mod_DrawLight(struct entity_render_s *ent, int numsurfaces, const int *surfacelist, const unsigned char *trispvs);

// Collision optimization using Bounding Interval Hierarchy
void Mod_CollisionBIH_TracePoint(model_t *model, const struct frameblend_s *frameblend, const skeleton_t *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask);
void Mod_CollisionBIH_TraceLine(model_t *model, const struct frameblend_s *frameblend, const skeleton_t *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask);
void Mod_CollisionBIH_TraceBox(model_t *model, const struct frameblend_s *frameblend, const skeleton_t *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask);
void Mod_CollisionBIH_TraceBrush(model_t *model, const struct frameblend_s *frameblend, const skeleton_t *skeleton, struct trace_s *trace, struct colbrushf_s *start, struct colbrushf_s *end, int hitsupercontentsmask, int skipsupercontentsmask);
void Mod_CollisionBIH_TracePoint_Mesh(model_t *model, const struct frameblend_s *frameblend, const skeleton_t *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask);
qbool Mod_CollisionBIH_TraceLineOfSight(struct model_s *model, const vec3_t start, const vec3_t end);
int Mod_CollisionBIH_PointSuperContents(struct model_s *model, int frame, const vec3_t point);
int Mod_CollisionBIH_PointSuperContents_Mesh(struct model_s *model, int frame, const vec3_t point);
bih_t *Mod_MakeCollisionBIH(model_t *model, qbool userendersurfaces, bih_t *out);

// alias models
struct frameblend_s;
struct skeleton_s;
void Mod_AliasInit(void);
int Mod_Alias_GetTagMatrix(const model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, int tagindex, matrix4x4_t *outmatrix);
int Mod_Alias_GetTagIndexForName(const model_t *model, unsigned int skin, const char *tagname);
int Mod_Alias_GetExtendedTagInfoForIndex(const model_t *model, unsigned int skin, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, int tagindex, int *parentindex, const char **tagname, matrix4x4_t *tag_localmatrix);

void Mod_Skeletal_FreeBuffers(void);

// sprite models
void Mod_SpriteInit(void);

// loaders
void Mod_Q1BSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IBSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_MAP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_OBJ_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP0_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP2_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP3_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_ZYMOTICMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_DARKPLACESMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_PSKMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDS2_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer, void *bufferend);

#endif	// MODEL_SHARED_H

