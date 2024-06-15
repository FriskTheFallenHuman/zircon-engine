// baker_jpeg_decode.c.h

// QUESTIONS:  Can we figure out the length of the time of the gif
// in milliseconds.

// image.h -->
unsigned char *JPEG_LoadImage_BGRA (const unsigned char *f, int filesize, int *miplevel);

extern int		image_width;
extern int		image_height;


WARP_X_ (OpenStream dpvsimpledecode_open gd_open_gif_alloc)

// opens a stream

WARP_X_ (dpvsimpledecodestream_t)

typedef struct jpegs_folder_stream_s
{
	int				errorcode_int;

	int				jpeg_framenum;
	
	int				info_imagewidth;
	int				info_imageheight;
	int				info_framerate; // frames per second
	double			info_aspectratio;

	stringlist_t	jpegnames_list;
	stringlist_t	jpegblob_list;
	int				*jpegblobsizes_alloc;

	int64_t			jpegs_total_disk_size;
} jpegs_folder_stream_t;

WARP_X_ (SCR_gifclip_f dpvsimpledecode_open)


// returns the width of the image data
WARP_X_ (dpvsimpledecode_getwidth)

unsigned int bakerjpegdecode_getwidth(void *stream)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;
	return s->info_imagewidth;
}

// returns the height of the image data
WARP_X_ (dpvsimpledecode_getheight)

unsigned int bakerjpegdecode_getheight(void *stream)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;
	return s->info_imageheight;
}

// returns the framerate of the stream
WARP_X_ (dpvsimpledecode_getframerate)

double bakerjpegdecode_getframerate(void *stream)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;
	return s->info_framerate; // We always say 100 fps for now.
}

// return aspect ratio of the stream
WARP_X_ (dpvsimpledecode_getaspectratio)
double bakerjpegdecode_getaspectratio (void *stream)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;
	return s->info_aspectratio;
}

// decodes a video frame to the supplied output pixels
WARP_X_ (dpvsimpledecode_video gd_get_frame SCR_gifclip_f video->decodeframe CL_Video_Frame)

// Baker: We are coming every 100th of second
// If we flip, set did_flip
int bakerjpegdecode_videoframe (void *stream, void *vid_vimagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;
	
	int framenum = s->jpeg_framenum;

	if (framenum >= s->jpegnames_list.numstrings) {
		return (s->errorcode_int = DPVSIMPLEDECODEERROR_EOF_1);
	}

	byte *jpeg_data = (byte *)s->jpegblob_list.strings[framenum];
	size_t jpeg_datasize = (size_t)s->jpegblobsizes_alloc[framenum];

	s->jpegs_total_disk_size += (fs_offset_t)jpeg_datasize;
	if (developer_loading.integer)
		Con_PrintLinef ("%4d: + " PRINTF_INT64 " -> " PRINTF_INT64, framenum, (int64_t)jpeg_datasize, (int64_t)s->jpegs_total_disk_size);

	// Load the first jpeg to get image_width/image_height globals that the DarkPlaces loader uses
	unsigned char *jpeg_bgra_data_zalloc = JPEG_LoadImage_BGRA (jpeg_data, jpeg_datasize, /*mipevel*/ NULL);

	memcpy (vid_vimagedata, jpeg_bgra_data_zalloc, s->info_imagewidth * s->info_imageheight * RGBA_4);

	Mem_FreeNull_ (jpeg_bgra_data_zalloc);
	
	s->jpeg_framenum ++;

	return (s->errorcode_int = DPVSIMPLEDECODEERROR_NONE_0);
}

WARP_X_ (gd_close_gif_maybe_returns_null dpvsimpledecode_close)

void bakerjpegdecode_close (void *stream)
{
	jpegs_folder_stream_t *s = (jpegs_folder_stream_t *)stream;

	if (s == NULL)
		return;

	if (developer_loading.integer) {
		Con_PrintLinef ("Size of all jpegs is %s", String_Num_To_Thousands_Sbuf(s->jpegs_total_disk_size));
	}

	stringlistfreecontents (&s->jpegnames_list);
	stringlistfreecontents (&s->jpegblob_list);
	Mem_FreeNull_ (s->jpegblobsizes_alloc);

	Mem_FreeNull_ (s);
}


