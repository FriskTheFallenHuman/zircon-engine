// menu_zform_connect.c.h

WARP_X_ (M_ZDev_Draw)

WARP_X_ (Object_Class_Default_Values)
// Return code is OK_NULL for ok otherwise a zalloced string that is freed on use.

typedef struct {
	oject_ref_s		*f;
	oject_ref_s		*ktab;
	oject_ref_s		*klist;
	oject_ref_s		*kentries;
	oject_ref_s		*kLblTitle;

	int				is_more_detail;
	stringlist_t	listaccum;
	int				any_error;			// Do nothing

	int				globals_cap;		// Because Baker used a metric mega-ton of globals in a mod.
} ezdev_s;

#pragma message ("Does r_restart call us")

ezdev_s _ezdev, *ezdev = &_ezdev;

void Map_Append_Pairs (stringlist_t *plist)
{
		stringlistappend2	(&ezdev->listaccum, "Map File", cl.worldname);
		stringlistappend2	(&ezdev->listaccum, "Title (message)", cl.worldmessage);
		stringlistappendf2	(&ezdev->listaccum, QUOTED_STR("sky"), g_skyname);
		stringlistappendf2	(&ezdev->listaccum, QUOTED_STR("fog"), "%f %f %f %f %f %f %f %f %f", r_refdef.fog_density, r_refdef.fog_red, r_refdef.fog_green, r_refdef.fog_blue, r_refdef.fog_alpha, r_refdef.fog_start, r_refdef.fog_end, r_refdef.fog_height, r_refdef.fog_fadedepth);
		stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

		if (sv.worldmodel) {
			ccs *stype = halflifebsp.integer ? "Half-Life 1" : sv_mapformat_is_quake2.integer ? "Quake 2" : sv_mapformat_is_quake3.integer ? "Quake 3" : String_Ends_With_Caseless (cl.worldname, ".obj") ? ".OBJ" : "Quake 1";
			stringlistappend2	(&ezdev->listaccum, "Server map format", stype);
			stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2
		}

		stringlistappendf2	(&ezdev->listaccum, "Supports wateralpha?", "%s", cl.worldmodel->brush.supportwateralpha ? "Yes" : "No");
		stringlistappendf2	(&ezdev->listaccum, "Has visibility data?", "%s", cl.worldmodel->brush.num_pvsclusters != 0 ? "Yes" : "No");

		va_super (levelshotname_jpg, MAX_QPATHX2_256, "levelshots/%s.jpg", cl.worldbasename);
		stringlistappendf2	(&ezdev->listaccum, "Has levelshots/mapname.tga?", "%s", cl.levelshotsname[0] ? cl.levelshotsname : "(No)");

		va_super (srt, MAX_QPATH_128, "maps/%s.rtlights", cl.worldbasename);
		int is_rts = FS_FileExists (srt);

		stringlistappendf2	(&ezdev->listaccum, "Using .rtlights?", "%s", is_rts ? srt : "(No)");

		va_super (entname, MAX_QPATH_128, "maps/%s.ent", cl.worldbasename);
		int is_ents = FS_FileExists (entname);
		
		if (cl.cdtrackname[0]) {
			stringlistappendf2	(&ezdev->listaccum, "Background music?", "%s", cl.cdtrackname);
		} else {
			if (cl.cdtrack) {
				va_super (trackname, MAX_QPATH_128, "music/track%02u.ogg", cl.cdtrack);
				stringlistappendf2	(&ezdev->listaccum, "Background music?", "(No .. it would be %s)", trackname);
			} else {
				stringlistappend2	(&ezdev->listaccum, "Background music?", "(No and map specifies sounds 0 for none)");
			}
		}

		stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

		char *entities_a = (char *)FS_LoadFile(entname, tempmempool, fs_quiet_true, fs_size_ptr_null);
		char *entities = AorB (entities_a, cl.worldmodel->brush.entities);

		stringlistappendf2	(&ezdev->listaccum, "Worldspawn keys:", "[from %s]", entities_a ? entname : cl.worldname);

		stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

		// appends key, value to list
		String_Worldspawn_Values_stringlistappend (&ezdev->listaccum, entities);
		Mem_FreeNull_ (entities_a);

		stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

		prvm_prog_t *prog = SVVM_prog;
		if (prog) {
			int enable_intermap = 0;
			int v_enable_intermap_offset = PRVM_ED_FindGlobalOffset(prog, "enable_intermap");
			if (v_enable_intermap_offset >= 0) {
				enable_intermap = PRVM_GLOBALFIELDFLOAT(v_enable_intermap_offset);
			} // if

			stringlistappendf2	(&ezdev->listaccum, "QC enable_intermap", "%d", enable_intermap);
			stringlistappendf2	(&ezdev->listaccum, "Intermap # Save map states", "# SIVS = %d", sv_intermap_siv_list.numstrings);
			stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

			if (enable_intermap) {
				stringlistappendf2	(&ezdev->listaccum, "+ QC time", "%f", sv.time);
				stringlistappendf2	(&ezdev->listaccum, "- QC surplustime", "%f", sv.intermap_surplustime);
				stringlistappendf2	(&ezdev->listaccum, "+ QC totaltimeatstart (of map)", "%f", sv.intermap_totaltimeatstart);
				
				stringlistappend2	(&ezdev->listaccum, "", "================");
				stringlistappendf2	(&ezdev->listaccum, "All levels time", "%f", (sv.time - sv.intermap_surplustime) + sv.intermap_totaltimeatstart );
				
				stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

				//Con_PrintLinef ("enable_intermap  = %d", have_intermap);
				stringlistappendf2	(&ezdev->listaccum, "QC startspot",  QUOTED_S, sv.intermap_startspot );
				stringlistappendf2	(&ezdev->listaccum, "QC startorigin",  VECTOR3_5d1F, VECTOR3_SEND(sv.intermap_startorigin) );

				stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2

				stringlistappendf2	(&ezdev->listaccum, "sv.was_intermap_loaded_from_siv", "%d", sv.was_intermap_loaded_from_siv);
				stringlistappendf2	(&ezdev->listaccum, "total time at last map exit", "%f", sv.intermap_totaltimeatlastexit);

				stringlistappend2	(&ezdev->listaccum, "", ""); // DIV2
				
				if (sv_intermap_siv_list.numstrings) {
					stringlistappend2	(&ezdev->listaccum, "SIVS:", "");
					for (int idx = 0 ; idx < sv_intermap_siv_list.numstrings; idx += 2) {
						// .SIV data - Intermap saved game
						const char *sxy_map = sv_intermap_siv_list.strings[idx + 0];
						const char *sxy_siv = sv_intermap_siv_list.strings[idx + 1];
						stringlistappendf2	(&ezdev->listaccum, sxy_map, "idx %3d strlen %d", idx, (int)strlen(sxy_siv) );
					} // for
				}		
				else {
					stringlistappend2	(&ezdev->listaccum, "No SIVS", "");
				}
			} // enable_intermap
		} // prog


}



