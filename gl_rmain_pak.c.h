// gl_rmain_pak.c.h
// dir textures/wiz*.jpg <-- works


// pak_this_map <folder>

// Real path for a file?

// dir textures/wiz*.jpg

//FS_FileExtension(list.strings[i])

//what is the condump command?

//	file = FS_OpenRealFile(Cmd_Argv(cmd, 1), "w", false);
//demo is example of write binary ...
// cls.demofile = FS_OpenRealFile(name, "wb", false);

//unsigned char *filedata = FS_LoadFile(filename, tempmempool, fs_quiet_true, &filesize);

//WARP_X_ (FS_Which_f)//(cmd_state_t *cmd); // Finds where a file is
// How do we read the whole damn file?

//Probably 			filedata = FS_LoadFile(filename, tempmempool, fs_quiet_true, &filesize);
//			if (!filedata)
//				continue;

			// How do we find with a file pattern?
// Let's see what did does... example:

//cbool File_Copy (const char *src_path_to_file, const char *dst_path_to_file)
//{
//	// Has to copy the file without changing the date
//Dev_Note ("TODO: File_Copy messes up the date and loads to memory on non-Windows fixme")
//Dev_Note ("TODO: Note folder dates will probably be changed. ")
//	if (!File_Exists (src_path_to_file))
//		return false;
//
//	block_start__
//
//#ifdef PLATFORM_WINDOWS
//	// More complicated than it should be.
//	// CopyFile: Requires target directory to exist.
//	cbool ret;
//	File_Mkdir_Recursive (dst_path_to_file);
//	// CopyFile doesn't preserve date modified  Neither does Ex
//	// CopyFileEx (src_path_to_file, dst_path_to_file, NULL, NULL, NULL, 0);
//
//	//return CopyFile (src_path_to_file, dst_path_to_file, false /*don't fail if file exists*/) != 0;
//	ret = CopyFile (src_path_to_file, dst_path_to_file, false /*don't fail if file exists*/) != 0;
//}

WARP_X_ (SV_Map_f CL_Record_f r_listmaptextures)

//]pak_this_map park
//folder is
//   0: models/outdoors/tree_leaves3_shader
//   1: noshader
//   2: models/outdoors/tree_bark
//   3: models/furniture/cabinet_wood_side
//   4: models/furniture/cabinet_wood_top
//   5: models/furniture/cabinet_wood_front
//   6: textures/zz_rgb/color_gray_32
//   7: textures/ambientcg/paper_005
//   8: textures/ambientcg/wood_051
//   9: textures/azirc0/floor_lite_slate
//  10: textures/texturecan/wood_0066
//  11: textures/texturecan/tiles_0108
//  12: textures/ambientcg/Planks029L
//  13: textures/ambientcg/grass_001
//  14: textures/texturecan/concrete_0026
//  15: textures/texturecan/bricks_0020
//  16: textures/texturecan/metal_0077
//  17: textures/trak5x_sh/base_base1a
//  18: textures/trak5x_sh/light_light3a
//  19: textures/ambientcg/wood_floor_056
//  20: textures/common/caulk
//  21: textures/liquids/mirror_solid
//  22: textures/ambientcg/sandstone_wall_base_1k
//  23: textures/skies/jf_nebula_sky_nolight
//  24: textures/texturecan/bricks_0019
//  25: textures/texturecan/rooftop_0007
//  26: textures/exx/wall_crete03
//  27: textures/texturecan/tiles_0097
//  28: textures/texturecan/ground_0041
//  29: textures/texturecan/bricks_0023
//  30: textures/trak4x_sh/floor_tile3b
//  31: textures/texturecan/bricks_0018
//  32: textures/trak5x_sh/floor_floor2f
//  33: textures/common/nodraw
//  34: textures/decals/splatter01
//  35: textures/liquids/water4_tzork_rk
//  36: textures/exx/base_crete03
//  37: textures/fence/base_chainlink_dark2_nomipmap
//  38: textures/fx/corona_white
//  39: textures/effects_sh/brown_glass
//]copy


// jf_nebula_sky_nolight

// Shader_get_text - first instance
// example pak_this_shader "textures/trak5x_sh/floor_floor2f"

// Returns null if can't

