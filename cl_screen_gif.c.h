// Baker: The delay is in centi-seconds -- 1/100 the of a second.


WARP_X_ (gd_GIF_t)

//#define GIF_HEAVY_X





#undef strncpy

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef struct Entry_s {
    unsigned short /* uint16_t */ length;
    unsigned short /* uint16_t */ prefix;
    unsigned char /* uint8_t */  suffix;
} Entry_t;

typedef struct Table_s {
    int bulk;
    int nentries;
    Entry_t *entries;
} Table_t;

static void discard_sub_blocks(gd_GIF_t *gif)
{
    byte size;

    do {
        baker_read	(&gif->b, &size, 1);
        baker_lseek	(&gif->b, size, SEEK_CUR);
    } while (size);
}

static void read_plain_text_ext(gd_GIF_t *gif)
{
    if (gif->plain_text) {
        unsigned short	tx, ty, tw, th;
        byte			cw, ch, fg, bg;
        off_t			sub_block;

		baker_lseek (&gif->b, 1, SEEK_CUR); // block size = 12
        
		tx = baker_read_usshort(&gif->b);
        ty = baker_read_usshort(&gif->b);
        tw = baker_read_usshort(&gif->b);
        th = baker_read_usshort(&gif->b);
        
		baker_read (&gif->b, &cw, 1);
        baker_read (&gif->b, &ch, 1);
        baker_read (&gif->b, &fg, 1);
        baker_read (&gif->b, &bg, 1);
        sub_block = baker_lseek(&gif->b, 0, SEEK_CUR);

		gif->plain_text(gif, tx, ty, tw, th, cw, ch, fg, bg);
        
		baker_lseek(&gif->b, sub_block, SEEK_SET);
    } else {
        // Discard plain text metadata
		baker_lseek(&gif->b, 13, SEEK_CUR);
    }
    // Discard plain text sub-blocks
    discard_sub_blocks(gif);
}

static void read_graphic_control_ext(gd_GIF_t *gif)
{
    byte rdit;

    // Discard block size (always 0x04)
    baker_lseek	(&gif->b, 1, SEEK_CUR);
    baker_read	(&gif->b, &rdit, 1);
    
	gif->gce.disposal_method	= (rdit >> 2) & 3;
    gif->gce.gifinput			= rdit & 2;
    gif->gce.transparency		= rdit & 1;
    gif->gce.delay_100			= baker_read_usshort(&gif->b);
    
	baker_read (&gif->b, &gif->gce.transparency_index, 1);
    
	// Skip block terminator
    baker_lseek (&gif->b, 1, SEEK_CUR);
}

static void read_comment_ext(gd_GIF_t *gif)
{
    if (gif->comment) {
		off_t sub_block = baker_lseek(&gif->b, 0, SEEK_CUR);
        gif->comment(gif);
		baker_lseek(&gif->b, sub_block, SEEK_SET);
    }
    // Discard comment sub-blocks
    discard_sub_blocks(gif);
}

static void read_application_ext(gd_GIF_t *gif)
{
    char app_id[8];
    char app_auth_code[3];

    // Discard block size (always 0x0B)
	baker_lseek	(&gif->b, 1, SEEK_CUR);
	
	// Application Identifier
	baker_read	(&gif->b, app_id, 8);
	baker_read	(&gif->b, app_auth_code, 3);

    if (String_Starts_With_PRE (app_id, "NETSCAPE")) {
		// Baker: getframe is coming here
        // Discard block size (0x03) and constant byte (0x01)
		baker_lseek(&gif->b, 2, SEEK_CUR); // += 2
		gif->loop_count = baker_read_usshort(&gif->b);
		baker_lseek(&gif->b, 1, SEEK_CUR);
    } else if (gif->application) {
		off_t sub_block = baker_lseek(&gif->b, 0, SEEK_CUR);
        gif->application(gif, app_id, app_auth_code);
		baker_lseek (&gif->b, sub_block, SEEK_SET);
        discard_sub_blocks(gif);
    } else {
        discard_sub_blocks(gif);
    }
}

