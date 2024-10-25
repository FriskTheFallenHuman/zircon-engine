/*
Copyright (C) 2020 Ashley Rose Hale (LadyHavoc)
Copyright (C) 2020 David Knapp (Cloudwalk)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* darkplaces.h - Master definitions file for Darkplaces engine */

#ifndef DARKPLACES_H
#define DARKPLACES_H

typedef unsigned char byte;
typedef unsigned int rgba4;
typedef unsigned char rgb3;
typedef unsigned char ubpalette1;
typedef unsigned int bgra4;

typedef const char ccs;

//WARP_X_ (CL_InitCommands infostring)
extern const char *buildstring;
extern const char *buildmajor;
extern const char *buildstringshort; // Baker r8002: Zircon console name
extern char engineversion[128];
extern char engineversionshort[128]; // Baker r8002: Zircon console name

extern int globs;

#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "sys.h"
#include "qtypes.h"
#include "qdefs.h"
#include "zone.h"
#include "thread.h"
#include "com_game.h"
#include "com_infostring.h"
#include "baker.h"



// Baker: Function call clarity ...

#define ASSIGN(x)						(x)		// Clarifying assignment in parens
#define ALL_FLAGS_ANTIZERO				(~0)

#define fs_caseless_true				true
#define fs_caseless_false				false
#define fs_quiet_true					true
#define fs_quiet_FALSE					false

#define fs_size_ptr_null				NULL
#define fs_pakfile_null					NULL
#define fs_package_index_reply_null		NULL

#define fs_nonblocking_false			false	// Baker: I haven't seen a blocking one yet ...

#define fs_all_files_empty_string		""
#define fs_make_unique_true				true
#define fs_make_unique_false			false

#define fs_quoted_true					true
#define fs_quoted_false					false
#define fs_quoted_auto_2				2
#define fs_quote_remove_true			true
#define fs_quote_remove_false			false

#define fs_one_per_line_true			true
#define fs_one_per_line_false			false
#define fs_reply_already_loaded_null	NULL
#define fs_keep_plain_dirs_false		false
#define fs_is_dlcache_false				false

#define fs_gamedironly_false			false
#define fs_gamedironly_true				true

#define	q_is_dirty_true					true
#define	q_is_dirty_false				false

//#define fs_loadinfo_in_null			NULL
//#define fs_loadinfo_out_null			NULL
#define q_is_large_modelindex_true		true
#define q_is_large_modelindex_false		false
#define q_is_large_soundindex_false		false
#define q_is_large_soundindex_true		true
#define q_unghosted_true				true
#define q_undo_action_normal_0			0
#define q_undo_action_add_1				1
#define q_undo_action_delete_neg_1		-1
#define q_netchange_zero				0

#define q_savefile_NULL					NULL
#define q_savestring_NULL				NULL

#define q_is_siv_write_true				true
#define q_is_siv_write_false			false

#define q_is_forceloop_true				true	// Baker: force sound looping.
#define q_is_forceloop_false			false
#define q_is_aborted_download_false		false
#define q_is_aborted_download_true		true

#define q_is_saveload_false				false
#define q_is_saveload_true				true

#define q_rgb3_white					1.0, 1.0, 1.0
#define q_rgba_solid_white_4_parms		1.0, 1.0, 1.0, 1.0
#define q_rgba_solid_gray50_4_parms		0.5, 0.5, 0.5, 1.0
#define q_rgba_solid_black_4_parms		0.0, 0.0, 0.0, 1.0
#define q_rgba_alpha50_black_4_parms	0.0, 0.0, 0.0, 0.5
#define q_rgba_alpha75_black_4_parms	0.0, 0.0, 0.0, 0.75
#define q_rgba_alpha25_black_4_parms	0.0, 0.0, 0.0, 0.25
#define q_rgba_solid_gray25_4_parms		0.25, 0.25, 0.25, 1.0

#define q_is_from_nothing_false			false
#define q_is_from_nothing_true			true
#define q_s_loadgame_NULL				NULL
#define q_s_startspot_EmptyString		""

#define q_folder_NULL					NULL

#define q_vm_wildcard_NULL				NULL
#define q_vm_classname_NULL				NULL
#define q_vm_targetname_NULL			NULL
#define q_stringlist_NULL				NULL
#define q_vm_printfree_true				true
#define q_vm_printfree_false			false
#define q_is_console_true				true



#define q_is_quakeworld_true			true
#define q_is_quakeworld_false			false

#define q_is_doublewidth_true			true
#define q_is_doublewidth_false			false

