// model_shared_q3shader_loop.c.h

// Baker this while loop is so big, separating ...

if (String_Match_Caseless(com_token, "}"))
	break;
if (String_Match_Caseless(com_token, "{")) {
	static q3shaderinfo_layer_t dummy;
	if (shader.numlayers < Q3SHADER_MAXLAYERS_8) {
		layer = shader.layers + shader.numlayers++;
	} else {
		// parse and process it anyway, just don't store it (so a map $lightmap or such stuff still is found)
		memset(&dummy, 0, sizeof(dummy));
		layer = &dummy;
	}
	layer->rgbgen.rgbgen = Q3RGBGEN_IDENTITY;
	layer->alphagen.alphagen = Q3ALPHAGEN_IDENTITY;
	layer->tcgen.tcgen = Q3TCGEN_TEXTURE;
	layer->blendfunc[0] = GL_ONE;
	layer->blendfunc[1] = GL_ZERO;
	while (COM_ParseToken_QuakeC(&text, false)) {
		if (String_Match_Caseless(com_token, "}"))
			break;
		if (String_Match_Caseless(com_token, NEWLINE))
			continue;
		numparameters = 0;
		for (j = 0; String_NOT_Match(com_token, NEWLINE) && String_NOT_Match(com_token, "}");j++) {
			if (j < TEXTURE_MAXFRAMES_64 + 4) {
				// remap dp_water to dpwater, dp_reflect to dpreflect, etc.
				if (j == 0 && String_Starts_With_Caseless(com_token, "dp_")) dpsnprintf(parameter[j], sizeof(parameter[j]), "dp%s", &com_token[3]);
				else strlcpy(parameter[j], com_token, sizeof(parameter[j]));

				numparameters = j + 1;
			}
			if (!COM_ParseToken_QuakeC(&text, true))
				break;
		} // j
		//for (j = numparameters;j < TEXTURE_MAXFRAMES_64 + 4;j++)
		//	parameter[j][0] = 0;
		if (developer_insane.integer) {
			Con_DPrintf ("%s %d: ", shader.name, shader.numlayers - 1);
			for (j = 0;j < numparameters;j++)
				Con_DPrintf (" %s", parameter[j]);
			Con_DPrint("\n");
		}
		if (numparameters >= 2 && String_Match_Caseless(parameter[0], "blendfunc")) {
			if (numparameters == 2) {
				if (String_Match_Caseless(parameter[1], "add")) {
					layer->blendfunc[0] = GL_ONE;
					layer->blendfunc[1] = GL_ONE;
				} else if (String_Match_Caseless(parameter[1], "addalpha")) {
					layer->blendfunc[0] = GL_SRC_ALPHA;
					layer->blendfunc[1] = GL_ONE;
				} else if (String_Match_Caseless(parameter[1], "filter")) {
					layer->blendfunc[0] = GL_DST_COLOR;
					layer->blendfunc[1] = GL_ZERO;
				} else if (String_Match_Caseless(parameter[1], "blend")) {
					layer->blendfunc[0] = GL_SRC_ALPHA;
					layer->blendfunc[1] = GL_ONE_MINUS_SRC_ALPHA;
				}
			} else if (numparameters == 3) {
				int k;
				for (k = 0;k < 2;k++) {
					if (String_Match_Caseless(parameter[k+1], "GL_ONE")) layer->blendfunc[k] = GL_ONE;
					else if (String_Match_Caseless(parameter[k+1], "GL_ZERO")) layer->blendfunc[k] = GL_ZERO;
					else if (String_Match_Caseless(parameter[k+1], "GL_SRC_COLOR")) layer->blendfunc[k] = GL_SRC_COLOR;
					else if (String_Match_Caseless(parameter[k+1], "GL_SRC_ALPHA")) layer->blendfunc[k] = GL_SRC_ALPHA;
					else if (String_Match_Caseless(parameter[k+1], "GL_DST_COLOR")) layer->blendfunc[k] = GL_DST_COLOR;
					else if (String_Match_Caseless(parameter[k+1], "GL_DST_ALPHA")) layer->blendfunc[k] = GL_DST_ALPHA;
					else if (String_Match_Caseless(parameter[k+1], "GL_ONE_MINUS_SRC_COLOR")) layer->blendfunc[k] = GL_ONE_MINUS_SRC_COLOR;
					else if (String_Match_Caseless(parameter[k+1], "GL_ONE_MINUS_SRC_ALPHA")) layer->blendfunc[k] = GL_ONE_MINUS_SRC_ALPHA;
					else if (String_Match_Caseless(parameter[k+1], "GL_ONE_MINUS_DST_COLOR")) layer->blendfunc[k] = GL_ONE_MINUS_DST_COLOR;
					else if (String_Match_Caseless(parameter[k+1], "GL_ONE_MINUS_DST_ALPHA")) layer->blendfunc[k] = GL_ONE_MINUS_DST_ALPHA;
					else layer->blendfunc[k] = GL_ONE; // default in case of parsing error
				} // k
			} // if
		} // if
		if (numparameters >= 2 && String_Match_Caseless(parameter[0], "alphafunc"))
			layer->alphatest = true;
		if (numparameters >= 2 && String_Isin2_Caseless(parameter[0], "map", "clampmap")) {
				// Baker: nmap # gloss ... "map"
			if (String_Match_Caseless(parameter[0], "clampmap"))
				layer->clampmap = true;
			layer->sh_numframes = 1;
			layer->animframerate = 1;

			// Baker: expand q3shader_data->char_ptrs
			layer->sh_ptexturename = (char **)Mem_ExpandableArray_AllocRecord (&q3shader_data->char_ptrs);
			layer->sh_ptexturename[0] = Mem_strdup (q3shaders_mem, parameter[1]);
			if (String_Match_Caseless(parameter[1], "$lightmap"))
				shader.lighting = true;
#if 0
		} else if (numparameters >= 2 && String_Isin1_Caseless(parameter[0], "glossmap")) {
				// Baker: nmap # gloss ... "map"
			if (layer->sh_ptexturename[0] == NULL) {
				Con_DPrintLinef (CON_WARN "glossmap without map", search->filenames[fileindex], parameter[1]);
			} else {
				// Baker: Allocate a pointer, then do a strdup and assign that pointer.
				layer->sh_ptexturename_gloss = (char **)Mem_ExpandableArray_AllocRecord (&q3shader_data->char_ptrs);
				layer->sh_ptexturename_gloss[0] = Mem_strdup (q3shaders_mem, parameter[1]);
			}
#endif
		} else if (numparameters >= 3 && String_Isin2_Caseless(parameter[0], "animmap", "animclampmap")) {
			// animmap 18 textures/causticBr/FWATER01.png textures/causticBr/FWATER02.png textures/causticBr/FWATER03.png 
			layer->sh_numframes = min(numparameters - 2, TEXTURE_MAXFRAMES_64);
			layer->animframerate = atof(parameter[1]);
			layer->sh_ptexturename = (char **) Mem_Alloc (q3shaders_mem, sizeof (char *) * layer->sh_numframes);
			for (i = 0; i < layer->sh_numframes; i ++)
				layer->sh_ptexturename[i] = Mem_strdup (q3shaders_mem, parameter[i + 2]);
		} else if (numparameters >= 3 && String_Isin1_Caseless(parameter[0], "framemap")) {
			// Baker: For use with buttons
			// Uses entity frame, like with buttons.
			// buttonmap textures/causticBr/FWATER01.png textures/causticBr/FWATER02.png 
			// 3 params, right?  So subtract 1 for number of frames.
			layer->sh_numframes = min(numparameters - 1, TEXTURE_MAXFRAMES_64);
			if (layer->sh_numframes != 2) {
				Con_PrintLinef (CON_WARN "framemap supports only 2 textures at this time, %d frames specified", layer->sh_numframes);
				if (layer->sh_numframes >= 2)
					layer->sh_numframes = 2;
				else layer->sh_numframes = 0;
			}
			if (layer->sh_numframes) {
				layer->sh_ptexturename = (char **) Mem_Alloc (q3shaders_mem, sizeof (char *) * layer->sh_numframes);
				for (i = 0; i < layer->sh_numframes; i ++) {
					layer->sh_ptexturename[i] = Mem_strdup (q3shaders_mem, parameter[i + 2]);
				} // for
			}
		} else if (numparameters >= 2 && String_Match_Caseless(parameter[0], "rgbgen")) {
			for (i = 0; i < numparameters - 2 && i < Q3RGBGEN_MAXPARMS_3; i ++)
				layer->rgbgen.parms[i] = atof(parameter[i+2]);
			     if (String_Match_Caseless(parameter[1], "identity"))         layer->rgbgen.rgbgen = Q3RGBGEN_IDENTITY;
			else if (String_Match_Caseless(parameter[1], "const"))            layer->rgbgen.rgbgen = Q3RGBGEN_CONST;
			else if (String_Match_Caseless(parameter[1], "entity"))           layer->rgbgen.rgbgen = Q3RGBGEN_ENTITY;
			else if (String_Match_Caseless(parameter[1], "exactvertex"))      layer->rgbgen.rgbgen = Q3RGBGEN_VERTEX; // Baker: Q3RGBGEN_EXACTVERTEX;
			else if (String_Match_Caseless(parameter[1], "identitylighting")) layer->rgbgen.rgbgen = Q3RGBGEN_IDENTITYLIGHTING;
			else if (String_Match_Caseless(parameter[1], "lightingdiffuse"))  layer->rgbgen.rgbgen = Q3RGBGEN_LIGHTINGDIFFUSE;
			else if (String_Match_Caseless(parameter[1], "oneminusentity"))   layer->rgbgen.rgbgen = Q3RGBGEN_ONEMINUSENTITY;
			else if (String_Match_Caseless(parameter[1], "oneminusvertex"))   layer->rgbgen.rgbgen = Q3RGBGEN_ONEMINUSVERTEX;
			else if (String_Match_Caseless(parameter[1], "vertex"))           layer->rgbgen.rgbgen = Q3RGBGEN_VERTEX;
			else if (String_Match_Caseless(parameter[1], "wave")) {
				layer->rgbgen.rgbgen = Q3RGBGEN_WAVE;
				layer->rgbgen.wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[2]);
				for (i = 0;i < numparameters - 3 && i < Q3WAVEPARMS_4;i++)
					layer->rgbgen.waveparms[i] = atof(parameter[i+3]);
			}
			else Con_DPrintLinef ("%s parsing warning: unknown rgbgen %s", search->filenames[fileindex], parameter[1]);
		}
		else if (numparameters >= 2 && String_Match_Caseless(parameter[0], "alphagen")) {
			for (i = 0;i < numparameters - 2 && i < Q3ALPHAGEN_MAXPARMS_1; i++) {
				layer->alphagen.parms[i] = atof(parameter[i+2]);
			}
			     if (String_Match_Caseless(parameter[1], "identity"))         layer->alphagen.alphagen = Q3ALPHAGEN_IDENTITY;
			else if (String_Match_Caseless(parameter[1], "const"))            layer->alphagen.alphagen = Q3ALPHAGEN_CONST;
			else if (String_Match_Caseless(parameter[1], "entity"))           layer->alphagen.alphagen = Q3ALPHAGEN_ENTITY;
			else if (String_Match_Caseless(parameter[1], "lightingspecular")) layer->alphagen.alphagen = Q3ALPHAGEN_LIGHTINGSPECULAR;
			else if (String_Match_Caseless(parameter[1], "oneminusentity"))   layer->alphagen.alphagen = Q3ALPHAGEN_ONEMINUSENTITY;
			else if (String_Match_Caseless(parameter[1], "oneminusvertex"))   layer->alphagen.alphagen = Q3ALPHAGEN_ONEMINUSVERTEX;
			else if (String_Match_Caseless(parameter[1], "portal"))           layer->alphagen.alphagen = Q3ALPHAGEN_PORTAL;
			else if (String_Match_Caseless(parameter[1], "vertex"))           layer->alphagen.alphagen = Q3ALPHAGEN_VERTEX;
			else if (String_Match_Caseless(parameter[1], "wave")) {
				layer->alphagen.alphagen = Q3ALPHAGEN_WAVE;
				layer->alphagen.wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[2]);
				for (i = 0;i < numparameters - 3 && i < Q3WAVEPARMS_4;i++)
					layer->alphagen.waveparms[i] = atof(parameter[i+3]);
			}
			else Con_DPrintLinef ("%s parsing warning: unknown alphagen %s", search->filenames[fileindex], parameter[1]);
		}
		else if (numparameters >= 2 && (String_Match_Caseless(parameter[0], "texgen") || String_Match_Caseless(parameter[0], "tcgen"))) {
			// observed values: tcgen environment
			// no other values have been observed in real shaders
			for (i = 0;i < numparameters - 2 && i < Q3TCGEN_MAXPARMS_6;i++)
				layer->tcgen.parms[i] = atof(parameter[i+2]);
			     if (String_Match_Caseless(parameter[1], "base"))        layer->tcgen.tcgen = Q3TCGEN_TEXTURE;
			else if (String_Match_Caseless(parameter[1], "texture"))     layer->tcgen.tcgen = Q3TCGEN_TEXTURE;
			else if (String_Match_Caseless(parameter[1], "environment")) layer->tcgen.tcgen = Q3TCGEN_ENVIRONMENT;
			else if (String_Match_Caseless(parameter[1], "lightmap"))    layer->tcgen.tcgen = Q3TCGEN_LIGHTMAP;
			else if (String_Match_Caseless(parameter[1], "vector"))      layer->tcgen.tcgen = Q3TCGEN_VECTOR;
			else Con_DPrintLinef ("%s parsing warning: unknown tcgen mode %s", search->filenames[fileindex], parameter[1]);
		}
		else if (numparameters >= 2 && String_Match_Caseless(parameter[0], "tcmod")) {

			// observed values:
			// tcmod rotate #
			// tcmod scale # #
			// tcmod scroll # #
			// tcmod stretch sin # # # #
			// tcmod stretch triangle # # # #
			// tcmod transform # # # # # #
			// tcmod turb # # # #
			// tcmod turb sin # # # #  (this is bogus)
			// no other values have been observed in real shaders
			for (tcmodindex = 0;tcmodindex < Q3MAXTCMODS_8;tcmodindex++)
				if (!layer->tcmods[tcmodindex].tcmod)
					break;
			if (tcmodindex < Q3MAXTCMODS_8) {
				for (i = 0;i < numparameters - 2 && i < Q3TCMOD_MAXPARMS_6;i++)
					layer->tcmods[tcmodindex].parms[i] = atof(parameter[i+2]);

					 if (String_Match_Caseless(parameter[1], "entitytranslate")) layer->tcmods[tcmodindex].tcmod = Q3TCMOD_ENTITYTRANSLATE;
				else if (String_Match_Caseless(parameter[1], "rotate"))          layer->tcmods[tcmodindex].tcmod = Q3TCMOD_ROTATE;
				else if (String_Match_Caseless(parameter[1], "scale")) {
					layer->tcmods[tcmodindex].tcmod = Q3TCMOD_SCALE;
				}

				else if (String_Match_Caseless(parameter[1], "scroll"))          layer->tcmods[tcmodindex].tcmod = Q3TCMOD_SCROLL;
				else if (String_Match_Caseless(parameter[1], "page"))            layer->tcmods[tcmodindex].tcmod = Q3TCMOD_PAGE;
				else if (String_Match_Caseless(parameter[1], "stretch"))
				{
					layer->tcmods[tcmodindex].tcmod = Q3TCMOD_STRETCH;
					layer->tcmods[tcmodindex].wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[2]);
					for (i = 0;i < numparameters - 3 && i < Q3WAVEPARMS_4;i++)
						layer->tcmods[tcmodindex].waveparms[i] = atof(parameter[i+3]);
				}
				else if (String_Match_Caseless(parameter[1], "transform"))       layer->tcmods[tcmodindex].tcmod = Q3TCMOD_TRANSFORM;
				else if (String_Match_Caseless(parameter[1], "turb"))            layer->tcmods[tcmodindex].tcmod = Q3TCMOD_TURBULENT;
				else Con_DPrintLinef ("%s parsing warning: unknown tcmod mode %s", search->filenames[fileindex], parameter[1]);
			}
			else
				Con_DPrintLinef ("%s parsing warning: too many tcmods on one layer", search->filenames[fileindex]);
		}
		// break out a level if it was a closing brace (not using the character here to not confuse vim)
		if (String_Match_Caseless(com_token, "}"))
			break;
	}
	if (isin2 (layer->rgbgen.rgbgen, Q3RGBGEN_LIGHTINGDIFFUSE, Q3RGBGEN_VERTEX))
		shader.lighting = true;
	if (layer->alphagen.alphagen == Q3ALPHAGEN_VERTEX) {
		if (layer == shader.layers + 0) {
			// vertex controlled transparency
			shader.vertexalpha = true;
		} else {
			// multilayer terrain shader or similar
			shader.textureblendalpha = true;
			if (mod_q3shader_force_terrain_alphaflag.integer /*d: 0*/)
				shader.layers[0].dptexflags |= TEXF_ALPHA;
		}
	} // if

	if (mod_q3shader_force_addalpha.integer /* d: 0*/) {
		// for a long while, DP treated GL_ONE GL_ONE as GL_SRC_ALPHA GL_ONE
		// this cvar brings back this behaviour
		if (layer->blendfunc[0] == GL_ONE && layer->blendfunc[1] == GL_ONE)
			layer->blendfunc[0] = GL_SRC_ALPHA;
	}

	layer->dptexflags = 0;
	if (layer->alphatest)
		layer->dptexflags |= TEXF_ALPHA;
	switch (layer->blendfunc[0]) {
		case GL_SRC_ALPHA:
		case GL_ONE_MINUS_SRC_ALPHA:
			layer->dptexflags |= TEXF_ALPHA;
			break;
	} // sw
	switch (layer->blendfunc[1]) {
		case GL_SRC_ALPHA:
		case GL_ONE_MINUS_SRC_ALPHA:
			layer->dptexflags |= TEXF_ALPHA;
			break;
	} // sw
	if (!(shader.surfaceparms & Q3SURFACEPARM_NOMIPMAPS))
		layer->dptexflags |= TEXF_MIPMAP;
	if (!(shader.textureflags & Q3TEXTUREFLAG_NOPICMIP))
		layer->dptexflags |= TEXF_PICMIP | TEXF_COMPRESS;
	if (layer->clampmap)
		layer->dptexflags |= TEXF_CLAMP;
	continue;
}
numparameters = 0;
//for (j = 0; strcasecmp(com_token, "\n") && strcasecmp(com_token, "}");j++) {
for (j = 0; false == String_Match_Caseless (com_token, "\n") && false == String_Match_Caseless(com_token, "}"); j ++) {
	if (j < TEXTURE_MAXFRAMES_64 + 4) {
		// remap dp_water to dpwater, dp_reflect to dpreflect, etc.
		if (j == 0 && String_Starts_With_Caseless_PRE(com_token, "dp_"))
			c_dpsnprintf1 (parameter[j], "dp%s", &com_token[3]);
		else
			c_strlcpy(parameter[j], com_token);
		numparameters = j + 1;
	}
	if (!COM_ParseToken_QuakeC(&text, true))
		break;
}
//for (j = numparameters;j < TEXTURE_MAXFRAMES_64 + 4;j++)
//	parameter[j][0] = 0;
if (fileindex == 0 && String_Match_Caseless(com_token, "}"))
	break;
