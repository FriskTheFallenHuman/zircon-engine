/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "quakedef.h"
#include "image.h"
#include "wad.h"

#include "cl_video.h"

#include "ft2.h"
#include "ft2_fontdefs.h"


dp_fonts_t dp_fonts;
/*static */mempool_t *fonts_mempool = NULL;

#if 0 // FFA
const char *g_p_selbeyond;
int g_p_selbeyond_x;
#endif

cvar_t r_textshadow = {CF_CLIENT | CF_ARCHIVE, "r_textshadow", "0", "draws a shadow on all text to improve readability (note: value controls offset, 1 = 1 pixel, 1.5 = 1.5 pixels, etc)"};
cvar_t r_textbrightness = {CF_CLIENT | CF_ARCHIVE, "r_textbrightness", "0", "additional brightness for text color codes (0 keeps colors as is, 1 makes them all white)"};
cvar_t r_textcontrast = {CF_CLIENT | CF_ARCHIVE, "r_textcontrast", "1", "additional contrast for text color codes (1 keeps colors as is, 0 makes them all black)"};

cvar_t r_font_postprocess_blur = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_blur", "0", "font blur amount"};
cvar_t r_font_postprocess_outline = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_outline", "0", "font outline amount"};
cvar_t r_font_postprocess_shadow_x = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_x", "0", "font shadow X shift amount, applied during outlining"};
cvar_t r_font_postprocess_shadow_y = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_y", "0", "font shadow Y shift amount, applied during outlining"};
cvar_t r_font_postprocess_shadow_z = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_z", "0", "font shadow Z shift amount, applied during blurring"};
cvar_t r_font_hinting = {CF_CLIENT | CF_ARCHIVE, "r_font_hinting", "3", "0 = no hinting, 1 = light autohinting, 2 = full autohinting, 3 = full hinting"};
cvar_t r_font_antialias = {CF_CLIENT | CF_ARCHIVE, "r_font_antialias", "1", "0 = monochrome, 1 = grey" /* , 2 = rgb, 3 = bgr" */};
cvar_t r_nearest_2d = {CF_CLIENT | CF_ARCHIVE, "r_nearest_2d", "1", "use nearest filtering on all 2d textures (including conchars) [Zircon default]"}; // Baker r8103 - default 1 for crispness
cvar_t r_nearest_conchars = {CF_CLIENT | CF_ARCHIVE, "r_nearest_conchars", "1", "use nearest filtering on conchars texture [Zircon default]"}; // Baker r8103 - default 1 for crispness

//=============================================================================
/* Support Routines */

static cachepic_t *cachepichash[CACHEPICHASHSIZE_256];
static cachepic_t cachepics[MAX_CACHED_PICS_2048];
static int numcachepics;

rtexturepool_t *drawtexturepool;

int draw_frame = 1;

/*
================
Draw_CachePic
================
*/
// FIXME: move this to client somehow
cachepic_t *Draw_CachePic_Flags(const char *path, unsigned int cachepicflags)
{
	int crc, hashkey;
	cachepic_t *pic;
	int texflags;

	texflags = TEXF_ALPHA;
	if (!(cachepicflags & CACHEPICFLAG_NOCLAMP))
		texflags |= TEXF_CLAMP;
	if (cachepicflags & CACHEPICFLAG_MIPMAP)
		texflags |= TEXF_MIPMAP;
	if (!(cachepicflags & CACHEPICFLAG_NOCOMPRESSION) && gl_texturecompression_2d.integer && gl_texturecompression.integer)
		texflags |= TEXF_COMPRESS;
	if (cachepicflags & CACHEPICFLAG_LINEAR)
		texflags |= TEXF_FORCELINEAR;
	else if ((cachepicflags & CACHEPICFLAG_NEAREST) || r_nearest_2d.integer)
		texflags |= TEXF_FORCENEAREST;

	// check whether the picture has already been cached
	crc = CRC_Block((unsigned char *)path, strlen(path));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE_256;

	for (pic = cachepichash[hashkey];pic;pic = pic->chain)
	{
		if (String_Match(path, pic->name))
		{
			// if it was created (or replaced) by Draw_NewPic, just return it
			if (!(pic->cacheflags & CACHEPICFLAG_NEWPIC))
			{
				// reload the pic if texflags changed in important ways
				// ignore TEXF_COMPRESS when comparing, because fallback pics remove the flag, and ignore TEXF_MIPMAP because QC specifies that
				if ((pic->texflags ^ texflags) & ~(TEXF_COMPRESS | TEXF_MIPMAP))
				{
					Con_DPrintf ("Draw_CachePic(" QUOTED_S "): frame %d: reloading pic due to mismatch on flags\n", path, draw_frame);
					goto reload;
				}
				if (!pic->skinframe || !pic->skinframe->base)
				{
					if (pic->cacheflags & CACHEPICFLAG_FAILONMISSING_256)
						return NULL;
					Con_DPrintf ("Draw_CachePic(" QUOTED_S "): frame %d: reloading pic\n", path, draw_frame);
					goto reload;
				}
				if (!(cachepicflags & CACHEPICFLAG_NOTPERSISTENT))
					pic->autoload = false; // caller is making this pic persistent
			}
			if (pic->skinframe)
				R_SkinFrame_MarkUsed(pic->skinframe);
			pic->lastusedframe = draw_frame;
			return pic;
		}
	}

	if (numcachepics == MAX_CACHED_PICS_2048)
	{
		Con_DPrintLinef ("Draw_CachePic(" QUOTED_S "): frame %d: numcachepics == MAX_CACHED_PICS_2048", path, draw_frame);
		// FIXME: support NULL in callers?
		return cachepics; // return the first one
	}
	Con_DPrintLinef ("Draw_CachePic(" QUOTED_S "): frame %d: loading pic%s", path, draw_frame, (cachepicflags & CACHEPICFLAG_NOTPERSISTENT) ? " notpersist" : "");
	pic = cachepics + (numcachepics++);
	memset(pic, 0, sizeof(*pic));
	strlcpy (pic->name, path, sizeof(pic->name));
	// link into list
	pic->chain = cachepichash[hashkey];
	cachepichash[hashkey] = pic;

reload:
	if (pic->skinframe)
		R_SkinFrame_PurgeSkinFrame(pic->skinframe);

	pic->cacheflags = cachepicflags;
	pic->texflags = texflags;
	pic->autoload = (cachepicflags & CACHEPICFLAG_NOTPERSISTENT) != 0;
	pic->lastusedframe = draw_frame;

	if (pic->skinframe)
	{
		// reload image after it was unloaded or texflags changed significantly
		R_SkinFrame_LoadExternal_SkinFrame(pic->skinframe, pic->name, texflags | TEXF_FORCE_RELOAD, (cachepicflags & CACHEPICFLAG_QUIET) == 0, (cachepicflags & CACHEPICFLAG_FAILONMISSING_256) == 0);
	}
	else
	{
		// load high quality image (this falls back to low quality too)
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, texflags | TEXF_FORCE_RELOAD, (cachepicflags & CACHEPICFLAG_QUIET) == 0, (cachepicflags & CACHEPICFLAG_FAILONMISSING_256) == 0);
	}

	// get the dimensions of the image we loaded (if it was successful)
	if (pic->skinframe && pic->skinframe->base)
	{
		pic->width = R_TextureWidth(pic->skinframe->base);
		pic->height = R_TextureHeight(pic->skinframe->base);
	}

	// check for a low quality version of the pic and use its size if possible, to match the stock hud
	Image_GetStockPicSize(pic->name, &pic->width, &pic->height);

	return pic;
}

cachepic_t *Draw_CachePic (const char *path)
{
	return Draw_CachePic_Flags (path, 0); // default to persistent!
}

const char *Draw_GetPicName(cachepic_t *pic)
{
	if (pic == NULL)
		return "";
	return pic->name;
}

int Draw_GetPicWidth(cachepic_t *pic)
{
	if (pic == NULL)
		return 0;
	return pic->width;
}

int Draw_GetPicHeight(cachepic_t *pic)
{
	if (pic == NULL)
		return 0;
	return pic->height;
}

qbool Draw_IsPicLoaded(cachepic_t *pic)
{
	if (pic == NULL)
		return false;
	if (pic->autoload && (!pic->skinframe || !pic->skinframe->base))
	{
		Con_DPrintLinef ("Draw_IsPicLoaded(" QUOTED_S "): Loading external skin", pic->name);
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, pic->texflags | TEXF_FORCE_RELOAD, q_tx_complain_false, q_tx_fallback_notexture_true);
	}
	// skinframe will only be NULL if the pic was created with CACHEPICFLAG_FAILONMISSING_256 and not found
	return pic->skinframe != NULL && pic->skinframe->base != NULL;
}

rtexture_t *Draw_GetPicTexture(cachepic_t *pic)
{
	if (pic == NULL)
		return NULL;
	if (pic->autoload && (!pic->skinframe || !pic->skinframe->base))
	{
		Con_DPrintLinef ("Draw_GetPicTexture(" QUOTED_S "): Loading external skin", pic->name);
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, pic->texflags | TEXF_FORCE_RELOAD, q_tx_complain_false, q_tx_fallback_notexture_true);
	}
	pic->lastusedframe = draw_frame;
	return pic->skinframe ? pic->skinframe->base : NULL;
}