static void read_ext(gd_GIF_t *gif)
{
    byte label;

	baker_read(&gif->b, &label, 1);

	switch (label) {
    case 0x01:
		read_plain_text_ext(gif);
        break;

    case 0xF9:
        read_graphic_control_ext(gif);
        break;

    case 0xFE:
        read_comment_ext(gif);
        break;

    case 0xFF:
        read_application_ext(gif); // getframe is coming here
        break;

    default:
        fprintf(stderr, "unknown extension: %02X\n", label);
    } // sw
}

static Table_t *new_table(int key_size)
{
    int key;
    int init_bulk = MAX(1 << (key_size + 1), 0x100);
    Table_t *table = (Table_t *)malloc(sizeof(*table) + sizeof(Entry_t) * init_bulk);
    if (table) {
        table->bulk = init_bulk;
        table->nentries = (1 << key_size) + 2;
        table->entries = (Entry_t *) &table[1];
		for (key = 0; key < (1 << key_size); key++) {
            table->entries[key].length = 1;
			table->entries[key].prefix = 0xFFF;
			table->entries[key].suffix = key;
		} // for
    } // if
    return table;
}

/* Add table entry. Return value:
 *  0 on success
 *  +1 if key size must be incremented after this addition
 *  -1 if could not realloc table */
static int add_entry(Table_t **tablep, unsigned short length, unsigned short prefix, unsigned char /* uint8_t */ suffix)
{
    Table_t *table = *tablep;
    if (table->nentries == table->bulk) {
        table->bulk *= 2;
        table = (Table_t *)realloc(table, sizeof(*table) + sizeof(Entry_t) * table->bulk);
        if (!table) return -1;
        table->entries = (Entry_t *) &table[1];
        *tablep = table;
    }
    table->entries[table->nentries].length = length;
	table->entries[table->nentries].prefix = prefix;
	table->entries[table->nentries].suffix = suffix;
    table->nentries++;
    if ((table->nentries & (table->nentries - 1)) == 0)
        return 1;
    return 0;
}

static unsigned short get_key(gd_GIF_t *gif, int key_size, byte *sub_len, byte *shift, byte *byte)
{
    int bits_read;
    int rpad;
    int frag_size;
    unsigned short key;

    key = 0;
    for (bits_read = 0; bits_read < key_size; bits_read += frag_size) {
        rpad = (*shift + bits_read) % 8;
        if (rpad == 0) {
            // Update byte
            if (*sub_len == 0) {
				baker_read(&gif->b, sub_len, 1); // Must be nonzero!
                if (*sub_len == 0)
                    return 0x1000;
            }
            
			baker_read(&gif->b, byte, 1);
            (*sub_len)--;
        }
        frag_size = MIN(key_size - bits_read, 8 - rpad);
        key |= ((unsigned short) ((*byte) >> rpad)) << bits_read;
    }
    // Clear extra bits to the left
    key &= (1 << key_size) - 1;
    *shift = (*shift + key_size) % 8;
    return key;
}

// Compute output index of y-th input line, in frame of height h
static int interlaced_line_index(int h, int y)
{
    int p; // number of lines in current pass

    p = (h - 1) / 8 + 1;
    if (y < p) // pass 1
        return y * 8;
    y -= p;
    p = (h - 5) / 8 + 1;
    if (y < p) // pass 2
        return y * 8 + 4;
    y -= p;
    p = (h - 3) / 4 + 1;
    if (y < p) // pass 3
        return y * 4 + 2;
    y -= p;
    // pass 4
    return y * 2 + 1;
}

