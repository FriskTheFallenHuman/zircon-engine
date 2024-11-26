// menu_options2.c.h

extern cvar_t sbar_quake;
extern cvar_t r_viewmodel_quake;
extern cvar_t r_waterdeform;
extern cvar_t cl_bobmodel_speed;
extern cvar_t cl_bobmodel_classic;
extern cvar_t cl_bob;


const char *get_overbright4_text (int level)
{
	switch (level) {
	default:
	case  0:	return "DarkPlaces";
	case  1:	return "Quakespasm";
	case  2:	return "Minimum Light 3";
//	case  3:	return "Minimum Light 0.5";
	}
}

const char *get_bobbing2_text (int level)
{
	switch (level) {
	default:
	case  0:	return "Quake";
	case  1:	return "DarkPlaces";
	case  2:	return "Off";
	}
}

int get_bobbing2_rot()
{
	if (cl_bob.value == 0) return 2; // Off
	if (cl_bobmodel_classic.value == 0) return 1; // DarkPlaces
	return 0;
}

// Get the current level.  DarkPlaces 0, Quakespasm 1
int get_overbright4_rot()
{
	if (r_minlight.value == 3) return 2;
	//if (r_minlight.value == 0.5) return 3;
	if (gl_overbright_models.value) return 1;	// Off
	return 0;
}


static void set_overbright4 (int level)
{
	switch (level) {
	default:
	case 0:  Cbuf_AddTextLine (cmd_local, "gl_overbright_models 0; r_minlight 0"); break; // DarkPlaces
	case 1:  Cbuf_AddTextLine (cmd_local, "gl_overbright_models 1; r_minlight 0"); break; // Quakespasm
	case 2:  Cbuf_AddTextLine (cmd_local, "gl_overbright_models 0; r_minlight 3"); break; // Minimum Light 3
	//case 3:  Cbuf_AddTextLine (cmd_local, "gl_overbright_models 0; r_minlight 0.5"); break; // Minimum Light 0.5
	} // sw
}

static void set_bobbing2 (int level)
{
	switch (level) {
	default:
	case 0: // Quake
		Cbuf_AddTextLine(cmd_local, "cl_bobmodel_classic 1; cvar_reset cl_bob");
		break;

	case 1:  // DarkPlaces
		Cbuf_AddTextLine (cmd_local, "cl_bobmodel_classic 0; cvar_reset cl_bob");
		break;


	case 2:  // Off
		Cbuf_AddTextLine (cmd_local, "cvar_reset cl_bobmodel_classic; cl_bob 0");
		break;
	} // sw

}

// Gun position
const char *get_gunpos3_text (int level)
{
	switch (level) {
	default:
	case  0:	return "DarkPlaces ";
	case  1:	return "Quake";
	}
}

int get_gunpos3_rot()
{
	if (!r_viewmodel_quake.value) return 0;	// Off
	return 1;
}


static void set_gunpos3 (int level)
{
	switch (level) {
	default:
	case 0:  Cbuf_AddTextLine (cmd_local, "r_viewmodel_quake 0"); break; // DarkPlaces
	case 1:  Cbuf_AddTextLine (cmd_local, "r_viewmodel_quake 1"); break; // Quake
	} // sw
}


// Warp
const char *get_waterwarp_text (int level)
{
	switch (level) {
	default:
	case  0:	return "off";
	case  2:	return "twisty (2)";
	case  1:	return "classic (1)";
	}
}

int set_waterwarp6()
{
	if (!r_waterwarp.value) return 0;
	if (r_waterwarp.value >= 2) return 2;
	return 1;
}


int get_waterwarp_rot(void)
{
	if (!r_waterwarp.value) return 0;
	if (r_waterwarp.value == 2) return 2;
	return 1;
}


// Warp
const char *get_waterdeform_text (int level)
{
	switch (level) {
	default:
	case  0:	return "off";
	case  2:	return "liquids (2)";
	case  1:	return "water/slime (1)";
	}
}

int get_deform_rot(void)
{
	if (!r_waterdeform.value) return 0;
	if (r_waterdeform.value >= 2) return 2;
	return 1;
}


const char *get_statusbar4_text(int level)
{
	switch (level) {
	default:
	case  0:	return "DarkPlaces";
	case  2:	return "Re-Release";
	case  1:	return "Quake Backtile";
	}
}

int get_statusbar4_rot (void)
{
	if (!sbar_quake.value) return 0;
	if (sbar_quake.value >= 2) return 2;
	return 1;	
}


