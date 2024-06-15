// baker_gif_decode.c.h

// QUESTIONS:  Can we figure out the length of the time of the gif
// in milliseconds.


WARP_X_ (OpenStream dpvsimpledecode_open gd_open_gif_alloc)

//video->stream = dpvsimpledecode_open( video, video->filename, &errorstring);

// opens a stream

WARP_X_ (dpvsimpledecodestream_t)

typedef struct gifstream_s
{
	void		*giffy;
	int			errorcode_int;

	int			gif_framenum; // Set to 0 on start, maybe -1 on end?

	int			info_totalframes;
	double		info_totalsecondsofvideo;

	int			info_imagewidth;
	int			info_imageheight;
	int			info_framerate; // frames per second
	double		info_aspectratio;

	int			oldframe;
	int			oldframe_cents_start;
	int			oldframe_cents_end;
} gifstream_t;

WARP_X_ (SCR_gifclip_f dpvsimpledecode_open)


WARP_X_ (dpvsimpledecode_getwidth)

// returns the width of the image data
unsigned int bakergifdecode_getwidth(void *stream)
{
	gifstream_t *s = (gifstream_t *)stream;
	return s->info_imagewidth;
}


WARP_X_ (dpvsimpledecode_getheight)
// returns the height of the image data
unsigned int bakergifdecode_getheight(void *stream)
{
	gifstream_t *s = (gifstream_t *)stream;
	return s->info_imageheight;
}


WARP_X_ (dpvsimpledecode_getframerate)

// returns the framerate of the stream
double bakergifdecode_getframerate(void *stream)
{
	return 100.0; // We always say 100 fps for now.
}

// return aspect ratio of the stream
WARP_X_ (dpvsimpledecode_getaspectratio)
double bakergifdecode_getaspectratio (void *stream)
{
	gifstream_t *s = (gifstream_t *)stream;
	return s->info_aspectratio;
}

// decodes a video frame to the supplied output pixels
WARP_X_ (dpvsimpledecode_video gd_get_frame SCR_gifclip_f video->decodeframe CL_Video_Frame)

// Baker: Note: We seem to be ignoring rowbytes here.

static void _process_to_buffer (clvideo_t *video, gifstream_t *s, byte *imagedata)
{
	byte *vimageub = (byte *)imagedata;
	int numpels = video->width * video->height;
	int numpelbytes3 = numpels * RGB_3;

	gd_GIF_t *gifster = (gd_GIF_t *)s->giffy;
	byte *color = &gifster->gct.colors[gifster->bgindex * RGB_3];
	byte		*frame_rgb3_alloc	= Mem_TempAlloc_Bytes (numpelbytes3);

	for (int idx = 0; idx < numpelbytes3; idx += RGB_3) {
		frame_rgb3_alloc[idx + 0] = color[0];
		frame_rgb3_alloc[idx + 1] = color[1];
		frame_rgb3_alloc[idx + 2] = color[2];
	}

	gd_render_frame (s->giffy, frame_rgb3_alloc);

	for (int idx = 0; idx < numpels; idx ++) {
		// Do we need to flip 0 and 2 color index? YES
		vimageub[idx * RGBA_4 + 0] = frame_rgb3_alloc[idx * RGB_3 + 2];		
		vimageub[idx * RGBA_4 + 1] = frame_rgb3_alloc[idx * RGB_3 + 1];
		vimageub[idx * RGBA_4 + 2] = frame_rgb3_alloc[idx * RGB_3 + 0];
		vimageub[idx * RGBA_4 + 3] = 255; // alpha
	}

	Mem_FreeNull_ (frame_rgb3_alloc);
}

