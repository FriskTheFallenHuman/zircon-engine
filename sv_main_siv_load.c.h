// sv_main_siv_load.c.h

qbool SV_SpawnServer_Intermap_SIV_Is_Ok (const char *mapshortname, const char *s_sivdata, const char *s_startspot, const vec3_t startorigin, float totaltimeatstart) // Q2X
{
int is_ok;
func_t foff;
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ent;
	int i;
	model_t *worldmodel;
	char modelname[sizeof(sv.worldname)];

	WARP_X_ (SV_Loadgame_from)
	#pragma message (".siv open load file")

	const char *s_load_game_contents = s_sivdata;

	if (!s_load_game_contents)
		return false; // FAIL -- couldn't open file.

	Cvar_SetValueQuick	(&sv_freezenonclients, 0); // Baker r0090: freezeall

#ifdef CONFIG_MENU
	// Baker: What is cl equivalent?
	if (cls.state != ca_dedicated) {
		Cvar_SetValueQuick	(&tool_inspector, 0); // Baker r0106: tool inspector
		Cvar_SetValueQuick	(&tool_marker, 0); // Baker r0109: tool marker
	}
#endif

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_SetQuick (&hostname, "UNNAMED");
	scr_centertime_off = 0;


	Cvar_SetQuick		(&prvm_sv_gamecommands, "");  // Baker r7103 gamecommand autocomplete
	Cvar_SetQuick		(&prvm_sv_progfields, "");  // Baker r7103 gamecommand autocomplete

//
// make cvars consistant
//

	if (coop.integer) {
		// Baker: At least notify what is happening ...
		// Yes standard Quake will zero out deathmatch with coop 1 also ...
		Con_PrintLinef (QUOTED_STR("coop") " is " QUOTED_S ", setting deathmatch 0", coop.string);
		Cvar_SetValueQuick (&deathmatch, 0);
		Cvar_SetValueQuick (&campaign, 0);
	}
	else if (!deathmatch.integer)
		Cvar_SetValueQuick (&campaign, 1);
	else
		Cvar_SetValueQuick (&campaign, 0);
	// LadyHavoc: it can be useful to have skills outside the range 0-3...
	//current_skill = bound(0, (int)(skill.value + 0.5), 3);
	//Cvar_SetValue ("skill", (float)current_skill);
	current_skill = (int)(skill.value + 0.5);

	Con_DPrintLinef ("SpawnServer: %s", mapshortname);

	// Baker: a memset 0 occurs several lines down memset (&sv, 0, sizeof(sv)); @ line 1753

	c_dpsnprintf1 (modelname, "maps/%s.bsp", mapshortname);

	if (!FS_FileExists(modelname)) {
		c_dpsnprintf1 (modelname, "maps/%s", mapshortname);
		if (!FS_FileExists(modelname)) {
			Con_PrintLinef ("SpawnServer: no map file named %s", modelname);
			return false;
		}
	} // if

//	SV_LockThreadMutex();

	if (!host_isclient.integer)
		Sys_MakeProcessNice();
	else {
		SCR_BeginLoadingPlaque(false);
		S_StopAllSounds();
	}

	if (sv.active) {
		// Baker: From Doombringer ...
		client_t *client;
		for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
		{
			if (client->netconnection)
			{
				MSG_WriteByte(&client->netconnection->message, svc_stufftext);
				MSG_WriteString(&client->netconnection->message, "reconnect"  NEWLINE);
			}
		}
		World_End(&sv.world);
		if (PRVM_serverfunction(SV_Shutdown)) {
			func_t s = PRVM_serverfunction(SV_Shutdown);
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_serverfunction(SV_Shutdown) = 0; // prevent it from getting called again
			prog->ExecuteProgram(prog, s,"SV_Shutdown() required");
		}
	}

	// free q3 shaders so that any newly downloaded shaders will be active
	Mod_FreeQ3Shaders();

	worldmodel = Mod_ForName (modelname, false, developer.integer > 0, NULL);
	if (!worldmodel || !worldmodel->TraceBox) {
		Con_PrintLinef ("Couldn't load map %s", modelname);

		if (!host_isclient.integer)
			Sys_MakeProcessMean();

//		SV_UnlockThreadMutex();
		return false;
	}

	Collision_Cache_Reset (true);

	svs.changelevel_issued = false;		// now safe to issue another

	// make the map a required file for clients
	Curl_ClearRequirements();
	Curl_RequireFile(modelname);

//
// tell all connected clients that we are going to a new level
//
	if (!sv.active) {
		// open server port
		NetConn_OpenServerPorts(true);
	}


//
// set up the new server
//

	memset (&sv, 0, sizeof(sv));

	// tell SV_Frame() to reset its timers
	sv.spawnframe = host.superframecount;
	sv.zirconprotcolextensions_sv = sv_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0;
	if (sv_players_walk_thru_players.integer == 0)
		Flag_Remove_From (sv.zirconprotcolextensions_sv, ZIRCON_EXT_WALKTHROUGH_PLAYERS_IS_ACTIVE_128);
	if (sv_allow_zircon_move.integer /*d: 1*/) {
		Con_DPrintLinef ("Server allows Zircon Free Movement, setting sv_cullentities_nevercullbmodels 1");
		Cvar_SetValueQuick (&sv_cullentities_nevercullbmodels, 1);
		Cvar_SetValueQuick (&sv_clmovement_soundreliable, 1);
	} else {
		Flag_Remove_From (sv.zirconprotcolextensions_sv, ZIRCON_EXT_FREEMOVE_4);
	}

	// if running a local client, make sure it doesn't try to access the last
	// level's data which is no longer valiud
	cls.signon = SIGNON_ZERO; // SV_SPAWNSERVER - Baker: interesting

	// Baker: How does it know brush already?  Set above worldmodel
	Cvar_SetValueQuick (&halflifebsp, worldmodel->brush.ishlbsp);
	Cvar_SetValueQuick (&sv_mapformat_is_quake2, worldmodel->brush.isq2bsp);
	Cvar_SetValueQuick (&sv_mapformat_is_quake3, worldmodel->brush.isq3bsp);

	if (*sv_random_seed.string) {
		srand(sv_random_seed.integer);
		Con_PrintLinef (CON_WARN "NOTE: random seed is %d; use for debugging/benchmarking only!\nUnset sv_random_seed to get real random numbers again.", sv_random_seed.integer);
	}

	// Baker: Moved this up

	// set level base name variables for later use
	c_strlcpy (sv.name, mapshortname);
	c_strlcpy (sv.intermap_startspot, s_startspot); // .CROSS startspot SV_SpawnServer_Intermap_SIV_Is_Ok
	VectorCopy (startorigin, sv.intermap_startorigin); // .CROSS first enter map "SV_SpawnServer"
	sv.intermap_totaltimeatstart = totaltimeatstart;
	sv.intermap_totaltimeatlastexit = 999999; // We don't know yet ...
	sv.intermap_surplustime = 999999; // We don't know yet ...

	sv.was_intermap_loaded_from_siv = true;

	c_strlcpy (sv.worldname, modelname);
	FS_StripExtension(sv.worldname, sv.worldnamenoextension, sizeof(sv.worldnamenoextension));
	c_strlcpy (sv.worldbasename, String_Starts_With_PRE(sv.worldnamenoextension, "maps/"/*, 5*/) ?
		sv.worldnamenoextension + 5 : sv.worldnamenoextension);
	//Cvar_SetQuick(&sv_worldmessage, sv.worldmessage); // set later after QC is spawned
	Cvar_SetQuick(&sv_worldname, sv.worldname);
	Cvar_SetQuick(&sv_worldnamenoextension, sv.worldnamenoextension);
	Cvar_SetQuick(&sv_worldbasename, sv.worldbasename);

	const char *s_progs = sv_progs.string; // What about save game map load?
#if 0
	char *s_entities = NULL;
	int s_entities_was_allocated = false;


	if (is_intermap) {
		// Baker: intermap loads NO entities from the map
		goto intermap_no_load_entities;
	}
#endif



#if 0
	// Baker: Twice for now ... fix in future ...load replacement entity file if found
	if (sv_entpatch.integer &&
		(s_entities = (char *)FS_LoadFile(va(vabuf, sizeof(vabuf), "%s.ent", sv.worldnamenoextension), tempmempool, fs_quiet_true, fs_size_ptr_null))) {
		Con_PrintLinef ("Loaded %s.ent for sv_progs check", sv.worldnamenoextension);
		s_entities_was_allocated = true;
	}
	else
		s_entities = worldmodel->brush.entities;
#endif

#if 0000
	const char *s_sv_progs_for_map = String_Worldspawn_Value_For_Key_Sbuf (s_entities, "sv_progs");

	if (s_sv_progs_for_map) {
		Con_PrintLinef ("Map sv_progs key: " QUOTED_S " loading progs", s_sv_progs_for_map);
		s_progs = s_sv_progs_for_map;
	}
#endif

#if 0
	if (s_entities_was_allocated)
		Mem_Free(s_entities);

intermap_no_load_entities:
#endif

	// Baker: Problem ... we need worldspawn keys from .siv file
	// in particular the "sv_progs" "progs.day"

	SV_SpawnServer_VM_Setup (s_progs); // Progs init

	sv.active = true;

	sv.protocol = Protocol_EnumForName(sv_protocolname.string);
	if (sv.protocol == PROTOCOL_UNKNOWN_0) {
		char buffer[1024];
		Protocol_Names(buffer, sizeof(buffer));
		Con_PrintLinef (CON_ERROR "Unknown sv_protocolname " QUOTED_S ", valid values are:" NEWLINE "%s", sv_protocolname.string, buffer);
		sv.protocol = PROTOCOL_QUAKE;
	}

// load progs to get entity field count
	//PR_LoadProgs ( sv_progs.string );

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;

	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;

	sv.signon.maxsize = sizeof(sv.signon_buf);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buf;

// leave slots at start for clients only
	//prog->num_edicts = svs.maxclients+1;

	sv.state = ss_loading;
	prog->allowworldwrites = true;
	sv.paused = false; // .siv?

#if 0
	#pragma message (".siv sv.time = 1.0")
	sv.time = 1.0;
#endif

	Mod_ClearUsed();
	worldmodel->used = true;

	sv.worldmodel = worldmodel;
	sv.models[1] = sv.worldmodel;

//
// clear world interaction links
//
	World_SetSize	(&sv.world, sv.worldname, sv.worldmodel->normalmins, sv.worldmodel->normalmaxs, prog);
	World_Start		(&sv.world);

	c_strlcpy (sv.sound_precache[0], "");

	c_strlcpy(sv.model_precache[0], "");
	c_strlcpy(sv.model_precache[1], sv.worldname);
	for (i = 1; i < sv.worldmodel->brush.numsubmodels && i + 1 < MAX_MODELS_8192;i++) {
		c_dpsnprintf1 (sv.model_precache[i+1], "*%d", i);
		sv.models[i+1] = Mod_ForName (sv.model_precache[i+1], false, false, sv.worldname);
	}
	if (i < sv.worldmodel->brush.numsubmodels)
		Con_PrintLinef ("Too many submodels (MAX_MODELS_8192 is %d)", MAX_MODELS_8192);

//
// load the rest of the entities
//
	// AK possible hack since num_edicts is still 0
	ent = PRVM_EDICT_NUM(0);
	memset (ent->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
	ent->free = false;
	PRVM_serveredictstring	(ent, model) = PRVM_SetEngineString(prog, sv.worldname);
	PRVM_serveredictfloat	(ent, modelindex) = 1;		// world model
	PRVM_serveredictfloat	(ent, solid) = SOLID_BSP_4;
	PRVM_serveredictfloat	(ent, movetype) = MOVETYPE_PUSH_7;
	VectorCopy				(sv.world.mins, PRVM_serveredictvector(ent, mins));
	VectorCopy				(sv.world.maxs, PRVM_serveredictvector(ent, maxs));
	VectorCopy				(sv.world.mins, PRVM_serveredictvector(ent, absmin));
	VectorCopy				(sv.world.maxs, PRVM_serveredictvector(ent, absmax));

	if (coop.value)
		PRVM_serverglobalfloat(coop) = coop.integer;
	else
		PRVM_serverglobalfloat(deathmatch) = deathmatch.integer;

	PRVM_serverglobalstring(mapname) = PRVM_SetEngineString(prog, sv.name);

	int v_startspot_offset = PRVM_ED_FindGlobalOffset(prog, "startspot");
	if (v_startspot_offset >= 0) {
		PRVM_GLOBALFIELDSTRING(v_startspot_offset) = PRVM_SetEngineString(prog, sv.intermap_startspot);
	} // if

	int v_startorigin_offset = PRVM_ED_FindGlobalOffset(prog, "startorigin");
	if (v_startorigin_offset >= 0) {
		//VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
		VectorCopy(sv.intermap_startorigin, PRVM_GLOBALFIELDVECTOR(v_startorigin_offset) );
		//PRVM_GLOBALFIELDVECTOR(v_startspot_offset) = PRVM_SetEngineString(prog, sv.intermap_startorigin);
	} // if

	int v_totaltimeatstart_offset = PRVM_ED_FindGlobalOffset(prog, "totaltimeatstart");
	if (v_totaltimeatstart_offset >= 0) {
		//VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
		PRVM_GLOBALFIELDFLOAT(v_totaltimeatstart_offset) = sv.intermap_totaltimeatstart;
		//PRVM_GLOBALFIELDVECTOR(v_startspot_offset) = PRVM_SetEngineString(prog, sv.intermap_startorigin);
	} // if

	// intermap_map_set

// serverflags are for cross level information (sigils)
	PRVM_serverglobalfloat(serverflags) = svs.serverflags;

	// we need to reset the spawned flag on all connected clients here so that
	// their thinks don't run during startup (before PutClientInServer)
	// we also need to set up the client entities now
	// and we need to set the ->edict pointers to point into the progs edicts
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++) {
		host_client->begun = false;
		host_client->edict = PRVM_EDICT_NUM(i + 1);
		PRVM_ED_ClearEdict(prog, host_client->edict);
	}

major_major_major_major:
	// Baker: Load the entities from file.
WARP_X_ (SV_Loadgame_from)
#pragma message (".siv load from file")


loading_ents_here_and_the_time_etc:
	is_ok = SV_Loadgame_Intermap_Do_Ents (s_load_game_contents); // frees string
	
	int v_totaltimeatlastexit_offset = PRVM_ED_FindGlobalOffset(prog, "totaltimeatlastexit");
	if ( v_totaltimeatlastexit_offset >= 0) {
		//VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
		PRVM_GLOBALFIELDFLOAT(v_totaltimeatstart_offset) = sv.intermap_totaltimeatlastexit;
		//PRVM_GLOBALFIELDVECTOR(v_startspot_offset) = PRVM_SetEngineString(prog, sv.intermap_startorigin);
	} // if

	if (is_ok == false) {
		// Host erorr instead?
		Con_PrintLinef ("SpawnServer: no map file named %s", modelname);
		return false;
	}

//	edict 1 through maxclients how?

run_returntomap:
	WARP_X_ (SV_Savegame_f SV_Restart_f)
	PRVM_serverglobalfloat(time) = sv.time;
	sv.intermap_surplustime = sv.time;
	int v_surplustime_offset = PRVM_ED_FindGlobalOffset(prog, "surplustime");
	if ( v_surplustime_offset >= 0) {
		//VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
		PRVM_GLOBALFIELDFLOAT(v_surplustime_offset) = sv.intermap_surplustime;
		//PRVM_GLOBALFIELDVECTOR(v_startspot_offset) = PRVM_SetEngineString(prog, sv.intermap_startorigin);
	} // if

	foff = PRVM_ED_FindFunctionOffset(prog, "ReturnToMap");
	if (foff) {
		Con_DPrintLinef ("Calling ReturnToMap");
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(PRVM_EDICT_NUM(0));//PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, foff, "QC function ReturnToMap is missing");
	}

#if 0
	// load replacement entity file if found
	if (sv_entpatch.integer && (entities = (char *)FS_LoadFile(va(vabuf, sizeof(vabuf), "%s.ent", sv.worldnamenoextension), tempmempool, fs_quiet_true, fs_size_ptr_null))) {
		Con_PrintLinef ("Loaded %s.ent", sv.worldnamenoextension);
		PRVM_ED_LoadFromFile(prog, entities);
		Mem_Free(entities);
	}
	else
		PRVM_ED_LoadFromFile(prog, sv.worldmodel->brush.entities);
#endif

#if 1  // Baker r9067: loadgame precaches "precache at any time models and sounds"
	// Baker r9067: Only a host is client situation requires this fix.
	if (1 /*s_loadgame*/ && host_isclient.integer) {
		// read extended data if present
		// the extended data is stored inside a /* */ comment block, which the
		// parser intentionally skips, so we have to check for it manually here
#if 0
		#include "sv_main_precache.c.h"
#endif
	}
#endif

	// LadyHavoc: clear world angles (to fix e3m3.bsp)
	VectorClear(PRVM_serveredictvector(prog->edicts, angles));

// all setup is completed, any further precache statements are errors
//	sv.state = ss_active; // LadyHavoc: workaround for svc_precache bug
	prog->allowworldwrites = false;

#if 0

// run two frames to allow everything to settle
#pragma message (".siv sv.time = 1.0001")
	sv.time = 1.0001;
	for (i = 0;i < sv_init_frame_count.integer;i++) {
		sv.frametime = 0.1;
		SV_Physics ();
	}
#endif

	// Once all init frames have been run, we consider svqc code fully initialized.
	prog->inittime = host.realtime;

	if (!host_isclient.integer)
		Mod_PurgeUnused();

// create a baseline for more efficient communications
	if (isin8 (sv.protocol,		PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999,
		PROTOCOL_QUAKE,			PROTOCOL_QUAKEDP,		PROTOCOL_NEHAHRAMOVIE,
		PROTOCOL_NEHAHRABJP,	PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3
		))
		SV_CreateBaseline ();

	sv.state = ss_active; // LadyHavoc: workaround for svc_precache bug

// send serverinfo to all connected clients, and set up botclients coming back from a level change
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++) {
		host_client->clientconnectcalled = false; // do NOT call ClientDisconnect if he drops before ClientConnect!
		if (!host_client->active)
			continue;
		if (host_client->netconnection)
			SV_SendServerinfo(host_client);
		else {
			int j;
			// if client is a botclient coming from a level change, we need to
			// set up client info that normally requires networking

			// copy spawn parms out of the client_t
			for (j = 0 ; j < NUM_SPAWN_PARMS_16; j ++)
				(&PRVM_serverglobalfloat(parm1))[j] = host_client->spawn_parms[j];

			// call the spawn function
			host_client->clientconnectcalled = true;
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
			prog->ExecuteProgram(prog, PRVM_serverfunction(ClientConnect), "QC function ClientConnect is missing");
			prog->ExecuteProgram(prog, PRVM_serverfunction(PutClientInServer), "QC function PutClientInServer is missing");
			host_client->begun = true;
		} // if
	} // for host_client

	// update the map title cvar
	c_strlcpy (sv.worldmessage, PRVM_GetString(prog, PRVM_serveredictstring(prog->edicts, message)) ); // map title (not related to filename)
	Cvar_SetQuick(&sv_worldmessage, sv.worldmessage);

	Con_DPrintLinef ("Server spawned.");
	NetConn_Heartbeat (2);

	if (!host_isclient.integer)
		Sys_MakeProcessMean();

//	SV_UnlockThreadMutex();
	return true; // Is ok!
}
