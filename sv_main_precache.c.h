// sv_main_precache.c.h

	// /*
	// // DarkPlaces extended savegame
	// sv.lightstyles 0 m
	// sv.lightstyles 63 a
	// sv.model_precache 1 maps/z2m1_somewhere.bsp
	// sv.sound_precache 1 weapons/r_exp3.wav

	char *text = (char *)FS_LoadFile (sloadgame, tempmempool, fs_quiet_FALSE, fs_size_ptr_null);
	if (!text) {
		Host_Error_Line ("ERROR: couldn't open %s", sloadgame );
	}

	const char *scomment = dp_strstr_reverse(text, "/*");
	if (scomment) {

        const char *slastmodel, *mt = slastmodel = dp_strstr_reverse(text, "sv.model_precache");
        if (slastmodel) {
            int mc =	SV_ModelIndex_Count ();	// expect 80
            int sc =	SV_SoundIndex_Count (); // expect 114?

            COM_ParseToken_Simple(&mt, false, false, false); // sv.model_precache
            COM_ParseToken_Simple(&mt, false, false, false); // 80

            int mc0 =	atoi(com_token) + 1;  // expect 81
            int preparse = 0;
            int jj = 0;

            if (mc0 > mc) {
                preparse = 1;
                Con_PrintLinef (CON_CYAN, "Loadgame has more models (%d) than QC precaches (%d) ", mc0, mc);
            }

            const char *slastsou, *ms = slastsou = dp_strstr_reverse(text, "sv.sound_precache");
            if (slastsou) {
                // sv.sound_precache 114 zirco/door_keypad_error.wav
                COM_ParseToken_Simple (&ms, false, false, false); // sv.sound_precache
                COM_ParseToken_Simple (&ms, false, false, false); // 114

                int sc0 =	atoi(com_token) + 1;  // expect 115

                if (sc0 > sc) {
                    preparse = true;
                    Con_PrintLinef (CON_CYAN "Loadgame has more sound (%d) than QC precaches (%d) ", sc0, sc);
                }

				// Looks like:
                //		sv.model_precache 80 models/zircon_dynamics/apple.md3
                //		sv.sound_precache 114 zirco/door_keypad_error.wav
                if (preparse) {
                    const char /**sfirstmodel, */ *tm = strstr(scomment, "sv.model_precache");

                    Con_PrintLinef (CON_CYAN "Precaching late models/sounds");

                    while (COM_Parse_Basic(&tm)) {
                        if (String_Match(com_token, "sv.lightstyles")) {
                            COM_Parse_Basic (&tm);
                            jj = atoi(com_token);
                            COM_Parse_Basic (&tm);
                        } else if (String_Match(com_token, "sv.model_precache")) {
                            COM_Parse_Basic (&tm);
                            jj = atoi(com_token);
                            COM_Parse_Basic (&tm);
                            if (jj >= 0 && jj < MAX_MODELS_8192) {
                                if (jj >= mc) {
                                    Con_PrintLinef (CON_CYAN "Precaching %s", com_token);
                                    c_strlcpy (sv.model_precache[jj], com_token);
                                    sv.models[jj] = Mod_ForName (sv.model_precache[jj], true, false, sv.model_precache[jj][0] == '*' ? sv.worldname : NULL);
                                }
                            } else Con_PrintLinef (CON_CYAN "unsupported model %d " QUOTED_S, jj, com_token);
                        } else if (String_Match(com_token, "sv.sound_precache")) {
                            COM_Parse_Basic (&tm);
                            jj = atoi(com_token);
                            COM_Parse_Basic (&tm);
                            if (jj >= 0 && jj < MAX_SOUNDS_4096) {
                                if (jj >= sc) {
                                    Con_PrintLinef (CON_CYAN "Precaching %s", com_token);
                                    c_strlcpy (sv.sound_precache[jj], com_token);
                                }
                            } else Con_PrintLinef (CON_CYAN "unsupported sound %d " QUOTED_S, jj, com_token);
                        } else if (String_Match(com_token, "sv.buffer")) {
                            if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                jj = atoi(com_token);
                                if (jj >= 0) {
                                    if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                    }
                                }
                                else
                                    Con_PrintLinef (CON_CYAN "unsupported stringbuffer index %d " QUOTED_S, jj, com_token);
                            }
                            else
                                Con_PrintLinef (CON_CYAN "unexpected end of line when parsing sv.buffer (expected buffer index)");
                        }
                        else if (String_Match(com_token, "sv.bufstr"))
                        {
                            if (!COM_ParseToken_Simple(&tm, false, false, true))
                                Con_PrintLinef (CON_CYAN "unexpected end of line when parsing sv.bufstr");
                            else {
                                jj = atoi(com_token);
                                //stringbuffer = BufStr_FindCreateReplace(prog, jj, STRINGBUFFER_SAVED, "string");
                                if (1) { //stringbuffer)

                                    if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                        //k = atoi(com_token);
                                        if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                            //BufStr_Set(prog, stringbuffer, k, com_token);
                                        } else Con_PrintLinef (CON_CYAN "unexpected end of line when parsing sv.bufstr (expected string)");
                                    } else Con_PrintLinef (CON_CYAN "unexpected end of line when parsing sv.bufstr (expected strindex)");
                                } else Con_PrintLinef (CON_CYAN "failed to create stringbuffer %d " QUOTED_S, jj, com_token);
                            }
                        }
                        // skip any trailing text or unrecognized commands
                        while (COM_ParseToken_Simple(&tm, true, false, true) && String_NOT_Match (com_token, NEWLINE))
                            ; // wh
                    } // wh
                } // preparse
            } // slastsou
        } // slastmodel
    } // scomment

	Mem_Free(text);