// Baker: We are coming every 100th of second
// If we flip, set did_flip
int bakergifdecode_videoframe (clvideo_t *video, void *stream, void *imagedata, 
	unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, 
	int	imagebytesperrow, int framenum_wanted, double time_wanted, int *pframe_returned)
{
	gifstream_t *s = (gifstream_t *)stream;
	int centiseconds_wanted = (int)(time_wanted * 100); // floor

	int cur_frame		= framenum_wanted ? s->oldframe : -1;
	int cur_frame_start	= framenum_wanted ? s->oldframe_cents_start: 0;
	int cur_frame_end	= framenum_wanted ? s->oldframe_cents_end : 0;

	if (cur_frame == -1)
		s->gif_framenum = -1;

	if (s->gif_framenum != s->oldframe) {
		// Baker: What is diff?
		// int j = 5;
	}

	if (centiseconds_wanted < cur_frame_end) {
		// Baker: This should not ever happen .. and seems to not.
		return (s->errorcode_int = DPVSIMPLEDECODEERROR_NONE_0);
	}

	// Read each frame advancing time until we get the frame we want
	while (1) {
		s->gif_framenum ++; // -1 to 0
		int ret = gd_get_frame (s->giffy, /*shall render to buffer?*/ true);

		cur_frame_start = cur_frame_end;
		cur_frame_end += ((gd_GIF_t *)s->giffy)-> gce.delay_100;

		if (ret <= 0) {
			// Baker: End of frames.  Just return EOF .. that's it.
			return (s->errorcode_int = DPVSIMPLEDECODEERROR_EOF_1);
		}
		
		if (cur_frame_end < centiseconds_wanted)
			continue;

		s->oldframe = s->gif_framenum;
		s->oldframe_cents_start = cur_frame_start;
		s->oldframe_cents_end = cur_frame_end;

		// Found it
		video->framegif_endtime = (double)cur_frame_end / 100.0;
		*pframe_returned = s->gif_framenum;

		break;
	}

	_process_to_buffer (video, s, (byte *)imagedata);

	return (s->errorcode_int = DPVSIMPLEDECODEERROR_NONE_0);
}

WARP_X_ (gd_close_gif_maybe_returns_null dpvsimpledecode_close)

void bakergifdecode_close (void *stream)
{
	gifstream_t *s = (gifstream_t *)stream;

	if (s == NULL)
		return;

	if (s->giffy)
		s->giffy = (gd_GIF_t *)gd_close_gif_maybe_returns_null (s->giffy);

	WARP_X_ (gifstream_t)

	Mem_FreeNull_ (s);
}

// For our purposes, we will need to get the number of frames.
// We then can calculate the number of milliseconds of the whole animation.
WARP_X_ (dpvsimpledecode_open OpenStream gd_open_gif_alloc bakergifdecode_videoframe SCR_gifclip_f)
void *bakergifdecode_open (clvideo_t *video, char *filename, const char **perrorstring)
{
	// If does not end with .gif, get out.
	if (false == String_Ends_With (filename, ".gif")) {
		return NULL;
	}

	fs_offset_t filesize;
	byte *filedata_alloc = FS_LoadFile (filename, tempmempool, fs_quiet_true, &filesize);

	// Baker: File not found, just get out
	if (filedata_alloc == NULL) {
		return NULL;
	}

	double total_seconds = 0;
	int total_frames = gif_get_length_numframes (filedata_alloc, filesize, &total_seconds);
	void *giffy_ma		= gd_open_gif_from_memory_alloc (filedata_alloc, filesize);

	Mem_FreeNull_ (filedata_alloc);

	if (!giffy_ma) {
		return NULL;
	}

	gifstream_t *s_a	= (gifstream_t *)Mem_ZMalloc_SizeOf(gifstream_t);
	
	int imagewidth		= gd_getwidth(giffy_ma);
	int imageheight		= gd_getheight(giffy_ma);

	//int numpels = imagewidth * imageheight * RGBA_4; // sizeof(*s_a->videopixels) /*4*/;

	s_a->giffy				= giffy_ma;
	s_a->info_imagewidth	= imagewidth;
	s_a->info_imageheight	= imageheight;
	s_a->info_framerate		= 0;
	s_a->info_aspectratio	= (double)s_a->info_imagewidth / (double)s_a->info_imageheight;

	s_a->info_totalframes = total_frames;
	s_a->info_totalsecondsofvideo = total_seconds;
	
	// set the interface
	video->close			= bakergifdecode_close;
	video->getwidth			= bakergifdecode_getwidth;
	video->getheight		= bakergifdecode_getheight;
	video->getframerate		= bakergifdecode_getframerate;
	video->decodeframe		= NULL;
	video->decodeframegif	= bakergifdecode_videoframe;
	video->getaspectratio	= bakergifdecode_getaspectratio;
	video->is_gif_stream	= true;

	if (developer_loading.value) {
		// 0.05 frames per second * 20 frames = 1 second
		Con_PrintLinef ("Total time is %f", s_a->info_totalsecondsofvideo);
	}


	return s_a;

exitor:

	// Error cleanup
	giffy_ma = gd_close_gif_maybe_returns_null(giffy_ma);
	Mem_FreeNull_ (s_a);

	return NULL;
}


