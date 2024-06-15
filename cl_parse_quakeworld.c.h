// cl_parse_quakeworld.c.h

WARP_X_ (CL_ParseServerMessage)

		CL_NetworkTimeReceived(host.realtime); // qw has no clock

		// kill all qw nails
		cl.qw_num_nails = 0;

		// fade weapon view kick
		cl.qw_weaponkick = min(cl.qw_weaponkick + 10 * bound(0, cl.time - cl.oldtime, 0.1), 0);

		cls.servermovesequence = cls.netcon->qw.incoming_sequence;

		qwplayerupdatereceived = false;

		while (1) {
			if (cl_message.badread)
				Host_Error_Line ("CL_ParseServerMessage: Bad QW server message");

			cmd = MSG_ReadByte(&cl_message);

			if (cmd == -1)
			{
				SHOWNET("END OF MESSAGE");
				break;		// end of message
			}

			cmdindex = cmdcount & 31;
			cmdcount++;
			cmdlog[cmdindex] = cmd;

			SHOWNET(qw_svc_strings[cmd]);
			cmdlogname[cmdindex] = qw_svc_strings[cmd];
			if (!cmdlogname[cmdindex])
			{
				// LadyHavoc: fix for bizarre problem in MSVC that I do not understand (if I assign the string pointer directly it ends up storing a NULL pointer)
				const char *d = "<unknown>";
				cmdlogname[cmdindex] = d;
			}

			// other commands
			switch (cmd)
			{
			default:
				{
					char description[32*64], logtemp[64];
					int count;
					strlcpy(description, "packet dump: ", sizeof(description));
					j = cmdcount - 32;
					if (j < 0)
						j = 0;
					count = cmdcount - j;
					j &= 31;
					while(count > 0) {
						dpsnprintf(logtemp, sizeof(logtemp), "%3d:%s ", cmdlog[j], cmdlogname[j]);
						strlcat(description, logtemp, sizeof(description));
						count--;
						j++;
						j &= 31;
					}
					description[strlen(description)-1] = '\n'; // replace the last space with a newline
					Con_Print(description);
					Host_Error_Line ("CL_ParseServerMessage: Illegible server message");
				}
				break;

			case qw_svc_nop:
				//Con_Printf ("qw_svc_nop\n");
				break;

			case qw_svc_disconnect:
				if (cls.demonum != -1)
					CL_NextDemo();
				else
					CL_DisconnectEx(q_is_kicked_true, "Server disconnected");
				break;

			case qw_svc_print:
				// CTEXT
				j = MSG_ReadByte(&cl_message);
				str = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
				if (CL_ExaminePrintString(str)) { // look for anything interesting like player IP addresses or ping reports
					if (j == 3) {
						// chat
						va(vabuf, sizeof(vabuf), "\1%s", str);
						if (1 /*cl_pext_qw_coloredtext.value*/ && String_Contains (vabuf, "&"))
							CSQC_AddPrintTextQWColor (vabuf);
						else
							CSQC_AddPrintText(vabuf);	//[515]: csqc
					}
					else {
						if (1 /*cl_pext_qw_coloredtext.value*/ && String_Contains (str, "&"))
							CSQC_AddPrintTextQWColor (str);
						else
							CSQC_AddPrintText(str);
					}
				}
				break;

			case qw_svc_centerprint:
				// CTEXT
				CL_VM_Parse_CenterPrint(MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));	//[515]: csqc
				break;

			case qw_svc_stufftext:
				// CTEXT
				CL_VM_Parse_StuffCmd(MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), q_is_quakeworld_true);	//[515]: csqc
				break;

			case qw_svc_damage:
				// svc_damage protocol is identical to nq
				V_ParseDamage ();
				break;

			// Baker: FS_ChangeGameDirs occurs here.
			case qw_svc_serverdata:
				//Cbuf_Execute(); // make sure any stuffed commands are done
				CL_ParseServerInfo(q_is_quakeworld_true);
