// menu_options2.c.h


const char *ef2 (int level)
{
	switch (level) {
	default:
	case  0:	return "DarkPlaces";
	case  1:	return "Quake";
	case  2:	return "Off";
	}
}

int getef2()
{
	if (cl_bobmodel_speed.value <= 0.5 ) return 2;	// Off
	if (cl_bobmodel_speed.value <= 4.5 ) return 1;
	return 0;
}


static void setlevel2 (int level)
{
	//int x = getef2();
	
	switch (level) {
	case 0:  // DarkPlaces
		Cbuf_AddTextLine ("cl_bobmodel_speed 7; cl_bobmodel_side 0.15; cl_bobmodel 0.06");
		break;

	case 1: // Quake
		Cbuf_AddTextLine ("cl_bobmodel_speed 4; cl_bobmodel_side 0; cl_bobmodel 0.06");
		break;

	case 2:  // Off
		Cbuf_AddTextLine ("cl_bobmodel_speed 0; cl_bobmodel_side 0; cl_bobmodel 0");
		break;
	} // sw

}

// Gun position
const char *ef3 (int level)
{
	switch (level) {
	default:
	case  0:	return "DarkPlaces ";
	case  1:	return "Quake";
	//M_DrawCharacter (x-8, y, 128);
	//for (i = 0;i < SLIDER_RANGE;i++)
	//	M_DrawCharacter (x + i*8, y, 129);
	//M_DrawCharacter (x+i*8, y, 130);
	//M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);

	}
}

int getef3()
{
	if (!r_viewmodel_quake.value) return 0;	// Off
	return 1;
	
}


static void setlevel3 (int level)
{
	//int x = getef2();
	
	switch (level) {
	case 0:  // DarkPlaces
		Cbuf_AddTextLine ("r_viewmodel_quake 0");
		break;

	case 1: // Quake
		Cbuf_AddTextLine ("r_viewmodel_quake 1");
		break;

	} // sw

}

// Gun position
const char *ef4 (int level)
{
	switch (level) {
	case  0:	return "DarkPlaces";
	case  2:	return "Re-Release";
	default:
	case  1:	return "Quake Backtile";
	}
}

// Warp
const char *efwarp (int level)
{
	switch (level) {
	case  0:	return "off";
	case  2:	return "twisty (2)";
	default:
	case  1:	return "classic (1)";
	}
}

int getefwaterwarp()
{
	extern cvar_t r_waterwarp;
	if (!r_waterwarp.value) return 0;
	if (r_waterwarp.value >= 2) return 2;
	return 1;
}

// Warp
const char *efdistort (int level)
{
	switch (level) {
	case  0:	return "off";
	case  2:	return "liquids (2)";
	default:
	case  1:	return "water/slime (1)";
	}
}

int getefdistort()
{
	if (!r_waterdeform.value) return 0;
	if (r_waterdeform.value >= 2) return 2;
	return 1;
}

int getef4()
{
	extern cvar_t sbar_quake;
	if (!sbar_quake.value) return 0;
	if (sbar_quake.value >= 2) return 2;
	
	return 1;	
}


static void setlevel4 (int level)
{
	//int x = getef2();
	
	switch (level) {
	case 0:  // Off
		Cbuf_AddTextLine ("sbar_quake 0");
		break;

	case 1:  // Traditional
		Cbuf_AddTextLine ("sbar_quake 1");
		break;

	case 2:  // Remaster
		Cbuf_AddTextLine ("sbar_quake 2");
		break;

	} // sw

}


// Gun position
const char *ef5 (int level)
{
	switch (level) {
	case  0:	return "off";
	default:
	case  1:	return "on";
	case  2:	return "top corner";
	}
}

int getef5()
{
	if (!showfps.value) return 0;
	if (showfps.value < 0) return 2;
	return 1;	
}


static void setlevel5 (int level)
{
	//int x = getef2();
	
	switch (level) {
	case 0:  // Off
		Cbuf_AddTextLine ("showfps 0");
		break;

	case 1:  // Traditional
		Cbuf_AddTextLine ("showfps 1");
		break;

	case 2: // DarkPlacs
		Cbuf_AddTextLine ("showfps -1");
		break;

	} // sw

}



const char *ef (int level)
{
	switch (level) {
	//case 0:		return "0/5 -Decals, -Sky";
	case 0:		return "1/6 -Dynamic Light";
	case 1:		return "2/6  Vanilla -gloss -decals";
	default:
	case 2:		return "3/6  Default";
	case 3:		return "4/6 +Bloom, +Deluxe";
	case 4:		return "5/6 +Offset Mapping";
	case 5:		return "6/6 +Realtime Light";
	}
}

