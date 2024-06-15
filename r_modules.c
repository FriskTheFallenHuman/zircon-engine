
#include "quakedef.h"

#define MAXRENDERMODULES_20 20

typedef struct rendermodule_s
{
	int active; // set by start, cleared by shutdown
	const char *name;
	void(*start)(void);
	void(*shutdown)(void);
	void(*newmap)(void);
	void(*devicelost)(void);
	void(*devicerestored)(void);
}
rendermodule_t;

rendermodule_t rendermodule[MAXRENDERMODULES_20];

void R_Modules_InitOnce(void)
{
	Cmd_AddCommand(CF_CLIENT, "r_restart", R_Modules_Restart_f, "restarts renderer");
}

void R_RegisterModule(const char *name, void(*start)(void), void(*shutdown)(void), void(*newmap)(void), void(*devicelost)(void), void(*devicerestored)(void))
{
	int i;
	for (i = 0; i < MAXRENDERMODULES_20; i ++) {
		if (rendermodule[i].name == NULL)
			break;
		if (String_Match(name, rendermodule[i].name)) {
			Con_PrintLinef ("R_RegisterModule: module " QUOTED_S " registered twice", name);
			return;
		}
	}
	if (i >= MAXRENDERMODULES_20)
		Sys_Error ("R_RegisterModule: ran out of renderer module slots (%d)", MAXRENDERMODULES_20);
	rendermodule[i].active = 0;
	rendermodule[i].name = name;
	rendermodule[i].start = start;
	rendermodule[i].shutdown = shutdown;
	rendermodule[i].newmap = newmap;
	rendermodule[i].devicelost = devicelost;
	rendermodule[i].devicerestored = devicerestored;
}

void R_Modules_Start(void)
{
// Baker: Looks like this more or less
WARP_X_ (Palette_InitOnce)
WARP_X_ (sdl_start)
WARP_X_ (gl_backend_start)
WARP_X_ (r_textures_start r_textures_shutdown)
WARP_X_ (gl_main_start)
WARP_X_ (gl_draw_start)
WARP_X_ (r_shadow_start)
WARP_X_ (r_sky_start)
WARP_X_ (sbar_start)
WARP_X_ (r_part_start)
WARP_X_ (r_explosion_start)
WARP_X_ (r_lightningbeams_start)
WARP_X_ (CL_MeshEntities_Start)
WARP_X_ (mod_start)
WARP_X_ (cl_video_start)

	int i;
	for (i = 0; i < MAXRENDERMODULES_20; i ++) {
		if (rendermodule[i].name == NULL)
			continue;
		if (rendermodule[i].active) {
			Con_PrintLinef ("R_Modules_Start: module " QUOTED_S " already active", rendermodule[i].name);
			continue;
		}
		Con_DPrintLinef ("Starting render module " QUOTED_S, rendermodule[i].name);
		rendermodule[i].active = 1;
		rendermodule[i].start();
	}
}

void R_Modules_Shutdown(void)
{
	int i;
	// shutdown in reverse
	for (i = MAXRENDERMODULES_20 - 1; i >= 0; i--) {
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		Con_DPrintLinef ("Stopping render module " QUOTED_S, rendermodule[i].name);
		rendermodule[i].active = 0;
		rendermodule[i].shutdown();
	}
}

void R_Modules_Restart_f(cmd_state_t *cmd)
{
	CL_StartVideo();
	Con_PrintLinef ("Restarting renderer");
	SCR_BeginLoadingPlaque(false);
	R_Modules_Shutdown();
	R_Modules_Start();
}

void R_Modules_NewMap(void)
{
	R_SkinFrame_PrepareForPurge();
	for (int i = 0; i < MAXRENDERMODULES_20; i ++) {
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		rendermodule[i].newmap();
	}
	R_SkinFrame_Purge();
}

void R_Modules_DeviceLost(void)
{
	int i;
	for (i = 0;i < MAXRENDERMODULES_20;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		if (!rendermodule[i].devicelost)
			continue;
		rendermodule[i].devicelost();
	}
}


void R_Modules_DeviceRestored(void)
{
	int i;
	for (i = 0;i < MAXRENDERMODULES_20;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		if (!rendermodule[i].devicerestored)
			continue;
		rendermodule[i].devicerestored();
	}
}

