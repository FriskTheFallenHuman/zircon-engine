// model_brush_vbsp.c.h


static void Mod_VBSP_LoadEntities(sizebuf_t *sb)
{
	loadmodel->brush.entities = NULL;
	if (!sb->cursize)
		return;
	loadmodel->brush.entities = (char *)Mem_Alloc(loadmodel->mempool, sb->cursize + 1);
	MSG_ReadBytes(sb, sb->cursize, (unsigned char *)loadmodel->brush.entities);
	loadmodel->brush.entities[sb->cursize] = 0;
}

static void Mod_VBSP_LoadVertexes(sizebuf_t *sb)
{
	mvertex_t	*out;
	int			i, count;
	int			structsize = 12;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadVertexes: funny lump size in %s", loadmodel->model_name);
	count = sb->cursize / structsize;
	out = (mvertex_t *)Mem_Alloc(loadmodel->mempool, count*sizeof(*out));

	loadmodel->brushq1.vertexes = out;
	loadmodel->brushq1.numvertexes = count;

	for ( i=0 ; i<count ; i++, out++)
	{
		out->position[0] = MSG_ReadLittleFloat(sb);
		out->position[1] = MSG_ReadLittleFloat(sb);
		out->position[2] = MSG_ReadLittleFloat(sb);
	}
}

static void Mod_VBSP_LoadEdges(sizebuf_t *sb)
{
	medge_t *out;
	int 	i, count;
	int		structsize = 4;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadEdges: funny lump size in %s", loadmodel->model_name);
	count = sb->cursize / structsize;
	out = (medge_t *)Mem_Alloc(loadmodel->mempool, count * sizeof(*out));

	loadmodel->brushq1.edges = out;
	loadmodel->brushq1.numedges = count;

	for ( i=0 ; i<count ; i++, out++)
	{
		out->v[0] = (unsigned short)MSG_ReadLittleShort(sb);
		out->v[1] = (unsigned short)MSG_ReadLittleShort(sb);
		
		if ((int)out->v[0] >= loadmodel->brushq1.numvertexes || (int)out->v[1] >= loadmodel->brushq1.numvertexes)
		{
			Con_PrintLinef ("Mod_VBSP_LoadEdges: %s has invalid vertex indices in edge %d (vertices %d %d >= numvertices %d)", loadmodel->model_name, i, out->v[0], out->v[1], loadmodel->brushq1.numvertexes);
			if (!loadmodel->brushq1.numvertexes)
				Host_Error_Line ("Mod_VBSP_LoadEdges: %s has edges but no vertexes, cannot fix", loadmodel->model_name);
				
			out->v[0] = 0;
			out->v[1] = 0;
		}
	}
}

static void Mod_VBSP_LoadSurfedges(sizebuf_t *sb)
{
	int		i;
	int structsize = 4;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadSurfedges: funny lump size in %s",loadmodel->model_name);
	loadmodel->brushq1.numsurfedges = sb->cursize / structsize;
	loadmodel->brushq1.surfedges = (int *)Mem_Alloc(loadmodel->mempool, loadmodel->brushq1.numsurfedges * sizeof(int));

	for (i = 0;i < loadmodel->brushq1.numsurfedges;i++)
		loadmodel->brushq1.surfedges[i] = MSG_ReadLittleLong(sb);
}

static void Mod_VBSP_LoadTextures(sizebuf_t *sb)
{
	Con_PrintLinef (CON_WARN "Mod_VBSP_LoadTextures: Don't know how to do this yet");
}

static void Mod_VBSP_LoadPlanes(sizebuf_t *sb)
{
	int			i;
	mplane_t	*out;
	int structsize = 20;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadPlanes: funny lump size in %s", loadmodel->model_name);
	loadmodel->brush.num_planes = sb->cursize / structsize;
	loadmodel->brush.data_planes = out = (mplane_t *)Mem_Alloc(loadmodel->mempool, loadmodel->brush.num_planes * sizeof(*out));

	for (i = 0;i < loadmodel->brush.num_planes;i++, out++)
	{
		out->normal[0] = MSG_ReadLittleFloat(sb);
		out->normal[1] = MSG_ReadLittleFloat(sb);
		out->normal[2] = MSG_ReadLittleFloat(sb);
		out->dist = MSG_ReadLittleFloat(sb);
		MSG_ReadLittleLong(sb); // type is not used, we use PlaneClassify
		PlaneClassify(out);
	}
}