int getef()
{
	//if (cl_particles_quality.value <= 0.45 ) return 0;
	if (cl_particles_quality.value <= 0.85 ) return 0;
	if (cl_particles_quality.value <= 0.995 ) return 1;
	if (cl_particles_quality.value <= 1.005 ) return 2;
	if (cl_particles_quality.value <= 1.015 ) return 3;
	if (cl_particles_quality.value <= 1.025 ) return 4;
	return 5;
}


static void setlevel (int level)
{
	//int x = getef();
	
	switch (level) {
	//case 0: 
	//	// Level -2 - crap
	//	Cbuf_AddTextLine ("cl_decals 0; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 0.4; gl_flashblend 1; mod_q3bsp_nolightmaps 1; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 100; r_drawparticles_drawdistance 250; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_gloss 0; r_shadow_realtime_dlight 0; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 0; r_showsurfaces 3; r_sky 0; r_subdivisions_tolerance 16; r_water 0; r_water_resolutionmultiplier 0");  
	//	break;

	case 0: 
		// Level -1 - Degraded .. r_shadow_realtime_dlight 0 .. less particles .. worse mirror r_shadow_usenormalmap 0 def 1
		Cbuf_AddTextLine ("cl_decals 0; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 0.8; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 0; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 0; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (iszirc || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine ("r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 0");
		} else {
			Cbuf_AddTextLine ("r_water 0; r_water_resolutionmultiplier 0.5; r_shadow_gloss 0");
		}
		break;

	case 1:  // Vanilla level							
		Cbuf_AddTextLine ("cl_decals 0; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 0.990; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (iszirc || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine ("r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 0");
		} else {
			Cbuf_AddTextLine ("r_water 1; r_water_resolutionmultiplier 0.5; r_shadow_gloss 0");
		}
		break;


	case 2:  // default level							
		Cbuf_AddTextLine ("cl_decals 1; cl_decals_fadetime 1; cl_decals_models 0; cl_particles_quality 1; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 0; r_coronas_occlusionquery 0; r_depthfirst 0; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 0; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4");
		if (iszirc || gamemode == GAME_QUAKE3_QUAKE1) {
			Cbuf_AddTextLine ("r_water 1; r_water_resolutionmultiplier 1.0; r_shadow_gloss 1");
		} else {
			Cbuf_AddTextLine ("r_water 1; r_water_resolutionmultiplier 0.5; r_shadow_gloss 1");
		}
		break;

	case 3:  // Good r_depthfirst r_coronas_occlusionquery r_bloom cl_decals_fadetime r_glsl_deluxemapping
		Cbuf_AddTextLine ("cl_decals 1; cl_decals_fadetime 4; cl_decals_models 0; cl_particles_quality 1.01; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 0; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 0; r_shadow_realtime_world 0; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 0; r_shadow_shadowmapping 0; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 4; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine ("r_shadow_gloss 1");
		break;

	case 4:  // Gooderx r_glsl_offsetmapping r_subdivisions_tolerance
		Cbuf_AddTextLine ("cl_decals 1; cl_decals_fadetime 10; cl_decals_models 0; cl_particles_quality 1.02; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 2000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 1; r_glsl_offsetmapping_reliefmapping 0; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 1; r_shadow_realtime_world 1; r_shadow_realtime_world_shadows 0; r_shadow_realtime_world_lightmaps 1; r_shadow_shadowmapping 1; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 2; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine ("r_shadow_gloss 1");
		break;

	case 5: // Highest r_shadow_realtime_world
		Cbuf_AddTextLine ("cl_decals 1; cl_decals_fadetime 10; cl_decals_models 1; cl_particles_quality 1.03; gl_flashblend 0; mod_q3bsp_nolightmaps 0; r_bloom 1; r_coronas_occlusionquery 1; r_depthfirst 2; r_drawdecals_drawdistance 500; r_drawparticles_drawdistance 3000; r_glsl_deluxemapping 1; r_glsl_offsetmapping 1; r_glsl_offsetmapping_reliefmapping 1; r_motionblur 0.4; r_shadow_gloss 1; r_shadow_realtime_dlight 1; r_shadow_realtime_dlight_shadows 1; r_shadow_realtime_world 1; r_shadow_realtime_world_shadows 1; r_shadow_realtime_world_lightmaps 1; r_shadow_shadowmapping 1; r_shadow_usenormalmap 1; r_showsurfaces 0; r_sky 1; r_subdivisions_tolerance 1; r_water 1; r_water_resolutionmultiplier 1");
		Cbuf_AddTextLine ("r_shadow_gloss 1");
		break;
	} // sw

}

