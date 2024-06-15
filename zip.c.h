// zip.c.h -- zip

//minimus
//#define CORE_LOCAL
//#include "environment.h"
//#include "core.h"
//#include "zip.h"


#include "miniz.h"

#define query_exists	0
#define	query_index		1
#define	query_filesize	2
#define query_offset	3

#define logd Con_PrintLinef 
// Returns not_found_neg1 on failure
int Zip_File_Query (const char *zipfile_url, const char *filename, int query_type)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);

	if (status)
	{
		mz_zip_archive_file_stat cur;

		int i;

		// Get and print information about each file in the archive.
		for (i = 0; i < (int) curzip.m_total_files; i++)
		{
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				break;
			}

			// If name doesn't match keep going
			if (strcasecmp (cur.m_filename, filename))
				continue;

			switch (query_type)
			{
			case query_exists:
			case query_index:
				mz_zip_reader_end (&curzip);
				return i;
//			case query_offset:
//				CloseZip (curzip);
//				return curzip->files[i].filepos;
			case query_filesize:
				mz_zip_reader_end (&curzip);
				return cur.m_uncomp_size;
			}
		}

		mz_zip_reader_end (&curzip);
	}
	return -1;
}


size_t Zip_File_Size (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);
	qbool found = (result == - 1) ? false : true;

	if (found)
		return result;

	return 0;
}


qbool Zip_Has_File (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);

	qbool found = (result == not_found_neg1) ? false : true;

	return found;
}

// If inside_zip_filename is NULL, we do them all
// Returns true (ok) or false on failure

typedef void (*printline_fn_t) (const char *fmt, ...);
WARP_X_ (Con_PrintLinef)

int sZip_Extract_File_Is_Ok (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url, printline_fn_t my_printline)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	int written;

	if (status)
	{
		mz_zip_archive_file_stat cur;
		char current_url[MAX_OSPATH];
		const char *curfile = inside_zip_filename ? destfile_url : current_url;
		int i;

		for (i = 0, written = 0; i < (int) curzip.m_total_files; i++) {
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				return false;
			}

			// If inside_zip_filename is NULL, we do them all
			if (inside_zip_filename && strcasecmp (cur.m_filename, inside_zip_filename))
				continue;

			// If it is a directory, we aren't writing it
			if (String_Ends_With (cur.m_filename, "/"))
				continue;

			// If we are extracting the whole thing, destfile_url is folder + cur->name becomes url for curfile
			if (!inside_zip_filename) // 13 May 2015 commented out
				c_dpsnprintf2 (current_url, "%s/%s", destfile_url, cur.m_filename);
			else c_strlcpy (current_url, destfile_url);

#if 000 // Feb 15 2024
			// make the folder
			File_Mkdir_Recursive (curfile);
#endif

			// extract
			if (!mz_zip_reader_extract_to_file (&curzip, cur.m_file_index, current_url, 0))
			{
				logd ("mz_zip_reader_extract_to_file() failed!");
				mz_zip_reader_end (&curzip);
				return false;
			}

			my_printline ("Extracted to %s", curfile);
			written ++;

			if (inside_zip_filename)
				break; // Asked for just one so get out
		}

		mz_zip_reader_end (&curzip);

		return written;
	}

	logd ("Couldn't open zip %s", zipfile_url);
	return false;
}


qbool Zip_Extract_File (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url)
{
	if (sZip_Extract_File_Is_Ok (zipfile_url, inside_zip_filename, destfile_url, Con_PrintLinef))
		return true;

	return false;
}


int Zip_Unzip (const char *zipfile_url, const char *dest_folder_url)
{
	int n = sZip_Extract_File_Is_Ok (zipfile_url, NULL, dest_folder_url, Con_PrintLinef);

	return n;
}

// Returns true if ok, false is failed.
// Requires a real file system url
qbool Zip_List_Print_Is_Ok (const char *zipfile_url)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file (&curzip, zipfile_url, 0);

	if (status) {
		for (unsigned int idx = 0, found = 0; idx < curzip.m_total_files; idx ++, found ++) {
			// get individual details
			mz_zip_archive_file_stat cur;

			if (!mz_zip_reader_file_stat (&curzip, idx, &cur)) {
				logd ("mz_zip_reader_file_stat() failed!");
				break;
			}

			logd ("%04d: %s", found, cur.m_filename);
		} // for

		mz_zip_reader_end (&curzip);
		return true;
	}
	return false;
}

#if 0
clist_t * Zip_List_Alloc (const char *zipfile_url)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	clist_t * list = NULL;

	if (status)
	{
		mz_zip_archive_file_stat cur;
		int i, found;

		// Get and print information about each file in the archive.
		for (i = 0, found = 0; i < (int) curzip.m_total_files; i++, found ++)
		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				List_Free (&list);
				return NULL;
			}

			List_Add (&list, cur.m_filename);
		}

		mz_zip_reader_end (&curzip);
	}

	return list;
}

clist_t * Zip_List_Details_Alloc (const char *zipfile_url, const char *s_delimiter)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	clist_t * list = NULL;

	if (!delimiter) delimiter = "\t";

	if (status)
	{
		mz_zip_archive_file_stat cur;
		int i, found;

		// Get and print information about each file in the archive.
		for (i = 0, found = 0; i < (int) curzip.m_total_files; i++, found ++)
		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				List_Free (&list);
				return NULL;
			}

			List_Add(&list, va("%s%s%g",cur.m_filename,delimiter,(double)cur.m_uncomp_size));
			// Future: ,delimiter,(double)cur.m_time));
		}

		mz_zip_reader_end (&curzip);
	}

	return list;
}
#endif

#if 0
int Zip_Zip_Folder (const char *zipfile_url, const char *source_folder_url)
{
	clist_t *files = File_List_Recursive_Relative_Alloc_Deprecated (source_folder_url, NULL);
	int count = 0;

	if (files)
	{
		mz_zip_archive curzip = {0};
		clist_t *listitem;

		// Make the location of the zip file
		File_Mkdir_Recursive (zipfile_url);

		if (!mz_zip_writer_init_file(&curzip, zipfile_url, MZ_ZIP_FLAG_COMPRESSED_DATA))
		{
			List_Free (&files);
			return 0;
		}

		for (listitem = files, count = 0; listitem; listitem = listitem->next, count ++)
		{
			const char * full_url = va ("%s/%s", source_folder_url, listitem->name);

			if (!mz_zip_writer_add_file(&curzip, listitem->name, full_url, NULL, 0, MZ_DEFAULT_LEVEL /* flags*/))
			{
				mz_zip_writer_finalize_archive(&curzip);
				mz_zip_writer_end(&curzip);
				List_Free (&files);
				return 0;
			}

			logd ("%04d: Added %s", count, listitem->name);
		}

		// close the file
		mz_zip_writer_finalize_archive(&curzip);
		mz_zip_writer_end(&curzip);

		// Free the list
		List_Free (&files);
		return count;
	}
	return 0;
}
#endif


