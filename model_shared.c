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
// model_shared.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "image.h"
#include "r_shadow.h"
#include "polygon.h"

cvar_t r_mipskins = {CF_CLIENT | CF_ARCHIVE, "r_mipskins", "0", "mipmaps model skins so they render faster in the distance and do not display noise artifacts, can cause discoloration of skins if they contain undesirable border colors"};
cvar_t r_mipnormalmaps = {CF_CLIENT | CF_ARCHIVE, "r_mipnormalmaps", "1", "mipmaps normalmaps (turning it off looks sharper but may have aliasing)"};
cvar_t mod_generatelightmaps_unitspersample = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_unitspersample", "8", "lightmap resolution"};
cvar_t mod_generatelightmaps_borderpixels = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_borderpixels", "2", "extra space around polygons to prevent sampling artifacts"};
cvar_t mod_generatelightmaps_texturesize = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_texturesize", "1024", "size of lightmap textures"};
cvar_t mod_generatelightmaps_lightmapsamples = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_lightmapsamples", "16", "number of shadow tests done per lightmap pixel"};
cvar_t mod_generatelightmaps_vertexsamples = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_vertexsamples", "16", "number of shadow tests done per vertex"};
cvar_t mod_generatelightmaps_gridsamples = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_gridsamples", "64", "number of shadow tests done per lightgrid cell"};
cvar_t mod_generatelightmaps_lightmapradius = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_lightmapradius", "16", "sampling area around each lightmap pixel"};
cvar_t mod_generatelightmaps_vertexradius = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_vertexradius", "16", "sampling area around each vertex"};
cvar_t mod_generatelightmaps_gridradius = {CF_CLIENT | CF_ARCHIVE, "mod_generatelightmaps_gridradius", "64", "sampling area around each lightgrid cell center"};

model_t *loadmodel;

// Supported model formats
static modloader_t loader[] =
{
	{"obj", NULL, 0, Mod_OBJ_Load},
	{NULL, "IDPO", 4, Mod_IDP0_Load},
	{NULL, "IDP2", 4, Mod_IDP2_Load},
	{NULL, "IDP3", 4, Mod_IDP3_Load},	// IDP3 md3?
	{NULL, "IDSP", 4, Mod_IDSP_Load}, // Baker: SPRITE
	{NULL, "IDS2", 4, Mod_IDS2_Load},
	{NULL, "\035", 1, Mod_Q1BSP_Load},
	{NULL, "\036", 1, Mod_HLBSP_Load},
	{NULL, "BSP2", 4, Mod_BSP2_Load},
	{NULL, "2PSB", 4, Mod_2PSB_Load},
	{NULL, "IBSP", 4, Mod_IBSP_Load},
	{NULL, "VBSP", 4, Mod_VBSP_Load},
	{NULL, "ZYMOTICMODEL", 13, Mod_ZYMOTICMODEL_Load},
	{NULL, "DARKPLACESMODEL", 16, Mod_DARKPLACESMODEL_Load},
	{NULL, "PSKMODEL", 9, Mod_PSKMODEL_Load},
	{NULL, "INTERQUAKEMODEL", 16, Mod_INTERQUAKEMODEL_Load},
	{"map", NULL, 0, Mod_MAP_Load},
	{NULL, NULL, 0, NULL}
};

static mempool_t *mod_mempool;
static memexpandablearray_t models;

static mempool_t *q3shaders_mem;
typedef struct q3shader_hash_entry_s
{
  shader_t shader;
  struct q3shader_hash_entry_s* chain;
} q3shader_hash_entry_t;
#define Q3SHADER_HASH_SIZE_1021  1021
typedef struct q3shader_data_s
{
  memexpandablearray_t hash_entries;
  q3shader_hash_entry_t hash[Q3SHADER_HASH_SIZE_1021];
  memexpandablearray_t char_ptrs;
} q3shader_data_t;
static q3shader_data_t *q3shader_data;

WARP_X_CALLERS_ ()
static void mod_start(void)
{
	int i, count;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;

	SCR_PushLoadingScreen("Loading models", 1.0);
	count = 0;
	for (i = 0;i < nummodels;i++)
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && mod->model_name[0] != '*')
			if (mod->used)
				++count;
	for (i = 0;i < nummodels;i++)
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && mod->model_name[0] != '*')
			if (mod->used)
			{
				SCR_PushLoadingScreen(mod->model_name, 1.0 / count);
				Mod_LoadModel(mod, true, false);
				SCR_PopLoadingScreen(false);
			}
	SCR_PopLoadingScreen(false);
}

int is_game_switch; // Baker r9003: Clear models/sounds on gamedir change
static void mod_shutdown(void)
{
	int i;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;

	for (i = 0; i < nummodels; i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && (mod->loaded || mod->mempool)) {
			Mod_UnloadModel(mod);
			// Baker r9003: Clear models/sounds on gamedir change
			if (is_game_switch) {
				mod->used = false; // Baker .. loadmodel: stuff from prior gamedir persisted
			} // if
		} // mod
 	} // for

	Mod_FreeQ3Shaders();
	Mod_Skeletal_FreeBuffers();

	// Baker r9003: Clear models/sounds on gamedir change
	if (is_game_switch) {
		//Mod_PurgeUnused (); // Baker .. loadmodel: stuff from prior gamedir persisted
		Mod_PurgeALL ();
#ifdef CONFIG_MENU
		S_PurgeALL (); //S_PurgeUnused();
#endif
		is_game_switch = false; // Baker r9062: This is where.
	} else {
		// Could be ALT-TAB or vid restart
	}
}

WARP_X_ (mod->brushq1.lightmapupdateflags)
static void mod_newmap(void)
{
	msurface_t *surface;
	int i, j, k, l, surfacenum, ssize, tsize;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;

	for (i = 0;i < nummodels;i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->mempool) {
			for (j = 0;j < mod->num_textures && mod->data_textures;j++) {
				// note that materialshaderpass and backgroundshaderpass point to shaderpasses[] and so do the pre/post shader ranges, so this catches all of them...
				for (l = 0; l < Q3SHADER_MAXLAYERS_8; l++)
					if (mod->data_textures[j].shaderpasses[l])
						for (k = 0; k < mod->data_textures[j].shaderpasses[l]->numframes; k++)
							R_SkinFrame_MarkUsed(mod->data_textures[j].shaderpasses[l]->skinframes[k]);
			}
			if (mod->brush.solidskyskinframe)
				R_SkinFrame_MarkUsed(mod->brush.solidskyskinframe);
			if (mod->brush.alphaskyskinframe)
				R_SkinFrame_MarkUsed(mod->brush.alphaskyskinframe);
		}
	}

	if (!cl_stainmaps_clearonload.integer)
		return;

	for (i = 0;i < nummodels;i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->mempool && mod->data_surfaces) {
			for (surfacenum = 0, surface = mod->data_surfaces;surfacenum < mod->num_surfaces;surfacenum++, surface++) {
				if (surface->lightmapinfo && surface->lightmapinfo->stainsamples) {
					ssize = (surface->lightmapinfo->extents[0] >> 4) + 1;
					tsize = (surface->lightmapinfo->extents[1] >> 4) + 1;
					memset(surface->lightmapinfo->stainsamples, 255, ssize * tsize * 3);
					mod->brushq1.lightmapupdateflags[surfacenum] = true;
				}
			}
		}
	}
}

/*
===============
Mod_InitOnce
===============
*/
static void Mod_Print_f(cmd_state_t *cmd);
static void Mod_Precache_f(cmd_state_t *cmd);
static void Mod_Decompile_f(cmd_state_t *cmd);
static void Mod_TextureFindPos_f(cmd_state_t *cmd);
static void Mod_GenerateLightmaps_f(cmd_state_t *cmd);
void Mod_InitOnce (void)
{
	mod_mempool = Mem_AllocPool("modelinfo", 0, NULL);
	Mem_ExpandableArray_NewArray(&models, mod_mempool, sizeof(model_t), 16);

	Mod_BrushInit();
	Mod_AliasInit();
	Mod_SpriteInit();

	Cvar_RegisterVariable(&r_mipskins);
	Cvar_RegisterVariable(&r_mipnormalmaps);
	Cvar_RegisterVariable(&mod_generatelightmaps_unitspersample);
	Cvar_RegisterVariable(&mod_generatelightmaps_borderpixels);
	Cvar_RegisterVariable(&mod_generatelightmaps_texturesize);

	Cvar_RegisterVariable(&mod_generatelightmaps_lightmapsamples);
	Cvar_RegisterVariable(&mod_generatelightmaps_vertexsamples);
	Cvar_RegisterVariable(&mod_generatelightmaps_gridsamples);
	Cvar_RegisterVariable(&mod_generatelightmaps_lightmapradius);
	Cvar_RegisterVariable(&mod_generatelightmaps_vertexradius);
	Cvar_RegisterVariable(&mod_generatelightmaps_gridradius);

	Cmd_AddCommand(CF_CLIENT, "modellist", Mod_Print_f, "prints a list of loaded models");
	Cmd_AddCommand(CF_CLIENT, "modelprecache", Mod_Precache_f, "load a model");
	Cmd_AddCommand(CF_CLIENT, "modeldecompile", Mod_Decompile_f, "exports a model in several formats for editing purposes");
	Cmd_AddCommand(CF_CLIENT, "texturefindpos", Mod_TextureFindPos_f, "finds the position of the first instance of a texture in a map.  Partial texture names allowed [Zircon]");
	Cmd_AddCommand(CF_CLIENT, "mod_generatelightmaps", Mod_GenerateLightmaps_f, "rebuilds lighting on current worldmodel");
}

void Mod_RenderInit(void)
{
	R_RegisterModule("Models", mod_start, mod_shutdown, mod_newmap, NULL, NULL);
}

void Mod_UnloadModel (model_t *mod)
{
	char name[MAX_QPATH_128];
	qbool used;
	model_t *parentmodel;

	if (developer_loading.integer)
		Con_PrintLinef ("unloading model %s", mod->model_name);

	strlcpy(name, mod->model_name, sizeof(name));
	parentmodel = mod->brush.parentmodel;
	used = mod->used;
	if (mod->mempool)
	{
		if (mod->surfmesh.data_element3i_indexbuffer && !mod->surfmesh.data_element3i_indexbuffer->isdynamic)
			R_Mesh_DestroyMeshBuffer(mod->surfmesh.data_element3i_indexbuffer);
		mod->surfmesh.data_element3i_indexbuffer = NULL;
		if (mod->surfmesh.data_element3s_indexbuffer && !mod->surfmesh.data_element3s_indexbuffer->isdynamic)
			R_Mesh_DestroyMeshBuffer(mod->surfmesh.data_element3s_indexbuffer);
		mod->surfmesh.data_element3s_indexbuffer = NULL;
		if (mod->surfmesh.data_vertex3f_vertexbuffer && !mod->surfmesh.data_vertex3f_vertexbuffer->isdynamic)
			R_Mesh_DestroyMeshBuffer(mod->surfmesh.data_vertex3f_vertexbuffer);
		mod->surfmesh.data_vertex3f_vertexbuffer = NULL;
		mod->surfmesh.data_svector3f_vertexbuffer = NULL;
		mod->surfmesh.data_tvector3f_vertexbuffer = NULL;
		mod->surfmesh.data_normal3f_vertexbuffer = NULL;
		mod->surfmesh.data_texcoordtexture2f_vertexbuffer = NULL;
		mod->surfmesh.data_texcoordlightmap2f_vertexbuffer = NULL;
		mod->surfmesh.data_lightmapcolor4f_vertexbuffer = NULL;
		mod->surfmesh.data_skeletalindex4ub_vertexbuffer = NULL;
		mod->surfmesh.data_skeletalweight4ub_vertexbuffer = NULL;
	}
	// free textures/memory attached to the model
	R_FreeTexturePool(&mod->texturepool);
	Mem_FreePool(&mod->mempool);
	// clear the struct to make it available
	memset(mod, 0, sizeof(model_t));
	// restore the fields we want to preserve
	strlcpy(mod->model_name, name, sizeof(mod->model_name));
	mod->brush.parentmodel = parentmodel;
	mod->used = used;
	mod->loaded = false;
}

static void R_Model_Null_Draw(entity_render_t *ent)
{
	return;
}


typedef void (*mod_framegroupify_parsegroups_t) (unsigned int i, int start, int len, float fps, qbool loop, const char *name, void *pass);

static int Mod_FrameGroupify_ParseGroups(const char *buf, mod_framegroupify_parsegroups_t cb, void *pass)
{
	const char *bufptr;
	int start, len;
	float fps;
	unsigned int i;
	qbool loop;
	char name[64];

	bufptr = buf;
	i = 0;
	while(bufptr)
	{
		// an anim scene!

		// REQUIRED: fetch start
		COM_ParseToken_Simple(&bufptr, true, false, true);
		if (!bufptr)
			break; // end of file
		if (String_Match(com_token, "\n"))
			continue; // empty line
		start = atoi(com_token);

		// REQUIRED: fetch length
		COM_ParseToken_Simple(&bufptr, true, false, true);
		if (!bufptr || String_Match(com_token, "\n"))
		{
			Con_Printf ("framegroups file: missing number of frames\n");
			continue;
		}
		len = atoi(com_token);

		// OPTIONAL args start
		COM_ParseToken_Simple(&bufptr, true, false, true);

		// OPTIONAL: fetch fps
		fps = 20;
		if (bufptr && strcmp(com_token, "\n"))
		{
			fps = atof(com_token);
			COM_ParseToken_Simple(&bufptr, true, false, true);
		}

		// OPTIONAL: fetch loopflag
		loop = true;
		if (bufptr && strcmp(com_token, "\n"))
		{
			loop = (atoi(com_token) != 0);
			COM_ParseToken_Simple(&bufptr, true, false, true);
		}

		// OPTIONAL: fetch name
		name[0] = 0;
		if (bufptr && strcmp(com_token, "\n"))
		{
			strlcpy(name, com_token, sizeof(name));
			COM_ParseToken_Simple(&bufptr, true, false, true);
		}

		// OPTIONAL: remaining unsupported tokens (eat them)
		while (bufptr && strcmp(com_token, "\n"))
			COM_ParseToken_Simple(&bufptr, true, false, true);

		//Con_Printf ("data: %d %d %d %f %d (%s)\n", i, start, len, fps, loop, name);

		if (cb)
			cb(i, start, len, fps, loop, (name[0] ? name : NULL), pass);
		++i;
	}

	return i;
}

static void Mod_FrameGroupify_ParseGroups_Store (unsigned int i, int start, int len, float fps, qbool loop, const char *name, void *pass)
{
	model_t *mod = (model_t *) pass;
	animscene_t *anim = &mod->animscenes[i];
	if (name)
		strlcpy(anim->name, name, sizeof(anim[i].name));
	else
		dpsnprintf(anim->name, sizeof(anim[i].name), "groupified_%d_anim", i);
	anim->firstframe = bound(0, start, mod->num_poses - 1);
	anim->framecount = bound(1, len, mod->num_poses - anim->firstframe);
	anim->framerate = max(1, fps);
	anim->loop = !!loop;
	//Con_Printf ("frame group %d is %d %d %f %d\n", i, start, len, fps, loop);
}

static void Mod_FrameGroupify(model_t *mod, const char *buf)
{
	unsigned int cnt;

	// 0. count
	cnt = Mod_FrameGroupify_ParseGroups(buf, NULL, NULL);
	if (!cnt)
	{
		Con_Printf ("no scene found in framegroups file, aborting\n");
		return;
	}
	mod->numframes = cnt;

	// 1. reallocate
	// (we do not free the previous animscenes, but model unloading will free the pool owning them, so it's okay)
	mod->animscenes = (animscene_t *) Mem_Alloc(mod->mempool, sizeof(animscene_t) * mod->numframes);

	// 2. parse
	Mod_FrameGroupify_ParseGroups(buf, Mod_FrameGroupify_ParseGroups_Store, mod);
}

static void Mod_FindPotentialDeforms(model_t *mod)
{
	int i, j;
	texture_t *texture;
	mod->wantnormals = false;
	mod->wanttangents = false;
	for (i = 0;i < mod->num_textures;i++)
	{
		texture = mod->data_textures + i;
		if (texture->materialshaderpass && texture->materialshaderpass->tcgen.tcgen == Q3TCGEN_ENVIRONMENT)
			mod->wantnormals = true;
		if (texture->materialshaderpass && texture->materialshaderpass->tcgen.tcgen == Q3TCGEN_ENVIRONMENT)
			mod->wantnormals = true;
		for (j = 0;j < Q3MAXDEFORMS_4;j++)
		{
			if (texture->deforms[j].deform == Q3DEFORM_AUTOSPRITE)
			{
				mod->wanttangents = true;
				mod->wantnormals = true;
				break;
			}
			if (texture->deforms[j].deform != Q3DEFORM_NONE)
				mod->wantnormals = true;
		}
	}
}

/*
==================
Mod_LoadModel

Loads a model
==================
*/
WARP_X_CALLERS_ ()
model_t *Mod_LoadModel(model_t *mod, qbool crash, qbool checkdisk)
{
	unsigned int crc;
	void *buf;
	fs_offset_t filesize = 0;
	char vabuf[1024];

	mod->used = true;

	if (mod->model_name[0] == '*') // submodel
		return mod;

	if (String_Match(mod->model_name, "null")) {
		if (mod->loaded)
			return mod;

		if (mod->loaded || mod->mempool)
			Mod_UnloadModel(mod);

		if (developer_loading.integer)
			Con_PrintLinef ("loading model %s", mod->model_name);

		mod->used = true;
		mod->crc = (unsigned int)-1;
		mod->loaded = false;

		VectorClear(mod->normalmins);
		VectorClear(mod->normalmaxs);
		VectorClear(mod->yawmins);
		VectorClear(mod->yawmaxs);
		VectorClear(mod->rotatedmins);
		VectorClear(mod->rotatedmaxs);

		mod->modeldatatypestring = "null";
		mod->type = mod_null;
		mod->Draw = R_Model_Null_Draw;
		mod->numframes = 2;
		mod->numskins = 1;

		// no fatal errors occurred, so this model is ready to use.
		mod->loaded = true;

		return mod;
	} // if

	crc = 0;
	buf = NULL;

	// even if the model is loaded it still may need reloading...

	// if it is not loaded or checkdisk is true we need to calculate the crc
	if (!mod->loaded || checkdisk) {
		if (checkdisk && mod->loaded)
			Con_DPrintLinef ("checking model %s", mod->model_name);
		buf = FS_LoadFile (mod->model_name, tempmempool, fs_quiet_FALSE, &filesize);
		if (buf) {
			crc = CRC_Block((unsigned char *)buf, filesize);
			// we need to reload the model if the crc does not match
			if (mod->crc != crc)
				mod->loaded = false;
		}
	}

	// if the model is already loaded and checks passed, just return
	if (mod->loaded) {
		if (buf)
			Mem_Free(buf);
		return mod;
	}

	if (developer_loading.integer)
		Con_PrintLinef ("loading model %s", mod->model_name);

	SCR_PushLoadingScreen(mod->model_name, 1);

	// LadyHavoc: unload the existing model in this slot (if there is one)
	if (mod->loaded || mod->mempool)
		Mod_UnloadModel(mod);

	// load the model
	mod->used = true;
	mod->crc = crc;
	// errors can prevent the corresponding mod->loaded = true;
	mod->loaded = false;

	// default lightmap scale
	mod->lightmapscale = 1;

	// default model radius and bounding box (mainly for missing models)
	mod->radius = 16;
	VectorSet(mod->normalmins, -mod->radius, -mod->radius, -mod->radius);
	VectorSet(mod->normalmaxs, mod->radius, mod->radius, mod->radius);
	VectorSet(mod->yawmins, -mod->radius, -mod->radius, -mod->radius);
	VectorSet(mod->yawmaxs, mod->radius, mod->radius, mod->radius);
	VectorSet(mod->rotatedmins, -mod->radius, -mod->radius, -mod->radius);
	VectorSet(mod->rotatedmaxs, mod->radius, mod->radius, mod->radius);

	if (!q3shaders_mem) {
		// load q3 shaders for the first time, or after a level change
		Mod_LoadQ3Shaders();
	}

	if (buf) {
		int i;
		const char *ext = FS_FileExtension(mod->model_name);
		char *bufend = (char *)buf + filesize;

		// all models use memory, so allocate a memory pool
		mod->mempool = Mem_AllocPool(mod->model_name, 0, NULL);

		// We need to have a reference to the base model in case we're parsing submodels
		loadmodel = mod; // (Mod_IDP0_Load)

		// Call the appropriate loader. Try matching magic bytes.
		for (i = 0; loader[i].Load; i++) {
			// Headerless formats can just load based on extension. Otherwise match the magic string.
			if ((loader[i].extension && String_Match_Caseless(ext, loader[i].extension) && !loader[i].header) ||
			   (loader[i].header && !memcmp(buf, loader[i].header, loader[i].headersize)))
			{
				//size_t mod_sz = sizeof(*mod);
				model_t mod_fallback = *mod;

				// Matched. Load it.
				WARP_X_ (Mod_IDP0_Load)
				int res = loader[i].Load(mod, buf, bufend);
				Mem_Free(buf);

				if (res == false) {
					*mod = mod_fallback; // Struct copy
					goto other_failure;
				}

				Mod_FindPotentialDeforms(mod);

				buf = FS_LoadFile(va(vabuf, sizeof(vabuf), "%s.framegroups", mod->model_name), tempmempool, fs_quiet_FALSE, &filesize);
				if (buf)
				{
					Mod_FrameGroupify(mod, (const char *)buf);
					Mem_Free(buf);
				}

				Mod_SetDrawSkyAndWater(mod);
				Mod_BuildVBOs();
				break;
			}
		}
other_failure:
		if (!loader[i].Load) {
			Con_PrintLinef (CON_ERROR "Mod_LoadModel: model " QUOTED_S " is of unknown/unsupported type", mod->model_name);
		}
	}
	else if (crash) {

		if (String_Match(mod->model_name, "progs/beam.mdl")) {
			// Baker r1461: beam.mdl quit printing this thing nothing uses ...
			Con_DPrintLinef (CON_ERROR "Mod_LoadModel: %s not found", mod->model_name);
		} else {
			// LadyHavoc: Sys_Error was *ANNOYING*
			Con_PrintLinef (CON_ERROR "Mod_LoadModel: %s not found", mod->model_name);
		}
	}

	// no fatal errors occurred, so this model is ready to use.
	mod->loaded = true;

	SCR_PopLoadingScreen(false);

	return mod;
}

void Mod_ClearUsed(void)
{
	int i;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;
	for (i = 0; i < nummodels; i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0])
			mod->used = false;
	}
}

void Mod_PurgeUnused(void)
{
	int i;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;
	for (i = 0;i < nummodels; i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && !mod->used) {
			Mod_UnloadModel(mod);
			Mem_ExpandableArray_FreeRecord(&models, mod);
		} // if
	} // for
}

void Mod_PurgeALL(void)
{
	int i;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;
	for (i = 0;i < nummodels; i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && 
			mod->model_name[0] /*&& !mod->used*/) {
			Mod_UnloadModel(mod);
			Mem_ExpandableArray_FreeRecord(&models, mod);
		} // if
	} // for
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName(const char *name, const char *parentname)
{
	int i;
	int nummodels;
	model_t *mod;

	if (!parentname)
		parentname = "";

	nummodels = (int)Mem_ExpandableArray_IndexRange(&models);

	if (!name[0])
		Host_Error_Line ("Mod_ForName: empty name");

	// search the currently loaded models
	for (i = 0; i < nummodels; i++) {
		if ((mod = (model_t*) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && String_Match(mod->model_name, name) && ((!mod->brush.parentmodel && !parentname[0]) || (mod->brush.parentmodel && parentname[0] && String_Match(mod->brush.parentmodel->model_name, parentname)))) {
			mod->used = true;
			return mod;
		}
	} // for

	// no match found, create a new one
	mod = (model_t *) Mem_ExpandableArray_AllocRecord(&models);
	strlcpy(mod->model_name, name, sizeof(mod->model_name));
	if (parentname[0])
		mod->brush.parentmodel = Mod_FindName(parentname, NULL);
	else
		mod->brush.parentmodel = NULL;
	mod->loaded = false;
	mod->used = true;
	return mod;
}

extern qbool vid_opened;

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName(const char *name, qbool crash, qbool checkdisk, const char *parentname)
{
	model_t *model;

	// FIXME: So we don't crash if a server is started early.
	if (!vid_opened)
		CL_StartVideo();

	model = Mod_FindName(name, parentname);
	if (!model->loaded || checkdisk)
		Mod_LoadModel(model, crash, checkdisk);
	return model;
}

/*
==================
Mod_Reload

Reloads all models if they have changed
==================
*/
void Mod_Reload(void)
{
	int i, count;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;

	SCR_PushLoadingScreen("Reloading models", 1.0);
	count = 0;
	for (i = 0; i < nummodels; i++) {
		if ((mod = (model_t *) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && mod->model_name[0] != '*' && mod->used)
			++count;
	}
	for (i = 0;i < nummodels;i++) {
		if ((mod = (model_t *) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && mod->model_name[0] != '*' && mod->used) {
			SCR_PushLoadingScreen(mod->model_name, 1.0 / count);
			Mod_LoadModel(mod, true, true);
			SCR_PopLoadingScreen(false);
		} // if
	} // for i
	SCR_PopLoadingScreen(false);
}

unsigned char *mod_base;


//=============================================================================

/*
================
Mod_Print
================
*/
static void Mod_Print_f(cmd_state_t *cmd)
{
	int i;
	int nummodels = (int)Mem_ExpandableArray_IndexRange(&models);
	model_t *mod;

	Con_PrintLinef ("Loaded models:");
	for (i = 0; i < nummodels; i++) {
		if ((mod = (model_t *) Mem_ExpandableArray_RecordAtIndex(&models, i)) && mod->model_name[0] && mod->model_name[0] != '*')
		{
			if (mod->brush.numsubmodels)
				Con_PrintLinef ("%4iK %s (%d submodels)", mod->mempool ? (int)((mod->mempool->totalsize + 1023) / 1024) : 0, mod->model_name, mod->brush.numsubmodels);
			else
				Con_PrintLinef ("%4iK %s", mod->mempool ? (int)((mod->mempool->totalsize + 1023) / 1024) : 0, mod->model_name);
		}
	}
}

/*
================
Mod_Precache
================
*/
static void Mod_Precache_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) == 2)
		Mod_ForName(Cmd_Argv(cmd, 1), false, true, Cmd_Argv(cmd, 1)[0] == '*' ? cl.model_name[1] : NULL);
	else
		Con_Print("usage: modelprecache <filename>\n");
}

int Mod_BuildVertexRemapTableFromElements(int numelements, const int *elements, int numvertices, int *remapvertices)
{
	int i, count;
	unsigned char *used;
	used = (unsigned char *)Mem_Alloc(tempmempool, numvertices);
	memset(used, 0, numvertices);
	for (i = 0;i < numelements;i++)
		used[elements[i]] = 1;
	for (i = 0, count = 0;i < numvertices;i++)
		remapvertices[i] = used[i] ? count++ : -1;
	Mem_Free(used);
	return count;
}

