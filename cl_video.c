
#include "quakedef.h"
#include "cl_video.h"

WARP_X_ (DrawQ_SuperPic_Video)

// cvars
cvar_t cl_video_subtitles = {CF_CLIENT | CF_ARCHIVE, "cl_video_subtitles", "0", "show subtitles for videos, uses notify font (if they are present)"};
cvar_t cl_video_subtitles_lines = {CF_CLIENT | CF_ARCHIVE, "cl_video_subtitles_lines", "4", "how many lines to occupy for subtitles"};
cvar_t cl_video_subtitles_textsize = {CF_CLIENT | CF_ARCHIVE, "cl_video_subtitles_textsize", "16", "textsize for subtitles"};
cvar_t cl_video_scale = {CF_CLIENT | CF_ARCHIVE, "cl_video_scale", "1", "scale of video, 1 = fullscreen, 0.75 - 3/4 of screen etc."};
cvar_t cl_video_scale_vpos = {CF_CLIENT | CF_ARCHIVE, "cl_video_scale_vpos", "0", "vertical align of scaled video, -1 is top, 1 is bottom"};
cvar_t cl_video_stipple = {CF_CLIENT | CF_ARCHIVE, "cl_video_stipple", "0", "draw interlacing-like effect on videos, similar to scr_stipple but static and used only with video playing."};
cvar_t cl_video_brightness = {CF_CLIENT | CF_ARCHIVE, "cl_video_brightness", "1", "brightness of video, 1 = fullbright, 0.75 - 3/4 etc."};
cvar_t cl_video_keepaspectratio = {CF_CLIENT | CF_ARCHIVE, "cl_video_keepaspectratio", "0", "keeps aspect ratio of fullscreen videos, leaving black color on unfilled areas, a value of 2 let video to be stretched horizontally with top & bottom being sliced out"};
cvar_t cl_video_fadein = {CF_CLIENT | CF_ARCHIVE, "cl_video_fadein", "0", "fading-from-black effect once video is started, in seconds"};
cvar_t cl_video_fadeout = {CF_CLIENT | CF_ARCHIVE, "cl_video_fadeout", "0", "fading-to-black effect once video is ended, in seconds"};

cvar_t v_glslgamma_video = {CF_CLIENT | CF_ARCHIVE, "v_glslgamma_video", "1", "applies GLSL gamma to played video, could be a fraction, requires r_glslgamma_2d 1."};

// DPV stream decoder
#include "dpvsimpledecode.h"

// VorteX: libavcodec implementation
#include "cl_video_libavw.c.h"

// JAM video decoder used by Blood Omnicide
#ifdef JAMVIDEO
#include "cl_video_jamdecode.c.h"
#endif

// constants (and semi-constants)
static int  cl_videormask;
static int  cl_videobmask;
static int  cl_videogmask;

// Baker: Let's keep it real, this is set to 4 so make const
#define cl_videobytesperpixel_4 RGBA_4 // Probably BGRA_4 but whatever ...
//static int	cl_videobytesperpixel_4; // CL_Video_Init sets this to 4


static int cl_num_videos;
static clvideo_t cl_videos[ MAXCLVIDEOS_65 ];
static rtexturepool_t *cl_videotexturepool;

static clvideo_t *FindUnusedVid( void )
{
	int i;
	for( i = 1 ; i < MAXCLVIDEOS_65 ; i++ )
		if ( cl_videos[ i ].state == CLVIDEO_UNUSED_0 )
			return &cl_videos[ i ];
	return NULL;
}


#include "baker_jpeg_decode.c.h"
#include "baker_gif_decode.c.h"

WARP_X_ (bakergifdecode_open dpvsimpledecode_open)
static qbool OpenStream( clvideo_t * video )
{
	const char *errorstring;

	video->stream = bakerjpegdecode_open( video, video->filename, &errorstring);
	if (video->stream)
		return true;

	video->stream = bakergifdecode_open( video, video->filename, &errorstring);
	if (video->stream)
		return true;
	
	video->stream = dpvsimpledecode_open (video, video->filename, &errorstring);
	if (video->stream)
		return true;

#ifdef JAMVIDEO
	video->stream = jam_open( video, video->filename, &errorstring);
	if (video->stream)
		return true;
#endif

	video->stream = LibAvW_OpenVideo (video, video->filename, &errorstring);
	if (video->stream)
		return true;

	Con_PrintLinef (CON_ERROR "unable to open video file " QUOTED_S ", error: %s", video->filename, errorstring);
	return false;
}