static void Mod_VBSP_LoadTexinfo(sizebuf_t *sb)
{
	mtexinfo_t *out;
	int i, j, k, count, miptex;
	int structsize = 72;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadTexinfo: funny lump size in %s", loadmodel->model_name);
	count = sb->cursize / structsize;
	out = (mtexinfo_t *)Mem_Alloc(loadmodel->mempool, count * sizeof(*out));

	loadmodel->brushq1.texinfo = out;
	loadmodel->brushq1.numtexinfo = count;

	for (i = 0;i < count;i++, out++)
	{
		for (k = 0;k < 2;k++)
			for (j = 0;j < 4;j++)
				out->vecs[k][j] = MSG_ReadLittleFloat(sb);

		for (k = 0;k < 2;k++)
			for (j = 0;j < 4;j++)
				MSG_ReadLittleFloat(sb); // TODO lightmapVecs

		out->q1flags = MSG_ReadLittleLong(sb);
		miptex = MSG_ReadLittleLong(sb);

		if (out->q1flags & TEX_SPECIAL)
		{
			// if texture chosen is NULL or the shader needs a lightmap,
			// force to notexture water shader
			out->textureindex = loadmodel->num_textures - 1;
		}
		else
		{
			// if texture chosen is NULL, force to notexture
			out->textureindex = loadmodel->num_textures - 2;
		}
		// see if the specified miptex is valid and try to use it instead
		if (loadmodel->data_textures)
		{
			if ((unsigned int) miptex >= (unsigned int) loadmodel->num_textures)
				Con_PrintLinef ("error in model " QUOTED_S ": invalid miptex index %d(of %d)", loadmodel->model_name, miptex, loadmodel->num_textures);
			else
				out->textureindex = miptex;
		}
	}
}

