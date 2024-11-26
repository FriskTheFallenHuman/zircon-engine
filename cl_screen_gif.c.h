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

#if 0 // August 14 2024 - Not sure if this works or not.  Not going to guess.
WARP_X_ (bakergifdecode_open)
void AngleCalc_f (cmd_state_t *cmd)
{
	//float a = 500;
	//float b = 60;
	//// What is c?
	//float c = sqrt( (a * a + b * b) );
	//Con_PrintLinef ("Hypo is %f", c);

	//// Angles 90
	//// What are other 2 angles?

    float a, b, c, A, B, C, R, s, pi, area;
    pi = acos((double)-1);

//float angba = atan2(ab.y, ab.x);
//float angbc = atan2(cb.y, cb.x);
//float rslt = angba - angbc;
//float rs = (rslt * 180) / 3.141592;
//https://stackoverflow.com/questions/3486172/angle-between-3-points


    //if (scanf("%f %f %f", &a, &b, &c) != 3)
    //{
    //    fprintf(stderr, "Failed to read 3 numbers\n");
    //    return 1;
    //}
	a = 500;
	b = 60;
	c = sqrt( (a * a + b * b) ); // Hypotenuse 503.xx
    if (a <= 0 || b <= 0 || c <= 0) {
        Con_PrintLinef ("Sides must be strictly positive");
        return;
    }

    s = (a + b + c) / 2;
    if (a > s || b > s || c > s) {
        Con_PrintLinef ("The three sides %.2f, %.2f, %.2f do not form a triangle",
                a, b, c);
        return;
    }

    area = sqrt(s * (s - a) * (s - b) * (s - c));

    R = (a * b * c) / (4 * area);

    A = (180 / pi) * asin(a / (2 * R));
    B = (180 / pi) * asin(b / (2 * R));
    C = (180 / pi) * asin(c / (2 * R));

    Con_PrintLinef ("Sides:  %6.2f %6.2f %6.2f", a, b, c);
    Con_PrintLinef ("Angles: %6.2f %6.2f %6.2f", A, B, C);

}
#endif

#ifdef CONFIG_MENU
void SCR_reclip_f (cmd_state_t *cmd)
{
	// Baker: Does this fix FTE null character warning? YES OR NO.
	// Baker: NO, and there aren't any.
	// And it reports as last line, perhaps ending comments fucks up ftegui only
	// because command line no issue.
	// Ignore FTE gui.

	char *s_z = Sys_GetClipboardData_Unlimited_ZAlloc();
	int slen = strlen(s_z);
	for (int j = 0; j < slen; j ++) {
		if (s_z[j] == 0 || s_z[j] > 126)
			s_z[j] = '@';
	}
	Con_PrintLinef ("Strlen is %f", (double)strlen(s_z) );
	Clipboard_Set_Text (s_z);
	Mem_FreeNull_ (s_z);
}
#endif

	//int j = Sys_CheckParm("-csg");
	//ccs *mapfullpath = sys.argv[i+1];
	//if (j && j + 1 < sys.argc &&

	//{
	//"classname" "info_decal"
	//"targetname" "path_1"
	//"texture" "maps_sh/dirt_field"
	//"options" ""
	//"target" "pathend_1"
	//"drop" "32"
	//"origin" "13092 -4442 32"
	//}
	//{
	//"classname" "info_decal"
	//"targetname" "pathend_1"
	//"texture" "maps_sh/dirt_field"
	//"options" ""
	//"origin" "12772 -8634 32"
	//}


WARP_X_ (String_Worldspawn_Value_For_Key_Sbuf)
	typedef struct _info_decal_st {
		vec3_t	origin;

		char	*classname;
		char	*targetname;
		char	*target;			// "target" "pathend_1"

		float	drop;				// "drop" "32"
		float	force_z;
		float	jitter_x;
		float	jitter_y;
		float	step;
		vec3_t	surface_size;
		char	*texture;			// "texture" "maps_sh/dirt_field"

		struct _info_decal_st	*next;
	} info_decal_s;

#define NFO_SURFACE_SIZE_DEFAULT_512	512
#define NFO_STEP_DEFAULT_384			384
#define NFO_DROP_DEFAULT_32				32

#define NFO_info_decal_start "info_decal_start"
#define NFO_info_decal_point "info_decal_point"

static info_decal_s *FindTarget (info_decal_s *head, ccs *target)
{
	for ( info_decal_s *d = head; d; d = d->next) {
		if (String_Match (d->targetname, target))
			return d;
		//Con_PrintLinef ("Decal origin " VECTOR3_5d1F, VECTOR3_SEND( d->origin));
	}
	Con_PrintLinef (CON_WARN "Warning: target %s not found", target);
	return NULL;
}

//-csg C:\galaxy\zircon\maps\treefall8
//char *String_Worldspawn_Value_For_Key_Sbuf
// (const char *s_entities_string, const char *find_keyname)
				static ccs *s_deke = "{\r\n"
   "\"classname\" \"_dec"
   "al\"\r\n"
   "\"target\" \"[DECALTARGET]\""
   "\r\n"
   "{\r\n"
   "\tpatchDef2\r\n"
   "\t{\r\n"
   "\t\t[TEXTURE]\r\n"
   "\t\t( 3 3 0 0 0 )\r"
   "\n"
   "\t\t(\r\n"
   "\t\t\t(\r\n"
   "\t\t\t\t( [ORIGIN_X-256] [ORIGIN_Y+256] [ORIGIN_Z_0]"
   " -2.005859 0"
   ".982873 )\r\n"
   "\t\t\t\t( [ORIGIN_X_0] [ORIGIN_Y+256] [ORIGIN_Z_0]"
   "  -1.505859 0.98"
   "2873 )\r\n"
   "\t\t\t\t( [ORIGIN_X+256] [ORIGIN_Y+256] [ORIGIN_Z_0]"
   "  -1.005859 0."
   "982873 )\r\n"
   "\t\t\t)\r\n"
   "\t\t\t(\r\n"
   "\t\t\t\t( [ORIGIN_X-256] [ORIGIN_Y_0] [ORIGIN_Z_0]"
   "  -2.005859 1.4"
   "82873 )\r\n"
   "\t\t\t\t( [ORIGIN_X_0] [ORIGIN_Y_0] [ORIGIN_Z_0]"
   " -1.505859 1.4828"
   "73 )\r\n"
   "\t\t\t\t( [ORIGIN_X+256] [ORIGIN_Y_0] [ORIGIN_Z_0]"
   " -1.005859 1.48"
   "2873 )\r\n"
   "\t\t\t)\r\n"
   "\t\t\t(\r\n"
   "\t\t\t\t( [ORIGIN_X-256] [ORIGIN_Y-256] [ORIGIN_Z_0]"
   " -2.005859 "
   "1.982873 )\r\n"
   "\t\t\t\t( [ORIGIN_X_0] [ORIGIN_Y-256] [ORIGIN_Z_0]"
   " -1.505859 1.9"
   "82873 )\r\n"
   "\t\t\t\t( [ORIGIN_X+256] [ORIGIN_Y-256] [ORIGIN_Z_0]"
   " -1.005859 1"
   ".982873 )\r\n"
   "\t\t\t)\r\n"
   "\t\t)\r\n"
   "\t}\r\n"
   "}\r\n"
   "}\r\n";

