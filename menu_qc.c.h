// menu_qc.c.h


//============================================================================
// Menu prog handling

static const char *m_required_func[] = {
"m_init",
"m_keydown",
"m_draw",
"m_toggle",
"m_shutdown",
};

static int m_numrequiredfunc = sizeof(m_required_func) / sizeof(char*);

static prvm_required_field_t m_required_fields[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x) {ev_float, #x},
#define PRVM_DECLARE_menufieldvector(x) {ev_vector, #x},
#define PRVM_DECLARE_menufieldstring(x) {ev_string, #x},
#define PRVM_DECLARE_menufieldedict(x) {ev_entity, #x},
#define PRVM_DECLARE_menufieldfunction(x) {ev_function, #x},
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

static int m_numrequiredfields = sizeof(m_required_fields) / sizeof(m_required_fields[0]);

static prvm_required_field_t m_required_globals[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x) {ev_float, #x},
#define PRVM_DECLARE_menuglobalvector(x) {ev_vector, #x},
#define PRVM_DECLARE_menuglobalstring(x) {ev_string, #x},
#define PRVM_DECLARE_menuglobaledict(x) {ev_entity, #x},
#define PRVM_DECLARE_menuglobalfunction(x) {ev_function, #x},
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

static int m_numrequiredglobals = sizeof(m_required_globals) / sizeof(m_required_globals[0]);

void MR_SetRouting (qbool forceold);

void MVM_error_cmd(const char *format, ...) DP_FUNC_PRINTF(1);
void MVM_error_cmd(const char *format, ...)
{
	prvm_prog_t *prog = MVM_prog;
	static qbool processingError = false;
	char errorstring[MAX_INPUTLINE];
	va_list argptr;

	va_start (argptr, format);
	dpvsnprintf (errorstring, sizeof(errorstring), format, argptr);
	va_end (argptr);
	Con_Printf( "Menu_Error: %s\n", errorstring );

	if( !processingError ) {
		processingError = true;
		PRVM_Crash(prog);
		processingError = false;
	} else {
		Con_Printf( "Menu_Error: Recursive call to MVM_error_cmd (from PRVM_Crash)!\n" );
	}

	// fall back to the normal menu

	// say it
	Con_Print("Falling back to normal menu\n");

	key_dest = key_game;

	// init the normal menu now -> this will also correct the menu router pointers
	MR_SetRouting (true);

	// reset the active scene, too (to be on the safe side ;))
   R_SelectScene( RST_CLIENT );

	Host_AbortCurrentFrame();
}

static void MVM_begin_increase_edicts(prvm_prog_t *prog)
{
}

static void MVM_end_increase_edicts(prvm_prog_t *prog)
{
}

static void MVM_init_edict(prvm_prog_t *prog, prvm_edict_t *edict)
{
}

static void MVM_free_edict(prvm_prog_t *prog, prvm_edict_t *ed)
{
}

static void MVM_count_edicts(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *ent;
	int active;

	active = 0;
	for (i=0 ; i<prog->num_edicts ; i++)
	{
		ent = PRVM_EDICT_NUM(i);
		if (ent->priv.required->free)
			continue;
		active++;
	}

	Con_Printf("num_edicts:%3i\n", prog->num_edicts);
	Con_Printf("active    :%3i\n", active);
}

static qbool MVM_load_edict(prvm_prog_t *prog, prvm_edict_t *ent)
{
	return true;
}

static void MP_KeyEvent (int key, int ascii, qbool downevent)
{
	prvm_prog_t *prog = MVM_prog;

	// pass key
	prog->globals.fp[OFS_PARM0] = (prvm_vec_t) key;
	prog->globals.fp[OFS_PARM1] = (prvm_vec_t) ascii;
	if (downevent)
		prog->ExecuteProgram(prog, PRVM_menufunction(m_keydown),"m_keydown(float key, float ascii) required");
	else if (PRVM_menufunction(m_keyup))
		prog->ExecuteProgram(prog, PRVM_menufunction(m_keyup),"m_keyup(float key, float ascii) required");
}

static void MP_Draw (void)
{
	prvm_prog_t *prog = MVM_prog;
	// declarations that are needed right now

	float oldquality;

	R_SelectScene( RST_MENU );

	// reset the temp entities each frame
	r_refdef.scene.numtempentities = 0;

	// menu scenes do not use reduced rendering quality
	oldquality = r_refdef.view.quality;
	r_refdef.view.quality = 1;
	// TODO: this needs to be exposed to R_SetView (or something similar) ASAP [2/5/2008 Andreas]
	r_refdef.scene.time = realtime;

	// FIXME: this really shouldnt error out lest we have a very broken refdef state...?
	// or does it kill the server too?
	PRVM_G_FLOAT(OFS_PARM0) = vid.width;
	PRVM_G_FLOAT(OFS_PARM1) = vid.height;
	prog->ExecuteProgram(prog, PRVM_menufunction(m_draw),"m_draw() required");

	// TODO: imo this should be moved into scene, too [1/27/2008 Andreas]
	r_refdef.view.quality = oldquality;

	R_SelectScene( RST_CLIENT );
}

static void MP_ToggleMenu(int mode)
{
	prvm_prog_t *prog = MVM_prog;

	prog->globals.fp[OFS_PARM0] = (prvm_vec_t) mode;
	prog->ExecuteProgram(prog, PRVM_menufunction(m_toggle),"m_toggle(float mode) required");
}

static void MP_NewMap(void)
{
	prvm_prog_t *prog = MVM_prog;
	if (PRVM_menufunction(m_newmap))
		prog->ExecuteProgram(prog, PRVM_menufunction(m_newmap),"m_newmap() required");
}

const serverlist_entry_t *serverlist_callbackentry = NULL;
static int MP_GetServerListEntryCategory(const serverlist_entry_t *entry)
{
	prvm_prog_t *prog = MVM_prog;
	serverlist_callbackentry = entry;
	if (PRVM_menufunction(m_gethostcachecategory))
	{
		prog->globals.fp[OFS_PARM0] = (prvm_vec_t) -1;
		prog->ExecuteProgram(prog, PRVM_menufunction(m_gethostcachecategory),"m_gethostcachecategory(float entry) required");
		serverlist_callbackentry = NULL;
		return prog->globals.fp[OFS_RETURN];
	}
	else
	{
		return 0;
	}
}

static void MP_Shutdown (void)
{
	prvm_prog_t *prog = MVM_prog;
	if (prog->loaded)
		prog->ExecuteProgram(prog, PRVM_menufunction(m_shutdown),"m_shutdown() required");

	// reset key_dest
	key_dest = key_game;

	// AK not using this cause Im not sure whether this is useful at all instead :
	PRVM_Prog_Reset(prog);
}

static void MP_Init (void)
{
	prvm_prog_t *prog = MVM_prog;
	PRVM_Prog_Init(prog);

	prog->edictprivate_size = 0; // no private struct used
	prog->name = "menu";
	prog->num_edicts = 1;
	prog->limit_edicts = M_MAX_EDICTS;
	prog->extensionstring = vm_m_extensions;
	prog->builtins = vm_m_builtins;
	prog->numbuiltins = vm_m_numbuiltins;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = MVM_begin_increase_edicts;
	prog->end_increase_edicts   = MVM_end_increase_edicts;
	prog->init_edict            = MVM_init_edict;
	prog->free_edict            = MVM_free_edict;
	prog->count_edicts          = MVM_count_edicts;
	prog->load_edict            = MVM_load_edict;
	prog->init_cmd              = MVM_init_cmd;
	prog->reset_cmd             = MVM_reset_cmd;
	prog->error_cmd             = MVM_error_cmd;
	prog->ExecuteProgram        = MVM_ExecuteProgram;

	// allocate the mempools
	prog->progs_mempool = Mem_AllocPool(menu_progs.string, 0, NULL);

	PRVM_Prog_Load(prog, menu_progs.string, NULL, 0, m_numrequiredfunc, m_required_func, m_numrequiredfields, m_required_fields, m_numrequiredglobals, m_required_globals);

	// note: OP_STATE is not supported by menu qc, we don't even try to detect
	// it here

	in_client_mouse = true;

	// call the prog init
	prog->ExecuteProgram(prog, PRVM_menufunction(m_init),"m_init() required");

	// Once m_init was called, we consider menuqc code fully initialized.
	prog->inittime = realtime;
}

//============================================================================
// Menu router

void (*MR_KeyEvent) (int key, int ascii, qbool downevent);
void (*MR_Draw) (void);
void (*MR_ToggleMenu) (int mode);
void (*MR_Shutdown) (void);
void (*MR_NewMap) (void);
int (*MR_GetServerListEntryCategory) (const serverlist_entry_t *entry);

void MR_SetRouting(qbool forceold)
{
	// if the menu prog isnt available or forceqmenu ist set, use the old menu
	if(!FS_FileExists(menu_progs.string) || forceqmenu.integer || forceold)
	{
		// set menu router function pointers
		MR_KeyEvent = M_KeyEvent;
		MR_Draw = M_Draw;
		MR_ToggleMenu = M_ToggleMenu;
		MR_Shutdown = M_Shutdown;
		MR_NewMap = M_NewMap;
		MR_GetServerListEntryCategory = M_GetServerListEntryCategory;
		M_Init();
	}
	else
	{
		// set menu router function pointers
		MR_KeyEvent = MP_KeyEvent;
		MR_Draw = MP_Draw;
		MR_ToggleMenu = MP_ToggleMenu;
		MR_Shutdown = MP_Shutdown;
		MR_NewMap = MP_NewMap;
		MR_GetServerListEntryCategory = MP_GetServerListEntryCategory;
		MP_Init();
	}
}

void MR_Restart(void)
{
	if(MR_Shutdown)
		MR_Shutdown ();
	MR_SetRouting (false);
}

static void Call_MR_ToggleMenu_f(void)
{
	int m;
	m = ((Cmd_Argc() < 2) ? -1 : atoi(Cmd_Argv(1)));
	Host_StartVideo();
	if(MR_ToggleMenu)
		MR_ToggleMenu(m);
}

void MR_Init_Commands(void)
{
	// set router console commands
	Cvar_RegisterVariable (&forceqmenu);
	Cvar_RegisterVariable (&menu_options_colorcontrol_correctionvalue);
	Cvar_RegisterVariable (&menu_progs);
	Cmd_AddCommand ("menu_restart",MR_Restart, "restart menu system (reloads menu.dat)");
	Cmd_AddCommand ("togglemenu", Call_MR_ToggleMenu_f, "opens or closes menu");
}

void MR_Init(void)
{
	vid_mode_t res[1024];
	size_t res_count, i;

	res_count = VID_ListModes(res, sizeof(res) / sizeof(*res));
	res_count = VID_SortModes(res, res_count, false, false, true);
	if(res_count)
	{
		video_resolutions_count = (int)res_count;
		video_resolutions = (video_resolution_t *) Mem_Alloc(cls.permanentmempool, sizeof(*video_resolutions) * (video_resolutions_count + 1));
		memset(&video_resolutions[video_resolutions_count], 0, sizeof(video_resolutions[video_resolutions_count]));
		for(i = 0; i < res_count; ++i)
		{
			int n, d, t;
			video_resolutions[i].type = "Detected mode"; // FIXME make this more dynamic
			video_resolutions[i].width = res[i].width;
			video_resolutions[i].height = res[i].height;
			video_resolutions[i].pixelheight = res[i].pixelheight_num / (double) res[i].pixelheight_denom;
			n = res[i].pixelheight_denom * video_resolutions[i].width;
			d = res[i].pixelheight_num * video_resolutions[i].height;
			while(d)
			{
				t = n;
				n = d;
				d = t % d;
			}
			d = (res[i].pixelheight_num * video_resolutions[i].height) / n;
			n = (res[i].pixelheight_denom * video_resolutions[i].width) / n;
			switch(n * 0x10000 | d)
			{
				case 0x00040003:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "Standard 4x3";
					break;
				case 0x00050004:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 512;
					if(res[i].pixelheight_denom == res[i].pixelheight_num)
						video_resolutions[i].type = "Square Pixel (LCD) 5x4";
					else
						video_resolutions[i].type = "Short Pixel (CRT) 5x4";
					break;
				case 0x00080005:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 400;
					if(res[i].pixelheight_denom == res[i].pixelheight_num)
						video_resolutions[i].type = "Widescreen 8x5";
					else
						video_resolutions[i].type = "Tall Pixel (CRT) 8x5";

					break;
				case 0x00050003:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 384;
					video_resolutions[i].type = "Widescreen 5x3";
					break;
				case 0x000D0009:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 400;
					video_resolutions[i].type = "Widescreen 14x9";
					break;
				case 0x00100009:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "Widescreen 16x9";
					break;
				case 0x00030002:
					video_resolutions[i].conwidth = 720;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "NTSC 3x2";
					break;
				case 0x000D000B:
					video_resolutions[i].conwidth = 720;
					video_resolutions[i].conheight = 566;
					video_resolutions[i].type = "PAL 14x11";
					break;
				case 0x00080007:
					if(video_resolutions[i].width >= 512)
					{
						video_resolutions[i].conwidth = 512;
						video_resolutions[i].conheight = 448;
						video_resolutions[i].type = "SNES 8x7";
					}
					else
					{
						video_resolutions[i].conwidth = 256;
						video_resolutions[i].conheight = 224;
						video_resolutions[i].type = "NES 8x7";
					}
					break;
				default:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 640 * d / n;
					video_resolutions[i].type = "Detected mode";
					break;
			}
			if(video_resolutions[i].conwidth > video_resolutions[i].width || video_resolutions[i].conheight > video_resolutions[i].height)
			{
				int f1, f2;
				f1 = video_resolutions[i].conwidth > video_resolutions[i].width;
				f2 = video_resolutions[i].conheight > video_resolutions[i].height;
				if(f1 > f2)
				{
					video_resolutions[i].conwidth = video_resolutions[i].width;
					video_resolutions[i].conheight = video_resolutions[i].conheight / f1;
				}
				else
				{
					video_resolutions[i].conwidth = video_resolutions[i].conwidth / f2;
					video_resolutions[i].conheight = video_resolutions[i].height;
				}
			}
		}
	}
	else
	{
		video_resolutions = video_resolutions_hardcoded;
		video_resolutions_count = sizeof(video_resolutions_hardcoded) / sizeof(*video_resolutions_hardcoded) - 1;
	}

	menu_video_resolutions_forfullscreen = !!vid_fullscreen.integer;
	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);

	// use -forceqmenu to use always the normal quake menu (it sets forceqmenu to 1)
// COMMANDLINEOPTION: Client: -forceqmenu disables menu.dat (same as +forceqmenu 1)
	if(Sys_CheckParm("-forceqmenu"))
		Cvar_SetValueQuick(&forceqmenu,1);
	// use -useqmenu for debugging proposes, cause it starts
	// the normal quake menu only the first time
// COMMANDLINEOPTION: Client: -useqmenu causes the first time you open the menu to use the quake menu, then reverts to menu.dat (if forceqmenu is 0)
	if(Sys_CheckParm("-useqmenu"))
		MR_SetRouting (true);
	else
		MR_SetRouting (false);
}


	
	
		
		


