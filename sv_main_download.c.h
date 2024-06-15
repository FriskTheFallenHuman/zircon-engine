// sv_main_download.c.h

// Baker: Describe the DarkPlaces download process

// What does a chunk start download do?

// QW: "downloads maps/subterfuge.bsp"
// What is the DarkPlaces "during process"
// svc_downloaddata -> clc_ackdownloaddata

WARP_X_ (CL_ParseServerMessage -> svc_downloaddata    -> CL_ParseDownload_DP)
WARP_X_ (SV_ReadClientMessage  -> clc_ackdownloaddata -> SV_ReadClientMessage)

WARP_X_ (downloadx_sv_frame: "sv_startdownload" starts one)
WARP_X_ (downloadx_sv_start_download)
WARP_X_ (download maps/alcyone.bsp -> "cl_downloadbegin" SV_Download_f)

// Find svc_download in ezQuake for chunked what's it do?

// ezQ SV Download start 
	//if (sv_client->fteprotocolextensions & FTE_PEXT_CHUNKEDDOWNLOADS)
	//{
	//	ClientReliableWrite_Begin (sv_client, svc_download, 10+strlen(name));
	//	ClientReliableWrite_Long (sv_client, -1); // <----------------- neg -1 size and name
	//	ClientReliableWrite_Long (sv_client, sv_client->downloadsize);
	//	ClientReliableWrite_String (sv_client, name);
	//}
// ezQ SV_Download has a deny downloadsize is -1, we don't need.
// There is a SV_StopDownload that issues a -3

// Baker: What does a CL chunk download request look like
// 
// QW_CL_SendClientCommandf (q_is_reliable_false, "nextdl %d %d %d", chunk_wanted, 
//								cls.qw_downloadpercent, chunked_download_number_key);