baker_string_t *CSG_Process_BSAlloc (ccs *datasrc);


void SCR_vectornormal3_f (cmd_state_t *cmd)
{
	if (Cmd_Argc (cmd) < 10) {
		Con_PrintLinef ("Requires ax ay az bx by bz cx cy cz");
		return;
	}
	vec3_t a = { atof(Cmd_Argv (cmd, 1)), atof(Cmd_Argv (cmd, 2)), atof(Cmd_Argv (cmd, 3)) };
	vec3_t b = { atof(Cmd_Argv (cmd, 4)), atof(Cmd_Argv (cmd, 5)), atof(Cmd_Argv (cmd, 6)) };
	vec3_t c = { atof(Cmd_Argv (cmd, 7)), atof(Cmd_Argv (cmd, 8)), atof(Cmd_Argv (cmd, 9)) };
	Con_PrintLinef ("a: " VECTOR3_5d1F, VECTOR3_SEND(a));
	Con_PrintLinef ("b: " VECTOR3_5d1F, VECTOR3_SEND(b));
	Con_PrintLinef ("c: " VECTOR3_5d1F, VECTOR3_SEND(c));

	vec3_t pba; VectorSubtract (b, a, pba);
	vec3_t pca; VectorSubtract (c, a, pca);
	//vec3_t pn; VectorMultiply (pab, pac, pn);
	vec3_t cross; CrossProduct (pba, pca, cross);
	Con_PrintVarVector3 (pba);
	Con_PrintVarVector3 (pca);
	Con_PrintVarVector3 (cross);


}


#ifdef CONFIG_MENU
void SCR_csgtool_clipboard_f (cmd_state_t *cmd)
{
	ccs *data_za = Sys_GetClipboardData_Unlimited_ZAlloc();

	baker_string_t *bs_extras_alloc = CSG_Process_BSAlloc (data_za);

	if (bs_extras_alloc) {
		Clipboard_Set_Text (bs_extras_alloc->string);
		Con_PrintLinef ("Clipboard set strlen = %d", (int)bs_extras_alloc->length);
	} else {
		Con_PrintLinef ("NO DATA FOR CLIPBOARD");
	}

	BakerString_Destroy_And_Null_It (&bs_extras_alloc); // It checks for NULL
	Mem_FreeNull_ (data_za);
}
#endif


#include "csg_pasteat.c.h" // csgtool2_pasteat_clipboard

#ifdef CONFIG_MENU
void SCR_csgtool2_pasteat_clipboard_f (cmd_state_t *cmd)
{

	ccs *data_za = Sys_GetClipboardData_Unlimited_ZAlloc();
	
	Con_PrintLinef ("Clipboarded get strlen = %d ...", (int)strlen(data_za));

	entitylist_t list = {0};

	int is_ok = entitylist_parsemaptxt (&list, data_za);

	baker_string_t *bsa = entitylist_maptext_bsalloc (&list);
	Clipboard_Set_Text (bsa->string);
	Con_PrintLinef ("Clipboarded it strlen = %d -- done", (int)bsa->length);
	BakerString_Destroy_And_Null_It (&bsa);

	//entitylistprint			(&list);
	entitylistfreecontents	(&list);

	Mem_FreeNull_ (data_za);
	
}
#endif // CONFIG_MENU

