// cl_screen.h

#ifndef CL_SCREEN_H
#define CL_SCREEN_H

#include "qtypes.h"

void SHOWLMP_decodehide(void);
void SHOWLMP_decodeshow(void);
void SHOWLMP_drawall(void);

extern struct cvar_s vid_conwidth;
extern struct cvar_s vid_conheight;

extern struct cvar_s vid_pixelheight;

extern struct cvar_s scr_screenshot_jpeg;
extern struct cvar_s scr_screenshot_jpeg_quality;
extern struct cvar_s scr_screenshot_png;
extern struct cvar_s scr_screenshot_gammaboost;
extern struct cvar_s scr_screenshot_name;

extern struct cvar_s tool_inspector; // Baker r0106: tool inspector
extern struct cvar_s tool_marker; // Baker r0109: tool marker

void CL_Screen_NewMap(void);
void CL_Screen_Init(void);
void CL_Screen_Shutdown(void);
void CL_UpdateScreen(void);

qbool R_Stereo_Active(void);
qbool R_Stereo_ColorMasking(void);

// Baker: Take a screenshot and make it a JPEG base64 string
// We use this to embed screenshots into the save game file.
char *Screenshot_To_Jpeg_String_Malloc_512_320 (void);
byte *Screenshot_To_Jpeg_Blob_ZAlloc_512_320 (size_t *pblobsizeout);

extern int old_vid_kickme; // Baker: 360p scaling


WARP_X_ (SCR_gifclip_f)


typedef struct gd_Palette {
    int size;
    byte colors[0x100 * RGB_3];
} gd_Palette;

typedef struct gd_GCE {
    unsigned short	delay_100;				// 1/100 of a second delay
    byte			transparency_index;		// What is this
    byte			disposal_method;		// default: Add non-transparent pixels to canvas
											// 2: Restore to background color.
											// 3: Restore to previous, i.e., don't update canvas
    int				gifinput;
    int				transparency;			// a boolean, does it have transparency?
} gd_GCE;

typedef struct gd_GIF_s {
	bakerbuf_t		b;

	size_t			blobz_size;
	byte			*blobz_alloc;
	qbool			shall_process_data;

	off_t			anim_start;
    unsigned short	width, height;
    unsigned short	depth;
    unsigned short	loop_count;
    gd_GCE			gce;
    gd_Palette		*palette;
    gd_Palette		lct, gct;

    void (*plain_text)(
        struct gd_GIF_s *gif, unsigned short tx, unsigned short ty,
        unsigned short tw, unsigned short th, byte cw, byte ch,
        byte fg, byte bg
    );

	void (*comment)(struct gd_GIF_s *gif);
    void (*application)(struct gd_GIF_s *gif, char id[8], char auth[3]);
    unsigned short	fx, fy, fw, fh;
    byte			bgindex;
    rgb3			*canvas_rgb3;
	ubpalette1		*frame_palette_pels;
} gd_GIF_t;


void *gd_open_gif_alloc(const char *fname);
void *gd_open_gif_from_memory_alloc (const void *data, size_t datalen);
	int gif_get_length_numframes (const void *data, size_t datasize, double *ptotal_seconds);
	int gd_get_frame (void *z, qbool shall_process_data);
	void gd_render_frame(void *z, byte *buffer);
	int gd_is_bgcolor(void *z, byte color[3]);
	void gd_rewind (void *z);
	
	int gd_getwidth (void *z);
	int gd_getheight (void *z);

void *gd_close_gif_maybe_returns_null (void *z);

extern int is_q3_shader_video_tex; // Baker: Global to transport data to Mod_LoadTextureFromQ3Shader
extern byte *is_q3_shader_video_tex_vimagedata;

#define DYNAMICTEX_Q3_START(vimagedata_bgra4) \
	is_q3_shader_video_tex_vimagedata = (byte *)vimagedata_bgra4; \
	is_q3_shader_video_tex = true // Ender

#define DYNAMICTEX_Q3_END() \
	is_q3_shader_video_tex_vimagedata = NULL; \
	is_q3_shader_video_tex = false // Ender


bgra4 *SCR_Screenshot_Get_JPEG_BGRA4_From_Save_File_Alloc (const char *s_savegame_name);


#endif // ! CL_SCREEN_H