/* Decompress image pixels.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int read_image_data (gd_GIF_t *gif, int interlace)
{
    byte			sub_len, shift, byte;
    int				init_key_size, key_size, table_is_full = false;
    int				frm_off, frm_size, str_len = 0, i, p, x, y;
    unsigned short	key, clear, stop;
    int				ret;
    Table_t			*table;
	Entry_t			entry = {0};
    off_t			start, endofwhat; // Baker: end of what?

	baker_read	(&gif->b, &byte, 1);
    key_size = (int) byte;
    if (key_size < 2 || key_size > 8)
        return -1;
    
	start = baker_lseek(&gif->b, 0, SEEK_CUR);
    discard_sub_blocks(gif);
    
	// Baker this is used later ...
	endofwhat = baker_lseek (&gif->b, 0, SEEK_CUR); // Baker: This does nothing except return position

	baker_lseek (&gif->b, start, SEEK_SET);

    clear			= 1 << key_size;
    stop			= clear + 1;
    table			= new_table(key_size);
    key_size++;

    init_key_size = key_size;
    sub_len			= shift = 0;
    key				= get_key(gif, key_size, &sub_len, &shift, &byte); /* clear code */
    frm_off			= 0;
    ret				= 0;

    frm_size = gif->fw*gif->fh;

	if (gif->shall_process_data) {
		while (frm_off < frm_size) {
			if (key == clear) {
				key_size = init_key_size;
				table->nentries = (1 << (key_size - 1)) + 2;
				table_is_full = 0;
			} else if (!table_is_full) {
				ret = add_entry(&table, str_len + 1, key, entry.suffix);
				if (ret == -1) {
					free(table);
					return -1;
				}
				if (table->nentries == 0x1000) {
					ret = 0;
					table_is_full = 1;
				}
			}
			key = get_key (gif, key_size, &sub_len, &shift, &byte);
			if (key == clear) continue;
			if (key == stop || key == 0x1000) break;
			if (ret == 1) key_size++;
			entry = table->entries[key];
			str_len = entry.length;
			for (i = 0; i < str_len; i++) {
				p = frm_off + entry.length - 1;
				x = p % gif->fw;
				y = p / gif->fw;
				if (interlace)
					y = interlaced_line_index((int) gif->fh, y);
				gif->frame_palette_pels[(gif->fy + y) * gif->width + gif->fx + x] = entry.suffix; // GIF_HEAVY_X
				if (entry.prefix == 0xFFF)
					break;
				else
					entry = table->entries[entry.prefix];
			}
			frm_off += str_len;
			if (key < table->nentries - 1 && !table_is_full)
				table->entries[table->nentries - 1].suffix = entry.suffix;
		}
		free(table);

		if (key == stop)
			baker_read (&gif->b, &sub_len, 1); // Must be zero!

	} // if shall process data

    
	baker_lseek (&gif->b, endofwhat, SEEK_SET);
    return 0;
}

/* Read image.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int read_image(gd_GIF_t *gif)
{
    byte fisrz;
    int interlace;

    // Image Descriptor
    gif->fx = baker_read_usshort(&gif->b);
    gif->fy = baker_read_usshort(&gif->b);
    
    if (gif->fx >= gif->width || gif->fy >= gif->height)
        return -1;
    
    gif->fw = baker_read_usshort(&gif->b);
    gif->fh = baker_read_usshort(&gif->b);
    
    gif->fw = MIN(gif->fw, gif->width - gif->fx);
    gif->fh = MIN(gif->fh, gif->height - gif->fy);
    
	baker_read (&gif->b, &fisrz, 1);
    interlace = fisrz & 0x40;
    // Ignore Sort Flag
    // Local Color Table_t?
    if (fisrz & 0x80) {
        // Read LCT
        gif->lct.size = 1 << ((fisrz & 0x07) + 1);
		baker_read (&gif->b, gif->lct.colors, 3 * gif->lct.size);
        gif->palette = &gif->lct;
    } else
        gif->palette = &gif->gct;
    
	// Image Data
    return read_image_data (gif, interlace);
}

WARP_X_CALLERS_ (gd_render_frame)

static void _gd_render_frame_render_frame_rect (gd_GIF_t *gif, rgb3 *buffer)
{
    int i, j, k;
    byte index, *color3;
    i = gif->fy * gif->width + gif->fx;
    for (j = 0; j < gif->fh; j++) { // height
		int ofsy = (gif->fy + j) * gif->width;
        for (k = 0; k < gif->fw; k++) { // width
			index = gif->frame_palette_pels[ofsy + gif->fx + k]; // palette index GIF_HEAVY_X
            color3 = &gif->palette->colors[index * RGB_3];
            if (!gif->gce.transparency || index != gif->gce.transparency_index)
                memcpy	(&buffer[(i + k) * RGB_3], color3, 3);
        }
        i += gif->width;
    } // for
}

static void gif_dispose(void *z)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
    int i, j, k;
    byte *bgcolor;
    switch (gif->gce.disposal_method) {
    case 2: /* Restore to background color. */
		// Baker: This looks expensive.
        bgcolor = &gif->palette->colors[gif->bgindex * RGB_3];
        i = gif->fy * gif->width + gif->fx;
        for (j = 0; j < gif->fh; j++) {
            for (k = 0; k < gif->fw; k++)
                memcpy (&gif->canvas_rgb3[(i+k) * RGB_3], bgcolor, RGB_3);
            i += gif->width;
        }
        break;
    case 3: /* Restore to previous, i.e., don't update canvas.*/
        break;
    default:
        /* Add frame non-transparent pixels to canvas. */
        _gd_render_frame_render_frame_rect (gif, gif->canvas_rgb3);
    }
}