baker_string_t *CSG_Process_BSAlloc (ccs *datasrc) // Returns a (baker_string_t *) or NULL if no data
{
	//Con_PrintLinef ("Strlen is %f", (double)strlen(s_z) );
	ccs *text = datasrc;

	mempool_t *dbpool = Mem_AllocPool("Temp CSG Pool", /*flags*/ 0, /*parent*/ NULL);

	//Clipboard_Set_Text (s_z);
	char		value[4096];
	char		key[128];

	baker_string_t *bs_decals = BakerString_Create_Malloc (text);
	BakerString_CatC (bs_decals, "// This line" NEWLINE);
	BakerString_CatC (bs_decals, text);

// Read some data ...
//	if (COM_Parse_Basic(&datasrc) == false || com_token[0] != '{')	// Opening brace is start of worldspawn
//		goto exitor;



	info_decal_s *head = NULL;
	info_decal_s *cur = NULL;


	int state = 0;
	int edictnum = 0;
	int depth = 0;
	int in_patch = 0;

	int idnum = 0;

	// Plan ... find all "classname" "info_decal"
	// Need to struct allocate making a chain.
	#define STATE_IGNORE_NEG_1	-1
	#define STATE_CHECKCLASS_0   0
	#define STATE_WANTED_1		 1
	#define STATE_BRUSHDATA_2	 2

	int iters = 0;

	while (1) {
		iters ++;
		// Read some datasrc ...
		if (COM_Parse_Basic(&datasrc) == false)
			break; // End of string data

		//DebugPrintLinef ("iters = %7d: Token %s", iters, com_token);
		//if (String_Match (com_token, "classname")) {
		//	int j = 5;
		//}

		if (state == STATE_BRUSHDATA_2) {
			if (com_token[0] == '}') {
				depth --;
				//Con_PrintLinef ("Depth -- to %d", depth);
				//DebugPrintLinef ("Depth -- to %d", depth);

				if (depth == 0) {
					state = STATE_CHECKCLASS_0; // Out of brush data
				}
			} else if (com_token[0] == '{') {
				depth ++;
				//Con_PrintLinef ("Depth ++ to %d", depth);
				//DebugPrintLinef ("Depth ++ to %d", depth);
			} else {
				//Con_PrintLinef ("Brush data %s", com_token);
				//DebugPrintLinef ("Brush data %s", com_token);
			}

			continue;
		}

		if (com_token[0] == '}') {	// End of entity
			state = STATE_CHECKCLASS_0;
			depth --;
			int in_patch = 0;
			//cur = NULL;
			//DebugPrintLinef ("End of entity");
			edictnum ++;
			continue;
		}

		if (com_token[0] == '{') {
			// Baker: Brush data .. read until ...

			depth ++;
			//Con_PrintLinef ("Entering brush data depth = %d", depth);
			if (depth == 2) {
				//DebugPrintLinef ("Entering brush data depth = %d", depth);
				state = STATE_BRUSHDATA_2;
			}
			continue;
		}

		if (String_Match (com_token, "patchDef2")) {
			in_patch ++;
			//DebugPrintLinef ("Entering patchDef2 = %d", in_patch);
			state = STATE_BRUSHDATA_2;
			continue;
		}

		c_strlcpy (key, com_token);
		if (state == STATE_IGNORE_NEG_1) {
			//if (COM_Parse_Basic(&datasrc) == false) break; // Key
			if (COM_Parse_Basic(&datasrc) == false) break; // value
			continue;
		} // UNWANTED CLASSNAME ENTITY


		if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
		c_strlcpy (value, com_token);

		Con_PrintLinef (QUOTED_S ": Read " QUOTED_S, cur && cur->targetname ? cur->targetname : "(ignored)", key, " = " QUOTED_S, value);

		if (state == STATE_CHECKCLASS_0) {
			// Copy datasrc over, skipping a prefix of '_' in a keyname
			if (String_Match (key, "classname") && String_Isin2 (value, NFO_info_decal_start, NFO_info_decal_point)) {

				Con_PrintLinef ("Found decal point");

				// Alloc and set struct
				info_decal_s *old = cur;
				cur = (info_decal_s *)Mem_Alloc(dbpool, sizeof(info_decal_s) );
				cur->classname = (char *)Mem_strdup(dbpool, value);

				if (old)
					old->next = cur;
				if (!head)
					head = cur;

				// We do not do defaults EVER.  We resolve during.
				state = STATE_WANTED_1;
			} else {
				state = STATE_IGNORE_NEG_1;
				Con_PrintLinef ("Ignoring %s = %s", key, value);
			}
			continue;
		}
		else if (state  == STATE_WANTED_1) {
			if (String_Match (key, "origin"))				{	Math_atov (value, cur->origin);
			} else if (String_Match (key, "targetname"))	{	cur->targetname = (char *)Mem_strdup(dbpool, value);
			} else if (String_Match (key, "target"))		{	cur->target = (char *) Mem_strdup(dbpool, value);

			} else if (String_Match (key, "drop"))			{	cur->drop = atof(value); // d: 32
			} else if (String_Match (key, "force_z"))		{	cur->force_z = atof(value);
			} else if (String_Match (key, "jitter_x"))		{	cur->jitter_x = atof(value);
			} else if (String_Match (key, "jitter_y"))		{	cur->jitter_y = atof(value);
			} else if (String_Match (key, "surface_size"))	{	Math_atov (value, cur->surface_size); // d: 512 512
			} else if (String_Match (key, "step"))			{	cur->step = atof(value); // d: 384
			} else if (String_Match (key, "texture"))		{
				cur->texture = (char *)Mem_strdup(dbpool, value);
				if (String_Starts_With (cur->texture, "textures/")) {
					char *s_after = cur->texture + strlen ("textures/");
					c_strlcpy (	cur->texture, s_after); // Does colliding work?  Shall see.
				}

			}
			else {
				//Con_Printf (QUOTED_S ": Read unprocessed " QUOTED_S, cur->targetname, key, " = " QUOTED_S, value);
			}
		}
	} // While

	Con_PrintLinef ("Decals");
	for ( info_decal_s *d = head; d; d = d->next) {
		WARP_X_ (FindTarget)
		//{
		//"classname" "info_decal"
		//"targetname" "path_1"
		//"texture" "maps_sh/dirt_field"
		//"options" ""
		//"target" "pathend_1"
		//"drop" "32"
		//"origin" "13092 -4442 32"
		//}
		info_decal_s *a = d;
		Con_PrintLinef ("a->target: %s", a->target);

		if (String_Match (a->classname, NFO_info_decal_start) == false)
			continue;

		info_decal_s *b = NULL;
		if (a->target)
			b = FindTarget (head, a->target);
		else Con_PrintLinef ("End of target list");
		if (b) {
			//vec_t *pa = a->origin;
			//vec_t *pb = b->origin;
			vec3_t delta;		VectorSubtract (b->origin, a->origin, delta);
			float vlen = VectorLength (delta);
			float step = a->step ? a->step : 256 /*default step*/;
			float numsteps = ceil( vlen / step);
			float numstepsceil = ceil(numsteps);
			float divsteps = 1 / numsteps;
			Con_PrintVarVector3 (a->origin);
			Con_PrintVarVector3 (b->origin);
			Con_PrintVarVector3 (delta);
			Con_PrintVarFloat (vlen);
			Con_PrintVarFloat (step);
			Con_PrintVarFloat (numsteps);
			vec3_t stepamt;
			//VectorM (divsteps, delta, stepamt);
			VectorScale (delta, divsteps, stepamt);
			Con_PrintVarVector3 (stepamt);

			for (int j = 0; j <= numstepsceil; j ++) {
				idnum ++; // unique id
				Con_PrintVarInt (j);
				float pct = j / numstepsceil;

				if (j == numstepsceil)
					pct = 1;
				//float pctdist = pct * vlen;
				Con_PrintVarFloat (pct);
				//Con_PrintVarFloat (pctdist);

				vec3_t v;
				VectorCopy (a->origin, v);
				VectorMA (v, pct, delta, v);
				Con_PrintVarVector3 (v);
				// [TEXTURE]
				//String_Edit_Replace (x, [ORIGIN_X-256],
				// [ORIGIN_X-256]
				// [ORIGIN_X_0]
				// [ORIGIN_Y-256]
				// [ORIGIN_Y+256]
				// [ORIGIN_Y_0]

				char decal_target[1024];
				c_dpsnprintf1 (decal_target, "targo%d", (int)idnum);

				size_t dekeout_size = strlen(s_deke) + ONE_CHAR_1 + 1000;
				char *dekeout_za = (char *)Mem_Alloc (dbpool, dekeout_size);
				vec3_t vjitter = {0,0,0};
				strlcpy (dekeout_za, s_deke, dekeout_size);
				String_Edit_Replace (dekeout_za, dekeout_size, "[DECALTARGET]", decal_target);
				String_Edit_Replace (dekeout_za, dekeout_size, "[TEXTURE]", a->texture);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_X-256]",		vjitter[0] + v[0] - (a->surface_size[0] / 2.0) );
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_X_0]",		vjitter[0] + v[0] + 0.0);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_X+256]",		vjitter[0] + v[0] + (a->surface_size[0] / 2.0) );
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Y-256]",		vjitter[1] + v[1] - (a->surface_size[1] / 2.0) );
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Y_0]",		vjitter[1] + v[1] + 0.0);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Y+256]",		vjitter[1] + v[1] + (a->surface_size[1] / 2.0));
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Z_0]",		vjitter[2] + v[2] + 0.0); // Decal point
				BakerString_CatC (bs_decals, dekeout_za);
				Mem_FreeNull_ (dekeout_za);

					//{
					//"classname" "info_null"
					//"targetname" "t1_4"
					//"origin" "11780 -2949 -35"
					//}

				ccs *starg = "\t\t\t\t\t{\r\n\t\t\t\t\t\"classname\" \"info_null\"\r\n\t\t\t"
					"\t\t\"targetname\" \"[DECALTARGET]\"\r\n\t\t\t\t\t\"origin\" \"[ORIGIN_X_0] "
					"[ORIGIN_Y_0] [ORIGIN_Z_0]\"\r\n\t\t\t\t\t}\r\n";

				dekeout_size = strlen(starg) + ONE_CHAR_1 + 1000;
				dekeout_za = (char *)Mem_Alloc (dbpool, dekeout_size);
				strlcpy (dekeout_za, starg, dekeout_size);
				String_Edit_Replace (dekeout_za, dekeout_size, "[DECALTARGET]", decal_target);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_X_0]", vjitter[0] + v[0] + 0.0);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Y_0]", vjitter[1] + v[1] + 0.0);
				String_Edit_Replace_Float (dekeout_za, dekeout_size, "[ORIGIN_Z_0]", a->force_z ?
					a->force_z : (vjitter[2] + v[2] - a->drop )
					  );

				BakerString_CatC (bs_decals, dekeout_za);
				Mem_FreeNull_ (dekeout_za);

				// Find { (newline) twice.  The 2nd one we insert this before.
				ccs *sx = "{\n"
