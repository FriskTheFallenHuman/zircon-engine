// sv_main_precache.c.h

	// /*
	// // DarkPlaces extended savegame
	// sv.lightstyles 0 m
	// sv.lightstyles 63 a
	// sv.model_precache 1 maps/z2m1_somewhere.bsp
	// sv.sound_precache 1 weapons/r_exp3.wav

	char *text = (char *)FS_LoadFile (sloadgame , tempmempool, false, NULL, NOLOADINFO_IN_NULL, NOLOADINFO_OUT_NULL); // AXX1 START
	if (!text) {
		Host_Error ("ERROR: couldn't open %s.\n", sloadgame );
	}

	const char *scomment = dpstrrstr(text, "/*");
	if (scomment) {

        const char *slastmodel, *mt = slastmodel = dpstrrstr(text, "sv.model_precache");
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
                Con_PrintLinef("Loadgame has more models (%d) than QC precaches (%d) ", mc0, mc);
            }

            const char *slastsou, *ms = slastsou = dpstrrstr(text, "sv.sound_precache");
            if (slastsou) {

                // sv.sound_precache 114 zirco/door_keypad_error.wav
                COM_ParseToken_Simple(&ms, false, false, false); // sv.sound_precache
                COM_ParseToken_Simple(&ms, false, false, false); // 114

                int sc0 =	atoi(com_token) + 1;  // expect 115

                if (sc0 > sc) {
                    preparse = 1;
                    Con_PrintLinef("Loadgame has more sound (%d) than QC precaches (%d) ", sc0, sc);
                }

                //sv.model_precache 80 models/zircon_dynamics/apple.md3
                //sv.sound_precache 114 zirco/door_keypad_error.wav
                if (preparse) {



                    const char /**sfirstmodel, */ *tm = strstr(scomment, "sv.model_precache");

                    Con_PrintLinef("Precaching late models/sounds");

                    while (COM_ParseToken_Simple(&tm, false, false, true)) {
                        if (String_Does_Match(com_token, "sv.lightstyles")) {
                            COM_ParseToken_Simple(&tm, false, false, true);
                            jj = atoi(com_token);
                            COM_ParseToken_Simple(&tm, false, false, true);
                        } else if (String_Does_Match(com_token, "sv.model_precache")) {
                            COM_ParseToken_Simple(&tm, false, false, true);
                            jj = atoi(com_token);
                            COM_ParseToken_Simple(&tm, false, false, true);
                            if (jj >= 0 && jj < MAX_MODELS) {
                                if (jj >= mc) {
                                    Con_PrintLinef("Precaching %s", com_token);
                                    strlcpy(sv.model_precache[jj], com_token, sizeof(sv.model_precache[jj]));
                                    sv.models[jj] = Mod_ForName (sv.model_precache[jj], true, false, sv.model_precache[jj][0] == '*' ? sv.worldname : NULL);
                                }
                            } else Con_Printf("unsupported model %d \"%s\"\n", jj, com_token);
                        } else if (String_Does_Match(com_token, "sv.sound_precache")) {
                            COM_ParseToken_Simple(&tm, false, false, true);
                            jj = atoi(com_token);
                            COM_ParseToken_Simple(&tm, false, false, true);
                            if (jj >= 0 && jj < MAX_SOUNDS) {
                                if (jj >= sc) {
                                    Con_PrintLinef("Precaching %s", com_token);
                                    strlcpy(sv.sound_precache[jj], com_token, sizeof(sv.sound_precache[jj]));
                                }
                            } else Con_Printf("unsupported sound %d \"%s\"\n", jj, com_token);
                        } else if (String_Does_Match(com_token, "sv.buffer")) {
                            if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                jj = atoi(com_token);
                                if (jj >= 0) {
                                    //k = STRINGBUFFER_SAVED;
                                    if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                        //k |= atoi(com_token);
                                    }

                                    //if (!BufStr_FindCreateReplace(prog, jj, k, "string"))
                                    //	Con_Printf("failed to create stringbuffer %jj\n", jj);
                                }
                                else
                                    Con_Printf("unsupported stringbuffer index %jj \"%s\"\n", jj, com_token);
                            }
                            else
                                Con_Printf("unexpected end of line when parsing sv.buffer (expected buffer index)\n");
                        }
                        else if (String_Does_Match(com_token, "sv.bufstr"))
                        {
                            if (!COM_ParseToken_Simple(&tm, false, false, true))
                                Con_Printf("unexpected end of line when parsing sv.bufstr\n");
                            else {
                                jj = atoi(com_token);
                                //stringbuffer = BufStr_FindCreateReplace(prog, jj, STRINGBUFFER_SAVED, "string");
                                if (1) { //stringbuffer)

                                    if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                        //k = atoi(com_token);
                                        if (COM_ParseToken_Simple(&tm, false, false, true)) {
                                            //BufStr_Set(prog, stringbuffer, k, com_token);
                                        } else Con_Printf("unexpected end of line when parsing sv.bufstr (expected string)\n");
                                    } else Con_Printf("unexpected end of line when parsing sv.bufstr (expected strindex)\n");
                                } else Con_Printf("failed to create stringbuffer %jj \"%s\"\n", jj, com_token);
                            }
                        }
                        // skip any trailing text or unrecognized commands
                        while (COM_ParseToken_Simple(&tm, true, false, true) && strcmp(com_token, "\n"))
                            ; // wh
                    } // wh
                } // preparse
            } // slastsou
        } // slastmodel
    } // scomment

	Mem_Free(text);