WARP_X_ (CL_Models_Query)
qbool EZDev_Models_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] == NULL_CHAR_0)
		return false;

	// No detail then don't do worldmap brush models like *40 (door or such)
	if (ezdev->is_more_detail == false)
		if (key[0] == '*')
			return false;

	// 2 columns .. modelindex,modelname as key,n0
	int modelindex = n0;
	stringlistappendf (&ezdev->listaccum, "%d", modelindex);
	stringlistappendf (&ezdev->listaccum, key);

	return false; // Shall stop
}

WARP_X_ (PRVM_Fields_Query)
qbool EZDev_Fields_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	if (key[0] == NULL_CHAR_0)
		return false;

	//CREATE TABLE CUSTOMERS ( ID INT NOT NULL, NAME VARCHAR (20) NOT NULL, AGE INT NOT NULL, ADDRESS CHAR (25), SALARY DECIMAL (18, 2), PRIMARY KEY (ID) );
	// classname, origin, model?
	ccs *vartype_str = VarType_For_EV (n0);

	//va_super (sline, 1024,
	// S_FMT_LEFT_PAD_20 " %4d t: %4d" S_FMT_LEFT_PAD_40,
	//	key, idx, (int)n1, vartype_str);
	//stringlistappend (&mz->list, idx);
	stringlistappendf (&ezdev->listaccum, "%d", idx);
	stringlistappend (&ezdev->listaccum, key); // ?
	//stringlistappendf (&ezdev->listaccum, "%d", n1); // ?
	stringlistappend (&ezdev->listaccum, vartype_str);

	// What is our plan to get only the fields we want here?
	// Like column query ... hmmm ...
	// SQL let's you pick field

	return false; // Shall stop
}