"( [BOX_X+8] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X+8] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X+8] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 0 -1 -0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+8] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X+8] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+8] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X+8] [BOX_Y+8] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+8] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X+8] [BOX_Y-8] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+8] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X+8] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"{\n"
"( [BOX_X-8] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X-8] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X-8] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ -0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-8] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X-8] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-8] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X-8] [BOX_Y-8] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-8] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X-8] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-8] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X-8] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"{\n"
"( [BOX_X-4] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z-8] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z+8] ) common/caulk [ -1 -0 -0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"{\n"
"( [BOX_X+4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) common/caulk [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) common/caulk [ 1 -0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+8] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+8] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+8] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"{\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z-4] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z-4] ) common/caulk [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y+4] [BOX_Z-4] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z-4] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+4] [BOX_Z-4] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z-4] ) common/caulk [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y-4] [BOX_Z-4] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z-4] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+4] [BOX_Z-8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z-8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z-8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z-4] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z-4] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z-4] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"{\n"
"( [BOX_X+4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z+4] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z+8] ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z+4] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) common/caulk [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X+4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z+4] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z+4] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z+8] ) common/caulk [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y-4] [BOX_Z+8] ) ( [BOX_X-4] [BOX_Y+4] [BOX_Z+8] ) ( [BOX_X+4] [BOX_Y-4] [BOX_Z+8] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"( [BOX_X-4] [BOX_Y+4] [BOX_Z+4] ) ( [BOX_X-4] [BOX_Y-4] [BOX_Z+4] ) ( [BOX_X+4] [BOX_Y+4] [BOX_Z+4] ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 0.5 0.5 0 160 0\n"
"}\n"
"";
			}
			//vec3_t pmid = {0};
			//
			//VectorM (scale1,
			//ccs *texturename = a->texture;

			//float vlen =
			//vec3_t step =
		} else {
			Con_PrintLinef ("Found it");
		}
		//for (info_decal_s *e = d; e; e = e->next) {

		//}
		Con_PrintLinef ("Decal origin " VECTOR3_5d1F, VECTOR3_SEND( d->origin));
	}

exitor:

	Mem_EmptyPool (dbpool);
	if (head /*bs_decals->length*/) {
		return bs_decals;
	} else {
		return NULL;
	}
}

// "mapversion" "220"
// ( 480 704 -260 ) ( 480 672 -260 ) ( 480 704 -132 ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0
typedef struct _brush_entry_st {
	vec3_t a;				// ( 480 704 -260 )
	vec3_t b;				//( 480 672 -260 )
	vec3_t c;				// ( 480 704 -132 )
	char *texture;	// common/caulk
	vec4_t xtra1;		// [ 0 1 0 0 ]
	vec4_t xtra2;		// [ 0 0 -1 0 ]
	float ftrail[6];	// -0 1 1 0 160 0
	struct _brush_entry_st *next;
} brush_entry_s;