qbool Mod_ValidateElements(int *element3i, unsigned short *element3s, int numtriangles, int firstvertex, int numvertices, const char *filename, int fileline)
{
	int first = firstvertex, last = first + numvertices - 1, numelements = numtriangles * 3;
	int i;
	int invalidintcount = 0, invalidintexample = 0;
	int invalidshortcount = 0, invalidshortexample = 0;
	int invalidmismatchcount = 0, invalidmismatchexample = 0;
	if (element3i)
	{
		for (i = 0; i < numelements; i++)
		{
			if (element3i[i] < first || element3i[i] > last)
			{
				invalidintcount++;
				invalidintexample = i;
			}
		}
	}
	if (element3s)
	{
		for (i = 0; i < numelements; i++)
		{
			if (element3s[i] < first || element3s[i] > last)
			{
				invalidintcount++;
				invalidintexample = i;
			}
		}
	}
	if (element3i && element3s)
	{
		for (i = 0; i < numelements; i++)
		{
			if (element3s[i] != element3i[i])
			{
				invalidmismatchcount++;
				invalidmismatchexample = i;
			}
		}
	}
	if (invalidintcount || invalidshortcount || invalidmismatchcount)
	{
		Con_Printf ("Mod_ValidateElements(%d, %d, %d, %p, %p) called at %s:%d", numelements, first, last, (void *)element3i, (void *)element3s, filename, fileline);
		Con_Printf (", %d elements are invalid in element3i (example: element3i[%d] == %d)", invalidintcount, invalidintexample, element3i ? element3i[invalidintexample] : 0);
		Con_Printf (", %d elements are invalid in element3s (example: element3s[%d] == %d)", invalidshortcount, invalidshortexample, element3s ? element3s[invalidshortexample] : 0);
		Con_Printf (", %d elements mismatch between element3i and element3s (example: element3s[%d] is %d and element3i[%d] is %d)", invalidmismatchcount, invalidmismatchexample, element3s ? element3s[invalidmismatchexample] : 0, invalidmismatchexample, element3i ? element3i[invalidmismatchexample] : 0);
		Con_Print(".  Please debug the engine code - these elements have been modified to not crash, but nothing more.\n");

		// edit the elements to make them safer, as the driver will crash otherwise
		if (element3i)
			for (i = 0; i < numelements; i++)
				if (element3i[i] < first || element3i[i] > last)
					element3i[i] = first;
		if (element3s)
			for (i = 0; i < numelements; i++)
				if (element3s[i] < first || element3s[i] > last)
					element3s[i] = first;
		if (element3i && element3s)
			for (i = 0; i < numelements; i++)
				if (element3s[i] != element3i[i])
					element3s[i] = element3i[i];

		return false;
	}
	return true;
}

// warning: this is an expensive function!
void Mod_BuildNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const int *elements, float *normal3f, qbool areaweighting)
{
	int i, j;
	const int *element;
	float *vectorNormal;
	float areaNormal[3];
	// clear the vectors
	memset(normal3f + 3 * firstvertex, 0, numvertices * sizeof(float[3]));
	// process each vertex of each triangle and accumulate the results
	// use area-averaging, to make triangles with a big area have a bigger
	// weighting on the vertex normal than triangles with a small area
	// to do so, just add the 'normals' together (the bigger the area
	// the greater the length of the normal is
	element = elements;
	for (i = 0; i < numtriangles; i++, element += 3)
	{
		TriangleNormal(
			vertex3f + element[0] * 3,
			vertex3f + element[1] * 3,
			vertex3f + element[2] * 3,
			areaNormal
			);

		if (!areaweighting)
			VectorNormalize(areaNormal);

		for (j = 0;j < 3;j++)
		{
			vectorNormal = normal3f + element[j] * 3;
			vectorNormal[0] += areaNormal[0];
			vectorNormal[1] += areaNormal[1];
			vectorNormal[2] += areaNormal[2];
		}
	}
	// and just normalize the accumulated vertex normal in the end
	vectorNormal = normal3f + 3 * firstvertex;
	for (i = 0; i < numvertices; i++, vectorNormal += 3)
		VectorNormalize(vectorNormal);
}

#if 0
static void Mod_BuildBumpVectors(const float *v0, const float *v1, const float *v2, const float *tc0, const float *tc1, const float *tc2, float *svector3f, float *tvector3f, float *normal3f)
{
	float f, tangentcross[3], v10[3], v20[3], tc10[2], tc20[2];
	// 79 add/sub/negate/multiply (1 cycle), 1 compare (3 cycle?), total cycles not counting load/store/exchange roughly 82 cycles
	// 6 add, 28 subtract, 39 multiply, 1 compare, 50% chance of 6 negates

	// 6 multiply, 9 subtract
	VectorSubtract(v1, v0, v10);
	VectorSubtract(v2, v0, v20);
	normal3f[0] = v20[1] * v10[2] - v20[2] * v10[1];
	normal3f[1] = v20[2] * v10[0] - v20[0] * v10[2];
	normal3f[2] = v20[0] * v10[1] - v20[1] * v10[0];
	// 12 multiply, 10 subtract
	tc10[1] = tc1[1] - tc0[1];
	tc20[1] = tc2[1] - tc0[1];
	svector3f[0] = tc10[1] * v20[0] - tc20[1] * v10[0];
	svector3f[1] = tc10[1] * v20[1] - tc20[1] * v10[1];
	svector3f[2] = tc10[1] * v20[2] - tc20[1] * v10[2];
	tc10[0] = tc1[0] - tc0[0];
	tc20[0] = tc2[0] - tc0[0];
	tvector3f[0] = tc10[0] * v20[0] - tc20[0] * v10[0];
	tvector3f[1] = tc10[0] * v20[1] - tc20[0] * v10[1];
	tvector3f[2] = tc10[0] * v20[2] - tc20[0] * v10[2];
	// 12 multiply, 4 add, 6 subtract
	f = DotProduct(svector3f, normal3f);
	svector3f[0] -= f * normal3f[0];
	svector3f[1] -= f * normal3f[1];
	svector3f[2] -= f * normal3f[2];
	f = DotProduct(tvector3f, normal3f);
	tvector3f[0] -= f * normal3f[0];
	tvector3f[1] -= f * normal3f[1];
	tvector3f[2] -= f * normal3f[2];
	// if texture is mapped the wrong way (counterclockwise), the tangents
	// have to be flipped, this is detected by calculating a normal from the
	// two tangents, and seeing if it is opposite the surface normal
	// 9 multiply, 2 add, 3 subtract, 1 compare, 50% chance of: 6 negates
	CrossProduct(tvector3f, svector3f, tangentcross);
	if (DotProduct(tangentcross, normal3f) < 0)
	{
		VectorNegate(svector3f, svector3f);
		VectorNegate(tvector3f, tvector3f);
	}
}
#endif

// warning: this is a very expensive function!
void Mod_BuildTextureVectorsFromNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const float *texcoord2f, const float *normal3f, const int *elements, float *svector3f, float *tvector3f, qbool areaweighting)
{
	int i, tnum;
	float sdir[3], tdir[3], normal[3], *svec, *tvec;
	const float *v0, *v1, *v2, *tc0, *tc1, *tc2, *n;
	float f, tangentcross[3], v10[3], v20[3], tc10[2], tc20[2];
	const int *e;
	// clear the vectors
	memset(svector3f + 3 * firstvertex, 0, numvertices * sizeof(float[3]));
	memset(tvector3f + 3 * firstvertex, 0, numvertices * sizeof(float[3]));
	// process each vertex of each triangle and accumulate the results
	for (tnum = 0, e = elements;tnum < numtriangles;tnum++, e += 3)
	{
		v0 = vertex3f + e[0] * 3;
		v1 = vertex3f + e[1] * 3;
		v2 = vertex3f + e[2] * 3;
		tc0 = texcoord2f + e[0] * 2;
		tc1 = texcoord2f + e[1] * 2;
		tc2 = texcoord2f + e[2] * 2;

		// 79 add/sub/negate/multiply (1 cycle), 1 compare (3 cycle?), total cycles not counting load/store/exchange roughly 82 cycles
		// 6 add, 28 subtract, 39 multiply, 1 compare, 50% chance of 6 negates

		// calculate the edge directions and surface normal
		// 6 multiply, 9 subtract
		VectorSubtract(v1, v0, v10);
		VectorSubtract(v2, v0, v20);
		normal[0] = v20[1] * v10[2] - v20[2] * v10[1];
		normal[1] = v20[2] * v10[0] - v20[0] * v10[2];
		normal[2] = v20[0] * v10[1] - v20[1] * v10[0];

		// calculate the tangents
		// 12 multiply, 10 subtract
		tc10[1] = tc1[1] - tc0[1];
		tc20[1] = tc2[1] - tc0[1];
		sdir[0] = tc10[1] * v20[0] - tc20[1] * v10[0];
		sdir[1] = tc10[1] * v20[1] - tc20[1] * v10[1];
		sdir[2] = tc10[1] * v20[2] - tc20[1] * v10[2];
		tc10[0] = tc1[0] - tc0[0];
		tc20[0] = tc2[0] - tc0[0];
		tdir[0] = tc10[0] * v20[0] - tc20[0] * v10[0];
		tdir[1] = tc10[0] * v20[1] - tc20[0] * v10[1];
		tdir[2] = tc10[0] * v20[2] - tc20[0] * v10[2];

		// if texture is mapped the wrong way (counterclockwise), the tangents
		// have to be flipped, this is detected by calculating a normal from the
		// two tangents, and seeing if it is opposite the surface normal
		// 9 multiply, 2 add, 3 subtract, 1 compare, 50% chance of: 6 negates
		CrossProduct(tdir, sdir, tangentcross);
		if (DotProduct(tangentcross, normal) < 0)
		{
			VectorNegate(sdir, sdir);
			VectorNegate(tdir, tdir);
		}

		if (!areaweighting)
		{
			VectorNormalize(sdir);
			VectorNormalize(tdir);
		}
		for (i = 0;i < 3;i++)
		{
			VectorAdd(svector3f + e[i]*3, sdir, svector3f + e[i]*3);
			VectorAdd(tvector3f + e[i]*3, tdir, tvector3f + e[i]*3);
		}
	}
	// make the tangents completely perpendicular to the surface normal, and
	// then normalize them
	// 16 assignments, 2 divide, 2 sqrt, 2 negates, 14 adds, 24 multiplies
	for (i = 0, svec = svector3f + 3 * firstvertex, tvec = tvector3f + 3 * firstvertex, n = normal3f + 3 * firstvertex;i < numvertices;i++, svec += 3, tvec += 3, n += 3)
	{
		f = -DotProduct(svec, n);
		VectorMA(svec, f, n, svec);
		VectorNormalize(svec);
		f = -DotProduct(tvec, n);
		VectorMA(tvec, f, n, tvec);
		VectorNormalize(tvec);
	}
}

void Mod_AllocSurfMesh(mempool_t *mempool, int numvertices, int numtriangles, qbool lightmapoffsets, qbool vertexcolors)
{
	unsigned char *data;
	data = (unsigned char *)Mem_Alloc(mempool, numvertices * (3 + 3 + 3 + 3 + 2 + 2 + (vertexcolors ? 4 : 0)) * sizeof(float) + numvertices * (lightmapoffsets ? 1 : 0) * sizeof(int) + numtriangles * sizeof(int[3]) + (numvertices <= 65536 ? numtriangles * sizeof(unsigned short[3]) : 0));
	loadmodel->surfmesh.num_vertices = numvertices;
	loadmodel->surfmesh.num_triangles = numtriangles;
	if (loadmodel->surfmesh.num_vertices)
	{
		loadmodel->surfmesh.data_vertex3f = (float *)data, data += sizeof(float[3]) * loadmodel->surfmesh.num_vertices;
		loadmodel->surfmesh.data_svector3f = (float *)data, data += sizeof(float[3]) * loadmodel->surfmesh.num_vertices;
		loadmodel->surfmesh.data_tvector3f = (float *)data, data += sizeof(float[3]) * loadmodel->surfmesh.num_vertices;
		loadmodel->surfmesh.data_normal3f = (float *)data, data += sizeof(float[3]) * loadmodel->surfmesh.num_vertices;
		loadmodel->surfmesh.data_texcoordtexture2f = (float *)data, data += sizeof(float[2]) * loadmodel->surfmesh.num_vertices;
		loadmodel->surfmesh.data_texcoordlightmap2f = (float *)data, data += sizeof(float[2]) * loadmodel->surfmesh.num_vertices;
		if (vertexcolors)
			loadmodel->surfmesh.data_lightmapcolor4f = (float *)data, data += sizeof(float[4]) * loadmodel->surfmesh.num_vertices;
		if (lightmapoffsets)
			loadmodel->surfmesh.data_lightmapoffsets = (int *)data, data += sizeof(int) * loadmodel->surfmesh.num_vertices;
	}
	if (loadmodel->surfmesh.num_triangles)
	{
		loadmodel->surfmesh.data_element3i = (int *)data, data += sizeof(int[3]) * loadmodel->surfmesh.num_triangles;
		if (loadmodel->surfmesh.num_vertices <= 65536)
			loadmodel->surfmesh.data_element3s = (unsigned short *)data, data += sizeof(unsigned short[3]) * loadmodel->surfmesh.num_triangles;
	}
}

shadowmesh_t *Mod_ShadowMesh_Alloc(mempool_t *mempool, int maxverts, int maxtriangles)
{
	shadowmesh_t *newmesh;
	newmesh = (shadowmesh_t *)Mem_Alloc(mempool, sizeof(shadowmesh_t));
	newmesh->mempool = mempool;
	newmesh->maxverts = maxverts;
	newmesh->maxtriangles = maxtriangles;
	newmesh->numverts = 0;
	newmesh->numtriangles = 0;
	memset(newmesh->sideoffsets, 0, sizeof(newmesh->sideoffsets));
	memset(newmesh->sidetotals, 0, sizeof(newmesh->sidetotals));

	newmesh->vertex3f = (float *)Mem_Alloc(mempool, maxverts * sizeof(float[3]));
	newmesh->element3i = (int *)Mem_Alloc(mempool, maxtriangles * sizeof(int[3]));
	newmesh->vertexhashtable = (shadowmeshvertexhash_t **)Mem_Alloc(mempool, SHADOWMESHVERTEXHASH * sizeof(shadowmeshvertexhash_t *));
	newmesh->vertexhashentries = (shadowmeshvertexhash_t *)Mem_Alloc(mempool, maxverts * sizeof(shadowmeshvertexhash_t));
	return newmesh;
}

int Mod_ShadowMesh_AddVertex(shadowmesh_t *mesh, const float *vertex3f)
{
	int hashindex, vnum;
	shadowmeshvertexhash_t *hash;
	// this uses prime numbers intentionally
	hashindex = (unsigned int) (vertex3f[0] * 2003 + vertex3f[1] * 4001 + vertex3f[2] * 7919) % SHADOWMESHVERTEXHASH;
	for (hash = mesh->vertexhashtable[hashindex];hash;hash = hash->next)
	{
		vnum = (hash - mesh->vertexhashentries);
		if (mesh->vertex3f[vnum * 3 + 0] == vertex3f[0] && mesh->vertex3f[vnum * 3 + 1] == vertex3f[1] && mesh->vertex3f[vnum * 3 + 2] == vertex3f[2])
			return hash - mesh->vertexhashentries;
	}
	vnum = mesh->numverts++;
	hash = mesh->vertexhashentries + vnum;
	hash->next = mesh->vertexhashtable[hashindex];
	mesh->vertexhashtable[hashindex] = hash;
	mesh->vertex3f[vnum * 3 + 0] = vertex3f[0];
	mesh->vertex3f[vnum * 3 + 1] = vertex3f[1];
	mesh->vertex3f[vnum * 3 + 2] = vertex3f[2];
	return vnum;
}

void Mod_ShadowMesh_AddMesh(shadowmesh_t *mesh, const float *vertex3f, int numtris, const int *element3i)
{
	int i;

	for (i = 0;i < numtris;i++)
	{
#if 1 // June 2
		// Baker: A nice macro bones wrote to simplify this ...
		if ((mesh->numtriangles * 3 + 2) * sizeof(int) + 1 >= Mem_Size(mesh->element3i))
#else
		// Signed-off-by: bones_was_here <bones_was_here@xonotic.au>
		if ((mesh->numtriangles * 3 + 2) * sizeof(int) + 1 >=
			((memheader_t *)((unsigned char *)mesh->element3i - sizeof(memheader_t)))->size)
#endif
		{
			// FIXME: we didn't allocate enough space for all the tris, see R_Mod_CompileShadowMap
			Con_PrintLinef (CON_WARN "Mod_ShadowMesh_AddMesh: insufficient memory allocated!");
			return;
		}

		mesh->element3i[mesh->numtriangles * 3 + 0] = Mod_ShadowMesh_AddVertex(mesh, vertex3f + 3 * element3i[i * 3 + 0]);
		mesh->element3i[mesh->numtriangles * 3 + 1] = Mod_ShadowMesh_AddVertex(mesh, vertex3f + 3 * element3i[i * 3 + 1]);
		mesh->element3i[mesh->numtriangles * 3 + 2] = Mod_ShadowMesh_AddVertex(mesh, vertex3f + 3 * element3i[i * 3 + 2]);
		mesh->numtriangles++;
	}

	// the triangle calculation can take a while, so let's do a keepalive here
	CL_KeepaliveMessage(false);
}

shadowmesh_t *Mod_ShadowMesh_Begin(mempool_t *mempool, int maxverts, int maxtriangles)
{
	// the preparation before shadow mesh initialization can take a while, so let's do a keepalive here
	CL_KeepaliveMessage(false);

	return Mod_ShadowMesh_Alloc(mempool, maxverts, maxtriangles);
}

static void Mod_ShadowMesh_CreateVBOs(shadowmesh_t *mesh)
{
	if (!mesh->numverts)
		return;

	// make sure we don't crash inside the driver if something went wrong, as it's annoying to debug
	Mod_ValidateElements(mesh->element3i, mesh->element3s, mesh->numtriangles, 0, mesh->numverts, __FILE__, __LINE__);

	// upload short indices as a buffer
	if (mesh->element3s && !mesh->element3s_indexbuffer)
		mesh->element3s_indexbuffer = R_Mesh_CreateMeshBuffer(mesh->element3s, mesh->numtriangles * sizeof(short[3]), "shadowmesh", true, false, false, true);

	// upload int indices as a buffer
	if (mesh->element3i && !mesh->element3i_indexbuffer && !mesh->element3s)
		mesh->element3i_indexbuffer = R_Mesh_CreateMeshBuffer(mesh->element3i, mesh->numtriangles * sizeof(int[3]), "shadowmesh", true, false, false, false);

	// vertex buffer is several arrays and we put them in the same buffer
	//
	// is this wise?  the texcoordtexture2f array is used with dynamic
	// vertex/svector/tvector/normal when rendering animated models, on the
	// other hand animated models don't use a lot of vertices anyway...
	if (!mesh->vbo_vertexbuffer)
	{
		mesh->vbooffset_vertex3f = 0;
		mesh->vbo_vertexbuffer = R_Mesh_CreateMeshBuffer(mesh->vertex3f, mesh->numverts * sizeof(float[3]), "shadowmesh", false, false, false, false);
	}
}

shadowmesh_t *Mod_ShadowMesh_Finish(shadowmesh_t *mesh, qbool createvbo)
{
	if (mesh->numverts >= 3 && mesh->numtriangles >= 1)
	{
		if (mesh->vertexhashentries)
			Mem_Free(mesh->vertexhashentries);
		mesh->vertexhashentries = NULL;
		if (mesh->vertexhashtable)
			Mem_Free(mesh->vertexhashtable);
		mesh->vertexhashtable = NULL;
		if (mesh->maxverts > mesh->numverts)
		{
			mesh->vertex3f = (float *)Mem_Realloc(mesh->mempool, mesh->vertex3f, mesh->numverts * sizeof(float[3]));
			mesh->maxverts = mesh->numverts;
		}
		if (mesh->maxtriangles > mesh->numtriangles)
		{
			mesh->element3i = (int *)Mem_Realloc(mesh->mempool, mesh->element3i, mesh->numtriangles * sizeof(int[3]));
			mesh->maxtriangles = mesh->numtriangles;
		}
		if (mesh->numverts <= 65536)
		{
			int i;
			mesh->element3s = (unsigned short *)Mem_Alloc(mesh->mempool, mesh->numtriangles * sizeof(unsigned short[3]));
			for (i = 0;i < mesh->numtriangles*3;i++)
				mesh->element3s[i] = mesh->element3i[i];
		}
		if (createvbo)
			Mod_ShadowMesh_CreateVBOs(mesh);
	}

	// this can take a while, so let's do a keepalive here
	CL_KeepaliveMessage(false);

	return mesh;
}

void Mod_ShadowMesh_CalcBBox(shadowmesh_t *mesh, vec3_t mins, vec3_t maxs, vec3_t center, float *radius)
{
	int i;
	vec3_t nmins, nmaxs, ncenter, temp;
	float nradius2, dist2, *v;
	VectorClear(nmins);
	VectorClear(nmaxs);
	// calculate bbox
	VectorCopy(mesh->vertex3f, nmins);
	VectorCopy(mesh->vertex3f, nmaxs);
	for (i = 0, v = mesh->vertex3f;i < mesh->numverts;i++, v += 3)
	{
		if (nmins[0] > v[0]) { nmins[0] = v[0]; } if (nmaxs[0] < v[0]) { nmaxs[0] = v[0]; }
		if (nmins[1] > v[1]) { nmins[1] = v[1]; } if (nmaxs[1] < v[1]) { nmaxs[1] = v[1]; }
		if (nmins[2] > v[2]) { nmins[2] = v[2]; } if (nmaxs[2] < v[2]) { nmaxs[2] = v[2]; }
	}
	// calculate center and radius
	ncenter[0] = (nmins[0] + nmaxs[0]) * 0.5f;
	ncenter[1] = (nmins[1] + nmaxs[1]) * 0.5f;
	ncenter[2] = (nmins[2] + nmaxs[2]) * 0.5f;
	nradius2 = 0;
	for (i = 0, v = mesh->vertex3f;i < mesh->numverts;i++, v += 3)
	{
		VectorSubtract(v, ncenter, temp);
		dist2 = DotProduct(temp, temp);
		if (nradius2 < dist2)
			nradius2 = dist2;
	}
	// return data
	if (mins)
		VectorCopy(nmins, mins);
	if (maxs)
		VectorCopy(nmaxs, maxs);
	if (center)
		VectorCopy(ncenter, center);
	if (radius)
		*radius = sqrt(nradius2);
}

void Mod_ShadowMesh_Free(shadowmesh_t *mesh)
{
	if (mesh->element3i_indexbuffer)
		R_Mesh_DestroyMeshBuffer(mesh->element3i_indexbuffer);
	if (mesh->element3s_indexbuffer)
		R_Mesh_DestroyMeshBuffer(mesh->element3s_indexbuffer);
	if (mesh->vbo_vertexbuffer)
		R_Mesh_DestroyMeshBuffer(mesh->vbo_vertexbuffer);
	if (mesh->vertex3f)
		Mem_Free(mesh->vertex3f);
	if (mesh->element3i)
		Mem_Free(mesh->element3i);
	if (mesh->element3s)
		Mem_Free(mesh->element3s);
	if (mesh->vertexhashentries)
		Mem_Free(mesh->vertexhashentries);
	if (mesh->vertexhashtable)
		Mem_Free(mesh->vertexhashtable);
	Mem_Free(mesh);
}

void Mod_CreateCollisionMesh(model_t *mod)
{
	int k, numcollisionmeshtriangles;
	qbool usesinglecollisionmesh = false;
	const msurface_t *surface = NULL;

	mempool_t *mempool = mod->mempool;
	if (!mempool && mod->brush.parentmodel)
		mempool = mod->brush.parentmodel->mempool;
	// make a single combined collision mesh for physics engine use
	// TODO rewrite this to use the collision brushes as source, to fix issues with e.g. common/caulk which creates no drawsurface
	numcollisionmeshtriangles = 0;
	for (k = mod->submodelsurfaces_start;k < mod->submodelsurfaces_end;k++)
	{
		surface = mod->data_surfaces + k;
		if (String_Match(surface->texture->name, "collision") || String_Match(surface->texture->name, "collisionconvex")) // found collision mesh
		{
			usesinglecollisionmesh = true;
			numcollisionmeshtriangles = surface->num_triangles;
			break;
		}
		if (!(surface->texture->supercontents & SUPERCONTENTS_SOLID))
			continue;
		numcollisionmeshtriangles += surface->num_triangles;
	}
	mod->brush.collisionmesh = Mod_ShadowMesh_Begin(mempool, numcollisionmeshtriangles * 3, numcollisionmeshtriangles);
	if (usesinglecollisionmesh)
		Mod_ShadowMesh_AddMesh(mod->brush.collisionmesh, mod->surfmesh.data_vertex3f, surface->num_triangles, (mod->surfmesh.data_element3i + 3 * surface->num_firsttriangle));
	else
	{
		for (k = mod->submodelsurfaces_start; k < mod->submodelsurfaces_end; k++)
		{
			surface = mod->data_surfaces + k;
			if (!(surface->texture->supercontents & SUPERCONTENTS_SOLID))
				continue;
			Mod_ShadowMesh_AddMesh(mod->brush.collisionmesh, mod->surfmesh.data_vertex3f, surface->num_triangles, (mod->surfmesh.data_element3i + 3 * surface->num_firsttriangle));
		}
	}
	mod->brush.collisionmesh = Mod_ShadowMesh_Finish(mod->brush.collisionmesh, false);
}

#if 0
static void Mod_GetTerrainVertex3fTexCoord2fFromBGRA(const unsigned char *imagepixels, int imagewidth, int imageheight, int ix, int iy, float *vertex3f, float *texcoord2f, matrix4x4_t *pixelstepmatrix, matrix4x4_t *pixeltexturestepmatrix)
{
	float v[3], tc[3];
	v[0] = ix;
	v[1] = iy;
	if (ix >= 0 && iy >= 0 && ix < imagewidth && iy < imageheight)
		v[2] = (imagepixels[((iy*imagewidth)+ix)*4+0] + imagepixels[((iy*imagewidth)+ix)*4+1] + imagepixels[((iy*imagewidth)+ix)*4+2]) * (1.0f / 765.0f);
	else
		v[2] = 0;
	Matrix4x4_Transform(pixelstepmatrix, v, vertex3f);
	Matrix4x4_Transform(pixeltexturestepmatrix, v, tc);
	texcoord2f[0] = tc[0];
	texcoord2f[1] = tc[1];
}

static void Mod_GetTerrainVertexFromBGRA(const unsigned char *imagepixels, int imagewidth, int imageheight, int ix, int iy, float *vertex3f, float *svector3f, float *tvector3f, float *normal3f, float *texcoord2f, matrix4x4_t *pixelstepmatrix, matrix4x4_t *pixeltexturestepmatrix)
{
	float vup[3], vdown[3], vleft[3], vright[3];
	float tcup[3], tcdown[3], tcleft[3], tcright[3];
	float sv[3], tv[3], nl[3];
	Mod_GetTerrainVertex3fTexCoord2fFromBGRA(imagepixels, imagewidth, imageheight, ix, iy, vertex3f, texcoord2f, pixelstepmatrix, pixeltexturestepmatrix);
	Mod_GetTerrainVertex3fTexCoord2fFromBGRA(imagepixels, imagewidth, imageheight, ix, iy - 1, vup, tcup, pixelstepmatrix, pixeltexturestepmatrix);
	Mod_GetTerrainVertex3fTexCoord2fFromBGRA(imagepixels, imagewidth, imageheight, ix, iy + 1, vdown, tcdown, pixelstepmatrix, pixeltexturestepmatrix);
	Mod_GetTerrainVertex3fTexCoord2fFromBGRA(imagepixels, imagewidth, imageheight, ix - 1, iy, vleft, tcleft, pixelstepmatrix, pixeltexturestepmatrix);
	Mod_GetTerrainVertex3fTexCoord2fFromBGRA(imagepixels, imagewidth, imageheight, ix + 1, iy, vright, tcright, pixelstepmatrix, pixeltexturestepmatrix);
	Mod_BuildBumpVectors(vertex3f, vup, vright, texcoord2f, tcup, tcright, svector3f, tvector3f, normal3f);
	Mod_BuildBumpVectors(vertex3f, vright, vdown, texcoord2f, tcright, tcdown, sv, tv, nl);
	VectorAdd(svector3f, sv, svector3f);
	VectorAdd(tvector3f, tv, tvector3f);
	VectorAdd(normal3f, nl, normal3f);
	Mod_BuildBumpVectors(vertex3f, vdown, vleft, texcoord2f, tcdown, tcleft, sv, tv, nl);
	VectorAdd(svector3f, sv, svector3f);
	VectorAdd(tvector3f, tv, tvector3f);
	VectorAdd(normal3f, nl, normal3f);
	Mod_BuildBumpVectors(vertex3f, vleft, vup, texcoord2f, tcleft, tcup, sv, tv, nl);
	VectorAdd(svector3f, sv, svector3f);
	VectorAdd(tvector3f, tv, tvector3f);
	VectorAdd(normal3f, nl, normal3f);
}