void Draw_Frame(void)
{
	int i;
	cachepic_t *pic;
	static double nextpurgetime;
	if (nextpurgetime > host.realtime)
		return;
	nextpurgetime = host.realtime + 0.05;
	for (i = 0, pic = cachepics;i < numcachepics;i++, pic++)
	{
		if (pic->autoload && pic->skinframe && pic->skinframe->base && pic->lastusedframe < draw_frame - 3)
		{
			Con_DPrintLinef ("Draw_Frame(%d): Unloading " QUOTED_S, draw_frame, pic->name);
			R_SkinFrame_PurgeSkinFrame(pic->skinframe);
		}
	}
	draw_frame++;
}

cachepic_t *Draw_NewPic(const char *picname, int width, int height, unsigned char *pixels_bgra, textype_t textype, int texflags)
{
	int crc, hashkey;
	cachepic_t *pic;

	crc = CRC_Block((unsigned char *)picname, strlen(picname));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE_256;
	for (pic = cachepichash[hashkey];pic;pic = pic->chain)
		if (String_Match (picname, pic->name))
			break;

	if (pic)
	{
		if (pic->cacheflags & CACHEPICFLAG_NEWPIC && pic->skinframe && pic->skinframe->base && pic->width == width && pic->height == height)
		{
			Con_DPrintLinef ("Draw_NewPic(" QUOTED_S "): frame %d: updating texture", picname, draw_frame);
			R_UpdateTexture(pic->skinframe->base, pixels_bgra, 0, 0, 0, width, height, 1, 0);
			R_SkinFrame_MarkUsed(pic->skinframe);
			pic->lastusedframe = draw_frame;
			return pic;
		}
		Con_DPrintLinef ("Draw_NewPic(" QUOTED_S "): frame %d: reloading pic because flags/size changed", picname, draw_frame);
	}
	else
	{
		if (numcachepics == MAX_CACHED_PICS_2048)
		{
			Con_DPrintLinef ("Draw_NewPic(" QUOTED_S "): frame %d: numcachepics == MAX_CACHED_PICS_2048", picname, draw_frame);
			// FIXME: support NULL in callers?
			return cachepics; // return the first one
		}
		Con_DPrintLinef ("Draw_NewPic(" QUOTED_S "): frame %d: creating new cachepic", picname, draw_frame);
		pic = cachepics + (numcachepics++);
		memset(pic, 0, sizeof(*pic));
		strlcpy (pic->name, picname, sizeof(pic->name));
		// link into list
		pic->chain = cachepichash[hashkey];
		cachepichash[hashkey] = pic;
	}

	R_SkinFrame_PurgeSkinFrame(pic->skinframe);

	pic->autoload = false;
	pic->cacheflags = CACHEPICFLAG_NEWPIC; // disable texflags checks in Draw_CachePic
	pic->cacheflags |= (texflags & TEXF_CLAMP) ? 0 : CACHEPICFLAG_NOCLAMP;
	pic->cacheflags |= (texflags & TEXF_FORCENEAREST) ? CACHEPICFLAG_NEAREST : 0;
	pic->width = width;
	pic->height = height;
	pic->skinframe = R_SkinFrame_LoadInternalBGRA (picname, texflags | TEXF_FORCE_RELOAD, pixels_bgra, width, height, 0, 0, 0, vid.sRGB2D, /*q1skyload*/ false);
	pic->lastusedframe = draw_frame;
	return pic;
}

void Draw_FreePic(const char *picname)
{
	int crc;
	int hashkey;
	cachepic_t *pic;
	// this doesn't really free the pic, but does free its texture
	crc = CRC_Block((unsigned char *)picname, strlen(picname));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE_256;
	for (pic = cachepichash[hashkey]; pic; pic = pic->chain) {
		if (String_Match (picname, pic->name) && pic->skinframe) {
			Con_DPrintLinef ("Draw_FreePic(" QUOTED_S "): frame %d: freeing pic", picname, draw_frame);
			R_SkinFrame_PurgeSkinFrame (pic->skinframe);
			return;
		}
	}
} 

static float snap_to_pixel_x(float x, float roundUpAt);
extern int con_linewidth; // to force rewrapping

#include "bundle_font.c.h"

WARP_X_ (VM_loadfont)
//float loadfont(string fontname, string fontmaps, string sizes, float slot)
//                   PARM0            PARM 1?         
//float loadfont(string fontname, string fontmaps, string sizes, float slot)
//ourfontImpact	= loadfont("user6", "fonts/Anton-Regular.ttf",		"32", -1, 0, 0);
// 3 duds at end are slot, scale and fontmap
void LoadFontDPEx (dp_font_t *dpf, /*const char *dpfontusername, */const char *font_ttf_list, const char *sizes_in, const byte *ft_data_in, fs_offset_t ft_datasize)
{
	const char *c, *cm;
	char mainfont[MAX_QPATH_128];
	if (!sizes_in[0])
		sizes_in = "10";


	memset	(dpf->fallbacks, 0,			sizeof(dpf->fallbacks));
	memset	(dpf->fallback_faces, 0,	sizeof(dpf->fallback_faces));

	// first font is handled "normally"
	c = strchr(font_ttf_list, ':');
	cm = strchr(font_ttf_list, ',');
	if (c && (!cm || c < cm))
		dpf->req_face = atoi(c+1);
	else
	{
		dpf->req_face = 0;
		c = cm;
	}

	// Check for no separator or length too long
	if (!c || (c - font_ttf_list) > MAX_QPATH_128)
		c_strlcpy	(mainfont, font_ttf_list);
	else
	{
		// Baker: This is a length copy with null termination
		c_strlcpy_size_z (mainfont, font_ttf_list, c - font_ttf_list);
	}

	// handle fallbacks
	for (int i = 0; i < MAX_FONT_FALLBACKS_3; ++i) {
		c = strchr(font_ttf_list, ','); // Find a comma
		if (!c)
			break;
		font_ttf_list = c + 1;
		if (!*font_ttf_list)
			break;
		c = strchr(font_ttf_list, ':');
		cm = strchr(font_ttf_list, ',');
		if (c && (!cm || c < cm))
			dpf->fallback_faces[i] = atoi(c+1);
		else {
			dpf->fallback_faces[i] = 0; // dpf->req_face; could make it stick to the default-font's face index
			c = cm;
		}
		if (!c || (c-font_ttf_list) > MAX_QPATH_128) {
			c_strlcpy (dpf->fallbacks[i], font_ttf_list);
		}
		else
		{
			c_strlcpy_size_z (dpf->fallbacks[i], font_ttf_list, c - font_ttf_list);
		}
	}

	// handle sizes
	for (int i = 0; i < MAX_FONT_SIZES_16; ++i)
		dpf->req_sizes[i] = -1;

	int numsizes;
	for (numsizes = 0, c = sizes_in; ; ) {
		if (!COM_ParseToken_VM_Tokenize(&c, 0))
			break;
		int sz = atof(com_token);
		// detect crap size
		if (sz < 0.001f || sz > 1000.0f) {
			//VM_Warningf (prog, "VM_loadfont: bad font size %s", com_token);
			continue;
		}

		// check overflow
		if (numsizes == MAX_FONT_SIZES_16) {
			//VM_Warningf(prog, "VM_loadfont: MAX_FONT_SIZES_16 = %d exceeded", MAX_FONT_SIZES_16);
			break;
		}
		dpf->req_sizes[numsizes] = sz;
		numsizes ++;
	}

	// load
	LoadFontDP (/*override?*/ true, mainfont, dpf, scale_1_0, /*voffset*/ 0, ft_data_in, ft_datasize);

	// return index of loaded font
	//PRVM_G_FLOAT(OFS_RETURN) = (dpf - dp_fonts.dpf);
}