#define q_was_a_space_false				false
#define q_was_a_space_true				true

#define count_1							1

#define q_is_menu_fill_false			false
#define q_is_menu_fill_true				true
#define q_is_zautocomplete_true			true
#define q_is_zautocomplete_false		false
#define q_is_suppress_print_true		true
#define q_is_suppress_print_false		false
#define q_darken_true					true
#define q_darken_false					false
#define q_strip_exten_true				true
#define q_strip_exten_false				false
#define q_reply_buf_NULL				NULL
#define q_reply_size_0					0
#define q_is_fence_model_false			false // Quake .mdl with MF_FENCE 16384 set means color 255 is transparent
#define q_is_sky_load_false				false


#define q_mouse_relative_false			false
#define q_mouse_hidecursor_false		false

#define q_is_kicked_true				true	// host.hook.Disconnect, CL_DisconnectEx
#define q_is_kicked_false				false
#define q_disconnect_message_NULL		NULL
#define q_is_leaving_false				false	// SV_DropClient
#define q_is_leaving_true				true

#define scale_1_0						1.0
#define alpha_1_0						1.0
#define alpha_0_75						0.75
#define alpha_0_5						0.5
#define alpha_0_25						0.25

#define	maxlen_0						0

#define	ignorecolorcodes_true			true
#define	ignorecolorcodes_false			false
#define	OUTCOLOR_NULL					NULL
#define	DATA_NULL						NULL
#define	DATASIZE_0						0

#define q_tx_complain_false				false
#define q_tx_complain_true				true
#define q_tx_fallback_notexture_false	false
#define q_tx_fallback_notexture_true	true
#define q_tx_do_external_true			true
#define q_tx_do_external_false			false
#define q_tx_is_sRGB_false				false
#define q_tx_allowfixtrans_false		false
#define q_tx_allowfixtrans_true			true
#define q_tx_convertsrgb_false			false
#define q_tx_miplevel_null				NULL
#define q_tx_miplevel_neg1				-1
#define q_tx_palette_NULL				NULL
#define q_tx_warn_missing_true			true
#define q_tx_warn_missing_false			false

#define q_snd_entnum_neg1				-1
#define q_snd_channel_0					0
#define q_snd_attenuation_0				0		// No sound falloff, full loud everywhere
#define q_snd_volume_read_arg_neg1		-1.0
#define q_snd_forceloop_false			false
#define q_snd_forceloop_true			true
#define q_snd_startposition_0			0


#define q_levelsound_true				true
#define q_levelsound_false				false

#define q_is_static_true				true
#define q_is_static_false				false

#define q_fail_on_missing_false			false
#define q_fail_on_missing_true			true

#define q_reply_len_NULL				NULL

#define q_skysurfaces_false				false		// GL_DepthMask
#define q_skysurfaces_true				true		// GL_DepthMask
#define q_debug_false					false		// GL_DepthMask
#define q_debug_true					true		// GL_DepthMask
#define q_depthonly_false				false		// GL_DepthMask
#define q_depthonly_true				true		// GL_DepthMask
#define q_write_depth_false				false		// GL_DepthMask
#define q_write_depth_true				true		// GL_DepthMask
#define q_prepass_false					false
#define q_prepass_true					true
#define q_is_ui_fog_ignore_false		false		// fog related, presumably to exclude fog from 
													//   affecting certain elements

#define q_is_ui_false					false
#define q_is_ui_true					true

#define q_wants_normals_true			true
#define q_wants_normals_false			false
#define q_wants_tangents_true			true
#define q_wants_tangents_false			false

#define q_text_maxlen_0					0
#define q_outcolor_null					NULL
#define q_ignore_color_codes_true		true
#define q_ignore_color_codes_false		false

#define	q_wildcard_fieldname_null		NULL

#define q_net_suppress_reliables_true	true
#define q_net_suppress_reliables_false	false

#define q_net_burstrate_0				0
#define q_net_rate_10000				10000


#define qnfo_send_true					true
#define qnfo_send_false					false
#define	qnfo_allowstar_true				true
#define	qnfo_allowstar_false			false
#define qnfo_allowmodel_true			true
#define qnfo_allowmodel_false			false
#define qnfo_quiet_true					true
#define qnfo_quiet_false				false

#define qsv_resetcache_true				true
#define qsv_resetcache_false			false
#define qsv_querydp_true				true
#define qsv_queryqw_true				true
#define qsv_querydp_false				false
#define qsv_queryqw_false				false
#define qsv_consoleoutput_true			true
#define qsv_consoleoutput_false			false

