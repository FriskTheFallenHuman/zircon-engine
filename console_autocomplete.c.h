// console_autocomplete.c.h



// Performed by:
WARP_X_ (Key_ClearEditLine if console which is called by Con_ToggleConsole)
// Con_ToggleConsole resets history to -1
// Partial reset needs to be performed in the console
// by damn near any key that is not TAB or shift TAB

// What keys should not reset autocomplete
// 1. TAB and shift TAB
// 2. ALT-TAB
// 3. Probably ALT-ENTER
// 4. Mousewheel scrolling through history
// 5. Setting zoom like CTRL-ZERO or CTRL-PLUS or CTRL-MINUS


autocomplete_t _g_autocomplete;

int GetSkyList_Count (const char *s_prefix)
{
	fssearch_t	*t;
	char		spattern[1024] = "gfx/env/*"; // Default if no prefix provided
	int			num_matches = 0;
	int			j;

	// Baker: if a prefix was provided use it literally
	// We hope it looks like "gfx/env/s"
	if (s_prefix && s_prefix[0]) {
		c_dpsnprintf1 (spattern, "%s*", s_prefix);
	}

	t = FS_Search(spattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
	if (t && t->numfilenames > 0) {
		num_matches = t->numfilenames;
		for (j = 0; j < t->numfilenames; j++) {
			char *sxy = t->filenames[j];
			File_URL_Edit_Remove_Extension (sxy);

			// We are using _rt .. what about the other crazy supported suffixes like "pz"
			// Nah .. we are doing Quake skyboxes only
			int slen = (int)strlen (sxy); // We know strlen >= 3
			if (String_Ends_With (sxy, "_rt")) {
				sxy[slen - 3] = 0;
			} else if (String_Ends_With (sxy, "rt")) {
				sxy[slen - 2] = 0;
			} else {
				// No trail of _rt or rt
				continue;
			}

			SPARTIAL_EVAL_

			num_matches ++;
		} // for
	} // if


	if (t) FS_FreeSearch(t);

	return num_matches;
}

int GetTexMode_Count (const char *s_prefix)
{
	// This list has to be alpha sorted for
	// autocomplete to work right
	const char *slist[] =  {
		"GL_LINEAR",
		"GL_LINEAR_MIPMAP_LINEAR",
		"GL_LINEAR_MIPMAP_NEAREST",
		"GL_NEAREST",
		"GL_NEAREST_MIPMAP_LINEAR",
		"GL_NEAREST_MIPMAP_NEAREST",
	};

	int	array_count = (int)ARRAY_COUNT(slist);
	int num_matches = 0;

	for (int idx = 0; idx < array_count; idx++) {
		const char *sxy =  slist[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

	} // idx
	return num_matches;
}


/*
GetMapList

Made by [515]
Prints not only map filename, but also
its format (q1/q2/q3/hl) and even its message
*/
//[515]: here is an ugly hack.. two gotos... oh my... *but it works*
//LadyHavoc: rewrote bsp type detection, rewrote message extraction to do proper worldspawn parsing
//LadyHavoc: added .ent file loading, and redesigned error handling to still try the .ent file even if the map format is not recognized, this also eliminated one goto
//LadyHavoc: FIXME: man this GetMapList is STILL ugly code even after my cleanups...

int m_maplist_count;
maplist_s m_maplist[MAXMAPLIST_4096];

// Baker r0086: Rewritten so .obj maps located in maps folder are listed like other maps.
qbool GetMapList (const char *s_partial, char *completedname,
	int completednamebufferlength, int is_menu_fill,
	int is_zautocomplete, int is_suppress_print)
{
	fssearch_t	*t_bsp;
	fssearch_t	*t_obj;
	char		s_pattern[1024];
	char		s_map_title[1024];
	int			j, k, our_max_length, p, partial_length, our_min_length;
	unsigned char *lengths_array;
	qfile_t		*f;
	unsigned char buf[1024];

	if (is_menu_fill) {
		m_maplist_count = 0;
	}

	c_dpsnprintf1 (s_pattern, "maps/%s*.bsp", s_partial);

	t_bsp = FS_Search(s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, is_menu_fill ? fs_gamedironly_true : fs_gamedironly_false);

	c_dpsnprintf1 (s_pattern, "maps/%s*.obj", s_partial);
	t_obj = FS_Search(s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, is_menu_fill ? fs_gamedironly_true : fs_gamedironly_false);

	if (!t_bsp && !t_obj)
		return 0;

	// Baker: Make list
	stringlist_t	maplist;

	stringlistinit	(&maplist);

	if (t_bsp) {
		fssearch_t	*pt = t_bsp;
		for (int idx = 0; idx < pt->numfilenames; idx++ ) {
			stringlistappend (&maplist, pt->filenames[idx]);
		} // for

		FS_FreeSearch(pt);
	} // if

	if (t_obj) {
		fssearch_t	*pt = t_obj;
		for (int idx = 0; idx < pt->numfilenames; idx++ ) {
			stringlistappend (&maplist, pt->filenames[idx]);
		} // for
		FS_FreeSearch (pt);
	} // if

	stringlistsort	(&maplist, fs_make_unique_true);

	if (maplist.numstrings > 1) {
		if (is_menu_fill == false && is_suppress_print == false)
			Con_PrintLinef (CON_BRONZE " %d maps found:", maplist.numstrings);
	}

	lengths_array = (unsigned char *)Z_Malloc(maplist.numstrings);
	our_min_length = 666;

	for (our_max_length = j = 0; j < maplist.numstrings; j++) {
		k = (int)strlen(maplist.strings[j]);
		k -= 9; // Why 9?  maps/ is 5, .bsp is 4
		if (our_max_length < k)
			our_max_length = k;
		else
		if (our_min_length > k)
			our_min_length = k;
		lengths_array[j] = k;
	}
	partial_length = (int)strlen(s_partial);

	for (j = 0; j < maplist.numstrings; j ++) {
		char *s_this_filename = maplist.strings[j];
		int lumpofs = 0, lumplen = 0;
		char *entities = NULL;
		const char *data = NULL;
		char keyname[64];
		char entfilename[MAX_QPATH_128];
		char desc[64];
		int map_format_code = 0;
		int is_playable = false;
		desc[0] = 0;
		int is_obj_map = String_Ends_With (s_this_filename, ".obj");

		c_strlcpy (s_map_title, CON_RED "ERROR: open failed" CON_WHITE);

		p = 0;
		f = FS_OpenVirtualFile(s_this_filename, fs_quiet_true);

		if (f) {
			c_strlcpy (s_map_title, CON_RED "ERROR: not a known map format" CON_WHITE);

			memset(buf, 0, 1024);
			FS_Read(f, buf, 1024);
			//#pragma message ("Baker: It is said that .bsp that are .md3
			//or such bypass requirement of info_player_start")
			if (is_obj_map) {
					c_strlcpy (desc, "OBJ");
					map_format_code = 7;
			} else if (!memcmp(buf, "IBSP", 4)) {
				p = LittleLong(((int *)buf)[1]);
				if (isin3 (p, Q3BSPVERSION_46, Q3BSPVERSION_LIVE_47, Q3BSPVERSION_IG_48)) {
					q3dheader_t *header = (q3dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q3LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q3LUMP_ENTITIES].filelen);
					c_dpsnprintf1 (desc, "Q3BSP%d", p);
					if (p == 47)		map_format_code = 8;
					else if (p == 48)	map_format_code = 9;
					else map_format_code = 3;
				}
				else if (p == Q2BSPVERSION) {
					q2dheader_t *header = (q2dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q2LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q2LUMP_ENTITIES].filelen);
					c_dpsnprintf1 (desc, "Q2BSP%d", p);
					map_format_code = 2;
				}
				else {
					c_dpsnprintf1 (desc, "IBSP%d", p);
					map_format_code = 4;
				}
			} else if (BuffLittleLong(buf) == BSPVERSION /*29*/) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP29");
				map_format_code = 1;
			} else if (BuffLittleLong(buf) == 30 /*Half-Life*/) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSPHL");
				map_format_code = -1;
			} else if (!memcmp(buf, "BSP2", 4)) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP2");
				map_format_code = 1;
			} else if (!memcmp(buf, "2PSB", 4)) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP2RMQe");
				map_format_code = 1;
			} else if (!memcmp(buf, "VBSP", 4)) {
				hl2dheader_t *header = (hl2dheader_t *)buf;
				lumpofs = LittleLong(header->lumps[HL2LUMP_ENTITIES].fileofs);
				lumplen = LittleLong(header->lumps[HL2LUMP_ENTITIES].filelen);
				int versi = LittleLong(((int *)buf)[1]);
				c_dpsnprintf1 (desc, "VBSP%d", versi);
				map_format_code = 5;
			} else {
				c_dpsnprintf1(desc, "unknown%d", BuffLittleLong(buf));
				map_format_code = -2;
			}
			c_strlcpy (entfilename, s_this_filename);
			memcpy (entfilename + strlen(entfilename) - 4, ".ent", 5);
			entities = (char *)FS_LoadFile(entfilename, tempmempool, fs_quiet_true, fs_size_ptr_null);

			if (entities == NULL && lumplen >= 10) {
				FS_Seek(f, lumpofs, SEEK_SET);
				entities = (char *)Z_Malloc(lumplen + 1);
				FS_Read(f, entities, lumplen);
			}
			if (entities) {
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				s_map_title[0] = 0;
				is_playable = String_Contains (entities, "info_player_start") ||
					String_Contains (entities, "info_player_deathmatch");

				data = entities;
				for (;;) {
					int this_len;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (com_token[0] == '{')
						continue;
					if (com_token[0] == '}')
						break;

					// skip leading whitespace
					for (k = 0; com_token[k] && ISWHITESPACE(com_token[k]);k++);
					for (this_len = 0; this_len < (int)sizeof(keyname) - 1 && com_token[k+this_len] && !ISWHITESPACE(com_token[k+this_len]);this_len++) {
						keyname[this_len] = com_token[k + this_len];
					} // for
					keyname[this_len] = 0;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (developer_extra.integer)
						Con_DPrintLinef ("key: %s %s", keyname, com_token);
					if (String_Match(keyname, "message")) {
						// get the map title
						c_strlcpy (s_map_title, com_token);
						break;
					} // if "message"
				} // for
			} // if entities
		}
		if (entities)
			Z_Free(entities);
		if (f)
			FS_Close(f);

		if (is_obj_map == false) {
			*(s_this_filename + lengths_array[j] + 5) = NULL_CHAR_0; // Strips extension
		}

		// Ignore unplayable map
		if (!is_playable)
			continue;

		if (is_zautocomplete) {
			const char *sxy = s_this_filename + 5;

			SPARTIAL_EVAL_
		}

		if (is_menu_fill == false && is_suppress_print == false) {
			// If we print, print only maps with spawnpoints
			// This should avoid healthboxes and such
			//if (is_zautocomplete == false) {
				// Zircon autocomplete does not print here on 2nd autocomplete
				// For example, press TAB it prints
				// Press TAB again, it does not
				Con_PrintLinef ("%16s (%-8s) %s", s_this_filename + 5, desc, s_map_title);
			//}
		} else {
			// Map fill
			if (m_maplist_count < (int)ARRAY_COUNT(m_maplist) /* map with no title should still list */ /*&& s_map_title[0]*/) {
				// Baker: Explanation ...
				// The maps menu column 1 is "E1M1" we limit this to 15 characters, hence buffer size of 16
				// The maps title column is limited to 27 characters
				// However, we need the full filename after "maps/" in case we truncated
				//
				char s_file_trunc_at_16[16];
				char s_title_trunc_at_28[28];

				c_strlcpy (s_file_trunc_at_16, s_this_filename + 5);
				c_strlcpy (s_title_trunc_at_28, s_map_title);

				maplist_s *mx = &m_maplist[m_maplist_count];

				freenull_ (mx->s_name_after_maps_folder_a);
				freenull_ (mx->s_name_trunc_16_a);
				freenull_ (mx->s_map_title_trunc_28_a);

				mx->s_name_after_maps_folder_a	= (unsigned char *)strdup	(s_this_filename + 5);
				mx->s_name_trunc_16_a			= (unsigned char *)strdup	(s_file_trunc_at_16);
				mx->s_map_title_trunc_28_a		= (unsigned char *)strdup	(s_title_trunc_at_28);
				mx->s_bsp_code					= (unsigned char *) (
					isin1 (map_format_code, 8) ? "QL" :
					isin1 (map_format_code, 9) ? "IG" :
					isin2 (map_format_code, 3,4) ? "Q3" :
					isin1 (map_format_code, 7)	  ? "OBJ" :
							"");
				m_maplist_count ++;
			} // if m_maplist_count < max
		} // if menu fill
	} // for numfilenames
	if (is_menu_fill == false && is_suppress_print == false)
		Con_Print("\n");

	for (p = partial_length; p < our_min_length; p ++)
	{
		k = *(maplist.strings[0] + 5 + p);
		if (k == 0)
			goto endcomplete;
		for (j = 1; j < maplist.numstrings; j++)
			if (*(maplist.strings[j] + 5 + p) != k)
				goto endcomplete;
	}