WARP_X_CALLERS_ (R_RealGetTexture)
static void VideoUpdateCallback(rtexture_t *rt, void *data)
{
	clvideo_t *video = (clvideo_t *) data; // CPIFX: VideoUpdateCallback -- Baker I don't think this ever gets called by DarkPlaces Beta
	Draw_NewPic(video->name, video->width, video->height, (unsigned char *)video->vimagedata, TEXTYPE_BGRA, TEXF_CLAMP);
}

WARP_X_CALLERS_ (OpenVideo DrawQ_SuperPic_Video)
static void LinkVideoTexture (clvideo_t *video)
{
	// CPIFX: New ... it is an assignment WakeVideo and OpenVideo
	//video->cpif = Draw_CachePic_Flags(name, CACHEPICFLAG_NOTPERSISTENT | CACHEPICFLAG_QUIET);

	WARP_X_ (OpenVideo)

	// Baker: OpenVideo calls us

	// RESTART CRASH HERE

	rtexture_t *rt = Draw_GetPicTexture(video->cpif); // video->cpif; 

	if (rt) {
		// int j = 5; // Weird!  I hope this never happens
	}

	R_MakeTextureDynamic(rt /*Draw_GetPicTexture(video->cpif)*/, VideoUpdateCallback, video);
}

static void UnlinkVideoTexture (clvideo_t *video)
{
	// free the texture (this does not destroy the cachepic_t, which is eternal)
	// CPIFX: Baker: I think this is called to clear the buff free old pic

	rtexture_t *rt = Draw_GetPicTexture(video->cpif); //video->cpif;

	if (rt) {
		if (video->cpif->skinframe) {
			// R_PurgeTexture
			R_SkinFrame_PurgeSkinFrame(video->cpif->skinframe);
			video->cpif->skinframe = NULL; // ? Q1SKY
		}
	}

	R_UnMakeTextureDynamic (rt); // Accepts NULL
	Draw_FreePic	(video->name);
	Mem_Free		(video->vimagedata); // free the image data
}

static void SuspendVideo( clvideo_t * video )
{
	if (video->suspended)
		return;
	video->suspended = true;
	UnlinkVideoTexture(video);
	// if we are in firstframe mode, also close the stream
	if (video->state == CLVIDEO_FIRSTFRAME_4) {
		if (video->stream)
			video->close(video->stream);
		video->stream = NULL;
	}
}

static qbool WakeVideo( clvideo_t * video )
{
	if ( !video->suspended )
		return true;
	video->suspended = false;

	if ( video->state == CLVIDEO_FIRSTFRAME_4)
		if ( !OpenStream( video ) ) {
			video->state = CLVIDEO_UNUSED_0;
			return false;
		}

	video->vimagedata = Mem_Alloc( cls.permanentmempool, video->width * video->height * cl_videobytesperpixel_4 );
	LinkVideoTexture( video );

	// update starttime
	video->starttime += host.realtime - video->lasttime;

	return true;
}