char *BrushFacer_ZA (ccs *datasrc) // Returns a (baker_string_t *) or NULL if no data
{
	//Con_PrintLinef ("Strlen is %f", (double)strlen(s_z) );
	ccs *text = datasrc;

	mempool_t *dbpool = Mem_AllocPool("Temp CSG Pool", /*flags*/ 0, /*parent*/ NULL);

	//Clipboard_Set_Text (s_z);
	//char		value[4096];
	//char		key[128];

	//baker_string_t *bs_decals = BakerString_Create_Malloc ("");
	//BakerString_CatC (bs_decals, "// This line" NEWLINE);
	//BakerString_CatC (bs_decals, text);
	// Read some data ...
//	if (COM_Parse_Basic(&datasrc) == false || com_token[0] != '{')	// Opening brace is start of worldspawn
//		goto exitor;
	stringlist_t blist = {0};
	voidlist_s vstartslist = {0};
	voidlist_s vbeyondslist = {0};

	//info_decal_s *cur = NULL;
	brush_entry_s *head = NULL;
	brush_entry_s *cur = NULL;

	int state = 0;
	int edictnum = 0;
	int depth = 0;
	int in_patch = 0;

	int idnum = 0;

	// Plan ... find all "classname" "info_decal"
	// Need to struct allocate making a chain.
	#define STATE_IGNORE_NEG_1	-1
	#define STATE_CHECKCLASS_0   0
	#define STATE_WANTED_1		 1
	#define STATE_BRUSHDATA_2	 2

	int iters = 0;

//{ // -> 1
//"classname" "worldspawn"
//"mapversion" "220"
//"_generator" "J.A.C.K. 1.1.3773 Freeware (vpQuake3)"
//{ // -> 2
//( 480 704 -260 ) ( 480 672 -260 ) ( 480 704 -132 ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0
//( 480 672 -260 ) ( 448 672 -260 ) ( 480 672 -132 ) common/caulk [ 1 0 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0
//( 480 704 -260 ) ( 448 672 -260 ) ( 480 672 -260 ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] -0 1 1 0 160 0
//( 480 704 -132 ) ( 480 672 -132 ) ( 448 672 -132 ) common/caulk [ 1 0 0 0 ] [ 0 -1 0 0 ] -0 1 1 0 160 0
//( 448 672 -260 ) ( 480 704 -260 ) ( 448 672 -132 ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0
//} // -> 1
	ccs *s_start_of_worldspawn_brushes = NULL; // at ( 480 704 -260 ) ( 480 672 -260 ) ( 480 704 -132 ) common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0
												// after the 2nd "{"
	ccs *s_last_brush_end = NULL; // after "}"
	ccs *s_end = NULL;
	int did_depth_2 = false;
	int is_ok = false;
	int total_brushes = 0;
	int up_brushes = 0;
	while (1) {
		iters ++;
		// Read some datasrc ...
		if (COM_Parse_Basic(&datasrc) == false)
			break; // End of string data

		if (com_token[0] == '{') {
			depth ++; // to 1
			if (depth == 2 && did_depth_2 == false) {
				// We are done reading brushes
				s_start_of_worldspawn_brushes = datasrc;
				did_depth_2 = true;
			}
		}
		else if (com_token[0] == '}') {
			depth --;
			if (depth == 1) {
				s_last_brush_end = datasrc;
				//break; // Test break point
			}
			if (depth == 0) {
				// We are done reading brushes
				is_ok = true;
				break;
			}
		} else if (depth == 2) {
			// Brush data
			Con_PrintLinef ("%s", com_token);

			// Allocate
			// Alloc and set struct
			brush_entry_s *old = cur;
			cur = (brush_entry_s *)Mem_Alloc(dbpool, sizeof(brush_entry_s) );
			//cur->classname = (char *)Mem_strdup(dbpool, value);

			if (old)
				old->next = cur;
			if (!head)
				head = cur;

//(
//			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data ALREADY READ THIS ONE
			if (com_token[0] != '(') break;
			ccs *s_start_line = datasrc; // Typically on a space before a number
			if (s_start_line[0] == SPACE_CHAR_32)
				s_start_line = &s_start_line[1]; // Advance to next character.

//480
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->a[0] = atof(com_token);
//704
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->a[1] = atof(com_token);
//-260
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->a[2] = atof(com_token);
//)
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != ')') break;
//(
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != '(') break;
//480
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->b[0] = atof(com_token);
//672
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->b[1] = atof(com_token);
//-260
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->b[2] = atof(com_token);
//)
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != ')') break;
//(
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != '(') break;
//480
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->c[0] = atof(com_token);
//704
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->c[1] = atof(com_token);
//-132
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->c[2] = atof(com_token);
//)
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != ')') break;
//common/caulk
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->texture = (char *) Mem_strdup(dbpool, com_token);
//[
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != '[') break;
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra1[0] = atof(com_token);
//1
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra1[1] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra1[2] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra1[3] = atof(com_token);
//]
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != ']') break;
//[
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != '[') break;
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra2[0] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra2[1] = atof(com_token);
//-1
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra2[2] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->xtra2[3] = atof(com_token);
//]
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			if (com_token[0] != ']') break;
//-0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[0] = atof(com_token);
//1
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[1] = atof(com_token);
//1
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[2] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[3] = atof(com_token);
//160
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[4] = atof(com_token);
//0
			if (COM_Parse_Basic(&datasrc) == false) break; // End of string data
			cur->ftrail[5] = atof(com_token);

			vec3_t pba; VectorSubtract (cur->b, cur->a, pba);
			vec3_t pca; VectorSubtract (cur->c, cur->a, pca);
			//vec3_t pn; VectorMultiply (pab, pac, pn);
			vec3_t cross; CrossProduct (pba, pca, cross);
			//Con_PrintVarVector3 (pba);
			//Con_PrintVarVector3 (pca);
			//Con_PrintVarVector3 (cross);
			int is_up_brush = false;
			if (cross[2] < 0) {
				// This one is facing up.
				is_up_brush = true;
				up_brushes ++;

				Mem_FreeNull_ (cur->texture);
				cur->texture = (char *) Mem_strdup(dbpool, "textures/up");
			}

			total_brushes ++;
			char *sout = Mem_strdupf (dbpool, /*"("*/ VECTOR3_LOSSLESS  ")" "(" VECTOR3_LOSSLESS  ")" "(" VECTOR3_LOSSLESS  ") "
				" %s "
				"[" VECTOR4_LOSSLESS "] "
				"[" VECTOR4_LOSSLESS "] "
				 VECTOR6_LOSSLESS,
				 VECTOR3_SEND(cur->a),
				 VECTOR3_SEND(cur->b),
				 VECTOR3_SEND(cur->c),
				 cur->texture,
				 VECTOR4_SEND(cur->xtra1),
				 VECTOR4_SEND(cur->xtra2),
				 VECTOR6_SEND(cur->ftrail)
				 );
			Con_PrintLinef ("%s", sout);
//			int j = 5;
			//if (
			if (is_up_brush) {
				ccs *s_end_line = datasrc; // It's on a carriage return (13) before a newline (10)
				stringlistappend (&blist, sout);
				voids_add1 (&vstartslist, s_start_line);
				voids_add1 (&vbeyondslist, s_end_line);
			}


		} // if depth = 2




	} // while

	char *s_za = NULL;

	if (is_ok) {
		Con_PrintLinef ("Parsed ok");
		Con_PrintVarInt (total_brushes);
		Con_PrintVarInt (up_brushes);
	} else {
		Con_PrintLinef ("Parse error");
		goto exitor;
	}

	s_za = Z_StrDup (text);
	// Replace each of these in reverse.
	for (int idx = blist.numstrings - 1; idx >= 0; idx --) {
		char *sxy = blist.strings[idx];
		//String_Edit_Replace_Memory_Constant
		String_Replace_Proxy_ZWork (&s_za, text, sxy, (ccs *)vstartslist.vloats[idx], (ccs *)vbeyondslist.vloats[idx]);
	} // for

	stringlistfreecontents (&blist);
	voids_freecontents (&vstartslist);
	voids_freecontents (&vbeyondslist);

exitor:

	Mem_EmptyPool (dbpool);
	return s_za;
}


#include "zeq.c.h"