static void Mod_ConstructTerrainPatchFromBGRA(const unsigned char *imagepixels, int imagewidth, int imageheight, int x1, int y1, int width, int height, int *element3i, float *vertex3f, float *svector3f, float *tvector3f, float *normal3f, float *texcoord2f, matrix4x4_t *pixelstepmatrix, matrix4x4_t *pixeltexturestepmatrix)
{
	int x, y, ix, iy, *e;
	e = element3i;
	for (y = 0;y < height;y++)
	{
		for (x = 0;x < width;x++)
		{
			e[0] = (y + 1) * (width + 1) + (x + 0);
			e[1] = (y + 0) * (width + 1) + (x + 0);
			e[2] = (y + 1) * (width + 1) + (x + 1);
			e[3] = (y + 0) * (width + 1) + (x + 0);
			e[4] = (y + 0) * (width + 1) + (x + 1);
			e[5] = (y + 1) * (width + 1) + (x + 1);
			e += 6;
		}
	}
	for (y = 0, iy = y1;y < height + 1;y++, iy++)
		for (x = 0, ix = x1;x < width + 1;x++, ix++, vertex3f += 3, texcoord2f += 2, svector3f += 3, tvector3f += 3, normal3f += 3)
			Mod_GetTerrainVertexFromBGRA(imagepixels, imagewidth, imageheight, ix, iy, vertex3f, texcoord2f, svector3f, tvector3f, normal3f, pixelstepmatrix, pixeltexturestepmatrix);
}
#endif

#if 0
void Mod_Terrain_SurfaceRecurseChunk(model_t *model, int stepsize, int x, int y)
{
	float mins[3];
	float maxs[3];
	float chunkwidth = min(stepsize, model->terrain.width - 1 - x);
	float chunkheight = min(stepsize, model->terrain.height - 1 - y);
	float viewvector[3];
	unsigned int firstvertex;
	unsigned int *e;
	float *v;
	if (chunkwidth < 2 || chunkheight < 2)
		return;
	VectorSet(mins, model->terrain.mins[0] +  x    * stepsize * model->terrain.scale[0], model->terrain.mins[1] +  y    * stepsize * model->terrain.scale[1], model->terrain.mins[2]);
	VectorSet(maxs, model->terrain.mins[0] + (x+1) * stepsize * model->terrain.scale[0], model->terrain.mins[1] + (y+1) * stepsize * model->terrain.scale[1], model->terrain.maxs[2]);
	viewvector[0] = bound(mins[0], localvieworigin, maxs[0]) - model->terrain.vieworigin[0];
	viewvector[1] = bound(mins[1], localvieworigin, maxs[1]) - model->terrain.vieworigin[1];
	viewvector[2] = bound(mins[2], localvieworigin, maxs[2]) - model->terrain.vieworigin[2];
	if (stepsize > 1 && VectorLength(viewvector) < stepsize*model->terrain.scale[0]*r_terrain_lodscale.value)
	{
		// too close for this stepsize, emit as 4 chunks instead
		stepsize /= 2;
		Mod_Terrain_SurfaceRecurseChunk(model, stepsize, x, y);
		Mod_Terrain_SurfaceRecurseChunk(model, stepsize, x+stepsize, y);
		Mod_Terrain_SurfaceRecurseChunk(model, stepsize, x, y+stepsize);
		Mod_Terrain_SurfaceRecurseChunk(model, stepsize, x+stepsize, y+stepsize);
		return;
	}
	// emit the geometry at stepsize into our vertex buffer / index buffer
	// we add two columns and two rows for skirt
	outwidth = chunkwidth+2;
	outheight = chunkheight+2;
	outwidth2 = outwidth-1;
	outheight2 = outheight-1;
	outwidth3 = outwidth+1;
	outheight3 = outheight+1;
	firstvertex = numvertices;
	e = model->terrain.element3i + numtriangles;
	numtriangles += chunkwidth*chunkheight*2+chunkwidth*2*2+chunkheight*2*2;
	v = model->terrain.vertex3f + numvertices;
	numvertices += (chunkwidth+1)*(chunkheight+1)+(chunkwidth+1)*2+(chunkheight+1)*2;
	// emit the triangles (note: the skirt is treated as two extra rows and two extra columns)
	for (ty = 0;ty < outheight;ty++)
	{
		for (tx = 0;tx < outwidth;tx++)
		{
			*e++ = firstvertex + (ty  )*outwidth3+(tx  );
			*e++ = firstvertex + (ty  )*outwidth3+(tx+1);
			*e++ = firstvertex + (ty+1)*outwidth3+(tx+1);
			*e++ = firstvertex + (ty  )*outwidth3+(tx  );
			*e++ = firstvertex + (ty+1)*outwidth3+(tx+1);
			*e++ = firstvertex + (ty+1)*outwidth3+(tx  );
		}
	}
	// TODO: emit surface vertices (x+tx*stepsize, y+ty*stepsize)
	for (ty = 0;ty <= outheight;ty++)
	{
		skirtrow = ty == 0 || ty == outheight;
		ry = y+bound(1, ty, outheight)*stepsize;
		for (tx = 0;tx <= outwidth;tx++)
		{
			skirt = skirtrow || tx == 0 || tx == outwidth;
			rx = x+bound(1, tx, outwidth)*stepsize;
			v[0] = rx*scale[0];
			v[1] = ry*scale[1];
			v[2] = heightmap[ry*terrainwidth+rx]*scale[2];
			v += 3;
		}
	}
	// TODO: emit skirt vertices
}

void Mod_Terrain_UpdateSurfacesForViewOrigin(model_t *model)
{
	for (y = 0;y < model->terrain.size[1];y += model->terrain.
	Mod_Terrain_SurfaceRecurseChunk(model, model->terrain.maxstepsize, x, y);
	Mod_Terrain_BuildChunk(model,
}
#endif

static int Mod_LoadQ3Shaders_EnumerateWaveFunc(const char *s)
{
	int offset = 0;
	if (!strncasecmp(s, "user", 4)) // parse stuff like "user1sin", always user<n>func
	{
		offset = bound(0, s[4] - '0', 9);
		offset = (offset + 1) << Q3WAVEFUNC_USER_SHIFT;
		s += 4;
		if (*s)
			++s;
	}
	if (String_Match_Caseless(s, "sin"))             return offset | Q3WAVEFUNC_SIN;
	if (String_Match_Caseless(s, "square"))          return offset | Q3WAVEFUNC_SQUARE;
	if (String_Match_Caseless(s, "triangle"))        return offset | Q3WAVEFUNC_TRIANGLE;
	if (String_Match_Caseless(s, "sawtooth"))        return offset | Q3WAVEFUNC_SAWTOOTH;
	if (String_Match_Caseless(s, "inversesawtooth")) return offset | Q3WAVEFUNC_INVERSESAWTOOTH;
	if (String_Match_Caseless(s, "noise"))           return offset | Q3WAVEFUNC_NOISE;
	if (String_Match_Caseless(s, "none"))            return offset | Q3WAVEFUNC_NONE;
	Con_DPrintLinef ("Mod_LoadQ3Shaders: unknown wavefunc %s", s);
	return offset | Q3WAVEFUNC_NONE;
}

void Mod_FreeQ3Shaders(void)
{
	Mem_FreePool(&q3shaders_mem);
}

static void Q3Shader_AddToHash (shader_t *shader)
{
	unsigned short hash = CRC_Block_CaseInsensitive ((const unsigned char *)shader->name, strlen (shader->name));
	q3shader_hash_entry_t *entry = q3shader_data->hash + (hash % Q3SHADER_HASH_SIZE_1021);
	q3shader_hash_entry_t *lastEntry = NULL;
	do
	{
		if (String_Match_Caseless (entry->shader.name, shader->name))
		{
			// redeclaration
			if (shader->dpshaderkill)
			{
				// killed shader is a redeclarion? we can safely ignore it
				return;
			}
			else if (entry->shader.dpshaderkill)
			{
				// replace the old shader!
				// this will skip the entry allocating part
				// below and just replace the shader
				break;
			}
			else
			{
				unsigned char *start, *end, *start2;
				start = (unsigned char *) (&shader->Q3SHADERINFO_COMPARE_START);
				end = ((unsigned char *) (&shader->Q3SHADERINFO_COMPARE_END)) + sizeof(shader->Q3SHADERINFO_COMPARE_END);
				start2 = (unsigned char *) (&entry->shader.Q3SHADERINFO_COMPARE_START);
				if (memcmp(start, start2, end - start))
					Con_DPrintf ("Shader '%s' already defined, ignoring mismatching redeclaration\n", shader->name);
				else
					Con_DPrintf ("Shader '%s' already defined\n", shader->name);
				return;
			}
		}
		lastEntry = entry;
		entry = entry->chain;
	}
	while (entry != NULL);
	if (entry == NULL)
	{
		if (lastEntry->shader.name[0] != 0)
		{
			/* Add to chain */
			q3shader_hash_entry_t *newEntry = (q3shader_hash_entry_t*)
			  Mem_ExpandableArray_AllocRecord (&q3shader_data->hash_entries);

			while (lastEntry->chain != NULL) lastEntry = lastEntry->chain;
			lastEntry->chain = newEntry;
			newEntry->chain = NULL;
			lastEntry = newEntry;
		}
		/* else: head of chain, in hash entry array */
		entry = lastEntry;
	}
	memcpy (&entry->shader, shader, sizeof (shader_t));
}

void Mod_LoadQ3Shaders(void)
{
	int j;
	int fileindex;
	fssearch_t *search;
	char *f;
	const char *text;
	shader_t shader;
	q3shaderinfo_layer_t *layer;
	int numparameters;
	char parameter[TEXTURE_MAXFRAMES_64 + 4][Q3PATHLENGTH_64];
	char *custsurfaceparmnames[256]; // VorteX: q3map2 has 64 but well, someone will need more
	unsigned long custsurfaceflags[256];
	int numcustsurfaceflags;
	qbool dpshaderkill;
	int i, tcmodindex;

	Mod_FreeQ3Shaders();

	q3shaders_mem = Mem_AllocPool("q3shaders", 0, NULL);
	q3shader_data = (q3shader_data_t*)Mem_Alloc (q3shaders_mem,
		sizeof (q3shader_data_t));
	Mem_ExpandableArray_NewArray (&q3shader_data->hash_entries,
		q3shaders_mem, sizeof (q3shader_hash_entry_t), 256);
	Mem_ExpandableArray_NewArray (&q3shader_data->char_ptrs,
		q3shaders_mem, sizeof (char **), 256);

	// parse custinfoparms.txt
	// Baker: Blood Omnicide uses this file as such ...
	// Surfaceflags
	// The shader checks for these keywords shader.surfaceflags
	// Script example?
	//models/pushables/stone1
	//{
	//	dp_glossexponentmod 0.25
	//	surfaceparm stone
	//	dpmeshcollisions
	//	{
	//		map models/pushables/stone1
	//	}
	//	{
	//		map $lightmap
	//	}
	//}

	//{
	//grass 0x00000001 // 1 = NODAMAGE
	//ice 0x00000002 // 2 = SLICK
	//dirt 0x00000003 // 3 = SLICK + NODAMAGE
	//wood 0x00000008 // 8 = LADDER
	//oldwood 0x00000009 // 9 = LADDER + NODAMAGE
	//ground 0x0000000a // 10 = LADDER + SLICK
	//sand 0x0000000b // 11 = LADDER + SLICK + NODAMAGE
	//dullmetal 0x00000040 // 64 = FLESH
	//brookwater 0x00000041 // 65 = FLESH + NODAMAGE
	//glass 0x00000042 // 66 = FLESH + SLICK
	//fur 0x00000043 // 67 = FLESH + SLICK + NODAMAGE
	//marble 0x00000048 // 72 = FLESH + LADDER
	//hay 0x00000049 // 73 = FLESH + LADDER + NODAMAGE
	//slate 0x0000004a // 74 = FLESH + LADDER + SLICK
	//flesh 0x0000004b // 75 = FLESH + LADDER + SLICK + NODAMAGE
	//metal 0x00001000 // 4096 = METALSTEPS
	//oldmetal 0x00001001 // 4097 = METALSTEPS + NODAMAGE
	//stone 0x00001002 // 4098 = METALSTEPS + SLICK
	//oldstone 0x00001003 // 4099 = METALSTEPS + SLICK + NODAMAGE
	//plant 0x00001008 // 4104 = METALSTEPS + LADDER
	//}


	numcustsurfaceflags = 0;
	if ((text = f = (char *)FS_LoadFile("scripts/custinfoparms.txt", tempmempool, fs_quiet_FALSE, fs_size_ptr_null)) != NULL)
	{
		//if (!COM_ParseToken_QuakeC(&text, false) || strcasecmp(com_token, "{"))
		if (!COM_ParseToken_QuakeC(&text, false) || String_NOT_Match(com_token, "{")) {
			Con_DPrintLinef ("scripts/custinfoparms.txt: contentflags section parsing error - expected " 
								QUOTED_STR("{") ", found " QUOTED_S, com_token);
		}
		else
		{
			while (COM_ParseToken_QuakeC(&text, false))
				if (String_Match_Caseless(com_token, "}"))
					break;
			// custom surfaceflags section
			//if (!COM_ParseToken_QuakeC(&text, false) || strcasecmp(com_token, "{"))
			if (!COM_ParseToken_QuakeC(&text, false) || false == String_NOT_Match(com_token, "{"))
				Con_DPrintLinef ("scripts/custinfoparms.txt: surfaceflags section parsing error - expected " QUOTED_STR("{") ", found " QUOTED_S, com_token);
			else
			{
				while(COM_ParseToken_QuakeC(&text, false)) {
					if (String_Match_Caseless(com_token, "}"))
						break;
					// register surfaceflag
					if (numcustsurfaceflags >= 256) {
						Con_PrintLinef ("scripts/custinfoparms.txt: surfaceflags section parsing error - max 256 surfaceflags exceeded");
						break;
					}
					// name
					j = (int)strlen(com_token)+1;
					custsurfaceparmnames[numcustsurfaceflags] = (char *)Mem_Alloc(tempmempool, j);
					strlcpy(custsurfaceparmnames[numcustsurfaceflags], com_token, j+1);
					// value
					if (COM_ParseToken_QuakeC(&text, false))
						custsurfaceflags[numcustsurfaceflags] = strtol(com_token, NULL, 0);
					else
						custsurfaceflags[numcustsurfaceflags] = 0;
					numcustsurfaceflags++;
				} // while
			}
		}
		Mem_Free(f);
	}

	// parse shaders
	search = FS_Search("scripts/*.shader", fs_caseless_true, fs_quiet_FALSE, fs_pakfile_null, fs_gamedironly_false);
	if (!search)
		return;
	for (fileindex = 0;fileindex < search->numfilenames;fileindex++) {
		text = f = (char *)FS_LoadFile(search->filenames[fileindex], tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
		if (!f)
			continue;
		while (COM_ParseToken_QuakeC(&text, false)) {
			memset (&shader, 0, sizeof(shader));
			shader.name[0] = 0;
			shader.surfaceparms = 0;
			shader.surfaceflags = 0;
			shader.textureflags = 0;
			shader.numlayers = 0;
			shader.lighting = false;
			shader.vertexalpha = false;
			shader.textureblendalpha = false;
			shader.skyboxname[0] = 0;
			shader.deforms[0].deform = Q3DEFORM_NONE;
			shader.dpnortlight = false;
			shader.dpshadow = false;
			shader.dpnoshadow = false;
			shader.dpmeshcollisions = false;
			shader.dpshaderkill = false;
			shader.dpreflectcube[0] = 0;
			shader.reflectmin = 0;
			shader.reflectmax = 1;
			shader.refractfactor = 1;
			Vector4Set(shader.refractcolor4f, 1, 1, 1, 1);
			shader.reflectfactor = 1;
			Vector4Set(shader.reflectcolor4f, 1, 1, 1, 1);
			shader.r_water_wateralpha = 1;
			shader.r_water_waterscroll[0] = 0;
			shader.r_water_waterscroll[1] = 0;
			shader.offsetmapping = (mod_q3shader_default_offsetmapping.value) ? OFFSETMAPPING_DEFAULT : OFFSETMAPPING_OFF;
			shader.offsetscale = mod_q3shader_default_offsetmapping_scale.value;
			shader.offsetbias = mod_q3shader_default_offsetmapping_bias.value;
			shader.biaspolygonoffset = mod_q3shader_default_polygonoffset.value;
			shader.biaspolygonfactor = mod_q3shader_default_polygonfactor.value;
			shader.transparentsort = TRANSPARENTSORT_DISTANCE;
			shader.specularscalemod = 1;
			shader.specularpowermod = 1;
			shader.rtlightambient = 0;
			// WHEN ADDING DEFAULTS HERE, REMEMBER TO PUT DEFAULTS IN ALL LOADERS
			// JUST GREP FOR "specularscalemod = 1".

			c_strlcpy(shader.name, com_token);
			if (!COM_ParseToken_QuakeC(&text, false) || String_NOT_Match(com_token, "{")) {
				Con_DPrintLinef ("%s parsing error - expected " QUOTED_STR ("{") ", found " QUOTED_S, search->filenames[fileindex], com_token);
				break;
			}
			while (COM_ParseToken_QuakeC(&text, /*retnewline?*/ false)) {
				// Baker: Separated out the 500 lines of code to separate file
				// to make it easier to work with.
				#include "model_shared_q3shader_loop.c.h"
			} // while
			// hide this shader if a cvar said it should be killed
			if (shader.dpshaderkill)
				shader.numlayers = 0;
			// fix up multiple reflection types
			if (shader.textureflags & Q3TEXTUREFLAG_WATERSHADER)
				shader.textureflags &= ~(Q3TEXTUREFLAG_REFRACTION | Q3TEXTUREFLAG_REFLECTION | Q3TEXTUREFLAG_CAMERA);

			Q3Shader_AddToHash (&shader);
		} // while token
		Mem_Free(f);
	} // for
	FS_FreeSearch(search);
	// free custinfoparm values
	for (j = 0; j < numcustsurfaceflags; j++)
		Mem_Free(custsurfaceparmnames[j]);
}

shader_t *Mod_LookupQ3Shader(const char *name)
{
	unsigned short hash;
	q3shader_hash_entry_t *entry;
	if (!q3shaders_mem)
		Mod_LoadQ3Shaders();

	hash = CRC_Block_CaseInsensitive ((const unsigned char *)name, strlen (name));
	entry = q3shader_data->hash + (hash % Q3SHADER_HASH_SIZE_1021);
	while (entry != NULL) {
		if (String_Starts_With_Caseless (entry->shader.name, name))
			return &entry->shader;
		entry = entry->chain;
	} // while
	return NULL;
}

texture_shaderpass_t *Mod_CreateShaderPass(mempool_t *mempool, skinframe_t *skinframe)
{
	texture_shaderpass_t *shaderpass = (texture_shaderpass_t *)Mem_Alloc(mempool, sizeof(*shaderpass));
	shaderpass->animframerate = 0.0f;
	shaderpass->numframes = 1;
	shaderpass->blendfunc[0] = GL_ONE;
	shaderpass->blendfunc[1] = GL_ZERO;
	shaderpass->rgbgen.rgbgen = Q3RGBGEN_IDENTITY;
	shaderpass->alphagen.alphagen = Q3ALPHAGEN_IDENTITY;
	shaderpass->alphatest = false;
	shaderpass->tcgen.tcgen = Q3TCGEN_TEXTURE;
	shaderpass->skinframes[0] = skinframe;
	return shaderpass;
}

texture_shaderpass_t *Mod_CreateShaderPassFromQ3ShaderLayer(mempool_t *mempool, const char *modelname, q3shaderinfo_layer_t *layer, int layerindex, int texflags, const char *texturename)
{
	int j;
	texture_shaderpass_t *shaderpass = (texture_shaderpass_t *)Mem_Alloc(mempool, sizeof(*shaderpass));
	shaderpass->alphatest = layer->alphatest != 0;
	shaderpass->animframerate = layer->animframerate;
	shaderpass->numframes = layer->sh_numframes;
	shaderpass->blendfunc[0] = layer->blendfunc[0];
	shaderpass->blendfunc[1] = layer->blendfunc[1];
	shaderpass->rgbgen = layer->rgbgen;
	shaderpass->alphagen = layer->alphagen;
	shaderpass->tcgen = layer->tcgen;
	for (j = 0; j < Q3MAXTCMODS_8 && layer->tcmods[j].tcmod != Q3TCMOD_NONE; j++)
		shaderpass->tcmods[j] = layer->tcmods[j];
	for (j = 0; j < layer->sh_numframes; j++)
		shaderpass->skinframes[j] = R_SkinFrame_LoadExternal(layer->sh_ptexturename[j], texflags, q_tx_complain_false, q_tx_fallback_notexture_true);
	return shaderpass;
}

qbool Mod_LoadTextureFromQ3Shader(mempool_t *mempool, const char *modelname, texture_t *texture, const char *name, qbool warnmissing,
								  qbool fallback, int shall_do_external, int defaulttexflags, int defaultmaterialflags)
{
	int texflagsmask, texflagsor;
	qbool success = true;
	shader_t *shader;
	if (!name)
		name = "";
	c_strlcpy (texture->name, name);
	texture->basealpha = 1.0f;
	shader = name[0] ? Mod_LookupQ3Shader(name) : NULL;

	// allow disabling of picmip or compression by defaulttexflags
	texflagsmask = ~0;
	if (!(defaulttexflags & TEXF_PICMIP))
		texflagsmask &= ~TEXF_PICMIP;
	if (!(defaulttexflags & TEXF_COMPRESS))
		texflagsmask &= ~TEXF_COMPRESS;
	texflagsor = 0;
	if (defaulttexflags & TEXF_ISWORLD)
		texflagsor |= TEXF_ISWORLD;
	if (defaulttexflags & TEXF_ISSPRITE)
		texflagsor |= TEXF_ISSPRITE;
	// unless later loaded from the shader
	texture->offsetmapping = (mod_noshader_default_offsetmapping.value) /*defaults 1*/ ? OFFSETMAPPING_DEFAULT : OFFSETMAPPING_OFF;
	texture->offsetscale = 1;
	texture->offsetbias = 0;
	texture->specularscalemod = 1;
	texture->specularpowermod = 1;
	texture->rtlightambient = 0;
	texture->transparentsort = TRANSPARENTSORT_DISTANCE;
	// WHEN ADDING DEFAULTS HERE, REMEMBER TO PUT DEFAULTS IN ALL LOADERS
	// JUST GREP FOR "specularscalemod = 1".

	if (shader)
	{
		if (developer_loading.integer)
			Con_PrintLinef ("%s: loaded shader for %s", modelname, name);

		if (shader->surfaceparms & Q3SURFACEPARM_SKY)
		{
			texture->basematerialflags = MATERIALFLAG_SKY;
			if (shader->skyboxname[0] && loadmodel)
			{
				// quake3 seems to append a _ to the skybox name, so this must do so as well
				dpsnprintf(loadmodel->brush.skybox, sizeof(loadmodel->brush.skybox), "%s_", shader->skyboxname);
			}
		}
		else if ((texture->surfaceflags & Q3SURFACEFLAG_NODRAW) || shader->numlayers == 0)
			texture->basematerialflags = MATERIALFLAG_NODRAW | MATERIALFLAG_NOSHADOW;
		else
			texture->basematerialflags = MATERIALFLAG_WALL;

		if (shader->layers[0].alphatest)
			texture->basematerialflags |= MATERIALFLAG_ALPHATEST | MATERIALFLAG_NOSHADOW;
		if (shader->textureflags & Q3TEXTUREFLAG_TWOSIDED)
			texture->basematerialflags |= MATERIALFLAG_NOSHADOW | MATERIALFLAG_NOCULLFACE;
		if (shader->textureflags & Q3TEXTUREFLAG_POLYGONOFFSET)
		{
			texture->biaspolygonoffset += shader->biaspolygonoffset;
			texture->biaspolygonfactor += shader->biaspolygonfactor;
		}
		if (shader->textureflags & Q3TEXTUREFLAG_REFRACTION)
			texture->basematerialflags |= MATERIALFLAG_REFRACTION;
		if (shader->textureflags & Q3TEXTUREFLAG_REFLECTION)
			texture->basematerialflags |= MATERIALFLAG_REFLECTION;
		if (shader->textureflags & Q3TEXTUREFLAG_WATERSHADER)
			texture->basematerialflags |= MATERIALFLAG_WATERSHADER;
		if (shader->textureflags & Q3TEXTUREFLAG_CAMERA)
			texture->basematerialflags |= MATERIALFLAG_CAMERA;
		texture->customblendfunc[0] = GL_ONE;
		texture->customblendfunc[1] = GL_ZERO;
		texture->transparentsort = shader->transparentsort;
		if (shader->numlayers > 0)
		{
			texture->customblendfunc[0] = shader->layers[0].blendfunc[0];
			texture->customblendfunc[1] = shader->layers[0].blendfunc[1];
/*
Q3 shader blendfuncs actually used in the game (* = supported by DP)
* additive               GL_ONE GL_ONE
additive weird         GL_ONE GL_SRC_ALPHA
additive weird 2       GL_ONE GL_ONE_MINUS_SRC_ALPHA
* alpha                  GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
alpha inverse          GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
brighten               GL_DST_COLOR GL_ONE
brighten               GL_ONE GL_SRC_COLOR
brighten weird         GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
brighten weird 2       GL_DST_COLOR GL_SRC_ALPHA
* modulate               GL_DST_COLOR GL_ZERO
* modulate               GL_ZERO GL_SRC_COLOR
modulate inverse       GL_ZERO GL_ONE_MINUS_SRC_COLOR
modulate inverse alpha GL_ZERO GL_SRC_ALPHA
modulate weird inverse GL_ONE_MINUS_DST_COLOR GL_ZERO
* modulate x2            GL_DST_COLOR GL_SRC_COLOR
* no blend               GL_ONE GL_ZERO
nothing                GL_ZERO GL_ONE
*/
			// if not opaque, figure out what blendfunc to use
			if (shader->layers[0].blendfunc[0] != GL_ONE || shader->layers[0].blendfunc[1] != GL_ZERO)
			{
				if (shader->layers[0].blendfunc[0] == GL_ONE && shader->layers[0].blendfunc[1] == GL_ONE)
					texture->basematerialflags |= MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
				else if (shader->layers[0].blendfunc[0] == GL_SRC_ALPHA && shader->layers[0].blendfunc[1] == GL_ONE)
					texture->basematerialflags |= MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
				else if (shader->layers[0].blendfunc[0] == GL_SRC_ALPHA && shader->layers[0].blendfunc[1] == GL_ONE_MINUS_SRC_ALPHA)
					texture->basematerialflags |= MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
				else
					texture->basematerialflags |= MATERIALFLAG_CUSTOMBLEND | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
			}
		}
		if (!shader->lighting)
			texture->basematerialflags |= MATERIALFLAG_FULLBRIGHT;

		// here be dragons: convert quake3 shaders to material
		if (shader->numlayers > 0)
		{
			int i;
			int terrainbackgroundlayer = -1;
			int lightmaplayer = -1;
			int alphagenspecularlayer = -1;
			int rgbgenvertexlayer = -1;
			int rgbgendiffuselayer = -1;
			int materiallayer = -1;
			int endofprelayers = 0;
			int firstpostlayer = 0;
			int shaderpassindex = 0;
			for (i = 0; i < shader->numlayers; i ++) {
				if (shader->layers[i].sh_ptexturename != NULL && String_Match_Caseless(shader->layers[i].sh_ptexturename[0], "$lightmap"))
					lightmaplayer = i;
				if (shader->layers[i].rgbgen.rgbgen == Q3RGBGEN_VERTEX)
					rgbgenvertexlayer = i;
				if (shader->layers[i].rgbgen.rgbgen == Q3RGBGEN_LIGHTINGDIFFUSE)
					rgbgendiffuselayer = i;
				if (shader->layers[i].alphagen.alphagen == Q3ALPHAGEN_LIGHTINGSPECULAR)
					alphagenspecularlayer = i;
			}
			if (shader->numlayers >= 2
			 && shader->layers[1].alphagen.alphagen == Q3ALPHAGEN_VERTEX
			 && (shader->layers[0].blendfunc[0] == GL_ONE && shader->layers[0].blendfunc[1] == GL_ZERO && 
			   !shader->layers[0].alphatest)
			 && ((shader->layers[1].blendfunc[0] == GL_SRC_ALPHA && shader->layers[1].blendfunc[1] == 
			   GL_ONE_MINUS_SRC_ALPHA)
				 || (shader->layers[1].blendfunc[0] == GL_ONE && 
				 shader->layers[1].blendfunc[1] == GL_ZERO && shader->layers[1].alphatest)))
			{
				// terrain blend or certain other effects involving alphatest over a regular layer
				terrainbackgroundlayer = 0;
				materiallayer = 1;
				// terrain may be vertex lit (in which case both layers are rgbGen vertex) or
				// lightmapped (in which ase the third layer is lightmap)
				firstpostlayer = lightmaplayer >= 0 ? lightmaplayer + 1 : materiallayer + 1;
			}
			else if (lightmaplayer == 0)
			{
				// ordinary texture but with $lightmap before diffuse
				materiallayer = 1;
				firstpostlayer = lightmaplayer + 2;
			}
			else if (lightmaplayer >= 1)
			{
				// ordinary texture - we don't properly apply lighting to the prelayers, but oh well...
				endofprelayers = lightmaplayer - 1;
				materiallayer = lightmaplayer - 1;
				firstpostlayer = lightmaplayer + 1;
			}
			else if (rgbgenvertexlayer >= 0)
			{
				// map models with baked lighting
				materiallayer = rgbgenvertexlayer;
				endofprelayers = rgbgenvertexlayer;
				firstpostlayer = rgbgenvertexlayer + 1;
				// special case for rgbgen vertex if MATERIALFLAG_VERTEXCOLOR is expected on this material
				if (defaultmaterialflags & MATERIALFLAG_VERTEXCOLOR)
					texture->basematerialflags |= MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX;
			}
			else if (rgbgendiffuselayer >= 0)
			{
				// entity models with dynamic lighting
				materiallayer = rgbgendiffuselayer;
				endofprelayers = rgbgendiffuselayer;
				firstpostlayer = rgbgendiffuselayer + 1;
				// player models often have specular as a pass after diffuse - we don't currently make use of that specular texture (would need to meld it into the skinframe)...
				if (alphagenspecularlayer >= 0)
					firstpostlayer = alphagenspecularlayer + 1;
			}
			else
			{
				// special effects shaders - treat first as primary layer and do everything else as post
				endofprelayers = 0;
				materiallayer = 0;
				firstpostlayer = 1;
			}
			// convert the main material layer
			// FIXME: if alphagenspecularlayer is used, we should pass a specular texture name to R_SkinFrame_LoadExternal and have it load that texture instead of the assumed name for _gloss texture
			if (materiallayer >= 0)
				texture->materialshaderpass = texture->shaderpasses[shaderpassindex++] = Mod_CreateShaderPassFromQ3ShaderLayer(mempool, modelname, &shader->layers[materiallayer], materiallayer, (shader->layers[materiallayer].dptexflags & texflagsmask) | texflagsor, texture->name);
			// convert the terrain background blend layer (if any)
			if (terrainbackgroundlayer >= 0)
				texture->backgroundshaderpass = texture->shaderpasses[shaderpassindex++] = Mod_CreateShaderPassFromQ3ShaderLayer(mempool, modelname, &shader->layers[terrainbackgroundlayer], terrainbackgroundlayer, (shader->layers[terrainbackgroundlayer].dptexflags & texflagsmask) | texflagsor, texture->name);
			// convert the prepass layers (if any)
			texture->startpreshaderpass = shaderpassindex;
			for (i = 0; i < endofprelayers; i++)
				texture->shaderpasses[shaderpassindex++] = Mod_CreateShaderPassFromQ3ShaderLayer(mempool, modelname, &shader->layers[i], i, (shader->layers[i].dptexflags & texflagsmask) | texflagsor, texture->name);
			texture->endpreshaderpass = shaderpassindex;
			texture->startpostshaderpass = shaderpassindex;
			// convert the postpass layers (if any)
			for (i = firstpostlayer; i < shader->numlayers; i++)
				texture->shaderpasses[shaderpassindex++] = Mod_CreateShaderPassFromQ3ShaderLayer(mempool, modelname, &shader->layers[i], i, (shader->layers[i].dptexflags & texflagsmask) | texflagsor, texture->name);
			texture->startpostshaderpass = shaderpassindex;
		}

		if (shader->dpshadow)
			texture->basematerialflags &= ~MATERIALFLAG_NOSHADOW;
		if (shader->dpnoshadow)
			texture->basematerialflags |= MATERIALFLAG_NOSHADOW;
		if (shader->dpnortlight)
			texture->basematerialflags |= MATERIALFLAG_NORTLIGHT;
		if (shader->vertexalpha)
			texture->basematerialflags |= MATERIALFLAG_ALPHAGEN_VERTEX;
		memcpy(texture->deforms, shader->deforms, sizeof(texture->deforms));
		texture->reflectmin = shader->reflectmin;
		texture->reflectmax = shader->reflectmax;
		texture->refractfactor = shader->refractfactor;
		Vector4Copy(shader->refractcolor4f, texture->refractcolor4f);
		texture->reflectfactor = shader->reflectfactor;
		Vector4Copy(shader->reflectcolor4f, texture->reflectcolor4f);
		texture->r_water_wateralpha = shader->r_water_wateralpha;
		Vector2Copy(shader->r_water_waterscroll, texture->r_water_waterscroll);
		texture->offsetmapping = shader->offsetmapping;
		texture->offsetscale = shader->offsetscale;
		texture->offsetbias = shader->offsetbias;
		texture->specularscalemod = shader->specularscalemod;
		texture->specularpowermod = shader->specularpowermod;
		texture->rtlightambient = shader->rtlightambient;
		texture->refractive_index = mod_q3shader_default_refractive_index.value;
		if (shader->dpreflectcube[0])
			texture->reflectcubetexture = R_GetCubemap(shader->dpreflectcube);

		// set up default supercontents (on q3bsp this is overridden by the q3bsp loader)
		texture->supercontents = SUPERCONTENTS_SOLID | SUPERCONTENTS_OPAQUE;
		if (shader->surfaceparms & Q3SURFACEPARM_LAVA         ) texture->supercontents  = SUPERCONTENTS_LAVA         ;
		if (shader->surfaceparms & Q3SURFACEPARM_SLIME        ) texture->supercontents  = SUPERCONTENTS_SLIME        ;
		if (shader->surfaceparms & Q3SURFACEPARM_WATER        ) texture->supercontents  = SUPERCONTENTS_WATER        ;
		if (shader->surfaceparms & Q3SURFACEPARM_NONSOLID     ) texture->supercontents  = 0                          ;
		if (shader->surfaceparms & Q3SURFACEPARM_PLAYERCLIP   ) texture->supercontents  = SUPERCONTENTS_PLAYERCLIP   ;
		if (shader->surfaceparms & Q3SURFACEPARM_BOTCLIP      ) texture->supercontents  = SUPERCONTENTS_MONSTERCLIP  ;
		if (shader->surfaceparms & Q3SURFACEPARM_SKY          ) texture->supercontents  = SUPERCONTENTS_SKY          ;

	//	if (shader->surfaceparms & Q3SURFACEPARM_ALPHASHADOW  ) texture->supercontents |= SUPERCONTENTS_ALPHASHADOW  ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_AREAPORTAL   ) texture->supercontents |= SUPERCONTENTS_AREAPORTAL   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_CLUSTERPORTAL) texture->supercontents |= SUPERCONTENTS_CLUSTERPORTAL;
	//	if (shader->surfaceparms & Q3SURFACEPARM_DETAIL       ) texture->supercontents |= SUPERCONTENTS_DETAIL       ;
		if (shader->surfaceparms & Q3SURFACEPARM_DONOTENTER   ) texture->supercontents |= SUPERCONTENTS_DONOTENTER   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_FOG          ) texture->supercontents |= SUPERCONTENTS_FOG          ;
		if (shader->surfaceparms & Q3SURFACEPARM_LAVA         ) texture->supercontents |= SUPERCONTENTS_LAVA         ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_LIGHTFILTER  ) texture->supercontents |= SUPERCONTENTS_LIGHTFILTER  ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_METALSTEPS   ) texture->supercontents |= SUPERCONTENTS_METALSTEPS   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NODAMAGE     ) texture->supercontents |= SUPERCONTENTS_NODAMAGE     ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NODLIGHT     ) texture->supercontents |= SUPERCONTENTS_NODLIGHT     ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NODRAW       ) texture->supercontents |= SUPERCONTENTS_NODRAW       ;
		if (shader->surfaceparms & Q3SURFACEPARM_NODROP       ) texture->supercontents |= SUPERCONTENTS_NODROP       ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NOIMPACT     ) texture->supercontents |= SUPERCONTENTS_NOIMPACT     ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NOLIGHTMAP   ) texture->supercontents |= SUPERCONTENTS_NOLIGHTMAP   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NOMARKS      ) texture->supercontents |= SUPERCONTENTS_NOMARKS      ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NOMIPMAPS    ) texture->supercontents |= SUPERCONTENTS_NOMIPMAPS    ;
		if (shader->surfaceparms & Q3SURFACEPARM_NONSOLID     ) texture->supercontents &=~SUPERCONTENTS_SOLID        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_ORIGIN       ) texture->supercontents |= SUPERCONTENTS_ORIGIN       ;
		if (shader->surfaceparms & Q3SURFACEPARM_PLAYERCLIP   ) texture->supercontents |= SUPERCONTENTS_PLAYERCLIP   ;
		if (shader->surfaceparms & Q3SURFACEPARM_SKY          ) texture->supercontents |= SUPERCONTENTS_SKY          ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_SLICK        ) texture->supercontents |= SUPERCONTENTS_SLICK        ;
		if (shader->surfaceparms & Q3SURFACEPARM_SLIME        ) texture->supercontents |= SUPERCONTENTS_SLIME        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_STRUCTURAL   ) texture->supercontents |= SUPERCONTENTS_STRUCTURAL   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_TRANS        ) texture->supercontents |= SUPERCONTENTS_TRANS        ;
		if (shader->surfaceparms & Q3SURFACEPARM_WATER        ) texture->supercontents |= SUPERCONTENTS_WATER        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_POINTLIGHT   ) texture->supercontents |= SUPERCONTENTS_POINTLIGHT   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_HINT         ) texture->supercontents |= SUPERCONTENTS_HINT         ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_DUST         ) texture->supercontents |= SUPERCONTENTS_DUST         ;
		if (shader->surfaceparms & Q3SURFACEPARM_BOTCLIP      ) texture->supercontents |= SUPERCONTENTS_BOTCLIP      | SUPERCONTENTS_MONSTERCLIP;
	//	if (shader->surfaceparms & Q3SURFACEPARM_LIGHTGRID    ) texture->supercontents |= SUPERCONTENTS_LIGHTGRID    ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_ANTIPORTAL   ) texture->supercontents |= SUPERCONTENTS_ANTIPORTAL   ;

		texture->surfaceflags = shader->surfaceflags;
		if (shader->surfaceparms & Q3SURFACEPARM_ALPHASHADOW  ) texture->surfaceflags |= Q3SURFACEFLAG_ALPHASHADOW  ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_AREAPORTAL   ) texture->surfaceflags |= Q3SURFACEFLAG_AREAPORTAL   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_CLUSTERPORTAL) texture->surfaceflags |= Q3SURFACEFLAG_CLUSTERPORTAL;
	//	if (shader->surfaceparms & Q3SURFACEPARM_DETAIL       ) texture->surfaceflags |= Q3SURFACEFLAG_DETAIL       ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_DONOTENTER   ) texture->surfaceflags |= Q3SURFACEFLAG_DONOTENTER   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_FOG          ) texture->surfaceflags |= Q3SURFACEFLAG_FOG          ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_LAVA         ) texture->surfaceflags |= Q3SURFACEFLAG_LAVA         ;
		if (shader->surfaceparms & Q3SURFACEPARM_LIGHTFILTER  ) texture->surfaceflags |= Q3SURFACEFLAG_LIGHTFILTER  ;
		if (shader->surfaceparms & Q3SURFACEPARM_METALSTEPS   ) texture->surfaceflags |= Q3SURFACEFLAG_METALSTEPS   ;
		if (shader->surfaceparms & Q3SURFACEPARM_NODAMAGE     ) texture->surfaceflags |= Q3SURFACEFLAG_NODAMAGE     ;
		if (shader->surfaceparms & Q3SURFACEPARM_NODLIGHT     ) texture->surfaceflags |= Q3SURFACEFLAG_NODLIGHT     ;
		if (shader->surfaceparms & Q3SURFACEPARM_NODRAW       ) texture->surfaceflags |= Q3SURFACEFLAG_NODRAW       ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NODROP       ) texture->surfaceflags |= Q3SURFACEFLAG_NODROP       ;
		if (shader->surfaceparms & Q3SURFACEPARM_NOIMPACT     ) texture->surfaceflags |= Q3SURFACEFLAG_NOIMPACT     ;
		if (shader->surfaceparms & Q3SURFACEPARM_NOLIGHTMAP   ) texture->surfaceflags |= Q3SURFACEFLAG_NOLIGHTMAP   ;
		if (shader->surfaceparms & Q3SURFACEPARM_NOMARKS      ) texture->surfaceflags |= Q3SURFACEFLAG_NOMARKS      ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_NOMIPMAPS    ) texture->surfaceflags |= Q3SURFACEFLAG_NOMIPMAPS    ;
		if (shader->surfaceparms & Q3SURFACEPARM_NONSOLID     ) texture->surfaceflags |= Q3SURFACEFLAG_NONSOLID     ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_ORIGIN       ) texture->surfaceflags |= Q3SURFACEFLAG_ORIGIN       ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_PLAYERCLIP   ) texture->surfaceflags |= Q3SURFACEFLAG_PLAYERCLIP   ;
		if (shader->surfaceparms & Q3SURFACEPARM_SKY          ) texture->surfaceflags |= Q3SURFACEFLAG_SKY          ;
		if (shader->surfaceparms & Q3SURFACEPARM_SLICK        ) texture->surfaceflags |= Q3SURFACEFLAG_SLICK        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_SLIME        ) texture->surfaceflags |= Q3SURFACEFLAG_SLIME        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_STRUCTURAL   ) texture->surfaceflags |= Q3SURFACEFLAG_STRUCTURAL   ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_TRANS        ) texture->surfaceflags |= Q3SURFACEFLAG_TRANS        ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_WATER        ) texture->surfaceflags |= Q3SURFACEFLAG_WATER        ;
		if (shader->surfaceparms & Q3SURFACEPARM_POINTLIGHT   ) texture->surfaceflags |= Q3SURFACEFLAG_POINTLIGHT   ;
		if (shader->surfaceparms & Q3SURFACEPARM_HINT         ) texture->surfaceflags |= Q3SURFACEFLAG_HINT         ;
		if (shader->surfaceparms & Q3SURFACEPARM_DUST         ) texture->surfaceflags |= Q3SURFACEFLAG_DUST         ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_BOTCLIP      ) texture->surfaceflags |= Q3SURFACEFLAG_BOTCLIP      ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_LIGHTGRID    ) texture->surfaceflags |= Q3SURFACEFLAG_LIGHTGRID    ;
	//	if (shader->surfaceparms & Q3SURFACEPARM_ANTIPORTAL   ) texture->surfaceflags |= Q3SURFACEFLAG_ANTIPORTAL   ;

		if (shader->dpmeshcollisions)
			texture->basematerialflags |= MATERIALFLAG_MESHCOLLISIONS;
		if (shader->dpshaderkill && developer_extra.integer)
			Con_DPrintLinef ("^1%s:^7 killing shader ^3" QUOTED_S " because of cvar", modelname, name);
	} // End shader
	else if (String_Match(texture->name, "noshader") || !texture->name[0])
	{
		if (developer_extra.integer)
			Con_DPrintLinef ("^1%s:^7 using fallback noshader material for ^3" QUOTED_S, modelname, name);
		texture->basematerialflags = defaultmaterialflags;
		texture->supercontents = SUPERCONTENTS_SOLID | SUPERCONTENTS_OPAQUE;
	}
	else if (String_Match(texture->name, "common/nodraw") || String_Match(texture->name, "textures/common/nodraw"))
	{
		if (developer_extra.integer)
			Con_DPrintLinef ("^1%s:^7 using fallback nodraw material for ^3" QUOTED_S, modelname, name);
		texture->basematerialflags = MATERIALFLAG_NODRAW | MATERIALFLAG_NOSHADOW;
		texture->supercontents = SUPERCONTENTS_SOLID;
	}
	else
	{
		// Baker: This is the non-shader path (shader not found)
		// Baker: "success is set to true before we ever get here.
		if (developer_extra.integer)
			Con_DPrintLinef ("^1%s:^7 No shader found for texture ^3" QUOTED_S, modelname, texture->name);

		if (texture->surfaceflags & Q3SURFACEFLAG_NODRAW) {
			texture->basematerialflags = MATERIALFLAG_NODRAW | MATERIALFLAG_NOSHADOW;
			texture->supercontents = SUPERCONTENTS_SOLID;
		}
		else if (texture->surfaceflags & Q3SURFACEFLAG_SKY) {
			texture->basematerialflags = MATERIALFLAG_SKY;
			texture->supercontents = SUPERCONTENTS_SKY;
		} else {
			texture->basematerialflags = defaultmaterialflags;
			texture->supercontents = SUPERCONTENTS_SOLID | SUPERCONTENTS_OPAQUE;
		}
		
		if (cls.state == ca_dedicated) {
			texture->materialshaderpass = NULL;
			success = false;
		}
		else
		{
#if 1
			if (fallback || shall_do_external)
			{
				// Baker: Old one didn't have fallback
				extern int q3_shader_did_fallback;
				extern int is_q3_shader_video_tex;
				extern byte *is_q3_shader_video_tex_vimagedata;
				
				q3_shader_did_fallback = false;
				skinframe_t *skinframe;
				
				if (is_q3_shader_video_tex) {
					skinframe = R_SkinFrame_LoadInternalBGRA(texture->name, 
						TEXF_FORCE_RELOAD  /*TEXF_MIPMAP | TEXF_ALPHA*/, 
						is_q3_shader_video_tex_vimagedata, image_width, image_height, 
						/*compare w/h/crc/srgb*/ 0, 0, 0, false, /*q1skyload*/ false); // TODO what sRGB argument to put here?
				} else {
					// Baker: nmap # gloss noshader path 0
					skinframe = R_SkinFrame_LoadExternal(texture->name,
						defaulttexflags, q_tx_complain_false, fallback);

				}

				if (skinframe)
				{
					texture->materialshaderpass = texture->shaderpasses[0] = 
						Mod_CreateShaderPass(mempool, skinframe);
					if (texture->materialshaderpass->skinframes[0]->hasalpha)
						texture->basematerialflags |= MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
					if (texture->q2contents)
						texture->supercontents = Mod_Q2BSP_SuperContentsFromNativeContents(texture->q2contents);
				}
				else
					success = false;

				if (q3_shader_did_fallback) {
					if (warnmissing)
						Con_PrintLinef (CON_RED "%s:" CON_WHITE " could not load texture " CON_BRONZE QUOTED_S, loadmodel->model_name, texture->name);

				}
			}
			else
				success = false;
			if (!success)
				if (warnmissing)
					Con_PrintLinef (CON_RED "%s:" CON_WHITE " could not load texture " CON_BRONZE QUOTED_S, loadmodel->model_name, texture->name);
#else
			// HEREON
			skinframe_t *skinframe = R_SkinFrame_LoadExternal(texture->name, defaulttexflags, q_tx_complain_false, fallback);
			if (skinframe)
			{
				texture->materialshaderpass = texture->shaderpasses[0] = Mod_CreateShaderPass(mempool, skinframe);
				if (texture->materialshaderpass->skinframes[0]->hasalpha)
					texture->basematerialflags |= MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
				if (texture->q2contents)
					texture->supercontents = Mod_Q2BSP_SuperContentsFromNativeContents(texture->q2contents);
			}
			else
				success = false;
			if (!success && warnmissing)
				Con_PrintLinef ("^1%s:^7 could not load texture " CON_BRONZE QUOTED_S, modelname, texture->name);
#endif
		}
	}
	// init the animation variables
	texture->currentframe = texture;
	texture->currentmaterialflags = texture->basematerialflags;
	if (!texture->materialshaderpass)
		texture->materialshaderpass = texture->shaderpasses[0] = Mod_CreateShaderPass(mempool, R_SkinFrame_LoadMissing());
	if (!texture->materialshaderpass->skinframes[0])
		texture->materialshaderpass->skinframes[0] = R_SkinFrame_LoadMissing();
	texture->currentskinframe = texture->materialshaderpass ? texture->materialshaderpass->skinframes[0] : NULL;
	texture->backgroundcurrentskinframe = texture->backgroundshaderpass ? texture->backgroundshaderpass->skinframes[0] : NULL;
	return success;
}