/* Return 1 if got a frame; 0 if got GIF trailer; -1 if error. */
WARP_X_ (gif_get_frame_read_image)


int gd_get_frame (void *z, qbool shall_process_data)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;

	gif->shall_process_data = shall_process_data;

    char sep;

    gif_dispose	(gif);
	baker_read	(&gif->b, &sep, 1);
    while (sep != ',') {
        if (sep == ';')	return 0;
        if (sep == '!')	
			read_ext(gif);
        else			return -1;
        
		baker_read (&gif->b, &sep, 1);
    }
    if (read_image(gif) == -1)
        return -1;
    return 1;
}

// Baker: This looks like it outputs to rgb3
void gd_render_frame(void *z, rgb3 *buffer)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
    memcpy	(buffer, gif->canvas_rgb3, gif->width * gif->height * RGB_3);
    _gd_render_frame_render_frame_rect(gif, buffer);
}

int gd_is_bgcolor(void *z, byte color[3])
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
    return !memcmp(&gif->palette->colors[gif->bgindex*3], color, 3);
}

int gd_getwidth (void *z)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
	return gif->width;
}

int gd_getheight (void *z)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
	return gif->height;
}


void gd_rewind (void *z)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
	baker_lseek(&gif->b, gif->anim_start, SEEK_SET);
}

WARP_X_ (SCR_gifclip_f gd_open_gif)
// Baker: We are freeing the gif, but where was it allocated?
// gd_open_gif callocs the gif struct

void *gd_close_gif_maybe_returns_null (void *z)
{
	gd_GIF_t *gif = (gd_GIF_t *)z;
	if (gif) {
		if (gif->b.buf_malloc) {
			baker_close (&gif->b); // frees buf in struct b
		}

		if (gif->frame_palette_pels) {
			free (gif->frame_palette_pels);
			gif->frame_palette_pels = NULL;
		}

		if (gif->blobz_alloc) {
			free(gif->blobz_alloc);    
			gif->blobz_alloc = NULL;
		}

		free (gif);
	} // gif
	return NULL;
}

