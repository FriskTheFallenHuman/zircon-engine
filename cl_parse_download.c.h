// cl_parse_download.c.h

WARP_X_ (SV_Download_f clc_ackdownloaddata svc_downloaddata -> CL_ParseDownload_DP)

WARP_X_ (SV_Zircon_Extensions_Send)
static void CL_BeginDownloads_Prespawn_Zircon_Extensions_Send (void) // @ SIGNON_1 will go to 2
{
	//char vabuf[1024];
	// Do we know protocol yet?
	WARP_X_ (SV_PreSpawn_f)
	if (cls.protocol == PROTOCOL_DARKPLACES7 && cl_pext.integer) {
		WARP_X_ (SV_PreSpawn_f)
		Con_DPrintLinef ("CL_BeginDownloads SIGNON_1: Telling server our extensions %d", CLIENT_SUPPORTED_ZIRCON_EXT);
		//c_dpsnprintf1 (vabuf, "prespawn %d", CLIENT_SUPPORTED_ZIRCON_EXT);
		CL_ForwardToServerf ("prespawn %d", cl_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0); // ZIRCON_PEXT CL TO SV MOVE TO SIGNON_2 QW does not come here // HITS
		if (cl_pext.integer == 0)
			Con_PrintLinef (CON_RED "Warning: " CON_WHITE " Client protocol extensions disabled"); 
	} else
		CL_ForwardToServerf ("prespawn"); // MOVE TO SIGNON_2 QW does not come here // HITS

}

WARP_X_ (CL_SignonReply SIGNON_1 sends us here)
// Quakeworld comes here, but leaves

