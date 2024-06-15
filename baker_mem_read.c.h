// baker_mem_read.c.h -- pak file. Orgins in 2014 and 2015.

#define BB_REMAINING(bb) (bb->filesize - bb->cursor)
//ssize_t read_x(int fildes, void *buf, size_t nbyte);


int baker_read (bakerbuf_t *bb, void *dst, size_t readsize)
{
	// returns number of bytes read ... -1 on error, 0 on no action
	if (BB_REMAINING(bb) < readsize)
		readsize = BB_REMAINING(bb);

	if (readsize <= 0)
		return 0;
	
	memcpy (dst, &bb->buf_malloc[bb->cursor], readsize);
	bb->cursor += readsize;

	return readsize;
}

//off_t lseek(int fildes, off_t offset, int whence);

int baker_lseek(bakerbuf_t *bb, int offset, int whence)
{
	if (whence == SEEK_SET) bb->cursor = offset;
	else if (whence == SEEK_CUR) bb->cursor += offset;
	else if (whence == SEEK_CUR) bb->cursor = bb->filesize + offset;
	//If whence is SEEK_SET the file offset is set to offset bytes.
	//If whence is SEEK_CUR the file offset is set to its current location plus offset.
	//If whence is SEEK_END the file offset is set to the size of the file plus offset.
	return bb->cursor;
}

WARP_X_ (read_num)
unsigned short baker_read_usshort(bakerbuf_t *bb)
{
    unsigned char /* uint8_t */ bytes[2];

    //read(fd_for_func, bytes, 2);
	baker_read (bb, bytes, 2);
    return bytes[0] + (((unsigned short /* uint16_t */) bytes[1]) << 8);
}

 //int open(const char *path, int oflag, ... );
qbool baker_open_actual_file_is_ok (bakerbuf_t *bb, const char *path)
{
	byte *buf = (byte *)File_To_Memory_Alloc (path, &bb->filesize);

	if (!buf)
		return false;

	bb->buf_malloc = buf;
	bb->cursor = 0;

	return true;
}

qbool baker_open_from_memory_is_ok (bakerbuf_t *bb, const void *data, size_t datalen)
{
	bb->filesize = datalen;
	byte *buf = (byte *)malloc (datalen);

	if (!buf)
		return false;

	memcpy (buf, data, datalen);

	bb->buf_malloc = buf;
	bb->cursor = 0;
	bb->did_malloc = true;

	return true;
}

bakerbuf_t *baker_open_from_memory_NO_MALLOC_is_ok (const void *data, size_t datalen)
{
	bakerbuf_t *bb	= (bakerbuf_t *)calloc(sizeof (bakerbuf_t), 1);
	bb->filesize = datalen;
	//byte *buf = (byte *)malloc (datalen);

	//if (!buf)
	//	return false;

	//memcpy (buf, data, datalen);

	bb->buf_malloc = (byte *)data;//buf;
	bb->cursor = 0;
	bb->did_malloc = false;

	return bb;
}

bakerbuf_t *baker_close (bakerbuf_t *bb)
{
	if (bb->did_malloc && bb->buf_malloc) {
		freenull_ (bb->buf_malloc);
	}

	if (bb->did_malloc == false) {
		freenull_ (bb);
	}

	return NULL;
}