gamedir_change:
				break;

			case qw_svc_setangle:
				for (j = 0 ; j < 3 ; j ++)
					cl.viewangles[j] = MSG_ReadAngle(&cl_message, cls.protocol);
				if (!cls.demoplayback)
				{
					cl.fixangle[0] = true;
					VectorCopy(cl.viewangles, cl.mviewangles[0]);
					// disable interpolation if this is new
					if (!cl.fixangle[1])
						VectorCopy(cl.viewangles, cl.mviewangles[1]);
				}
				break;

			case qw_svc_lightstyle:
				j = MSG_ReadByte(&cl_message);
				if (j >= cl.max_lightstyle)
				{
					Con_PrintLinef ("svc_lightstyle >= MAX_LIGHTSTYLES_256");
					break;
				}
				strlcpy (cl.lightstyle[j].map,  MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof (cl.lightstyle[j].map));
				cl.lightstyle[j].map[MAX_STYLESTRING_64 - 1] = 0;
				cl.lightstyle[j].length = (int)strlen(cl.lightstyle[j].map);
				break;

			case qw_svc_sound:
				CL_ParseStartSoundPacket (q_is_large_soundindex_false);
				break;

			case qw_svc_stopsound:
				j = (unsigned short) MSG_ReadShort(&cl_message);
				S_StopSound(j>>3, j&7);
				break;

			case qw_svc_updatefrags:
				j = MSG_ReadByte(&cl_message);
				if (j >= cl.maxclients)
					Host_Error_Line ("CL_ParseServerMessage: svc_updatefrags >= cl.maxclients");
				cl.scores[j].frags = (signed short) MSG_ReadShort(&cl_message);
				break;

			case qw_svc_updateping:
				j = MSG_ReadByte(&cl_message);
				if (j >= cl.maxclients)
					Host_Error_Line ("CL_ParseServerMessage: svc_updateping >= cl.maxclients");
				cl.scores[j].qw_ping = MSG_ReadShort(&cl_message);
				break;

			case qw_svc_updatepl:
				j = MSG_ReadByte(&cl_message);
				if (j >= cl.maxclients)
					Host_Error_Line ("CL_ParseServerMessage: svc_updatepl >= cl.maxclients");
				cl.scores[j].qw_packetloss = MSG_ReadByte(&cl_message);
				break;

			case qw_svc_updateentertime:
				j = MSG_ReadByte(&cl_message);
				if (j >= cl.maxclients)
					Host_Error_Line ("CL_ParseServerMessage: svc_updateentertime >= cl.maxclients");
				// seconds ago
				cl.scores[j].qw_entertime = cl.time - MSG_ReadFloat(&cl_message);
				break;

			case qw_svc_spawnbaseline:
				j = (unsigned short) MSG_ReadShort(&cl_message);
				if (j < 0 || j >= MAX_EDICTS_32768)
					Host_Error_Line ("CL_ParseServerMessage: svc_spawnbaseline: invalid entity number %d", j);
				if (j >= cl.max_entities)
					CL_ExpandEntities(j);
				CL_ParseBaseline(cl.entities + j, q_is_large_modelindex_false, q_fitz_version_none_0, q_is_static_false);
				break;

			case qw_svc_spawnstatic:
				CL_ParseStatic(q_is_large_modelindex_false, q_fitz_version_none_0);
				break;
			case qw_svc_temp_entity:
				if (!CL_VM_Parse_TempEntity())
					CL_ParseTempEntity ();
				break;

			case qw_svc_killedmonster:
				cl.stats[STAT_MONSTERS]++;
				break;

			case qw_svc_foundsecret:
				cl.stats[STAT_SECRETS]++;
				break;

			case qw_svc_updatestat:
				j = MSG_ReadByte(&cl_message);
				if (j < 0 || j >= MAX_CL_STATS_256)
					Host_Error_Line ("svc_updatestat: %d is invalid", j);
				cl.stats[j] = MSG_ReadByte(&cl_message);
				break;

			case qw_svc_updatestatlong:
				j = MSG_ReadByte(&cl_message);
				if (j < 0 || j >= MAX_CL_STATS_256)
					Host_Error_Line ("svc_updatestatlong: %d is invalid", j);
				cl.stats[j] = MSG_ReadLong(&cl_message);
				break;

			case qw_svc_spawnstaticsound:
				CL_ParseStaticSound (q_is_large_soundindex_false, q_fitz_version_none_0);
				break;

			case qw_svc_cdtrack:
				cl.cdtrack = cl.looptrack = MSG_ReadByte(&cl_message);

				if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
					CDAudio_Play ((unsigned char)cls.forcetrack, q_looping_true);
				else
					CDAudio_Play ((unsigned char)cl.cdtrack, q_looping_true);

				break;

			case qw_svc_intermission:
				if (!cl.intermission)
					cl.completed_time = cl.time;
				cl.intermission = 1;
				MSG_ReadVector(&cl_message, cl.qw_intermission_origin, cls.protocol);
				for (j = 0; j < 3; j++)
					cl.qw_intermission_angles[j] = MSG_ReadAngle(&cl_message, cls.protocol);
				break;

			case qw_svc_finale:
				if (!cl.intermission)
					cl.completed_time = cl.time;
				cl.intermission = 2;
				SCR_CenterPrint(MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
				break;

			case qw_svc_sellscreen:
				Cmd_ExecuteString(cmd_local, "help", src_local, true);
				break;

			case qw_svc_smallkick:
				cl.qw_weaponkick = -2;
				break;
			case qw_svc_bigkick:
				cl.qw_weaponkick = -4;
				break;

			case qw_svc_muzzleflash:
				j = (unsigned short) MSG_ReadShort(&cl_message);
				// NOTE: in QW this only worked on clients
				if (j < 0 || j >= MAX_EDICTS_32768)
					Host_Error_Line ("CL_ParseServerMessage: svc_spawnbaseline: invalid entity number %d", j);
				if (j >= cl.max_entities)
					CL_ExpandEntities(j);
				cl.entities[j].persistent.muzzleflash = 1.0f;
				break;

			case qw_svc_updateuserinfo:
				QW_CL_UpdateUserInfo();
				break;

			case qw_svc_setinfo:
				QW_CL_SetInfo();
				break;

			case qw_svc_serverinfo:
				QW_CL_ServerInfo(); // HEREON
				break;

			case qw_svc_download:
				QW_CL_ParseDownload(q_is_oob_false);
				break;

			case qw_svc_playerinfo:
				// slightly kill qw player entities now that we know there is
				// an update of player entities this frame...
				if (!qwplayerupdatereceived) {
					qwplayerupdatereceived = true;
					for (j = 1; j < cl.maxclients; j++)
						cl.entities_active[j] = false;
				}
				EntityStateQW_ReadPlayerUpdate();
				break;

			case qw_svc_nails:
				QW_CL_ParseNails();
				break;

			case qw_svc_chokecount:
				(void) MSG_ReadByte(&cl_message);
				// FIXME: apply to netgraph
				//for (j = 0;j < j;j++)
				//	cl.frames[(cls.netcon->qw.incoming_acknowledged-1-j)&QW_UPDATE_MASK_63].receivedtime = -2;
				break;

			case qw_svc_modellist:
				QW_CL_ParseModelList (q_is_doublewidth_false);
				break;

			case qw_svc_fte_modellistshort:
				QW_CL_ParseModelList (q_is_doublewidth_true);
				break;

			case qw_svc_soundlist:
				QW_CL_ParseSoundList();
				break;

			case qw_svc_packetentities:
				EntityFrameQW_CL_ReadFrame (q_is_delta_false);
				// first update is the final signon stage
				if (cls.signon == SIGNONS_4 - 1) {
					cls.signon = SIGNONS_4; // QUAKEWORLD
					CL_SignonReply ();
					cls.world_frames = 0; cls.world_start_realtime = 0; 
				}
				break;

			case qw_svc_deltapacketentities:
				EntityFrameQW_CL_ReadFrame (q_is_delta_true);
				// first update is the final signon stage
				if (cls.signon == SIGNONS_4 - 1) {
					cls.signon = SIGNONS_4; // QUAKEWORLD
					CL_SignonReply ();
					cls.world_frames = 0; cls.world_start_realtime = 0; 
				}
				break;

			case qw_svc_maxspeed:
				cl.movevars_maxspeed = MSG_ReadFloat(&cl_message);
				break;

			case qw_svc_entgravity:
				cl.movevars_entgravity = MSG_ReadFloat(&cl_message);
				if (!cl.movevars_entgravity)
					cl.movevars_entgravity = 1.0f;
				break;

			case qw_svc_setpause:
				cl.paused = MSG_ReadByte(&cl_message) != 0;
				if (cl.paused && snd_cdautopause.integer)
					CDAudio_Pause ();
				else if (bgmvolume.value > 0.0f)
					CDAudio_Resume ();

				S_PauseGameSounds (cl.paused);
				break;
			}
		}

		if (qwplayerupdatereceived)
		{
			// fully kill any player entities that were not updated this frame
			for (j = 1; j <= cl.maxclients; j++)
				if (!cl.entities_active[j])
					cl.entities[j].state_current.active = false;
		}
