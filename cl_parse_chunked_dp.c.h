// cl_parse_chunked_dp.c.h



// Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE DPCHUNKS

// Baker: Our chunked process for DarkPlaces
// 1. CL: "download maps/aerowalk.bsp chunked"

// This expresses our preferences for chunks to the server.

// 2. SV: SV_Download_f (open file), sends
//		"cl_downloadbegin 3232323 maps/aerowalk.bsp deflate chunked" (HAS CHUNKS)
//		"cl_downloadbegin 3232321 maps/aerowalk.bsp deflate"
//  IF SERVER DOES NOT HAVE FILE OR REJECTS?
//  We get a "stopdownload" .. which might go to next download.  It also writes the file.
WARP_X_ (SV_Download_f, CL_StopDownload_DP_QW_f)

// Where is the 100% check on the server?
//"cl_downloadfinished"


WARP_X_ (CL_DownloadBegin_DP_f )
WARP_X_ (CL_DPChunks_CL_ParseChunkedDownload )
WARP_X_ (CL_DPChunks_CL_BeginDownload_Start )
WARP_X_ (CL_DPChunks_CL_SendChunkDownloadReq )
// 3. CL: If "chunked" is in the reply, prepare the chunked receive mechanism reseting it.
//        CL_DPChunks_CL_BeginDownload_Start

// If we are chunks, the server will not be sending download data!

// We will be sending:
// "chunk chunknumber chunkkey" in out-of-band unreliable packets. Asking for what we need.

// The server will reply up to 30 times a frame chunks that we request.
// These chunks will be in out-of-band unreliable packets.

// 

// clc_ackdownloaddata

// When we have the last chunk ...

//		1) we send "chunk -1 -1" (RELIABLE), so the server knows it can close the file.
//		2) We write the file to disk
//	    3) We should proceed to the next file in queue, if there is a queue.

// QW_CL_SendClientCommandf(q_is_reliable_true, "nextdl %d %d %d", chunk_wanted, cls.qw_downloadpercent, chunked_download_number_key


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


WARP_X_CALLERS_ (CL_DownloadBegin_DP_f)
//	if (Cmd_Argc(cmd) >= 4 && String_Match(Cmd_Argv(cmd, 4), "chunked"))
//		CL_DPChunks_CL_BeginDownload_Start (size);


// Prepare client chunk stuff
void CL_DPChunks_CL_BeginDownload_Start (int download_size)
{
	// Baker: Initialize chunked data
	cls.qw_downloadmethod  = DL_DPCHUNKS_3;
	cls.qw_downloadpercent = 0;
	cls.qw_downloadstarttime = Sys_DirtyTime ();

	chunked_download_number_key++;

	chunked_downloadsize     = download_size;

	firstblock				= 0;
	chunked_receivedbytes	= 0;
	blockcycle				= -1;	//so it requests 0 first. :)
	memset (recievedblock, 0, sizeof(recievedblock));
}

WARP_X_ (CL_SendMove)
//	MSG_WriteByte	A2C_PRINT
//	SZ_Write		"\\chunk"
//	Long			chunked_download_number_key
//	Byte			qw_svc_download_41
//	Long			chunknum;
//  Bytes[1024]		(chunk data)