void Mod_LoadCustomMaterial(mempool_t *mempool, texture_t *texture, const char *name, int supercontents, int materialflags, skinframe_t *skinframe)
{
	if (!(materialflags & (MATERIALFLAG_WALL | MATERIALFLAG_SKY)))
		Con_DPrintLinef (CON_RED "Custom texture " CON_ERROR QUOTED_S " does not have MATERIALFLAG_WALL set", texture->name);

	strlcpy(texture->name, name, sizeof(texture->name));
	texture->basealpha = 1.0f;
	texture->basematerialflags = materialflags;
	texture->supercontents = supercontents;

	texture->offsetmapping = (mod_noshader_default_offsetmapping.value /*d: 1*/) ? OFFSETMAPPING_DEFAULT : OFFSETMAPPING_OFF;
	texture->offsetscale = 1;
	texture->offsetbias = 0;
	texture->specularscalemod = 1;
	texture->specularpowermod = 1;
	texture->rtlightambient = 0;
	texture->transparentsort = TRANSPARENTSORT_DISTANCE;
	// WHEN ADDING DEFAULTS HERE, REMEMBER TO PUT DEFAULTS IN ALL LOADERS
	// JUST GREP FOR "specularscalemod = 1".

	if (developer_extra.integer)
		Con_DPrintLinef (CON_RED "Custom texture " CON_BRONZE " " QUOTED_S, texture->name);
	if (skinframe)
		texture->materialshaderpass = texture->shaderpasses[0] = Mod_CreateShaderPass(mempool, skinframe);

	// init the animation variables
	texture->currentmaterialflags = texture->basematerialflags;
	texture->currentframe = texture;
	texture->currentskinframe = skinframe;
	texture->backgroundcurrentskinframe = NULL;
}

void Mod_UnloadCustomMaterial(texture_t *texture, qbool purgeskins)
{
	long unsigned int i, j;
	for (i = 0; i < sizeof(texture->shaderpasses) / sizeof(texture->shaderpasses[0]); i++)
	{
		if (texture->shaderpasses[i])
		{
			if (purgeskins)
				for (j = 0; j < sizeof(texture->shaderpasses[i]->skinframes) / sizeof(skinframe_t *);j++)
					if (texture->shaderpasses[i]->skinframes[j] && texture->shaderpasses[i]->skinframes[j]->base)
						R_SkinFrame_PurgeSkinFrame(texture->shaderpasses[i]->skinframes[j]);
			Mem_Free(texture->shaderpasses[i]);
			texture->shaderpasses[i] = NULL;
		}
	}
	texture->materialshaderpass = NULL;
	texture->currentskinframe = NULL;
	texture->backgroundcurrentskinframe = NULL;
}

skinfile_t *Mod_LoadSkinFiles(void)
{
	int i, words, line, wordsoverflow;
	char *text;
	const char *data;
	skinfile_t *skinfile = NULL, *first = NULL;
	skinfileitem_t *skinfileitem;
	char word[10][MAX_QPATH_128];
	char vabuf[1024];

/*
sample file:
U_bodyBox,models/players/Legoman/BikerA2.tga
U_RArm,models/players/Legoman/BikerA1.tga
U_LArm,models/players/Legoman/BikerA1.tga
U_armor,common/nodraw
U_sword,common/nodraw
U_shield,common/nodraw
U_homb,common/nodraw
U_backpack,common/nodraw
U_colcha,common/nodraw
tag_head,
tag_weapon,
tag_torso,
*/
	memset(word, 0, sizeof(word));
	for (i = 0;i < 256 && (data = text = (char *)FS_LoadFile(va(vabuf, sizeof(vabuf), "%s_%d.skin", loadmodel->model_name, i), tempmempool, fs_quiet_true, fs_size_ptr_null));i++) {
		// If it's the first file we parse
		if (skinfile == NULL)
		{
			skinfile = (skinfile_t *)Mem_Alloc(loadmodel->mempool, sizeof(skinfile_t));
			first = skinfile;
		}
		else
		{
			skinfile->next = (skinfile_t *)Mem_Alloc(loadmodel->mempool, sizeof(skinfile_t));
			skinfile = skinfile->next;
		}
		skinfile->next = NULL;

		for(line = 0;;line++)
		{
			// parse line
			if (!COM_ParseToken_QuakeC(&data, true))
				break;
			if (String_Match(com_token, "\n"))
				continue;
			words = 0;
			wordsoverflow = false;
			do
			{
				if (words < 10)
					strlcpy(word[words++], com_token, sizeof (word[0]));
				else
					wordsoverflow = true;
			}
			while (COM_ParseToken_QuakeC(&data, true) && String_NOT_Match(com_token, NEWLINE));
			if (wordsoverflow) {
				Con_PrintLinef (
					"Mod_LoadSkinFiles: parsing error in file \"%s_%d.skin\" on line #%d: line with too many statements, skipping", 
					loadmodel->model_name, i, line);
				continue;
			}
			// words is always >= 1
			if (String_Match(word[0], "replace"))
			{
				if (words == 3)
				{
					if (developer_loading.integer)
						Con_PrintLinef ("Mod_LoadSkinFiles: parsed mesh " QUOTED_S " shader replacement " QUOTED_S, word[1], word[2]);
					skinfileitem = (skinfileitem_t *)Mem_Alloc(loadmodel->mempool, sizeof(skinfileitem_t));
					skinfileitem->next = skinfile->items;
					skinfile->items = skinfileitem;
					c_strlcpy (skinfileitem->skname, word[1]); //, sizeof (skinfileitem->name));
					c_strlcpy (skinfileitem->replacement, word[2]); //, sizeof (skinfileitem->replacement));
				}
				else
					Con_PrintLinef ("Mod_LoadSkinFiles: parsing error in file \"%s_%d.skin\" on line #%d: wrong number of parameters to command " QUOTED_S ", see documentation in DP_GFX_SKINFILES extension in dpextensions.qc", loadmodel->model_name, i, line, word[0]);
			}
			//else if (words >= 2 && !strncmp(word[0], "tag_", 4))
			else if (words >= 2 && String_Starts_With_PRE(word[0], "tag_"))
			{
				// tag name, like "tag_weapon,"
				// not used for anything (not even in Quake3)
			}
			else if (words >= 2 && String_Match(word[1], ","))
			{
				// mesh shader name, like "U_RArm,models/players/Legoman/BikerA1.tga"
				if (developer_loading.integer)
					Con_PrintLinef ("Mod_LoadSkinFiles: parsed mesh " QUOTED_S " shader replacement " QUOTED_S, word[0], word[2]);
				skinfileitem = (skinfileitem_t *)Mem_Alloc(loadmodel->mempool, sizeof(skinfileitem_t));
				skinfileitem->next = skinfile->items;
				skinfile->items = skinfileitem;
				c_strlcpy (skinfileitem->skname, word[0]);//, sizeof (skinfileitem->name));
				c_strlcpy (skinfileitem->replacement, word[2]);//, sizeof (skinfileitem->replacement));
			}
			else
				Con_PrintLinef ("Mod_LoadSkinFiles: parsing error in file \"%s_%d.skin\" on line #%d: does not look like tag or mesh specification, or replace command, see documentation in DP_GFX_SKINFILES extension in dpextensions.qc", loadmodel->model_name, i, line);
		}
		Mem_Free(text);
	}
	if (i)
		loadmodel->numskins = i;
	return first;
}