static void Mod_VBSP_LoadFaces(sizebuf_t *sb)
{
	msurface_t *surface;
	int i, j, count, surfacenum, planenum, smax, tmax, ssize, tsize, firstedge, numedges, totalverts, totaltris, lightmapnumber, lightmapsize, totallightmapsamples, lightmapoffset, texinfoindex;
	float texmins[2], texmaxs[2], val;
	rtexture_t *lightmaptexture, *deluxemaptexture;
	char vabuf[1024];
	int structsize =  56;

	if (sb->cursize % structsize)
		Host_Error_Line ("Mod_VBSP_LoadFaces: funny lump size in %s",loadmodel->model_name);
	count = sb->cursize / structsize;
	loadmodel->data_surfaces = (msurface_t *)Mem_Alloc(loadmodel->mempool, count*sizeof(msurface_t));
	loadmodel->data_surfaces_lightmapinfo = (msurface_lightmapinfo_t *)Mem_Alloc(loadmodel->mempool, count*sizeof(msurface_lightmapinfo_t));

	loadmodel->num_surfaces = count;

	loadmodel->brushq1.firstrender = true;
	loadmodel->brushq1.lightmapupdateflags = (unsigned char *)Mem_Alloc(loadmodel->mempool, count*sizeof(unsigned char));

	totalverts = 0;
	totaltris = 0;
	for (surfacenum = 0;surfacenum < count;surfacenum++)
	{
		numedges = BuffLittleShort(sb->data + structsize * surfacenum + 8);
		totalverts += numedges;
		totaltris += numedges - 2;
	}

	Mod_AllocSurfMesh(loadmodel->mempool, totalverts, totaltris, true, false);

	lightmaptexture = NULL;
	deluxemaptexture = r_texture_blanknormalmap;
	lightmapnumber = 0;
	lightmapsize = bound(256, gl_max_lightmapsize.integer, (int)vid.maxtexturesize_2d);
	totallightmapsamples = 0;

	totalverts = 0;
	totaltris = 0;
	for (surfacenum = 0, surface = loadmodel->data_surfaces;surfacenum < count;surfacenum++, surface++)
	{
		surface->lightmapinfo = loadmodel->data_surfaces_lightmapinfo + surfacenum;
		planenum = (unsigned short)MSG_ReadLittleShort(sb);
		/*side = */MSG_ReadLittleShort(sb); // TODO support onNode?
		firstedge = MSG_ReadLittleLong(sb);
		numedges = (unsigned short)MSG_ReadLittleShort(sb);
		texinfoindex = (unsigned short)MSG_ReadLittleShort(sb);
		MSG_ReadLittleLong(sb); // skipping over dispinfo and surfaceFogVolumeID, both short
		for (i = 0;i < MAXLIGHTMAPS;i++)
			surface->lightmapinfo->styles[i] = MSG_ReadByte(sb);
		lightmapoffset = MSG_ReadLittleLong(sb);

		// FIXME: validate edges, texinfo, etc?
		if ((unsigned int) firstedge > (unsigned int) loadmodel->brushq1.numsurfedges || (unsigned int) numedges > (unsigned int) loadmodel->brushq1.numsurfedges || (unsigned int) firstedge + (unsigned int) numedges > (unsigned int) loadmodel->brushq1.numsurfedges)
			Host_Error_Line ("Mod_VBSP_LoadFaces: invalid edge range (firstedge %d, numedges %d, model edges %d)", firstedge, numedges, loadmodel->brushq1.numsurfedges);
		if ((unsigned int) texinfoindex >= (unsigned int) loadmodel->brushq1.numtexinfo)
			Host_Error_Line ("Mod_VBSP_LoadFaces: invalid texinfo index %d(model has %d texinfos)", texinfoindex, loadmodel->brushq1.numtexinfo);
		if ((unsigned int) planenum >= (unsigned int) loadmodel->brush.num_planes)
			Host_Error_Line ("Mod_VBSP_LoadFaces: invalid plane index %d (model has %d planes)", planenum, loadmodel->brush.num_planes);

		surface->lightmapinfo->texinfo = loadmodel->brushq1.texinfo + texinfoindex;
		surface->texture = loadmodel->data_textures + surface->lightmapinfo->texinfo->textureindex;

		// Q2BSP doesn't use lightmaps on sky or warped surfaces (water), but still has a lightofs of 0
		if (lightmapoffset == 0 && (surface->texture->q2flags & (Q2SURF_SKY | Q2SURF_WARP)))
			lightmapoffset = -1;

		//surface->flags = surface->texture->flags;
		//if (LittleShort(in->side))
		//	surface->flags |= SURF_PLANEBACK;
		//surface->plane = loadmodel->brush.data_planes + planenum;

		surface->num_firstvertex = totalverts;
		surface->num_vertices = numedges;
		surface->num_firsttriangle = totaltris;
		surface->num_triangles = numedges - 2;
		totalverts += numedges;
		totaltris += numedges - 2;

		// convert edges back to a normal polygon
		for (i = 0;i < surface->num_vertices;i++)
		{
			int lindex = loadmodel->brushq1.surfedges[firstedge + i];
			float s, t;
			// note: the q1bsp format does not allow a 0 surfedge (it would have no negative counterpart)
			if (lindex >= 0)
				VectorCopy(loadmodel->brushq1.vertexes[loadmodel->brushq1.edges[lindex].v[0]].position, (loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3);
			else
				VectorCopy(loadmodel->brushq1.vertexes[loadmodel->brushq1.edges[-lindex].v[1]].position, (loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3);
			s = DotProduct(((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3), surface->lightmapinfo->texinfo->vecs[0]) + surface->lightmapinfo->texinfo->vecs[0][3];
			t = DotProduct(((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3), surface->lightmapinfo->texinfo->vecs[1]) + surface->lightmapinfo->texinfo->vecs[1][3];
			(loadmodel->surfmesh.data_texcoordtexture2f + 2 * surface->num_firstvertex)[i * 2 + 0] = s / surface->texture->width;
			(loadmodel->surfmesh.data_texcoordtexture2f + 2 * surface->num_firstvertex)[i * 2 + 1] = t / surface->texture->height;
			(loadmodel->surfmesh.data_texcoordlightmap2f + 2 * surface->num_firstvertex)[i * 2 + 0] = 0;
			(loadmodel->surfmesh.data_texcoordlightmap2f + 2 * surface->num_firstvertex)[i * 2 + 1] = 0;
			(loadmodel->surfmesh.data_lightmapoffsets + surface->num_firstvertex)[i] = 0;
		}

		for (i = 0;i < surface->num_triangles;i++)
		{
			(loadmodel->surfmesh.data_element3i + 3 * surface->num_firsttriangle)[i * 3 + 0] = 0 + surface->num_firstvertex;
			(loadmodel->surfmesh.data_element3i + 3 * surface->num_firsttriangle)[i * 3 + 1] = i + 1 + surface->num_firstvertex;
			(loadmodel->surfmesh.data_element3i + 3 * surface->num_firsttriangle)[i * 3 + 2] = i + 2 + surface->num_firstvertex;
		}

		// compile additional data about the surface geometry
		Mod_BuildNormals(surface->num_firstvertex, surface->num_vertices, surface->num_triangles, loadmodel->surfmesh.data_vertex3f, (loadmodel->surfmesh.data_element3i + 3 * surface->num_firsttriangle), loadmodel->surfmesh.data_normal3f, r_smoothnormals_areaweighting.integer != 0);
		Mod_BuildTextureVectorsFromNormals(surface->num_firstvertex, surface->num_vertices, surface->num_triangles, loadmodel->surfmesh.data_vertex3f, loadmodel->surfmesh.data_texcoordtexture2f, loadmodel->surfmesh.data_normal3f, (loadmodel->surfmesh.data_element3i + 3 * surface->num_firsttriangle), loadmodel->surfmesh.data_svector3f, loadmodel->surfmesh.data_tvector3f, r_smoothnormals_areaweighting.integer != 0);
		BoxFromPoints(surface->mins, surface->maxs, surface->num_vertices, (loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex));

		// generate surface extents information
		texmins[0] = texmaxs[0] = DotProduct((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex), surface->lightmapinfo->texinfo->vecs[0]) + surface->lightmapinfo->texinfo->vecs[0][3];
		texmins[1] = texmaxs[1] = DotProduct((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex), surface->lightmapinfo->texinfo->vecs[1]) + surface->lightmapinfo->texinfo->vecs[1][3];
		for (i = 1;i < surface->num_vertices;i++)
		{
			for (j = 0;j < 2;j++)
			{
				val = DotProduct((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3, surface->lightmapinfo->texinfo->vecs[j]) + surface->lightmapinfo->texinfo->vecs[j][3];
				texmins[j] = min(texmins[j], val);
				texmaxs[j] = max(texmaxs[j], val);
			}
		}
		for (i = 0;i < 2;i++)
		{
			surface->lightmapinfo->texturemins[i] = (int) floor(texmins[i] / 16.0) * 16;
			surface->lightmapinfo->extents[i] = (int) ceil(texmaxs[i] / 16.0) * 16 - surface->lightmapinfo->texturemins[i];
		}

		smax = surface->lightmapinfo->extents[0] >> 4;
		tmax = surface->lightmapinfo->extents[1] >> 4;
		ssize = (surface->lightmapinfo->extents[0] >> 4) + 1;
		tsize = (surface->lightmapinfo->extents[1] >> 4) + 1;

		// lighting info
		surface->lightmaptexture = NULL;
		surface->deluxemaptexture = r_texture_blanknormalmap;
		if (lightmapoffset == -1)
		{
			surface->lightmapinfo->samples = NULL;
#if 1
			// give non-lightmapped water a 1x white lightmap
			if (!loadmodel->brush.isq2bsp && surface->texture->name[0] == '*' && (surface->lightmapinfo->texinfo->q1flags & TEX_SPECIAL) && ssize <= 256 && tsize <= 256)
			{
				surface->lightmapinfo->samples = (unsigned char *)Mem_Alloc(loadmodel->mempool, ssize * tsize * 3);
				surface->lightmapinfo->styles[0] = 0;
				memset(surface->lightmapinfo->samples, 128, ssize * tsize * 3);
			}
#endif
		}
		else
			surface->lightmapinfo->samples = loadmodel->brushq1.lightdata + lightmapoffset;

		// check if we should apply a lightmap to this
		if (!(surface->lightmapinfo->texinfo->q1flags & TEX_SPECIAL) || surface->lightmapinfo->samples)
		{
			if (ssize > 256 || tsize > 256)
				Host_Error_Line ("Bad surface extents");

			if (lightmapsize < ssize)
				lightmapsize = ssize;
			if (lightmapsize < tsize)
				lightmapsize = tsize;

			totallightmapsamples += ssize*tsize;

			// force lightmap upload on first time seeing the surface
			//
			// additionally this is used by the later code to see if a
			// lightmap is needed on this surface (rather than duplicating the
			// logic above)
			loadmodel->brushq1.lightmapupdateflags[surfacenum] = true;
			loadmodel->lit = true;
		}
	}

	// small maps (such as ammo boxes especially) don't need big lightmap
	// textures, so this code tries to guess a good size based on
	// totallightmapsamples (size of the lightmaps lump basically), as well as
	// trying to max out the size if there is a lot of lightmap data to store
	// additionally, never choose a lightmapsize that is smaller than the
	// largest surface encountered (as it would fail)
	i = lightmapsize;
	for (lightmapsize = 64; (lightmapsize < i) && (lightmapsize < bound(128, gl_max_lightmapsize.integer, (int)vid.maxtexturesize_2d)) && (totallightmapsamples > lightmapsize*lightmapsize); lightmapsize*=2)
		;

	// now that we've decided the lightmap texture size, we can do the rest
	if (cls.state != ca_dedicated)
	{
		int stainmapsize = 0;
		mod_alloclightmap_state_t allocState;

		Mod_AllocLightmap_Init(&allocState, loadmodel->mempool, lightmapsize, lightmapsize);
		for (surfacenum = 0, surface = loadmodel->data_surfaces;surfacenum < count;surfacenum++, surface++)
		{
			int iu, iv, lightmapx = 0, lightmapy = 0;
			float u, v, ubase, vbase, uscale, vscale;

			if (!loadmodel->brushq1.lightmapupdateflags[surfacenum])
				continue;

			smax = surface->lightmapinfo->extents[0] >> 4;
			tmax = surface->lightmapinfo->extents[1] >> 4;
			ssize = (surface->lightmapinfo->extents[0] >> 4) + 1;
			tsize = (surface->lightmapinfo->extents[1] >> 4) + 1;
			stainmapsize += ssize * tsize * 3;

			if (!lightmaptexture || !Mod_AllocLightmap_Block(&allocState, ssize, tsize, &lightmapx, &lightmapy))
			{
				// allocate a texture pool if we need it
				if (loadmodel->texturepool == NULL)
					loadmodel->texturepool = R_AllocTexturePool();
				// could not find room, make a new lightmap
				loadmodel->brushq3.num_mergedlightmaps = lightmapnumber + 1;
				loadmodel->brushq3.data_lightmaps = (rtexture_t **)Mem_Realloc(loadmodel->mempool, loadmodel->brushq3.data_lightmaps, loadmodel->brushq3.num_mergedlightmaps * sizeof(loadmodel->brushq3.data_lightmaps[0]));
				loadmodel->brushq3.data_deluxemaps = (rtexture_t **)Mem_Realloc(loadmodel->mempool, loadmodel->brushq3.data_deluxemaps, loadmodel->brushq3.num_mergedlightmaps * sizeof(loadmodel->brushq3.data_deluxemaps[0]));
				loadmodel->brushq3.data_lightmaps[lightmapnumber] = lightmaptexture = R_LoadTexture2D(loadmodel->texturepool, va(vabuf, sizeof(vabuf), "lightmap%d", lightmapnumber), lightmapsize, lightmapsize, NULL, TEXTYPE_BGRA, TEXF_FORCELINEAR | TEXF_ALLOWUPDATES, q_tx_miplevel_neg1, q_tx_palette_NULL);
				if (loadmodel->brushq1.nmaplightdata)
					loadmodel->brushq3.data_deluxemaps[lightmapnumber] = deluxemaptexture = R_LoadTexture2D(loadmodel->texturepool, va(vabuf, sizeof(vabuf), "deluxemap%d", lightmapnumber), lightmapsize, lightmapsize, NULL, TEXTYPE_BGRA, TEXF_FORCELINEAR | TEXF_ALLOWUPDATES, q_tx_miplevel_neg1, q_tx_palette_NULL);
				lightmapnumber++;
				Mod_AllocLightmap_Reset(&allocState);
				Mod_AllocLightmap_Block(&allocState, ssize, tsize, &lightmapx, &lightmapy);
			}
			surface->lightmaptexture = lightmaptexture;
			surface->deluxemaptexture = deluxemaptexture;
			surface->lightmapinfo->lightmaporigin[0] = lightmapx;
			surface->lightmapinfo->lightmaporigin[1] = lightmapy;

			uscale = 1.0f / (float)lightmapsize;
			vscale = 1.0f / (float)lightmapsize;
			ubase = lightmapx * uscale;
			vbase = lightmapy * vscale;

			for (i = 0;i < surface->num_vertices;i++)
			{
				u = ((DotProduct(((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3), surface->lightmapinfo->texinfo->vecs[0]) + surface->lightmapinfo->texinfo->vecs[0][3]) + 8 - surface->lightmapinfo->texturemins[0]) * (1.0 / 16.0);
				v = ((DotProduct(((loadmodel->surfmesh.data_vertex3f + 3 * surface->num_firstvertex) + i * 3), surface->lightmapinfo->texinfo->vecs[1]) + surface->lightmapinfo->texinfo->vecs[1][3]) + 8 - surface->lightmapinfo->texturemins[1]) * (1.0 / 16.0);
				(loadmodel->surfmesh.data_texcoordlightmap2f + 2 * surface->num_firstvertex)[i * 2 + 0] = u * uscale + ubase;
				(loadmodel->surfmesh.data_texcoordlightmap2f + 2 * surface->num_firstvertex)[i * 2 + 1] = v * vscale + vbase;
				// LadyHavoc: calc lightmap data offset for vertex lighting to use
				iu = (int) u;
				iv = (int) v;
				(loadmodel->surfmesh.data_lightmapoffsets + surface->num_firstvertex)[i] = (bound(0, iv, tmax) * ssize + bound(0, iu, smax)) * 3;
			}
		}

		if (cl_stainmaps.integer)
		{
			// allocate stainmaps for permanent marks on walls and clear white
			unsigned char *stainsamples = NULL;
			stainsamples = (unsigned char *)Mem_Alloc(loadmodel->mempool, stainmapsize);
			memset(stainsamples, 255, stainmapsize);
			// assign pointers
			for (surfacenum = 0, surface = loadmodel->data_surfaces;surfacenum < count;surfacenum++, surface++)
			{
				if (!loadmodel->brushq1.lightmapupdateflags[surfacenum])
					continue;
				ssize = (surface->lightmapinfo->extents[0] >> 4) + 1;
				tsize = (surface->lightmapinfo->extents[1] >> 4) + 1;
				surface->lightmapinfo->stainsamples = stainsamples;
				stainsamples += ssize * tsize * 3;
			}
		}
	}

	// generate ushort elements array if possible
	if (loadmodel->surfmesh.data_element3s)
		for (i = 0;i < loadmodel->surfmesh.num_triangles*3;i++)
			loadmodel->surfmesh.data_element3s[i] = loadmodel->surfmesh.data_element3i[i];
}

// Valve BSP loader
// Cloudwalk: Wasn't sober when I wrote this. I screamed and ran away at the face loader
int Mod_VBSP_Load(model_t *mod, void *buffer, void *bufferend)
{
	static cvar_t *testing = NULL; // TEMPORARY
	int i;
	sizebuf_t sb;
	sizebuf_t lumpsb[HL2HEADER_LUMPS];

	if (!testing || !testing->integer)
	{
		if (!testing)
			testing = Cvar_Get(&cvars_all, "mod_bsp_vbsptest", "0", CF_CLIENT | CF_SERVER, "uhhh");
		Host_Error_Line ("Mod_VBSP_Load: not yet fully implemented. Change the now-generated " QUOTED_STR ("mod_bsp_vbsptest") " to 1 if you wish to test this");
	}
	else
	{

		MSG_InitReadBuffer(&sb, (unsigned char *)buffer, (unsigned char *)bufferend - (unsigned char *)buffer);

		mod->type = mod_brushhl2;

		MSG_ReadLittleLong(&sb);
		MSG_ReadLittleLong(&sb); // TODO version check

		mod->modeldatatypestring = "VBSP";

		// read lumps
		for (i = 0; i < HL2HEADER_LUMPS; i++)
		{
			int offset = MSG_ReadLittleLong(&sb);
			int size = MSG_ReadLittleLong(&sb);
			MSG_ReadLittleLong(&sb); // TODO support version
			MSG_ReadLittleLong(&sb); // TODO support ident
			if (offset < 0 || offset + size > sb.cursize)
				Host_Error_Line ("Mod_VBSP_Load: %s has invalid lump %d (offset %d, size %d, file size %d)", mod->model_name, i, offset, size, (int)sb.cursize);
			MSG_InitReadBuffer(&lumpsb[i], sb.data + offset, size);
		}

		MSG_ReadLittleLong(&sb); // TODO support revision field

		mod->soundfromcenter = true;
		mod->TraceBox = Mod_CollisionBIH_TraceBox;
		mod->TraceBrush = Mod_CollisionBIH_TraceBrush;
		mod->TraceLine = Mod_CollisionBIH_TraceLine;
		mod->TracePoint = Mod_CollisionBIH_TracePoint;
		mod->PointSuperContents = Mod_CollisionBIH_PointSuperContents;
		mod->TraceLineAgainstSurfaces = Mod_CollisionBIH_TraceLine;
		mod->brush.TraceLineOfSight = Mod_Q3BSP_TraceLineOfSight; // probably not correct
		mod->brush.SuperContentsFromNativeContents = Mod_Q3BSP_SuperContentsFromNativeContents; // probably not correct
		mod->brush.NativeContentsFromSuperContents = Mod_Q3BSP_NativeContentsFromSuperContents; // probably not correct
		mod->brush.GetPVS = Mod_BSP_GetPVS;
		mod->brush.FatPVS = Mod_BSP_FatPVS;
		mod->brush.BoxTouchingPVS = Mod_BSP_BoxTouchingPVS;
		mod->brush.BoxTouchingLeafPVS = Mod_BSP_BoxTouchingLeafPVS;
		mod->brush.BoxTouchingVisibleLeafs = Mod_BSP_BoxTouchingVisibleLeafs;
		mod->brush.FindBoxClusters = Mod_BSP_FindBoxClusters;
		mod->brush.LightPoint = Mod_Q3BSP_LightPoint; // probably not correct
		mod->brush.FindNonSolidLocation = Mod_BSP_FindNonSolidLocation;
		mod->brush.AmbientSoundLevelsForPoint = NULL;
		mod->brush.RoundUpToHullSize = NULL;
		mod->brush.PointInLeaf = Mod_BSP_PointInLeaf;
		mod->Draw = R_Mod_Draw;
		mod->DrawDepth = R_Mod_DrawDepth;
		mod->DrawDebug = R_Mod_DrawDebug;
		mod->DrawPrepass = R_Mod_DrawPrepass;
		mod->GetLightInfo = R_Mod_GetLightInfo;
		mod->CompileShadowMap = R_Mod_CompileShadowMap;
		mod->DrawShadowMap = R_Mod_DrawShadowMap;
		mod->DrawLight = R_Mod_DrawLight;

		// allocate a texture pool if we need it
		if (mod->texturepool == NULL)
			mod->texturepool = R_AllocTexturePool();

		Mod_VBSP_LoadEntities(&lumpsb[HL2LUMP_ENTITIES]);
		Mod_VBSP_LoadVertexes(&lumpsb[HL2LUMP_VERTEXES]);
		Mod_VBSP_LoadEdges(&lumpsb[HL2LUMP_EDGES]);
		Mod_VBSP_LoadSurfedges(&lumpsb[HL2LUMP_SURFEDGES]);
		Mod_VBSP_LoadTextures(&lumpsb[HL2LUMP_TEXDATA/*?*/]);
		//Mod_VBSP_LoadLighting(&lumpsb[HL2LUMP_LIGHTING]);
		Mod_VBSP_LoadPlanes(&lumpsb[HL2LUMP_PLANES]);
		Mod_VBSP_LoadTexinfo(&lumpsb[HL2LUMP_TEXINFO]);

		// AHHHHHHH
		Mod_VBSP_LoadFaces(&lumpsb[HL2LUMP_FACES]);
	}
	return false;
}