void LoadFontDP (qbool override, const char *name, dp_font_t *fnt, float scale, float voffset, const byte *ft_data_in, fs_offset_t ft_datasize)
{
	int i, ch;
	float maxwidth;
	char widthfile[MAX_QPATH_128];
	char *widthbuf;
	fs_offset_t widthbufsize;

	if (override || !fnt->texpath[0]) {
		strlcpy(fnt->texpath, name, sizeof(fnt->texpath));
		// load the cvars when the font is FIRST loader
		fnt->settings.scale = scale;
		fnt->settings.voffset = voffset;
		fnt->settings.antialias = r_font_antialias.integer;
		fnt->settings.hinting = r_font_hinting.integer;
		fnt->settings.outline = r_font_postprocess_outline.value;
		fnt->settings.blur = r_font_postprocess_blur.value;
		fnt->settings.shadowx = r_font_postprocess_shadow_x.value;
		fnt->settings.shadowy = r_font_postprocess_shadow_y.value;
		fnt->settings.shadowz = r_font_postprocess_shadow_z.value;
	}
	// fix bad scale
	if (fnt->settings.scale <= 0)
		fnt->settings.scale = 1;

	if (drawtexturepool == NULL)
		return; // before gl_draw_start, so will be loaded later

	if (fnt->ft2)
	{
		// clear freetype font
		Font_UnloadFont(fnt->ft2);
		Mem_Free(fnt->ft2);
		fnt->ft2 = NULL;
	}

	if (fnt->req_face != -1) {
		if (false == Font_LoadFont(fnt->texpath, fnt, ft_data_in, ft_datasize))
			Con_DPrintLinef ("Failed to load font-file for " QUOTED_S ", it will not support as many characters.", fnt->texpath);
	}

	// Baker: The upload happens here
	fnt->pic_font = Draw_CachePic_Flags(fnt->texpath, CACHEPICFLAG_QUIET | CACHEPICFLAG_NOCOMPRESSION |
		(r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0) |
		CACHEPICFLAG_FAILONMISSING_256);
	if (!Draw_IsPicLoaded(fnt->pic_font))
	{
		for (i = 0; i < MAX_FONT_FALLBACKS_3; ++i)
		{
			if (!fnt->fallbacks[i][0])
				break;
			fnt->pic_font = Draw_CachePic_Flags(fnt->fallbacks[i], CACHEPICFLAG_QUIET | CACHEPICFLAG_NOCOMPRESSION | (r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0) | CACHEPICFLAG_FAILONMISSING_256);
			if (Draw_IsPicLoaded(fnt->pic_font))
				break;
		}
		if (!Draw_IsPicLoaded(fnt->pic_font))
		{
			fnt->pic_font = Draw_CachePic_Flags("gfx/conchars", CACHEPICFLAG_NOCOMPRESSION | (r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0));
			strlcpy(widthfile, "gfx/conchars.width", sizeof(widthfile));
		}
		else
			dpsnprintf(widthfile, sizeof(widthfile), "%s.width", fnt->fallbacks[i]);
	}
	else
		dpsnprintf(widthfile, sizeof(widthfile), "%s.width", fnt->texpath);

	// unspecified width == 1 (base width)
	for(ch = 0; ch < 256; ++ch)
		fnt->width_of[ch] = 1;

	// FIXME load "name.width", if it fails, fill all with 1
	if ((widthbuf = (char *) FS_LoadFile(widthfile, tempmempool, fs_quiet_true, &widthbufsize)))
	{
		float extraspacing = 0;
		const char *p = widthbuf;

		ch = 0;
		while(ch < 256)
		{
			if (!COM_ParseToken_Simple(&p, false, false, true))
				return;

			switch(*com_token)
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '+':
				case '-':
				case '.':
					fnt->width_of[ch] = atof(com_token) + extraspacing;
					ch++;
					break;
				default:
					if (String_Match(com_token, "extraspacing"))
					{
						if (!COM_ParseToken_Simple(&p, false, false, true))
							return;
						extraspacing = atof(com_token);
					}
					else if (String_Match(com_token, "scale")) // font
					{
						if (!COM_ParseToken_Simple(&p, false, false, true))
							return;
						fnt->settings.scale = atof(com_token);
					}
					else
					{
						Con_DPrintLinef ("Warning: skipped unknown font property %s", com_token);
						if (!COM_ParseToken_Simple(&p, false, false, true))
							return;
					}
					break;
			}
		}

		Mem_Free(widthbuf);
	}

	if (fnt->ft2)
	{
		for (i = 0; i < MAX_FONT_SIZES_16; ++i)
		{
			ft2_font_map_t *map = Font_MapForIndex(fnt->ft2, i);
			if (!map)
				break;
			for(ch = 0; ch < 256; ++ch)
				map->width_of[ch] = Font_SnapTo(fnt->width_of[ch], 1/map->size);
		}
	}

	maxwidth = fnt->width_of[0];
	for(i = 1; i < 256; ++i)
		maxwidth = max(maxwidth, fnt->width_of[i]);
	fnt->maxwidth = maxwidth;

	// fix up maxwidth for overlap
	fnt->maxwidth *= fnt->settings.scale;

	if (fnt == FONT_CONSOLE)
		con_linewidth = -1; // rewrap console in next frame

	if (fnt->ft2) {
		// Avoid div by 0
		if (fnt->ft2->ft_baker_height) {
			fnt->ft2->ft_baker_descend_pct = abs(fnt->ft2->ft_baker_descend) / (float)fnt->ft2->ft_baker_height;
			fnt->ft_baker_descend_pct = fnt->ft2->ft_baker_descend_pct;
		}
	}
}

extern cvar_t developer_font;
dp_font_t *FindFont(const char *title, qbool allocate_new)
{
	int i, oldsize;

	// find font
	for(i = 0; i < dp_fonts.maxsize_font; ++i)
		if (String_Match(dp_fonts.f[i].title, title))
			return &dp_fonts.f[i];
	// if not found - try allocate
	if (allocate_new)
	{
		// find any font with empty title
		for(i = 0; i < dp_fonts.maxsize_font; ++i)
		{
			if (String_Match(dp_fonts.f[i].title, ""))
			{
				strlcpy(dp_fonts.f[i].title, title, sizeof(dp_fonts.f[i].title));
				return &dp_fonts.f[i];
			}
		}
		// if no any 'free' fonts - expand buffer
		oldsize = dp_fonts.maxsize_font;
		dp_fonts.maxsize_font = dp_fonts.maxsize_font + FONTS_EXPAND;
		if (developer_font.integer)
			Con_PrintLinef ("FindFont: enlarging fonts buffer (%d -> %d)", oldsize, dp_fonts.maxsize_font);
		dp_fonts.f = (dp_font_t *)Mem_Realloc(fonts_mempool, dp_fonts.f, sizeof(dp_font_t) * dp_fonts.maxsize_font);
		// relink ft2 structures
		for(i = 0; i < oldsize; ++i)
			if (dp_fonts.f[i].ft2)
				dp_fonts.f[i].ft2->settings = &dp_fonts.f[i].settings;
		// register a font in first expanded slot
		c_strlcpy (dp_fonts.f[oldsize].title, title);
		return &dp_fonts.f[oldsize];
	}
	return NULL;
}

static float snap_to_pixel_x(float x, float roundUpAt)
{
	float pixelpos = x * vid.width / vid_conwidth.value;
	int snap = (int) pixelpos;
	if (pixelpos - snap >= roundUpAt) ++snap;
	return ((float)snap * vid_conwidth.value / vid.width);
	/*
	x = (int)(x * vid.width / vid_conwidth.value);
	x = (x * vid_conwidth.value / vid.width);
	return x;
	*/
}

static float snap_to_pixel_y(float y, float roundUpAt)
{
	float pixelpos = y * vid.height / vid_conheight.value;
	int snap = (int) pixelpos;
	if (pixelpos - snap > roundUpAt) ++snap;
	return ((float)snap * vid_conheight.value / vid.height);
	/*
	y = (int)(y * vid.height / vid_conheight.value);
	y = (y * vid_conheight.value / vid.height);
	return y;
	*/
}

static void LoadFont_f(cmd_state_t *cmd)
{
	dp_font_t *f;
	int i, sizes;
	const char *filelist, *c, *cm;
	float sz, scale, voffset;
	char mainfont[MAX_QPATH_128];

	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("Available font commands:");
		for (i = 0; i < dp_fonts.maxsize_font; ++i) {
			if (dp_fonts.f[i].title[0])
				Con_PrintLinef ("  loadfont %s gfx/tgafile[...] [custom switches] [sizes...]", dp_fonts.f[i].title);
		} // for

		Con_Printf ("A font can simply be gfx/tgafile, or alternatively you" NEWLINE
		   "can specify multiple fonts and faces" NEWLINE
		   "Like this: gfx/vera-sans:2,gfx/fallback:1" NEWLINE
		   "to load face 2 of the font gfx/vera-sans and use face 1" NEWLINE
		   "of gfx/fallback as fallback font." NEWLINE
		   "You can also specify a list of font sizes to load, like this:" NEWLINE
		   "loadfont console gfx/conchars,gfx/fallback 8 12 16 24 32" NEWLINE
		   "In many cases, 8 12 16 24 32 should be a good choice." NEWLINE
		   "custom switches:" NEWLINE
		   " scale x : scale all characters by this amount when rendering (doesnt change line height)" NEWLINE
		   " voffset x : offset all chars vertical when rendering, this is multiplied to character height"
		);
		return;
	} // args < 2

	f = FindFont (Cmd_Argv(cmd, 1), /*allocnew?*/ true);
	if (f == NULL) {
		Con_PrintLinef ("font function not found");
		return;
	}

	if (Cmd_Argc(cmd) < 3)
		filelist = "gfx/conchars";
	else
		filelist = Cmd_Argv(cmd, 2);

	memset(f->fallbacks, 0, sizeof(f->fallbacks));
	memset(f->fallback_faces, 0, sizeof(f->fallback_faces));

	// first font is handled "normally"
	c = strchr(filelist, ':');
	cm = strchr(filelist, ',');
	if (c && (!cm || c < cm))
		f->req_face = atoi(c+1);
	else
	{
		f->req_face = 0;
		c = cm;
	}

	if (!c || (c - filelist) > MAX_QPATH_128)
		strlcpy(mainfont, filelist, sizeof(mainfont));
	else
	{
		memcpy(mainfont, filelist, c - filelist);
		mainfont[c - filelist] = 0;
	}

	for(i = 0; i < MAX_FONT_FALLBACKS_3; ++i)
	{
		c = strchr(filelist, ',');
		if (!c)
			break;
		filelist = c + 1;
		if (!*filelist)
			break;
		c = strchr(filelist, ':');
		cm = strchr(filelist, ',');
		if (c && (!cm || c < cm))
			f->fallback_faces[i] = atoi(c+1);
		else
		{
			f->fallback_faces[i] = 0; // f->req_face; could make it stick to the default-font's face index
			c = cm;
		}
		if (!c || (c-filelist) > MAX_QPATH_128)
		{
			strlcpy(f->fallbacks[i], filelist, sizeof(mainfont));
		}
		else
		{
			memcpy(f->fallbacks[i], filelist, c - filelist);
			f->fallbacks[i][c - filelist] = 0;
		}
	} // MAX_FONT_FALLBACKS_3

	// for now: by default load only one size: the default size
	f->req_sizes[0] = 0;
	for(i = 1; i < MAX_FONT_SIZES_16; ++i)
		f->req_sizes[i] = -1;

	scale = 1;
	voffset = 0;
	if (Cmd_Argc(cmd) >= 4)
	{
		for(sizes = 0, i = 3; i < Cmd_Argc(cmd); ++i)
		{
			// special switches
			if (String_Match(Cmd_Argv(cmd, i), "scale")) // font
			{
				i++;
				if (i < Cmd_Argc(cmd))
					scale = atof(Cmd_Argv(cmd, i));
				continue;
			}
			if (String_Match(Cmd_Argv(cmd, i), "voffset"))
			{
				i++;
				if (i < Cmd_Argc(cmd))
					voffset = atof(Cmd_Argv(cmd, i));
				continue;
			}

			if (sizes == -1)
				continue; // no slot for other sizes

			// parse one of sizes
			sz = atof(Cmd_Argv(cmd, i));
			if (sz > 0.001f && sz < 1000.0f) // do not use crap sizes
			{
				// search for duplicated sizes
				int j;
				for (j=0; j<sizes; j++)
					if (f->req_sizes[j] == sz)
						break;
				if (j != sizes)
					continue; // sz already in req_sizes, don't add it again

				if (sizes == MAX_FONT_SIZES_16)
				{
					Con_PrintLinef (CON_WARN "Warning: specified more than %d different font sizes, exceding ones are ignored", MAX_FONT_SIZES_16);
					sizes = -1;
					continue;
				}
				f->req_sizes[sizes] = sz;
				sizes++;
			}
		}
	}

	LoadFontDP (/*override?*/ true, mainfont, f, scale, voffset, DATA_NULL, DATASIZE_0);
}