static void LoadSubtitles( clvideo_t *video, const char *subtitlesfile )
{
	char *subtitle_text;
	const char *data;
	float subtime, sublen;
	int numsubs = 0;

	if (gamemode == GAME_BLOODOMNICIDE) {
		cvar_t *langcvar;

		langcvar = Cvar_FindVar(&cvars_all, "language", CF_CLIENT | CF_SERVER);
		subtitle_text = NULL;
		if (langcvar) {
			char overridename[MAX_QPATH_128];
			c_dpsnprintf2 (overridename, "locale/%s/%s", langcvar->string, subtitlesfile);
			subtitle_text = (char *)FS_LoadFile(overridename, cls.permanentmempool, fs_quiet_FALSE, fs_size_ptr_null);
		}
		if (!subtitle_text)
			subtitle_text = (char *)FS_LoadFile(subtitlesfile, cls.permanentmempool, fs_quiet_FALSE, fs_size_ptr_null);
	}
	else
	{
		subtitle_text = (char *)FS_LoadFile(subtitlesfile, cls.permanentmempool, fs_quiet_FALSE, fs_size_ptr_null);
	}

	if (!subtitle_text) {
		Con_DPrintLinef ( "LoadSubtitles: can't open subtitle file " QUOTED_S, subtitlesfile );
		return;
	}

	// parse subtitle_text
	// line is: (START TIME) (DURATION) "text" where
	//    x - start time
	//    y - seconds last (
	//			if 0 - last thru next sub, 
	//			if negative - (last to next sub less this amount of seconds) - gap between

	data = subtitle_text;
	for (;;)
	{
		if (!COM_ParseToken_QuakeC(&data, false))
			break;
		subtime = atof( com_token );
		if (!COM_ParseToken_QuakeC(&data, false))
			break;
		sublen = atof( com_token );
		if (!COM_ParseToken_QuakeC(&data, false))
			break;
		if (!com_token[0])
			continue;
		// check limits
		if (video->subtitles == CLVIDEO_MAX_SUBTITLES_512)
		{
			Con_PrintLinef (CON_WARN "WARNING: CLVIDEO_MAX_SUBTITLES_512 = %d reached when reading subtitles from " QUOTED_S, CLVIDEO_MAX_SUBTITLES_512, subtitlesfile);
			break;	
		}
		// add a sub
		video->subtitle_text[numsubs] = (char *) Mem_Alloc(cls.permanentmempool, strlen(com_token) + 1);
		memcpy (video->subtitle_text[numsubs], com_token, strlen(com_token) + 1);
		video->subtitle_start[numsubs] = subtime;
		video->subtitle_end[numsubs] = sublen;
		if (numsubs > 0) // make true len for prev sub, autofix overlapping subtitles
		{
			if (video->subtitle_end[numsubs-1] <= 0)
				video->subtitle_end[numsubs-1] = max(video->subtitle_start[numsubs-1], video->subtitle_start[numsubs] + video->subtitle_end[numsubs-1]);
			else
				video->subtitle_end[numsubs-1] = min(video->subtitle_start[numsubs-1] + video->subtitle_end[numsubs-1], video->subtitle_start[numsubs]);
		}
		numsubs++;
		// todo: check timing for consistency?
	}
	if (numsubs > 0) // make true len for prev sub, autofix overlapping subtitles
	{
		if (video->subtitle_end[numsubs-1] <= 0)
			video->subtitle_end[numsubs-1] = (float)99999999; // fixme: make it end when video ends?
		else
			video->subtitle_end[numsubs-1] = video->subtitle_start[numsubs-1] + 
												video->subtitle_end[numsubs-1];
	}
	Z_Free( subtitle_text );
	video->subtitles = numsubs;
/*
	Con_Printf ( "video->subtitles: %d\n", video->subtitles );
	for (numsubs = 0; numsubs < video->subtitles; numsubs++)
		Con_Printf ( "  %03.2f %03.2f : %s\n", video->subtitle_start[numsubs], video->subtitle_end[numsubs], video->subtitle_text[numsubs] );
*/
}

WARP_X_ (LinkVideoTexture)
static clvideo_t *OpenVideo( clvideo_t *video, const char *filename, const char *name, 
							int owner, const char *subtitlesfile )
{
	c_strlcpy (video->filename, filename);

	// Baker: I see nothing zeroing the struct, so we do this ...
	video->is_gif_stream = false; // Baker: We need to do this before OpenStream
	video->decodeframegif = NULL;
	
	c_strlcpy (video->name, name);
	video->ownertag = owner;

	// Baker: Require "video" folder.
	if (false == String_Starts_With_PRE(name, CL_VIDEO_SLASH_PREFIX)) 
		return NULL;
	video->cpif = Draw_CachePic_Flags(name, CACHEPICFLAG_NOTPERSISTENT | CACHEPICFLAG_QUIET);

	// Yes we have one.
	// rtexture_t *rt = Draw_GetPicTexture(video->cpif);

	// Baker: CPIF
	if (!OpenStream(video) )
		return NULL;

	video->state		= CLVIDEO_FIRSTFRAME_4;
	video->framenum_vid	= -1;
	video->framegif_endtime = 0;
	
	video->frames_per_second = video->getframerate (video->stream);
	video->lasttime		= host.realtime;
	video->subtitles	= 0;

	video->width		= video->getwidth	(video->stream);
	video->height		= video->getheight	(video->stream);
	video->vimagedata	= Mem_Alloc( cls.permanentmempool, video->width * video->height * cl_videobytesperpixel_4 );
	LinkVideoTexture (video);

	// VorteX: load simple subtitle_text file
	if (subtitlesfile[0])
		LoadSubtitles (video, subtitlesfile);

	return video;
}

clvideo_t *CL_Cin_OpenVideo (const char *filename, const char *name, int owner, const char *subtitlesfile)
{
	clvideo_t *video;
	// sanity check
	if ( !name || !*name || String_NOT_Start_With_PRE(name, CL_VIDEO_SLASH_PREFIX)) {
		Con_DPrintLinef ( "CL_Cin_OpenVideo: Bad video texture name " QUOTED_S, name );
		return NULL;
	}

	video = FindUnusedVid ();
	if (video == NULL) {
		Con_PrintLinef (CON_ERROR "CL_Cin_OpenVideo: unable to open video " QUOTED_S " - video limit reached", filename );
		return NULL;
	}
	video = OpenVideo(video, filename, name, owner, subtitlesfile);
	// expand the active range to include the new entry
	if (video) {
		cl_num_videos = Largest(cl_num_videos, (int)(video - cl_videos) + 1);
	}
	return video;
}