static void set_statusbar4 (int level)
{
	switch (level) {
	default:
	case 0:		Cbuf_AddTextLine (cmd_local, "sbar_quake 0");	break; // DarkPlaces
	case 1:		Cbuf_AddTextLine (cmd_local, "sbar_quake 1");	break; // Traditional
	case 2:		Cbuf_AddTextLine (cmd_local, "sbar_quake 2");	break; // Remaster
	} // sw
}


// Show fps
const char *get_showfps5_text (int level)
{
	switch (level) {
	default:
	case  0:	return "off";
	case  1:	return "on";
	case  2:	return "top corner";
	}
}

int get_showfps5_rot()
{
	if (!showfps.value) return 0;
	if (showfps.value < 0) return 2;
	return 1;	
}


static void set_showfps5 (int level)
{
	switch (level) {
	default:
	case 0: Cbuf_AddTextLine (cmd_local, "showfps 0"); break;
	case 1:	Cbuf_AddTextLine (cmd_local, "showfps 1"); break;
	case 2: Cbuf_AddTextLine (cmd_local, "showfps -1"); break;
	} // sw
}



const char *get_effects0_text (int level)
{
	switch (level) {
	case 0:		return "1/6 -Dynamic Light";
	case 1:		return "2/6  Vanilla -gloss -decals";
	default:
	case 2:		return "3/6  Default";
	case 3:		return "4/6 +Bloom, +Deluxe";
	case 4:		return "5/6 +Offset Mapping";
	case 5:		return "6/6 +Realtime Light";
	}
}

int get_effects0_rot()
{
	if (cl_particles_quality.value <= 0.85 ) return 0;
	if (cl_particles_quality.value <= 0.995 ) return 1;
	if (cl_particles_quality.value <= 1.005 ) return 2;
	if (cl_particles_quality.value <= 1.015 ) return 3;
	if (cl_particles_quality.value <= 1.025 ) return 4;
	return 5;
}


static void set_effects0 (int level)
{
	switch (level) {
	case 0: 
		Cbuf_AddTextLine (cmd_local, "cl_decals 0; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 0.8; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 0; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 0; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (fs_is_zircon_galaxy || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine (cmd_local, "r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 0");
		} else {
			Cbuf_AddTextLine (cmd_local, "r_water 0; r_water_resolutionmultiplier 0.5; r_shadow_gloss 0");
		}
		break;

	case 1:  // Vanilla level							
		Cbuf_AddTextLine (cmd_local, "cl_decals 0; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 0.990; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (fs_is_zircon_galaxy || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine (cmd_local, "r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 0");
		} else {
			Cbuf_AddTextLine (cmd_local, "r_water 1; r_water_resolutionmultiplier 0.5; r_shadow_gloss 0");
		}
		break;


	case 2:  // default level							
		Cbuf_AddTextLine (cmd_local, "cl_decals 1; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 1; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (fs_is_zircon_galaxy || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine (cmd_local, "r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 1");
		} else {
			Cbuf_AddTextLine (cmd_local, "r_water 1; r_water_resolutionmultiplier 0.5; r_shadow_gloss 1");
		}
		break;

	case 3:  // Good r_depthfirst r_coronas_occlusionquery r_bloom cl_decals_fadetime r_glsl_deluxemapping
		Cbuf_AddTextLine (cmd_local, "cl_decals 1; cl_decals_fadetime 4; cl_decals_models 0; cl_particles_quality 1.01; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine (cmd_local, "r_shadow_gloss 1");
		break;

	case 4:  // Gooderx r_glsl_offsetmapping r_subdivisions_tolerance
		Cbuf_AddTextLine (cmd_local, "cl_decals 1; cl_decals_fadetime 10; cl_decals_models 0; cl_particles_quality 1.02; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 1; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 1; r_shadow_realtime_world 1; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 1; r_shadow_shadowmapping 1; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 2; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine (cmd_local, "r_shadow_gloss 1");
		break;

	case 5: // Highest r_shadow_realtime_world
		Cbuf_AddTextLine (cmd_local, "cl_decals 1; cl_decals_fadetime 10; cl_decals_models 1; cl_particles_quality 1.03; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 3000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 1; r_glsl_offsetmapping_reliefmapping 1; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 1; r_shadow_realtime_world 1; r_shadow_realtime_world_shadows 1; r_shadow_realtime_world_lightmaps 1; r_shadow_shadowmapping 1; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 1; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine (cmd_local, "r_shadow_gloss 1");
		break;
	} // sw

}