/*
===============
Draw_Init
===============
*/
static void gl_draw_start(void)
{
	int i;
	char vabuf[1024];
	drawtexturepool = R_AllocTexturePool();

	numcachepics = 0;
	memset(cachepichash, 0, sizeof(cachepichash));

	font_start();

	// load default font textures
	for(i = 0; i < dp_fonts.maxsize_font; i ++) {
		if (dp_fonts.f[i].title[0]) {
			va(vabuf, sizeof(vabuf), "gfx/font_%s", dp_fonts.f[i].title);
			LoadFontDP (/*override?*/ false, vabuf, &dp_fonts.f[i], scale_1_0, /*voffset*/ 0, DATA_NULL, DATASIZE_0);
		}
	}
}

static void gl_draw_shutdown(void)
{
	font_shutdown();

	R_FreeTexturePool(&drawtexturepool);

	numcachepics = 0;
	memset(cachepichash, 0, sizeof(cachepichash));
}

static void gl_draw_newmap(void)
{
	int i;
	font_newmap();

	// mark all of the persistent pics so they are not purged...
	for (i = 0; i < numcachepics; i++)
	{
		cachepic_t *pic = cachepics + i;
		if (!pic->autoload && pic->skinframe)
			R_SkinFrame_MarkUsed(pic->skinframe);
	}
}

void GL_Draw_Init (void)
{
	int i, j;

	Cvar_RegisterVariable(&r_font_postprocess_blur);
	Cvar_RegisterVariable(&r_font_postprocess_outline);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_x);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_y);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_z);
	Cvar_RegisterVariable(&r_font_hinting);
	Cvar_RegisterVariable(&r_font_antialias);
	Cvar_RegisterVariable(&r_textshadow);
	Cvar_RegisterVariable(&r_textbrightness);
	Cvar_RegisterVariable(&r_textcontrast);
	Cvar_RegisterVariable(&r_nearest_2d);
	Cvar_RegisterVariable(&r_nearest_conchars);

	Cvar_RegisterCallback(&r_nearest_conchars, R_Nearest_Conchars_Callback);

	// allocate fonts storage
	fonts_mempool = Mem_AllocPool ("FONTS", 0, NULL);
	dp_fonts.maxsize_font = MAX_FONTS_AT_START_16;
	dp_fonts.f = (dp_font_t *)Mem_Alloc(fonts_mempool, sizeof(dp_font_t) * dp_fonts.maxsize_font);
	memset (dp_fonts.f, 0, sizeof(dp_font_t) * dp_fonts.maxsize_font);

	// assign starting font names
	strlcpy(FONT_DEFAULT->title, "default", sizeof(FONT_DEFAULT->title));
	strlcpy(FONT_DEFAULT->texpath, "gfx/conchars", sizeof(FONT_DEFAULT->texpath));
	strlcpy(FONT_CONSOLE->title, "console", sizeof(FONT_CONSOLE->title));
	strlcpy(FONT_SBAR->title, "sbar", sizeof(FONT_SBAR->title));
	strlcpy(FONT_NOTIFY->title, "notify", sizeof(FONT_NOTIFY->title));
	strlcpy(FONT_CHAT->title, "chat", sizeof(FONT_CHAT->title));
	strlcpy(FONT_CENTERPRINT->title, "centerprint", sizeof(FONT_CENTERPRINT->title));
	strlcpy(FONT_INFOBAR->title, "infobar", sizeof(FONT_INFOBAR->title));
	strlcpy(FONT_MENU->title, "menu", sizeof(FONT_MENU->title));

	for(i = 0, j = 0; i < MAX_USERFONTS; ++i)
		if (!FONT_USER(i)->title[0])
			dpsnprintf(FONT_USER(i)->title, sizeof(FONT_USER(i)->title), "user%d", j++);

	Cmd_AddCommand (CF_CLIENT, "loadfont", LoadFont_f, "loadfont function tganame loads a font; example: loadfont console gfx/veramono; loadfont without arguments lists the available functions");
	R_RegisterModule("GL_Draw", gl_draw_start, gl_draw_shutdown, gl_draw_newmap, NULL, NULL);
}

void DrawQ_Start(void)
{
	r_refdef.draw2dstage = 1;
	R_ResetViewRendering2D_Common(0, NULL, NULL, 0, 0, vid.width, vid.height, vid_conwidth.integer, vid_conheight.integer);
}

qbool r_draw2d_force = false;

#if 123

void DrawQ_ProcessDrawFlag(int flags, qbool alpha)
{
	if(flags == DRAWFLAG_ADDITIVE)
	{
		GL_DepthMask(false);
		GL_BlendFunc(alpha ? GL_SRC_ALPHA : GL_ONE, GL_ONE);
	}
	else if(flags == DRAWFLAG_MODULATE)
	{
		GL_DepthMask(false);
		GL_BlendFunc(GL_DST_COLOR, GL_ZERO);
	}
	else if(flags == DRAWFLAG_2XMODULATE)
	{
		GL_DepthMask(false);
		GL_BlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
	}
	else if(flags == DRAWFLAG_SCREEN)
	{
		GL_DepthMask(false);
		GL_BlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}
	else if(alpha)
	{
		GL_DepthMask(false);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_DepthMask(true);
		GL_BlendFunc(GL_ONE, GL_ZERO);
	}
}

void DrawQ_Mesh (drawqueuemesh_t *mesh, int flags, qbool hasalpha)
{
	DrawQ_Start ();

	if(!r_draw2d.integer && !r_draw2d_force)
		return;
	DrawQ_ProcessDrawFlag(flags, hasalpha);

	R_SetupShader_Generic(mesh->texture, /*gamma trippy texalpha*/ (flags & DRAWFLAGS_BLEND) ? false : true, true, false);

	R_Mesh_PrepareVertices_Generic_Arrays(mesh->num_vertices, mesh->data_vertex3f, mesh->data_color4f, mesh->data_texcoord2f);
	R_Mesh_Draw(0, mesh->num_vertices, 0, mesh->num_triangles, mesh->data_element3i, NULL, 0, mesh->data_element3s, NULL, 0);
}

#endif // 123