static clvideo_t *CL_GetVideoBySlot(int slot)
{
	clvideo_t *video = &cl_videos[slot];

	if (video->suspended) {
		if (!WakeVideo(video))
			return NULL;
		else if (video->state == CLVIDEO_RESETONWAKEUP_5) {
			video->framenum_vid = -1;
			video->framegif_endtime = 0;
		}
	}

	video->lasttime = host.realtime;

	return video;
}

clvideo_t *CL_GetVideoByName(const char *name)
{
	int i;
	for (i = 0; i < cl_num_videos; i ++ )
		if (cl_videos[ i ].state != CLVIDEO_UNUSED_0 && String_Match (cl_videos[i].name, name) )
			break;

	if (i != cl_num_videos)
		return CL_GetVideoBySlot(i);
	else
		return NULL;
}

void CL_SetVideoState(clvideo_t *video, clvideostate_t state)
{
	if (!video)
		return;

	video->lasttime = host.realtime;
	video->state = state;
	if (state == CLVIDEO_FIRSTFRAME_4)
		CL_RestartVideo(video);
}

void CL_RestartVideo (clvideo_t *video)
{
	if (!video)
		return;

	// reset time
	video->starttime = video->lasttime = host.realtime;
	video->framenum_vid = -1;
	video->framegif_endtime = 0;

	// reopen stream
	WARP_X_ (dpvsimpledecode_close)
	if (video->stream)
		video->close(video->stream);
	video->stream = NULL;
	if (!OpenStream(video))
		video->state = CLVIDEO_UNUSED_0;
}

// close video
void CL_CloseVideo(clvideo_t * video)
{
	int i;

	if (!video || video->state == CLVIDEO_UNUSED_0)
		return;

	// close stream
	if (!video->suspended || video->state != CLVIDEO_FIRSTFRAME_4)
	{
		if (video->stream)
			video->close(video->stream);
		video->stream = NULL;
	}
	// unlink texture
	if (!video->suspended)
		UnlinkVideoTexture(video);
	// purge subtitles
	if (video->subtitles)
	{
		for (i = 0; i < video->subtitles; i++)
			Z_Free( video->subtitle_text[i] );
		video->subtitles = 0;
	}
	video->state = CLVIDEO_UNUSED_0;
}