#ifdef CONFIG_MENU
WARP_X_ (FS_Init_Dir)
void SCR_brushfacer2_clipboard_f (cmd_state_t *cmd)
{
	baker_string_t *bsa = NULL;
	char *clipboard_za = Sys_GetClipboardData_Unlimited_ZAlloc();
	int slen = (int)strlen(clipboard_za);
	Con_PrintLinef ("strlen %d", slen);
	char *s2 = String_Replace_Malloc (clipboard_za, CARRIAGE_RETURN, "");
	int slen2 = (int)strlen(s2);
	Con_PrintLinef ("strlen2 %d", slen);

	entitylist_t list_map1 = {0};	// aafter.map
	int isok = entitylist_parsemaptxt (&list_map1, s2);
	if (isok == false) {
		Con_PrintLinef ("entitylist_parsemaptxt failed");
		goto exitor; // failed to parse
	}
	
	int num_faces = 0;
	int num_upfaces = entitylist_brush0_facer (&list_map1, &num_faces);
	// Now what?

	Con_PrintLinef ("Num faces %d, num up faces %d", num_faces, num_upfaces);
	
	bsa = entitylist_maptext_bsalloc (&list_map1);
	if (!bsa) {
		Con_PrintLinef ("Unable to make string"); // error processing the map for some reason.
		goto exitor;
	}

	// Write
	Clipboard_Set_Text (bsa->string);
	Con_PrintLinef ("Set to clipboard strlen %d", (int)bsa->length);

exitor:
	BakerString_Destroy_And_Null_It (&bsa);
	entitylistfreecontents (&list_map1);
	
	Mem_FreeNull_ (clipboard_za);
	freenull_ (s2);
}


void SCR_brushfacer_clipboard_f (cmd_state_t *cmd)
{
	ccs *data_za = Sys_GetClipboardData_Unlimited_ZAlloc();

	char *s_za = BrushFacer_ZA(data_za);

	if (s_za) {
		Clipboard_Set_Text (s_za);
		Con_PrintLinef ("Clipboard set strlen = %d", strlen(s_za));

	} else {
		Con_PrintLinef ("NO DATA FOR CLIPBOARD");
	}

	Mem_FreeNull_ (s_za);
	Mem_FreeNull_ (data_za);
}