void Mod_FreeSkinFiles(skinfile_t *skinfile)
{
	skinfile_t *next;
	skinfileitem_t *skinfileitem, *nextitem;
	for (; skinfile;skinfile = next) {
		next = skinfile->next;
		for (skinfileitem = skinfile->items;skinfileitem;skinfileitem = nextitem) {
			nextitem = skinfileitem->next;
			Mem_Free(skinfileitem);
		}
		Mem_Free(skinfile);
	}
}

int Mod_CountSkinFiles(skinfile_t *skinfile)
{
	int i;
	for (i = 0;skinfile;skinfile = skinfile->next, i++);
	return i;
}

void Mod_SnapVertices(int numcomponents, int numvertices, float *vertices, float snap)
{
	int i;
	double isnap = 1.0 / snap;
	for (i = 0;i < numvertices*numcomponents;i++)
		vertices[i] = floor(vertices[i]*isnap)*snap;
}

int Mod_RemoveDegenerateTriangles(int numtriangles, const int *inelement3i, int *outelement3i, const float *vertex3f)
{
	int i, outtriangles;
	float edgedir1[3], edgedir2[3], temp[3];
	// a degenerate triangle is one with no width (thickness, surface area)
	// these are characterized by having all 3 points colinear (along a line)
	// or having two points identical
	// the simplest check is to calculate the triangle's area
	for (i = 0, outtriangles = 0;i < numtriangles;i++, inelement3i += 3)
	{
		// calculate first edge
		VectorSubtract(vertex3f + inelement3i[1] * 3, vertex3f + inelement3i[0] * 3, edgedir1);
		VectorSubtract(vertex3f + inelement3i[2] * 3, vertex3f + inelement3i[0] * 3, edgedir2);
		CrossProduct(edgedir1, edgedir2, temp);
		if (VectorLength2(temp) < 0.001f)
			continue; // degenerate triangle (no area)
		// valid triangle (has area)
		VectorCopy(inelement3i, outelement3i);
		outelement3i += 3;
		outtriangles++;
	}
	return outtriangles;
}

void Mod_VertexRangeFromElements(int numelements, const int *elements, int *firstvertexpointer, int *lastvertexpointer)
{
	int i, e;
	int firstvertex, lastvertex;
	if (numelements > 0 && elements)
	{
		firstvertex = lastvertex = elements[0];
		for (i = 1;i < numelements;i++)
		{
			e = elements[i];
			firstvertex = min(firstvertex, e);
			lastvertex = max(lastvertex, e);
		}
	}
	else
		firstvertex = lastvertex = 0;
	if (firstvertexpointer)
		*firstvertexpointer = firstvertex;
	if (lastvertexpointer)
		*lastvertexpointer = lastvertex;
}

void Mod_SetDrawSkyAndWater(model_t *mod)
{
	int j;
	uint64_t basematerialflags = 0;
	// by default assume there is no sky or water used in this model
	mod->DrawSky = NULL;
	mod->DrawAddWaterPlanes = NULL;
	// combine all basematerialflags observed in the submodelsurfaces range, then check for special flags
	for (j = mod->submodelsurfaces_start; j < mod->submodelsurfaces_end; j++)
		if (mod->data_surfaces[j].texture)
			basematerialflags |= mod->data_surfaces[j].texture->basematerialflags;
	if (basematerialflags & MATERIALFLAG_SKY)
		mod->DrawSky = R_Mod_DrawSky;
	if (basematerialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_REFLECTION | MATERIALFLAG_CAMERA))
		mod->DrawAddWaterPlanes = R_Mod_DrawAddWaterPlanes;
}

typedef struct Mod_MakeSortedSurfaces_qsortsurface_s
{
	int surfaceindex;
	q3deffect_t *effect;
	texture_t *texture;
	rtexture_t *lightmaptexture;
}
Mod_MakeSortedSurfaces_qsortsurface_t;

static int Mod_MakeSortedSurfaces_qsortfunc(const void *a, const void *b)
{
	const Mod_MakeSortedSurfaces_qsortsurface_t *l = (Mod_MakeSortedSurfaces_qsortsurface_t*)a;
	const Mod_MakeSortedSurfaces_qsortsurface_t *r = (Mod_MakeSortedSurfaces_qsortsurface_t*)b;
	if (l->effect < r->effect)
		return -1;
	if (l->effect > r->effect)
		return 1;
	if (l->texture < r->texture)
		return -1;
	if (l->texture > r->texture)
		return 1;
	if (l->lightmaptexture < r->lightmaptexture)
		return -1;
	if (l->lightmaptexture > r->lightmaptexture)
		return 1;
	if (l->surfaceindex < r->surfaceindex)
		return -1;
	if (l->surfaceindex > r->surfaceindex)
		return 1;
	return 0;
}

void Mod_MakeSortedSurfaces(model_t *mod)
{
	// make an optimal set of texture-sorted batches to draw...
	int j, k;
	Mod_MakeSortedSurfaces_qsortsurface_t *info;

	if (cls.state == ca_dedicated)
		return;

	info = (Mod_MakeSortedSurfaces_qsortsurface_t*)Mem_Alloc(loadmodel->mempool, mod->num_surfaces * sizeof(*info));
	if (!mod->modelsurfaces_sorted)
		mod->modelsurfaces_sorted = (int *) Mem_Alloc(loadmodel->mempool, mod->num_surfaces * sizeof(*mod->modelsurfaces_sorted));
	// the goal is to sort by submodel (can't change which submodel a surface belongs to), and then by effects and textures
	for (j = 0; j < mod->num_surfaces; j++)
	{
		info[j].surfaceindex = j;
		info[j].effect = mod->data_surfaces[j].effect;
		info[j].texture = mod->data_surfaces[j].texture;
		info[j].lightmaptexture = mod->data_surfaces[j].lightmaptexture;
	}
	for (k = 0; k < mod->brush.numsubmodels; k++)
		if (mod->brush.submodels[k]->submodelsurfaces_end > mod->brush.submodels[k]->submodelsurfaces_start + 1)
			qsort(info + mod->brush.submodels[k]->submodelsurfaces_start, (size_t)mod->brush.submodels[k]->submodelsurfaces_end - mod->brush.submodels[k]->submodelsurfaces_start, sizeof(*info), Mod_MakeSortedSurfaces_qsortfunc);
	for (j = 0; j < mod->num_surfaces; j++)
		mod->modelsurfaces_sorted[j] = info[j].surfaceindex;
	Mem_Free(info);
}

WARP_X_CALLERS_ (Mod_LoadModel)
void Mod_BuildVBOs(void)
{
	if (cls.state == ca_dedicated)
		return;

	if (!loadmodel->surfmesh.num_vertices)
		return;

	if (gl_paranoid.integer && loadmodel->surfmesh.data_element3s && loadmodel->surfmesh.data_element3i) {
		int i;
		for (i = 0;i < loadmodel->surfmesh.num_triangles*3;i++) {
			if (loadmodel->surfmesh.data_element3s[i] != loadmodel->surfmesh.data_element3i[i]) {
				Con_PrintLinef ("Mod_BuildVBOs: element %u is incorrect (%u should be %u)", i, 
					loadmodel->surfmesh.data_element3s[i], loadmodel->surfmesh.data_element3i[i]);
				loadmodel->surfmesh.data_element3s[i] = loadmodel->surfmesh.data_element3i[i];
			}
		}
	}

	// upload short indices as a buffer
	if (loadmodel->surfmesh.data_element3s && !loadmodel->surfmesh.data_element3s_indexbuffer)
		loadmodel->surfmesh.data_element3s_indexbuffer = R_Mesh_CreateMeshBuffer(loadmodel->surfmesh.data_element3s, loadmodel->surfmesh.num_triangles * sizeof(short[3]), loadmodel->model_name, true, false, false, true);

	// upload int indices as a buffer
	if (loadmodel->surfmesh.data_element3i && !loadmodel->surfmesh.data_element3i_indexbuffer && !loadmodel->surfmesh.data_element3s)
		loadmodel->surfmesh.data_element3i_indexbuffer = R_Mesh_CreateMeshBuffer(loadmodel->surfmesh.data_element3i, loadmodel->surfmesh.num_triangles * sizeof(int[3]), loadmodel->model_name, true, false, false, false);

	// only build a vbo if one has not already been created (this is important for brush models which load specially)
	// we put several vertex data streams in the same buffer
	if (!loadmodel->surfmesh.data_vertex3f_vertexbuffer)
	{
		int size;
		unsigned char *mem;
		size = 0;
		loadmodel->surfmesh.data_vertex3f_bufferoffset           = size;if (loadmodel->surfmesh.data_vertex3f          ) size += loadmodel->surfmesh.num_vertices * sizeof(float[3]);
		loadmodel->surfmesh.data_svector3f_bufferoffset          = size;if (loadmodel->surfmesh.data_svector3f         ) size += loadmodel->surfmesh.num_vertices * sizeof(float[3]);
		loadmodel->surfmesh.data_tvector3f_bufferoffset          = size;if (loadmodel->surfmesh.data_tvector3f         ) size += loadmodel->surfmesh.num_vertices * sizeof(float[3]);
		loadmodel->surfmesh.data_normal3f_bufferoffset           = size;if (loadmodel->surfmesh.data_normal3f          ) size += loadmodel->surfmesh.num_vertices * sizeof(float[3]);
		loadmodel->surfmesh.data_texcoordtexture2f_bufferoffset  = size;if (loadmodel->surfmesh.data_texcoordtexture2f ) size += loadmodel->surfmesh.num_vertices * sizeof(float[2]);
		loadmodel->surfmesh.data_texcoordlightmap2f_bufferoffset = size;if (loadmodel->surfmesh.data_texcoordlightmap2f) size += loadmodel->surfmesh.num_vertices * sizeof(float[2]);
		loadmodel->surfmesh.data_lightmapcolor4f_bufferoffset    = size;if (loadmodel->surfmesh.data_lightmapcolor4f   ) size += loadmodel->surfmesh.num_vertices * sizeof(float[4]);
		loadmodel->surfmesh.data_skeletalindex4ub_bufferoffset   = size;if (loadmodel->surfmesh.data_skeletalindex4ub  ) size += loadmodel->surfmesh.num_vertices * sizeof(unsigned char[4]);
		loadmodel->surfmesh.data_skeletalweight4ub_bufferoffset  = size;if (loadmodel->surfmesh.data_skeletalweight4ub ) size += loadmodel->surfmesh.num_vertices * sizeof(unsigned char[4]);
		mem = (unsigned char *)Mem_Alloc(tempmempool, size);
		if (loadmodel->surfmesh.data_vertex3f          ) memcpy(mem + loadmodel->surfmesh.data_vertex3f_bufferoffset          , loadmodel->surfmesh.data_vertex3f          , loadmodel->surfmesh.num_vertices * sizeof(float[3]));
		if (loadmodel->surfmesh.data_svector3f         ) memcpy(mem + loadmodel->surfmesh.data_svector3f_bufferoffset         , loadmodel->surfmesh.data_svector3f         , loadmodel->surfmesh.num_vertices * sizeof(float[3]));
		if (loadmodel->surfmesh.data_tvector3f         ) memcpy(mem + loadmodel->surfmesh.data_tvector3f_bufferoffset         , loadmodel->surfmesh.data_tvector3f         , loadmodel->surfmesh.num_vertices * sizeof(float[3]));
		if (loadmodel->surfmesh.data_normal3f          ) memcpy(mem + loadmodel->surfmesh.data_normal3f_bufferoffset          , loadmodel->surfmesh.data_normal3f          , loadmodel->surfmesh.num_vertices * sizeof(float[3]));
		if (loadmodel->surfmesh.data_texcoordtexture2f ) memcpy(mem + loadmodel->surfmesh.data_texcoordtexture2f_bufferoffset , loadmodel->surfmesh.data_texcoordtexture2f , loadmodel->surfmesh.num_vertices * sizeof(float[2]));
		if (loadmodel->surfmesh.data_texcoordlightmap2f) memcpy(mem + loadmodel->surfmesh.data_texcoordlightmap2f_bufferoffset, loadmodel->surfmesh.data_texcoordlightmap2f, loadmodel->surfmesh.num_vertices * sizeof(float[2]));
		if (loadmodel->surfmesh.data_lightmapcolor4f   ) memcpy(mem + loadmodel->surfmesh.data_lightmapcolor4f_bufferoffset   , loadmodel->surfmesh.data_lightmapcolor4f   , loadmodel->surfmesh.num_vertices * sizeof(float[4]));
		if (loadmodel->surfmesh.data_skeletalindex4ub  ) memcpy(mem + loadmodel->surfmesh.data_skeletalindex4ub_bufferoffset  , loadmodel->surfmesh.data_skeletalindex4ub  , loadmodel->surfmesh.num_vertices * sizeof(unsigned char[4]));
		if (loadmodel->surfmesh.data_skeletalweight4ub ) memcpy(mem + loadmodel->surfmesh.data_skeletalweight4ub_bufferoffset , loadmodel->surfmesh.data_skeletalweight4ub , loadmodel->surfmesh.num_vertices * sizeof(unsigned char[4]));
		loadmodel->surfmesh.data_vertex3f_vertexbuffer = R_Mesh_CreateMeshBuffer(mem, size, loadmodel->model_name, false, false, false, false);
		loadmodel->surfmesh.data_svector3f_vertexbuffer = loadmodel->surfmesh.data_svector3f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_tvector3f_vertexbuffer = loadmodel->surfmesh.data_tvector3f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_normal3f_vertexbuffer = loadmodel->surfmesh.data_normal3f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_texcoordtexture2f_vertexbuffer = loadmodel->surfmesh.data_texcoordtexture2f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_texcoordlightmap2f_vertexbuffer = loadmodel->surfmesh.data_texcoordlightmap2f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_lightmapcolor4f_vertexbuffer = loadmodel->surfmesh.data_lightmapcolor4f ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_skeletalindex4ub_vertexbuffer = loadmodel->surfmesh.data_skeletalindex4ub ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		loadmodel->surfmesh.data_skeletalweight4ub_vertexbuffer = loadmodel->surfmesh.data_skeletalweight4ub ? loadmodel->surfmesh.data_vertex3f_vertexbuffer : NULL;
		Mem_Free(mem);
	}
}

extern cvar_t mod_obj_orientation;
// Baker: If connected ...
// We need to find an entity for RSurf_ActiveModelEntity
// To get it to do:
// 	memcpy(rsurface.frameblend, ent->frameblend, sizeof(ent->frameblend)); // ALIASXPRIME!
// model->AnimateVertices
WARP_X_ (RSurf_ActiveModelEntity Mod_MDL_AnimateVertices ALIASXPRIME Mod_IDP0_Load)

// Baker: It must be precached.
// Baker: No return in this function.
WARP_X_CALLERS_ (Mod_Decompile_f)
static void Mod_Decompile_OBJ (model_t *model, const char *rawname, const char *filename, 
							   const char *mtlfilename, const char *originalfilename, int framenumz, 
							   int submodelnum, ccs *printf_pattern, baker_string_t *bsa)
{
	int submodelindex, vertexindex, surfaceindex, triangleindex, textureindex, countvertices = 0, countsurfaces = 0, countfaces = 0, counttextures = 0;
	int a, b, c;
	const char *texname;
	const int *e;
	const float *v, *vn, *vt;
	size_t jj;
	size_t outbufferpos = 0;
	size_t outbuffermax = 0x100000; // 1 048 576
	char *outbuffer = (char *) Z_Malloc(outbuffermax), *oldbuffer;
	const msurface_t *surface;
	const int maxtextures = 256;
	char *texturenames = (char *) Z_Malloc(maxtextures * MAX_QPATH_128);
	model_t *submodel;

	// construct the mtllib file
	jj = dpsnprintf (outbuffer + outbufferpos, outbuffermax - outbufferpos, 
		"# mtllib for %s exported by darkplaces engine" NEWLINE, originalfilename);

	if (jj > 0)
		outbufferpos += jj;

	for (surfaceindex = 0, surface = model->data_surfaces;surfaceindex < model->num_surfaces;surfaceindex++, surface++) {
		countsurfaces++;
		countvertices += surface->num_vertices;
		countfaces += surface->num_triangles;
		texname = (surface->texture && surface->texture->name[0]) ? surface->texture->name : "default";
		for (textureindex = 0;textureindex < counttextures;textureindex++)
			if (String_Match(texturenames + textureindex * MAX_QPATH_128, texname))
				break;
		if (textureindex < counttextures)
			continue; // already wrote this material entry
		if (textureindex >= maxtextures)
			continue; // just a precaution
		textureindex = counttextures++;
		strlcpy(texturenames + textureindex * MAX_QPATH_128, texname, MAX_QPATH_128);
		if (outbufferpos >= outbuffermax >> 1) {
			outbuffermax *= 2;
			oldbuffer = outbuffer;
			outbuffer = (char *) Z_Malloc(outbuffermax);
			memcpy(outbuffer, oldbuffer, outbufferpos);
			Z_Free(oldbuffer);
		}

		jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, 
			"newmtl %s"			NEWLINE 
			"Ns 96.078431"		NEWLINE 
			"Ka 0 0 0"			NEWLINE 
			"Kd 0.64 0.64 0.64" NEWLINE 
			"Ks 0.5 0.5 0.5"	NEWLINE 
			"Ni 1"				NEWLINE 
			"d 1"				NEWLINE 
			"illum 2"			NEWLINE 
			"map_Kd %s%s"		NEWLINE NEWLINE, texname, texname, strstr(texname, ".tga") ? "" : ".tga"
		);

		if (jj > 0)
			outbufferpos += jj;
	}

	// write the mtllib file
	FS_WriteFile (mtlfilename, outbuffer, outbufferpos);

	// construct the obj file - ALIASX
	outbufferpos = 0;
	jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "# model exported from %s by darkplaces engine\n# %d vertices, %d faces, %d surfaces\nmtllib %s\n", originalfilename, countvertices, countfaces, countsurfaces, mtlfilename);
	if (jj > 0)
		outbufferpos += jj;
	
	float *freevertex3f = NULL;

	if (framenumz > 0 && isin1(model->type, mod_alias) ) {
		WARP_X_ (Mod_MDLMD2MD3_TraceLine Mod_Alias_MorphMesh_CompileFrames)

		frameblend_t frameblend[MAX_FRAMEBLENDS_8] = {0};

		freevertex3f = (float *)Mem_Alloc(tempmempool, model->surfmesh.num_vertices * sizeof(float[3]));

		//memcpy (freevertex3f, 

		frameblend[0].subframe = framenumz;
		frameblend[1].subframe = framenumz;
		frameblend[0].rlerp = 0;
		frameblend[1].rlerp = 1;

		//model->AnimateVertices (model, frameblend, NULL, freevertex3f, NULL, NULL, NULL); // Mod_MDL_AnimateVertices - TRACELINE

		model->AnimateVertices(model, frameblend, NULL, freevertex3f, NULL, NULL, NULL); // Mod_MDL_AnimateVertices - TRACELINE
	}

	while (0) {
		// Baker: Not sure if model->num_frames is always same as num_poses
		// because there are framegroups (rare) and not familiar with all the various model types
		int numframes = model->num_poses; 
		int trinum = 184;
		//if (loadmodel->surfmesh.data_element3s)
		//	for (i = 0; i < loadmodel->surfmesh.num_triangles * 3; i ++)
		//		loadmodel->surfmesh.data_element3s[i] = loadmodel->surfmesh.data_element3i[i];*/
		// We should output to a text file.
		char filename[MAX_QPATH_128];
		c_strlcpy (filename, rawname);
		//File_URL_Edit_Remove_Extension (filename);
		c_strlcat (filename, ".md3tags");

		//va_super (filename, MAX_QPATH_128, "
		qfile_t *f = FS_OpenRealFile(filename, "wb", fs_quiet_FALSE); // WRITE-EON -- Baker: CleanShader for jack_scripts

		if (!f) {
			Con_PrintLinef ("ERROR: couldn't open " QUOTED_S, filename);
			break;
		}

		// PHASE 1 - print triangles for frame 0
		FS_PrintLinef (f, "// " QUOTED_S, rawname);
		FS_PrintLinef (f, "// ");
		FS_PrintLinef (f, "// Triangles ");

		const int *etri;
		surface = model->data_surfaces;
		// How many triangles?  For feral model = 293 (294 because 0-293)
		for (triangleindex = 0, etri = model->surfmesh.data_element3i + surface->num_firsttriangle * 3;
			triangleindex < surface->num_triangles; triangleindex ++, etri += 3) {
			// TAGALIASX
			int a = etri[0]+1, b = etri[1]+1, c = etri[2]+1;

			vec3_t p0, p1, p2;
			//vec3_t center;
		
			WARP_X_ (NORMAL Collision_CalcPlanesForTriangleBrushFloat Mod_IDP0_Load)

			VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+0], p0);
			VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+1], p2);
			VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+2], p1);

			FS_PrintLinef (f, "// #%4d " VECTOR9_5d1F, 
				triangleindex,
				VECTOR3_SEND (p0),
				VECTOR3_SEND (p1),
				VECTOR3_SEND (p2)
			);
		}

		FS_CloseNULL_ (f);

		
		//	FS_OpenRealFile
		break;
	} // while 1
	
	float *vertex3ff = freevertex3f ? freevertex3f : model->surfmesh.data_vertex3f;

	int shall_invert_y = isin1(model->type, mod_alias) ? false : true; // isin3(model->type, mod_brushq1, mod_brushq2, mod_brushq3);

	for (vertexindex = 0, v = framenumz ? vertex3ff : model->surfmesh.data_vertex3f, 
		vn = model->surfmesh.data_normal3f, 
		vt = model->surfmesh.data_texcoordtexture2f;
		vertexindex < model->surfmesh.num_vertices;vertexindex++, v += 3, vn += 3, vt += 2)
	{
		if (outbufferpos >= outbuffermax >> 1) {
			outbuffermax *= 2;
			oldbuffer = outbuffer;
			outbuffer = (char *) Z_Malloc(outbuffermax);
			memcpy(outbuffer, oldbuffer, outbufferpos);
			Z_Free(oldbuffer);
		}

		if (mod_obj_orientation.integer /*d: 1*/) {
			// This is the norm
			if (bsa) {
				// Baker: Remember not to flip Y and Z
				va_super (sbuf, 1024, "%g, %g, %g," NEWLINE, v[0], v[1], v[2]);
				BakerString_CatC (bsa, sbuf);
			}

			jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, 
			"v %f %f %f"	NEWLINE
			"vn %f %f %f"	NEWLINE
			"vt %f %f"		NEWLINE, 
			v[0], v[2], v[1], vn[0], vn[2], vn[1], vt[0], shall_invert_y ? 1-vt[1] : vt[1]  );
		}
		else
			jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, 
			"v %f %f %f"	NEWLINE 
			"vn %f %f %f"	NEWLINE 
			"vt %f %f"		NEWLINE, 
			v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], shall_invert_y ? 1-vt[1] : vt[1]  );
		if (jj > 0)
			outbufferpos += jj;
	}

	int start_index		= submodelnum ? submodelnum + 0 : 0;
	int beyond_index	= submodelnum ? submodelnum + 1 : max(1, model->brush.numsubmodels);

	//for (submodelindex = 0; submodelindex < max(1, model->brush.numsubmodels); submodelindex ++) {
	for (submodelindex = start_index; submodelindex < beyond_index; submodelindex ++) {
		jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "o %d\n", submodelindex);
		if (jj > 0)
			outbufferpos += jj;
		submodel = model->brush.numsubmodels ? model->brush.submodels[submodelindex] : model;
		for (surfaceindex = submodel->submodelsurfaces_start;surfaceindex < submodel->submodelsurfaces_end;surfaceindex++) {
			surface = model->data_surfaces + surfaceindex;
			jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "usemtl %s\n", (surface->texture && surface->texture->name[0]) ? surface->texture->name : "default");
			if (jj > 0)
				outbufferpos += jj;
			for (triangleindex = 0, e = model->surfmesh.data_element3i + surface->num_firsttriangle * 3;
				triangleindex < surface->num_triangles;triangleindex++, e += 3) {
				if (outbufferpos >= outbuffermax >> 1) {
					outbuffermax *= 2;
					oldbuffer = outbuffer;
					outbuffer = (char *) Z_Malloc(outbuffermax);
					memcpy(outbuffer, oldbuffer, outbufferpos);
					Z_Free(oldbuffer);
				}
				a = e[0]+1;
				b = e[1]+1;
				c = e[2]+1;
				if (mod_obj_orientation.integer /*d: 1*/) {
					// Baker: This is the norm
					jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, 
					"f %d/%d/%d %d/%d/%d %d/%d/%d" NEWLINE, a,a,a,b,b,b,c,c,c);
				}
				else
					jj = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, 
					"f %d/%d/%d %d/%d/%d %d/%d/%d" NEWLINE, a,a,a,c,c,c,b,b,b);
				if (jj > 0)
					outbufferpos += jj;
			}
		}
	}

	// write the obj file
	FS_WriteFile(filename, outbuffer, outbufferpos);

	// clean up
	Z_Free(outbuffer);
	Z_Free(texturenames);

	if (freevertex3f)
		Mem_Free(freevertex3f);


	// print some stats
	Con_Printf ("Wrote %s (%d bytes, %d vertices, %d faces, %d surfaces with %d distinct textures)\n", filename, (int)outbufferpos, countvertices, countfaces, countsurfaces, counttextures);
}