// update all videos
void CL_Video_Frame (void) 
{
	clvideo_t *video;
	int destframe;
	int i;

	if (!cl_num_videos)
		return;

	#define SECS_INTO_VID_BY_FRAMENUM(VIDEO) (VIDEO->framenum_vid * VIDEO->frames_per_second)
	#define SECS_INTO_VID_BY_STARTTIME(VIDEO) (host.realtime - VIDEO->starttime)

	for (video = cl_videos, i = 0; i < cl_num_videos; video ++, i ++) {
		if (video->state != CLVIDEO_UNUSED_0 && !video->suspended) {
			// Baker: I haven't determined the purpose of this.
			// If the video isn't unused and hasn't fired in 2.0 seconds we suspend it, but why
			// And does this happen?  Does pause video hit here.
			// How do we pause a video
			if (host.realtime - video->lasttime > CL_SUSPEND_THRESHOLD_SECS_2_0) {
				SuspendVideo(video);
				continue;
			}

			if (video->state == CLVIDEO_PAUSE_3) {
				// Baker: If the video is paused
				// We are setting the start time to now
				// minus what I would assume is the progress into the file.

				video->starttime = host.realtime - SECS_INTO_VID_BY_FRAMENUM(video);
									// video->framenum * video->frames_per_second;
				continue;
			}
			// read video frame from stream if time has come
			if (video->state == CLVIDEO_FIRSTFRAME_4)
				destframe = 0;
			else
				destframe = (int)(SECS_INTO_VID_BY_STARTTIME(video) * video->frames_per_second);
			if (destframe < 0)
				destframe = 0;

			if (video->is_gif_stream) {
				double time_into_vid = SECS_INTO_VID_BY_STARTTIME(video); // 5.0
				if (video->framegif_endtime == 0 || time_into_vid >= video->framegif_endtime) { 
					video->framenum_vid ++; // Baker: Make sure this is -1 first time.
					int frame_returned = -1;
					WARP_X_ (bakergifdecode_videoframe)
					if (video->decodeframegif (video, video->stream,  video->vimagedata,  cl_videormask,  cl_videogmask,  cl_videobmask,  cl_videobytesperpixel_4,  cl_videobytesperpixel_4 * video->width, video->framenum_vid, time_into_vid, &frame_returned)) {
						// Baker: EOF (or error)
						CL_RestartVideo (video); // Baker: This does what?
						if (video->state == CLVIDEO_PLAY_1) {
							// Baker: The video state will be video->state == CLVIDEO_PLAY_1
							// barring anything weird ..
							// And we set it to first frame.
							video->state = CLVIDEO_FIRSTFRAME_4;
						}
						return;
					} // If EOF

					// FLIP ...
					goto gif_bypass; // We will do texture upload ...
				}

				// NO FLIP
				continue;
				
				
			}


			if (video->framenum_vid < destframe) {
				// Baker: This can flip multiple times, yes?
				// decodeframe has no idea
				do {
					video->framenum_vid ++;
					WARP_X_ (dpvsimpledecode_video)
					// Baker: video->vimagedata is updated here
					// What is our rtexture?
					if (video->decodeframe (video->stream,  video->vimagedata, 
						cl_videormask, 
						cl_videogmask, 
						cl_videobmask, 
						cl_videobytesperpixel_4, 
						cl_videobytesperpixel_4 * video->width)) 
					{ 
						// Baker: EOF (or error)
						CL_RestartVideo (video); // Baker: This does what?
						if (video->state == CLVIDEO_PLAY_1) {
							// Baker: The video state will be video->state == CLVIDEO_PLAY_1
							// barring anything weird ..
							// And we set it to first frame.
							video->state = CLVIDEO_FIRSTFRAME_4;
						}
						return; // Baker: WTF .. this is a for loop for all videos
					}
				} while(video->framenum_vid < destframe);

gif_bypass: NULLSTATEMENT ();
				rtexture_t *rt = Draw_GetPicTexture(video->cpif);
				// skinframe_t *sk = video->cpif->skinframe;
				
				WARP_X_ (DrawQ_SuperPic_Video)

				// Baker: rt can be null here
				R_MarkDirtyTexture(rt); // CPIF DIRTY .. Baker, this accepts NULL and returns
			} // If time to do frame

		} // not suspended
	} // for each video

	// stop main video
	if (cl_videos->state == CLVIDEO_FIRSTFRAME_4)
		CL_VideoStop(REASON_PLAYBACK_DONE_3);

	// reduce range to exclude unnecessary entries
	while (cl_num_videos > 0 && cl_videos[cl_num_videos-1].state == CLVIDEO_UNUSED_0)
		cl_num_videos--;
}

void CL_PurgeOwner(int owner)
{
	WARP_X_ (VID_OWNER_MENU_1)
	for (int i = 0 ; i < cl_num_videos ; i++)
		if (cl_videos[i].ownertag == owner)
			CL_CloseVideo(&cl_videos[i]);
}

typedef struct
{
	dp_font_t *font;
	float x;
	float y;
	float width;
	float height;
	float alignment; // 0 = left, 0.5 = center, 1 = right
	float fontsize;
	float textalpha;
}
cl_video_subtitle_info_t;

static float CL_DrawVideo_WordWidthFunc(void *passthrough, const char *w, size_t *length, float maxWidth)
{
	cl_video_subtitle_info_t *si = (cl_video_subtitle_info_t *) passthrough;

	if (w == NULL)
		return si->fontsize * si->font->maxwidth;
	if (maxWidth >= 0)
		return DrawQ_TextWidth_UntilWidth(w, length, si->fontsize, si->fontsize, false, si->font, -maxWidth); // -maxWidth: we want at least one char
	else if (maxWidth == -1)
		return DrawQ_TextWidth(w, *length, si->fontsize, si->fontsize, false, si->font);
	else
		return 0;
}

static int CL_DrawVideo_DisplaySubtitleLine(void *passthrough, const char *line, size_t length, float width, qbool isContinuation)
{
	cl_video_subtitle_info_t *si = (cl_video_subtitle_info_t *) passthrough;

	int x = (int) (si->x + (si->width - width) * si->alignment);
	if (length > 0)
		DrawQ_String(x, si->y, line, length, si->fontsize, si->fontsize, 1.0, 1.0, 1.0, si->textalpha, 0, NULL, false, si->font);
	si->y += si->fontsize;
	return 1;
}

int cl_videoplaying = false; // old, but still supported


// Baker: Ugly .dpv globals we are using for now ...
int is_q3_shader_video_tex; // Baker: Stupid global to transport data to Mod_LoadTextureFromQ3Shader
byte *is_q3_shader_video_tex_vimagedata;