WARP_X_ (PRVM_Fields_Query (SVVM_prog, ZDev_Globals_Feed_Shall_Stop_Fn))
qbool EZDev_Globals_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	int is_more_detail = true;
	if (is_more_detail == false) {
		if (n0 == ev_string_1 && strlen(value) == 0)
			return false; // Empty string
		if (value[0] == '0' && value[1] == NULL_CHAR_0)
			return false; // Value of 0
	}
	int count = ezdev->listaccum.numstrings / 3;
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	stringlistappendf (&ezdev->listaccum, "%d", count);
	stringlistappend (&ezdev->listaccum, key);
	stringlistappend (&ezdev->listaccum, value);

	if (ezdev->globals_cap && count >= ezdev->globals_cap) {
		stringlistappendf (&ezdev->listaccum, "%d", ezdev->globals_cap);
		stringlistappend (&ezdev->listaccum, "[GLOBALS CAP HIT]");
		stringlistappend (&ezdev->listaccum, "---");

		return true; // STOP!!!!!!!!!!!
	}
	return false; // Shall stop
}


WARP_X_ (PRVM_Entities_Query_Fieldname ZDev_Entities_Feed_Shall_Stop_Fn)

// Tricky: We will need to request:
// origin
// targetname
// model
qbool EZDev_Entities_Feed_Shall_Stop_Fn (prvm_prog_t *prog, int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	ccs *classname = key;
	// value is value
#if 0
	if (classname[0] == NULL_CHAR_0)
		return false;
#endif

	int edict_num = idx;
	//ccs *vartype_str = VarType_For_EV (n0);

	//va_super (sline, 1024, " %4d " S_FMT_LEFT_PAD_20, idx, /*key*/ value);
	//int modelindex = n0;
	stringlistappendf (&ezdev->listaccum, "%d", idx);
	//stringlistappend (&ezdev->listaccum, value);
	
	char *sxz = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "classname");
	char *s0z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "origin");
	char *s1z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "model");
	char *s2z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "frame");
	char *s3z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "targetname");
	char *s4z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "target");
	char *s5z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "noise1");
	char *s6z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "noise2");
	char *s7z = PRVM_Entities_Query_EdictNum_ZAlloc (prog, edict_num, "noise3");

//idx cla mod fm,  ori, tn,  n1, n2, n3
//150,300,250,100, 180, 180  180,180,180

	stringlistappend (&ezdev->listaccum, s0z ? sxz : ""); // Classname
	stringlistappend (&ezdev->listaccum, s0z ? s0z : ""); // Origin
	stringlistappend (&ezdev->listaccum, s1z ? s1z : ""); // Targetname
	stringlistappend (&ezdev->listaccum, s2z ? s2z : ""); // Model
	stringlistappend (&ezdev->listaccum, s3z ? s3z : ""); // Origin
	stringlistappend (&ezdev->listaccum, s4z ? s4z : ""); // Targetname
	stringlistappend (&ezdev->listaccum, s5z ? s5z : ""); // Noise1
	stringlistappend (&ezdev->listaccum, s6z ? s6z : ""); // Noise2
	stringlistappend (&ezdev->listaccum, s7z ? s7z : ""); // Noise3

	
	Z_FreeNull_ (sxz);
	Z_FreeNull_ (s0z);
	Z_FreeNull_ (s1z);
	Z_FreeNull_ (s2z);
	Z_FreeNull_ (s3z);
	Z_FreeNull_ (s4z);
	Z_FreeNull_ (s5z);
	Z_FreeNull_ (s6z);
	Z_FreeNull_ (s7z);

	return false; // Shall stop
}

WARP_X_ (R_WorldTextures_Query)
qbool EZDev_WorldTextures_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] != NULL_CHAR_0) {
		int ouridx = n0;
		stringlistappendf (&ezdev->listaccum, "%d", ouridx);
		stringlistappendf (&ezdev->listaccum, key);
	}
	return false; // Shall stop
}

WARP_X_ ( Mod_Shaders_Query)
qbool EZDev_Shaders_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c,
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] != NULL_CHAR_0) {
		int count = ezdev->listaccum.numstrings / 2;
		stringlistappendf (&ezdev->listaccum, "%d", count);
		stringlistappend (&ezdev->listaccum, key);
	}
	return false; // Shall stop
}