static void Mod_Decompile_SMD(model_t *model, const char *filename, int firstpose, int numposes, qbool writetriangles)
{
	int countnodes = 0, counttriangles = 0, countframes = 0;
	int surfaceindex;
	int triangleindex;
	int transformindex;
	int poseindex;
	int cornerindex;
	const int *e;
	size_t l;
	size_t outbufferpos = 0;
	size_t outbuffermax = 0x100000;
	char *outbuffer = (char *) Z_Malloc(outbuffermax), *oldbuffer;
	const msurface_t *surface;
	l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "version 1\nnodes\n");
	if (l > 0)
		outbufferpos += l;
	for (transformindex = 0;transformindex < model->num_bones;transformindex++)
	{
		if (outbufferpos >= outbuffermax >> 1)
		{
			outbuffermax *= 2;
			oldbuffer = outbuffer;
			outbuffer = (char *) Z_Malloc(outbuffermax);
			memcpy(outbuffer, oldbuffer, outbufferpos);
			Z_Free(oldbuffer);
		}
		countnodes++;
		l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i " QUOTED_S " %3i\n", transformindex, model->data_bones[transformindex].name, model->data_bones[transformindex].parent);
		if (l > 0)
			outbufferpos += l;
	}
	l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "end\nskeleton\n");
	if (l > 0)
		outbufferpos += l;
	for (poseindex = 0;poseindex < numposes;poseindex++)
	{
		countframes++;
		l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "time %d\n", poseindex);
		if (l > 0)
			outbufferpos += l;
		for (transformindex = 0;transformindex < model->num_bones;transformindex++)
		{
			float angles[3];
			float mtest[4][3];
			matrix4x4_t posematrix;
			if (outbufferpos >= outbuffermax >> 1)
			{
				outbuffermax *= 2;
				oldbuffer = outbuffer;
				outbuffer = (char *) Z_Malloc(outbuffermax);
				memcpy(outbuffer, oldbuffer, outbufferpos);
				Z_Free(oldbuffer);
			}

			// strangely the smd angles are for a transposed matrix, so we
			// have to generate a transposed matrix, then convert that...
			Matrix4x4_FromBonePose7s(&posematrix, model->num_posescale, model->data_poses7s + 7*(model->num_bones * poseindex + transformindex));
			Matrix4x4_ToArray12FloatGL(&posematrix, mtest);
			AnglesFromVectors(angles, mtest[0], mtest[2], false);
			if (angles[0] >= 180) angles[0] -= 360;
			if (angles[1] >= 180) angles[1] -= 360;
			if (angles[2] >= 180) angles[2] -= 360;

#if 0
{
			float a = DEG2RAD(angles[ROLL]);
			float b = DEG2RAD(angles[PITCH]);
			float c = DEG2RAD(angles[YAW]);
			float cy, sy, cp, sp, cr, sr;
			float test[4][3];
			// smd matrix construction, for comparing
			sy = sin(c);
			cy = cos(c);
			sp = sin(b);
			cp = cos(b);
			sr = sin(a);
			cr = cos(a);

			test[0][0] = cp*cy;
			test[0][1] = cp*sy;
			test[0][2] = -sp;
			test[1][0] = sr*sp*cy+cr*-sy;
			test[1][1] = sr*sp*sy+cr*cy;
			test[1][2] = sr*cp;
			test[2][0] = (cr*sp*cy+-sr*-sy);
			test[2][1] = (cr*sp*sy+-sr*cy);
			test[2][2] = cr*cp;
			test[3][0] = pose[9];
			test[3][1] = pose[10];
			test[3][2] = pose[11];
}
#endif
			l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f\n", transformindex, mtest[3][0], mtest[3][1], mtest[3][2], DEG2RAD(angles[ROLL]), DEG2RAD(angles[PITCH]), DEG2RAD(angles[YAW]));
			if (l > 0)
				outbufferpos += l;
		}
	}
	l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "end\n");
	if (l > 0)
		outbufferpos += l;
	if (writetriangles)
	{
		l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "triangles\n");
		if (l > 0)
			outbufferpos += l;
		for (surfaceindex = 0, surface = model->data_surfaces;surfaceindex < model->num_surfaces;surfaceindex++, surface++)
		{
			for (triangleindex = 0, e = model->surfmesh.data_element3i + surface->num_firsttriangle * 3;triangleindex < surface->num_triangles;triangleindex++, e += 3)
			{
				counttriangles++;
				if (outbufferpos >= outbuffermax >> 1)
				{
					outbuffermax *= 2;
					oldbuffer = outbuffer;
					outbuffer = (char *) Z_Malloc(outbuffermax);
					memcpy(outbuffer, oldbuffer, outbufferpos);
					Z_Free(oldbuffer);
				}
				l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%s\n", surface->texture && surface->texture->name[0] ? surface->texture->name : "default.bmp");
				if (l > 0)
					outbufferpos += l;
				for (cornerindex = 0;cornerindex < 3;cornerindex++)
				{
					const int index = e[2-cornerindex];
					const float *v = model->surfmesh.data_vertex3f + index * 3;
					const float *vn = model->surfmesh.data_normal3f + index * 3;
					const float *vt = model->surfmesh.data_texcoordtexture2f + index * 2;
					const int b = model->surfmesh.blends[index];
					if (b < model->num_bones)
						l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f %f %f\n"                          , b, v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], 1 - vt[1]);
					else
					{
						const blendweights_t *w = model->surfmesh.data_blendweights + b - model->num_bones;
						const unsigned char *wi = w->index;
						const unsigned char *wf = w->influence;
					    if (wf[3]) l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f %f %f 4 %d %f %d %f %d %f %d %f\n", wi[0], v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], 1 - vt[1], wi[0], wf[0]/255.0f, wi[1], wf[1]/255.0f, wi[2], wf[2]/255.0f, wi[3], wf[3]/255.0f);
						else if (wf[2]) l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f %f %f 3 %d %f %d %f %d %f\n"      , wi[0], v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], 1 - vt[1], wi[0], wf[0]/255.0f, wi[1], wf[1]/255.0f, wi[2], wf[2]/255.0f);
						else if (wf[1]) l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f %f %f 2 %d %f %d %f\n"            , wi[0], v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], 1 - vt[1], wi[0], wf[0]/255.0f, wi[1], wf[1]/255.0f);
						else            l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "%3i %f %f %f %f %f %f %f %f\n"                          , wi[0], v[0], v[1], v[2], vn[0], vn[1], vn[2], vt[0], 1 - vt[1]);
					}
					if (l > 0)
						outbufferpos += l;
				}
			}
		}
		l = dpsnprintf(outbuffer + outbufferpos, outbuffermax - outbufferpos, "end\n");
		if (l > 0)
			outbufferpos += l;
	}

	FS_WriteFile(filename, outbuffer, outbufferpos);
	Z_Free(outbuffer);

	Con_PrintLinef ("Wrote %s (%d bytes, %d nodes, %d frames, %d triangles)", filename, (int)outbufferpos, countnodes, countframes, counttriangles);
}

/*
================
Mod_Decompile_f

decompiles a model to editable files
================
*/

// RSurf_ActiveModelEntity
WARP_X_ (Mod_Decompile_OBJ)
static void Mod_Decompile_f(cmd_state_t *cmd)
{
	int i, j, k, first, count;
	model_t *mod;
	char inname[MAX_QPATH_128];
	char outname[MAX_QPATH_128];
	char mtlname[MAX_QPATH_128];
	char basename[MAX_QPATH_128];
	char animname[MAX_QPATH_128];
	char animname2[MAX_QPATH_128];
	char zymtextbuffer[16384];
	char dpmtextbuffer[16384];
	char framegroupstextbuffer[16384];
	int zymtextsize = 0;
	int dpmtextsize = 0;
	int framegroupstextsize = 0;
	char vabuf[1024];

	if (Cmd_Argc(cmd) > 3) {
		Con_PrintLinef ("usage: modeldecompile <filename> [framenum]");
		return;
	}

	c_strlcpy (inname, Cmd_Argv(cmd, 1));
	FS_StripExtension(inname, basename, sizeof(basename));
	
	mod = Mod_ForName(inname, false, true, inname[0] == '*' ? cl.model_name[1] : NULL);

	if (!mod) {
		Con_PrintLinef ("No such model");
		return;
	}

	int submodelindexwanted = 0;
	if (mod->brush.submodel) {
		// if we're decompiling a submodel, be sure to give it a proper name based on its parent
		submodelindexwanted = atoi(&inname[1]);
		FS_StripExtension(cl.model_name[1], outname, sizeof(outname));
		c_dpsnprintf2 (basename, "%s/%s", outname, mod->model_name);
		// Baker r7061: Model decompile, windows will not allow * in a file name
		dpreplacechar (basename, '*', '_'); 
		outname[0] = 0;
	}

	if (!mod->surfmesh.num_triangles) {
		Con_PrintLinef ("Empty model (or sprite)");
		return;
	}

	// export OBJ if possible (not on sprites)
obj_export:
	
	if (mod->surfmesh.num_triangles) {
		// Baker: Comes here if we have triangles.
		baker_string_t *bsa = BakerString_Create_Malloc ("");
		// [0] modeldecompile [1]progs/player.mdl [2] framenum
		ccs *arg2 = Cmd_Argv (cmd, 2);
		int framenumz = atoi(arg2);
		c_dpsnprintf1 (outname, "%s_decompiled.obj", basename);
		c_dpsnprintf1 (mtlname, "%s_decompiled.mtl", basename);
		Mod_Decompile_OBJ (mod, inname, outname, mtlname, inname, framenumz, submodelindexwanted, NULL, bsa);
		Clipboard_Set_Text (bsa->string);
		BakerString_Destroy_And_Null_It (&bsa);
	}

	// export SMD if possible (only for skeletal models)
	if (mod->surfmesh.num_triangles && mod->num_bones) {
		dpsnprintf(outname, sizeof(outname), "%s_decompiled/ref1.smd", basename);
		Mod_Decompile_SMD(mod, outname, 0, 1, true);
		int jj = dpsnprintf(zymtextbuffer + zymtextsize, sizeof(zymtextbuffer) - zymtextsize, "output out.zym\nscale 1\norigin 0 0 0\nmesh ref1.smd\n");
		if (jj > 0) zymtextsize += jj;
		jj = dpsnprintf(dpmtextbuffer + dpmtextsize, sizeof(dpmtextbuffer) - dpmtextsize, "outputdir .\nmodel out\nscale 1\norigin 0 0 0\nscene ref1.smd\n");
		if (jj > 0) dpmtextsize += jj;
		for (i = 0; i < mod->numframes; i = j) {
			strlcpy(animname, mod->animscenes[i].name, sizeof(animname));
			first = mod->animscenes[i].firstframe;
			if (mod->animscenes[i].framecount > 1)
			{
				// framegroup anim
				count = mod->animscenes[i].framecount;
				j = i + 1;
			}
			else
			{
				// individual frame
				// check for additional frames with same name
				for (jj = 0, k = (int)strlen(animname);animname[jj];jj++)
					if (animname[jj] < '0' || animname[jj] > '9')
						k = jj + 1;
				if (k > 0 && animname[k-1] == '_')
					--k;
				animname[k] = 0;
				count = mod->num_poses - first;
				for (j = i + 1;j < mod->numframes;j++)
				{
					strlcpy(animname2, mod->animscenes[j].name, sizeof(animname2));
					for (jj = 0, k = (int)strlen(animname2);animname2[jj];jj++)
						if (animname2[jj] < '0' || animname2[jj] > '9')
							k = jj + 1;
					if (k > 0 && animname[k-1] == '_')
						--k;
					animname2[k] = 0;
					if (strcmp(animname2, animname) || mod->animscenes[j].framecount > 1)
					{
						count = mod->animscenes[j].firstframe - first;
						break;
					}
				}
				// if it's only one frame, use the original frame name
				if (j == i + 1)
					strlcpy(animname, mod->animscenes[i].name, sizeof(animname));

			}
			dpsnprintf(outname, sizeof(outname), "%s_decompiled/%s.smd", basename, animname);
			Mod_Decompile_SMD(mod, outname, first, count, false);
			if (zymtextsize < (int)sizeof(zymtextbuffer) - 100)
			{
				jj = dpsnprintf(zymtextbuffer + zymtextsize, sizeof(zymtextbuffer) - zymtextsize, "scene %s.smd fps %g %s\n", animname, mod->animscenes[i].framerate, mod->animscenes[i].loop ? "" : " noloop");
				if (jj > 0) zymtextsize += jj;
			}
			if (dpmtextsize < (int)sizeof(dpmtextbuffer) - 100)
			{
				jj = dpsnprintf(dpmtextbuffer + dpmtextsize, sizeof(dpmtextbuffer) - dpmtextsize, "scene %s.smd fps %g %s\n", animname, mod->animscenes[i].framerate, mod->animscenes[i].loop ? "" : " noloop");
				if (jj > 0) dpmtextsize += jj;
			}
			if (framegroupstextsize < (int)sizeof(framegroupstextbuffer) - 100)
			{
				jj = dpsnprintf(framegroupstextbuffer + framegroupstextsize, sizeof(framegroupstextbuffer) - framegroupstextsize, "%d %d %f %d // %s\n", first, count, mod->animscenes[i].framerate, mod->animscenes[i].loop, animname);
				if (jj > 0) framegroupstextsize += jj;
			}
		}
		if (zymtextsize)
			FS_WriteFile(va(vabuf, sizeof(vabuf), "%s_decompiled/out_zym.txt", basename), zymtextbuffer, (fs_offset_t)zymtextsize);
		if (dpmtextsize)
			FS_WriteFile(va(vabuf, sizeof(vabuf), "%s_decompiled/out_dpm.txt", basename), dpmtextbuffer, (fs_offset_t)dpmtextsize);
		if (framegroupstextsize)
			FS_WriteFile(va(vabuf, sizeof(vabuf), "%s_decompiled.framegroups", basename), framegroupstextbuffer, (fs_offset_t)framegroupstextsize);
	}
}

WARP_X_ (Mod_Decompile_OBJ)
static int Mod_TextureFindPos_Did_Find (model_t *model, ccs *swanted)
{
	for (int submodelindex = 0;submodelindex < max(1, model->brush.numsubmodels);submodelindex++) {
		model_t *submodel = model->brush.numsubmodels ? model->brush.submodels[submodelindex] : model;
		for (int surfaceindex = submodel->submodelsurfaces_start;surfaceindex < submodel->submodelsurfaces_end;surfaceindex++) {
			const msurface_t *surface = model->data_surfaces + surfaceindex;
			ccs *texname = (surface->texture && surface->texture->name[0]) ? surface->texture->name : "default";

			//for (triangleindex = 0, e = model->surfmesh.data_element3i + surface->num_firsttriangle * 3;
			//	triangleindex < surface->num_triangles;triangleindex++, e += 3) {
			//} // for
			if (String_Contains_Caseless (texname, swanted)) {
				va_super (sorg, 1024, VECTOR3_LOSSLESS, VECTOR3_SEND(surface->mins));
				Clipboard_Set_Text (sorg);
				Con_PrintLinef ("First occurance of texture: %s at " VECTOR3_LOSSLESS " to " VECTOR3_LOSSLESS, texname, VECTOR3_SEND(surface->mins), VECTOR3_SEND(surface->maxs));
				Con_PrintLinef ("Location copied to clipboard");
				Con_PrintLinef ("setpos and press CTRL-V paste can take you there " CON_BRONZE "(you may wish to turn noclip on)");
				return true;
			}
		} // for
	}

	return false;
}


static void Mod_TextureFindPos_f(cmd_state_t *cmd)
{	
	if (!r_refdef.scene.worldmodel) {
		Con_PrintLinef ("No map running");
	}
	
	if (Cmd_Argc(cmd) > 2 || Cmd_Argc(cmd) == 1) {
		Con_PrintLinef ("usage: texturefindpos <partial or full texturename>");
		return;
	}

	
	//ccs *swanted = swanted0;
	ccs *swanted = Cmd_Argv (cmd, 1);
	ccs *swanted1 = NULL; // 2nd check

	int is_q1_animated = swanted[0] == '+';
	char animatedname[1024];
	if (is_q1_animated && swanted[1] && swanted[2]) {
		c_dpsnprintf1 (animatedname, "+0%s", &swanted[2]);
		swanted1 = animatedname;
	}

	ccs *sworld = cl.model_name[1];

	model_t *mod = Mod_ForName(sworld, q_crash_false, q_checkdisk_false, q_parentname_NULL);

	if (!mod) {
		Con_PrintLinef ("Can't load world model");
		return;
	}

	if (!mod->surfmesh.num_triangles) {
		Con_PrintLinef ("Empty model (or sprite)");
		return;
	}

	int did_find = false;
	while (mod->surfmesh.num_triangles) {
		// [0] modeldecompile [1]progs/player.mdl [2] framenum		
		did_find = Mod_TextureFindPos_Did_Find (mod, swanted);

		// If failed and is animated texture ... try the animated texture +0 variant
		if (!did_find && swanted1) {
			Con_PrintLinef ("Animated texture name not found " QUOTED_S " searching for base " 
				QUOTED_S " ...", swanted, animatedname);

			did_find = Mod_TextureFindPos_Did_Find (mod, swanted1);
		}

		break;
	}

	if (!did_find)
		Con_PrintLinef ("No instances of texture %s were found", swanted);
}


void Mod_AllocLightmap_Init(mod_alloclightmap_state_t *state, mempool_t *mempool, int width, int height)
{
	int y;
	memset(state, 0, sizeof(*state));
	state->width = width;
	state->height = height;
	state->currentY = 0;
	state->rows = (mod_alloclightmap_row_t *)Mem_Alloc(mempool, state->height * sizeof(*state->rows));
	for (y = 0;y < state->height;y++)
	{
		state->rows[y].currentX = 0;
		state->rows[y].rowY = -1;
	}
}

void Mod_AllocLightmap_Reset(mod_alloclightmap_state_t *state)
{
	int y;
	state->currentY = 0;
	for (y = 0;y < state->height;y++)
	{
		state->rows[y].currentX = 0;
		state->rows[y].rowY = -1;
	}
}

void Mod_AllocLightmap_Free(mod_alloclightmap_state_t *state)
{
	if (state->rows)
		Mem_Free(state->rows);
	memset(state, 0, sizeof(*state));
}

qbool Mod_AllocLightmap_Block(mod_alloclightmap_state_t *state, int blockwidth, int blockheight, int *outx, int *outy)
{
	mod_alloclightmap_row_t *row;
	int y;

	row = state->rows + blockheight;
	if ((row->rowY < 0) || (row->currentX + blockwidth > state->width))
	{
		if (state->currentY + blockheight <= state->height)
		{
			// use the current allocation position
			row->rowY = state->currentY;
			row->currentX = 0;
			state->currentY += blockheight;
		}
		else
		{
			// find another position
			for (y = blockheight;y < state->height;y++)
			{
				if ((state->rows[y].rowY >= 0) && (state->rows[y].currentX + blockwidth <= state->width))
				{
					row = state->rows + y;
					break;
				}
			}
			if (y == state->height)
				return false;
		}
	}
	*outy = row->rowY;
	*outx = row->currentX;
	row->currentX += blockwidth;

	return true;
}

typedef struct lightmapsample_s
{
	float pos[3];
	float sh1[4][3];
	float *vertex_color;
	unsigned char *lm_bgr;
	unsigned char *lm_dir;
}
lightmapsample_t;

typedef struct lightmapvertex_s
{
	int index;
	float pos[3];
	float normal[3];
	float texcoordbase[2];
	float texcoordlightmap[2];
	float lightcolor[4];
}
lightmapvertex_t;

typedef struct lightmaptriangle_s
{
	int triangleindex;
	int surfaceindex;
	int lightmapindex;
	int axis;
	int lmoffset[2];
	int lmsize[2];
	// 2D modelspace coordinates of min corner
	// snapped to lightmap grid but not in grid coordinates
	float lmbase[2];
	// 2D modelspace to lightmap coordinate scale
	float lmscale[2];
	float vertex[3][3];
	float mins[3];
	float maxs[3];
}
lightmaptriangle_t;

typedef struct lightmaplight_s
{
	float origin[3];
	float radius;
	float iradius;
	float radius2;
	float color[3];
	svbsp_t svbsp;
}
lightmaplight_t;

lightmaptriangle_t *mod_generatelightmaps_lightmaptriangles;

#define MAX_LIGHTMAPSAMPLES 64
static int mod_generatelightmaps_numoffsets[3];
static float mod_generatelightmaps_offsets[3][MAX_LIGHTMAPSAMPLES][3];

static int mod_generatelightmaps_numlights;
static lightmaplight_t *mod_generatelightmaps_lightinfo;

extern cvar_t r_shadow_lightattenuationdividebias;
extern cvar_t r_shadow_lightattenuationlinearscale;

static void Mod_GenerateLightmaps_LightPoint(model_t *model, const vec3_t pos, vec3_t ambient, vec3_t diffuse, vec3_t lightdir)
{
	int i;
	int index;
	int result;
	float relativepoint[3];
	float color[3];
	float dir[3];
	float dist;
	float dist2;
	float intensity;
	float sample[5*3];
	float lightorigin[3];
	float lightradius;
	float lightradius2;
	float lightiradius;
	float lightcolor[3];
	trace_t trace;
	for (i = 0;i < 5*3;i++)
		sample[i] = 0.0f;
	for (index = 0;;index++)
	{
		result = R_Shadow_GetRTLightInfo(index, lightorigin, &lightradius, lightcolor);
		if (result < 0)
			break;
		if (result == 0)
			continue;
		lightradius2 = lightradius * lightradius;
		VectorSubtract(lightorigin, pos, relativepoint);
		dist2 = VectorLength2(relativepoint);
		if (dist2 >= lightradius2)
			continue;
		lightiradius = 1.0f / lightradius;
		dist = sqrt(dist2) * lightiradius;
		intensity = (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist);
		if (intensity <= 0.0f)
			continue;
		if (model && model->TraceLine)
		{
			model->TraceLine(model, NULL, NULL, &trace, pos, lightorigin, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT | MATERIALFLAG_NOSHADOW);
			if (trace.fraction < 1)
				continue;
		}
		// scale down intensity to add to both ambient and diffuse
		//intensity *= 0.5f;
		VectorNormalize(relativepoint);
		VectorScale(lightcolor, intensity, color);
		VectorMA(sample    , 0.5f            , color, sample    );
		VectorMA(sample + 3, relativepoint[0], color, sample + 3);
		VectorMA(sample + 6, relativepoint[1], color, sample + 6);
		VectorMA(sample + 9, relativepoint[2], color, sample + 9);
		// calculate a weighted average light direction as well
		intensity *= VectorLength(color);
		VectorMA(sample + 12, intensity, relativepoint, sample + 12);
	}
	// calculate the direction we'll use to reduce the sample to a directional light source
	VectorCopy(sample + 12, dir);
	//VectorSet(dir, sample[3] + sample[4] + sample[5], sample[6] + sample[7] + sample[8], sample[9] + sample[10] + sample[11]);
	VectorNormalize(dir);
	// extract the diffuse color along the chosen direction and scale it
	diffuse[0] = (dir[0]*sample[3] + dir[1]*sample[6] + dir[2]*sample[ 9] + sample[ 0]);
	diffuse[1] = (dir[0]*sample[4] + dir[1]*sample[7] + dir[2]*sample[10] + sample[ 1]);
	diffuse[2] = (dir[0]*sample[5] + dir[1]*sample[8] + dir[2]*sample[11] + sample[ 2]);
	// subtract some of diffuse from ambient
	VectorMA(sample, -0.333f, diffuse, ambient);
	// store the normalized lightdir
	VectorCopy(dir, lightdir);
}

static void Mod_GenerateLightmaps_CreateLights_ComputeSVBSP_InsertSurfaces(const model_t *model, svbsp_t *svbsp, const float *mins, const float *maxs)
{
	int surfaceindex;
	int triangleindex;
	const msurface_t *surface;
	const float *vertex3f = model->surfmesh.data_vertex3f;
	const int *element3i = model->surfmesh.data_element3i;
	const int *e;
	float v2[3][3];
	for (surfaceindex = model->submodelsurfaces_start;surfaceindex < model->submodelsurfaces_end;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		if (!BoxesOverlap(surface->mins, surface->maxs, mins, maxs))
			continue;
		if (surface->texture->basematerialflags & MATERIALFLAG_NOSHADOW)
			continue;
		for (triangleindex = 0, e = element3i + 3*surface->num_firsttriangle;triangleindex < surface->num_triangles;triangleindex++, e += 3)
		{
			VectorCopy(vertex3f + 3*e[0], v2[0]);
			VectorCopy(vertex3f + 3*e[1], v2[1]);
			VectorCopy(vertex3f + 3*e[2], v2[2]);
			SVBSP_AddPolygon(svbsp, 3, v2[0], true, NULL, NULL, 0);
		}
	}
}

static void Mod_GenerateLightmaps_CreateLights_ComputeSVBSP(model_t *model, lightmaplight_t *lightinfo)
{
	int maxnodes = 1<<14;
	svbsp_node_t *nodes;
	float origin[3];
	float mins[3];
	float maxs[3];
	svbsp_t svbsp;
	VectorSet(mins, lightinfo->origin[0] - lightinfo->radius, lightinfo->origin[1] - lightinfo->radius, lightinfo->origin[2] - lightinfo->radius);
	VectorSet(maxs, lightinfo->origin[0] + lightinfo->radius, lightinfo->origin[1] + lightinfo->radius, lightinfo->origin[2] + lightinfo->radius);
	VectorCopy(lightinfo->origin, origin);
	nodes = (svbsp_node_t *)Mem_Alloc(tempmempool, maxnodes * sizeof(*nodes));
	for (;;)
	{
		SVBSP_Init(&svbsp, origin, maxnodes, nodes);
		Mod_GenerateLightmaps_CreateLights_ComputeSVBSP_InsertSurfaces(model, &svbsp, mins, maxs);
		if (svbsp.ranoutofnodes)
		{
			maxnodes *= 16;
			if (maxnodes > 1<<22)
			{
				Mem_Free(nodes);
				return;
			}
			Mem_Free(nodes);
			nodes = (svbsp_node_t *)Mem_Alloc(tempmempool, maxnodes * sizeof(*nodes));
		}
		else
			break;
	}
	if (svbsp.numnodes > 0)
	{
		svbsp.nodes = (svbsp_node_t *)Mem_Alloc(tempmempool, svbsp.numnodes * sizeof(*nodes));
		memcpy(svbsp.nodes, nodes, svbsp.numnodes * sizeof(*nodes));
		lightinfo->svbsp = svbsp;
	}
	Mem_Free(nodes);
}

static void Mod_GenerateLightmaps_CreateLights(model_t *model)
{
	int index;
	int result;
	lightmaplight_t *lightinfo;
	float origin[3];
	float radius;
	float color[3];
	mod_generatelightmaps_numlights = 0;
	for (index = 0;;index++)
	{
		result = R_Shadow_GetRTLightInfo(index, origin, &radius, color);
		if (result < 0)
			break;
		if (result > 0)
			mod_generatelightmaps_numlights++;
	}
	if (mod_generatelightmaps_numlights > 0)
	{
		mod_generatelightmaps_lightinfo = (lightmaplight_t *)Mem_Alloc(tempmempool, mod_generatelightmaps_numlights * sizeof(*mod_generatelightmaps_lightinfo));
		lightinfo = mod_generatelightmaps_lightinfo;
		for (index = 0;;index++)
		{
			result = R_Shadow_GetRTLightInfo(index, lightinfo->origin, &lightinfo->radius, lightinfo->color);
			if (result < 0)
				break;
			if (result > 0)
				lightinfo++;
		}
	}
	for (index = 0, lightinfo = mod_generatelightmaps_lightinfo;index < mod_generatelightmaps_numlights;index++, lightinfo++)
	{
		lightinfo->iradius = 1.0f / lightinfo->radius;
		lightinfo->radius2 = lightinfo->radius * lightinfo->radius;
		// TODO: compute svbsp
		Mod_GenerateLightmaps_CreateLights_ComputeSVBSP(model, lightinfo);
	}
}

static void Mod_GenerateLightmaps_DestroyLights(model_t *model)
{
	int i;
	if (mod_generatelightmaps_lightinfo)
	{
		for (i = 0;i < mod_generatelightmaps_numlights;i++)
			if (mod_generatelightmaps_lightinfo[i].svbsp.nodes)
				Mem_Free(mod_generatelightmaps_lightinfo[i].svbsp.nodes);
		Mem_Free(mod_generatelightmaps_lightinfo);
	}
	mod_generatelightmaps_lightinfo = NULL;
	mod_generatelightmaps_numlights = 0;
}

static qbool Mod_GenerateLightmaps_SamplePoint_SVBSP(const svbsp_t *svbsp, const float *pos)
{
	const svbsp_node_t *node;
	const svbsp_node_t *nodes = svbsp->nodes;
	int num = 0;
	while (num >= 0)
	{
		node = nodes + num;
		num = node->children[DotProduct(node->plane, pos) < node->plane[3]];
	}
	return num == -1; // true if empty, false if solid (shadowed)
}

static void Mod_GenerateLightmaps_SamplePoint(const float *pos, const float *normal, float *sample, int numoffsets, const float *offsets)
{
	int i;
	float relativepoint[3];
	float color[3];
	float offsetpos[3];
	float dist;
	float dist2;
	float intensity;
	int offsetindex;
	int hits;
	int tests;
	const lightmaplight_t *lightinfo;
	trace_t trace;
	for (i = 0;i < 5*3;i++)
		sample[i] = 0.0f;
	for (i = 0, lightinfo = mod_generatelightmaps_lightinfo;i < mod_generatelightmaps_numlights;i++, lightinfo++)
	{
		//R_SampleRTLights(pos, sample, numoffsets, offsets);
		VectorSubtract(lightinfo->origin, pos, relativepoint);
		// don't accept light from behind a surface, it causes bad shading
		if (normal && DotProduct(relativepoint, normal) <= 0)
			continue;
		dist2 = VectorLength2(relativepoint);
		if (dist2 >= lightinfo->radius2)
			continue;
		dist = sqrt(dist2) * lightinfo->iradius;
		intensity = dist < 1 ? ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) : 0;
		if (intensity <= 0)
			continue;
		if (cl.worldmodel && cl.worldmodel->TraceLine && numoffsets > 0)
		{
			hits = 0;
			tests = 1;
			if (Mod_GenerateLightmaps_SamplePoint_SVBSP(&lightinfo->svbsp, pos))
				hits++;
			for (offsetindex = 1;offsetindex < numoffsets;offsetindex++)
			{
				VectorAdd(pos, offsets + 3*offsetindex, offsetpos);
				if (!normal)
				{
					// for light grid we'd better check visibility of the offset point
					cl.worldmodel->TraceLine(cl.worldmodel, NULL, NULL, &trace, pos, offsetpos, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT | MATERIALFLAG_NOSHADOW);
					if (trace.fraction < 1)
						VectorLerp(pos, trace.fraction, offsetpos, offsetpos);
				}
				tests++;
				if (Mod_GenerateLightmaps_SamplePoint_SVBSP(&lightinfo->svbsp, offsetpos))
					hits++;
			}
			if (!hits)
				continue;
			// scale intensity according to how many rays succeeded
			// we know one test is valid, half of the rest will fail...
			//if (normal && tests > 1)
			//	intensity *= (tests - 1.0f) / tests;
			intensity *= (float)hits / tests;
		}
		// scale down intensity to add to both ambient and diffuse
		//intensity *= 0.5f;
		VectorNormalize(relativepoint);
		VectorScale(lightinfo->color, intensity, color);
		VectorMA(sample    , 0.5f            , color, sample    );
		VectorMA(sample + 3, relativepoint[0], color, sample + 3);
		VectorMA(sample + 6, relativepoint[1], color, sample + 6);
		VectorMA(sample + 9, relativepoint[2], color, sample + 9);
		// calculate a weighted average light direction as well
		intensity *= VectorLength(color);
		VectorMA(sample + 12, intensity, relativepoint, sample + 12);
	}
}