void CL_DrawVideo_SCR_DrawScreen(void)
{
	clvideo_t *video;
	float videotime, px, py, sx, sy, st[8], b;
	cl_video_subtitle_info_t si;
	int i;

	if (!cl_videoplaying) // cl_videoplaying
		return;

	video = CL_GetVideoBySlot( 0 );

	// fix cvars
	if (cl_video_scale.value <= 0 || cl_video_scale.value > 1)
		Cvar_SetValueQuick( &cl_video_scale, 1);
	if (cl_video_brightness.value <= 0 || cl_video_brightness.value > 10)
		Cvar_SetValueQuick( &cl_video_brightness, 1);

	// calc video proportions
	px = 0;
	py = 0;
	sx = vid_conwidth.integer;
	sy = vid_conheight.integer;
	st[0] = 0.0; st[1] = 0.0; 
	st[2] = 1.0; st[3] = 0.0; 
	st[4] = 0.0; st[5] = 1.0; 
	st[6] = 1.0; st[7] = 1.0; 
	if (cl_video_keepaspectratio.integer)
	{
		float a = video->getaspectratio(video->stream) / ((float)vid.width / (float)vid.height);
		if (cl_video_keepaspectratio.integer >= 2)
		{
			// clip instead of scale
			if (a < 1.0) // clip horizontally
			{
				st[1] = st[3] = (1 - a)*0.5;
				st[5] = st[7] = 1 - (1 - a)*0.5;
			}
			else if (a > 1.0) // clip vertically
			{
				st[0] = st[4] = (1 - 1/a)*0.5;
				st[2] = st[6] = (1/a)*0.5;
			}
		}
		else if (a < 1.0) // scale horizontally
		{
			px += sx * (1 - a) * 0.5;
			sx *= a;
		}
		else if (a > 1.0) // scale vertically
		{
			a = 1 / a;
			py += sy * (1 - a);
			sy *= a;
		}
	}

	if (cl_video_scale.value != 1)
	{
		px += sx * (1 - cl_video_scale.value) * 0.5;
		py += sy * (1 - cl_video_scale.value) * ((bound(-1, cl_video_scale_vpos.value, 1) + 1) / 2);
		sx *= cl_video_scale.value;
		sy *= cl_video_scale.value;
	}

	// calc brightness for fadein and fadeout effects
	b = cl_video_brightness.value;
	if (cl_video_fadein.value && (host.realtime - video->starttime) < cl_video_fadein.value)
		b = pow((host.realtime - video->starttime)/cl_video_fadein.value, 2);

// Baker: There is no way to calculate time left as we have no information on how long the video is

	else if (cl_video_fadeout.value && ((video->starttime + SECS_INTO_VID_BY_FRAMENUM(video) - host.realtime) < cl_video_fadeout.value))
		b = pow(((video->starttime + SECS_INTO_VID_BY_FRAMENUM(video)) - host.realtime)/cl_video_fadeout.value, 2);

	// draw black bg in case stipple is active or video is scaled
	if (cl_video_stipple.integer /*d: 0*/ || px != 0 || py != 0 || sx != vid_conwidth.integer || sy != vid_conheight.integer)
		DrawQ_Fill(0, 0, vid_conwidth.integer, vid_conheight.integer, 0, 0, 0, 1, 0);

	// enable video-only polygon stipple (of global stipple is not active)
	if (!scr_stipple.integer /*d: 0*/ && cl_video_stipple.integer /*d: 0*/) {
		Con_PrintLinef ("FIXME: cl_video_stipple not implemented");
		Cvar_SetValueQuick(&cl_video_stipple, 0);
	}

	// draw video
	// CPIFX: We get not found here.

	is_q3_shader_video_tex = true;
	is_q3_shader_video_tex_vimagedata = (unsigned char *)video->vimagedata;

	video->baker_tex.width = video->width;
	video->baker_tex.height = video->height;
	if (v_glslgamma_video.value >= 1 /*d: 1*/) {
		// This is the norm
		DrawQ_SuperPic_Video(&video->baker_tex, px, py, 
			video->cpif, sx, sy, st[0], st[1], b, b, b, 1, 
		st[2], st[3], b, b, b, 1, st[4], st[5], b, b, b, 1, st[6], st[7], b, b, b, 1, DRAWFLAG_NORMAL_0);
	}
	else
	{
		DrawQ_SuperPic_Video(&video->baker_tex, px, py, video->cpif, sx, sy, st[0], st[1], b, b, b, 1, st[2], st[3], b, b, b, 1, st[4], st[5], b, b, b, 1, st[6], st[7], b, b, b, 1, DRAWFLAG_NOGAMMA);
		if (v_glslgamma_video.value > 0.0)
			DrawQ_SuperPic_Video(&video->baker_tex, px, py, video->cpif, sx, sy, st[0], st[1], b, b, b, v_glslgamma_video.value, st[2], st[3], b, b, b, v_glslgamma_video.value, st[4], st[5], b, b, b, v_glslgamma_video.value, st[6], st[7], b, b, b, v_glslgamma_video.value, DRAWFLAG_NOGAMMA);
	}

	is_q3_shader_video_tex = false;

	// VorteX: draw subtitle_text
	if (!video->subtitles || !cl_video_subtitles.integer /*d: 0*/)
		return;

	// find current subtitle
	videotime = host.realtime - video->starttime;
	for (i = 0; i < video->subtitles; i++) {
		if (videotime >= video->subtitle_start[i] && videotime <= video->subtitle_end[i])
		{
			// found, draw it
			si.font = FONT_NOTIFY;
			si.x = vid_conwidth.integer * 0.1;
			si.y = vid_conheight.integer - (max(1, cl_video_subtitles_lines.value) * cl_video_subtitles_textsize.value);
			si.width = vid_conwidth.integer * 0.8;
			si.height = max(1, cl_video_subtitles_lines.integer) * cl_video_subtitles_textsize.value;
			si.alignment = 0.5;
			si.fontsize = cl_video_subtitles_textsize.value;
			si.textalpha = min(1, (videotime - video->subtitle_start[i])/0.5) * min(1, ((video->subtitle_end[i] - videotime)/0.3)); // fade in and fade out
			COM_Wordwrap_Num_Rows_Drawn(video->subtitle_text[i], strlen(video->subtitle_text[i]), 0, si.width, CL_DrawVideo_WordWidthFunc, &si, CL_DrawVideo_DisplaySubtitleLine, &si);
			break;
		}
	}
}