#define q_is_zircon_move_true			true
#define q_is_zircon_move_false			false

#define q_include_port_true				true
#define q_include_port_false			false
#define q_is_oob_true					true
#define q_is_oob_false					false

#define q_is_reliable_true				true
#define q_is_reliable_false				false


#define q_fitz_version_none_0			0
#define q_fitz_version_1				1
#define q_fitz_version_2				2

#define q_is_delta_true					true
#define q_is_delta_false				false

#define q_hitcsqcents_true				true
#define q_hitcsqcents_false				false
#define q_hitnetwork_ent_NULL			NULL
#define q_hitbrush_true					true

#define q_hitsuraces_true				true
#define q_hitsuraces_false				false

#define qp_parse_backslash_true			true
#define qp_parse_backslash_false		false

#define q_is_jpeg_false					false
#define q_is_jpeg_true					true
#define q_is_png_false					false
#define q_is_png_true					true

#define q_enabled_true					true	// "unghosted" M_ItemPrint
#define q_enabled_false					false	// "unghosted" M_ItemPrint

#define q_tryreal_true					true	// Real what?
#define q_tryreal_false					false	// "unghosted" M_ItemPrint

#define q_levelsound_true				true	// Significance of this is what? SFXFLAG_LEVELSOUND is added to sfx
#define q_levelsound_false				false

#define STARTPOS_0						0
#define ATTENUATION_0					0
#define ENT_NONE_NEG1					-1
#define ENT_CHANNEL_0					0
#define SND_SPEED_NORMAL_1_0			1.0

#define q_looping_true					true	// 
#define q_looping_false					false	// 

#include "common.h"
#include "filematch.h"
#include "fs.h"

#include "host.h"
#include "cvar.h"
#include "cmd.h"
#include "console.h"
#include "lhnet.h"
#include "mathlib.h"
#include "matrixlib.h"

extern cvar_t developer;
extern cvar_t developer_entityparsing;
extern cvar_t developer_extra;
extern cvar_t developer_insane;
extern cvar_t developer_loadingfile_fs;
extern cvar_t developer_loading;
extern cvar_t developer_spawnfunction_warnings;
extern cvar_t developer_execstring;
extern cvar_t developer_stuffcmd;
extern cvar_t developer_keycode;
extern cvar_t developer_zext;
extern cvar_t developer_movement;
extern cvar_t developer_svc;
extern cvar_t host_isclient;
extern cvar_t sessionid;

void LOC_LoadFile (void); // AURA 8.0
const char *LOC_GetString (const char *s_dollar_key); // AURA 1.2

#ifdef _WIN32
	// Baker: For us let's just, cast size_t and ssize_t to int64_t

	// As I understand it these are actually Windows C run-time specific and GCC uses?
	// https://stackoverflow.com/questions/9225567/how-to-print-a-int64-t-type-in-c
	// https://code.google.com/archive/p/msinttypes/downloads Visual Studio
	#define PRINTF_INT64			"%I64d" // This is stupid int64_t print .. how to do on non-Microsoft?
	#define PRINTF_UINT64			"%I64u" // This is stupid int64_t print .. how to do on non-Microsoft?
	#define PRINTF_INT64HEX			"%I64X" // Perhaps
	#define PRINTF_UINT64			"%I64u" // This is stupid int64_t print .. how to do on non-Microsoft?



	//#define PRIx64	"llX"		// lower case L	Baker: int64_t but as hex
	//#define PRIx32	 "lX"
#else // Non-Windows
	#define PRINTF_INT64			"%PRId64" // This is stupid int64_t print .. how to do on non-Microsoft?
	#define PRINTF_UINT64			"%PRIu64" // Perhaps
	#define PRINTF_INT64HEX			"%PRIx64" // Perhaps
#endif	

void stringlistappendfssearch (stringlist_t *plist, fssearch_t *t); // t == NULL is ok and handled
void stringlistappend_search_pattern (stringlist_t *plist, const char *s_pattern);

// Baker: Some compilers don't like variable declaration after a label or other edge cases

#define NULLSTATEMENT() 

#define SAVEGAME_PIC_NAME		"savegamepic"
#define	SAVEGAME_PIC_WIDTH_512	512
#define	SAVEGAME_PIC_HEIGHT_320	320
#define SAVEGAME_JPEG_MAXSIZE_STRING_SIZE (SAVEGAME_PIC_WIDTH_512 * SAVEGAME_PIC_HEIGHT_320 * RGBA_4)


#endif // ! 