WARP_X_ (CL: 	#0		"download maps/alcyone.bsp") // CL sends that to server
WARP_X_ (SV:	downloadx_sv_start_download				SV_Download_f	
		 "cl_downloadbegin 3232323 maps/alcyone.bsp"	)
WARP_X_ (CL:	downloadx_cl_start_download_1			CL_DownloadBegin_DP_f -> "sv_startdownload")
WARP_X_ (SV:	downloadx_sv_start_download_2			SV_StartDownload_f host_client->download_started = true)
WARP_X_ (SV:	downloadx_sv_start_download_during_1	SV_SendClientDatagram)
WARP_X_ (SV:	downloadx_cl_start_download_during_2	svc_downloaddata -> replies clc_ackdownloaddata)
WARP_X_ (SV:	downloadx_sv_start_download_during_3	clc_ackdownloaddata -> might do cl_downloadfinished)
WARP_X_ (CL:	downloadx_sv_finish_0					"cl_downloadfinished size crc models/filename.bsp")
WARP_X_ (CL:	downloadx_cl_finish_1					CL_DownloadFinished_DP_f)

// Baker: Our chunked process for DarkPlaces
// 1. CL: "download"
// 2. SV: SV_Download_f (open file), send "cl_downloadbegin [size bytes] model/map.bsp"
// 3. CL: "sv_download"
// 4. SV: host_client->download_started = true)
           // ... I suggest sending chunk -1 or whatever here, so client knows to spam away
// 5. downloadx_sv_start_download_during_1:  Chunked do nothing
// 6. downloadx_cl_start_download_during_2:  We won't be doing those.
// 7. downloadx_sv_start_download_during_3:  Again, nope.

// 8. CL_Input.c ask for chunks.
// 9. What occurs at 100%
//		MSG_WriteByte		(&buf, clc_ackdownloaddata);
//		MSG_WriteLong		(&buf, cls.dp_downloadack[i].start); // Give it the file size
//		MSG_WriteShort		(&buf, cls.dp_downloadack[i].size);

// SV: "cl_downloadfinished %d %d %s"

// Describe the next download process?
// sv sends ""cl_downloadfinished"
//              CL ... CL_DownloadFinished_DP_f does "download xxx xxx" to server.
//   

//SV: clc_ackdownloaddata


// The "cl_downloadfinished" might do a "next download"
WARP_X_ (CL:	downloadx_cl_finish_2_write_file		CL_StopDownload_DP )

// "stopdownload" 
// We can stop a download by doing "download f" where f is some file that doesn't exist.
// There is not a mechanism to stop the download chaining of "cl_downloadfinished"


// Finished?

// "stopdownload" is a client thing the server says when done


WARP_X_ (DP more data -> SV_SendClientMessages -> for cl -> SV_SendClientDatagram ->)

WARP_X_ (QW_CL_CheckOrDownloadFile)
// Where Quakeworld ask for download?  QW_CL_CheckOrDownloadFile
// Where Quakeworld do tempfile?  It's there!
// How does DarkPlaces Client know filename if we type "download"

WARP_X_ (CL_BeginDownloads_DP)
WARP_X_ (cl.loadmodel_total .. set to nummodels cl.loadsound_total)
WARP_X_ (svc_downloaddata -> CL_ParseDownload_DP CL_DownloadFinished_DP_f)
WARP_X_ (CL_Input.c sends clc_ackdownloaddata -> )
WARP_X_ (CL_StopDownload_DP also writes the download to file plus rescans file system)

WARP_X_ (If server ack size >= filesize, "cl_downloadfinished %d %d %s")

// Sends
// CL_ForwardToServerf ("download %s", cl.model_name[cl.downloadmodel_current]);

// 



// SIGNON_1
//   CL sends: "cl_begindownloads"
//   During:
//     
//	 SV sends: "cl_downloadfinished" 

//Cmd_AddCommand(CF_CLIENT, "cl_begindownloads", CL_BeginDownloads_DP_f, "used internally by darkplaces client while connecting (causes loading of models and sounds or triggers downloads for missing ones)");

//Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "cl_downloadbegin", CL_DownloadBegin_DP_f, "(networking) informs client of download file information, client replies with sv_startsoundload to begin the transfer");
//Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "stopdownload", CL_StopDownload_DP_QW_f, "terminates a download");
//Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "cl_downloadfinished", CL_DownloadFinished_DP_f, "signals that a download has finished and provides the client with file size and crc to check its integrity");


//// ezQuake "nextdl"
//// What does ezQuake do to start a download?
//// "download" .. think of "download" as open file.
//static void Cmd_NextDownload_f (void)
//{
//	byte    buffer[FILE_TRANSFER_BUF_SIZE];
//	int     r, tmp;
//	int     percent;
//	int     size;
//	double  frametime;
//
//	if (!sv_client->download)
//		return;
//
//#ifdef FTE_PEXT_CHUNKEDDOWNLOADS // DX NextDownload
//	if (sv_client->fteprotocolextensions & FTE_PEXT_CHUNKEDDOWNLOADS)
//	{
//		SV_NextChunkedDownload(atoi(Cmd_Argv(1)), atoi(Cmd_Argv(2)), atoi(Cmd_Argv(3)));
//		return;
//	}
//#endif
//
//	tmp = sv_client->downloadsize - sv_client->downloadcount;
//
//	frametime = max(0.05, min(0, sv_client->netchan.frame_rate));
//	//Sys_Printf("rate:%f\n", sv_client->netchan.frame_rate);
//
//	r = (int)((curtime + frametime - sv_client->netchan.cleartime)/sv_client->netchan.rate);
//	if (r <= 10)
//		r = 10;
//	if (r > FILE_TRANSFER_BUF_SIZE)
//		r = FILE_TRANSFER_BUF_SIZE;
//
//	// don't send too much if already buffering
//	if (sv_client->num_backbuf)
//		r = 10;
//
//	if (r > tmp)
//		r = tmp;
//
//	Con_DPrintf("Downloading: %d", r);
//	r = VFS_READ(sv_client->download, buffer, r, NULL);
//	Con_DPrintf(" => %d, total: %d => %d", r, sv_client->downloadsize, sv_client->downloadcount);
//
//baker_this_is_download_frame:
//	ClientReliableWrite_Begin (sv_client, svc_download, 6 + r);
//	ClientReliableWrite_Short (sv_client, r);
//	sv_client->downloadcount += r;
//	if (!(size = sv_client->downloadsize))
//		size = 1;
//	percent = (sv_client->downloadcount * (double)100.) / size;
//	if (percent == 100 && sv_client->downloadcount != sv_client->downloadsize)
//		percent = 99;
//	else if (percent != 100 && sv_client->downloadcount == sv_client->downloadsize)
//		percent = 100;
//	Con_DPrintf("; %d\n", percent);
//	ClientReliableWrite_Byte (sv_client, percent);
//	ClientReliableWrite_SZ (sv_client, buffer, r);
//	sv_client->file_percent = percent; //bliP: file percent
//
//	if (sv_client->downloadcount == sv_client->downloadsize)
//		SV_CompleteDownoload();
//}



// qqshka: percent is optional, u can't relay on it

// Quakeworld repeatedly spams "nextdl" to get more chunks
//void SV_NextChunkedDownload (int chunknum, int percent, int chunked_download_number_key_cl)
static void SV_Chunk_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) != 3) {
		Con_PrintLinef ("SV_Chunk_f arg count not 3");
		return;
	}