void DrawQ_Pic(float x, float y, cachepic_t *pic, float width, float height, float red, float green, float blue, float alpha, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture
	// looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;

	texture_t *mm = Mod_Mesh_GetTexture(mod, pic->name, flags, pic->texflags,
		MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX |
		MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW);



	surf = Mod_Mesh_AddSurface(mod, mm, true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x        , y         , 0, 0, 0, -1, 0, 0, 0, 0, red, green, blue, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y         , 0, 0, 0, -1, 1, 0, 0, 0, red, green, blue, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y + height, 0, 0, 0, -1, 1, 1, 0, 0, red, green, blue, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x        , y + height, 0, 0, 0, -1, 0, 1, 0, 0, red, green, blue, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_RotPic(float x, float y, cachepic_t *pic, float width, float height, float org_x, float org_y, float angle, float red, float green, float blue, float alpha, int flags)
{
	float af = DEG2RAD(-angle); // forward
	float ar = DEG2RAD(-angle + 90); // right
	float sinaf = sin(af);
	float cosaf = cos(af);
	float sinar = sin(ar);
	float cosar = cos(ar);
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, pic->name, flags, pic->texflags, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x - cosaf *          org_x  - cosar *           org_y , y - sinaf *          org_x  - sinar *           org_y , 0, 0, 0, -1, 0, 0, 0, 0, red, green, blue, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + cosaf * (width - org_x) - cosar *           org_y , y + sinaf * (width - org_x) - sinar *           org_y , 0, 0, 0, -1, 1, 0, 0, 0, red, green, blue, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + cosaf * (width - org_x) + cosar * (height - org_y), y + sinaf * (width - org_x) + sinar * (height - org_y), 0, 0, 0, -1, 1, 1, 0, 0, red, green, blue, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x - cosaf *          org_x  + cosar * (height - org_y), y - sinaf *          org_x  + sinar * (height - org_y), 0, 0, 0, -1, 0, 1, 0, 0, red, green, blue, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_Fill(float x, float y, float width, float height, float red, float green, float blue, float alpha, int flags)
{
	DrawQ_Pic(x, y, Draw_CachePic("white"), width, height, red, green, blue, alpha, flags);
}

void DrawQ_Fill_Box (float x, float y, float width, float height, float red, float green, float blue, float alpha, int flags, int thickness)
{
	// TOP
	DrawQ_Pic(x, y, Draw_CachePic("white"), width - thickness, thickness, red, green, blue, alpha, flags);

	// RIGHT
	DrawQ_Pic(x + width - thickness, y, Draw_CachePic("white"), thickness, height - thickness, red, green, blue, alpha, flags);

	// BOTTOM
	DrawQ_Pic(x + thickness, y + height - thickness, Draw_CachePic("white"), width - thickness, thickness, red, green, blue, alpha, flags);

	// LEFT
	DrawQ_Pic(x, y + thickness, Draw_CachePic("white"), thickness, height - thickness, red, green, blue, alpha, flags);

}


/// color tag printing
static const vec4_t string_colors[] =
{
	// Baker r1004: Use a Quake identical bronze color instead of yellow
	// Quake3 colors
	// LadyHavoc: why on earth is cyan before magenta in Quake3?
	// LadyHavoc: note: Doom3 uses white for [0] and [7]
	/*0*/		{0.0, 0.0, 0.0, 1.0}, // black 0
	/*1*/		{1.0, 0.0, 0.0, 1.0}, // red 1											//{0.75, 0.50, 0.50, 1.0}, // Baker 1007 soft purple

	/*2*/		{0.0, 1.0, 0.0, 1.0}, // green 2

	/*3*/		{1.03, 0.5, 0.36, 1.0}, // Baker 1007 bronzey Baker 1007 3
				// US: 143 69 50
				// MV: 143 67 51
	/*4*/		{0.0, 0.0, 1.0, 1.0}, // 4 blue
	/*5*/		{0.0, 1.0, 1.0, 1.0}, // 5 cyan
	/*6*/		{1.0, 0.0, 1.0, 1.0}, // 6 magenta
	/*7*/		{1.0, 1.0, 1.0, 1.0}, // 7 white
	// [515]'s BX_COLOREDTEXT extension
	{1.0, 1.0, 1.0, 0.5}, // 8 half transparent
	{0.5, 0.5, 0.5, 1.0}  // 9 half brightness
	// Black's color table
	//{1.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 0.0, 1.0},
	//{0.0, 1.0, 0.0, 1.0},
	//{0.0, 0.0, 1.0, 1.0},
	//{1.0, 1.0, 0.0, 1.0},
	//{0.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 1.0, 1.0},
	//{0.1, 0.1, 0.1, 1.0}
};

#define STRING_COLORS_COUNT	(sizeof(string_colors) / sizeof(vec4_t))

static void DrawQ_GetTextColor(float color[4], int colorindex, float r, float g, float b, float a, qbool shadow)
{
	float C = r_textcontrast.value;
	float B = r_textbrightness.value;
	if (colorindex & 0x10000) // that bit means RGB color
	{
		color[0] = ((colorindex >> 12) & 0xf) / 15.0;
		color[1] = ((colorindex >> 8) & 0xf) / 15.0;
		color[2] = ((colorindex >> 4) & 0xf) / 15.0;
		color[3] = (colorindex & 0xf) / 15.0;
	}
	else
		Vector4Copy(string_colors[colorindex], color);
	Vector4Set(color, color[0] * r * C + B, color[1] * g * C + B, color[2] * b * C + B, color[3] * a);
	if (shadow)
	{
		float shadowalpha = (color[0]+color[1]+color[2]) * 0.8;
		Vector4Set(color, 0, 0, 0, color[3] * bound(0, shadowalpha, 1));
	}
}

// returns a colorindex (format 0x1RGBA) if str is a valid RGB string
// returns 0 otherwise
static int RGBstring_to_colorindex(const char *str)
{
	Uchar ch;
	int ind = 0x0001 << 4;
	do {
		if (*str <= '9' && *str >= '0')
			ind |= (*str - '0');
		else
		{
			ch = tolower(*str);
			if (ch >= 'a' && ch <= 'f')
				ind |= (ch - 87);
			else
				return 0;
		}
		++str;
		ind <<= 4;
	} while(!(ind & 0x10000));
	return ind | 0xf; // add costant alpha value
}

// NOTE: this function always draws exactly one character if maxwidth <= 0
// Baker: maxlen works how with unicode?  with colorcodes?
float DrawQ_TextWidth_UntilWidth_TrackColors_Scale(const char *text, size_t *maxlen, float w, float h, float sw, float sh, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt, float maxwidthf)
{
	// Baker: renamed to text_start2
	const char *text_start2 = text;
	int colorindex;
	size_t i;
	float x_out = 0;
	Uchar ch, mapch, nextch;
	Uchar prevch = 0; // used for kerning
	float kx;
	float baker_old_outf;
	int map_index = 0;
	size_t bytes_left;
	ft2_font_map_t *fontmap = NULL;
	ft2_font_map_t *map = NULL;
	//ft2_font_map_t *prevmap = NULL;
	ft2_font_t *ft2 = fnt->ft2;
	// float ftbase_x;
	qbool snap = true;
	qbool least_one = false;
	float dw; // display w
	//float dh; // display h
	const float *width_of;

	if (!h) h = w;
	if (!h) {
		w = h = 1;
		snap = false;
	}
	// do this in the end
	w *= fnt->settings.scale;
	h *= fnt->settings.scale;

	// find the most fitting size:
	if (ft2 != NULL)
	{
		if (snap) // Baker: This appears the norm
			map_index = Font_IndexForSize(ft2, h, &w, &h);
		else
			map_index = Font_IndexForSize(ft2, h, NULL, NULL);
		fontmap = Font_MapForIndex(ft2, map_index);
	}

	dw = w * sw;
	//dh = h * sh;

	if (*maxlen < 1)
		*maxlen = 1<<30;

	if (!outcolor || *outcolor == -1)
		colorindex = STRING_COLOR_DEFAULT;
	else
		colorindex = *outcolor;

	// maxwidth /= fnt->scale; // w and h are multiplied by it already
	// ftbase_x = snap_to_pixel_x(0);

	if (maxwidthf <= 0) // 0
	{
		least_one = true;
		maxwidthf = -maxwidthf; // 1
	}

	//if (snap)
	//	x_out = snap_to_pixel_x(x_out, 0.4); // haha, it's 0 anyway

	if (fontmap)
		width_of = fontmap->width_of;
	else
		width_of = fnt->width_of;

	i = 0;
	while (((bytes_left = *maxlen - (text - text_start2)) > 0) && *text)
	{
		size_t i0 = i;
		nextch = ch = u8_getnchar(text, &text, bytes_left);
		i = text - text_start2;
		if (!ch)
			break;
		if (ch == ' ' && !fontmap) // Baker: Having a fontmap is the norm
		{
			if (!least_one || i0) // never skip the first character
			if (x_out + width_of[(int) ' '] * dw > maxwidthf) // -2
			{
				i = i0;
				break; // oops, can't draw this
			}
			x_out += width_of[(int) ' '] * dw;
			continue;
		}

		// Baker: Process color code if specified
		if (ch == STRING_COLOR_TAG && !ignorecolorcodes && i < *maxlen)
		{
			ch = *text; // colors are ascii, so no u8_ needed
			if (ch <= '9' && ch >= '0') // ^[0-9] found
			{
				colorindex = ch - '0';
				++text;
				++i;
				continue;
			}
			else if (ch == STRING_COLOR_RGB_TAG_CHAR && i + 3 < *maxlen ) // ^x_out found
			{
				const char *text_p = &text[1];
				int tempcolorindex = RGBstring_to_colorindex(text_p);
				if (tempcolorindex)
				{
					colorindex = tempcolorindex;
					i+=4;
					text += 4;
					continue;
				}
			}
			else if (ch == STRING_COLOR_TAG) // ^^ found
			{
				i++;
				text++;
			}
			i--;
		}
		ch = nextch;

		if (!fontmap || (ch <= 0xFF && fontmap->glyphs[ch].image) || (ch >= 0xE000 && ch <= 0xE0FF)) {
			// Baker: I bet this rarely hits with ascii.
			if (ch > 0xE000)
				ch -= 0xE000;
			if (ch > 0xFF)
				continue;
			if (fontmap)
				map = ft2_oldstyle_map;
			prevch = 0;
			if (!least_one || i0) // never skip the first character
			if (x_out + width_of[ch] * dw > maxwidthf) { // -3
				i = i0;
				break; // oops, can't draw this
			}
			x_out += width_of[ch] * dw;
		} else {
			// Baker: Seems to be the norm for ttf fonts
			if (!map || map == ft2_oldstyle_map || ch < map->start || ch >= map->start + FONT_CHARS_PER_MAP)
			{
				// Baker: I bet unicode maybe even normally comes here
				map = FontMap_FindForChar(fontmap, ch);
				if (!map)
				{
					if (!Font_LoadMapForIndex(ft2, map_index, ch, &map, &ft2->ft_baker_ascend, &ft2->ft_baker_descend, &ft2->ft_baker_height))
						break;
					if (!map)
						break;
				}
			}
			mapch = ch - map->start;
			baker_old_outf = x_out;
			if (prevch && Font_GetKerningForMap(ft2, map_index, w, h, prevch, ch, &kx, NULL))
				x_out += kx * dw; // Baker: For roboto this is not hitting
			x_out += map->glyphs[mapch].advance_x * dw;
			if (x_out >= maxwidthf) {
				x_out = baker_old_outf;
				i --;
				break;
			}
			//prevmap = map;
			prevch = ch;
		}
	}

	*maxlen = i;

	if (outcolor)
		*outcolor = colorindex;

	return x_out;
}

// Baker: Modified .. maxwidth supports rounding type
// Some types of mouse selection of text needs certain rounding
// Like selecting between 2 character, needs the closest match
// but extending a selection needs rounding towards most text selected.
float DrawQ_TextWidth_RoundType(const char *text, size_t *maxlen, float w, float h, float sw, float sh, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt, float maxwidthf, int roundtype)
{
	// Baker: renamed to text_start2
	const char *text_start2 = text;
	int colorindex;
	size_t i;
	float x_out = 0;
	Uchar ch, mapch, nextch;
	Uchar prevch = 0; // used for kerning
	float kx;
	float baker_old_outf;
	int map_index = 0;
	size_t bytes_left;
	ft2_font_map_t *fontmap = NULL;
	ft2_font_map_t *map = NULL;
	//ft2_font_map_t *prevmap = NULL;
	ft2_font_t *ft2 = fnt->ft2;
	// float ftbase_x;
	qbool snap = true;
	qbool least_one = false;
	float dw; // display w
	//float dh; // display h
	const float *width_of;

	if (!h) h = w;
	if (!h) {
		w = h = 1;
		snap = false;
	}
	// do this in the end
	w *= fnt->settings.scale;
	h *= fnt->settings.scale;

	// find the most fitting size:
	if (ft2 != NULL)
	{
		if (snap) // Baker: This appears the norm
			map_index = Font_IndexForSize(ft2, h, &w, &h);
		else
			map_index = Font_IndexForSize(ft2, h, NULL, NULL);
		fontmap = Font_MapForIndex(ft2, map_index);
	}

	dw = w * sw;
	//dh = h * sh;

	if (*maxlen < 1)
		*maxlen = 1<<30;

	if (!outcolor || *outcolor == -1)
		colorindex = STRING_COLOR_DEFAULT;
	else
		colorindex = *outcolor;

	// maxwidth /= fnt->scale; // w and h are multiplied by it already
	// ftbase_x = snap_to_pixel_x(0);

	if (maxwidthf <= 0) // 0
	{
		least_one = true;
		maxwidthf = -maxwidthf; // 1
	}

	//if (snap)
	//	x_out = snap_to_pixel_x(x_out, 0.4); // haha, it's 0 anyway

	if (fontmap)
		width_of = fontmap->width_of;
	else
		width_of = fnt->width_of;

	i = 0;
	while (((bytes_left = *maxlen - (text - text_start2)) > 0) && *text)
	{
		size_t i0 = i;
		nextch = ch = u8_getnchar(text, &text, bytes_left);
		i = text - text_start2;
		if (!ch)
			break;
		if (ch == ' ' && !fontmap) // Baker: Having a fontmap is the norm
		{
			if (!least_one || i0) // never skip the first character
			if (x_out + width_of[(int) ' '] * dw > maxwidthf) // -2
			{
				i = i0;
				break; // oops, can't draw this
			}
			x_out += width_of[(int) ' '] * dw;
			continue;
		}

		// Baker: Process color code if specified
		if (ch == STRING_COLOR_TAG && !ignorecolorcodes && i < *maxlen)
		{
			ch = *text; // colors are ascii, so no u8_ needed
			if (ch <= '9' && ch >= '0') // ^[0-9] found
			{
				colorindex = ch - '0';
				++text;
				++i;
				continue;
			}
			else if (ch == STRING_COLOR_RGB_TAG_CHAR && i + 3 < *maxlen ) // ^x_out found
			{
				const char *text_p = &text[1];
				int tempcolorindex = RGBstring_to_colorindex(text_p);
				if (tempcolorindex)
				{
					colorindex = tempcolorindex;
					i+=4;
					text += 4;
					continue;
				}
			}
			else if (ch == STRING_COLOR_TAG) // ^^ found
			{
				i++;
				text++;
			}
			i--;
		}
		ch = nextch;

		if (!fontmap || (ch <= 0xFF && fontmap->glyphs[ch].image) || (ch >= 0xE000 && ch <= 0xE0FF)) {
			// Baker: I bet this rarely hits with ascii.
			if (ch > 0xE000)
				ch -= 0xE000;
			if (ch > 0xFF)
				continue;
			if (fontmap)
				map = ft2_oldstyle_map;
			prevch = 0;
			if (!least_one || i0) // never skip the first character
			if (x_out + width_of[ch] * dw > maxwidthf) { // -3
				i = i0;
				break; // oops, can't draw this
			}
			x_out += width_of[ch] * dw;
		} else {
			// Baker: Seems to be the norm for ttf fonts
			if (!map || map == ft2_oldstyle_map || ch < map->start || ch >= map->start + FONT_CHARS_PER_MAP)
			{
				// Baker: I bet unicode maybe even normally comes here
				map = FontMap_FindForChar(fontmap, ch);
				if (!map)
				{
					if (!Font_LoadMapForIndex(ft2, map_index, ch, &map, &ft2->ft_baker_ascend, &ft2->ft_baker_descend, &ft2->ft_baker_height))
						break;
					if (!map)
						break;
				}
			}
			mapch = ch - map->start;
			baker_old_outf = x_out;
			if (prevch && Font_GetKerningForMap(ft2, map_index, w, h, prevch, ch, &kx, NULL))
				x_out += kx * dw; // Baker: For roboto this is not hitting
			
			float ch_widthf = map->glyphs[mapch].advance_x * dw;
			x_out += ch_widthf;
			// Baker: Let's say we hit middle of first character?
			if (x_out >= maxwidthf) {
				float deltaf = x_out - baker_old_outf;
				float distance_lo = abs(maxwidthf - baker_old_outf);
				float distance_hi = abs(maxwidthf - x_out);
				float plus_half = baker_old_outf + 0.5 * deltaf;

				if (distance_lo <= distance_hi)
					i = i - 1; // Round to previous
				else 
					i = i; // Round to next
				//if (plus_half >= maxwidthf) {
				//	i = i; // Round to next
				//} else {
				//	i = i - 1; // Round to previous
				//}
				x_out = baker_old_outf;
				break;
			}
			//prevmap = map;
			prevch = ch;
		}
	}

	*maxlen = i;

	if (outcolor)
		*outcolor = colorindex;

	return x_out;
} // End DrawQ_TextWidth_RoundType

float DrawQ_Color[4];
// Baker: What is the return value here?  Looks like x after
float DrawQ_String_Scale(float startx, float starty, const char *text, size_t maxlen, float w, float h, float sw, float sh, float basered, float basegreen, float baseblue, float basealpha, int flags, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	int shadow, colorindex = STRING_COLOR_DEFAULT;
	size_t i;
	float x = startx, y, s, t, u, v, thisw;
	Uchar ch, mapch, nextch;
	Uchar prevch = 0; // used for kerning
	int map_index = 0;
	//ft2_font_map_t *prevmap = NULL; // the previous map
	ft2_font_map_t *map = NULL;     // the currently used map
	ft2_font_map_t *fontmaphere = NULL; // the font map for the size
	float ftbase_y;
	const char *text_start = text;
	float kx, ky;
	ft2_font_t *ft2 = fnt->ft2;
	qbool snap = true;
	float pix_x, pix_y;
	size_t bytes_left;
	float dw, dh;
	const float *width_of;
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf = NULL;
	int e0, e1, e2, e3;
	int tw, th;
	tw = Draw_GetPicWidth(fnt->pic_font);
	th = Draw_GetPicHeight(fnt->pic_font);

	if (!h) h = w;
	if (!h) {
		h = w = 1;
		snap = false;
	}

	starty -= (fnt->settings.scale - 1) * h * 0.5 - fnt->settings.voffset*h; // center & offset
	w *= fnt->settings.scale;
	h *= fnt->settings.scale;

	if (ft2 != NULL)
	{
		if (snap)
			map_index = Font_IndexForSize(ft2, h, &w, &h);
		else
			map_index = Font_IndexForSize(ft2, h, NULL, NULL);
		fontmaphere = Font_MapForIndex(ft2, map_index);
	}

	dw = w * sw;
	dh = h * sh;

	// draw the font at its baseline when using freetype
	//ftbase_x = 0;
	ftbase_y = dh * (4.5/6.0);

	if (maxlen < 1)
		maxlen = 1<<30;

	if (!r_draw2d.integer && !r_draw2d_force)
		return startx + DrawQ_TextWidth_UntilWidth_TrackColors_Scale(text, &maxlen, w, h, sw, sh, NULL, ignorecolorcodes, fnt, 1000000000);

	//ftbase_x = snap_to_pixel_x(ftbase_x);
	if (snap)
	{
		startx = snap_to_pixel_x(startx, 0.4);
		starty = snap_to_pixel_y(starty, 0.4);
		ftbase_y = snap_to_pixel_y(ftbase_y, 0.3);
	}

	pix_x = vid.width / vid_conwidth.value;
	pix_y = vid.height / vid_conheight.value;

	if (fontmaphere)
		width_of = fontmaphere->width_of;
	else
		width_of = fnt->width_of;

	for (shadow = r_textshadow.value != 0 && basealpha > 0;shadow >= 0;shadow--) {
		prevch = 0;
		text = text_start;

		if (!outcolor || *outcolor == -1)
			colorindex = STRING_COLOR_DEFAULT;
		else
			colorindex = *outcolor;

		DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);

		x = startx;
		y = starty;
		/*
		if (shadow)
		{
			x += r_textshadow.value * vid.width / vid_conwidth.value;
			y += r_textshadow.value * vid.height / vid_conheight.value;
		}
		*/
		while (((bytes_left = maxlen - (text - text_start)) > 0) && *text)
		{

baker_font:
			// Baker text variable advances as each prints
			// Bytes left is a massive number perhaps to prevent
			// a runaway loop from occurring somehow
			// maxlen is also a massive number
			// i is the delta in bytes from current text "text"
			//                                 to the start of text
			//  a positive number that is assigned ONE after the first
			// character is processed assuming it is not an extended character
			// or colored
			// for text highlight we could check if text >= stopping point
			// and set a variable

#if 0 // FFA
			// Baker: u8_getnchar advances text
			const char *b4 = text;
			if (g_p_selbeyond && text >= g_p_selbeyond) {
				g_p_selbeyond_x = x;
				g_p_selbeyond = NULL;
			}
#endif

			nextch = ch = u8_getnchar(text, &text, bytes_left);

			i = text - text_start;
			if (!ch) {
				break;
			}
			if (ch == ' ' && !fontmaphere)
			{
				x += width_of[(int) ' '] * dw;
				continue;
			}
			if (ch == STRING_COLOR_TAG && !ignorecolorcodes && i < maxlen)
			{
				ch = *text; // colors are ascii, so no u8_ needed
				if (ch <= '9' && ch >= '0') // ^[0-9] found
				{
					colorindex = ch - '0';
					DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);
					++text;
					++i;
					continue;
				}
				else if (ch == STRING_COLOR_RGB_TAG_CHAR && i+3 < maxlen ) // ^x found
				{
					const char *text_p = &text[1];
					int tempcolorindex = RGBstring_to_colorindex(text_p);
					if (tempcolorindex)
					{
						colorindex = tempcolorindex;
						DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);
						i+=4;
						text+=4;
						continue;
					}
				}
				else if (ch == STRING_COLOR_TAG)
				{
					i++;
					text++;
				}
				i--;
			} // COLOR TAG "^"
			// get the backup
			ch = nextch;
			// using a value of -1 for the oldstyle map because NULL means uninitialized...
			// this way we don't need to rebind fnt->tex for every old-style character
			// E000..E0FF: emulate old-font characters (to still have smileys and such available)
			if (shadow)
			{
				x += 1.0/pix_x * r_textshadow.value;
				y += 1.0/pix_y * r_textshadow.value;
			}
			if (!fontmaphere || (ch <= 0xFF && fontmaphere->glyphs[ch].image) || (ch >= 0xE000 && ch <= 0xE0FF))
			{
				if (ch >= 0xE000)
					ch -= 0xE000;
				if (ch > 0xFF)
					goto out;
				if (fontmaphere)
					map = ft2_oldstyle_map;
				prevch = 0;
				//num = (unsigned char) text[i];
				//thisw = fnt->width_of[num];
				thisw = fnt->width_of[ch];
				// FIXME make these smaller to just include the occupied part of the character for slightly faster rendering
				if (r_nearest_conchars.integer)
				{
					s = (ch & 15)*0.0625f;
					t = (ch >> 4)*0.0625f;
					u = 0.0625f * thisw;
					v = 0.0625f;
				}
				else
				{
					s = (ch & 15)*0.0625f + (0.5f / tw);
					t = (ch >> 4)*0.0625f + (0.5f / th);
					u = 0.0625f * thisw - (1.0f / tw);
					v = 0.0625f - (1.0f / th);
				}
				surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, fnt->pic_font->name, flags, (r_nearest_conchars.value ? TEXF_FORCENEAREST : 0) | TEXF_ALPHA | TEXF_CLAMP, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
				e0 = Mod_Mesh_IndexForVertex(mod, surf, x         , y   , 10, 0, 0, -1, s  , t  , 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e1 = Mod_Mesh_IndexForVertex(mod, surf, x+dw*thisw, y   , 10, 0, 0, -1, s+u, t  , 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e2 = Mod_Mesh_IndexForVertex(mod, surf, x+dw*thisw, y+dh, 10, 0, 0, -1, s+u, t+v, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e3 = Mod_Mesh_IndexForVertex(mod, surf, x         , y+dh, 10, 0, 0, -1, s  , t+v, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
				Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
				x += width_of[ch] * dw;
			} else {
				if (!map || map == ft2_oldstyle_map || ch < map->start || ch >= map->start + FONT_CHARS_PER_MAP)
				{
					// find the new map
					map = FontMap_FindForChar(fontmaphere, ch);
					if (!map)
					{
						if (!Font_LoadMapForIndex(ft2, map_index, ch, &map, /*pascender descender height*/ NULL, NULL, NULL))
						{
							shadow = -1;
							break;
						}
						if (!map)
						{
							// this shouldn't happen
							shadow = -1;
							break;
						}
					}
				}

				mapch = ch - map->start;
				thisw = map->glyphs[mapch].advance_x;

				//x += ftbase_x;
				y += ftbase_y;
				if (prevch && Font_GetKerningForMap(ft2, map_index, w, h, prevch, ch, &kx, &ky)) {
					x += kx * dw;
					y += ky * dh;
				}
				else
					kx = ky = 0;
				surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, map->pic->name, flags, TEXF_ALPHA | TEXF_CLAMP, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
				e0 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmin, y + dh * map->glyphs[mapch].vymin, 10, 0, 0, -1, map->glyphs[mapch].txmin, map->glyphs[mapch].tymin, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e1 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmax, y + dh * map->glyphs[mapch].vymin, 10, 0, 0, -1, map->glyphs[mapch].txmax, map->glyphs[mapch].tymin, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e2 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmax, y + dh * map->glyphs[mapch].vymax, 10, 0, 0, -1, map->glyphs[mapch].txmax, map->glyphs[mapch].tymax, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e3 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmin, y + dh * map->glyphs[mapch].vymax, 10, 0, 0, -1, map->glyphs[mapch].txmin, map->glyphs[mapch].tymax, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
				Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
				//x -= ftbase_x;
				y -= ftbase_y;

				x += thisw * dw;

				//prevmap = map;
				prevch = ch;
			}
out:
			if (shadow)
			{
				x -= 1.0/pix_x * r_textshadow.value;
				y -= 1.0/pix_y * r_textshadow.value;
			}
		} // while
#if 0 // FFA
		if (g_p_selbeyond) {
			g_p_selbeyond_x = x;
			g_p_selbeyond = NULL;
		}
#endif
	} // for shadow

	if (outcolor)
		*outcolor = colorindex;

	// note: this relies on the proper text (not shadow) being drawn last
	return x;
}

float DrawQ_String(float startx, float starty, const char *text, size_t maxlen, float w, float h, float basered, float basegreen, float baseblue, float basealpha, int flags, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	return DrawQ_String_Scale(startx, starty, text, maxlen, w, h, 1, 1, basered, basegreen, baseblue, basealpha, flags, outcolor, ignorecolorcodes, fnt);
}

float DrawQ_TextWidth_UntilWidth_TrackColors(const char *text, size_t *maxlen, float w, float h, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt, float maxwidth)
{
	return DrawQ_TextWidth_UntilWidth_TrackColors_Scale(text, maxlen, w, h, 1, 1, outcolor, ignorecolorcodes, fnt, maxwidth);
}

float DrawQ_TextWidth(const char *text, size_t maxlen, float w, float h, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	return DrawQ_TextWidth_UntilWidth(text, &maxlen, w, h, ignorecolorcodes, fnt, 1000000000);
}

float DrawQ_TextWidth_UntilWidth(const char *text, size_t *maxlen, float w, float h, qbool ignorecolorcodes, const dp_font_t *fnt, float maxWidth)
{
	return DrawQ_TextWidth_UntilWidth_TrackColors(text, maxlen, w, h, NULL, ignorecolorcodes, fnt, maxWidth);
}

#if 0
// not used
// no ^xrgb management
static int DrawQ_BuildColoredText(char *output2c, size_t maxoutchars, const char *text, int maxreadchars, qbool ignorecolorcodes, int *outcolor)
{
	int color, numchars = 0;
	char *outputend2c = output2c + maxoutchars - 2;
	if (!outcolor || *outcolor == -1)
		color = STRING_COLOR_DEFAULT;
	else
		color = *outcolor;
	if (!maxreadchars)
		maxreadchars = 1<<30;
	textend = text + maxreadchars;
	while (text != textend && *text)
	{
		if (*text == STRING_COLOR_TAG && !ignorecolorcodes && text + 1 != textend)
		{
			if (text[1] == STRING_COLOR_TAG)
				text++;
			else if (text[1] >= '0' && text[1] <= '9')
			{
				color = text[1] - '0';
				text += 2;
				continue;
			}
		}
		if (output2c >= outputend2c)
			break;
		*output2c++ = *text++;
		*output2c++ = color;
		numchars++;
	}
	output2c[0] = output2c[1] = 0;
	if (outcolor)
		*outcolor = color;
	return numchars;
}
#endif

void DrawQ_SuperPic(float x, float y, cachepic_t *pic, float width, float height, float s1, float t1, float r1, float g1, float b1, float a1, float s2, float t2, float r2, float g2, float b2, float a2, float s3, float t3, float r3, float g3, float b3, float a3, float s4, float t4, float r4, float g4, float b4, float a4, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;
	// Baker: pic->name is dpv?

	// Baker CPIF -- if there is a supplied pic
	// I don't think we are supposed to use
	texture_t *tex;

	if (pic) {
		//WARP_X_ (R_MakeTextureDynamic)
		//tex = pic->skinframe->base;
		//R_GetCurrentTexture (

		// Ok .. is this our own texture?
		tex = Mod_Mesh_GetTexture (
			mod,
			pic->name,
			flags,
			pic->texflags,
			MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW
		);
	} else {
		tex = Mod_Mesh_GetTexture (
			mod,
			pic->name,
			flags,
			pic->texflags,
			MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW
		);
	}
	surf = Mod_Mesh_AddSurface(mod, tex, /*batchwithprevioussurface ?*/ true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x        , y         , 0, 0, 0, -1, s1, t1, 0, 0, r1, g1, b1, a1);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y         , 0, 0, 0, -1, s2, t2, 0, 0, r2, g2, b2, a2);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y + height, 0, 0, 0, -1, s4, t4, 0, 0, r4, g4, b4, a4);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x        , y + height, 0, 0, 0, -1, s3, t3, 0, 0, r3, g3, b3, a3);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

// 1. Try to use the provided one
// 2. If that fails, try a builtin one?
WARP_X_ (CL_UpdateScreen_SCR_DrawScreen  R_Shadow_MakeTextures )
WARP_X_ (skinframe_t texture_t -> skinframe_t CL_DrawVideo_SCR_DrawScreen)

// Baker: The model here is CL_Mesh_UI, which is cl_meshentitymodels[MESH_UI_1]

//extern int did_create;

// Baker: videotex is used to transport video->width/video->height
void DrawQ_SuperPic_Video (void *videotex, float x, float y,
					 cachepic_t *pic, float width, float height,
					 float s1, float t1, float r1, float g1, float b1, float a1, float s2, float t2, float r2, float g2, float b2, float a2, float s3, float t3, float r3, float g3, float b3, float a3, float s4, float t4, float r4, float g4, float b4, float a4, int flags)
{
	//texture_t *videotex = (texture_t *)_videotex;
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here,
	// Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;

	texture_t *tex;
	rtexture_t *rt = Draw_GetPicTexture(pic /*video->cpif*/);

	WARP_X_ (CL_Video_Frame)

	if (1) {
		
		// Baker: videotex gets created here
		tex = Mod_Mesh_GetTexture (
			mod,
			pic->name,
			flags,
			pic->texflags,
			MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW
		);
	}

	if (rt->dirty_ic) {
		skinframe_t *skinframe = tex->materialshaderpass->skinframes[0];
		int textureflags= skinframe->textureflags;
		R_SkinFrame_PurgeSkinFrame(skinframe);
		textureflags &= ~TEXF_FORCE_RELOAD;

		skinframe->stain = NULL;
		skinframe->merged = NULL;
		skinframe->base = NULL;
		skinframe->pants = NULL;
		skinframe->shirt = NULL;
		skinframe->nmap = NULL;
		skinframe->gloss = NULL;
		skinframe->glow = NULL;
		skinframe->fog = NULL;
		skinframe->reflect = NULL;
		skinframe->hasalpha = false;

		extern rtexturepool_t *r_main_texturepool;
		extern byte *is_q3_shader_video_tex_vimagedata;
		
		// Baker: Similar to our Q1SKY fix upload fix
		skinframe->base = skinframe->merged =
			R_LoadTexture2D(
				r_main_texturepool,
				skinframe->basename,
				((texture_t *)(videotex))->width, ((texture_t *)videotex)->height, is_q3_shader_video_tex_vimagedata,
				TEXTYPE_BGRA,
				textureflags,
				-1,
				NULL
		);
		rt->dirty_ic = false;
	}

	surf = Mod_Mesh_AddSurface(mod, tex, /*batchwithprevioussurface ?*/ true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x        , y         , 0, 0, 0, -1, s1, t1, 0, 0, r1, g1, b1, a1);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y         , 0, 0, 0, -1, s2, t2, 0, 0, r2, g2, b2, a2);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y + height, 0, 0, 0, -1, s4, t4, 0, 0, r4, g4, b4, a4);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x        , y + height, 0, 0, 0, -1, s3, t3, 0, 0, r3, g3, b3, a3);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_Line (float width, float x1, float y1, float x2, float y2, float r, float g, float b, float alpha, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	float offsetx, offsety;
	// width is measured in real pixels
	if (fabs(x2 - x1) > fabs(y2 - y1))
	{
		offsetx = 0;
		offsety = 0.5f * width * vid_conheight.value / vid.height;
	}
	else
	{
		offsetx = 0.5f * width * vid_conwidth.value / vid.width;
		offsety = 0;
	}
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, "white", 0, 0, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x1 - offsetx, y1 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x2 - offsetx, y2 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x2 + offsetx, y2 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x1 + offsetx, y1 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_SetClipArea(float x, float y, float width, float height)
{
	int ix, iy, iw, ih;
	DrawQ_FlushUI();

	// We have to convert the con coords into real coords
	// OGL uses bottom to top (origin is in bottom left)
	ix = (int)(0.5 + x * ((float)r_refdef.view.width / vid_conwidth.integer)) + r_refdef.view.x;
	iy = (int)(0.5 + y * ((float)r_refdef.view.height / vid_conheight.integer)) + r_refdef.view.y;
	iw = (int)(0.5 + width * ((float)r_refdef.view.width / vid_conwidth.integer));
	ih = (int)(0.5 + height * ((float)r_refdef.view.height / vid_conheight.integer));
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		GL_Scissor(ix, vid.height - iy - ih, iw, ih);
		break;
	} // switch

	GL_ScissorTest(true);
}