// Baker: Exclusively from playvideo command.  Not cin_open.
WARP_X_ (VM_cin_open uses CL_Cin_OpenVideo)

int cl_videoplaying_sound;
void CL_PlayVideo_Start (char *filename, const char *subtitlesfile, const char *soundname)
{
	CL_StartVideo();

	if (cl_videos->state != CLVIDEO_UNUSED_0)
		CL_CloseVideo (cl_videos);

	// already contains "video/" prefix
	WARP_X_ (VM_cin_open)
	if (!OpenVideo(cl_videos, filename, filename, VID_OWNER_NON_MENU_0, subtitlesfile))
		return;

	// expand the active range to include the new entry
	cl_num_videos = Largest(cl_num_videos, 1);

	cl_videoplaying = true;

	CL_SetVideoState (cl_videos, CLVIDEO_PLAY_1);
	CL_RestartVideo	 (cl_videos);
	
	// Baker: honor the -nosound command line parameter :(
	// #pragma message ("How does this react and conprint if can't find the sound")
	if (false == Sys_CheckParm("-nosound"))
		//S_Play_Common (cmd_local, /*vol*/ 1.0f, /*attenuation*/ 1.0f);
		#pragma message ("Baker: Sound channel '9' seems to work but is it 'right' to do this?")
		if (S_LocalSoundEx (soundname, /*channel*/ 9, /*volume*/ 1.0f)) {
			cl_videoplaying_sound = true;
		}
}

void CL_Video_KeyEvent (int key, int ascii, qbool down ) 
{
	// only react to up events, to allow the user to delay the abortion point if it suddenly becomes interesting..
	if (!down) {
		// Baker: K_ESCAPE will do CL_VideoStop elsewhere and earlier ...
		if (isin5 (key, K_ESCAPE, K_ENTER, K_SPACE, K_MOUSE1, K_MOUSE2)) {
			CL_VideoStop (REASON_USER_ESCAPE_2);
		}
	}
}


void CL_VideoStop (int reason)
{
	cl_videoplaying = false;
	if (cl_videoplaying_sound) {
		// Stop the channel 0 sound
#if 0
		// Baker: This didn't work
		S_StopChannel (/*channel*/ 0, /*lockmuted*/ true, /*freesfx?*/ true);
#endif
#pragma message ("Baker: Sound channel '9' seems to work but is it 'right' to do this?")
		S_LocalSoundEx ("misc/null.wav", /*channel*/ 9, /*vol*/ 0); 
		cl_videoplaying_sound = false;
	}

	CL_CloseVideo (cl_videos);
}