// Baker: Might as well be "next download"
static void CL_BeginDownloads_DP (qbool aborteddownload)
{
	char vabuf[1024];
	// quakeworld works differently
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		return; // Baker: This never hits although CL_StopDownload_DP_QW_f might be able to come here

	// this would be a good place to do curl downloads
	if (Curl_Have_forthismap()) {
		Curl_Register_predownload(); // come back later
		return;
	}

	// if we got here...
	// curl is done, so let's start with the business
	if (!cl.loadbegun)
		SCR_PushLoadingScreen("Loading precaches", 1);
	cl.loadbegun = true;

	// if already downloading something from the previous level, don't stop it
	if (cls.qw_downloadname[0])
		return;

	if (cl.downloadcsqc) {
		size_t progsize;
		cl.downloadcsqc = false;
		if (cls.netcon
		 && !sv.active
		 && csqc_progname.string
		 && csqc_progname.string[0]
		 && csqc_progcrc.integer >= 0
		 && cl_serverextension_download.integer
		 && (FS_CRCFile(csqc_progname.string, &progsize) != csqc_progcrc.integer || ((int)progsize != csqc_progsize.integer && csqc_progsize.integer != -1))
		 && !FS_FileExists(va(vabuf, sizeof(vabuf), "dlcache/%s.%d.%d", csqc_progname.string, csqc_progsize.integer, csqc_progcrc.integer)))
		{
			Con_PrintLinef ("Downloading new CSQC code to dlcache/%s.%d.%d", csqc_progname.string, csqc_progsize.integer, csqc_progcrc.integer);

			// Baker: cl_serverextension_download defaults 0
			if (cl_serverextension_download.integer /*0*/ == 2 && FS_HasZlib())
				// NOT THE NORM
				CL_ForwardToServerf ("download %s deflate", csqc_progname.string);
			else {
				// THE NORM
				CL_ForwardToServerf ("download %s", csqc_progname.string);
			}
			return;
		}
	}

	if (cl.loadmodel_current < cl.loadmodel_total) {
		// loading models
		if (cl.loadmodel_current == 1) {
			// worldmodel counts as 16 models (15 + world model setup), for better progress bar
			SCR_PushLoadingScreen("Loading precached models",
				(
					(cl.loadmodel_total - 1) * LOADPROGRESSWEIGHT_MODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL_INIT
				) / (
					(cl.loadmodel_total - 1) * LOADPROGRESSWEIGHT_MODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL_INIT
				+	cl.loadsound_total * LOADPROGRESSWEIGHT_SOUND
				)
			);
		}
		for (; cl.loadmodel_current < cl.loadmodel_total;cl.loadmodel_current++) {
			SCR_PushLoadingScreen(cl.model_name[cl.loadmodel_current],
				(
					(cl.loadmodel_current == 1) ? LOADPROGRESSWEIGHT_WORLDMODEL : LOADPROGRESSWEIGHT_MODEL
				) / (
					(cl.loadmodel_total - 1) * LOADPROGRESSWEIGHT_MODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL_INIT
				)
			);
			if (cl.model_precache[cl.loadmodel_current] && cl.model_precache[cl.loadmodel_current]->Draw)
			{
				SCR_PopLoadingScreen(false);
				if (cl.loadmodel_current == 1)
				{
					SCR_PushLoadingScreen(cl.model_name[cl.loadmodel_current], 1.0 / cl.loadmodel_total);
					SCR_PopLoadingScreen(false);
				}
				continue;
			}
			CL_KeepaliveMessage(true);

			// if running a local game, calling Mod_ForName is a completely wasted effort...
			if (sv.active)
				cl.model_precache[cl.loadmodel_current] = sv.models[cl.loadmodel_current];
			else
			{
				if (cl.loadmodel_current == 1)
				{
					// they'll be soon loaded, but make sure we apply freshly downloaded shaders from a curled pk3
					Mod_FreeQ3Shaders();
				}
				cl.model_precache[cl.loadmodel_current] = Mod_ForName(cl.model_name[cl.loadmodel_current], false, false, cl.model_name[cl.loadmodel_current][0] == '*' ? cl.model_name[1] : NULL);
			}
			SCR_PopLoadingScreen(false);
			if (cl.model_precache[cl.loadmodel_current] && cl.model_precache[cl.loadmodel_current]->Draw && cl.loadmodel_current == 1)
			{
				// we now have the worldmodel so we can set up the game world
				SCR_PushLoadingScreen("world model setup",
					(
						LOADPROGRESSWEIGHT_WORLDMODEL_INIT
					) / (
						(cl.loadmodel_total - 1) * LOADPROGRESSWEIGHT_MODEL
					+	LOADPROGRESSWEIGHT_WORLDMODEL
					+	LOADPROGRESSWEIGHT_WORLDMODEL_INIT
					)
				);
				CL_SetupWorldModel();
				SCR_PopLoadingScreen(true);
				// cl_joinbeforedownloadsfinish - is the norm
				if (!cl.loadfinished && cl_joinbeforedownloadsfinish.integer) {
					cl.loadfinished = true;
					// now issue the spawn to move on to signon 2 like normal
					CL_BeginDownloads_Prespawn_Zircon_Extensions_Send ();
				} // if
			}
		} // for models
		SCR_PopLoadingScreen(false);
		// finished loading models
	}

	if (cl.loadsound_current < cl.loadsound_total) {
		// loading sounds
		if (cl.loadsound_current == 1)
			SCR_PushLoadingScreen("Loading precached sounds",
				(
					cl.loadsound_total * LOADPROGRESSWEIGHT_SOUND
				) / (
					(cl.loadmodel_total - 1) * LOADPROGRESSWEIGHT_MODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL
				+	LOADPROGRESSWEIGHT_WORLDMODEL_INIT
				+	cl.loadsound_total * LOADPROGRESSWEIGHT_SOUND
				)
			);
		for (;cl.loadsound_current < cl.loadsound_total;cl.loadsound_current++)
		{
			SCR_PushLoadingScreen(cl.sound_name[cl.loadsound_current], 1.0 / cl.loadsound_total);
			if (cl.sound_precache[cl.loadsound_current] && S_IsSoundPrecached(cl.sound_precache[cl.loadsound_current]))
			{
				SCR_PopLoadingScreen(false);
				continue;
			}
			CL_KeepaliveMessage(true);
			cl.sound_precache[cl.loadsound_current] = S_PrecacheSound(cl.sound_name[cl.loadsound_current], false, true);
			SCR_PopLoadingScreen(false);
		}
		SCR_PopLoadingScreen(false);
		// finished loading sounds
	}

	if (IS_NEXUIZ_DERIVED(gamemode))
		Cvar_SetValueQuick(&cl_serverextension_download, false);
		// in Nexuiz/Xonotic, the built in download protocol is kinda broken (misses lots
		// of dependencies) anyway, and can mess around with the game directory;
		// until this is fixed, only support pk3 downloads via curl, and turn off
		// individual file downloads other than for CSQC
		// on the other end of the download protocol, GAME_NEXUIZ/GAME_XONOTIC enforces writing
		// to dlcache only
		// idea: support download of pk3 files using this protocol later

	// note: the reason these loops skip already-loaded things is that it
	// enables this command to be issued during the game if desired

	if (cl.downloadmodel_current < cl.loadmodel_total) {
		// loading models

		for (;cl.downloadmodel_current < cl.loadmodel_total;cl.downloadmodel_current++) {
			if (aborteddownload) {
				if (cl.downloadmodel_current == 1) {
					// the worldmodel failed, but we need to set up anyway
					Mod_FreeQ3Shaders();
					CL_SetupWorldModel();
					if (!cl.loadfinished && cl_joinbeforedownloadsfinish.integer) {
						cl.loadfinished = true;
						// now issue the spawn to move on to signon 2 like normal
						if (cls.netcon)
							CL_BeginDownloads_Prespawn_Zircon_Extensions_Send ();
					}
				}
				aborteddownload = false;
				continue;
			} // aborted
			if (cl.model_precache[cl.downloadmodel_current] && cl.model_precache[cl.downloadmodel_current]->Draw)
				continue;
			CL_KeepaliveMessage(true);
			if (cl.model_name[cl.downloadmodel_current][0] != '*' && strcmp(cl.model_name[cl.downloadmodel_current], "null") && !FS_FileExists(cl.model_name[cl.downloadmodel_current])) {
				if (cl.downloadmodel_current == 1)
					Con_PrintLinef ("Map %s not found", cl.model_name[cl.downloadmodel_current]);
				else
					Con_PrintLinef ("Model %s not found", cl.model_name[cl.downloadmodel_current]);
				// regarding the * check: don't try to download submodels
				if (cl_serverextension_download.integer /*sv 2*/ && cls.netcon &&
					cl.model_name[cl.downloadmodel_current][0] != '*' && !sv.active) {
					if (Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE)
							CL_ForwardToServerf ("download %s chunked", cl.model_name[cl.downloadmodel_current]);
					else	CL_ForwardToServerf ("download %s", cl.model_name[cl.downloadmodel_current]);
					// we'll try loading again when the download finishes
					return;
				}
			}

			if (cl.downloadmodel_current == 1) {
				// they'll be soon loaded, but make sure we apply freshly downloaded shaders from a curled pk3
				Mod_FreeQ3Shaders();
			}

			cl.model_precache[cl.downloadmodel_current] = Mod_ForName(cl.model_name[cl.downloadmodel_current], false, true, cl.model_name[cl.downloadmodel_current][0] == '*' ? cl.model_name[1] : NULL);
			if (cl.downloadmodel_current == 1) {
				// we now have the worldmodel so we can set up the game world
				// or maybe we do not have it (cl_serverextension_download 0)
				// then we need to continue loading ANYWAY!
				CL_SetupWorldModel();
				if (!cl.loadfinished && cl_joinbeforedownloadsfinish.integer) {
					cl.loadfinished = true;
					// now issue the spawn to move on to signon 2 like normal
					if (cls.netcon)
						CL_BeginDownloads_Prespawn_Zircon_Extensions_Send ();
				} // if
			} // if
		} // for models

		// finished loading models
	}

	if (cl.downloadsound_current < cl.loadsound_total) {
		// loading sounds

		for (;cl.downloadsound_current < cl.loadsound_total;cl.downloadsound_current++) {
			char soundname[MAX_QPATH_128];
			if (aborteddownload) {
				aborteddownload = false;
				continue;
			}
			if (cl.sound_precache[cl.downloadsound_current] && S_IsSoundPrecached(cl.sound_precache[cl.downloadsound_current]))
				continue;
			CL_KeepaliveMessage(true);
			dpsnprintf(soundname, sizeof(soundname), "sound/%s", cl.sound_name[cl.downloadsound_current]);
			if (!FS_FileExists(soundname) && !FS_FileExists(cl.sound_name[cl.downloadsound_current])) {
				if (cl_serverextension_download.integer && cls.netcon && !sv.active) {
					//CL_ForwardToServerf(va(vabuf, sizeof(vabuf), "download %s", soundname));
					if (Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE)
							CL_ForwardToServerf ("download %s chunked", soundname);
					else	CL_ForwardToServerf ("download %s", soundname);

					// we'll try loading again when the download finishes
					return;
				}
			}
			cl.sound_precache[cl.downloadsound_current] = S_PrecacheSound(cl.sound_name[cl.downloadsound_current], false, true);
		} // sounds

		// finished loading sounds
	}

	SCR_PopLoadingScreen(false);

	if (!cl.loadfinished) {
		cl.loadfinished = true;

		// check memory integrity
		Mem_CheckSentinelsGlobal();

		// now issue the spawn to move on to signon 2 like normal
		if (cls.netcon)
			CL_BeginDownloads_Prespawn_Zircon_Extensions_Send ();
	} // if
}