WARP_X_ (Form_Event_Onload)
EXO___ char *DevTabSelectOnChange (oject_s *ktab)
{
	int iserr = 0;
	char *sfile = NULL;
	if (ezdev->any_error) return NULL;

	Form_Get (f, ktab);;
	int idx = ktab->selectedindex;

	// Property Set
	Z_StrDup_Realloc_ (ezdev->kLblTitle->caption_a,
		ezdev->ktab->list_strings_a.strings[idx]);

	// CLEAR LISTVIEW -- Object_Method_Clear
	stringlistfreecontents (&ezdev->klist->list_strings_a); //Object_Method_Clear
	ezdev->klist->selectedindex = 0;
	Lister_SetPctHorz (ezdev->klist, 0, ezdev->klist->servo.kscrollx);
	if (ezdev->klist->servo.kscrollx->value) {
		ezdev->klist->servo.kscrollx->value = 0;
	}
	Lister_SetPct (ezdev->klist, 0, ezdev->klist->servo.kscrolly);
	if (ezdev->klist->servo.kscrolly->value) {
		ezdev->klist->servo.kscrolly->value = 0;
	}
	// Scrollbar thumbs, still wrong
	Form_QueueRefresh (f);

	// TABSELECT
	iserr += Object_Property_Set
		(ezdev->ktab, "List",
			"Entities,Models,Sounds,Textures,Shaders,"
			"Globals,Fields,Map,Environment,"
			"CSQC Ents,CSQC Globals,CSQC Fields,quake.rc,default.cfg,autoexec.cfg,config.cfg,BSP .Entities,rtlights"
		);

	// Query prep
	stringlistfreecontents (&ezdev->listaccum);

	int numcolumns = 1;
	int numrows = 0;

	switch (idx) {
	case lw_entities_0: // Server Entities
	case lw_globals_5:
	case lw_fields_6:

		if (SVVM_prog->loaded == false) {
			numcolumns = 2;
			iserr += Object_Property_Set	(ezdev->klist, "ColumnWidths", "150,600");
			iserr += Object_Property_Set	(ezdev->klist, "ColumnHeaders", "No Data");
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No server progs loaded (map not running)");
			goto nolistpossible;
		}
		break;

	case lw_textures_3:

		if (!r_refdef.scene.worldmodel) {
			numcolumns = 2;
			iserr += Object_Property_Set	(ezdev->klist, "ColumnWidths", "150,600");
			iserr += Object_Property_Set	(ezdev->klist, "ColumnHeaders", "No Data");
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No world model for textures (map not running)");
			goto nolistpossible;
		}
		break;

	case lw_map_7:
	case lw_map_ents_16:
	case lw_rtlights_17:

		if (!cl.worldmodel) {
			numcolumns = 2;
			iserr += Object_Property_Set	(ezdev->klist, "ColumnWidths", "150,600");
			iserr += Object_Property_Set	(ezdev->klist, "ColumnHeaders", "No Data");
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No client world model (map not running)");
			goto nolistpossible;
		}
		break;

	case lw_csqc_ents_9:
	case lw_csqc_globals_10:
	case lw_csqc_fields_11:

		if (CLVM_prog->loaded == false) {
			numcolumns = 2;
			iserr += Object_Property_Set	(ezdev->klist, "ColumnWidths", "150,600");
			iserr += Object_Property_Set	(ezdev->klist, "ColumnHeaders", "No Data");
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No csqc progs running");
			goto nolistpossible;
		}
		break;

	} // sw

	switch (idx) {
	default:
		{
			//int j = 5;
		}
		break;

	case lw_entities_0: // Server Entities

		numcolumns = 10;

		PRVM_Entities_Query_Fieldname (SVVM_prog, EZDev_Entities_Feed_Shall_Stop_Fn, "classname");

		iserr += Object_Property_Set (ezdev->klist, "ColumnWidths",
			"150,250,200,190,90,180,180,180,180,180");
		iserr += Object_Property_Set (ezdev->klist, "ColumnHeaders",
			"Index,Classname,Origin,Model,Frame,Targetname,Target,Noise1,Noise2,Noise3");

		break;

	case lw_models_1: // Models

		numcolumns = 2;

		CL_Models_Query (EZDev_Models_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150, 400");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "ModelIndex,Model");

		if (ezdev->listaccum.numstrings == 0) {
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No precache model list");
		}
		break;

	case lw_sounds_2: // Sounds

		numcolumns = 2;

		CL_Sounds_Query (EZDev_Models_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150, 400");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "SoundIndex,Sound");
		if (ezdev->listaccum.numstrings == 0) {
			stringlistappend (&ezdev->listaccum, "");
			stringlistappend (&ezdev->listaccum, "No precache sound list");
		}
		break;

	case lw_textures_3: // Textures

		numcolumns = 2;

		R_WorldTextures_Query (EZDev_WorldTextures_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,600");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,World Texture");
		break;

	case lw_shaders_4:

		numcolumns = 2;

		Mod_Shaders_Query (EZDev_Shaders_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,600");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,Shader");
		break;

	case lw_globals_5:

		numcolumns = 3;

		PRVM_Globals_Query (SVVM_prog, EZDev_Globals_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,274,200");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,Global Variable,Value");
		break;

	case lw_fields_6:

		numcolumns = 3;

		PRVM_Fields_Query (SVVM_prog, EZDev_Fields_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,274,200");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,FieldName,VarType");
		break;

	case lw_map_7:

		numcolumns = 2;

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "300,600");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", ",");

		Map_Append_Pairs (&ezdev->listaccum);

		break;

	case lw_environment_8:

		numcolumns = 2;

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "300,600");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", ",");

		// path
		// basedir

		stringlistappend	(&ezdev->listaccum,  "Base game:");
		stringlistappendf	(&ezdev->listaccum,  gamedirname1);

		stringlistappend	(&ezdev->listaccum,  "");
		stringlistappend	(&ezdev->listaccum,  "");

		stringlistappend	(&ezdev->listaccum,  "Current directory:");
		stringlistappendf	(&ezdev->listaccum,  Sys_Getcwd_SBuf());

		stringlistappend	(&ezdev->listaccum,  "");
		stringlistappend	(&ezdev->listaccum,  "");

		void FS_Path_Feed (stringlist_t *plist);
		stringlistappend	(&ezdev->listaccum,  "Path:");
		stringlistappend	(&ezdev->listaccum,  "");
		FS_Path_Feed (&ezdev->listaccum);

		break;

	case lw_csqc_ents_9:

		numcolumns = 10;

		PRVM_Entities_Query_Fieldname (CLVM_prog, EZDev_Entities_Feed_Shall_Stop_Fn, /*field*/ NULL);

		iserr += Object_Property_Set (ezdev->klist, "ColumnWidths",
			"150,250,200,190,90,180,180,180,180,180");
		iserr += Object_Property_Set (ezdev->klist, "ColumnHeaders",
			"Index,Classname,Origin,Model,Frame,Targetname,Target,Noise1,Noise2,Noise3");

		// We need to do 10 columns
		if (ezdev->listaccum.numstrings == 0) {
			stringlistappend (&ezdev->listaccum, ""); // 0
			stringlistappend (&ezdev->listaccum, "csqc shows running but no csqc entities"); // 1
			stringlistappend (&ezdev->listaccum, ""); // 2
			stringlistappend (&ezdev->listaccum, ""); // 3
			stringlistappend (&ezdev->listaccum, ""); // 4
			stringlistappend (&ezdev->listaccum, ""); // 5
			stringlistappend (&ezdev->listaccum, ""); // 6
			stringlistappend (&ezdev->listaccum, ""); // 7
			stringlistappend (&ezdev->listaccum, ""); // 8
			stringlistappend (&ezdev->listaccum, ""); // 9
		}
		break;

	case lw_csqc_globals_10:

		numcolumns = 3;

		PRVM_Globals_Query (CLVM_prog, EZDev_Globals_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,274,200");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,Global Variable,Value");
		break;

	case lw_csqc_fields_11:

		numcolumns = 3;

		PRVM_Fields_Query (CLVM_prog, EZDev_Fields_Feed_Shall_Stop_Fn);

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "150,274,200");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Index,FieldName,VarType");
		break;


#define DOFILE(SFILEHERE) \
		numcolumns = 2; \
		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "70,600");	 \
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Line,Text"); \
		if (!stringlistappendfilelines_did_load(&ezdev->listaccum, SFILEHERE, /*numcol?*/ true)) { \
			stringlistappend (&ezdev->listaccum, ""); \
			stringlistappendf (&ezdev->listaccum, "%s not found", SFILEHERE); \
		} // Ender

	case lw_quake_rc_12:

		DOFILE("quake.rc");
		break;

	case lw_default_cfg_13:

		DOFILE("default.cfg");
		break;

	case lw_autoexec_cfg_14:

		DOFILE("autoexec.cfg");
		break;

	case lw_config_cfg_15:

		DOFILE("config.cfg");
		break;

	case lw_map_ents_16:

		numcolumns = 2;

		iserr += Object_Property_Set		(ezdev->klist, "ColumnWidths", "70,600");
		iserr += Object_Property_Set		(ezdev->klist, "ColumnHeaders", "Line,Text");

		{
			
			va_super (entname, MAX_QPATH_128, "%s.ent", cl.worldnamenoextension);
			char *entities_alloc = (char *)FS_LoadFile(entname, tempmempool, fs_quiet_true, fs_size_ptr_null);
			int slenplus1 = strlen(entities_alloc) + ONE_CHAR_1;
			String_Edit_Replace_Memory_Constant (entities_alloc, slenplus1, "\r", "");
			char *entities = entities_alloc ? entities_alloc : cl.worldmodel->brush.entities;
			//int stringlistappend_split_delimiter_linenumbers (stringlist_t *plist, ccs *file, ccs *delimiter)
			stringlistappend_split_delimiter_linenumbers (&ezdev->listaccum, entities, NEWLINE);


			Mem_FreeNull_ (entities_alloc);
		}
		break;

	case lw_rtlights_17:
	
		{
			va_super (rtlightsfilename, MAX_QPATH_128, "maps/%s.rtlights", cl.worldbasename);
			DOFILE(rtlightsfilename);
		}
		break;
	} // sw