#include "cl_screen.h"
// Baker: this callocs the gif struct
void *gd_open_gif_alloc(const char *fname)
{
    byte			sigver[3];
    unsigned short	width, height, depth;
    byte			fdsz, bgidx, aspect;
    int				i;
    byte			*bgcolor;
    int				gct_sz;
    gd_GIF_t		*gif;

	bakerbuf_t b	= {0};
	bakerbuf_t *bb	= &b;
	
	int ok = baker_open_actual_file_is_ok(bb, fname);

	if (ok == false) return NULL;

    // Header
	baker_read (bb, sigver, 3); // @ 0

    if (memcmp(sigver, "GIF", 3) != 0) {
        fprintf(stderr, "invalid signature\n");
		baker_close (bb); goto fail;
    }
    // Version
    
	baker_read (bb, sigver, 3); // @ 3, 6
    if (memcmp(sigver, "89a", 3) != 0) {
        fprintf(stderr, "invalid version\n");
        baker_close (bb); goto fail;
    }
    // Width x Height
    
	width  = baker_read_usshort(bb); // @6, 8
	height = baker_read_usshort(bb); // @8 , 10
	baker_read (bb, &fdsz, 1); // @ 10, 11
    
	// Presence of GCT
    if (!(fdsz & 0x80)) {
        fprintf(stderr, "no global color table\n");
        baker_close (bb); goto fail;
    }
    
	// Color Space's Depth
    depth = ((fdsz >> 4) & 7) + 1;
    
	// Ignore Sort Flag
    // GCT Size
	// Baker: Yay size!  Size of what?  Hopefully number of colors in color table
    gct_sz = 1 << ((fdsz & 0x07) + 1);
    
	// Background Color Index    
	baker_read (bb, &bgidx, 1); // @ 11, 12
    
	// Aspect Ratio
	baker_read (bb, &aspect, 1); // @12, 13
    
	// Create gd_GIF_t Structure
    gif = (gd_GIF_t *)calloc(1, sizeof(*gif));
	if (!gif) {
		baker_close (bb); goto fail;
	}

transfer:
	gif->b = b; // STRUCT COPY!
    gif->width  = width;
    gif->height = height;
    gif->depth  = depth;
    
	// Read GCT - Baker: I assume this is GIF color table.
    gif->gct.size = gct_sz;
	baker_read	(&gif->b, gif->gct.colors, RGB_3 * gif->gct.size);
    gif->palette = &gif->gct;
    gif->bgindex = bgidx;
    gif->frame_palette_pels = (unsigned char *)calloc(4, width * height); 
	// Baker: Why 4?  Because it is being clever and allocating the canvas also
    if (!gif->frame_palette_pels) {
		baker_close (&gif->b);
        free(gif);
        goto fail;
    }
	// Baker: Ok this is stupid ...
	// It allocates the rgb3 canvas after the palette_pels
	// For a total of 4 bytes per pixel
    gif->canvas_rgb3 = &gif->frame_palette_pels[width * height]; // Baker: Set canvas to after palette pels
    if (gif->bgindex)
        memset (gif->frame_palette_pels, gif->bgindex, gif->width * gif->height); // GIF_HEAVY_X

    bgcolor = &gif->palette->colors[gif->bgindex * RGB_3];
    if (bgcolor[0] || bgcolor[1] || bgcolor [2])
        for (i = 0; i < gif->width * gif->height; i++)
            memcpy (&gif->canvas_rgb3[i * RGB_3], bgcolor, RGB_3); // GIF_HEAVY_X

	gif->anim_start = baker_lseek(&gif->b, 0, SEEK_CUR); // WHY?
    goto ok;

fail:
    return 0;
ok:
    return gif;
}