static void CL_BeginDownloads_DP_f(cmd_state_t *cmd)
{
	// prevent cl_begindownloads from being issued multiple times in one match
	// to prevent accidentally cancelled downloads
	if (cl.loadbegun)
		Con_PrintLinef ("cl_begindownloads is only valid once per match");
	else
		CL_BeginDownloads_DP (q_is_aborted_download_false);
}

void Baker_CL_Download_Start_DP (const char *s_name, int size)
{
	// Baker: QW sets the name in different ways
	if (s_name) {
		c_strlcpy (cls.qw_downloadname, s_name);
	}
	cls.qw_downloadmemorymaxsize = size;

	if (cls.qw_downloadmemory) {
		Mem_Free(cls.qw_downloadmemory);
		cls.qw_downloadmemory = NULL;
		Con_PrintLinef (CON_RED "Had to free download memory first?");
	}

	cls.qw_downloadmemory = (unsigned char *) Mem_Alloc(cls.permanentmempool,
		cls.qw_downloadmemorymaxsize);
}

WARP_X_CALLERS_ (QW_CL_ParseDownload yesh)
void Baker_CL_Download_During_ReadMsg_Size_Enlarge_QW (int size)
{
	// make sure the buffer is big enough to include this new fragment
	if (!cls.qw_downloadmemory || cls.qw_downloadmemorymaxsize <
		cls.qw_downloadmemorycursize + size) {
		unsigned char *old;
		while (cls.qw_downloadmemorymaxsize < cls.qw_downloadmemorycursize + size)
			cls.qw_downloadmemorymaxsize *= 2;
		old = cls.qw_downloadmemory;
		cls.qw_downloadmemory = (unsigned char *)Mem_Alloc(cls.permanentmempool, cls.qw_downloadmemorymaxsize);
		if (old)
		{
			memcpy(cls.qw_downloadmemory, old, cls.qw_downloadmemorycursize);
			Mem_Free(old);
		}
	}

	// read the fragment out of the packet
	MSG_ReadBytes(&cl_message, size, cls.qw_downloadmemory + cls.qw_downloadmemorycursize);
	cls.qw_downloadmemorycursize += size;
	cls.qw_downloadspeedcount += size;
}