if (developer_insane.integer) {
	Con_DPrintf ("%s: ", shader.name);
	for (j = 0;j < numparameters;j++)
		Con_DPrintf (" %s", parameter[j]);
	Con_DPrint("\n");
}
if (numparameters < 1)
	continue;
if (String_Match_Caseless(parameter[0], "surfaceparm") && numparameters >= 2) {
	if (String_Match_Caseless(parameter[1], "alphashadow"))		shader.surfaceparms |= Q3SURFACEPARM_ALPHASHADOW;
	else if (String_Match_Caseless(parameter[1], "areaportal"))	shader.surfaceparms |= Q3SURFACEPARM_AREAPORTAL;
	else if (String_Match_Caseless(parameter[1], "botclip"))		shader.surfaceparms |= Q3SURFACEPARM_BOTCLIP;
	else if (String_Match_Caseless(parameter[1], "clusterportal")) shader.surfaceparms |= Q3SURFACEPARM_CLUSTERPORTAL;
	else if (String_Match_Caseless(parameter[1], "detail"))		shader.surfaceparms |= Q3SURFACEPARM_DETAIL;
	else if (String_Match_Caseless(parameter[1], "donotenter"))	shader.surfaceparms |= Q3SURFACEPARM_DONOTENTER;
	else if (String_Match_Caseless(parameter[1], "dust"))			shader.surfaceparms |= Q3SURFACEPARM_DUST;
	else if (String_Match_Caseless(parameter[1], "hint"))			shader.surfaceparms |= Q3SURFACEPARM_HINT;
	else if (String_Match_Caseless(parameter[1], "fog"))			shader.surfaceparms |= Q3SURFACEPARM_FOG;
	else if (String_Match_Caseless(parameter[1], "lava"))			shader.surfaceparms |= Q3SURFACEPARM_LAVA;
	else if (String_Match_Caseless(parameter[1], "lightfilter"))	shader.surfaceparms |= Q3SURFACEPARM_LIGHTFILTER;
	else if (String_Match_Caseless(parameter[1], "lightgrid"))		shader.surfaceparms |= Q3SURFACEPARM_LIGHTGRID;
	else if (String_Match_Caseless(parameter[1], "metalsteps"))	shader.surfaceparms |= Q3SURFACEPARM_METALSTEPS;
	else if (String_Match_Caseless(parameter[1], "nodamage"))		shader.surfaceparms |= Q3SURFACEPARM_NODAMAGE;
	else if (String_Match_Caseless(parameter[1], "nodlight"))		shader.surfaceparms |= Q3SURFACEPARM_NODLIGHT;
	else if (String_Match_Caseless(parameter[1], "nodraw"))		shader.surfaceparms |= Q3SURFACEPARM_NODRAW;
	else if (String_Match_Caseless(parameter[1], "nodrop"))		shader.surfaceparms |= Q3SURFACEPARM_NODROP;
	else if (String_Match_Caseless(parameter[1], "noimpact"))		shader.surfaceparms |= Q3SURFACEPARM_NOIMPACT;
	else if (String_Match_Caseless(parameter[1], "nolightmap"))	shader.surfaceparms |= Q3SURFACEPARM_NOLIGHTMAP;
	else if (String_Match_Caseless(parameter[1], "nomarks"))		shader.surfaceparms |= Q3SURFACEPARM_NOMARKS;
	else if (String_Match_Caseless(parameter[1], "nomipmaps"))		shader.surfaceparms |= Q3SURFACEPARM_NOMIPMAPS;
	else if (String_Match_Caseless(parameter[1], "nonsolid"))		shader.surfaceparms |= Q3SURFACEPARM_NONSOLID;
	else if (String_Match_Caseless(parameter[1], "origin"))		shader.surfaceparms |= Q3SURFACEPARM_ORIGIN;
	else if (String_Match_Caseless(parameter[1], "playerclip"))	shader.surfaceparms |= Q3SURFACEPARM_PLAYERCLIP;
	else if (String_Match_Caseless(parameter[1], "sky"))			shader.surfaceparms |= Q3SURFACEPARM_SKY;
	else if (String_Match_Caseless(parameter[1], "slick"))			shader.surfaceparms |= Q3SURFACEPARM_SLICK;
	else if (String_Match_Caseless(parameter[1], "slime"))			shader.surfaceparms |= Q3SURFACEPARM_SLIME;
	else if (String_Match_Caseless(parameter[1], "structural"))	shader.surfaceparms |= Q3SURFACEPARM_STRUCTURAL;
	else if (String_Match_Caseless(parameter[1], "trans"))			shader.surfaceparms |= Q3SURFACEPARM_TRANS;
	else if (String_Match_Caseless(parameter[1], "water"))			shader.surfaceparms |= Q3SURFACEPARM_WATER;
	else if (String_Match_Caseless(parameter[1], "pointlight"))	shader.surfaceparms |= Q3SURFACEPARM_POINTLIGHT;
	else if (String_Match_Caseless(parameter[1], "antiportal"))	shader.surfaceparms |= Q3SURFACEPARM_ANTIPORTAL;
	else if (String_Match_Caseless(parameter[1], "skip"))
		; // shader.surfaceparms |= Q3SURFACEPARM_SKIP; FIXME we don't have enough #defines for this any more, and the engine doesn't need this one anyway
	else
	{
		// try custom surfaceparms
		for (j = 0; j < numcustsurfaceflags; j++) {
			if (String_Match_Caseless(custsurfaceparmnames[j], parameter[1])) {
				shader.surfaceflags |= custsurfaceflags[j];
				break;
			}
		}
		// failed all
		if (j == numcustsurfaceflags)
			Con_DPrintLinef ("%s parsing warning: unknown surfaceparm " QUOTED_S, search->filenames[fileindex], parameter[1]);
	}
} // surfaceparm
else if (String_Match_Caseless(parameter[0], "dpshadow"))			shader.dpshadow = true;
else if (String_Match_Caseless(parameter[0], "dpnoshadow"))		shader.dpnoshadow = true;
else if (String_Match_Caseless(parameter[0], "dpnortlight"))		shader.dpnortlight = true;
else if (String_Match_Caseless(parameter[0], "dpreflectcube"))		c_strlcpy (shader.dpreflectcube, parameter[1]);
else if (String_Match_Caseless(parameter[0], "dpmeshcollisions"))	shader.dpmeshcollisions = true;
// this sets dpshaderkill to true if dpshaderkillifcvarzero was used, and to false if dpnoshaderkillifcvarzero was used
else if (((dpshaderkill = String_Match_Caseless(parameter[0], "dpshaderkillifcvarzero")) || String_Match_Caseless(parameter[0], "dpnoshaderkillifcvarzero")) && numparameters >= 2) {
	if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) == 0.0f)
		shader.dpshaderkill = dpshaderkill;
}
// this sets dpshaderkill to true if dpshaderkillifcvar was used, and to false if dpnoshaderkillifcvar was used
else if (((dpshaderkill = String_Match_Caseless(parameter[0], "dpshaderkillifcvar")) || 
	String_Match_Caseless(parameter[0], "dpnoshaderkillifcvar")) && numparameters >= 2) {
	const char *op = NULL;
	if (numparameters >= 3)
		op = parameter[2];
	if (!op) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) != 0.0f)
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, "==")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) == atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, "!=")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) != atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, ">")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) > atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, "<")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) < atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, ">=")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) >= atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else if (numparameters >= 4 && String_Match(op, "<=")) {
		if (Cvar_VariableValue(&cvars_all, parameter[1], ~0) <= atof(parameter[3]))
			shader.dpshaderkill = dpshaderkill;
	} else {
		Con_DPrintLinef ("%s parsing warning: unknown dpshaderkillifcvar op " QUOTED_S ", or not enough arguments", search->filenames[fileindex], op);
	}
} else if (String_Match_Caseless(parameter[0], "sky") && numparameters >= 2) {
	// some q3 skies don't have the sky parm set
	shader.surfaceparms |= Q3SURFACEPARM_SKY;
	strlcpy(shader.skyboxname, parameter[1], sizeof(shader.skyboxname));
} else if (String_Match_Caseless(parameter[0], "skyparms") && numparameters >= 2) {
	// some q3 skies don't have the sky parm set
	shader.surfaceparms |= Q3SURFACEPARM_SKY;
	//if (!atoi(parameter[1]) && strcasecmp(parameter[1], "-"))
	if (atoi(parameter[1]) == 0 && String_NOT_Match (parameter[1], "-") )
		c_strlcpy (shader.skyboxname, parameter[1]);
} else if (String_Match_Caseless(parameter[0], "cull") && numparameters >= 2) {
	if (String_Match_Caseless(parameter[1], "disable") || String_Match_Caseless(parameter[1], "none") || String_Match_Caseless(parameter[1], "twosided"))
		shader.textureflags |= Q3TEXTUREFLAG_TWOSIDED;
} else if (String_Match_Caseless(parameter[0], "nomipmaps")) shader.surfaceparms |= Q3SURFACEPARM_NOMIPMAPS;
else if (String_Match_Caseless(parameter[0], "nopicmip")) shader.textureflags |= Q3TEXTUREFLAG_NOPICMIP;
else if (String_Match_Caseless(parameter[0], "polygonoffset")) shader.textureflags |= Q3TEXTUREFLAG_POLYGONOFFSET;
else if (String_Match_Caseless(parameter[0], "dppolygonoffset")) {
	shader.textureflags |= Q3TEXTUREFLAG_POLYGONOFFSET;
	if (numparameters >= 2)
	{
		shader.biaspolygonfactor = atof(parameter[1]);
		if (numparameters >= 3)
			shader.biaspolygonoffset = atof(parameter[2]);
		else
			shader.biaspolygonoffset = 0;
	}
} else if (String_Match_Caseless(parameter[0], "dptransparentsort") && numparameters >= 2) {
	shader.textureflags |= Q3TEXTUREFLAG_TRANSPARENTSORT;
	if (String_Match_Caseless(parameter[1], "sky"))
		shader.transparentsort = TRANSPARENTSORT_SKY;
	else if (String_Match_Caseless(parameter[1], "distance"))
		shader.transparentsort = TRANSPARENTSORT_DISTANCE;
	else if (String_Match_Caseless(parameter[1], "hud"))
		shader.transparentsort = TRANSPARENTSORT_HUD;
	else
		Con_DPrintLinef ("%s parsing warning: unknown dptransparentsort category " QUOTED_S ", or not enough arguments", search->filenames[fileindex], parameter[1]);
} else if (String_Match_Caseless(parameter[0], "dprefract") && numparameters >= 5) {
	shader.textureflags |= Q3TEXTUREFLAG_REFRACTION;
	shader.refractfactor = atof(parameter[1]);
	Vector4Set(shader.refractcolor4f, atof(parameter[2]), atof(parameter[3]), atof(parameter[4]), 1);
} else if (String_Match_Caseless(parameter[0], "dpreflect") && numparameters >= 6) {
	shader.textureflags |= Q3TEXTUREFLAG_REFLECTION;
	shader.reflectfactor = atof(parameter[1]);
	Vector4Set(shader.reflectcolor4f, atof(parameter[2]), atof(parameter[3]), atof(parameter[4]), atof(parameter[5]));
} else if (String_Match_Caseless(parameter[0], "dpcamera")) {
	shader.textureflags |= Q3TEXTUREFLAG_CAMERA;
} else if (String_Match_Caseless(parameter[0], "dpwater") && numparameters >= 12) {
	shader.textureflags |= Q3TEXTUREFLAG_WATERSHADER;
	shader.reflectmin = atof(parameter[1]);
	shader.reflectmax = atof(parameter[2]);
	shader.refractfactor = atof(parameter[3]);
	shader.reflectfactor = atof(parameter[4]);
	Vector4Set(shader.refractcolor4f, atof(parameter[5]), atof(parameter[6]), atof(parameter[7]), 1);
	Vector4Set(shader.reflectcolor4f, atof(parameter[8]), atof(parameter[9]), atof(parameter[10]), 1);
	shader.r_water_wateralpha = atof(parameter[11]);
} else if (String_Match_Caseless(parameter[0], "dpwaterscroll") && numparameters >= 3) {
	shader.r_water_waterscroll[0] = 1/atof(parameter[1]);
	shader.r_water_waterscroll[1] = 1/atof(parameter[2]);
} else if (String_Match_Caseless(parameter[0], "dpglossintensitymod") && numparameters >= 2) {
	shader.specularscalemod = atof(parameter[1]);
} else if (String_Match_Caseless(parameter[0], "dpglossexponentmod") && numparameters >= 2) {
	shader.specularpowermod = atof(parameter[1]);
} else if (String_Match_Caseless(parameter[0], "dprtlightambient") && numparameters >= 2) {
	shader.rtlightambient = atof(parameter[1]);
} else if (String_Match_Caseless(parameter[0], "dpoffsetmapping") && numparameters >= 2) {
	if (String_Match_Caseless(parameter[1], "disable") || String_Match_Caseless(parameter[1], "none") || String_Match_Caseless(parameter[1], "off")) shader.offsetmapping = OFFSETMAPPING_OFF;
	else if (String_Match_Caseless(parameter[1], "default") || String_Match_Caseless(parameter[1], "normal")) shader.offsetmapping = OFFSETMAPPING_DEFAULT;
	else if (String_Match_Caseless(parameter[1], "linear")) shader.offsetmapping = OFFSETMAPPING_LINEAR;
	else if (String_Match_Caseless(parameter[1], "relief")) shader.offsetmapping = OFFSETMAPPING_RELIEF;
	if (numparameters >= 3)
		shader.offsetscale = atof(parameter[2]);
	if (numparameters >= 5) {
		if (String_Match_Caseless(parameter[3], "bias")) shader.offsetbias = atof(parameter[4]);
		else if (String_Match_Caseless(parameter[3], "match")) shader.offsetbias = 1.0f - atof(parameter[4]);
		else if (String_Match_Caseless(parameter[3], "match8")) shader.offsetbias = 1.0f - atof(parameter[4]) / 255.0f;
		else if (String_Match_Caseless(parameter[3], "match16")) shader.offsetbias = 1.0f - atof(parameter[4]) / 65535.0f;
	} // numparameters >= 5
}
else if (String_Match_Caseless(parameter[0], "deformvertexes") && numparameters >= 2) {
	int deformindex;
	for (deformindex = 0;deformindex < Q3MAXDEFORMS_4;deformindex++)
		if (!shader.deforms[deformindex].deform)
			break;
	if (deformindex < Q3MAXDEFORMS_4) {
		for (i = 0;i < numparameters - 2 && i < Q3DEFORM_MAXPARMS_3;i++)
			shader.deforms[deformindex].parms[i] = atof(parameter[i+2]);
		     if (String_Match_Caseless(parameter[1], "projectionshadow")) shader.deforms[deformindex].deform = Q3DEFORM_PROJECTIONSHADOW;
		else if (String_Match_Caseless(parameter[1], "autosprite"      )) shader.deforms[deformindex].deform = Q3DEFORM_AUTOSPRITE;
		else if (String_Match_Caseless(parameter[1], "autosprite2"     )) shader.deforms[deformindex].deform = Q3DEFORM_AUTOSPRITE2;
		else if (String_Match_Caseless(parameter[1], "text0"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT0;
		else if (String_Match_Caseless(parameter[1], "text1"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT1;
		else if (String_Match_Caseless(parameter[1], "text2"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT2;
		else if (String_Match_Caseless(parameter[1], "text3"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT3;
		else if (String_Match_Caseless(parameter[1], "text4"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT4;
		else if (String_Match_Caseless(parameter[1], "text5"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT5;
		else if (String_Match_Caseless(parameter[1], "text6"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT6;
		else if (String_Match_Caseless(parameter[1], "text7"           )) shader.deforms[deformindex].deform = Q3DEFORM_TEXT7;
		else if (String_Match_Caseless(parameter[1], "bulge"           )) shader.deforms[deformindex].deform = Q3DEFORM_BULGE;
		else if (String_Match_Caseless(parameter[1], "normal"          )) shader.deforms[deformindex].deform = Q3DEFORM_NORMAL;
		else if (String_Match_Caseless(parameter[1], "wave"            ))
		{
			shader.deforms[deformindex].deform = Q3DEFORM_WAVE;
			shader.deforms[deformindex].wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[3]);
			for (i = 0;i < numparameters - 4 && i < Q3WAVEPARMS_4;i++)
				shader.deforms[deformindex].waveparms[i] = atof(parameter[i+4]);
		}
		else if (String_Match_Caseless(parameter[1], "move"))
		{
			shader.deforms[deformindex].deform = Q3DEFORM_MOVE;
			shader.deforms[deformindex].wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[5]);
			for (i = 0;i < numparameters - 6 && i < Q3WAVEPARMS_4;i++)
				shader.deforms[deformindex].waveparms[i] = atof(parameter[i+6]);
		}
#if 1 // Baker r0084: roundwave deformation
		else if (String_Match_Caseless(parameter[1], "roundwave"))
		{
			shader.deforms[deformindex].deform = Q3DEFORM_ROUNDWAVE;
			shader.deforms[deformindex].parms2[0] = atof(parameter[5]); // offsetx
			shader.deforms[deformindex].parms2[1] = atof(parameter[6]); // offsety
			shader.deforms[deformindex].parms2[2] = atof(parameter[7]); // offsetz
			shader.deforms[deformindex].wavefunc = Mod_LoadQ3Shaders_EnumerateWaveFunc(parameter[8]);
			for (i = 0;i < numparameters - 9 && i < Q3WAVEPARMS_4;i++)
				shader.deforms[deformindex].waveparms[i] = atof(parameter[i+9]);
		}
#endif
	}
}