WARP_X_CALLERS_ (NetConn_ClientParsePacket -> QW_CL_Parse_OOB_ChunkedDownload)
void CL_DPChunks_CL_ParseChunkedDownload (int is_oob)
{
	int chunknum;
	unsigned char data[DLBLOCKSIZE_1024];
	double tm;


	chunknum = MSG_ReadLong(&cl_message);
	//if (000001) Con_PrintLinef ("Chunknum %d (oob %d)", chunknum, is_oob);

	MSG_ReadBytes (&cl_message, DLBLOCKSIZE_1024, data);

	if (cls.qw_downloadmethod != DL_DPCHUNKS_3) {
		Con_PrintLinef ("Dropping OOB DP chunked message, no download active");
		return;
	}

	if (!cls.qw_downloadmemory) {
		// Baker: How would this happen?
		return;
	}

	// Baker: Does chunked download get recorded into a demo?  YES OR NO?
	// Baker: Does the svc_download stuff get recorded into a demo? YES OR NO?
	if (cls.demoplayback) {
		// Err, yeah, when playing demos we don't actually pay any attention to this.
		return;
	}

	if (chunknum < firstblock) {
		if (developer_qw.integer) Con_PrintLinef ("Received invalid chunk %d < firstblock (%d), ignoring ...", chunknum, firstblock);
		return;
	}

	if (chunknum - firstblock >= MAXBLOCKS_1024) {
		if (developer_qw.integer) Con_PrintLinef ("Received invalid chunk %d .. chunknum - firstblock >= MAXBLOCKS_1024, ignoring ...", chunknum);
		return;
	}

	if (recievedblock[chunknum&(MAXBLOCKS_1024-1)]) {
		if (developer_qw.integer) Con_PrintLinef ("Received duplicate chunk %d, ignoring ...", chunknum);
		return;
	}

	//KILLME Con_PrintLinef ("DPCHUNKS - CL_DPChunks_CL_ParseChunkedDownload received chunk %d", chunknum);

	chunked_receivedbytes += DLBLOCKSIZE_1024;
	recievedblock[chunknum&(MAXBLOCKS_1024-1)] = true;

	while (recievedblock[firstblock&(MAXBLOCKS_1024-1)]) {
		recievedblock[firstblock&(MAXBLOCKS_1024-1)] = false;
		firstblock++;
	}

	int chunk_write_start = chunknum * DLBLOCKSIZE_1024;
	int chunk_write_size  = DLBLOCKSIZE_1024;
	
	// final block is actually meant to be smaller than we recieve.
	if (chunked_downloadsize - chunknum * DLBLOCKSIZE_1024 < DLBLOCKSIZE_1024)	
		chunk_write_size  = chunked_downloadsize - chunknum * DLBLOCKSIZE_1024;

	Baker_CL_Download_During_Data_Start_Size_DP_And_Chunks (data, chunk_write_start, chunk_write_size);

	// Baker: Override the percent
	cls.qw_downloadpercent = chunked_receivedbytes/(float)chunked_downloadsize*100;
	cls.qw_downloadspeedcount += DLBLOCKSIZE_1024;

	tm = Sys_DirtyTime() - cls.qw_downloadstarttime; // how long we dl-ing
	cls.qw_downloadspeedrate = (tm ? chunked_receivedbytes / 1024 / tm : 0); // some average dl speed in KB/s

}

WARP_X_ (CL_DownloadBegin_DP_f)
void CL_DPChunks_CL_SendChunkDownloadReq (void)
{	
	

	int num_chunks_to_ask_for = bound (1, cl_chunksperframe.integer, 30);

	for (int j = 0; j < num_chunks_to_ask_for; j++) {
		int chunk_wanted = QW_CL_RequestADownloadChunk();
#if 0 // Hits	
		//KILLME Con_PrintLinef ("DPCHUNKS - CL_DPChunks_CL_SendChunkDownloadReq chunk_wanted %d", chunk_wanted);
#endif
		if (chunk_wanted == not_found_neg1) {
			//if (cls.qw_downloadmethod == DL_DPCHUNKS_3)
			
			cls.qw_downloadmemorycursize = cls.qw_downloadmemorymaxsize;

			// DOWNLOAD COMPLETE - LET SERVER KNOW SEND STOP RELIABLE
			MSG_WriteByte		(&cls.netcon->message, clc_ackdownloaddata);
			MSG_WriteLong		(&cls.netcon->message, cls.qw_downloadmemorymaxsize);
			MSG_WriteShort		(&cls.netcon->message, 0);
			// The server will receive this and send "cl_downloadfinished" to client
			// And that will write the file, etc.
			WARP_X_ (CL_DownloadFinished_DP_f CL_StopDownload_DP_QW_f)
			break;
		}

		if (!DP_CL_SendClientCommandf (q_is_reliable_false, "chunk %d %d", 
			chunk_wanted, chunked_download_number_key))
			break; // Hit limit for msg
	} // for

}