WARP_X_CALLERS_ (CL_ParseDownload_DP)
void Baker_CL_Download_During_Data_Start_Size_DP_And_Chunks (void *data, int start, int size)
{
	if (!cls.qw_downloadmemory || cls.qw_downloadmemorymaxsize <
		start /*cls.qw_downloadmemorycursize*/ + size)
		Host_Error_Line ("Allocate the damn memory for the download fool");
	memcpy (cls.qw_downloadmemory + start, data, size);
	cls.qw_downloadmemorycursize = start + size; // Baker: For chunks this is irrelevant
	cls.qw_downloadpercent = (int)floor((start+size) * 100.0 / cls.qw_downloadmemorymaxsize);
	cls.qw_downloadpercent = bound(0, cls.qw_downloadpercent, 100);
	cls.qw_downloadspeedcount += size;


}

void Baker_CL_Download_FWrite (int size, int crc)
{
	const char *extension;
	if (crc != not_found_neg1)
		Con_PrintLinef ("Downloaded " QUOTED_S " (%d bytes, %d CRC)", cls.qw_downloadname, size, crc); // DP
	else
		Con_PrintLinef ("Downloaded " QUOTED_S, cls.qw_downloadname); // QW
	FS_WriteFile(cls.qw_downloadname, cls.qw_downloadmemory, cls.qw_downloadmemorymaxsize);
	extension = FS_FileExtension(cls.qw_downloadname);
	if (String_Isin3_Caseless (extension, "pak", "pk3", "dpk"))
		FS_Rescan();
}

