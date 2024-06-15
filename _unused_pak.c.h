prove I don't compile by this error not happening

#if 0 // Baker: We don't need this!!  YAY!

// pak.c.h -- pak file. Orgins in 2014 and 2015.

//#define LittleLong(value) (value)

#define MAX_PAK_FILENAME 56
#define PAK_FILE_ENTRY_SIZE_64  64 // Hopefully sizeof(dpackfile_t)
#define PAK_FILE_HEADER_SIZE_12 12 // Hopefully sizeof(dpackheader_t)

typedef struct
{
	char    name[MAX_PAK_FILENAME];
	int     filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int     dirofs;
	int     dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK_2048       2048
#define PAK_HEADER "PACK"

typedef struct pak_s
{
	char			url[MAX_OSPATH];
	dpackheader_t   header;
	dpackfile_t		files[MAX_FILES_IN_PACK_2048];
// Add a structure for new files to add?
// When we write, do we need to qsort?
	int				numfiles;
	FILE			*f;

// Extra
	byte			*pHeader;
	byte			*pDirectory;
	int				length;
//	clist_t			*files_slist;
} pak_t;

// On OSX, we will need to have the pak file in memory the entire time?
// And we won't be using this function at all because it is "on disk"?
pak_t *Pack_Open_Memory_Malloc (const byte *mem, size_t mem_length)
{
	const byte *bmem = mem;
	size_t sz;

	pak_t *pack = (pak_t *)calloc(sz=sizeof(*pack), ONE_SIZEOF_BYTE_1);
	//_alert ("%d", (int)sz);
	pack->pHeader			= (byte *)bmem; // This doesn't hurt.
	pack->length			= mem_length;

	memcpy (&pack->header, pack->pHeader, sizeof(pack->header));

	if (memcmp (pack->header.id, PAK_HEADER, strlen(PAK_HEADER)) ) {
		//log_debug ("Data not a packfile"); // Communicate nature of the problem
		goto pakopenerr;
	}

	pack->header.dirofs	= LittleLong (pack->header.dirofs); // LittleLong on Little-endian processors like Intel just are the value
	pack->header.dirlen	= LittleLong (pack->header.dirlen);
	pack->numfiles		= pack->header.dirlen / PAK_FILE_ENTRY_SIZE_64; // sizeof(dpackfile_t);

	if (pack->numfiles > MAX_FILES_IN_PACK_2048) {
		//log_debug ("%s has %d files > max %d", "<embedded packfile>", pack->numfiles, MAX_FILES_IN_PACK_2048); // Communicate nature of the problem
		goto pakopenerr;
	}

	pack->pDirectory		= &pack->pHeader[pack->header.dirofs];
	memcpy ( (dpackfile_t *)&pack->files, pack->pDirectory, pack->header.dirlen);

#if 0 // Super debug
    {
        int n;
        for (n = 0; n < pack->numfiles; n ++) {
            const char *fn = pack->files[n].name;
			//_alert ("pak %d: %s", n, fn);
        }
    }
#endif // 1
	//_alert ("%d @ %p", pack->numfiles, pack);
	//_alert ("Pack_Open_Memory_Alloc Bundle: numfiles is %d, last file %s", pack->numfiles, pack->files[pack->numfiles - 1].name);
	return pack;

// Fail
pakopenerr:
	free (pack);
	return NULL;
}

//const void *Bundle_File_Memory_Pointer (const pak_t *mypak, const char *path_to_file, /*reply*/ size_t *mem_length)
//{
//	// // EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1 in Pack_File_Entry_To_Memory_Alloc so we have it
//	return Pack_File_Memory_Pointer (mypak, mainus.pak_blob_y, path_to_file, mem_length);
//}
// returns null
void *dPack_Find_File_Caseless (pak_t *pack, const char *path_to_file)
{
	int n;

	for (n = 0; n < pack->numfiles; n ++) {
		const char *fn = pack->files[n].name;
		if (!strcasecmp (path_to_file, fn))
			return &pack->files[n];
	}

	return NULL; // Not found
}

const void *Pack_File_Memory_Pointer (pak_t *pack, const byte *pak_blob, const char *path_to_file, /*reply*/ size_t *mem_length)
{
	dpackfile_t *row = (dpackfile_t *)dPack_Find_File_Caseless (pack, path_to_file);
	const byte *mem_o = mem_o = row ? &pak_blob[row->filepos] : NULL;

	if (!row) {
		logd ("File not found in pack: '%s'", path_to_file);
		return NULL;
	}

	NOT_MISSING_ASSIGN (mem_length, row->filelen);
	return mem_o;
}

#endif