#define CHUNKSIZE_1024 1024
	int chunknum						= atoi(Cmd_Argv(cmd, 1));
	int chunked_download_number_key_cl	= atoi(Cmd_Argv(cmd, 2));
	int sv_maxchunks_per_frame = bound(1, (int)sv_downloadchunksperframe.value /*D 15*/, 1000);

#if 0 // Huts
	//KILLME Con_PrintLinef ("DPCHUNKS - SV_Chunk_f %d key:%d", chunknum, chunked_download_number_key_cl);
#endif

	// Check if too much requests or client sent something wrong
	if (host_client->download_chunks_perframe >= sv_maxchunks_per_frame)
		return;
	
	if (chunked_download_number_key_cl < 1) {
		Con_PrintLinef ("SV_Chunk_f key < 1");
		return;
	}

	// Baker: Not sure what to do here.  Does DarkPlaces use user rate somehow?
	if (!host_client->download_chunks_perframe && /*rate throttle logic*/ 0) {
		// Some rate throttle logic would go here
		return; // ?
	}
		
	
	if (FS_Seek (host_client->download_file, chunknum * CHUNKSIZE_1024, SEEK_SET)) {
		Con_PrintLinef ("SV_Chunk_f FS_Seek didn't return 0, (READ ERROR) for chunknum %d", chunknum);
		return; // FIXME: ERROR of some kind
	}

	unsigned char chunk_data_1024[CHUNKSIZE_1024] = {0};
	int result = FS_Read (host_client->download_file, chunk_data_1024, CHUNKSIZE_1024);

	if (result <= 0) {
		Con_PrintLinef ("SV_Chunk_f FS_Read result <=0 %d chunknum %d", result, chunknum);
		goto skip_out; // FIXME: EOF/READ ERROR
	}

	byte	bufdata [
		4 +							// \377\377\377\377 aka -1 (LONG)
		1 +							// A2C_PRINT
		(sizeof("\\chunk")-1) +		// \chunk
		4 +							// (int) chunked_download_number_key_cl
		1 +							// (byte) svc_download
		4 +							// (int) chunknumber
		CHUNKSIZE_1024]; // byte + (sizeof("\\chunk")-1) + long + byte + long + CHUNKSIZE_1024
	
	sizebuf_t buf;
	memset (&buf, 0, sizeof(buf));
	buf.data	= bufdata;
	buf.maxsize = sizeof(bufdata);

	MSG_WriteLong				(&buf, -1); // Control packet or whatever
	MSG_WriteByte				(&buf, QW_A2C_PRINT_char_n);
	MSG_WriteUnterminatedString	(&buf, "\\chunk");
	MSG_WriteLong				(&buf, chunked_download_number_key_cl); // return back, so they sure what it proper chunk
	MSG_WriteByte				(&buf, qw_svc_download);
	MSG_WriteLong				(&buf, chunknum);
	SZ_Write					(&buf, chunk_data_1024, CHUNKSIZE_1024);

	NetConn_Write (
		host_client->netconnection->mysocket,		// socket
		buf.data, buf.cursize,						// data, length
		&host_client->netconnection->peeraddress	// destination
	);
	//KILLME Con_PrintLinef ("DPCHUNKS - SV Send of %d", chunknum); // Hits