// Baker: DarkPlaces only
static void CL_StopDownload_DP(int size, int crc)
{
	if (cls.qw_downloadmemory &&
		cls.qw_downloadmemorycursize == size &&
		CRC_Block(cls.qw_downloadmemory, cls.qw_downloadmemorycursize) == crc) {
		int existingcrc;
		size_t existingsize;

		if (cls.qw_download_deflate) {
			unsigned char *out_inflated; // deflate
			size_t inflated_size;
			out_inflated = FS_Inflate(cls.qw_downloadmemory, cls.qw_downloadmemorycursize, &inflated_size, tempmempool);
			Mem_Free(cls.qw_downloadmemory);
			if (out_inflated) {
				Con_PrintLinef ("Inflated download: new size: %u (%g%%)", (unsigned)inflated_size, 100.0 - 100.0*(cls.qw_downloadmemorycursize / (float)inflated_size));
				cls.qw_downloadmemory = out_inflated;
				cls.qw_downloadmemorycursize = (int)inflated_size;
			}
			else
			{
				cls.qw_downloadmemory = NULL;
				cls.qw_downloadmemorycursize = 0;
				Con_PrintLinef ("Cannot inflate download, possibly corrupt or zlib not present");
			}
		}

		if (!cls.qw_downloadmemory) {
			Con_PrintLinef ("Download " QUOTED_S " is corrupt (see above!)", cls.qw_downloadname);
		}
		else {

			crc = CRC_Block(cls.qw_downloadmemory, cls.qw_downloadmemorycursize);
			size = cls.qw_downloadmemorycursize;


			// finished file
			// save to disk only if we don't already have it
			// (this is mainly for playing back demos)
			existingcrc = FS_CRCFile(cls.qw_downloadname, &existingsize);
			if (existingsize || IS_NEXUIZ_DERIVED(gamemode) || String_Match(cls.qw_downloadname, csqc_progname.string))
				// let csprogs ALWAYS go to dlcache, to prevent "viral csprogs"; also, never put files outside dlcache for Nexuiz/Xonotic
			{
				if ((int)existingsize != size || existingcrc != crc) {
					// we have a mismatching file, pick another name for it
					char name[MAX_QPATH_128*2];
					dpsnprintf(name, sizeof(name), "dlcache/%s.%d.%d", cls.qw_downloadname, size, crc);
					if (!FS_FileExists(name))
					{
						Con_PrintLinef ("Downloaded " QUOTED_S " (%d bytes, %d CRC)", name, size, crc);
downloadx_cl_finish_2_write_file:
						FS_WriteFile(name, cls.qw_downloadmemory, cls.qw_downloadmemorycursize);
						if (String_Match(cls.qw_downloadname, csqc_progname.string))
						{
							if (cls.caughtcsprogsdata)
								Mem_Free(cls.caughtcsprogsdata);
							cls.caughtcsprogsdata = (unsigned char *) Mem_Alloc(cls.permanentmempool, cls.qw_downloadmemorycursize);
							memcpy(cls.caughtcsprogsdata, cls.qw_downloadmemory, cls.qw_downloadmemorycursize);
							cls.caughtcsprogsdatasize = cls.qw_downloadmemorycursize;
							Con_DPrintLinef ("Buffered " QUOTED_S, name);
						}
					}
				}
			}
			else
			{
				// we either don't have it or have a mismatching file...
				// so it's time to accept the file
				// but if we already have a mismatching file we need to rename
				// this new one, and if we already have this file in renamed form,
				// we do nothing
#if 1
				Baker_CL_Download_FWrite (size, crc);
#else
				Con_PrintLinef ("Downloaded " QUOTED_S " (%d bytes, %d CRC)", cls.qw_downloadname, size, crc);
				FS_WriteFile(cls.qw_downloadname, cls.qw_downloadmemory, cls.qw_downloadmemorycursize);
				extension = FS_FileExtension(cls.qw_downloadname);
				if (String_Isin3_Caseless (extension, "pak", "pk3", "dpk"))
					FS_Rescan();
#endif
			}
		}
	}
	else if (cls.qw_downloadmemory && size)
	{
		Con_PrintLinef ("Download " QUOTED_S " is corrupt (%d bytes, %d CRC, should be %d bytes, %d CRC), discarding", cls.qw_downloadname, size, crc, (int)cls.qw_downloadmemorycursize, (int)CRC_Block(cls.qw_downloadmemory, cls.qw_downloadmemorycursize));
		CL_BeginDownloads_DP (q_is_aborted_download_true);
	}

	// Baker_CL_Download_Clear
	if (cls.qw_downloadmemory)
		Mem_Free(cls.qw_downloadmemory);
	cls.qw_downloadmemory = NULL;
	cls.qw_downloadmethod = DL_NONE_0;
	cls.qw_downloadname[0] = 0;
	cls.qw_downloadmemorymaxsize = 0;
	cls.qw_downloadmemorycursize = 0;
	cls.qw_downloadpercent = 0;
}