void SCR_gifclip_f (cmd_state_t *cmd)
{


	if (Cmd_Argc(cmd) == 1) {
		Con_PrintLinef ("Usage: %s [gif] [optional framenum] decode gif and copy to clipboard", Cmd_Argv(cmd, 0) );
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
#endif // CONFIG_MENU


#if 0

	ccs *stemplate =
"//	mtllib unused.mtl"					NEWLINE
"g"										NEWLINE
""										NEWLINE
"v -[$ABSMIN_X] -[$ABSMIN_Y]  [$ABSMAX_Z]"				NEWLINE
"v  [$ABSMAX_X] -[$ABSMIN_Y]  [$ABSMAX_Z]"				NEWLINE
"v -[$ABSMIN_X]  [$ABSMAX_Y]  [$ABSMAX_Z]"				NEWLINE
"v  [$ABSMAX_X]  [$ABSMAX_Y]  [$ABSMAX_Z]"				NEWLINE
"v -[$ABSMIN_X]  [$ABSMAX_Y] -[$ABSMIN_Z]"				NEWLINE
"v  [$ABSMAX_X]  [$ABSMAX_Y] -[$ABSMIN_Z]"				NEWLINE
"v -[$ABSMIN_X] -[$ABSMIN_Y] -[$ABSMIN_Z]"				NEWLINE
"v  [$ABSMAX_X] -[$ABSMIN_Y] -[$ABSMIN_Z]"				NEWLINE
//String_Edit_Replace (sout, sizeof(sout), [$ABSMIN_X], replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);
//String_Edit_Replace (sout, sizeof(sout), sf, replace);

""										NEWLINE
"vt 1.000000 1.000000"					NEWLINE
"vt 1.000000 0.000000"					NEWLINE
"vt 0.000000 1.000000"					NEWLINE
"vt 0.000000 0.000000"					NEWLINE
""										NEWLINE
"vn  0.000000  0.000000  1.000000"		NEWLINE
"vn  0.000000  1.000000  0.000000"		NEWLINE
"vn  0.000000  0.000000 -1.000000"		NEWLINE
"vn  0.000000 -1.000000  0.000000"		NEWLINE
"vn  1.000000  0.000000  0.000000"		NEWLINE
"vn -1.000000  0.000000  0.000000"		NEWLINE
""										NEWLINE
"g [$P0]"								NEWLINE
"usemtl [$T0]"							NEWLINE
"f [$F1]/1/1 [$F2]/2/1 [$F3]/3/1"		NEWLINE
"f [$F3]/3/1 [$F2]/2/1 [$F4]/4/1"		NEWLINE
""										NEWLINE
"g [$P1]"								NEWLINE
"usemtl [$T1]"							NEWLINE
"f [$F3]/1/2 [$F4]/2/2 [$F5]/3/2"		NEWLINE
"f [$F5]/3/2 [$F4]/2/2 [$F6]/4/2"		NEWLINE
""										NEWLINE


"g [$P2]"								NEWLINE
"usemtl [$T2]"							NEWLINE
"f [$F5]/4/3 [$F6]/3/3 [$F7]/2/3]"		NEWLINE
"f [$F7]/2/3 [$F6]/3/3 [$F8]/1/3]"		NEWLINE
""										NEWLINE
"g [$P3]"								NEWLINE
"usemtl [$T3]"							NEWLINE
"f [$F7]/1/4 [$F8]/2/4 [$F1]/3/4"		NEWLINE
"f [$F1]/3/4 [$F8]/2/4 [$F2]/4/4"		NEWLINE
""										NEWLINE
"g [$P4]"								NEWLINE
"usemtl [$T4]"							NEWLINE
"f [$F2]/1/5 [$F8]/2/5 [$F4]/3/5"		NEWLINE
"f [$F4]/3/5 [$F8]/2/5 [$F6]/4/5"		NEWLINE
""										NEWLINE
"g [$P5]"								NEWLINE
"usemtl [$T5]"							NEWLINE
"f [$F7]/1/6 [$F1]/2/6 [$F5]/3/6"		NEWLINE
"f [$F5]/3/6 [$F1]/2/6 [$F3]/4/6"		NEWLINE;

#endif

// Noesis.exe ?cmode "pl_male01_hi.smd" "pl_male01_hi.rdm" -rotate 90 0 0
// -smdnorm -scale 2.0 -texpre "male01 tex_"

// Our goal is to make separate models from one input file.

void FS_BarMake_f (cmd_state_t *cmd)
{
	if (Cmd_Argc (cmd) < 4) {
		Con_PrintLinef ("Expected barmake <obj> <image> <replace_object_or_texture>");
		return;
	}
	// 100 bars of decreasing size, alpha 0 back.
	// 100 skins specifying [X] as the tag or texture
	ccs *objname = Cmd_Argv(cmd, 1);	// Image	gfx/hud/model.obj -> _0.skin
	ccs *image_tga_in = Cmd_Argv(cmd, 2);	// Image	gfx/hud/red.png -> gfx/hud/red_0.png .. gfx/hud/red_100.png
	ccs *tag = Cmd_Argv(cmd, 3);	// Skin File Thing to replace "
	char *image_no_ext_za = Z_StrDup (image_tga_in);
	File_URL_Edit_Remove_Extension (image_no_ext_za);
	//int w = ?;// Width of image
	bgra4 *pels_za = (bgra4 *)loadimagepixelsbgra (
		image_tga_in,
		q_tx_complain_false,
		q_tx_allowfixtrans_false,
		q_tx_convertsrgb_false,
		q_tx_miplevel_null
	);

	int w = image_width;
	int h = image_height;
	int pels_bytes = image_width * image_height * BGRA_4;
	if (pels_za == NULL) {
		Con_PrintLinef ("Couldn't open file " QUOTED_S, image_tga_in);
	}
	bgra4 *pelsout_za = NULL;

	for (int idx = 0; idx <= 100; idx ++) {
		Vid_SetWindowTitlef ("barmake %d/%d", idx, 100); // RESET

		float wpct = w * (idx/100.0);
		if (wpct) {
			pelsout_za = Image_Bilinear_Resize_ZAlloc (pels_za, w,h, wpct, h);
			bgra4 *pels_enlarge_za = (bgra4 *)Image_Enlarge_Canvas_ZAlloc (pelsout_za, wpct, h, BGRA_4, w, h, /*black alpha solid we hope*/ 0x0000000, /*is centered?*/ false);
			Mem_FreeNull_ (pelsout_za);
			pelsout_za = pels_enlarge_za; pels_enlarge_za = NULL;
		} else {
			// Black
			pelsout_za = (bgra4 *)Mem_Alloc (zonemempool, pels_bytes);
		}

		va_super (jpegfile, MAX_QPATH_128, "%s_%d.jpg", image_no_ext_za, idx);
		va_super (skinfile, MAX_QPATH_128, "%s_%d.skin", objname, idx);
		va_super (skintext, MAX_QPATH_128, "%s,%s", tag, jpegfile);

		int isok;
		isok = StringToFileConPrintIsOk (skinfile, skintext);
		if (!isok) goto fail_out;

		isok = Image_Save_JPEG_Is_Ok (jpegfile, pelsout_za, w, h);
		if (!isok) goto fail_out;

		Mem_FreeNull_ (pelsout_za);
	}

fail_out:

	Mem_FreeNull_ (pelsout_za);
	Mem_FreeNull_ (pels_za);
	Mem_FreeNull_ (image_no_ext_za);

	Vid_SetWindowTitlef (gamename); // RESET
}

#if 0 // Never finished, discovered a better way.
void FS_ObjMake_f (cmd_state_t *cmd)
{
// textures will be ? part
// objmake cubeoid.txt cubeoid/model.obj

//[x] [y] [z] [w] [h] [z]  t0 .. t5
//0   0    0   32  8   32

	if (Cmd_Argc(cmd) < 1 + 1) {
		Con_PrintLinef ("usage:" NEWLINE "%s <path/model.txt> builds obj as <path/model.obj>", Cmd_Argv(cmd, 0));
		return;
	}

	ccs *txtfilename = Cmd_Argv(cmd, 1);
	stringlist_t lines_list = {0};
	stringlist_t lines_list_out = {0};
	fs_offset_t filesize; char *txtfile_za = (char *)FS_LoadFile (txtfilename, tempmempool, fs_quiet_true, &filesize);

	if (!txtfile_za) {
		Con_PrintLinef ("Could not open " QUOTED_S " for reading");
		goto fail_out;
	}

	stringlistappend_split_lines_cr_scrub (&lines_list, txtfile_za);

	for (int linenum = 0; linenum < lines_list.numstrings; linenum ++) {
		char *s_this_line = lines_list.strings[linenum];
		String_Edit_Trim (s_this_line);

		stringlist_t lines_list = {0};
		if (String_Starts_With (s_this_line, "//") || s_this_line[0] == 0)
			continue; // Comment or empty line

		tokenize_console_s tcm = {0}, *tcx = &tcm;
		int argc = Tokenize_Console_16384_Za_Return_Argc (tcx, s_this_line); // Clears tcx first

//[x] [y] [z] [w] [h] [d]  t0 .. t5
//0   0    0   32  8   32
		//char sout[65536]; //

		ccs *s_0 = tcx->tokens_za[0];
		ccs *x_1 = tcx->tokens_za[1];
		ccs *y_2 = tcx->tokens_za[2];
		ccs *z_3 = tcx->tokens_za[3];
		ccs *w_4 = tcx->tokens_za[4];
		ccs *h_5 = tcx->tokens_za[5];
		ccs *d_6 = tcx->tokens_za[6];

		if (argc < 7) {
			Con_PrintLinef ("Line %d < 7 has too few arguments " QUOTED_S, linenum, s_this_line);
			goto bad_line_skip;
		}


		float x = atof(x_1); // Is this an origin?
		float y = atof(y_2);
		float z = atof(z_3);
		float w = atof(w_4);
		float h = atof(h_5);
		float d = atof(d_6);

//		vec3_t absmins = { x - w *.50

		//stringlistappendf (&lines_list_out, "%s", "thisline");
		//char

//		stringlistappend_split_lines_cr_scrub (&lines_list_out, txtline_za);
		Mem_FreeNull_ (txtfile_za);  // Contents of .txt file

bad_line_skip:
		Tokenize_Console_16384_FreeContents (tcx);
	} // for

	//
	// WRITE
	//

	char s_filenameout[MAX_QPATHX2_256];

	c_strlcpy						(s_filenameout, txtfilename);
	File_URL_Edit_Remove_Extension	(s_filenameout);
	c_strlcat						(s_filenameout, ".obj");

	Con_PrintLinef ("Writing to " QUOTED_S, s_filenameout);
	qfile_t *f = FS_OpenRealFile (s_filenameout, "wb", fs_quiet_FALSE); // WRITE-EON obj model adjust
	if (!f) {
		Con_PrintLinef ("Couldn't open file " QUOTED_S, s_filenameout);
		goto fail_out;
	}

	FS_PrintLinef (f, "// # Zircon .obj export");

	for (int linenum = 0; linenum < lines_list_out.numstrings; linenum ++) {
		char *s_this_line = lines_list_out.strings[linenum];
		FS_PrintLinef (f, "%s", s_this_line);
	} // for

	FS_CloseNULL_ (f);


fail_out:
	stringlistfreecontents (&lines_list); // Lines
	stringlistfreecontents (&lines_list_out); // Lines
	Mem_FreeNull_ (txtfile_za);  // Contents of .txt file
}
#endif

void FS_GifToShader_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) < 1 + 1) {
		Con_PrintLinef ("usage:" NEWLINE "%s <animated.gif> converts to tga and makes shader text", Cmd_Argv(cmd, 0));
		return;
	}

	const char	*s_filename = Cmd_Argv(cmd, 1);

	if (String_Ends_With (s_filename, ".gif") == false) {
		Con_PrintLinef ("filename does not end with .gif");
		return;
	}

	char			*s_realpath_alloc	= NULL;
	byte			*filedata_alloc		= NULL;
	byte			*frame_rgb3_alloc	= NULL;
	gd_GIF_t		*gif				= NULL;
	baker_string_t	*bstextures			= NULL;

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

	gif = (gd_GIF_t *)gd_open_gif_from_memory_alloc (filedata_alloc, filesize);

	int numpels			= gif->width * gif->height;
	int numpelbytes3	= numpels * RGB_3;
	int numpelbytes4	= numpels * RGBA_4;
	frame_rgb3_alloc	= Mem_TempAlloc_Bytes (numpelbytes3);

    Con_PrintLinef ("GIF %dx%d %d colors", gif->width, gif->height, gif->palette->size);

	int numframes = 0;

	char s_name_noext[MAX_QPATH_128];
	c_strlcpy (s_name_noext, s_filename);
	File_URL_Edit_Remove_Extension(s_name_noext);

	bstextures = BakerString_Create_Malloc ("");

		// Background color

		byte *color = &gif->gct.colors[gif->bgindex * RGB_3];

		for (int idx = 0; idx < numpelbytes3; idx += RGB_3) {
			frame_rgb3_alloc[idx + 0] = color[0];
			frame_rgb3_alloc[idx + 1] = color[1];
			frame_rgb3_alloc[idx + 2] = color[2];
		}


	//is_ok = false;
	while (1) {
		int ret = gd_get_frame(gif, /*shall process?*/ true);

		if (ret <= 0)
			break;

		Con_PrintLinef ("Frame %4d: delay is %d centiseconds", numframes, (int)gif->gce.delay_100);

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

		va_super (snameout, MAX_QPATH_128, "%s_%d.tga", s_name_noext, numframes);

		// Now turn to tga and write it.
		WARP_X_ (FS_JpegSplit_f)

#if 1
		unsigned char *jpegbuf2_3_alloc  = (unsigned char *)Mem_Alloc(tempmempool, gif->width * gif->height * RGB_3);

		size_t jpeg_bytes_maxsize = gif->width * gif->height * RGB_3;
		//unsigned char *jpeg_bytes_out_alloc = (unsigned char *)Mem_Alloc(tempmempool, jpeg_bytes_maxsize);

		qbool flipx_false = false;
		qbool flipy_true = true;
		qbool flipdiagonal_false = false;

		int	indices[4] = {0,1,2,3}; // BGRA
		Image_CopyMux (jpegbuf2_3_alloc, frame_rgb4_alloc, gif->width, gif->height, flipx_false, flipy_true,
			flipdiagonal_false, /*numoutput component*/ RGB_3, /*num input coponents*/ RGBA_4, indices);

		ret = Image_WriteTGABGR_preflipped (snameout, gif->width, gif->height, jpegbuf2_3_alloc);

		if (ret == false) {
			Con_PrintLinef ("Couldn't write " QUOTED_S, snameout);

		}

		Mem_Free (jpegbuf2_3_alloc);

		// Write ...
#endif
		if (numframes > 0)
			BakerString_Cat_No_Collide (bstextures, ONE_CHAR_1, " ");
		BakerString_Cat_No_Collide (bstextures, strlen(snameout), snameout);
		Con_PrintLinef ("Wrote %s", snameout);
		Mem_FreeNull_ (frame_rgb4_alloc);

		numframes ++;
	} // while

	if (is_ok == false) {
        Con_PrintLinef ("Could not get frame");
		is_ok = false; goto exitor;
	}

	Con_PrintLinef ("Total frames = %d", numframes);

	ccs *s = "[TEXTURE_NO_EXT]" NEWLINE
			"{" NEWLINE
			"	qer_editorimage [TEXTURE_0_TGA]" NEWLINE
			"	{" NEWLINE