static void CL_PlayVideo_f(cmd_state_t *cmd)
{
	CL_StartVideo (); // Baker: Starts up vid_init if needed

	if (Sys_CheckParm("-benchmark"))
		return;

	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("usage: playvideo <videoname> [custom_subtitles_file]" NEWLINE
			"plays video named video/<videoname>.dpv" NEWLINE
			"if custom subtitles file is not presented" NEWLINE 
			"it tries video/<videoname>.dpsub");
		return;
	}
	
	const char *extension = FS_FileExtension(Cmd_Argv(cmd, 1));

	// Baker: This is default extension
	// _fps is folder of jpegs, the folder is expected to indicate the frames per second
	// such as video/my_vid_20_fps/*.jpg
	// Construct the sound name
	char s_soundname[MAX_QPATH_128];
	c_dpsnprintf1 (s_soundname, "video/%s.ogg", Cmd_Argv(cmd, 1));

	char name[MAX_QPATH_128];
	int has_extension_already = extension[0] || String_Ends_With (Cmd_Argv(cmd, 1), "_fps");
	c_dpsnprintf2 (name, "video/%s%s", Cmd_Argv(cmd, 1), has_extension_already ? "" : ".dpv");

	// Baker: Extra param indicates a custom subtitles file
	if (Cmd_Argc(cmd) > 2)
		CL_PlayVideo_Start (name, Cmd_Argv(cmd, 2), s_soundname);
	else {
		char subtitlesfile[MAX_QPATH_128];
		// Baker: .gif support ... remove the extension first before adding dpsubs
		c_dpsnprintf1 (subtitlesfile, "video/%s", Cmd_Argv(cmd, 1));
		File_URL_Edit_Remove_Extension (subtitlesfile);
		c_strlcat (subtitlesfile, ".dpsubs");
		CL_PlayVideo_Start (name, subtitlesfile, s_soundname);
	}

	// Close the console
	Con_CloseConsole_If_Client(); // Baker r1003: close console for map/load/etc.

	if (host.hook.ToggleMenu)
		host.hook.ToggleMenu();
}

static void CL_StopVideo_f (cmd_state_t *cmd)
{
	CL_VideoStop (REASON_STOPVIDEO_CMD_4);
}

static void cl_video_start( void )
{
	int i;
	clvideo_t *video;

	cl_videotexturepool = R_AllocTexturePool();

	for( video = cl_videos, i = 0 ; i < cl_num_videos ; i++, video++ )
		if ( video->state != CLVIDEO_UNUSED_0 && !video->suspended )
			LinkVideoTexture( video );
}

static void cl_video_shutdown( void )
{
	int i;
	clvideo_t *video;

	for( video = cl_videos, i = 0 ; i < cl_num_videos ; i++, video++ ) {
		//if (??)
		if (video->state != CLVIDEO_UNUSED_0 && !video->suspended)
			SuspendVideo (video);
	}
	R_FreeTexturePool( &cl_videotexturepool );
}

static void cl_video_newmap( void )
{
}

void CL_Video_Init( void )
{
	union
	{
		unsigned char b[4];
		unsigned int i;
	}
	bgra;

	cl_num_videos = 0;

	// set masks in an endian-independent way (as they really represent bytes)
	bgra.i = 0;bgra.b[0] = 0xFF;cl_videobmask = bgra.i;
	bgra.i = 0;bgra.b[1] = 0xFF;cl_videogmask = bgra.i;
	bgra.i = 0;bgra.b[2] = 0xFF;cl_videormask = bgra.i;

	Cmd_AddCommand(CF_CLIENT, "playvideo", CL_PlayVideo_f, "play a .dpv video file" );
	Cmd_AddCommand(CF_CLIENT, "stopvideo", CL_StopVideo_f, "stop playing a .dpv video file" );

	Cvar_RegisterVariable(&cl_video_subtitles);
	Cvar_RegisterVariable(&cl_video_subtitles_lines);
	Cvar_RegisterVariable(&cl_video_subtitles_textsize);
	Cvar_RegisterVariable(&cl_video_scale);
	Cvar_RegisterVariable(&cl_video_scale_vpos);
	Cvar_RegisterVariable(&cl_video_brightness);
	Cvar_RegisterVariable(&cl_video_stipple);
	Cvar_RegisterVariable(&cl_video_keepaspectratio);
	Cvar_RegisterVariable(&cl_video_fadein);
	Cvar_RegisterVariable(&cl_video_fadeout);

	Cvar_RegisterVariable(&v_glslgamma_video);

	R_RegisterModule( "CL_Video", cl_video_start, cl_video_shutdown, cl_video_newmap, NULL, NULL );

	LibAvW_OpenLibrary();
}

void CL_Video_Shutdown( void )
{
	int i;

	for (i = 0 ; i < cl_num_videos ; i++)
		CL_CloseVideo(&cl_videos[ i ]);

	LibAvW_CloseLibrary();
}
