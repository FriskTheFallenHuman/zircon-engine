// ___warps.c.h -- current business -- and business is good!

// We want to do a struct array.  What is best way to do this?
// Problem: ?  Pointers should be fine.
WARP_X_ (stringlist_t)

// WAVE 1 - Edicts
// WAVE 2 - Edicts with tree
//typedef struct listview_item_s {
//	int	maxsize;
//	/// maxstrings changes as needed, causing reallocation of strings[] array
//	int maxstrings;
//	int numstrings;
//	char **strings;
//} list_item_t;
//
//typedef struct listview_item_s {
//	int	maxsize;
//	/// maxstrings changes as needed, causing reallocation of strings[] array
//	int maxstrings;
//	int numstrings;
//	char **strings;
//} list_column_t;
//
//typedef struct listview_s {
//	int	maxsize;
//	/// maxstrings changes as needed, causing reallocation of strings[] array
//	int maxstrings;
//	int numstrings;
//	char **strings;
//
//	struct {
//		int		first_visible_row;
//		int		num_visible_rows;
//	} viso;
//} list_view_t;
//
//		particle_t *oldparticles = cl.particles;
//		cl.max_particles = min(cl.max_particles * 2, MAX_PARTICLES);
//		cl.particles = (particle_t *) Mem_Alloc(cls.levelmempool, cl.max_particles * sizeof(particle_t));
//		memcpy(cl.particles, oldparticles, cl.num_particles * sizeof(particle_t));
//		Mem_Free(oldparticles);
//


// If we were to sort these, we need to sort the indexes.
// By a proxy.


///


SIV checklist:

sv_intermap_siv_list --
// 1. Allocated when?					When we write a .siv

// map is always a new game, changelevel is always a continued game.

// 2. Cleared when?						// "map" because that is a new game.
										// "loadgame" can stomp it.
										// "restart" should reload it, although ...
										// "changelevel" updates it


What is the reply to "prespawn"


WARP_X_ (quemove)

sequence is what?  cls.servermovesequence
where is sequence updated?  svc_entities every time
how?



	int movement_predicted; // PRED ZIRCON MOVE_2
	int zircon_last_seq;
	vec3_t zircon_origin_for_seq;


rgbGen

RENDER_STEP

textures/sch_fx/mxl_sklo_off
{
	qer_editorimage textures/sch_fx/sklo2.tga
	surfaceparm trans
	cull none
	
	{
		map textures/sch_fx/tinfx.tga
		blendfunc add
		rgbGen identity
		tcGen environment 
	}
	{
		map textures/sch_fx/sklo2.tga
		blendfunc blend
		rgbGen identity
	}
        {
		map $lightmap
		rgbGen identity
		blendFunc filter
	}
}


Baker: old FTE via archive.org https://web.archive.org/web/20191016210645if_/http://fte.triptohell.info/moodles/win32/fteqw.exe

wad.gfx_base

cl.fix_angle_count;


cl.movement_origin


ZIRCON_PEXT ZIRCON_EXT_CHUNKED_2 ZIRCON_PEXT cls.zirconprotocolextensions
Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE

NETWORK:
WARP_X_ (CL_ParseServerMessage EntityFrame5_CL_ReadFrame EntityFrameQW_CL_ReadFrame)
// Baker: Where does the origin come in?
HITT_PLAYERS_1
static void EntityFrame5_CL_ReadFrame_ReadUpdate (entity_state_t *s, int number)
// EntityStateQW_CL_ReadFrame_ReadEntityUpdate

teleport_ack_frame

// Baker:
// Zircon move
// Zircon move
// SV: Discovers teleport
// Zircon move
// Zircon move
// CL received teleport .. sv_inteleport?  Treats zircon moves as dp moves until gets clc_move?
// DarkPlaces move
// DarkPlaces move


Baker:
hitnetworkplayers 2

SV_Physics_ClientMove


For prediction to work, we must be ignoring server position?

Does it occur without jumping?  Yes straight walk no looking.
Looking up whole way?  Yes.

Write out stuff.

Where is read?

Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE

CL_TraceBox

CL_Frame ->

	cl.oldtime = cl.time;
	cl.time += clframetime;

1.	CL_Input();

2.	NetConn_ClientFrame();
		NetConn_ClientParsePacket
		NetConn_QueryQueueFrame // Servers

		// Leave these alone.  I am worried that network monitoring might throttle or something.
		net_slist_queriesperframe 500; net_slist_queriespersecond 200
			net_slist_queriespersecond	20
			net_slist_queriesperframe	4

3.	CL_SendMove();
		cl.mcmd.clx_frametime = bound(0.0, cl.mcmd.clx_time - cl.movecmd[1].clx_time, 0.255);
		// ridiculous value rejection (matches qw)
		if (cl.mcmd.clx_frametime > 0.25)
			cl.mcmd.clx_frametime = 0.1;
		cl.mcmd.clx_msec = (unsigned char)floor(cl.mcmd.clx_frametime * 1000);

	CL_UpdateWorld();
	CL_ClientMovement_Replay ();
		CL_ClientMovement_PlayerMove_Frame ();
			CL_ClientMovement_PlayerMove ();  s->cmd.clx_frametime
				CL_ClientMovement_Physics_Walk ();
					CL_ClientMovement_Move (s);

// We are at X with sequence, origin, angles, cl.clx_time velocity --> end move?
					// These are finals?
					s->velocity
					s->origin


	if (developer_texturelogging.integer)
		Log_Printf("textures.log", "%s\n", filename);



	EntityFrame5_CL_ReadFrame // We can detect a warp (oldseq is new sequence)
	// How detect Quakeworld warp?