skip_out:
	host_client->download_chunks_perframe ++;
}


WARP_X_ (clc_ackdownloaddata svc_downloaddata)


WARP_X_ (CL_BeginDownloads_Prespawn_Zircon_Extensions_Send)		// 
WARP_X_ (CL_BeginDownloads_DP)										// "cl_begindownloads" SIGNON_1 will go to 2
WARP_X_ (CL_BeginDownloads_Prespawn_Zircon_Extensions_Send)

// Baker: There is no cl_download, right?

WARP_X_ (host_client->netconnection->mysocket)
//void SV_Chunk_f (cmd_state_t *cmd)
//{
//	if (Cmd_Argv(cmd) != 4)
//		return;
//
//	int maxchunks_per_frame = bound(1, sv_downloadchunksperframe.integer, 30);
//
//	int chunknum						= atoi(Cmd_Argv(cmd, 1));
//	int percent							= atoi(Cmd_Argv(cmd, 2));
//	int chunked_download_number_key_cl	= atoi(Cmd_Argv(cmd, 3));
//	
//	// Check if too much requests or client sent something wrong
//	if (host_client->download_chunks_perframe >= maxchunks || chunked_download_number_key_cl < 1)
//		return;
//
//	if (0) {
//		// Some host_client-> rate limiter stuff
//	}
//
//	if (chunknum < 0) {
//		// Signal it's done
//		// Server should close file, stop download
//		goto skip_out;
//	}
//
//	if (FS_Seek (host_client->download_file, chunknum * CHUNKSIZE_1024, SEEK_SET))
//		return; // FIXME: ERROR of some kind
//
//	unsigned char chunk_data_1024[CHUNKSIZE_1024] = {0};
//	int i = FS_Read (host_client->download_file, chunk_data_1024, CHUNKSIZE_1024);
//
//	if (i <= 0)
//		goto skip_out; // FIXME: EOF/READ ERROR
//
//	byte	bufdata [
//		1 +							// A2C_PRINT
//		STRINGLEN("\\chunk") +		// \chunk
//		4 +							// (int) chunked_download_number_key_cl
//		1 +							// (byte) svc_download
//		4 +							// (int) chunknumber
//		CHUNKSIZE_1024]; // byte + (sizeof("\\chunk")-1) + long + byte + long + CHUNKSIZE_1024
//	
//	sizebuf_t buf;
//	memset (&buf, 0, sizeof(buf));
//	buf.data	= bufdata;
//	buf.maxsize = sizeof(bufdata);
//
//	MSG_WriteByte				(&buf, QW_A2C_PRINT_char_n);
//	MSG_WriteUnterminatedString	(&buf, "\\chunk");
//	MSG_WriteLong				(&buf, chunked_download_number_key_cl); // return back, so they sure what it proper chunk
//	MSG_WriteByte				(&buf, qw_svc_download);
//	MSG_WriteLong				(&buf, chunknum);
//	SZ_Write					(&buf, chunk_data_1024, CHUNKSIZE_1024);
//
//	NetConn_Write (
//		host_client->netconnection->mysocket,		// socket
//		buf.data, buf.cursize,						// data, length
//		&host_client->netconnection->peeraddress	// destination
//	);
//
//skip_out:
//	host_client->download_chunks_perframe++;
//}


