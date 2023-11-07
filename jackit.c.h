// jackit.c.h

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
	freenull3_ (s_fake_src_sa)
	freenull3_ (s_line_sa)
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
		freenull3_ (s_line_sa)
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

	freenull3_ (sline)
	return s_line_sa;

}

void CleanShader (const char *srel) // scripts/mine.shader
{
	char *s_clean_sa = NULL, *s_sa = NULL;
	// J.A.C.K. doesn't know these shaders, we strip to speed up load times
	const char *scrub[] = {
		"animmap"
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
	};

	char *sfile = String_Find_Skip_Past(srel, "/");// String_Skip_Char(srel, '/');
	//Con_PrintLinef ("sfile %s", sfile);

	char swrite[SYSTEM_STRING_SIZE_1024];
	dpsnprintf (swrite, sizeof(swrite), "_jack_scripts/%s", sfile);

	Con_PrintLinef ("Writing %s", swrite);

	qfile_t *f = FS_OpenRealFile(swrite, "wb", false);

	if (!f) {
		Con_Print("ERROR: couldn't open.\n");
		goto fail;
	}

	s_sa = (char *)FS_LoadFile (srel, tempmempool, false, NULL, NOLOADINFO_IN_NULL, NOLOADINFO_OUT_NULL); // AXX1 START

	if (!s_sa) goto fail;

	s_clean_sa = String_Replace_Alloc (s_sa, "\r", "");

	Mem_Free(s_sa); // AXX1 END

	sfake_start (s_clean_sa);

	while (sfake_is_eof() == false) {
		const char *sx = sfake_getlin();
		if (!sx) break;
		int is_ok = true;
		int qua = ARRAY_COUNT (scrub);
		int j;
		for (j = 0; j < qua; j ++) {
			const char *sf = scrub[j];
			if (String_Does_Contain_Caseless (sx, sf)) {
				is_ok = false;
				break;
			} // found shader not recognized by J.A.C.K. do not write
		}  // j
		if (is_ok) {
			//Con_PrintLinef ("%s", sx);
			FS_Printf (f,"%s\n", sx);
		} // else skip that line
	}

	sfake_close ();
	freenull3_ (s_clean_sa);

fail:
	if (f)
		FS_Close (f);
	return;
}
void SCR_Jackit_f(void)
{
	WARP_X_ (GetXList_Count  GetMapList Host_Savegame_to Host_Loadgame_f)

	char		spattern[SYSTEM_STRING_SIZE_1024] = "scripts/*.shader";
	fssearch_t *asearch = FS_Search(spattern, true, true, gamedironly_false);

	if (!asearch) {
		Con_PrintLinef ("Nothing for %s", spattern);
		return;
	}

	const char *swrite = "scripts/shaderlist.txt";
	qfile_t *fshadertxt = FS_OpenRealFile(swrite, "wb", false);
	if (fshadertxt) {
		FS_Printf (fshadertxt, "%s\n", "// Auto-generated from Zircon using \"jackit\" command");
		FS_Printf (fshadertxt, "%s\n", "// q3map uses this.  Map editors usually don't.  DarkPlaces/Zircon don't.");
		FS_Printf (fshadertxt, "%s\n", "// This list must be up to date for q3map to know of a shader.");
		FS_Printf (fshadertxt, "%s\n", "// we can pile up several shader files, the one in <gamedir>/scripts and ones in the mod dir or other spots");
	}

	int j;
	for (j = 0; j < asearch->numfilenames; j++) {
		//Con_PrintLinef ("%s", asearch->filenames[j]);
		CleanShader (asearch->filenames[j]);

		if (fshadertxt) {
			char *sfile = String_Find_Skip_Past (asearch->filenames[j], "/");
			char *sfile2 = File_URL_Edit_Remove_Extension (sfile);

			FS_Printf (fshadertxt, "%s\n", sfile2);
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