WARP_X_ (svc_downloaddata)
// Baker: How does this transition when 100% is hit?
static void CL_ParseDownload_DP(void)
{
	int start, size;
	static unsigned char data[NET_MAXMESSAGE_65536];
	start = MSG_ReadLong(&cl_message);
	size = (unsigned short)MSG_ReadShort(&cl_message);

	// record the start/size information to ack in the next input packet
	for (int j = 0; j < CL_MAX_DOWNLOADACKS_DP_QW_4; j++) {
		if (!cls.dp_downloadack[j].start && !cls.dp_downloadack[j].size) {
			cls.dp_downloadack[j].start = start;
			cls.dp_downloadack[j].size = size;
			break;
		}
	}

	if (developer_qw.integer)
		Con_PrintLinef ("CL_ParseDownload_DP size %d", (int)size);

	MSG_ReadBytes (&cl_message, size, data);

	if (!cls.qw_downloadname[0]) {
		if (size > 0)
			Con_PrintLinef ("CL_ParseDownload_DP: received %d bytes with no download active", size);
		return;
	}

	if (start + size > cls.qw_downloadmemorymaxsize)
		Host_Error_Line ("corrupt download message");

	// only advance cursize if the data is at the expected position
	// (gaps are unacceptable)
#if 1
	Baker_CL_Download_During_Data_Start_Size_DP_And_Chunks (data, start, size);
#else
	memcpy (cls.qw_downloadmemory + start, data, size);
	cls.qw_downloadmemorycursize = start + size;
	cls.qw_downloadpercent = (int)floor((start+size) * 100.0 / cls.qw_downloadmemorymaxsize);
	cls.qw_downloadpercent = bound(0, cls.qw_downloadpercent, 100);
	cls.qw_downloadspeedcount += size;
#endif
}

