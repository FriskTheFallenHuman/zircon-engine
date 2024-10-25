// jack_scripts.c.h

static char *s_fake_src_sa;
static char *s_fake_cursor;
static char *s_line_sa;

void sfake_start (char *s)
{
	setstr (s_fake_src_sa, s);
	s_fake_cursor = s_fake_src_sa;
//	_fake_pos = 0;
//	_fake_len = strlen(s);
}

void sfake_close ()
{
	//StrClear (_s_fake_src_sa);
	freenull_ (s_fake_src_sa)
	freenull_ (s_line_sa)
	s_fake_cursor = NULL;
}


int sfake_is_eof ()
{
	//printvarf (_fake_len);
	//printvarf (_fake_pos);
	if (!s_fake_cursor) return true;
	return (*s_fake_cursor) == 0;
}

// is this allocated or not?  Yes allocated, however caller need not deal with freeing it.  We do all that.
char *sfake_getlin () {
	if (!s_fake_cursor) return NULL;
	if (! (*s_fake_cursor) ) return NULL;
	const char *sstart = s_fake_cursor;

	if (s_line_sa) {
		freenull_ (s_line_sa);
	}

	int slinelen;
	char *sendline = strstr(s_fake_cursor, NEWLINE);
	if (sendline) {
		slinelen = sendline - s_fake_cursor;
		s_fake_cursor += slinelen;
		s_fake_cursor += ONE_CHAR_1;

	} else {
		slinelen = (int)strlen (s_fake_cursor);
		s_fake_cursor += slinelen;
	}

	char *sline = dpstrndup (sstart, slinelen);

	setstr (s_line_sa, sline);

	freenull_ (sline)
	return s_line_sa;

}

void CleanShader (const char *srel) // scripts/mine.shader
{
	char *s_clean_sa = NULL, *s_sa = NULL;
	// J.A.C.K. doesn't know these shaders, we strip to speed up load times
	const char *scrub_list[] = {
		//"framemap",	// buttonmap
		//"animmap",
		"dp_camera",
		"dp_reflect",
		"dp_reflectcube",
		"dp_refract",
		"dp_water",
		"dpcamera",
		"dpglossexponentmod",
		"dpglossintensitymod",
		"dpnoshadow",
		"dpreflect",
		"dpreflectcube",
		"dprefract",
		"dpwater",
        "dpoffsetmapping",
		"qer_alphaFunc",
// March 1 2024 ...
		"surfaceparm noimpact",
		"surfaceparm nolightmap",
		"distancecull",
		//"nonsolid",
		"deformVertexes roundwave",
		"q3map_bounceScale",

	};

	// 0 -> 1, 2 -> 3
	const char *substitute_list[] = {
		"framemap ",	"map ", // buttonmap
		"animmap",		"animMap ", // buttonmap
	};

	char *sfile = String_Find_Skip_Past(srel, "/");// String_Skip_Char(srel, '/');
	//Con_PrintLinef ("sfile %s", sfile);

	char swrite[SYSTEM_STRING_SIZE_1024];
	dpsnprintf (swrite, sizeof(swrite), "_jack_scripts/%s", sfile);

	Con_PrintLinef ("Writing %s", swrite);

	qfile_t *f = FS_OpenRealFile(swrite, "wb", fs_quiet_FALSE); // WRITE-EON -- Baker: CleanShader for jack_scripts

	if (!f) {
		Con_PrintLinef ("ERROR: couldn't open.");
		goto fail_without_cleanup;
	}

	s_sa = (char *)FS_LoadFile (srel, tempmempool, fs_quiet_FALSE, fs_size_ptr_null);

	if (!s_sa) goto fail_without_cleanup;

	s_clean_sa = String_Replace_Malloc (s_sa, CARRIAGE_RETURN, "");

	Mem_Free(s_sa);

	sfake_start (s_clean_sa);

	int scrub_list_count = ARRAY_COUNT(scrub_list);
	int substitute_list_count = ARRAY_COUNT(substitute_list);

	int linenum = 0;
	// Baker: This looks at each line, if it contains a shader listed above, it doesn't write that line.
	while (sfake_is_eof() == false) {
		const char *s_this_line = sfake_getlin();
		char editbuffer[1024];
		//Con_PrintLinef ("%04d: %s", linenum, s_this_line);
		//if (linenum == 293)
		//	int j = 5;
		linenum ++;
		if (!s_this_line) break;
		int is_ok = true;
		
		for (int idx = 0; idx < scrub_list_count; idx ++) {
			const char *s_scrub_this = scrub_list[idx];
			if (String_Contains_Caseless (s_this_line, s_scrub_this)) {
				is_ok = false;
				break;
			} // found shader not recognized by J.A.C.K. do not write
		}  // idx

		// 0 becomes 1, 2 becomes 3.
		for (int idx = 0; idx < substitute_list_count; idx += 2) {
			const char *s_replace_this = substitute_list[idx + 0];
			const char *s_with = substitute_list[idx + 1];
			if (String_Contains_Caseless (s_this_line, s_replace_this)) {
				c_strlcpy (editbuffer,  s_this_line);
				String_Edit_Replace (editbuffer, sizeof(editbuffer), s_replace_this, s_with); 
				s_this_line = editbuffer; // Write this instead.
				break; // We can only do 1
			} // found shader not recognized by J.A.C.K. do not write
		}  // idx

		if (is_ok) {
			FS_Printf (f, "%s" NEWLINE, s_this_line);
		} // else skip that line
	}

	sfake_close ();
	freenull_ (s_clean_sa);

fail_without_cleanup:
	if (f)
		FS_Close (f);
	return;
}