// shader_t *myshader = Mod_LookupQ3Shader(s_shader);

	//textures/decals/scorch01
	//{
	//	qer_editorimage textures/decals/scorch01
	//	dpoffsetmapping none

	//	surfaceparm trans
	//	surfaceparm nonsolid
	//	surfaceparm nodlight
	//	surfaceparm nolightmap

	//	polygonOffset
	//	sort 6
	//	cull none

	//	{
	//		map textures/decals/scorch01
	//		blendFunc filter
	//		rgbgen identity
	//	}
	//}

char *ShaderText_Alloc (shader_t *myshader, const char *s_shadername, char *s_return_shader, size_t s_return_shader_size)
{
	const int is_print = false;
	char *sout_alloc = NULL;
	fssearch_t *search = FS_Search("scripts/*.shader", fs_caseless_true, fs_quiet_FALSE, fs_pakfile_null, fs_gamedironly_false);

	if (!search)
		return NULL;

	char namebuf64[Q3PATHLENGTH_64];
	int is_done = false;

	char *f;
	for (int fileindex = 0; fileindex < search->numfilenames; fileindex ++) {
		const char *text = f = (char *)FS_LoadFile(search->filenames[fileindex], tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
		if (!f)
			continue;

		int bracket_depth = 0;

		const char *s0 = text;
		while (COM_ParseToken_QuakeC(&text, false)) {
			c_strlcpy (namebuf64, com_token);
			int is_one_we_want = String_Match (s_shadername, namebuf64);
			if (is_one_we_want) {
				const char *s_file = search->filenames[fileindex];
				if (s_return_shader) {
					strlcpy (s_return_shader, s_file, s_return_shader_size);
				}
				if (is_print) Con_PrintLinef (CON_BRONZE "Found shader" CON_WHITE " in %s", s_file);
			}
			if (!COM_ParseToken_QuakeC(&text, false) || String_NOT_Match(com_token, "{")) {
				if (is_print) Con_PrintLinef ("%s parsing error - expected \"{\", found " QUOTED_S, search->filenames[fileindex], com_token);
				break;
			}
			// Parsed a "{"
			bracket_depth ++;
			while (COM_ParseToken_QuakeC(&text, false)) {
				if (String_Match_Caseless(com_token, "}")) {
					bracket_depth --;
					if (bracket_depth <= 0)
						break;
				} else if (String_Match_Caseless(com_token, "{")) {
					bracket_depth ++;
				}
			} // While
			if (is_one_we_want) {


				is_done = true;
				//const char *s00 = s0;
				//while (ISWHITESPACE(*s00))
				//	s00++;
				// Start with the name, not any comments or whitespace before it.
				const char *s000 = strstr (s0,namebuf64);

				const char *s1 = text;
				size_t bufsize;
				sout_alloc = (char *)core_memdup_z(s000, s1-s000, &bufsize);
				char *sxx = sout_alloc;
				// Baker: Normal white space please
				WARP_X_ (TAB_CHARACTER)
				while (*sxx) {
					// Baker: Is it carriage returns that are toxic?
					// Baker: No tabs are toxic as well
					if (*sxx < 32 && *sxx != NEWLINE_CHAR_10)
						*sxx = 32;
					sxx++;
				}

				//c_strlcpy (shadertextbuf, sout_alloc);
				is_done =true;

				break; // We got what we need, quit looking...
			}
			s0 = text;
		} // while in the shader file
		Mem_Free (f); // free the file
		if (is_done) break; // No more checking.
	} // for each file
	FS_FreeSearch(search);

	return sout_alloc;
}



void Pak_Accum_Texture_Dependencies (stringlist_t *ptexture_dependency_list, const char *s_texture_maybe_with_extension)
{
	const int is_print = false;
	char s_texture_in_no_extension[MAX_QPATH_128];
	char s_texture_in_wild[MAX_QPATH_128];

	FS_StripExtension (s_texture_maybe_with_extension, s_texture_in_no_extension, sizeof(s_texture_in_no_extension)); // This copies the filename, does more than strip

	c_strlcpy (s_texture_in_wild, s_texture_in_no_extension);
	c_strlcat (s_texture_in_wild, "*");

	fssearch_t *search = FS_Search(s_texture_in_wild, fs_caseless_true, fs_quiet_FALSE, fs_pakfile_null, fs_gamedironly_false);

	if (!search) {
		Con_PrintLinef ("No search results for " QUOTED_S, s_texture_in_wild);
		return;
	}

	for (int fileindex = 0; fileindex < search->numfilenames; fileindex ++) {
		char *s_this_file = search->filenames[fileindex];
		int is_ok = String_Ends_With_Caseless (s_this_file, ".tga") ||
					String_Ends_With_Caseless (s_this_file, ".png") ||
					String_Ends_With_Caseless (s_this_file, ".jpg");

		if (is_ok == false)
			continue; // unrecognized file format, we only do images we can load.

		stringlistappend (ptexture_dependency_list, s_this_file);
		if (is_print)
			Con_PrintLinef ("Added " QUOTED_S, s_this_file);
	} // for
}

		//const char *text = f = (char *)FS_LoadFile(search->filenames[fileindex], tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
		//if (!f)
		//	continue;

// "pak_this_map"

//   .bsp.zip // must not be in a .pk3
//   //misc_mdl_entities ..
//   //misc_mdl_illusionary ...
//   textures/
//   scripts/
//   models/
//   maps/ ... any relevant shit, we aren't doing the lightmap

// Not captured:
// misc_mdl_entities, misc_mdl_illusionary, sounds, cdtrack
void Dependencies_For_This_Model (model_t *m, stringlist_t *ptexture_dependency_list,
								  stringlist_t *ptexture_shader_dependency_list,
								  stringlist_t *pshader_name_list,
								  stringlist_t *pshader_textblock_dependency_list, int is_print_stuff)
{
	const int is_exclude_textures_common = false; // DON'T
	texture_t	*t;
	int j;
	for (j = 0, t = m->data_textures; j < m->num_textures; j++, t++) {
		char *s_this_texture = t->name;
		if (s_this_texture[0] == 0) continue;
		if (String_Contains_Caseless(s_this_texture, "NO TEXTURE FOUND"))	continue;

		shader_t *myshader = Mod_LookupQ3Shader(s_this_texture);
		//Con_PrintLinef ("%4d: %s", j, t->name);

		if (myshader == NULL) {
			// Normal texture
			Pak_Accum_Texture_Dependencies (ptexture_dependency_list, s_this_texture);
			continue;
		}

		stringlistappend (pshader_name_list, s_this_texture); //

		if (String_Starts_With (s_this_texture, "textures/common/")) {
			if (is_print_stuff)
				Con_PrintLinef (CON_BRONZE "Common texture found and  %s " QUOTED_S, 
				s_this_texture, is_exclude_textures_common ? "EXCLUDING" : "ADDING!");

			if (is_exclude_textures_common)
				continue; // Do not add, we are exluding those
		}

		if (myshader) {
			char *s_shader_textblock_alloc = ShaderText_Alloc (myshader, s_this_texture, /*get shader file name?*/ NULL, 0);

			if (s_shader_textblock_alloc == NULL) {
				Con_PrintLinef (CON_ERROR "ERROR shader text is null %s", s_this_texture);
			} else {
				stringlistappend (pshader_textblock_dependency_list, s_shader_textblock_alloc);
			}

			freenull_ (s_shader_textblock_alloc)

			if (myshader->dpreflectcube[0]) {
				//Con_PrintLinef (" reflectcube = %s", myshader->dpreflectcube);
				Pak_Accum_Texture_Dependencies (ptexture_shader_dependency_list, myshader->dpreflectcube);
			}

			if (myshader->skyboxname[0]) {
				//Con_PrintLinef (" skybox = %s", myshader->skyboxname);
				Pak_Accum_Texture_Dependencies (ptexture_shader_dependency_list, myshader->skyboxname);
			}

			for (int k = 0; k < myshader->numlayers; k ++) {
				q3shaderinfo_layer_t *layer = myshader->layers + k;
				for (int framenum = 0; framenum < layer->sh_numframes; framenum ++) {
					const char *s_layer_texture = layer->sh_ptexturename[framenum];
					if (s_layer_texture == NULL) continue;
					if (String_Starts_With (s_layer_texture, "$")) {
						if (is_print_stuff)
							Con_PrintLinef ("Ignoring texture named " QUOTED_S, s_layer_texture);
						continue;
					}

					Pak_Accum_Texture_Dependencies (ptexture_shader_dependency_list, s_layer_texture);
				} // framenum

			} // for
		} // if
	} // for

}

// Baker: A filecopy that does not use the operating system and the date and time are not
// preserved.
fs_offset_t FS_FileCopy_Shitty (const char *s_src, const char *s_dst)
{
	fs_offset_t src_filesize; // fs_offset_t is int64_t even on 32 bit
	char *filedata = (char *)FS_LoadFile(s_src, tempmempool, fs_quiet_FALSE, &src_filesize);

	if (filedata == NULL) {
		Con_PrintLinef (CON_ERROR "Could not open file source " QUOTED_S, s_src);
		return 0;
	}

	//fs_offset_t dest_filesize;

	// Baker: It is a real file ...
	qfile_t *filetest = FS_OpenRealFile (s_dst, "rb", fs_quiet_FALSE);  // WRITE-EON file copy shitty

	if (filetest) {
		FS_Close (filetest);
		Con_PrintLinef (CON_ERROR "Dest already exists ... ignoring ... " QUOTED_S, s_dst);
		return 0;
	}

	qfile_t *fout = FS_OpenRealFile(s_dst, "wb", fs_quiet_FALSE);  // WRITE-EON  File Copy Shitty

	if (fout == NULL) {
		Con_PrintLinef (CON_ERROR "Could not open " QUOTED_S, s_dst);
		return 0;
	}

	// Baker: Not sure how the error handling works here ...
	FS_Write (fout, filedata, src_filesize);

	Mem_Free(filedata);
	FS_Close (fout);

	return src_filesize;
}

// Takes a block of text, accumulates shader names and textblocks
//			textures/decals/scorch01
//			{
//				qer_editorimage textures/decals/scorch01
//				dpoffsetmapping none
//
//				surfaceparm trans
//				surfaceparm nonsolid
//				surfaceparm nodlight
//				surfaceparm nolightmap
//
//				polygonOffset
//				sort 6
//				cull none
//
//				{
//					map textures/decals/scorch01
//					blendFunc filter
//					rgbgen identity
//				}
//			}

static void ShaderParseText_Proc (const char *s, stringlist_t *pshader_name_list, stringlist_t *pshader_textblock_dependency_list)
{
	const int is_print = false;
	int bracket_depth = 0;
	char namebuf64[Q3PATHLENGTH_64];

	const char *text;
	const char *s0 = text = s;
	while (COM_ParseToken_QuakeC(&text, false)) {
		c_strlcpy (namebuf64, com_token);
		stringlistappend (pshader_name_list, namebuf64);
		int is_one_we_want = true; // String_Match (s_shadername, namebuf64);
		if (!COM_ParseToken_QuakeC(&text, false) || String_NOT_Match(com_token, "{")) {
			if (is_print) Con_PrintLinef ("parsing error - expected \"{\", found " QUOTED_S, com_token);
			break;
		}
		// Parsed a "{"
		bracket_depth ++;
		while (COM_ParseToken_QuakeC(&text, false)) {
			if (String_Match_Caseless(com_token, "}")) {
				bracket_depth --;
				if (bracket_depth <= 0)
					break;
			} else if (String_Match_Caseless(com_token, "{")) {
				bracket_depth ++;
			}
		} // While

		if (is_one_we_want) {
			void *core_memdup_z (const void *src, size_t len, /*modify*/ size_t *bufsize_made);
			const char *s000 = strstr (s0,namebuf64);

			const char *s1 = text;
			size_t bufsize;
			char *s_alloc = (char *)core_memdup_z(s000, s1-s000, &bufsize);
			char *sxx = s_alloc;
			// Baker: Normal white space please
			while (*sxx) {
				// Baker: Is it carriage returns that are toxic?
				// Baker: No tabs are toxic as well
				if (*sxx < 32 && *sxx != NEWLINE_CHAR_10)
					*sxx = 32;
				sxx++;
			}
			stringlistappend (pshader_textblock_dependency_list, s_alloc);
			freenull_ (s_alloc);
		}
		s0 = text;
	} // while in the shader file

}

#if 0
// shadertextparse
static void R_ShaderTextParse_f (cmd_state_t *cmd)
{
	/*cleanupok*/ char *s_alloc = Sys_GetClipboardData_Unlimited_ZAlloc(); // zallocs
	if (s_alloc == NULL) {
		Con_PrintLinef ("Couldn't get clipboard text");
		return;
	}

	stringlist_t shader_textblock_dependency_list  = {0};
	stringlist_t shader_name_list = {0};

	ShaderParseText_Proc (s_alloc, &shader_name_list, &shader_textblock_dependency_list);

	stringlistsort (&shader_textblock_dependency_list, fs_make_unique_false);
	stringlistsort (&shader_name_list, fs_make_unique_false);

	stringlistprint (&shader_name_list,"Shader Names            ", Con_PrintLinef);
	stringlistprint (&shader_textblock_dependency_list,"Shader Text Blocks      ", Con_PrintLinef);

	stringlistfreecontents (&shader_textblock_dependency_list);
	stringlistfreecontents (&shader_name_list);

	if (s_alloc)
		Z_Free ((void *)s_alloc);

}
#endif

// "pak_this_map"
static void R_Pak_This_Map_f (cmd_state_t *cmd)
{
	int is_connected = (cls.state == ca_connected && cls.signon == SIGNONS_4 && cl.worldmodel);
	int is_overwrite = Cmd_Argc (cmd) >= 3 && String_Match_Caseless ("overwrite", Cmd_Argv(cmd, 2));
	int is_test = Cmd_Argc (cmd) == 1;

	if (!is_connected) {
		Con_PrintLinef ("Not connected");
		return;
	}

	//if (Cmd_Argc (cmd) < 2) {
	//	Con_PrintLinef ("usage: %s <folder>" NEWLINE "copies texture, skybox, shader dependencies to a folder", Cmd_Argv(cmd, 0));
	//	Con_PrintLinef ("usage: %s <folder>" NEWLINE "copies texture, skybox, shader dependencies to a folder", Cmd_Argv(cmd, 0));
	//	return;
	//}

	const char *s_folder_dest_write = Cmd_Argv (cmd, 1);

	// Light protection against user accidentially typing something stupid
	if (is_test == false &&
		(String_Match_Caseless (s_folder_dest_write, "maps") ||
		String_Match_Caseless (s_folder_dest_write, "scripts") ||
		String_Match_Caseless (s_folder_dest_write, "models") ||
		String_Match_Caseless (s_folder_dest_write, "progs") ||
		String_Match_Caseless (s_folder_dest_write, "textures") ||
		String_Match_Caseless (s_folder_dest_write, "sound"))) {

		Con_PrintLinef ("This command outputs files into a folder");
		Con_PrintLinef ("Try a different folder than one named" QUOTED_S, s_folder_dest_write);

		return;
	}

	if (is_test == false) Con_PrintLinef ("folder is %s", s_folder_dest_write);

	stringlist_t texture_dependency_list = {0};
	stringlist_t texture_shader_dependency_list = {0};
	stringlist_t shader_textblock_dependency_list  = {0};
	stringlist_t shader_name_list = {0};

	model_t		*m = cl.worldmodel;

	// FIRST -- CHECK FOR EXISTING STUFF
	qfile_t *f = NULL;

	char s_scripts_out[MAX_QPATH_128];
	extern float host_hoststartup_unique_num;
	char s_num[MAX_QPATH_128];
	c_dpsnprintf1 (s_num, "%f", host_hoststartup_unique_num);
	String_Edit_Replace (s_num, sizeof(s_num), ".", "_");
	c_dpsnprintf2 (s_scripts_out, "%s/scripts/map_%s.shader", s_folder_dest_write, s_num);

	if (is_test)
		goto test_no_write1;

	f = FS_OpenRealFile (s_scripts_out, "rb", fs_quiet_FALSE);  // WRITE-EON pak this map
	if (f) {
		FS_Close (f); f = NULL;
		Con_PrintLinef ("Existing data in folder ... ");

		if (is_overwrite == false) {
			Con_PrintLinef ("Overwrite not specified! .. add overwrite to your command ");
			Con_PrintLinef ("To enable cumulative processingOverwrite not specified!");
			Con_PrintLinef ("Or delete the existing data in that folder and try again");
			return;
		}
		Con_PrintLinef ("overwrite specified ... doing cumulative write ... " QUOTED_S, s_scripts_out);


		char *s_text_alloc = (char *)FS_LoadFile(s_scripts_out, tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
		if (!s_text_alloc) {
			Con_PrintLinef (CON_ERROR "Could not open " QUOTED_S, s_scripts_out);
			return;
		}

		// Read the file and parse the scripts
		ShaderParseText_Proc (s_text_alloc, 	&shader_name_list, &shader_textblock_dependency_list);
		Mem_Free (s_text_alloc);
	}

test_no_write1:
	Dependencies_For_This_Model (
		m,
		&texture_dependency_list,
		&texture_shader_dependency_list,
		&shader_name_list,
		&shader_textblock_dependency_list,
		/*is print stuff?*/ false
	);

	if (is_overwrite && is_test == false) {
		stringlistsort (&shader_textblock_dependency_list, fs_make_unique_true);
		stringlistsort (&shader_name_list, fs_make_unique_true);
	} else {
		stringlistsort (&shader_textblock_dependency_list, fs_make_unique_false);
		stringlistsort (&shader_name_list, fs_make_unique_false);
	}

	stringlistsort (&texture_dependency_list, fs_make_unique_false);
	stringlistsort (&texture_shader_dependency_list, fs_make_unique_false);

	stringlistprint (&texture_dependency_list,         "Raw Textures In Map     ", Con_PrintLinef);
	stringlistprint (&texture_shader_dependency_list,  "Textures Within Shaders ", Con_PrintLinef);
	stringlistprint (&shader_name_list,"Shader Names            ", Con_PrintLinef);
	stringlistprint (&shader_textblock_dependency_list,"Shader Text Blocks      ", Con_PrintLinef);

	if (is_test)
		goto test_is_done;

	// texture_dependency_list
	// texture_shader_dependency_list

	fs_offset_t written_bytes = 0;
	stringlist_t *targ = NULL;

file_copy_start:

	targ = &texture_dependency_list;
	for (int idx = 0; idx < targ->numstrings; idx ++) {
		char *s_src = targ->strings[idx];
		char s_dest[MAX_QPATH_128];
		c_dpsnprintf2 (s_dest, "%s/%s", s_folder_dest_write, s_src);
		written_bytes += FS_FileCopy_Shitty (s_src, s_dest);
	} // next

	targ = &texture_shader_dependency_list;
	for (int idx = 0; idx < targ->numstrings; idx ++) {
		char *s_src = targ->strings[idx];
		char s_dest[MAX_QPATH_128];
		c_dpsnprintf2 (s_dest, "%s/%s", s_folder_dest_write, s_src);
		written_bytes += FS_FileCopy_Shitty (s_src, s_dest);
	} // next


file_copy_end:
	// Need to write the scripts ...
	//char s_scripts_out[MAX_QPATH_128];
	//char s_scripts_out[MAX_QPATH_128];

	//char s_num[MAX_QPATH_128];
	c_dpsnprintf1 (s_num, "%f", host_hoststartup_unique_num);
	String_Edit_Replace (s_num, sizeof(s_num), ".", "_");
	c_dpsnprintf2 (s_scripts_out, "%s/scripts/map_%s.shader", s_folder_dest_write, s_num);

	c_dpsnprintf1 (s_scripts_out, "%s/scripts/map.shader", s_folder_dest_write);
	//qfile_t *
	f = FS_OpenRealFile (s_scripts_out, "wb", fs_quiet_FALSE);  // WRITE-EON Baker: pak_this_map
	if (!f) {
		Con_PrintLinef ("Couldn't open file " QUOTED_S, s_scripts_out);
		goto script_write_fail;
	}

	targ = &shader_textblock_dependency_list;
	for (int idx = 0; idx < targ->numstrings; idx ++) {
		char *s_textblock = targ->strings[idx];
		FS_Printf (f, "%s" NEWLINE, s_textblock);
	} // next
	FS_Close (f); f = NULL;


script_write_fail:
test_is_done:
	stringlistfreecontents (&texture_dependency_list);
	stringlistfreecontents (&texture_shader_dependency_list);
	stringlistfreecontents (&shader_textblock_dependency_list);
	stringlistfreecontents (&shader_name_list);

	if (is_test == false)
		Con_PrintLinef ("Wrote %s bytes", String_Num_To_Thousands_Sbuf((int)written_bytes));
}