endcomplete:
	if (p > partial_length && completedname && completednamebufferlength > 0) {
		memset(completedname, 0, completednamebufferlength);
		memcpy(completedname, (maplist.strings[0] + 5), Smallest(p, completednamebufferlength - 1));
	}
	Z_Free(lengths_array);

	stringlistfreecontents (&maplist);

	return p > partial_length;
}

int GetVideoList_Count (const char *s_prefix)
{
	// Get any .dpv
	// Get any _fps folders
	// Get any .gif
	char		s_pattern[1024];

	stringlist_t list = {0};
	fssearch_t	*t = NULL;

	// .gif
	c_dpsnprintf1 (s_pattern, "video/%s*.gif", s_prefix);
	t = FS_Search (s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
	stringlistappendfssearch (&list, t);
	FS_FreeSearch_Null_ (t);

	// .dpv
	c_dpsnprintf1 (s_pattern, "video/%s*.dpv", s_prefix);
	t = FS_Search (s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);
	stringlistappendfssearch (&list, t);
	FS_FreeSearch_Null_ (t);

	// Baker: .jpeg ... I want the autocomplete to work with the files in a .pak
	// However, .pk3 and .pak remove the directories information on load.
	// So this will not autocomplete from pk3 or pak
	// "playvideo" will work fine
	{
		stringlist_t all_contents_of_video_folder_list = {0};

#if 1
		char gamedir_slash_video_slash[MAX_OSPATH_EX_1024]; // id1/video/
		c_dpsnprintf2 (gamedir_slash_video_slash, "%s%s", fs_gamedir /* "id1/" */ , "video/" );
		// game pathos tends to be "id1/" here
		// game pathos tends to be "id1/progs/"

		// Baker: I totally don't trust this at this time.
		// This returns file names without the path
		listdirectory	(&all_contents_of_video_folder_list, gamedir_slash_video_slash /* id1/video/ */, "");

		//stringlist_condump (&all_contents_of_video_folder_list);

		stringlist_t fps_list = {0};
		stringlist_t *plist = &all_contents_of_video_folder_list;
		for (int idx = 0; idx < plist->numstrings; idx++) {
			char *sxy = plist->strings[idx];

			if (false == String_Ends_With (sxy, "_fps"))
				continue;

			if (false == String_Starts_With (sxy, s_prefix))
				continue;

			// Baker: The autocomplete doesn't want the full name just the partial

			char nameout[MAX_OSPATH_EX_1024];
			c_strlcpy (nameout, "video/");
			c_strlcat (nameout, sxy);
			stringlistappend (&fps_list, nameout);
		} // for
		//stringlist_condump (&fps_list);

		stringlistappendlist (&list, &fps_list);

		//
		stringlistfreecontents (&all_contents_of_video_folder_list);
		stringlistfreecontents (&fps_list);

#endif

	}
	stringlistsort (&list, fs_make_unique_true);

	int num_matches = 0;

	// Baker: We want to keep the extension
	// Baker: We want to remove "video\" prefix
	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *sxy = list.strings[idx];

		// Remove video/ prefix
		sxy += STRINGLEN("video/");

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents (&list);

	return num_matches;
}

int GetFileList_Count (const char *s_folder_with_slash_or_null, const char *s_prefix, const char *s_dot_extension, int is_strip_extension)
{
	fssearch_t	*t;
	char		s_pattern[1024];
	int			num_matches = 0;
	int			j;

	if (s_folder_with_slash_or_null)
		c_dpsnprintf3 (s_pattern, "%s%s*%s", s_folder_with_slash_or_null, s_prefix, s_dot_extension);
	else
		c_dpsnprintf2 (s_pattern, "%s*%s", s_prefix, s_dot_extension);

	t = FS_Search (s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (t && t->numfilenames > 0) {
		int array_count = t->numfilenames;

		for (j = 0; j < array_count; j ++) {
			char *sxy = t->filenames[j];

			if (is_strip_extension)
				File_URL_Edit_Remove_Extension (sxy);

			if (s_folder_with_slash_or_null)
				sxy += strlen (s_folder_with_slash_or_null);

			SPARTIAL_EVAL_

			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}

int GetModList_Count(const char *s_prefix)
{
	char *s_current_working_dir = Sys_Getcwd_SBuf(); // No trailing slash

	// Just in case
	String_Edit_RemoveTrailingUnixSlash (s_current_working_dir);

	// Determine Quake folder
	char s_quake_folder_trail_slash[1024];
	if (fs_basedir[0])
		c_strlcpy (s_quake_folder_trail_slash, fs_basedir);
	else
		c_dpsnprintf1 (s_quake_folder_trail_slash, "%s/", s_current_working_dir);

	// Get list of files in Quake folder
	stringlist_t list;

	stringlistinit	(&list); // memset 0
	listdirectory	(&list, s_quake_folder_trail_slash, fs_all_files_empty_string);
	stringlistsort	(&list, fs_make_unique_true);

	int num_matches = 0;
	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *s_this = list.strings[idx];

		if (String_Is_Dot(s_this) || String_Is_DotDot(s_this))
			continue; // ignore "." and ".." as filenames

		if (String_Starts_With_Caseless (s_this, s_prefix) == false)
			continue; // Not prefix match

		if (String_Isin2 (s_this, "bin32", "bin64"))
			continue; // These folders are for DLL

		char s_this_fullpath[1024];
		c_dpsnprintf2 (s_this_fullpath, "%s%s", s_quake_folder_trail_slash, s_this);

		int file_directory_type = FS_SysFileOrDirectoryType (s_this_fullpath);

		if (file_directory_type != FS_FILETYPE_DIRECTORY_2)
			continue; // It is not a directory

		// Baker: FS_CheckGameDir does so little AFAIK
		const char *s_description = FS_CheckGameDir(s_this);
		if (s_description == NULL || s_description == fs_checkgamedir_missing)
			continue;

		char *sxy = s_this;

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents(&list);

	return num_matches;
}

int GetCommad_Count (const char *s_prefix, const char *s_singleton)
{
	int			comma_items_count = String_Count_Char (s_singleton, ',') + 1;
	int			num_matches = 0;


	for (int idx = 0; idx < comma_items_count; idx ++) {
		char *sxy =  String_Instance_Malloc_Base1 (s_singleton, ',' , idx + 1, NULL);
		if (String_Starts_With_Caseless (sxy, s_prefix) == false) {
			goto continuey; // For free
		}

		SPARTIAL_EVAL_


		num_matches ++;
continuey:
		freenull_ (sxy);
	} // idx
	return num_matches;
}



int GetAny1_Count (const char *s_prefix, const char *s_singleton)
{
	int			num_matches = 0;
	// Ok .. this has to be sorted due to first/last.
	const char *slist[] =  {
		s_singleton,
	};

	int			array_count = (int)ARRAY_COUNT(slist);

	for (int idx = 0; idx < array_count; idx ++) {
		const char *sxy =  slist[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

		num_matches ++;
	} // idx
	return num_matches;
}

//int GetCopyCmd_Count (const char *s_prefix)
//{
//	int			num_matches = 0;
//	// Ok .. this has to be sorted due to first/last.
//	const char *slist[] =  {
//		"ents",
//	};
//
//	int			array_count = (int)ARRAY_COUNT(slist);
//
//	for (int idx = 0; idx < array_count; idx ++) {
//		const char *sxy =  slist[idx];
//		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
//			continue;
//
//		SPARTIAL_EVAL_
//
//		num_matches ++;
//	} // idx
//	return num_matches;
//}

int GetEdictsCmd_Count (const char *s_prefix)
{
	int num_matches = 0;
	// Ok .. this has to be sorted due to first/last.
	const char *slist[] =  {
		"classname",
		"message",
		"model",
		"noise",
		"targetname",
	};

	int			array_count = (int)ARRAY_COUNT(slist);

	for (int idx = 0; idx < array_count; idx ++) {
		const char *sxy =  slist[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

		num_matches ++;
	} // idx
	return num_matches;
}

//
int GetREditLightsEdit_Count (const char *s_prefix)
{
	// Ok .. this has to be sorted due to first/last.
	const char *slist[] =  {
		"ambient",
		"angles",
		"anglesx",
		"anglesy",
		"anglesz",
		"color",
		"colorscale",
		"corona",
		"coronasize",
		"cubemap",
		"diffuse",
		"move",
		"movex",
		"movey",
		"movez",
		"normalmode",
		"origin",
		"originscale",
		"originx",
		"originy",
		"originz",
		"radius",
		"radiusscale",
		"realtimemode",
		"shadows",
		"sizescale",
		"specular",
		"style",
	};

	int array_count = (int)ARRAY_COUNT(slist);
	int	num_matches = 0;

	for (int idx = 0; idx < array_count; idx++) {
		const char *sxy =  slist[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_
	} // idx
	return num_matches;
}

int GetModelList_Count (const char *s_prefix)
{
	char		spattern[1024];
	char		s_prefix2[1024] = {0};

	if (s_prefix && s_prefix[0]) {
		// Remove ext ... Why?
		c_strlcpy (s_prefix2, s_prefix);
		File_URL_Edit_Remove_Extension (s_prefix2);
	} else {
		c_strlcpy (s_prefix2, "progs/");

	}

	c_dpsnprintf1 (spattern, "%s*.mdl", s_prefix2);

	fssearch_t	*t = FS_Search (spattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	int num_matches = 0;
	if (t && t->numfilenames > 0) {
		// The file search already checked prefix validity
		for (int idx = 0; idx < t->numfilenames; idx++) {
			char *sxy = t->filenames[idx];

			SPARTIAL_EVAL_

			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}

int GetTexGeneric_Count (const char *s_prefix)
{
	char		spattern[1024];
	char		s_prefix2[1024] = {0};

	if (s_prefix && s_prefix[0]) {
		// Remove ext ... Why?
		c_strlcpy (s_prefix2, s_prefix);
		File_URL_Edit_Remove_Extension (s_prefix2);
	}

	c_dpsnprintf1 (spattern, "%s*.tga", s_prefix2);

	fssearch_t	*t = FS_Search (spattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	int num_matches = 0;

	if (t && t->numfilenames > 0) {
		for (int idx = 0; idx < t->numfilenames; idx++) {
			char *sxy = t->filenames[idx];

			SPARTIAL_EVAL_

			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}



int GetSoundList_Count (const char *s_prefix)
{
	char		s_prefix_no_ext[MAX_QPATHX2_256] = {0};

	// Remove extension before we add an extension in the pattern further down
	c_strlcpy (s_prefix_no_ext, s_prefix);
	File_URL_Edit_Remove_Extension (s_prefix_no_ext);

	stringlist_t list = {0};

	va_super(spattern1, MAX_QPATHX2_256, "%s*.wav", s_prefix_no_ext); stringlistappend_search_pattern (&list, spattern1);
	va_super(spattern2, MAX_QPATHX2_256, "%s*.ogg", s_prefix_no_ext); stringlistappend_search_pattern (&list, spattern2);

	stringlistsort (&list, fs_make_unique_true);

	int num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx++) {
		char *sxy = list.strings[idx];

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents (&list);
	return num_matches;
}


WARP_X_ (R_ReplaceWorldTexture)


int GetShaderList_Count (const char *s_prefix)
{
	if (!r_refdef.scene.worldmodel || !cl.islocalgame || !cl.worldmodel) {
		return 0;
	}

	model_t		*m = r_refdef.scene.worldmodel;

	stringlist_t	matchedSet;
	stringlistinit  (&matchedSet); // this does not allocate

	texture_t	*tx = m->data_textures;

	// We cannot do comparisons here as this list is NOT SORTED
	for (int j = 0; j < m->num_textures; j ++, tx ++) {
		if (String_Starts_With_Caseless (tx->name, s_prefix) == false)
			continue;

		stringlistappend (&matchedSet, tx->name);
	} // for

	// SORT
	stringlistsort (&matchedSet, fs_make_unique_true);

	int			num_matches = 0;

	for (int idx = 0; idx < matchedSet.numstrings; idx ++) {
		char *sxy = matchedSet.strings[idx];

		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents (&matchedSet);

	return num_matches;
}

int GetTexWorld_Count (const char *s_prefix)
{
	if (!r_refdef.scene.worldmodel || !cl.islocalgame || !cl.worldmodel) {
		return 0;
	}

	model_t		*m = r_refdef.scene.worldmodel;

	stringlist_t	matchedSet;
	stringlistinit  (&matchedSet); // this does not allocate

	texture_t	*tx = m->data_textures;

	// We cannot do comparisons here as this list is NOT SORTED
	for (int j = 0; j < m->num_textures; j ++, tx ++) {
		if (String_Starts_With_Caseless (tx->name, s_prefix) == false)
			continue;

		stringlistappend (&matchedSet, tx->name);
	} // for

	// SORT
	stringlistsort (&matchedSet, fs_make_unique_true);

	int			num_matches = 0;

	for (int idx = 0; idx < matchedSet.numstrings; idx ++) {
		char *sxy = matchedSet.strings[idx];

		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents (&matchedSet);

	return num_matches;
}


// "folder" "dir" "ls" -- this completes in a weird way
#pragma message ("Baker: KEEP ME - GetFolderList_Count - This folder path completion autocomplete needs cleanup")
int GetFolderList_Count (const char *s_prefix)
{
	autocomplete_t *ac = &_g_autocomplete;

	stringlist_t list;

	char s_prefix_copy[1024] ;
	char gamepathos[1024] ;
	char s_prefix_filename_only[1024] ;
	char sthisy[1024] ;

	const char *safterpath = File_URL_SkipPath (s_prefix);
		char sgdwork[1024];
		c_strlcpy (sgdwork, fs_gamedir);
		File_URL_Remove_Trailing_Unix_Slash (sgdwork);

		const char *slastcom = File_URL_SkipPath(sgdwork);
		char sgamedirlast[1024];
		c_strlcpy (sgamedirlast, slastcom);
		File_URL_Remove_Trailing_Unix_Slash (sgamedirlast);
		// sgamedirlast is like "id1" or "travail" or whatever

	WARP_X_ (Con_Folder_f)

	// What is dir?
	c_strlcpy  (s_prefix_copy, s_prefix);
	if (String_Ends_With (s_prefix_copy, "/") == false)
		File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (s_prefix_copy);
	c_strlcpy  (s_prefix_filename_only, safterpath);

	c_strlcpy  (gamepathos, sgamedirlast); // "id1"
	c_strlcat  (gamepathos, "/");
	// fs_gamedir "C:\Users\Main\Documents/My Games/zircon/id1/"
	// fs_gamedir "id1/"
	// gamedirname1
	if (s_prefix_copy[0])
		c_strlcat  (gamepathos, s_prefix_copy);

	if (s_prefix_copy[0] && String_Ends_With (gamepathos,  "/")==false) {
		c_strlcat  (gamepathos, "/");		// Directory to list
		c_strlcat  (s_prefix_copy, "/");
	}

	stringlistinit	(&list);
	listdirectory	(&list, gamepathos /*fs_gamedir*/, fs_all_files_empty_string);

	// SORT
	stringlistsort (&list, true);

	int num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *s_this = list.strings[idx];

		if (String_Is_Dot(s_this) || String_Is_DotDot(s_this))
			continue; // ignore "." and ".." as filenames

		if (String_Starts_With_Caseless (s_this, s_prefix_filename_only) == false)
			continue;

		// Preserve the result to a variable
		// outside the for loop in case there is only one match
		c_strlcpy (sthisy, s_prefix_copy);
		c_strlcat (sthisy, s_this);

		char *sxy = sthisy;
		File_URL_Edit_Remove_Extension (sxy);

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	if (num_matches == 1) {
		// If only one match, we indicate the intention to help
		// the autocompletion "enter the folder"
		if (String_Match (s_prefix, ac->s_match_alphalast_a)) {
			// Completely replace search results
			freenull_ (ac->s_match_after_a)
			freenull_ (ac->s_match_alphalast_a);
			freenull_ (ac->s_match_alphatop_a);
			freenull_ (ac->s_match_before_a);

			c_strlcpy (sthisy, s_prefix);
			c_strlcat (sthisy, "/");

			char *sxy = sthisy;
			SPARTIAL_EVAL_
		}
	}

	stringlistfreecontents(&list);

	return num_matches;
}

// s_path "sound/doors/" s_prefix "air" s_suffix ".wav"
WARP_X_ (GetSoundList_Count)
int CatList (stringlist_t *pmain_list, const char *s_path, const char *s_prefix, const char *s_suffix)
{
	// For our purposes, we need to skip the first path
	const char *s_subpath = String_Find_Skip_Past (s_path, "/");

	char		spattern[1024];
	char		s_prefix2[1024] = {0};

	if (s_prefix && s_prefix[0]) {
		// Remove ext ... Why?
		c_strlcpy (s_prefix2, s_prefix);
		File_URL_Edit_Remove_Extension (s_prefix2);
	}

	c_dpsnprintf3 (spattern, "%s%s%s"  /*"*.wav"*/, s_subpath, s_prefix2, s_suffix);

	fssearch_t	*t = FS_Search (spattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	int num_matches = 0;

	if (t && t->numfilenames > 0) {
		for (int idx = 0; idx < t->numfilenames; idx++) {
			// Baker: We are getting the fullpath like "progs/player.mdl"
			// However, for our purposes we need to reduce

			char *sxy = t->filenames[idx];
			const char *s_skippath = File_URL_SkipPath(sxy);
			stringlistappend (pmain_list, s_skippath);
			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}

// "showmodel"
// Autocomplete of paths and or .md3 .obj .mdl .spr .bsp in path
int GetShowModelList_Count (const char *s_prefix)
{
	autocomplete_t *ac = &_g_autocomplete;

	stringlist_t list;

	char s_prefix_copy[1024] ;
	char gamepathos[1024] ;
	char s_prefix_filename_only[1024] ;
	char sthisy[1024] ;

	const char *safterpath = File_URL_SkipPath (s_prefix);
		char sgdwork[1024];
		c_strlcpy (sgdwork, fs_gamedir); // "id1/"
		File_URL_Remove_Trailing_Unix_Slash (sgdwork);

		const char *slastcom = File_URL_SkipPath(sgdwork);
		char sgamedirlast[1024];
		c_strlcpy (sgamedirlast, slastcom);
		File_URL_Remove_Trailing_Unix_Slash (sgamedirlast);
		// sgamedirlast is like "id1" or "travail" or whatever

	WARP_X_ (Con_Folder_f)

	// What is dir?
	c_strlcpy  (s_prefix_copy, s_prefix);
	if (String_Ends_With (s_prefix_copy, "/") == false)
		File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (s_prefix_copy);
	c_strlcpy  (s_prefix_filename_only, safterpath);

	c_strlcpy  (gamepathos, sgamedirlast); // "id1"
	c_strlcat  (gamepathos, "/");
	// fs_gamedir "C:\Users\Main\Documents/My Games/zircon/id1/"
	// fs_gamedir "id1/"
	// gamedirname1
	if (s_prefix_copy[0])
		c_strlcat  (gamepathos, s_prefix_copy);

	if (s_prefix_copy[0] && String_Ends_With (gamepathos,  "/")==false) {
		c_strlcat  (gamepathos, "/");		// Directory to list
		c_strlcat  (s_prefix_copy, "/");
	}

	stringlistinit	(&list);
	// game pathos tends to be "id1/" here
	// game pathos tends to be "id1/progs/"
	listdirectory	(&list, gamepathos /*fs_gamedir*/, fs_all_files_empty_string);

	CatList (&list, gamepathos, s_prefix_filename_only, "*.mdl");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.md3");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.spr");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.obj");

	// SORT
	stringlistsort (&list, /*uniq*/ fs_make_unique_true);

	int num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *s_this = list.strings[idx];

		if (String_Is_Dot(s_this) || String_Is_DotDot(s_this))
			continue; // ignore "." and ".." as filenames

		if (String_Starts_With_Caseless (s_this, s_prefix_filename_only) == false)
			continue;

		if (String_Ends_With_Caseless (s_this, ".mdl") ||
			String_Ends_With_Caseless (s_this, ".md3") ||
			String_Ends_With_Caseless (s_this, ".spr") ||
			String_Ends_With_Caseless (s_this, ".obj") ||
			String_Contains (s_this, ".") == false)
		{
			// Stay
		} else {
			continue; // Disqualified
		}

		// Preserve the result to a variable
		// outside the for loop in case there is only one match
		c_strlcpy (sthisy, s_prefix_copy);
		c_strlcat (sthisy, s_this);

		char *sxy = sthisy;
		//File_URL_Edit_Remove_Extension (sxy);

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	// Baker: Don't offer a trailing slash if there is a dot in it, probably a .mdl or other file
	if (num_matches == 1 && String_Contains(ac->s_match_alphalast_a, ".") == false) {
		// If only one match, we indicate the intention to help
		// the autocompletion "enter the folder"
		if (String_Match (s_prefix, ac->s_match_alphalast_a)) {
			// Completely replace search results
			freenull_ (ac->s_match_after_a)
			freenull_ (ac->s_match_alphalast_a);
			freenull_ (ac->s_match_alphatop_a);
			freenull_ (ac->s_match_before_a);

			c_strlcpy (sthisy, s_prefix);
			c_strlcat (sthisy, "/");

			char *sxy = sthisy;
			SPARTIAL_EVAL_
		}
	}

	stringlistfreecontents(&list);

	return num_matches;
}


int GetAnimatedGIF_Count (const char *s_prefix)
{
	autocomplete_t *ac = &_g_autocomplete;

	stringlist_t list;

	char s_prefix_copy[1024] ;
	char gamepathos[1024] ;
	char s_prefix_filename_only[1024] ;
	char sthisy[1024] ;

	const char *safterpath = File_URL_SkipPath (s_prefix);
		char sgdwork[1024];
		c_strlcpy (sgdwork, fs_gamedir);
		File_URL_Remove_Trailing_Unix_Slash (sgdwork);

		const char *slastcom = File_URL_SkipPath(sgdwork);
		char sgamedirlast[1024];
		c_strlcpy (sgamedirlast, slastcom);
		File_URL_Remove_Trailing_Unix_Slash (sgamedirlast);
		// sgamedirlast is like "id1" or "travail" or whatever

	WARP_X_ (Con_Folder_f)

	// What is dir?
	c_strlcpy  (s_prefix_copy, s_prefix);
	if (String_Ends_With (s_prefix_copy, "/") == false)
		File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (s_prefix_copy);
	c_strlcpy  (s_prefix_filename_only, safterpath);

	c_strlcpy  (gamepathos, sgamedirlast); // "id1"
	c_strlcat  (gamepathos, "/");
	// fs_gamedir "C:\Users\Main\Documents/My Games/zircon/id1/"
	// fs_gamedir "id1/"
	// gamedirname1
	if (s_prefix_copy[0])
		c_strlcat  (gamepathos, s_prefix_copy);

	if (s_prefix_copy[0] && String_Ends_With (gamepathos,  "/")==false) {
		c_strlcat  (gamepathos, "/");		// Directory to list
		c_strlcat  (s_prefix_copy, "/");
	}

	stringlistinit	(&list);
	// game pathos tends to be "id1/" here
	// game pathos tends to be "id1/progs/"
	listdirectory	(&list, gamepathos /*fs_gamedir*/, fs_all_files_empty_string);

	CatList (&list, gamepathos, s_prefix_filename_only, "*.tga");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.jpg");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.png");

	// SORT
	stringlistsort (&list, /*uniq*/ fs_make_unique_true);

	int num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *s_this = list.strings[idx];

		if (String_Is_Dot(s_this) || String_Is_DotDot(s_this))
			continue; // ignore "." and ".." as filenames

		if (String_Starts_With_Caseless (s_this, s_prefix_filename_only) == false)
			continue;

		if (String_Ends_With_Caseless (s_this, ".gif") ||
			String_Contains (s_this, ".") == false)
		{
			// Stay
		} else {
			continue; // Disqualified
		}

		// Preserve the result to a variable
		// outside the for loop in case there is only one match
		c_strlcpy (sthisy, s_prefix_copy);
		c_strlcat (sthisy, s_this);

		char *sxy = sthisy;
		//File_URL_Edit_Remove_Extension (sxy);

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	// Baker: Don't offer a trailing slash if there is a dot in it, probably a .mdl or other file
	if (num_matches == 1 && String_Contains(ac->s_match_alphalast_a, ".") == false) {
		// If only one match, we indicate the intention to help
		// the autocompletion "enter the folder"
		if (String_Match (s_prefix, ac->s_match_alphalast_a)) {
			// Completely replace search results
			freenull_ (ac->s_match_after_a)
			freenull_ (ac->s_match_alphalast_a);
			freenull_ (ac->s_match_alphatop_a);
			freenull_ (ac->s_match_before_a);

			c_strlcpy (sthisy, s_prefix);
			c_strlcat (sthisy, "/");

			char *sxy = sthisy;
			SPARTIAL_EVAL_
		}
	}

	stringlistfreecontents(&list);

	return num_matches;
}

int GetShowTGAJPGPNGList_Count (const char *s_prefix)
{
	autocomplete_t *ac = &_g_autocomplete;

	stringlist_t list;

	char s_prefix_copy[1024] ;
	char gamepathos[1024] ;
	char s_prefix_filename_only[1024] ;
	char sthisy[1024] ;

	const char *safterpath = File_URL_SkipPath (s_prefix);
		char sgdwork[1024];
		c_strlcpy (sgdwork, fs_gamedir);
		File_URL_Remove_Trailing_Unix_Slash (sgdwork);

		const char *slastcom = File_URL_SkipPath(sgdwork);
		char sgamedirlast[1024];
		c_strlcpy (sgamedirlast, slastcom);
		File_URL_Remove_Trailing_Unix_Slash (sgamedirlast);
		// sgamedirlast is like "id1" or "travail" or whatever

	WARP_X_ (Con_Folder_f)

	// What is dir?
	c_strlcpy  (s_prefix_copy, s_prefix);
	if (String_Ends_With (s_prefix_copy, "/") == false)
		File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (s_prefix_copy);
	c_strlcpy  (s_prefix_filename_only, safterpath);

	c_strlcpy  (gamepathos, sgamedirlast); // "id1"
	c_strlcat  (gamepathos, "/");
	// fs_gamedir "C:\Users\Main\Documents/My Games/zircon/id1/"
	// fs_gamedir "id1/"
	// gamedirname1
	if (s_prefix_copy[0])
		c_strlcat  (gamepathos, s_prefix_copy);

	if (s_prefix_copy[0] && String_Ends_With (gamepathos,  "/")==false) {
		c_strlcat  (gamepathos, "/");		// Directory to list
		c_strlcat  (s_prefix_copy, "/");
	}

	stringlistinit	(&list);
	// game pathos tends to be "id1/" here
	// game pathos tends to be "id1/progs/"
	listdirectory	(&list, gamepathos /*fs_gamedir*/, fs_all_files_empty_string);

	CatList (&list, gamepathos, s_prefix_filename_only, "*.tga");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.jpg");
	CatList (&list, gamepathos, s_prefix_filename_only, "*.png");

	// SORT
	stringlistsort (&list, /*uniq*/ fs_make_unique_true);

	int num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *s_this = list.strings[idx];

		if (String_Is_Dot(s_this) || String_Is_DotDot(s_this))
			continue; // ignore "." and ".." as filenames

		if (String_Starts_With_Caseless (s_this, s_prefix_filename_only) == false)
			continue;

		if (String_Ends_With_Caseless (s_this, ".tga") ||
			String_Ends_With_Caseless (s_this, ".jpg") ||
			String_Ends_With_Caseless (s_this, ".png") ||
			String_Contains (s_this, ".") == false)
		{
			// Stay
		} else {
			continue; // Disqualified
		}

		// Preserve the result to a variable
		// outside the for loop in case there is only one match
		c_strlcpy (sthisy, s_prefix_copy);
		c_strlcat (sthisy, s_this);

		char *sxy = sthisy;
		//File_URL_Edit_Remove_Extension (sxy);

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	// Baker: Don't offer a trailing slash if there is a dot in it, probably a .mdl or other file
	if (num_matches == 1 && String_Contains(ac->s_match_alphalast_a, ".") == false) {
		// If only one match, we indicate the intention to help
		// the autocompletion "enter the folder"
		if (String_Match (s_prefix, ac->s_match_alphalast_a)) {
			// Completely replace search results
			freenull_ (ac->s_match_after_a)
			freenull_ (ac->s_match_alphalast_a);
			freenull_ (ac->s_match_alphatop_a);
			freenull_ (ac->s_match_before_a);

			c_strlcpy (sthisy, s_prefix);
			c_strlcat (sthisy, "/");

			char *sxy = sthisy;
			SPARTIAL_EVAL_
		}
	}

	stringlistfreecontents(&list);

	return num_matches;
}

int GetGameCommands_Count (const char *s_prefix, ccs *s_gamecommands_string)
{
	// This process depends on this s_gamecommands_string having items.
	if (s_gamecommands_string[0] == NULL_CHAR_0)
		return 0;


#if 1
	stringlist_t matchedSet = {0};
	char *s_za = Z_StrDup (s_gamecommands_string);
	String_Edit_Trim (s_za);
	//ccs *s_dbl = NULL;

	while (strstr(s_za, " " " ")) {
		size_t slenp1 = strlen(s_za) + ONE_CHAR_1;
		String_Edit_Replace (s_za, slenp1, "  ", " "); // Replace 2x spaces with 1x space.
	}

	stringlistappend_split (&matchedSet, s_za, " ");
	Mem_FreeNull_ (s_za);

#endif


#if 0
	stringlist_t matchedSet;

	stringlistinit	(&matchedSet); // this does not allocate, memset 0
	const char	*s_space_delim		= " ";
	int			s_len			= (int)strlen(s_gamecommands_string);
	int			s_delim_len		= (int)strlen(s_space_delim);

	// Baker: This works the searchpos against s_gamecommands_string
	// finding the delimiter (space) and adding a list item until there are no more spaces
	// (an iteration with no space adds the rest of the string.

	// Baker: have we tested this against a single item without a space to see what happens?
	// It looks like it can handle that.

	// BUILD LIST

	int			searchpos		= 0;
	while (1) {
		char s_this_copy[MAX_INPUTLINE_16384];
		const char	*space_pos	= strstr (&s_gamecommands_string[searchpos], s_space_delim); // string_find_pos_start_at(s, s_delim, searchpos);
		int			endpos		= (space_pos == NULL) ? (s_len - 1) : ( (space_pos - s_gamecommands_string) - 1); // (commapos == not_found_neg1) ? (s_len -1) : (commapos -1);
		int			this_w		= (endpos - searchpos + 1); // string_range_width (searchpos, endpos); (endpos - startpos + 1)

		memcpy (s_this_copy, &s_gamecommands_string[searchpos], this_w);
		s_this_copy[this_w] = NULL_CHAR_0; // term

		stringlistappend (&matchedSet, s_this_copy);

		// If no space found, we added the rest of the string as an item, so get out!
		if (space_pos == NULL)
			break;

		searchpos = (space_pos - s_gamecommands_string) + s_delim_len;
	} // while
#endif


	// SORT plus unique-ify
	stringlistsort (&matchedSet, fs_make_unique_true);

	int num_matches = 0;

	for (int idx = 0; idx < matchedSet.numstrings; idx ++) {
		char *sxy = matchedSet.strings[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

		num_matches ++;
	} // for

	stringlistfreecontents( &matchedSet );

	return num_matches;
}

int GetModelPrecacheList_Count (const char *s_prefix)
{
	if (!r_refdef.scene.worldmodel)
		return 0;

	stringlist_t list = {0};

	// Baker: Why 1?
	for (int j = 1; j < MAX_MODELS_8192 && cl.model_name[j][0]; j ++) {
		stringlistappend (&list, cl.model_name[j]);
	} // for

	// Sort

	stringlistsort (&list, fs_make_unique_true);

	int			num_matches = 0;

	for (int idx = 0; idx < list.numstrings; idx ++) {
		char *sxy =  list.strings[idx];
		if (String_Starts_With_Caseless (sxy, s_prefix) == false) {
			continue;
		}

		SPARTIAL_EVAL_

		num_matches ++;

	} // idx

	stringlistfreecontents (&list);

	return num_matches;
}