WARP_X_ (gd_close_gif_maybe_returns_null bakergifdecode_close)
void *gd_open_gif_from_memory_alloc (const void *data, size_t datalen)
{
    byte			sigver[3];
    unsigned short	width, height, depth;
    byte			fdsz, bgidx, aspect;
    byte			*bgcolor;
    int				gct_sz;
    gd_GIF_t		*gif;

	bakerbuf_t b	= {0};
	bakerbuf_t *bb	= &b;
	
	//int ok = baker_open_is_ok(bb, fname);
	int ok = baker_open_from_memory_is_ok (bb, data, datalen);

	if (ok == false) return NULL;

    // Header
	baker_read (bb, sigver, 3); // @ 0

    if (memcmp(sigver, "GIF", 3) != 0) {
        fprintf(stderr, "invalid signature\n");
		baker_close (bb); goto fail;
    }
    // Version
    
	baker_read (bb, sigver, 3); // @ 3, 6
    if (memcmp(sigver, "89a", 3) != 0) {
        fprintf(stderr, "invalid version\n");
        baker_close (bb); goto fail;
    }
    // Width x Height
    
	width  = baker_read_usshort(bb); // @6, 8
	height = baker_read_usshort(bb); // @8 , 10
	baker_read (bb, &fdsz, 1); // @ 10, 11
    
	// Presence of GCT
    if (!(fdsz & 0x80)) {
        fprintf(stderr, "no global color table\n");
        baker_close (bb); goto fail;
    }
    
	// Color Space's Depth
    depth = ((fdsz >> 4) & 7) + 1;
    
	// Ignore Sort Flag
    // GCT Size
	// Baker: Yay size!  Size of what?  Hopefully number of colors in color table
    gct_sz = 1 << ((fdsz & 0x07) + 1);
    
	// Background Color Index    
	baker_read (bb, &bgidx, 1); // @ 11, 12
    
	// Aspect Ratio
	baker_read (bb, &aspect, 1); // @12, 13
    
	// Create gd_GIF_t Structure
    gif = (gd_GIF_t *)calloc(1, sizeof(*gif));
	if (!gif) {
		baker_close (bb); goto fail;
	}

transfer:
	gif->b = b; // STRUCT COPY!
    gif->width  = width;
    gif->height = height;
    gif->depth  = depth;
    
	// Read GCT - Baker: I assume this is GIF color table.
    gif->gct.size = gct_sz;
	
	baker_read	(&gif->b, gif->gct.colors, RGB_3 * gif->gct.size);
    
	gif->palette = &gif->gct;
    gif->bgindex = bgidx;
    gif->frame_palette_pels = (unsigned char *)calloc(4, width * height);

	// Baker: Why 4?  Because it is being clever and allocating the canvas also
    if (!gif->frame_palette_pels) {
		baker_close (&gif->b);
        free(gif);
        goto fail;
    }

	// Baker: Ok this is stupid ...
	// It allocates the rgb3 canvas after the palette_pels
	// For a total of 4 bytes per pixel
    gif->canvas_rgb3 = &gif->frame_palette_pels[width * height]; // Baker: Set canvas to after palette pels
	if (gif->bgindex)
        memset (gif->frame_palette_pels, gif->bgindex, gif->width * gif->height); //GIF_HEAVY_X

    bgcolor = &gif->palette->colors[gif->bgindex * RGB_3];
    if (bgcolor[0] || bgcolor[1] || bgcolor [2])
        for (int i = 0; i < gif->width * gif->height; i ++)
            memcpy (&gif->canvas_rgb3[i * RGB_3], bgcolor, RGB_3); // GIF_HEAVY_X
    
	gif->anim_start = baker_lseek(&gif->b, 0, SEEK_CUR); // WHY?
    goto ok;

fail:
    return 0;
ok:
    return gif;
}

WARP_X_ (SCR_jpegextract_from_savegame_f)
int gif_get_length_numframes (const void *data, size_t datasize, double *ptotal_seconds)
{
	gd_GIF_t *gif = (gd_GIF_t *)gd_open_gif_from_memory_alloc (data, datasize);

	//int total_frames = 0;
	int total_centiseconds = 0;
	int numframes = 0;
	
	while (1) {
		int ret = gd_get_frame(gif, /*shall process?*/ false);
		if (ret <= 0)	break;
		total_centiseconds += gif->gce.delay_100;
		// Con_PrintLinef ("Frame %4d: delay is %d centiseconds tot %d", numframes, (int)gif->gce.delay_100, (int)total_centiseconds);
		
		numframes ++;
	} // while

	*ptotal_seconds = (total_centiseconds / 100.0);
	
	gif = (gd_GIF_t *)gd_close_gif_maybe_returns_null (gif);

	return numframes;
}