// For our purposes, we will need to get the number of frames.
// We then can calculate the number of milliseconds of the whole animation.
WARP_X_ (bakergifdecode_open dpvsimpledecode_open OpenStream gd_open_gif_alloc bakerjpegdecode_videoframe SCR_gifclip_f)
void *bakerjpegdecode_open (clvideo_t *video, char *filename, const char **perrorstring)
{
	// Folder of jpegs like my_vid_20_fps

	// Not a jpeg folder name, get out
	if (false == String_Ends_With (filename, "_fps")) {
		return NULL;
	}

	// Example name: video/my_vid_20_fps
	// A folder containing files with names like ...
	//  ezgif-frame-001.jpg
	//  ezgif-frame-002.jpg
	//  ezgif-frame-003.jpg
	// 
	// We sort them by the number part of the name
	// We sort all of the jpg in the folder alphabetically by the substring.

	char snamepoke[MAX_QPATH_128];
	c_strlcpy (snamepoke, filename);
	int ofs = strlen(snamepoke) - STRINGLEN("_fps");
	snamepoke[ofs] = 0; // Remove the _fps

	char *whereo = strrchr (snamepoke, '_');
	if (whereo == NULL) {
		return NULL;
	}

	// my_vid_20_fps --> whereo is _20
	int frames_per_second = atoi(whereo + 1);
	if (frames_per_second <= 0) {
		return NULL;
	}

	char pattern[MAX_QPATH_128];
	c_dpsnprintf1 (pattern, "%s/*.jpg", filename); // "Like video/my_vid_20_fps/*.jpg"

	// Ok we have a folder ... get a list of everything in it.
alloc_1: ;

	fssearch_t	*t = FS_Search (pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (!t)
		return NULL;

	if (t->numfilenames == 0) {
		FS_FreeSearch(t);
		return NULL;
	}

	// Baker: At this point, confident enough to start allocations ...
alloc_2: ;
	jpegs_folder_stream_t *s	= (jpegs_folder_stream_t *)Mem_ZMalloc_SizeOf(jpegs_folder_stream_t);

alloc_3: ;
	
	stringlistappendfssearch (&s->jpegnames_list, (fssearch_t *)t);
	
	// Determine prefix and format by removing digits from end after removing extension.
	// video\my_vid_20_fps\ezgif-frame-001.jpg
	// Q: Does t->filenames[idx] is full path or not? fullpath

	char saveme[MAX_QPATH_128];
	char *sample = t->filenames[0];
	c_strlcpy (saveme, sample); // Save as we are stripping the extension, we must restore it.
	File_URL_Edit_Remove_Extension (sample);

	int slen = strlen(sample);

	int numberofdigits = 0;
	for (int j = slen - 1; j >= 0; j --) {
		int x = sample[j];
		if (!isdigit(x))
			break;
		numberofdigits ++;
	} // for

	int offset_from_start = slen - numberofdigits;

	c_strlcpy (sample, saveme); // Restore

#if 0 // Baker, to verify the number parsing is correct.
	char *s_number = &sample[offset_from_start];
#endif

	stringlistsort_substring	(&s->jpegnames_list, fs_make_unique_true, offset_from_start, numberofdigits);
	
	if (developer_loading.integer) {
		stringlist_condump			(&s->jpegnames_list);
	}

	stringlist_t *plist = &s->jpegnames_list;

	s->jpegblobsizes_alloc = (int *)Z_Malloc(sizeof(int) * s->jpegnames_list.numstrings);

	for (int idx = 0; idx < plist->numstrings; idx++) {
		char *sxy = plist->strings[idx];

		fs_offset_t filesize;
		unsigned char *filedata = FS_LoadFile (sxy, tempmempool, fs_quiet_true, &filesize);

		if (filedata == NULL) {
			// Baker: This should be super-rare because it was reported as existing file
			Con_PrintLinef ("Couldn't open " QUOTED_S, sxy);
			goto exitor; // Should cleanup ok
		}

		stringlistappend_blob (&s->jpegblob_list, filedata, filesize);
		s->jpegblobsizes_alloc[idx] = (int)filesize;
		Mem_FreeNull_ (filedata);
	} // for

	byte *jpeg_data = (byte *)s->jpegblob_list.strings[0];
	size_t jpeg_datasize = (size_t)s->jpegblobsizes_alloc[0];

	// Load the first jpeg to get image_width/image_height globals that the DarkPlaces loader uses
	unsigned char *jpeg_bgra_data_zalloc = JPEG_LoadImage_BGRA (jpeg_data, jpeg_datasize, /*mipevel*/ NULL);

	Mem_FreeNull_ (jpeg_bgra_data_zalloc);

	if (developer_loading.value) {
		// 0.05 frames per second * 20 frames = 1 second
		Con_PrintLinef ("Total time is %f", (double)1.0/frames_per_second * s->jpegnames_list.numstrings);
	}
	
	s->jpeg_framenum		= 0;

	s->info_imagewidth		= image_width;
	s->info_imageheight		= image_height;
	s->info_framerate		= frames_per_second;
	s->info_aspectratio		= (double)s->info_imagewidth / (double)s->info_imageheight;

	// set the interface
	video->close			= bakerjpegdecode_close;
	video->getwidth			= bakerjpegdecode_getwidth;
	video->getheight		= bakerjpegdecode_getheight;
	video->getframerate		= bakerjpegdecode_getframerate;
	video->decodeframe		= bakerjpegdecode_videoframe;
	video->decodeframegif	= NULL;
	video->getaspectratio	= bakerjpegdecode_getaspectratio;
	video->is_gif_stream	= false;

	return s;

exitor:

	// Error cleanup
	if (t) FS_FreeSearch(t);
	if (s) {
		stringlistfreecontents (&s->jpegnames_list);
		stringlistfreecontents (&s->jpegblob_list);
		Mem_FreeNull_ (s->jpegblobsizes_alloc);
	}
	Mem_FreeNull_ (s);
	return NULL;
}