//			"		map [TEXTURE_0_TGA]" NEWLINE
			"		// Frequency: The number of times that the animation cycle will repeat within " NEWLINE
			"		// a one second time period." NEWLINE
			"		// The larger the value, the more repeats within a second. " NEWLINE
			"		animmap [FREQUENCY] [TEXTURES]" NEWLINE
			"	}" NEWLINE
			"	{" NEWLINE
			"		map $lightmap" NEWLINE
			"		tcGen lightmap" NEWLINE
			"		blendfunc filter" NEWLINE
			"	}" NEWLINE
			"" NEWLINE
			"}" NEWLINE;

	// The number of times that the animation cycle will repeat within a one second time period.
	// The larger the value, the more repeats within a second.
	//float frequency = 1.0 / (float)total_seconds;
	float frequency = 1.0 / (float)total_seconds;

	char *s_shader_text_a = (char *)Mem_Alloc (tempmempool, 16384);
	strlcpy (s_shader_text_a, s, 16384);

	va_super (s_tga0, MAX_QPATH_128, "%s_%d.tga", s_name_noext, 0);
	va_super (s_frequency, 1024, FLOAT_LOSSLESS_FORMAT, frequency);

	String_Edit_Replace (s_shader_text_a, 16384, "[TEXTURE_NO_EXT]", s_name_noext);
	String_Edit_Replace (s_shader_text_a, 16384, "[TEXTURE_0_TGA]", s_tga0);
	String_Edit_Replace (s_shader_text_a, 16384, "[FREQUENCY]", "1"); // s_frequency
	String_Edit_Replace (s_shader_text_a, 16384, "[TEXTURES]", bstextures->string);

	Clipboard_Set_Text (s_shader_text_a);
	Con_PrintLinef ("Shader text copied to clipboard");

//	char *s_tga = Z_StrDup (s_filename);


	Mem_FreeNull_ (s_shader_text_a);

exitor:
    gif = (gd_GIF_t *)gd_close_gif_maybe_returns_null (gif);
	Mem_FreeNull_ (frame_rgb3_alloc);
	Mem_FreeNull_ (s_realpath_alloc);
	Mem_FreeNull_ (filedata_alloc);

	BakerString_Destroy_And_Null_It (&bstextures);
}