void DrawQ_ResetClipArea(void)
{
	DrawQ_FlushUI();
	GL_ScissorTest(false);
}

void DrawQ_Finish(void)
{
	DrawQ_FlushUI();
	r_refdef.draw2dstage = 0;
}

void DrawQ_RecalcView(void)
{
	DrawQ_FlushUI();
	if (r_refdef.draw2dstage)
		r_refdef.draw2dstage = -1; // next draw call will set viewport etc. again
}

void DrawQ_FlushUI(void)
{
	model_t *mod = CL_Mesh_UI();
	if (mod->num_surfaces == 0)
		return;

	if (!r_draw2d.integer && !r_draw2d_force) {
		Mod_Mesh_Reset(mod);
		return;
	}

	// this is roughly equivalent to R_Mod_Draw, so the UI can use full material feature set
	r_refdef.view.colorscale = 1;
	r_textureframe++; // used only by R_GetCurrentTexture
	GL_DepthMask(false);

	Mod_Mesh_Finalize(mod);
	R_DrawModelSurfaces(&cl_meshentities[MESH_UI_1].render, 
		q_skysurfaces_false, q_write_depth_false, q_depthonly_false, 
		q_debug_false, q_prepass_false, q_is_ui_true // FFFFFT
		);
	//R_DrawModelSurfaces(&cl_meshentities[MESH_UI_1].render, 
	//	false, false, false, false, false, true // FFFFFT
	//	);

	

	Mod_Mesh_Reset(mod);
}



float Draw_StringWidthf (dp_font_t *dpf, ccs *s, float fontsize)
{
	//int slen = strlen(s);
	size_t maxlen = 0;

	float fwidth = DrawQ_TextWidth_UntilWidth_TrackColors_Scale(
		s, 
		&maxlen, fontsize, fontsize, scale_1_0, scale_1_0, q_outcolor_null, q_ignore_color_codes_false, 
		dpf, /*maxwidth*/ 1000000000)
		;

	//int j = maxlen; // Number of characters of the string that render
	return fwidth;
}

int Draw_StringWidthInt (dp_font_t *dpf, ccs *s, float fontsize)
{
	return ceil(Draw_StringWidthf(dpf,s,fontsize));
}

// This was a failure ...
//float FontStringSimplex127 (dp_font_t *dpf, ccs *s, float fontsize)
//{
//	// Baker: There is an advance field for glyphs, the below will never "work"
//	// for any of the fonts we are using.
//	ccs *p = s;
//	float w = 0;
//	while (*p) {
//		int ch = *p++;
//		if (ch <= 127)
//			w = dpf->width_of[ch] * fontsize;
//	}
//	return w;
//}