// Baker: "cl_downloadbegin"
// The server sends this as a reply to the client sending "download maps/aerowalk.bsp"

// PREPARE THE CLIENT TO RECEIVE A FILE
// Reply to the server "sv_startdownload"

// SVCHUNKS: 100% - Although, is it chunks?
// Baker: I don't think we need to care.
static void CL_DownloadBegin_DP_f(cmd_state_t *cmd)
{
//KILLME Con_PrintLinef ("DPCHUNKS - cl_downloadbegin");

	int size = atoi(Cmd_Argv(cmd, 1));

	if (size < 0 || size > 1<<30 || FS_CheckNastyPath(Cmd_Argv(cmd, 2), false)) {
		Con_PrintLinef ("cl_downloadbegin: received bogus information");
		CL_StopDownload_DP(0, 0);
		return;
	}

	if (cls.qw_downloadname[0])
		Con_PrintLinef ("Download of %s aborted", cls.qw_downloadname);

	CL_StopDownload_DP(0, 0);

downloadx_cl_start_download_1: // Prepare to receive a file

	// we're really beginning a download now, so initialize stuff
	//c_strlcpy (cls.qw_downloadname, Cmd_Argv(cmd, 2));

	if (developer_qw.integer)
		Con_PrintLinef ("CL_DownloadBegin_DP_f");

	const char *s_name = Cmd_Argv(cmd, 2);
	Baker_CL_Download_Start_DP (s_name, size); // Allocates memory, set name and size
	cls.qw_downloadnumber++;

	cls.qw_downloadmethod = DL_QW_1; // Baker: The important part it isn't "chunked"

	cls.qw_download_deflate = false;
	if (Cmd_Argc(cmd) >= 4) {
		if (String_Match(Cmd_Argv(cmd, 3), "deflate"))
			cls.qw_download_deflate = true;
		// check further encodings here
	}

	if (Cmd_Argc(cmd) >= 4 && String_Match(Cmd_Argv(cmd, 3), "chunked")) {
		CL_DPChunks_CL_BeginDownload_Start (size);
	}

	WARP_X_ (SV_StartDownload_f)
	CL_ForwardToServerf ("sv_startdownload");
}


// "stopdownload"
static void CL_StopDownload_DP_QW_f(cmd_state_t *cmd)
{
	//KILLME Con_PrintLinef ("DPCHUNKS - stopdownload");
	Curl_CancelAll();

#ifdef FTE_PEXT_CHUNKEDDOWNLOADS
	if (sv_client->fteprotocolextensions & FTE_PEXT_CHUNKEDDOWNLOADS)
	{
		char *name = ""; // FIXME: FTE's chunked dl does't support "cmd stopdl", work around

		ClientReliableWrite_Begin (sv_client, svc_download, 10+strlen(name));
		ClientReliableWrite_Long (sv_client, -1);
		ClientReliableWrite_Long (sv_client, -3); // -3 = dl was stopped
		ClientReliableWrite_String (sv_client, name);
	}
	else
#endif

	if (cls.qw_downloadname[0]) {
		Con_PrintLinef ("Download of %s aborted", cls.qw_downloadname);
		CL_StopDownload_DP(0, 0);
	}
	CL_BeginDownloads_DP(q_is_aborted_download_true);
}

// "cl_downloadfinished"
static void CL_DownloadFinished_DP_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) < 3) {
		Con_PrintLinef ("Malformed cl_downloadfinished command");
		return;
	}
	//KILLME Con_PrintLinef ("DPCHUNKS - cl_downloadfinished");
//downloadx_cl_finish_1:
	int filesize	= atoi(Cmd_Argv(cmd, 1));
	int filecrc		= atoi(Cmd_Argv(cmd, 2));
	CL_StopDownload_DP		(filesize, filecrc);
	CL_BeginDownloads_DP	(q_is_aborted_download_false); // Baker: Might as well be "next download"
}