static void Mod_GenerateLightmaps_LightmapSample(const float *pos, const float *normal, unsigned char *lm_bgr, unsigned char *lm_dir)
{
	float sample[5*3];
	float color[3];
	float dir[3];
	float f;
	Mod_GenerateLightmaps_SamplePoint(pos, normal, sample, mod_generatelightmaps_numoffsets[0], mod_generatelightmaps_offsets[0][0]);
	//VectorSet(dir, sample[3] + sample[4] + sample[5], sample[6] + sample[7] + sample[8], sample[9] + sample[10] + sample[11]);
	VectorCopy(sample + 12, dir);
	VectorNormalize(dir);
	//VectorAdd(dir, normal, dir);
	//VectorNormalize(dir);
	f = DotProduct(dir, normal);
	f = max(0, f) * 255.0f;
	VectorScale(sample, f, color);
	//VectorCopy(normal, dir);
	VectorSet(dir, (dir[0]+1.0f)*127.5f, (dir[1]+1.0f)*127.5f, (dir[2]+1.0f)*127.5f);
	lm_bgr[0] = (unsigned char)bound(0.0f, color[2], 255.0f);
	lm_bgr[1] = (unsigned char)bound(0.0f, color[1], 255.0f);
	lm_bgr[2] = (unsigned char)bound(0.0f, color[0], 255.0f);
	lm_bgr[3] = 255;
	lm_dir[0] = (unsigned char)dir[2];
	lm_dir[1] = (unsigned char)dir[1];
	lm_dir[2] = (unsigned char)dir[0];
	lm_dir[3] = 255;
}

static void Mod_GenerateLightmaps_VertexSample(const float *pos, const float *normal, float *vertex_color)
{
	float sample[5*3];
	Mod_GenerateLightmaps_SamplePoint(pos, normal, sample, mod_generatelightmaps_numoffsets[1], mod_generatelightmaps_offsets[1][0]);
	VectorCopy(sample, vertex_color);
}

static void Mod_GenerateLightmaps_GridSample(const float *pos, q3dlightgrid_t *s)
{
	float sample[5*3];
	float ambient[3];
	float diffuse[3];
	float dir[3];
	Mod_GenerateLightmaps_SamplePoint(pos, NULL, sample, mod_generatelightmaps_numoffsets[2], mod_generatelightmaps_offsets[2][0]);
	// calculate the direction we'll use to reduce the sample to a directional light source
	VectorCopy(sample + 12, dir);
	//VectorSet(dir, sample[3] + sample[4] + sample[5], sample[6] + sample[7] + sample[8], sample[9] + sample[10] + sample[11]);
	VectorNormalize(dir);
	// extract the diffuse color along the chosen direction and scale it
	diffuse[0] = (dir[0]*sample[3] + dir[1]*sample[6] + dir[2]*sample[ 9] + sample[ 0]) * 127.5f;
	diffuse[1] = (dir[0]*sample[4] + dir[1]*sample[7] + dir[2]*sample[10] + sample[ 1]) * 127.5f;
	diffuse[2] = (dir[0]*sample[5] + dir[1]*sample[8] + dir[2]*sample[11] + sample[ 2]) * 127.5f;
	// scale the ambient from 0-2 to 0-255 and subtract some of diffuse
	VectorScale(sample, 127.5f, ambient);
	VectorMA(ambient, -0.333f, diffuse, ambient);
	// encode to the grid format
	s->ambientrgb[0] = (unsigned char)bound(0.0f, ambient[0], 255.0f);
	s->ambientrgb[1] = (unsigned char)bound(0.0f, ambient[1], 255.0f);
	s->ambientrgb[2] = (unsigned char)bound(0.0f, ambient[2], 255.0f);
	s->diffusergb[0] = (unsigned char)bound(0.0f, diffuse[0], 255.0f);
	s->diffusergb[1] = (unsigned char)bound(0.0f, diffuse[1], 255.0f);
	s->diffusergb[2] = (unsigned char)bound(0.0f, diffuse[2], 255.0f);
	if (dir[2] >= 0.99f) {s->diffusepitch = 0;s->diffuseyaw = 0;}
	else if (dir[2] <= -0.99f) {s->diffusepitch = 128;s->diffuseyaw = 0;}
	else {s->diffusepitch = (unsigned char)(acos(dir[2]) * (127.5f/M_PI));s->diffuseyaw = (unsigned char)(atan2(dir[1], dir[0]) * (127.5f/M_PI));}
}

static void Mod_GenerateLightmaps_InitSampleOffsets(model_t *model)
{
	float radius[3];
	float temp[3];
	int i, j;
	memset(mod_generatelightmaps_offsets, 0, sizeof(mod_generatelightmaps_offsets));
	mod_generatelightmaps_numoffsets[0] = min(MAX_LIGHTMAPSAMPLES, mod_generatelightmaps_lightmapsamples.integer);
	mod_generatelightmaps_numoffsets[1] = min(MAX_LIGHTMAPSAMPLES, mod_generatelightmaps_vertexsamples.integer);
	mod_generatelightmaps_numoffsets[2] = min(MAX_LIGHTMAPSAMPLES, mod_generatelightmaps_gridsamples.integer);
	radius[0] = mod_generatelightmaps_lightmapradius.value;
	radius[1] = mod_generatelightmaps_vertexradius.value;
	radius[2] = mod_generatelightmaps_gridradius.value;
	for (i = 0;i < 3;i++)
	{
		for (j = 1;j < mod_generatelightmaps_numoffsets[i];j++)
		{
			VectorRandom(temp);
			VectorScale(temp, radius[i], mod_generatelightmaps_offsets[i][j]);
		}
	}
}

static void Mod_GenerateLightmaps_DestroyLightmaps(model_t *model)
{
	msurface_t *surface;
	int surfaceindex;
	int i;
	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		surface->lightmaptexture = NULL;
		surface->deluxemaptexture = NULL;
	}
	if (model->brushq3.data_lightmaps)
	{
		for (i = 0;i < model->brushq3.num_mergedlightmaps;i++)
			if (model->brushq3.data_lightmaps[i])
				R_FreeTexture(model->brushq3.data_lightmaps[i]);
		Mem_Free(model->brushq3.data_lightmaps);
		model->brushq3.data_lightmaps = NULL;
	}
	if (model->brushq3.data_deluxemaps)
	{
		for (i = 0;i < model->brushq3.num_mergedlightmaps;i++)
			if (model->brushq3.data_deluxemaps[i])
				R_FreeTexture(model->brushq3.data_deluxemaps[i]);
		Mem_Free(model->brushq3.data_deluxemaps);
		model->brushq3.data_deluxemaps = NULL;
	}
}

static void Mod_GenerateLightmaps_UnweldTriangles(model_t *model)
{
	msurface_t *surface;
	int surfaceindex;
	int vertexindex;
	int outvertexindex;
	int i;
	const int *e;
	surfmesh_t oldsurfmesh;
	size_t size;
	unsigned char *data;
	oldsurfmesh = model->surfmesh;
	model->surfmesh.num_triangles = oldsurfmesh.num_triangles;
	model->surfmesh.num_vertices = oldsurfmesh.num_triangles * 3;
	size = 0;
	size += model->surfmesh.num_vertices * sizeof(float[3]);
	size += model->surfmesh.num_vertices * sizeof(float[3]);
	size += model->surfmesh.num_vertices * sizeof(float[3]);
	size += model->surfmesh.num_vertices * sizeof(float[3]);
	size += model->surfmesh.num_vertices * sizeof(float[2]);
	size += model->surfmesh.num_vertices * sizeof(float[2]);
	size += model->surfmesh.num_vertices * sizeof(float[4]);
	data = (unsigned char *)Mem_Alloc(model->mempool, size);
	model->surfmesh.data_vertex3f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[3]);
	model->surfmesh.data_normal3f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[3]);
	model->surfmesh.data_svector3f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[3]);
	model->surfmesh.data_tvector3f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[3]);
	model->surfmesh.data_texcoordtexture2f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[2]);
	model->surfmesh.data_texcoordlightmap2f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[2]);
	model->surfmesh.data_lightmapcolor4f = (float *)data;data += model->surfmesh.num_vertices * sizeof(float[4]);
	if (model->surfmesh.num_vertices > 65536)
		model->surfmesh.data_element3s = NULL;

	if (model->surfmesh.data_element3i_indexbuffer && !model->surfmesh.data_element3i_indexbuffer->isdynamic)
		R_Mesh_DestroyMeshBuffer(model->surfmesh.data_element3i_indexbuffer);
	model->surfmesh.data_element3i_indexbuffer = NULL;
	if (model->surfmesh.data_element3s_indexbuffer && !model->surfmesh.data_element3s_indexbuffer->isdynamic)
		R_Mesh_DestroyMeshBuffer(model->surfmesh.data_element3s_indexbuffer);
	model->surfmesh.data_element3s_indexbuffer = NULL;
	if (model->surfmesh.data_vertex3f_vertexbuffer && !model->surfmesh.data_vertex3f_vertexbuffer->isdynamic)
		R_Mesh_DestroyMeshBuffer(model->surfmesh.data_vertex3f_vertexbuffer);
	model->surfmesh.data_vertex3f_vertexbuffer = NULL;
	model->surfmesh.data_svector3f_vertexbuffer = NULL;
	model->surfmesh.data_tvector3f_vertexbuffer = NULL;
	model->surfmesh.data_normal3f_vertexbuffer = NULL;
	model->surfmesh.data_texcoordtexture2f_vertexbuffer = NULL;
	model->surfmesh.data_texcoordlightmap2f_vertexbuffer = NULL;
	model->surfmesh.data_lightmapcolor4f_vertexbuffer = NULL;
	model->surfmesh.data_skeletalindex4ub_vertexbuffer = NULL;
	model->surfmesh.data_skeletalweight4ub_vertexbuffer = NULL;

	// convert all triangles to unique vertex data
	outvertexindex = 0;
	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		surface->num_firstvertex = outvertexindex;
		surface->num_vertices = surface->num_triangles*3;
		e = oldsurfmesh.data_element3i + surface->num_firsttriangle*3;
		for (i = 0;i < surface->num_triangles*3;i++)
		{
			vertexindex = e[i];
			model->surfmesh.data_vertex3f[outvertexindex*3+0] = oldsurfmesh.data_vertex3f[vertexindex*3+0];
			model->surfmesh.data_vertex3f[outvertexindex*3+1] = oldsurfmesh.data_vertex3f[vertexindex*3+1];
			model->surfmesh.data_vertex3f[outvertexindex*3+2] = oldsurfmesh.data_vertex3f[vertexindex*3+2];
			model->surfmesh.data_normal3f[outvertexindex*3+0] = oldsurfmesh.data_normal3f[vertexindex*3+0];
			model->surfmesh.data_normal3f[outvertexindex*3+1] = oldsurfmesh.data_normal3f[vertexindex*3+1];
			model->surfmesh.data_normal3f[outvertexindex*3+2] = oldsurfmesh.data_normal3f[vertexindex*3+2];
			model->surfmesh.data_svector3f[outvertexindex*3+0] = oldsurfmesh.data_svector3f[vertexindex*3+0];
			model->surfmesh.data_svector3f[outvertexindex*3+1] = oldsurfmesh.data_svector3f[vertexindex*3+1];
			model->surfmesh.data_svector3f[outvertexindex*3+2] = oldsurfmesh.data_svector3f[vertexindex*3+2];
			model->surfmesh.data_tvector3f[outvertexindex*3+0] = oldsurfmesh.data_tvector3f[vertexindex*3+0];
			model->surfmesh.data_tvector3f[outvertexindex*3+1] = oldsurfmesh.data_tvector3f[vertexindex*3+1];
			model->surfmesh.data_tvector3f[outvertexindex*3+2] = oldsurfmesh.data_tvector3f[vertexindex*3+2];
			model->surfmesh.data_texcoordtexture2f[outvertexindex*2+0] = oldsurfmesh.data_texcoordtexture2f[vertexindex*2+0];
			model->surfmesh.data_texcoordtexture2f[outvertexindex*2+1] = oldsurfmesh.data_texcoordtexture2f[vertexindex*2+1];
			if (oldsurfmesh.data_texcoordlightmap2f)
			{
				model->surfmesh.data_texcoordlightmap2f[outvertexindex*2+0] = oldsurfmesh.data_texcoordlightmap2f[vertexindex*2+0];
				model->surfmesh.data_texcoordlightmap2f[outvertexindex*2+1] = oldsurfmesh.data_texcoordlightmap2f[vertexindex*2+1];
			}
			if (oldsurfmesh.data_lightmapcolor4f)
			{
				model->surfmesh.data_lightmapcolor4f[outvertexindex*4+0] = oldsurfmesh.data_lightmapcolor4f[vertexindex*4+0];
				model->surfmesh.data_lightmapcolor4f[outvertexindex*4+1] = oldsurfmesh.data_lightmapcolor4f[vertexindex*4+1];
				model->surfmesh.data_lightmapcolor4f[outvertexindex*4+2] = oldsurfmesh.data_lightmapcolor4f[vertexindex*4+2];
				model->surfmesh.data_lightmapcolor4f[outvertexindex*4+3] = oldsurfmesh.data_lightmapcolor4f[vertexindex*4+3];
			}
			else
				Vector4Set(model->surfmesh.data_lightmapcolor4f + 4*outvertexindex, 1, 1, 1, 1);
			model->surfmesh.data_element3i[surface->num_firsttriangle*3+i] = outvertexindex;
			outvertexindex++;
		}
	}
	if (model->surfmesh.data_element3s)
		for (i = 0;i < model->surfmesh.num_triangles*3;i++)
			model->surfmesh.data_element3s[i] = model->surfmesh.data_element3i[i];

	// find and update all submodels to use this new surfmesh data
	for (i = 0;i < model->brush.numsubmodels;i++)
		model->brush.submodels[i]->surfmesh = model->surfmesh;
}

static void Mod_GenerateLightmaps_CreateTriangleInformation(model_t *model)
{
	msurface_t *surface;
	int surfaceindex;
	int i;
	int axis;
	float normal[3];
	const int *e;
	lightmaptriangle_t *triangle;
	// generate lightmap triangle structs
	mod_generatelightmaps_lightmaptriangles = (lightmaptriangle_t *)Mem_Alloc(model->mempool, model->surfmesh.num_triangles * sizeof(lightmaptriangle_t));
	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		e = model->surfmesh.data_element3i + surface->num_firsttriangle*3;
		for (i = 0;i < surface->num_triangles;i++)
		{
			triangle = &mod_generatelightmaps_lightmaptriangles[surface->num_firsttriangle+i];
			triangle->triangleindex = surface->num_firsttriangle+i;
			triangle->surfaceindex = surfaceindex;
			VectorCopy(model->surfmesh.data_vertex3f + 3*e[i*3+0], triangle->vertex[0]);
			VectorCopy(model->surfmesh.data_vertex3f + 3*e[i*3+1], triangle->vertex[1]);
			VectorCopy(model->surfmesh.data_vertex3f + 3*e[i*3+2], triangle->vertex[2]);
			// calculate bounds of triangle
			triangle->mins[0] = min(triangle->vertex[0][0], min(triangle->vertex[1][0], triangle->vertex[2][0]));
			triangle->mins[1] = min(triangle->vertex[0][1], min(triangle->vertex[1][1], triangle->vertex[2][1]));
			triangle->mins[2] = min(triangle->vertex[0][2], min(triangle->vertex[1][2], triangle->vertex[2][2]));
			triangle->maxs[0] = max(triangle->vertex[0][0], max(triangle->vertex[1][0], triangle->vertex[2][0]));
			triangle->maxs[1] = max(triangle->vertex[0][1], max(triangle->vertex[1][1], triangle->vertex[2][1]));
			triangle->maxs[2] = max(triangle->vertex[0][2], max(triangle->vertex[1][2], triangle->vertex[2][2]));
			// pick an axial projection based on the triangle normal
			TriangleNormal(triangle->vertex[0], triangle->vertex[1], triangle->vertex[2], normal);
			axis = 0;
			if (fabs(normal[1]) > fabs(normal[axis]))
				axis = 1;
			if (fabs(normal[2]) > fabs(normal[axis]))
				axis = 2;
			triangle->axis = axis;
		}
	}
}

static void Mod_GenerateLightmaps_DestroyTriangleInformation(model_t *model)
{
	if (mod_generatelightmaps_lightmaptriangles)
		Mem_Free(mod_generatelightmaps_lightmaptriangles);
	mod_generatelightmaps_lightmaptriangles = NULL;
}

float lmaxis[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

static void Mod_GenerateLightmaps_CreateLightmaps(model_t *model)
{
	msurface_t *surface;
	int surfaceindex;
	int lightmapindex;
	int lightmapnumber;
	int i;
	int j;
	int k;
	int x;
	int y;
	int axis;
	int axis1;
	int axis2;
	int retry;
	int pixeloffset;
	float trianglenormal[3];
	float samplecenter[3];
	float samplenormal[3];
	float temp[3];
	float lmiscale[2];
	float slopex;
	float slopey;
	float slopebase;
	float lmscalepixels;
	float lmmins;
	float lmmaxs;
	float lm_basescalepixels;
	int lm_borderpixels;
	int lm_texturesize;
	//int lm_maxpixels;
	const int *e;
	lightmaptriangle_t *triangle;
	unsigned char *lightmappixels;
	unsigned char *deluxemappixels;
	mod_alloclightmap_state_t lmstate;
	char vabuf[1024];

	// generate lightmap projection information for all triangles
	if (model->texturepool == NULL)
		model->texturepool = R_AllocTexturePool();
	lm_basescalepixels = 1.0f / max(0.0001f, mod_generatelightmaps_unitspersample.value);
	lm_borderpixels = mod_generatelightmaps_borderpixels.integer;
	lm_texturesize = bound(lm_borderpixels*2+1, 64, (int)vid.maxtexturesize_2d);
	//lm_maxpixels = lm_texturesize-(lm_borderpixels*2+1);
	Mod_AllocLightmap_Init(&lmstate, loadmodel->mempool, lm_texturesize, lm_texturesize);
	lightmapnumber = 0;
	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		e = model->surfmesh.data_element3i + surface->num_firsttriangle*3;
		lmscalepixels = lm_basescalepixels;
		for (retry = 0;retry < 30;retry++)
		{
			// after a couple failed attempts, degrade quality to make it fit
			if (retry > 1)
				lmscalepixels *= 0.5f;
			for (i = 0;i < surface->num_triangles;i++)
			{
				triangle = &mod_generatelightmaps_lightmaptriangles[surface->num_firsttriangle+i];
				triangle->lightmapindex = lightmapnumber;
				// calculate lightmap bounds in 3D pixel coordinates, limit size,
				// pick two planar axes for projection
				// lightmap coordinates here are in pixels
				// lightmap projections are snapped to pixel grid explicitly, such
				// that two neighboring triangles sharing an edge and projection
				// axis will have identical sample spacing along their shared edge
				k = 0;
				for (j = 0;j < 3;j++)
				{
					if (j == triangle->axis)
						continue;
					lmmins = floor(triangle->mins[j]*lmscalepixels)-lm_borderpixels;
					lmmaxs = floor(triangle->maxs[j]*lmscalepixels)+lm_borderpixels;
					triangle->lmsize[k] = (int)(lmmaxs-lmmins);
					triangle->lmbase[k] = lmmins/lmscalepixels;
					triangle->lmscale[k] = lmscalepixels;
					k++;
				}
				if (!Mod_AllocLightmap_Block(&lmstate, triangle->lmsize[0], triangle->lmsize[1], &triangle->lmoffset[0], &triangle->lmoffset[1]))
					break;
			}
			// if all fit in this texture, we're done with this surface
			if (i == surface->num_triangles)
				break;
			// if we haven't maxed out the lightmap size yet, we retry the
			// entire surface batch...
			if (lm_texturesize * 2 <= min(mod_generatelightmaps_texturesize.integer, (int)vid.maxtexturesize_2d))
			{
				lm_texturesize *= 2;
				surfaceindex = -1;
				lightmapnumber = 0;
				Mod_AllocLightmap_Free(&lmstate);
				Mod_AllocLightmap_Init(&lmstate, loadmodel->mempool, lm_texturesize, lm_texturesize);
				break;
			}
			// if we have maxed out the lightmap size, and this triangle does
			// not fit in the same texture as the rest of the surface, we have
			// to retry the entire surface in a new texture (can only use one)
			// with multiple retries, the lightmap quality degrades until it
			// fits (or gives up)
			if (surfaceindex > 0)
				lightmapnumber++;
			Mod_AllocLightmap_Reset(&lmstate);
		}
	}
	lightmapnumber++;
	Mod_AllocLightmap_Free(&lmstate);

	// now put triangles together into lightmap textures, and do not allow
	// triangles of a surface to go into different textures (as that would
	// require rewriting the surface list)
	model->brushq3.deluxemapping_modelspace = true;
	model->brushq3.deluxemapping = true;
	model->brushq3.num_mergedlightmaps = lightmapnumber;
	model->brushq3.data_lightmaps = (rtexture_t **)Mem_Alloc(model->mempool, model->brushq3.num_mergedlightmaps * sizeof(rtexture_t *));
	model->brushq3.data_deluxemaps = (rtexture_t **)Mem_Alloc(model->mempool, model->brushq3.num_mergedlightmaps * sizeof(rtexture_t *));
	lightmappixels = (unsigned char *)Mem_Alloc(tempmempool, model->brushq3.num_mergedlightmaps * lm_texturesize * lm_texturesize * 4);
	deluxemappixels = (unsigned char *)Mem_Alloc(tempmempool, model->brushq3.num_mergedlightmaps * lm_texturesize * lm_texturesize * 4);
	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		e = model->surfmesh.data_element3i + surface->num_firsttriangle*3;
		for (i = 0;i < surface->num_triangles;i++)
		{
			triangle = &mod_generatelightmaps_lightmaptriangles[surface->num_firsttriangle+i];
			TriangleNormal(triangle->vertex[0], triangle->vertex[1], triangle->vertex[2], trianglenormal);
			VectorNormalize(trianglenormal);
			VectorCopy(trianglenormal, samplenormal); // FIXME: this is supposed to be interpolated per pixel from vertices
			axis = triangle->axis;
			axis1 = axis == 0 ? 1 : 0;
			axis2 = axis == 2 ? 1 : 2;
			lmiscale[0] = 1.0f / triangle->lmscale[0];
			lmiscale[1] = 1.0f / triangle->lmscale[1];
			if (trianglenormal[axis] < 0)
				VectorNegate(trianglenormal, trianglenormal);
			CrossProduct(lmaxis[axis2], trianglenormal, temp);slopex = temp[axis] / temp[axis1];
			CrossProduct(lmaxis[axis1], trianglenormal, temp);slopey = temp[axis] / temp[axis2];
			slopebase = triangle->vertex[0][axis] - triangle->vertex[0][axis1]*slopex - triangle->vertex[0][axis2]*slopey;
			for (j = 0;j < 3;j++)
			{
				float *t2f = model->surfmesh.data_texcoordlightmap2f + e[i*3+j]*2;
				t2f[0] = ((triangle->vertex[j][axis1] - triangle->lmbase[0]) * triangle->lmscale[0] + triangle->lmoffset[0]) / lm_texturesize;
				t2f[1] = ((triangle->vertex[j][axis2] - triangle->lmbase[1]) * triangle->lmscale[1] + triangle->lmoffset[1]) / lm_texturesize;
#if 0
				samplecenter[axis1] = (t2f[0]*lm_texturesize-triangle->lmoffset[0])*lmiscale[0] + triangle->lmbase[0];
				samplecenter[axis2] = (t2f[1]*lm_texturesize-triangle->lmoffset[1])*lmiscale[1] + triangle->lmbase[1];
				samplecenter[axis] = samplecenter[axis1]*slopex + samplecenter[axis2]*slopey + slopebase;
				Con_Printf ("%f:%f %f:%f %f:%f = %f %f\n", triangle->vertex[j][axis1], samplecenter[axis1], triangle->vertex[j][axis2], samplecenter[axis2], triangle->vertex[j][axis], samplecenter[axis], t2f[0], t2f[1]);
#endif
			}

#if 0
			switch (axis)
			{
			default:
			case 0:
				forward[0] = 0;
				forward[1] = 1.0f / triangle->lmscale[0];
				forward[2] = 0;
				left[0] = 0;
				left[1] = 0;
				left[2] = 1.0f / triangle->lmscale[1];
				up[0] = 1.0f;
				up[1] = 0;
				up[2] = 0;
				origin[0] = 0;
				origin[1] = triangle->lmbase[0];
				origin[2] = triangle->lmbase[1];
				break;
			case 1:
				forward[0] = 1.0f / triangle->lmscale[0];
				forward[1] = 0;
				forward[2] = 0;
				left[0] = 0;
				left[1] = 0;
				left[2] = 1.0f / triangle->lmscale[1];
				up[0] = 0;
				up[1] = 1.0f;
				up[2] = 0;
				origin[0] = triangle->lmbase[0];
				origin[1] = 0;
				origin[2] = triangle->lmbase[1];
				break;
			case 2:
				forward[0] = 1.0f / triangle->lmscale[0];
				forward[1] = 0;
				forward[2] = 0;
				left[0] = 0;
				left[1] = 1.0f / triangle->lmscale[1];
				left[2] = 0;
				up[0] = 0;
				up[1] = 0;
				up[2] = 1.0f;
				origin[0] = triangle->lmbase[0];
				origin[1] = triangle->lmbase[1];
				origin[2] = 0;
				break;
			}
			Matrix4x4_FromVectors(&backmatrix, forward, left, up, origin);
#endif
#define LM_DIST_EPSILON (1.0f / 32.0f)
			for (y = 0;y < triangle->lmsize[1];y++)
			{
				pixeloffset = ((triangle->lightmapindex * lm_texturesize + y + triangle->lmoffset[1]) * lm_texturesize + triangle->lmoffset[0]) * 4;
				for (x = 0;x < triangle->lmsize[0];x++, pixeloffset += 4)
				{
					samplecenter[axis1] = (x+0.5f)*lmiscale[0] + triangle->lmbase[0];
					samplecenter[axis2] = (y+0.5f)*lmiscale[1] + triangle->lmbase[1];
					samplecenter[axis] = samplecenter[axis1]*slopex + samplecenter[axis2]*slopey + slopebase;
					VectorMA(samplecenter, 0.125f, samplenormal, samplecenter);
					Mod_GenerateLightmaps_LightmapSample(samplecenter, samplenormal, lightmappixels + pixeloffset, deluxemappixels + pixeloffset);
				}
			}
		}
	}

	for (lightmapindex = 0;lightmapindex < model->brushq3.num_mergedlightmaps;lightmapindex++)
	{
		model->brushq3.data_lightmaps[lightmapindex] = R_LoadTexture2D(model->texturepool, va(vabuf, sizeof(vabuf), "lightmap%d", lightmapindex), lm_texturesize, lm_texturesize, lightmappixels + lightmapindex * lm_texturesize * lm_texturesize * 4, TEXTYPE_BGRA, TEXF_FORCELINEAR, q_tx_miplevel_neg1, q_tx_palette_NULL);
		model->brushq3.data_deluxemaps[lightmapindex] = R_LoadTexture2D(model->texturepool, va(vabuf, sizeof(vabuf), "deluxemap%d", lightmapindex), lm_texturesize, lm_texturesize, deluxemappixels + lightmapindex * lm_texturesize * lm_texturesize * 4, TEXTYPE_BGRA, TEXF_FORCELINEAR, q_tx_miplevel_neg1, q_tx_palette_NULL);
	}

	if (lightmappixels)
		Mem_Free(lightmappixels);
	if (deluxemappixels)
		Mem_Free(deluxemappixels);

	for (surfaceindex = 0;surfaceindex < model->num_surfaces;surfaceindex++)
	{
		surface = model->data_surfaces + surfaceindex;
		if (!surface->num_triangles)
			continue;
		lightmapindex = mod_generatelightmaps_lightmaptriangles[surface->num_firsttriangle].lightmapindex;
		surface->lightmaptexture = model->brushq3.data_lightmaps[lightmapindex]; // LM_OID
		surface->deluxemaptexture = model->brushq3.data_deluxemaps[lightmapindex];
		surface->lightmapinfo = NULL;
	}

	model->brush.LightPoint = Mod_GenerateLightmaps_LightPoint;
	model->brushq1.lightdata = NULL;
	model->brushq1.lightmapupdateflags = NULL;
	model->brushq1.firstrender = false;
	model->brushq1.num_lightstyles = 0;
	model->brushq1.data_lightstyleinfo = NULL;
	for (i = 0;i < model->brush.numsubmodels;i++)
	{
		model->brush.submodels[i]->brushq1.lightmapupdateflags = NULL;
		model->brush.submodels[i]->brushq1.firstrender = false;
		model->brush.submodels[i]->brushq1.num_lightstyles = 0;
		model->brush.submodels[i]->brushq1.data_lightstyleinfo = NULL;
	}
}