WARP_X_ ("download maps/mymap.bsp chunked")
static void SV_Download_f(cmd_state_t *cmd)
{
	const char *whichpack, *whichpack2, *extension;
	qbool is_csqc_progs_file; // so we need to check only once

	int is_dpchunks = Cmd_Argc(cmd) >= 3 && 
		String_Match (Cmd_Argv(cmd, 2), "chunked"); // 

	host_client->download_chunked = is_dpchunks ? true : false;

	if (Cmd_Argc(cmd) < 2) {
		SV_ClientPrintf		("usage: download <filename> {<extensions>}*" NEWLINE);
		SV_ClientPrintf		("       supported extensions: deflate" NEWLINE);
		return;
	}

	Con_DPrintLinef ("SV_Download_f %s chunked ? %d %s", Cmd_Argv(cmd, 1), 
		is_dpchunks, 
		Cmd_Argv(cmd, 2)
	);

	//Con_DPrintLinef ("argc: %d", Cmd_Argc(cmd));
	//Con_DPrintLinef ("arg0: %s", Cmd_Argv(cmd,0));
	//Con_DPrintLinef ("arg1: %s", Cmd_Argv(cmd,1));
	//Con_DPrintLinef ("arg2: %s", Cmd_Argv(cmd,2));
	//Con_DPrintLinef ("arg3: %s", Cmd_Argv(cmd,3));

	const char *s_download_file = Cmd_Argv(cmd, 1);

	if (FS_CheckNastyPath(s_download_file, /*isgamedir?*/ false)) {
		SV_ClientPrintf		("Download rejected: nasty filename " QUOTED_S NEWLINE, s_download_file);
		return;
	}

	if (host_client->download_file) {
		// at this point we'll assume the previous download should be aborted
		Con_DPrintLinef ("Download of %s aborted by %s starting a new download", host_client->download_name, host_client->name);
		SV_ClientCommandsf(NEWLINE "stopdownload" NEWLINE);

		// close the file and reset variables
		FS_Close(host_client->download_file);
		host_client->download_file = NULL;
		host_client->download_name[0] = 0;
		host_client->download_expectedposition = 0;
		host_client->download_started = false;
	}

	is_csqc_progs_file = (sv.csqc_progname[0] && String_Match(s_download_file, sv.csqc_progname));
	
	if (!sv_allowdownloads.integer && !is_csqc_progs_file) {
		SV_ClientPrintf			("Downloads are disabled on this server" NEWLINE);
		SV_ClientCommandsf		(NEWLINE "stopdownload" NEWLINE);
		return;
	}

	Download_CheckExtensions(cmd); // Looks for "deflate" in args, sets host_client->download_deflate

	c_strlcpy (host_client->download_name, s_download_file);
	extension = FS_FileExtension(host_client->download_name);

	// host_client is asking to download a specified file
	if (developer_extra.integer)
		Con_DPrintLinef ("Download request for %s by %s", host_client->download_name, host_client->name);

	if (is_csqc_progs_file) {
		char extensions[MAX_QPATH_128]; // make sure this can hold all extensions
		extensions[0] = '\0';
		
		if (host_client->download_deflate)
			c_strlcat (extensions, " deflate");
		
		if (is_dpchunks)
			c_strlcat (extensions, " chunked");
			
		Con_DPrintLinef ("Downloading %s to %s", host_client->download_name, host_client->name);

		if (host_client->download_deflate && svs.csqc_progdata_deflated)
			host_client->download_file = FS_FileFromData(svs.csqc_progdata_deflated, svs.csqc_progsize_deflated, true);
		else
			host_client->download_file = FS_FileFromData(svs.csqc_progdata, sv.csqc_progsize, true);
		
downloadx_sv_start_download:
		// no, no space is needed between %s and %s :P
		WARP_X_ (CL_DownloadBegin_DP_f)
		SV_ClientCommandsf		(NEWLINE "cl_downloadbegin %d %s%s" NEWLINE, 
			(int)FS_FileSize(host_client->download_file), host_client->download_name, extensions);

		host_client->download_expectedposition = 0;
		host_client->download_started = false;
		host_client->sendsignon = true; // make sure this message is sent
		return;
	}

	if (!FS_FileExists(host_client->download_name)) {
		SV_ClientPrintf			("Download rejected: server does not have the file " QUOTED_S NEWLINE 
								"You may need to separately download or purchase the data archives for this game/mod to get this file"  NEWLINE, 
								host_client->download_name);
		SV_ClientCommandsf		(NEWLINE "stopdownload" NEWLINE); WARP_X_ (CL_StopDownload_DP)
		return;
	}

	// check if the user is trying to download part of registered Quake(r)
	whichpack = FS_WhichPack(host_client->download_name);
	whichpack2 = FS_WhichPack("gfx/pop.lmp");

	// Baker r0104: No one is going to use this insane method to steal Quake in 2023 (see above for #if 9)

	// check if the server has forbidden archive downloads entirely
	if (!sv_allowdownloads_inarchive.integer) {
		whichpack = FS_WhichPack(host_client->download_name);
		if (whichpack) {
			SV_ClientPrintf		("Download rejected: file " QUOTED_S " is in an archive (" QUOTED_S ")" NEWLINE 
								 "You must separately download or purchase the data archives for this game/mod to get this file" NEWLINE,
								host_client->download_name, whichpack);
								SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
			return;
		}
	}

	// Baker: sv_allowdownloads_config defaults 0
	if (!sv_allowdownloads_config.integer) {
		// This is the norm
		if (String_Match_Caseless(extension, "cfg")) {
			SV_ClientPrintf		("Download rejected: file " QUOTED_S " is a .cfg file which is forbidden for security reasons\nYou must separately download or purchase the data archives for this game/mod to get this file" NEWLINE, host_client->download_name);
			SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
			return;
		}
	}

	// Baker: sv_allowdownloads_dlcache defaults 0
	if (!sv_allowdownloads_dlcache.integer) {
		// This is the norm
		if (String_Starts_With_Caseless_PRE (host_client->download_name, "dlcache/")) {
			SV_ClientPrintf		("Download rejected: file " QUOTED_S " is in the dlcache/ directory "
									"which is forbidden for security reasons" NEWLINE
									"You must separately download or purchase the data archives for this game/mod to get this file" NEWLINE, 
									host_client->download_name);
			SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
			return;
		}
	}

	if (!sv_allowdownloads_archive.integer) {
		if (String_Match_Caseless (extension, "pak") || String_Match_Caseless(extension, "pk3") || String_Match_Caseless(extension, "dpk")) {
			SV_ClientPrintf		("Download rejected: file " QUOTED_S " is an archive" NEWLINE 
								  "You must separately download or purchase the data archives for this game/mod to get this file" NEWLINE, 
								  host_client->download_name);
			SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
			return;
		}
	}

	host_client->download_file = FS_OpenVirtualFile(host_client->download_name, fs_quiet_true);
	if (!host_client->download_file) {
		SV_ClientPrintf		("Download rejected: server could not open the file " QUOTED_S NEWLINE, host_client->download_name);
		SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
		return;
	}

	// Baker: 1,073,741,824 - 1 GB limit
	if (FS_FileSize(host_client->download_file) > 1<<30) {
		SV_ClientPrintf		("Download rejected: file " QUOTED_S " is very large" NEWLINE, host_client->download_name);
		SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
		FS_Close			(host_client->download_file);
		host_client->download_file = NULL;
		return;
	}

	if (FS_FileSize(host_client->download_file) < 0) {
		SV_ClientPrintf		("Download rejected: file " QUOTED_S " is not a regular file" NEWLINE, host_client->download_name);
		SV_ClientCommandsf	(NEWLINE "stopdownload" NEWLINE);
		FS_Close			(host_client->download_file);
		host_client->download_file = NULL;
		return;
	}

	Con_DPrintLinef ("Downloading %s to player named %s", host_client->download_name, host_client->name);

	/*
	 * we can only do this if we would actually deflate on the fly
	 * which we do not (yet)!
	{
		char extensions[MAX_QPATH_128]; // make sure this can hold all extensions
		extensions[0] = '\0';
		
		if (host_client->download_deflate)
			strlcat(extensions, " deflate", sizeof(extensions));

		// no, no space is needed between %s and %s :P
		SV_ClientCommandsf("\ncl_downloadbegin %d %s%s\n", (int)FS_FileSize(host_client->download_file), host_client->download_name, extensions);
	}
	*/

	WARP_X_ (CL_DownloadBegin_DP_f)
	WARP_X_ (SV_StartDownload_f)
	SV_ClientCommandsf	(NEWLINE "cl_downloadbegin %d %s%s" NEWLINE, 
								(int)FS_FileSize(host_client->download_file), 
								host_client->download_name, is_dpchunks ? " chunked" : "");

	host_client->download_expectedposition = 0;
	host_client->download_started = false;
	host_client->sendsignon = true; // make sure this message is sent

	// the rest of the download process is handled in SV_SendClientDatagram
	// and other code dealing with svc_downloaddata and clc_ackdownloaddata
	//
	// no svc_downloaddata messages will be sent until sv_startdownload is
	// sent by the client
}