void Con_Jack_Scripts_f(cmd_state_t *cmd)
{
	char		spattern[SYSTEM_STRING_SIZE_1024] = "scripts/*.shader";
	fssearch_t *asearch = FS_Search (spattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (!asearch) {
		Con_PrintLinef ("Nothing for %s", spattern);
		return;
	}

	const char *swrite = "scripts/shaderlist.txt";
	qfile_t *fshadertxt = FS_OpenRealFile(swrite, "wb", fs_quiet_FALSE); // WRITE-EON -- jack scripts
	if (fshadertxt) {
		FS_Printf (fshadertxt, "%s"  NEWLINE, "// Auto-generated from Zircon using " QUOTED_STR("jack_scripts") " command");
		FS_Printf (fshadertxt, "%s"  NEWLINE, "// q3map uses this.  Map editors usually don't.  DarkPlaces/Zircon don't.");
		FS_Printf (fshadertxt, "%s"  NEWLINE, "// This list must be up to date for q3map to know of a shader.");
		FS_Printf (fshadertxt, "%s"  NEWLINE, "// we can pile up several shader files, the one in <gamedir>/scripts and ones in the mod dir or other spots");
	}

	int j;
	for (j = 0; j < asearch->numfilenames; j++) {
		const char *s_file = asearch->filenames[j];
		Con_PrintLinef ("%s", s_file);
		//if (String_Contains (s_file, "common") == false)
		//	continue;

		CleanShader (asearch->filenames[j]);

		if (fshadertxt) {
			char *sfile = String_Find_Skip_Past (asearch->filenames[j], "/");
			char *sfile2 = File_URL_Edit_Remove_Extension (sfile);

			FS_Printf (fshadertxt, "%s" NEWLINE, sfile2);
		} // fshadertxt

	}
	FS_FreeSearch(asearch);

	if (fshadertxt) {
		Con_PrintLinef ("Updated %s", swrite);
		FS_Close (fshadertxt);
	}

	//gamedir/_jack_shaders

 //   Kill_JackShaders
 //   Gen_JackShaders
 //   MakeShaderListTxt

	return;
}