static void Mod_GenerateLightmaps_UpdateVertexColors(model_t *model)
{
	int i;
	for (i = 0;i < model->surfmesh.num_vertices;i++)
		Mod_GenerateLightmaps_VertexSample(model->surfmesh.data_vertex3f + 3*i, model->surfmesh.data_normal3f + 3*i, model->surfmesh.data_lightmapcolor4f + 4*i);
}

static void Mod_GenerateLightmaps_UpdateLightGrid(model_t *model)
{
	int x;
	int y;
	int z;
	int index = 0;
	float pos[3];
	for (z = 0;z < model->brushq3.num_lightgrid_isize[2];z++)
	{
		pos[2] = (model->brushq3.num_lightgrid_imins[2] + z + 0.5f) * model->brushq3.num_lightgrid_cellsize[2];
		for (y = 0;y < model->brushq3.num_lightgrid_isize[1];y++)
		{
			pos[1] = (model->brushq3.num_lightgrid_imins[1] + y + 0.5f) * model->brushq3.num_lightgrid_cellsize[1];
			for (x = 0;x < model->brushq3.num_lightgrid_isize[0];x++, index++)
			{
				pos[0] = (model->brushq3.num_lightgrid_imins[0] + x + 0.5f) * model->brushq3.num_lightgrid_cellsize[0];
				Mod_GenerateLightmaps_GridSample(pos, model->brushq3.data_lightgrid + index);
			}
		}
	}
}

extern cvar_t mod_q3bsp_nolightmaps;
static void Mod_GenerateLightmaps(model_t *model)
{
	//lightmaptriangle_t *lightmaptriangles = Mem_Alloc(model->mempool, model->surfmesh.num_triangles * sizeof(lightmaptriangle_t));
	model_t *oldloadmodel = loadmodel;
	loadmodel = model;

	Mod_GenerateLightmaps_InitSampleOffsets(model);
	Mod_GenerateLightmaps_DestroyLightmaps(model);
	Mod_GenerateLightmaps_UnweldTriangles(model);
	Mod_GenerateLightmaps_CreateTriangleInformation(model);
	Mod_GenerateLightmaps_CreateLights(model);
	if (!mod_q3bsp_nolightmaps.integer /*d: 0*/)
		Mod_GenerateLightmaps_CreateLightmaps(model);
	Mod_GenerateLightmaps_UpdateVertexColors(model);
	Mod_GenerateLightmaps_UpdateLightGrid(model);
	Mod_GenerateLightmaps_DestroyLights(model);
	Mod_GenerateLightmaps_DestroyTriangleInformation(model);

	loadmodel = oldloadmodel;
}

static void Mod_GenerateLightmaps_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 1)
	{
		Con_Printf ("usage: mod_generatelightmaps\n");
		return;
	}
	if (!cl.worldmodel)
	{
		Con_Printf ("no worldmodel loaded\n");
		return;
	}
	Mod_GenerateLightmaps(cl.worldmodel);
}

void Mod_Mesh_Create(model_t *mod, const char *name)
{
	memset(mod, 0, sizeof(*mod));
	strlcpy(mod->model_name, name, sizeof(mod->model_name));
	mod->mempool = Mem_AllocPool(name, 0, NULL);
	mod->texturepool = R_AllocTexturePool();
	mod->Draw = R_Mod_Draw;
	mod->DrawDepth = R_Mod_DrawDepth;
	mod->DrawDebug = R_Mod_DrawDebug;
	mod->DrawPrepass = R_Mod_DrawPrepass;
	mod->GetLightInfo = R_Mod_GetLightInfo;
	mod->DrawShadowMap = R_Mod_DrawShadowMap;
	mod->DrawLight = R_Mod_DrawLight;
}

void Mod_Mesh_Destroy(model_t *mod)
{
	Mod_UnloadModel(mod);
}

// resets the mesh model to have no geometry to render, ready for a new frame -
// the mesh will be prepared for rendering later using Mod_Mesh_Finalize
void Mod_Mesh_Reset(model_t *mod)
{
	mod->num_surfaces = 0;
	mod->surfmesh.num_vertices = 0;
	mod->surfmesh.num_triangles = 0;

	// Baker: Yes, it really does try to memset 0 for a NULL!  DarkPlaces Beta comment was right
	if (mod->surfmesh.data_vertexhash)
		memset(mod->surfmesh.data_vertexhash, -1, mod->surfmesh.num_vertexhashsize * sizeof(*mod->surfmesh.data_vertexhash));
	mod->DrawSky = NULL; // will be set if a texture needs it
	mod->DrawAddWaterPlanes = NULL; // will be set if a texture needs it
}

texture_t *Mod_Mesh_GetTexture(model_t *mod, const char *name, int defaultdrawflags, int defaulttexflags, int defaultmaterialflags)
{
	int i;
	texture_t *t;
	int drawflag = defaultdrawflags & DRAWFLAG_MASK;

	// Is it existing?
	for (i = 0, t = mod->data_textures; i < mod->num_textures; i++, t++)
		if (String_Match(t->name, name) && t->mesh_drawflag == drawflag 
			&& t->mesh_defaulttexflags == defaulttexflags 
			&& t->mesh_defaultmaterialflags == defaultmaterialflags)
			return t;

	// NEW - UnlinkVideoTexture
	WARP_X_ (DrawQ_SuperPic_Video cl_video_shutdown SuspendVideo)

	if (mod->num_textures >= mod->max_textures) {
		// REALLOC
		texture_t *oldtextures = mod->data_textures;
		mod->max_textures = max(mod->max_textures * 2, 1024);
		mod->data_textures = (texture_t *)Mem_Realloc(mod->mempool, 
			mod->data_textures, mod->max_textures * sizeof(*mod->data_textures));
		// update the pointers
		for (i = 0; i < mod->num_surfaces; i++)
			mod->data_surfaces[i].texture = mod->data_textures + (mod->data_surfaces[i].texture - oldtextures);
	}
	t = &mod->data_textures[mod->num_textures++];

	Mod_LoadTextureFromQ3Shader(mod->mempool, mod->model_name, t, name,
		q_tx_warn_missing_true, q_tx_fallback_notexture_true, q_tx_do_external_true,
		defaulttexflags, defaultmaterialflags);

	t->mesh_drawflag = drawflag;
	t->mesh_defaulttexflags = defaulttexflags;
	t->mesh_defaultmaterialflags = defaultmaterialflags;
	switch (defaultdrawflags & DRAWFLAG_MASK)
	{
	case DRAWFLAG_ADDITIVE: // q3map2 blendfunc add
		t->basematerialflags |= MATERIALFLAG_ADD | MATERIALFLAG_BLENDED;
		t->currentmaterialflags = t->basematerialflags;
		break;
	case DRAWFLAG_MODULATE: // q3map2 calls this?
		t->basematerialflags |= MATERIALFLAG_CUSTOMBLEND | MATERIALFLAG_BLENDED;
		t->currentmaterialflags = t->basematerialflags;
		t->customblendfunc[0] = GL_DST_COLOR;
		t->customblendfunc[1] = GL_ZERO;
		break;
	case DRAWFLAG_2XMODULATE:
		t->basematerialflags |= MATERIALFLAG_CUSTOMBLEND | MATERIALFLAG_BLENDED;
		t->currentmaterialflags = t->basematerialflags;
		t->customblendfunc[0] = GL_DST_COLOR;
		t->customblendfunc[1] = GL_SRC_COLOR;
		break;
	case DRAWFLAG_SCREEN: // sort of like q3map2 blendfunc filter which is gl_zero gl_src_color
		t->basematerialflags |= MATERIALFLAG_CUSTOMBLEND | MATERIALFLAG_BLENDED;
		t->currentmaterialflags = t->basematerialflags;
		t->customblendfunc[0] = GL_ONE_MINUS_DST_COLOR;
		t->customblendfunc[1] = GL_ONE;
		break;
	default:
		break;
	}
	return t;
}

msurface_t *Mod_Mesh_AddSurface(model_t *mod, texture_t *tex, qbool batchwithprevioussurface)
{
	msurface_t *surf;
	// batch if possible; primarily useful for UI rendering where bounding boxes don't matter
	if (batchwithprevioussurface && mod->num_surfaces > 0 && mod->data_surfaces[mod->num_surfaces - 1].texture == tex)
		return mod->data_surfaces + mod->num_surfaces - 1;
	// create new surface
	if (mod->max_surfaces == mod->num_surfaces)
	{
		mod->max_surfaces = 2 * max(mod->num_surfaces, 64);
		mod->data_surfaces = (msurface_t *)Mem_Realloc(mod->mempool, mod->data_surfaces, mod->max_surfaces * sizeof(*mod->data_surfaces));
		mod->modelsurfaces_sorted = (int *)Mem_Realloc(mod->mempool, mod->modelsurfaces_sorted, mod->max_surfaces * sizeof(*mod->modelsurfaces_sorted));
	}
	surf = mod->data_surfaces + mod->num_surfaces;
	mod->num_surfaces++;
	memset(surf, 0, sizeof(*surf));
	surf->texture = tex;
	surf->num_firsttriangle = mod->surfmesh.num_triangles;
	surf->num_firstvertex = mod->surfmesh.num_vertices;
	if (tex->basematerialflags & (MATERIALFLAG_SKY))
		mod->DrawSky = R_Mod_DrawSky;
	if (tex->basematerialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_REFLECTION | MATERIALFLAG_CAMERA))
		mod->DrawAddWaterPlanes = R_Mod_DrawAddWaterPlanes;
	return surf;
}

int Mod_Mesh_IndexForVertex(model_t *mod, msurface_t *surf, float x, float y, float z, float nx, float ny, float nz, float s, float t, float u, float v, float r, float g, float b, float a)
{
	int hashindex, h, vnum, mask;
	surfmesh_t *mesh = &mod->surfmesh;
	if (mesh->max_vertices == mesh->num_vertices)
	{
		mesh->max_vertices = max(mesh->num_vertices * 2, 256);
		mesh->data_vertex3f = (float *)Mem_Realloc(mod->mempool, mesh->data_vertex3f, mesh->max_vertices * sizeof(float[3]));
		mesh->data_svector3f = (float *)Mem_Realloc(mod->mempool, mesh->data_svector3f, mesh->max_vertices * sizeof(float[3]));
		mesh->data_tvector3f = (float *)Mem_Realloc(mod->mempool, mesh->data_tvector3f, mesh->max_vertices * sizeof(float[3]));
		mesh->data_normal3f = (float *)Mem_Realloc(mod->mempool, mesh->data_normal3f, mesh->max_vertices * sizeof(float[3]));
		mesh->data_texcoordtexture2f = (float *)Mem_Realloc(mod->mempool, mesh->data_texcoordtexture2f, mesh->max_vertices * sizeof(float[2]));
		mesh->data_texcoordlightmap2f = (float *)Mem_Realloc(mod->mempool, mesh->data_texcoordlightmap2f, mesh->max_vertices * sizeof(float[2]));
		mesh->data_lightmapcolor4f = (float *)Mem_Realloc(mod->mempool, mesh->data_lightmapcolor4f, mesh->max_vertices * sizeof(float[4]));
		// rebuild the hash table
		mesh->num_vertexhashsize = 4 * mesh->max_vertices;
		mesh->num_vertexhashsize &= ~(mesh->num_vertexhashsize - 1); // round down to pow2
		mesh->data_vertexhash = (int *)Mem_Realloc(mod->mempool, mesh->data_vertexhash, mesh->num_vertexhashsize * sizeof(*mesh->data_vertexhash));
		memset(mesh->data_vertexhash, -1, mesh->num_vertexhashsize * sizeof(*mesh->data_vertexhash));
		mask = mod->surfmesh.num_vertexhashsize - 1;
		// no need to hash the vertices for the entire model, the latest surface will suffice.
		for (vnum = surf ? surf->num_firstvertex : 0; vnum < mesh->num_vertices; vnum++)
		{
			// this uses prime numbers intentionally for computing the hash
			hashindex = (unsigned int)(mesh->data_vertex3f[vnum * 3 + 0] * 2003 + mesh->data_vertex3f[vnum * 3 + 1] * 4001 + mesh->data_vertex3f[vnum * 3 + 2] * 7919 + mesh->data_normal3f[vnum * 3 + 0] * 4097 + mesh->data_normal3f[vnum * 3 + 1] * 257 + mesh->data_normal3f[vnum * 3 + 2] * 17) & mask;
			for (h = hashindex; mesh->data_vertexhash[h] >= 0; h = (h + 1) & mask)
				; // just iterate until we find the terminator
			mesh->data_vertexhash[h] = vnum;
		}
	}
	mask = mod->surfmesh.num_vertexhashsize - 1;
	// this uses prime numbers intentionally for computing the hash
	hashindex = (unsigned int)(x * 2003 + y * 4001 + z * 7919 + nx * 4097 + ny * 257 + nz * 17) & mask;
	// when possible find an identical vertex within the same surface and return it
	for(h = hashindex;(vnum = mesh->data_vertexhash[h]) >= 0;h = (h + 1) & mask)
	{
		if (vnum >= surf->num_firstvertex
		 && mesh->data_vertex3f[vnum * 3 + 0] == x && mesh->data_vertex3f[vnum * 3 + 1] == y && mesh->data_vertex3f[vnum * 3 + 2] == z
		 && mesh->data_normal3f[vnum * 3 + 0] == nx && mesh->data_normal3f[vnum * 3 + 1] == ny && mesh->data_normal3f[vnum * 3 + 2] == nz
		 && mesh->data_texcoordtexture2f[vnum * 2 + 0] == s && mesh->data_texcoordtexture2f[vnum * 2 + 1] == t
		 && mesh->data_texcoordlightmap2f[vnum * 2 + 0] == u && mesh->data_texcoordlightmap2f[vnum * 2 + 1] == v
		 && mesh->data_lightmapcolor4f[vnum * 4 + 0] == r && mesh->data_lightmapcolor4f[vnum * 4 + 1] == g && mesh->data_lightmapcolor4f[vnum * 4 + 2] == b && mesh->data_lightmapcolor4f[vnum * 4 + 3] == a)
			return vnum;
	}
	// add the new vertex
	vnum = mesh->num_vertices++;
	if (surf->num_vertices > 0)
	{
		if (surf->mins[0] > x) surf->mins[0] = x;
		if (surf->mins[1] > y) surf->mins[1] = y;
		if (surf->mins[2] > z) surf->mins[2] = z;
		if (surf->maxs[0] < x) surf->maxs[0] = x;
		if (surf->maxs[1] < y) surf->maxs[1] = y;
		if (surf->maxs[2] < z) surf->maxs[2] = z;
	}
	else
	{
		VectorSet(surf->mins, x, y, z);
		VectorSet(surf->maxs, x, y, z);
	}
	surf->num_vertices = mesh->num_vertices - surf->num_firstvertex;
	mesh->data_vertexhash[h] = vnum;
	mesh->data_vertex3f[vnum * 3 + 0] = x;
	mesh->data_vertex3f[vnum * 3 + 1] = y;
	mesh->data_vertex3f[vnum * 3 + 2] = z;
	mesh->data_normal3f[vnum * 3 + 0] = nx;
	mesh->data_normal3f[vnum * 3 + 1] = ny;
	mesh->data_normal3f[vnum * 3 + 2] = nz;
	mesh->data_texcoordtexture2f[vnum * 2 + 0] = s;
	mesh->data_texcoordtexture2f[vnum * 2 + 1] = t;
	mesh->data_texcoordlightmap2f[vnum * 2 + 0] = u;
	mesh->data_texcoordlightmap2f[vnum * 2 + 1] = v;
	mesh->data_lightmapcolor4f[vnum * 4 + 0] = r;
	mesh->data_lightmapcolor4f[vnum * 4 + 1] = g;
	mesh->data_lightmapcolor4f[vnum * 4 + 2] = b;
	mesh->data_lightmapcolor4f[vnum * 4 + 3] = a;
	return vnum;
}

void Mod_Mesh_AddTriangle(model_t *mod, msurface_t *surf, int e0, int e1, int e2)
{
	surfmesh_t *mesh = &mod->surfmesh;
	if (mesh->max_triangles == mesh->num_triangles)
	{
		mesh->max_triangles = 2 * max(mesh->num_triangles, 128);
		mesh->data_element3s = (unsigned short *)Mem_Realloc(mod->mempool, mesh->data_element3s, mesh->max_triangles * sizeof(unsigned short[3]));
		mesh->data_element3i = (int *)Mem_Realloc(mod->mempool, mesh->data_element3i, mesh->max_triangles * sizeof(int[3]));
	}
	mesh->data_element3s[mesh->num_triangles * 3 + 0] = e0;
	mesh->data_element3s[mesh->num_triangles * 3 + 1] = e1;
	mesh->data_element3s[mesh->num_triangles * 3 + 2] = e2;
	mesh->data_element3i[mesh->num_triangles * 3 + 0] = e0;
	mesh->data_element3i[mesh->num_triangles * 3 + 1] = e1;
	mesh->data_element3i[mesh->num_triangles * 3 + 2] = e2;
	mesh->num_triangles++;
	surf->num_triangles++;
}

static void Mod_Mesh_MakeSortedSurfaces(model_t *mod)
{
	int i, j;
	texture_t *tex;

	// build the sorted surfaces list properly to reduce material setup
	// this is easy because we're just sorting on texture and don't care about the order of textures
	mod->submodelsurfaces_start = 0;
	mod->submodelsurfaces_end = 0;
	for (i = 0; i < mod->num_surfaces; i++)
		mod->data_surfaces[i].included = false;
	for (i = 0; i < mod->num_surfaces; i++)
	{
		if (mod->data_surfaces[i].included)
			continue;
		tex = mod->data_surfaces[i].texture;
		// j = i is intentional
		for (j = i; j < mod->num_surfaces; j++)
		{
			if (!mod->data_surfaces[j].included && mod->data_surfaces[j].texture == tex)
			{
				mod->data_surfaces[j].included = 1;
				mod->modelsurfaces_sorted[mod->submodelsurfaces_end++] = j;
			}
		}
	}
}

static void Mod_Mesh_ComputeBounds(model_t *mod)
{
	int i;
	vec_t x2a, x2b, y2a, y2b, z2a, z2b, x2, y2, z2, yawradius, rotatedradius;

	if (mod->surfmesh.num_vertices > 0)
	{
		// calculate normalmins/normalmaxs
		VectorCopy(mod->surfmesh.data_vertex3f, mod->normalmins);
		VectorCopy(mod->surfmesh.data_vertex3f, mod->normalmaxs);
		for (i = 1; i < mod->surfmesh.num_vertices; i++)
		{
			float x = mod->surfmesh.data_vertex3f[i * 3 + 0];
			float y = mod->surfmesh.data_vertex3f[i * 3 + 1];
			float z = mod->surfmesh.data_vertex3f[i * 3 + 2];
			// expand bounds to include this vertex
			if (mod->normalmins[0] > x) mod->normalmins[0] = x;
			if (mod->normalmins[1] > y) mod->normalmins[1] = y;
			if (mod->normalmins[2] > z) mod->normalmins[2] = z;
			if (mod->normalmaxs[0] < x) mod->normalmaxs[0] = x;
			if (mod->normalmaxs[1] < y) mod->normalmaxs[1] = y;
			if (mod->normalmaxs[2] < z) mod->normalmaxs[2] = z;
		}
		// calculate yawmins/yawmaxs, rotatedmins/maxs from normalmins/maxs
		// (fast but less accurate than doing it per vertex)
		x2a = mod->normalmins[0] * mod->normalmins[0];
		x2b = mod->normalmaxs[0] * mod->normalmaxs[0];
		y2a = mod->normalmins[1] * mod->normalmins[1];
		y2b = mod->normalmaxs[1] * mod->normalmaxs[1];
		z2a = mod->normalmins[2] * mod->normalmins[2];
		z2b = mod->normalmaxs[2] * mod->normalmaxs[2];
		x2 = max(x2a, x2b);
		y2 = max(y2a, y2b);
		z2 = max(z2a, z2b);
		yawradius = sqrt(x2 + y2);
		rotatedradius = sqrt(x2 + y2 + z2);
		VectorSet(mod->yawmins, -yawradius, -yawradius, mod->normalmins[2]);
		VectorSet(mod->yawmaxs, yawradius, yawradius, mod->normalmaxs[2]);
		VectorSet(mod->rotatedmins, -rotatedradius, -rotatedradius, -rotatedradius);
		VectorSet(mod->rotatedmaxs, rotatedradius, rotatedradius, rotatedradius);
		mod->radius = rotatedradius;
		mod->radius2 = x2 + y2 + z2;
	}
	else
	{
		VectorClear(mod->normalmins);
		VectorClear(mod->normalmaxs);
		VectorClear(mod->yawmins);
		VectorClear(mod->yawmaxs);
		VectorClear(mod->rotatedmins);
		VectorClear(mod->rotatedmaxs);
		mod->radius = 0;
		mod->radius2 = 0;
	}
}

void Mod_Mesh_Validate(model_t *mod)
{
	int i;
	qbool warned = false;
	for (i = 0; i < mod->num_surfaces; i++)
	{
		msurface_t *surf = mod->data_surfaces + i;
		int *e = mod->surfmesh.data_element3i + surf->num_firsttriangle * 3;
		int first = surf->num_firstvertex;
		int end = surf->num_firstvertex + surf->num_vertices;
		int j;
		for (j = 0;j < surf->num_triangles * 3;j++)
		{
			if (e[j] < first || e[j] >= end)
			{
				if (!warned)
					Con_DPrintf ("Mod_Mesh_Validate: detected corrupt surface - debug me!\n");
				warned = true;
				e[j] = first;
			}
		}
	}
}

static void Mod_Mesh_UploadDynamicBuffers(model_t *mod)
{
	mod->surfmesh.data_element3s_indexbuffer = mod->surfmesh.data_element3s ? R_BufferData_Store(mod->surfmesh.num_triangles * sizeof(short[3]), mod->surfmesh.data_element3s, R_BUFFERDATA_INDEX16, &mod->surfmesh.data_element3s_bufferoffset) : NULL;
	mod->surfmesh.data_element3i_indexbuffer = mod->surfmesh.data_element3i ? R_BufferData_Store(mod->surfmesh.num_triangles * sizeof(int[3]), mod->surfmesh.data_element3i, R_BUFFERDATA_INDEX32, &mod->surfmesh.data_element3i_bufferoffset) : NULL;
	mod->surfmesh.data_vertex3f_vertexbuffer = mod->surfmesh.data_vertex3f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[3]), mod->surfmesh.data_vertex3f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_vertex3f_bufferoffset) : NULL;
	mod->surfmesh.data_svector3f_vertexbuffer = mod->surfmesh.data_svector3f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[3]), mod->surfmesh.data_svector3f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_svector3f_bufferoffset) : NULL;
	mod->surfmesh.data_tvector3f_vertexbuffer = mod->surfmesh.data_tvector3f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[3]), mod->surfmesh.data_tvector3f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_tvector3f_bufferoffset) : NULL;
	mod->surfmesh.data_normal3f_vertexbuffer = mod->surfmesh.data_normal3f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[3]), mod->surfmesh.data_normal3f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_normal3f_bufferoffset) : NULL;
	mod->surfmesh.data_texcoordtexture2f_vertexbuffer = mod->surfmesh.data_texcoordtexture2f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[2]), mod->surfmesh.data_texcoordtexture2f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_texcoordtexture2f_bufferoffset) : NULL;
	mod->surfmesh.data_texcoordlightmap2f_vertexbuffer = mod->surfmesh.data_texcoordlightmap2f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[2]), mod->surfmesh.data_texcoordlightmap2f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_texcoordlightmap2f_bufferoffset) : NULL;
	mod->surfmesh.data_lightmapcolor4f_vertexbuffer = mod->surfmesh.data_lightmapcolor4f ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(float[4]), mod->surfmesh.data_lightmapcolor4f, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_lightmapcolor4f_bufferoffset) : NULL;
	mod->surfmesh.data_skeletalindex4ub_vertexbuffer = mod->surfmesh.data_skeletalindex4ub ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(unsigned char[4]), mod->surfmesh.data_skeletalindex4ub, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_skeletalindex4ub_bufferoffset) : NULL;
	mod->surfmesh.data_skeletalweight4ub_vertexbuffer = mod->surfmesh.data_skeletalweight4ub ? R_BufferData_Store(mod->surfmesh.num_vertices * sizeof(unsigned char[4]), mod->surfmesh.data_skeletalweight4ub, R_BUFFERDATA_VERTEX, &mod->surfmesh.data_skeletalweight4ub_bufferoffset) : NULL;
}

void Mod_Mesh_Finalize(model_t *mod)
{
	if (gl_paranoid.integer)
		Mod_Mesh_Validate(mod);
	Mod_Mesh_ComputeBounds(mod);
	Mod_Mesh_MakeSortedSurfaces(mod);
	if (!r_refdef.draw2dstage)
		Mod_BuildTextureVectorsFromNormals(0, mod->surfmesh.num_vertices, mod->surfmesh.num_triangles, mod->surfmesh.data_vertex3f, mod->surfmesh.data_texcoordtexture2f, mod->surfmesh.data_normal3f, mod->surfmesh.data_element3i, mod->surfmesh.data_svector3f, mod->surfmesh.data_tvector3f, true);
	Mod_Mesh_UploadDynamicBuffers(mod);
}

WARP_X_ (PRVM_Globals_Query)
void Mod_Shaders_Query (feed_fn_t myfeed_shall_stop)
{
	if (!q3shaders_mem)
		return;

	int count = 0;

	for (unsigned short hash = 0; hash < Q3SHADER_HASH_SIZE_1021; hash ++) {
		q3shader_hash_entry_t *entry = q3shader_data->hash + (hash % Q3SHADER_HASH_SIZE_1021);
		for (/*nada*/ ; entry; entry = entry->chain) {
			qbool shall_stop = myfeed_shall_stop (-1, entry->shader.name, "", NULL, NULL, NULL, 0, 1, 2);
			if (shall_stop)
				return;
		} // for entry
	} // for hash
}