nolistpossible:

	iserr += Object_Property_Set_Fmt	(ezdev->klist, "ColumnCount", "%d", numcolumns);
	numrows = ezdev->listaccum.numstrings / numcolumns;

#if 0
	char *s_z = stringlist_join_delim_zalloc (&ezdev->listaccum, ",", fs_quoted_false);
	// Anything with commas gets busted
	iserr += Object_Property_Set		(ezdev->klist, "List", s_z);
#else
	{
		// stringlistfreecontents (&ezdev->klist->list_strings_a);
		// ALREADY DONE
		stringlistappendlist (&ezdev->klist->list_strings_a, &ezdev->listaccum);
	}
#endif
	iserr += Object_Property_Set_Fmt	(ezdev->kentries, "Caption", "%d Entries", numrows);

	stringlistfreecontents (&ezdev->listaccum);

#if 0
	Z_FreeNull_ (s_z);
#endif
	return OK_NOERROR_NULL;
}

WARP_X_ (Form_Event_Onload)
EXO___ char *DevInit (oject_s *f)
{
	if (ezdev->listaccum.numstrings)
		stringlistfreecontents (&ezdev->listaccum);

	memset (&_ezdev, 0, sizeof(_ezdev));
	ezdev->f	 = f;
	ezdev->ktab = Object_Find_Name (f, "TabSelect1"); if (!ezdev->ktab) return Z_StrDup ("Couldn't find TabSelect1");
	ezdev->klist = Object_Find_Name (f, "ListView1"); if (!ezdev->klist) return Z_StrDup ("Couldn't find ListView1");
	ezdev->kentries = Object_Find_Name (f, "LabelEntries"); if (!ezdev->kentries) return Z_StrDup ("Couldn't find LabelEntries");
	ezdev->kLblTitle = Object_Find_Name (f, "LblTitle");
	ezdev->globals_cap = 2048;
	if (!ezdev->kLblTitle)
		return Z_StrDup ("Couldn't find LblTitle");

	WARP_X_ (feed)
	int iserr = 0;

	return OK_NOERROR_NULL;

	//TabSelect1 ...
//	List ... "Entities,Models,Sounds,Textures,Shaders,Globals,Fields,Map,Environment,CSQC Entities,CSQC Globals,CSQC Fields, Zircon Extras"
}

//void DevIndexChange (oject_s *ktabsel)
//{
//	//int index = ktabsel->selectedindex;
//
//
//}




