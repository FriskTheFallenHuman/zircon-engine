
#ifndef CL_VIDEO_H
#define CL_VIDEO_H

#include "qtypes.h"
#include "qdefs.h"

#define CL_VIDEO_SLASH_PREFIX	"video/"
#define CL_SUSPEND_THRESHOLD_SECS_2_0		2.0

#define VID_OWNER_NON_MENU_0	0
#define VID_OWNER_MENU_1		1

typedef enum clvideostate_e
{
	CLVIDEO_UNUSED_0,
	CLVIDEO_PLAY_1,
	CLVIDEO_LOOP_2,		// Baker: Does not occur in source code, cin_setstate could do this
	CLVIDEO_PAUSE_3,	// Baker: Does not occur in source code, cin_setstate could do this
	CLVIDEO_FIRSTFRAME_4,
	CLVIDEO_RESETONWAKEUP_5,
	CLVIDEO_STATECOUNT_6
} clvideostate_t;

#define CLVIDEO_MAX_SUBTITLES_512 512

extern struct cvar_s cl_video_subtitles;
extern struct cvar_s cl_video_subtitles_lines;
extern struct cvar_s cl_video_subtitles_textsize;
extern struct cvar_s cl_video_scale;
extern struct cvar_s cl_video_scale_vpos;
extern struct cvar_s cl_video_stipple;
extern struct cvar_s cl_video_brightness;
extern struct cvar_s cl_video_keepaspectratio;

typedef struct clvideo_s
{
	int		ownertag;
	clvideostate_t state;

	// private stuff
	void	*stream;

	double	starttime;
	int		framenum_vid;
	double	framegif_endtime;
	int		is_gif_stream;

	double	frames_per_second; // Baker: Think of this as frames per second

	void	*vimagedata;


	// Models tend to have texture_t and so does mesh.
	WARP_X_ (open LinkVideoTexture R_MakeTextureDynamic)
	WARP_X_ (free UnlinkVideoTexture /*this does not destroy the cachepic_t*/)
	WARP_X_ (video->decodeframe -> dpvsimpledecode_video /*this does not destroy the cachepic_t*/)
	WARP_X_ (R_MarkDirtyTexture clvideo_t)
	WARP_X_ (R_UploadFullTexture R_UploadPartialTexture )
	WARP_X_ (skinframe_t -> base r_texture )
	WARP_X_ (DrawQ_SuperPic_Video Mod_Mesh_GetTexture)

	// cachepic holds the relevant texture_t and we simply update the texture as needed
	struct cachepic_s *cpif;

	texture_t baker_tex; // Hopefully we can fill this in

	char	name[MAX_QPATH_128]; // name of this video UI element (not the filename)
	int		width;
	int		height;

	// VorteX: subtitles array
	int		subtitles;
	char	*subtitle_text[CLVIDEO_MAX_SUBTITLES_512];
	float	subtitle_start[CLVIDEO_MAX_SUBTITLES_512];
	float	subtitle_end[CLVIDEO_MAX_SUBTITLES_512];

	// this functions gets filled by video format module
	void (*close) (void *stream);
	unsigned int (*getwidth) (void *stream);
	unsigned int (*getheight) (void *stream);
	double (*getframerate) (void *stream);
	double (*getaspectratio) (void *stream);
	int (*decodeframe) (void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow);
	int (*decodeframegif) (struct clvideo_s *video, void *stream, void *imagedata, 
		unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, 
		int imagebytesperrow, int framenum_wanted, double time_wanted, int *pframe_returned);

	// if a video is suspended, it is automatically paused (else we'd still have to process the frames)
	// used to determine whether the video's resources should be freed or not
    double  lasttime;
	// when lasttime - realtime > THRESHOLD, all but the stream is freed
	qbool suspended;

	char	filename[MAX_QPATH_128];
} clvideo_t;

clvideo_t *	CL_Cin_OpenVideo (const char *filename, const char *name, int owner, const char *subtitlesfile);
clvideo_t *	CL_GetVideoByName (const char *name);
void		CL_SetVideoState (clvideo_t *video, clvideostate_t state);
void		CL_RestartVideo	(clvideo_t *video);

void		CL_CloseVideo	(clvideo_t *video);
void		CL_PurgeOwner	(int owner);

void		CL_Video_Frame (void); // update all videos
void		CL_Video_Init  (void);
void		CL_Video_Shutdown (void);

// old interface
extern int cl_videoplaying;

void CL_DrawVideo_SCR_DrawScreen (void);
void CL_PlayVideo_Start (char *filename, const char *subtitlesfile, const char *soundname);
void CL_VideoStop(int stop_reason);

#define REASON_NEW_MAP_1		1
#define REASON_USER_ESCAPE_2	2
#define REASON_PLAYBACK_DONE_3	3
#define REASON_STOPVIDEO_CMD_4	4

// new function used for fullscreen videos
// TODO: Andreas Kirsch: move this subsystem somewhere else (preferably host) since the cl_video system shouldnt do such work like managing key events..
void CL_Video_KeyEvent( int key, int ascii, qbool down );

#endif