WARP_X_ (bakergifdecode_open)
void SCR_gifclip_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) == 1) {
		Con_PrintLinef ("Usage: %s [gif] decode gif and copy to clipboard", Cmd_Argv(cmd, 0) );
		return;
	}

	const char *s_filename = Cmd_Argv(cmd, 1);
	const char *s_framenum = Cmd_Argc(cmd) == 3 ? Cmd_Argv(cmd, 2) : NULL;

	int framenum_wanted = s_framenum ? atoi(s_framenum) : 0;
	Con_PrintLinef ("Doing frame number %d", framenum_wanted);

	char		*s_realpath_alloc	= NULL;
	byte		*filedata_alloc		= NULL;
	byte		*frame_rgb3_alloc	= NULL;
	gd_GIF_t	*gif				= NULL;

	fs_offset_t filesize;
	filedata_alloc = FS_LoadFile (s_filename, tempmempool, fs_quiet_true, &filesize);

	int is_ok = true;
	if (filedata_alloc == NULL) {
		// We can return from here because no cleanup needed
		Con_PrintLinef ("Couldn't open " QUOTED_S, s_filename);
		is_ok = false; goto exitor;
	} // filedata_alloc

	double total_seconds = 0;
	int total_frames = gif_get_length_numframes (filedata_alloc, filesize, &total_seconds);
	Con_PrintLinef ("Number of frames = %d time is %f", total_frames, total_seconds);

#if 111
	gif = (gd_GIF_t *)gd_open_gif_from_memory_alloc (filedata_alloc, filesize);
#else
	s_realpath_alloc = FS_RealFilePath_Z_Alloc (s_filename);

	gif = (gd_GIF_t *)gd_open_gif_alloc(s_realpath_alloc);
#endif

	int numpels			= gif->width * gif->height;
	int numpelbytes3	= numpels * RGB_3;
	int numpelbytes4	= numpels * RGBA_4;
	frame_rgb3_alloc	= Mem_TempAlloc_Bytes (numpelbytes3);

    Con_PrintLinef ("GIF %dx%d %d colors", gif->width, gif->height, gif->palette->size);

	int numframes = 0;

	is_ok = false;
	while (1) {
		int ret = gd_get_frame(gif, /*shall process?*/ true);

		if (ret <= 0)
			break;

		Con_PrintLinef ("Frame %4d: delay is %d centiseconds", numframes, (int)gif->gce.delay_100);

		if (framenum_wanted == numframes) {
			is_ok = true;
			break;
		}

		numframes ++;
	} // while

	if (is_ok == false) {
        Con_PrintLinef ("Could not get frame");
		is_ok = false; goto exitor;
	}

	// Background color

	byte *color = &gif->gct.colors[gif->bgindex * RGB_3];

	for (int idx = 0; idx < numpelbytes3; idx += RGB_3) {
		frame_rgb3_alloc[idx + 0] = color[0];
		frame_rgb3_alloc[idx + 1] = color[1];
		frame_rgb3_alloc[idx + 2] = color[2];
	}

	// Return 1 if got a frame; 0 if got GIF trailer; -1 if error
	gd_render_frame (gif, frame_rgb3_alloc);

	byte *frame_rgb4_alloc = Mem_TempAlloc_Bytes (numpelbytes4);
	for (int idx = 0; idx < numpels; idx ++) {
		// Do we need to flip 0 and 2 color index? YES
		frame_rgb4_alloc[idx * RGBA_4 + 0] = frame_rgb3_alloc[idx * RGB_3 + 2];
		frame_rgb4_alloc[idx * RGBA_4 + 1] = frame_rgb3_alloc[idx * RGB_3 + 1];
		frame_rgb4_alloc[idx * RGBA_4 + 2] = frame_rgb3_alloc[idx * RGB_3 + 0];
		frame_rgb4_alloc[idx * RGBA_4 + 3] = 255; // alpha
	}

	int clipok = Sys_Clipboard_Set_Image_BGRA_Is_Ok ((bgra4 *)frame_rgb4_alloc, gif->width, gif->height);

	Con_PrintLinef (clipok ? "Image copied to clipboard" : "Clipboard copy failed");
	Con_PrintLinef ("Total frames = %d", numframes);

	Mem_FreeNull_ (frame_rgb4_alloc);

exitor:
    gif = (gd_GIF_t *)gd_close_gif_maybe_returns_null (gif);
	Mem_FreeNull_ (frame_rgb3_alloc);
	Mem_FreeNull_ (s_realpath_alloc);
	Mem_FreeNull_ (filedata_alloc);
}


