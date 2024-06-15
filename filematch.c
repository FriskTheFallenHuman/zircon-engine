
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "darkplaces.h"

#ifdef _WIN32
#include "utf8lib.h"
#endif

// LadyHavoc: some portable directory listing code I wrote for lmp2pcx, now used in darkplaces to load id1/*.pak and such...

int matchpattern(const char *in, const char *pattern, int caseinsensitive)
{
	return matchpattern_with_separator(in, pattern, caseinsensitive, "/\\:", false);
}

// wildcard_least_one: if true * matches 1 or more characters
//                     if false * matches 0 or more characters
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one)
{
	int c1, c2;
	while (*pattern)
	{
		switch (*pattern)
		{
		case 0:
			return 1; // end of pattern
		case '?': // match any single character
			if (*in == 0 || strchr(separators, *in))
				return 0; // no match
			in++;
			pattern++;
			break;
		case '*': // match anything until following string
			if (wildcard_least_one)
			{
				if (*in == 0 || strchr(separators, *in))
					return 0; // no match
				in++;
			}
			pattern++;
			while (*in)
			{
				if (strchr(separators, *in))
					break;
				// see if pattern matches at this offset
				if (matchpattern_with_separator(in, pattern, caseinsensitive, separators, wildcard_least_one))
					return 1;
				// nope, advance to next offset
				in++;
			}
			break;
		default:
			if (*in != *pattern)
			{
				if (!caseinsensitive)
					return 0; // no match
				c1 = *in;
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				c2 = *pattern;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				if (c1 != c2)
					return 0; // no match
			}
			in++;
			pattern++;
			break;
		}
	}
	if (*in)
		return 0; // reached end of pattern but not end of input
	return 1; // success
}

// a little strings system
void stringlistinit(stringlist_t *list)
{
	memset(list, 0, sizeof(*list));
}

void stringlistfreecontents(stringlist_t *list)
{
	int i;
	for (i = 0;i < list->numstrings;i++)
	{
		if (list->strings[i])
			Z_Free(list->strings[i]);
		list->strings[i] = NULL;
	}
	list->numstrings = 0;
	list->maxstrings = 0;
	if (list->strings)
		Z_Free(list->strings);
	list->strings = NULL;
}

void stringlistprint (stringlist_t *list, const char *title_optional, void (*myprintf)(const char *, ...) )
{
	if (title_optional)
		myprintf ("%s = %d", title_optional, list->numstrings);
	else {
		// Print nothing!
	}

	for (int idx = 0; idx < list->numstrings; idx ++) {
		const char *s = list->strings[idx];
		myprintf ("%4d: %s", idx, s);
	} // for idx

}

// Baker: Does not increase numstrings
const char **stringlist_nullterm_add (stringlist_t *list)
{
	char **oldstrings;

	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy(list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	
	list->strings[list->numstrings] = NULL; //Z_StrDup(text);
	//list->numstrings++;
	return (const char **)list->strings;
}


int stringlistappendfilelines_did_load (stringlist_t *plist, ccs *file, int is_number_column)
{
	fs_offset_t filesize = 0;
	char *sa = (char *)FS_LoadFile(file, tempmempool, fs_quiet_true, &filesize); 
	// FS_LoadFile Always appends a 0 byte.
	if (!sa)
		return false;

	//String_Edit_Replace_Char (sa, CARRIAGE_RETURN_CHAR_13, NEWLINE_CHAR_10, /*countreply*/ NULL);
	char *String_Edit_Whitespace_To_Space_Except_Newline (char *s_edit);
	String_Edit_Whitespace_To_Space_Except_Newline (sa);

	if (is_number_column) {
		stringlist_t listugly = {0};
		stringlistappend_split (&listugly, sa, NEWLINE);
		for (int n = 0; n < listugly.numstrings; n ++) {
			stringlistappendf (plist, "%d", n);
			stringlistappend (plist, listugly.strings[n]);
		} // for
		stringlistfreecontents (&listugly);
	} 
	else {
		stringlistappend_split (plist, sa, NEWLINE);
	}

	Mem_FreeNull_ (sa);
	return true;
}

int stringlistappend_split_delimiter_linenumbers (stringlist_t *plist, ccs *s, ccs *delimiter)
{
	//String_Edit_Whitespace_To_Space_Except_Newline (sa);	
	stringlist_t listugly = {0};
	stringlistappend_split (&listugly, s, delimiter);
	for (int n = 0; n < listugly.numstrings; n ++) {
		stringlistappendf (plist, "%d", n);
		stringlistappend (plist, listugly.strings[n]);
	} // for
	stringlistfreecontents (&listugly);

	return true;
}


void stringlistappend (stringlist_t *list, const char *text)
{
	//size_t textlen;
	char **oldstrings;

	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy(list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	
	list->strings[list->numstrings] = Z_StrDup(text);
	list->numstrings++;
}

void stringlistappendf (stringlist_t *list, const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	stringlistappend (list, text);
	VA_EXPAND_ALLOC_FREE (text);
}

void stringlistappendf2 (stringlist_t *list, ccs *s, const char *fmt, ...)
{
	stringlistappend (list, s);
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	stringlistappend (list, text);
	VA_EXPAND_ALLOC_FREE (text);
}

void stringlistappend2 (stringlist_t *list, ccs *s, ccs *s2)
{
	stringlistappend (list, s);
	stringlistappend (list, s2);
}

void stringlistappend3 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3)
{
	stringlistappend (list, s);
	stringlistappend (list, s2);
	stringlistappend (list, s3);
}

void stringlistappend4 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3, ccs *s4)
{
	stringlistappend (list, s);
	stringlistappend (list, s2);
	stringlistappend (list, s3);
	stringlistappend (list, s4);
}

void stringlistappend5 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3, ccs *s4, ccs *s5)
{
	stringlistappend (list, s);
	stringlistappend (list, s2);
	stringlistappend (list, s3);
	stringlistappend (list, s4);
	stringlistappend (list, s5);
}

void stringlistappend_len (stringlist_t *list, const char *text, int text_len)
{
	char **oldstrings;

	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy(list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	
	list->strings[list->numstrings] = (char *)Z_MemDup_Z (text, text_len);
	list->numstrings++;
}

WARP_X_ (VM_tokenize_console) // VM_tokenize_console does have way to identify column
void stringlistappend_tokenize_qcquotes (stringlist_t *plist, const char *text)
{
	const char *p = text;
	int num_tokens = 0;
	while (1) {
		// skip whitespace here to find token start pos
		while(*p && ISWHITESPACE(*p))
			p ++;

		const char *startpos = p;//tokens_startpos[num_tokens] = p - tokenize_string;
//		if (!COM_ParseToken_Console(&p))
//			break;

		if (!COM_ParseToken_QuakeC(&p, /*newline?*/ false))
			break;

		//tokens_endpos[num_tokens] = p - tokenize_string;

		//if (!COM_ParseToken_Console(&p))
		//	break;

		// Baker: Let's keep quotes so we know isn't a name or something
		if (*startpos == '\"') {
			va_super  (tmp, sizeof(com_token) + 2, QUOTED_S, com_token); // +2 quotes
			stringlistappend (plist, tmp);
		}
		else stringlistappend (plist, com_token);
		num_tokens ++;
	} // while

}

void stringlistappend_split_len (stringlist_t *plist, const char *text_inx, int text_strlen, const char *s_delimiter)
{
	// Because strstr won't honor the length, we must copy to buffer
	char *s_z = Z_StrDup_Len_Z (text_inx, text_strlen);


	int delimiter_slen = strlen(s_delimiter);
	const char *sbeyond = s_z + text_strlen;
	const char *cursor = s_z;
	const char *p;
	while ( p = strstr(cursor, s_delimiter) ) {
		int seglen = p - cursor; // p is beyond
//#ifdef _DEBUG
//		char *zz = Z_StrDup_Len_Z (cursor,seglen);
//		if (String_Contains(zz, "klook")) {
//			int j = 5;
//		}
//
//
//#endif

		stringlistappend_len (plist, cursor, seglen);
		cursor = p + delimiter_slen;//ONE_CHAR_1;
		//Z_FreeNull_(zz);

	}
	if (cursor != sbeyond) {
		// Hit the trail
		int seglen = sbeyond - cursor;
//#ifdef _DEBUG
//		char *zz = Z_StrDup_Len_Z (sbeyond,seglen);
//		if (String_Contains(zz, "klook")) {
//			int j = 5;
//		}
//
//
//#endif
		stringlistappend_len (plist, cursor, seglen);
		//Z_FreeNull_(zz);
	}
}

void stringlistappend_split (stringlist_t *plist, const char *text_in, const char *s_delimiter)
{
	stringlistappend_split_len (plist, text_in, strlen(text_in), s_delimiter);
}

void stringlistappend_textlines_len_cr_scrub (stringlist_t *plist, const char *text_in, int text_strlen)
{
	// Must make a copy to remove the carriage returns.
	char *text_alloc = Z_StrDup_Len_Z(text_in, text_strlen);

	// Baker: elim carriage returns
	String_Edit_Replace (text_alloc, text_strlen + ONE_CHAR_1, CARRIAGE_RETURN, "");

	int refreshed_strlen = strlen(text_alloc); // It may have changed after replace.

	char *sbeyond = text_alloc + refreshed_strlen;
	char *cursor = text_alloc;
	char *p;
	while ( p = strstr(cursor, NEWLINE) ) {
		int seglen = p - cursor; // p is beyond
		stringlistappend_len (plist, cursor, seglen);
		cursor = p + ONE_CHAR_1;
	}
	if (cursor != sbeyond) {
		// Hit the trail
		int seglen = sbeyond - cursor;
		stringlistappend_len (plist, cursor, seglen);
	}

	Z_FreeNull_ (text_alloc);
}

void stringlistappend_textlines_cr_scrub (stringlist_t *plist, const char *text_in)
{
	int text_len = strlen (text_in);
	stringlistappend_textlines_len_cr_scrub (plist,text_in,text_len);
}


void stringlistappend_blob (stringlist_t *list, const byte *blob, size_t blobsize)
{
	char **oldstrings;

	// Baker: This reallocs in batches of 4096
	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy (list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	//textlen = strlen(text) + 1;
	list->strings[list->numstrings] = (char *) Z_Malloc(blobsize);
	memcpy (list->strings[list->numstrings], blob, blobsize);
	list->numstrings++;
}

void stringlistappendlist (stringlist_t *plist, const stringlist_t *add_these)
{
	for (int idx = 0; idx < add_these->numstrings; idx++) {
		char *sxy = add_these->strings[idx];
		stringlistappend (plist, sxy);
	}

}

void stringlistappendfssearch (stringlist_t *plist, fssearch_t *t)
{
	if (!t)
		return;
	
	for (int idx = 0; idx < t->numfilenames; idx++) {
		char *sxy = t->filenames[idx];
		stringlistappend (plist, sxy);
	}

}

void stringlistappend_search_pattern (stringlist_t *plist, const char *s_pattern)
{
	fssearch_t	*t = FS_Search (s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (!t) return;

	// Baker: Entries should be the entire file name like
	// "sound/ambience/thunder.wav" or whatever

	stringlistappendfssearch (plist, t);

	if (t) FS_FreeSearch(t);
}

void stringlist_replace_at_index (stringlist_t *list, int idx, const char *text)
{
	if (in_range_beyond (0, idx, list->numstrings) == false) {
		Con_PrintLinef (CON_ERROR "string list at %d is out of bounds of 0 to %d", idx, list->numstrings);
		return;
	}
	Z_Free (list->strings[idx]);

	list->strings[idx] = Z_StrDup(text);
}

static int stringlistsort_cmp(const void *a, const void *b)
{
	return strcasecmp(*(const char **)a, *(const char **)b);
}

static int sstart = 0;
static int slength = 0;
static int stringlistsort_start_length_cmp(const void *_a, const void *_b)
{
	const char *a = *(const char **)_a;
	const char *b = *(const char **)_b;

	char *sa = Z_StrDup (a);
	char *sb = Z_StrDup (b);
	File_URL_Edit_Remove_Extension (sa);
	File_URL_Edit_Remove_Extension (sb);

	int result = 0;
#if 1 
	// Safety checks
	int slena = strlen (sa);
	int slenb = strlen (sb);

	if (slena < sstart + slength) {
		result = strcasecmp(sa, sb); // This is not right, but repeatably consistent in the sort order
		goto failout; // Too short
	}

	if (slenb < sstart + slength) {
		result = strcasecmp(sa, sb); // This is not right, but repeatably consistent in the sort order 
		goto failout; // Too short
	}
#endif

	memmove (sa, &sa[sstart], slength + 1); // +1 to copy the null too
	memmove (sb, &sb[sstart], slength + 1); // +1 to copy the null too

	result = strcasecmp(sa, sb); 

failout:

	Mem_FreeNull_ (sa);
	Mem_FreeNull_ (sb);

	return result;
}

void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) );

void stringlistsort(stringlist_t *list, qbool uniq)
{
	stringlistsort_custom (list, uniq, stringlistsort_cmp);
}

void stringlistsort_substring (stringlist_t *list, qbool uniq, int startpos, int slength)
{
	sstart = startpos;
	slength = slength;

	stringlistsort_custom (list, uniq, stringlistsort_start_length_cmp);
}

void stringlist_condump (stringlist_t *plist)
{
	for (int idx = 0; idx < plist->numstrings; idx++) {
		char *sxy = plist->strings[idx];

		Con_PrintLinef ("%4d: " QUOTED_S, idx, sxy);
	} // for
}

void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) )
{
	if (list->numstrings < 1)
		return;

	qsort(&list->strings[0], list->numstrings, sizeof(list->strings[0]), myfunc);

	// If Make Unique ... 
	if (uniq) {
		// i: the item to read
		// j: the item last written
	int i, j;
		for (i = 1, j = 0; i < list->numstrings; ++i)
		{
			char *save;
			if (String_Match_Caseless(list->strings[i], list->strings[j]))
				continue;
			++j;
			save = list->strings[j];
			list->strings[j] = list->strings[i];
			list->strings[i] = save;
		}
		for(i = j + 1; i < list->numstrings; i ++) {
			if (list->strings[i])
				Z_Free(list->strings[i]);
		}
		list->numstrings = j+1;
	}
}


// operating system specific code
static void adddirentry(stringlist_t *list, const char *path, const char *name)
{
	if (String_NOT_Match(name, ".") && String_NOT_Match(name, "..")) {
		char temp[MAX_OSPATH];
		dpsnprintf( temp, sizeof( temp ), "%s%s", path, name );
		stringlistappend(list, temp);
	}
}

#ifdef _WIN32
// Baker: This concats the results
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	#define BUFSIZE 4096
	char pattern[BUFSIZE] = {0};
	wchar patternw[BUFSIZE] = {0};
	char filename[BUFSIZE] = {0};
	wchar *filenamew;
	int lenw = 0;
	WIN32_FIND_DATAW n_file;
	HANDLE hFile;
	c_strlcpy (pattern, basepath);
	c_strlcat (pattern, path);
	c_strlcat (pattern, "*");
	fromwtf8(pattern, (int)strlen(pattern), patternw, BUFSIZE);
	// ask for the directory listing handle
	hFile = FindFirstFileW(patternw, &n_file);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	do {
		filenamew = n_file.cFileName;
		lenw = 0;
		while(filenamew[lenw] != 0) ++lenw;
		towtf8(filenamew, lenw, filename, BUFSIZE);
		adddirentry(list, path, filename);
	} while (FindNextFileW(hFile, &n_file) != 0);
	FindClose(hFile);
	#undef BUFSIZE
}
#else
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	char fullpath[MAX_OSPATH];
	DIR *dir;
	struct dirent *ent;
	dpsnprintf(fullpath, sizeof(fullpath), "%s%s", basepath, path);
#ifdef __ANDROID__
	// SDL currently does not support listing assets, so we have to emulate
	// it. We're using relative paths for assets, so that will do.
	if (basepath[0] != '/')
	{
		char listpath[MAX_OSPATH];
		qfile_t *listfile;
		dpsnprintf(listpath, sizeof(listpath), "%sls.txt", fullpath);
		char *buf = (char *) FS_SysLoadFile(listpath, tempmempool, true, NULL);
		if (!buf)
			return;
		char *p = buf;
		for (;;)
		{
			char *q = strchr(p, '\n');
			if (q == NULL)
				break;
			*q = 0;
			adddirentry(list, path, p);
			p = q + 1;
		}
		Mem_Free(buf);
		return;
	}
#endif
	dir = opendir(fullpath);
	if (!dir)
		return;

	while ((ent = readdir(dir)))
		adddirentry(list, path, ent->d_name);
	closedir(dir);
}
#endif

// Baker: Usage:
// stringlist_t matchedSet;	
// stringlistinit	(&matchedSet); // this does not allocate, memset 0
//
// stringlist_from_delim (&matchedSet, mystring);
//// SORT plus unique-ify
//stringlistsort (&matchedSet, fs_make_unique_true);
//stringlistfreecontents( &matchedSet );

// Baker: Takes a stringlist and turns it in a single string, spaces between at moment
// Example ?  No callers at moment.
// This was going to be used, then another idea came up
// But code is not quite finished (SAY WHY BASTARD!)

// Baker: This looks like "join" and it does work.
void stringlist_join_buf (stringlist_t *p_stringlist, ccs *s_delimiter, char *buf, size_t buflen)
{
	buf[0] = 0;
	strlcpy (buf, "", buflen);
	for (int idx = 0; idx < p_stringlist->numstrings; idx ++) {
		char *sxy = p_stringlist->strings[idx];
		if (idx > 0)
			strlcat (buf, " ", buflen);
		strlcat (buf, sxy, buflen);
	} // for
}

	//if (k_save) {
	//	// Baker: This could happen if somehow error occurs that exits function
	//	BakerString_Destroy_And_Null_It (&k_save); // nulls k_save
	//}

baker_string_t *stringlist_join_bkalloc (stringlist_t *plist, ccs *s_delimiter)
{
	baker_string_t *bs_malloc = BakerString_Create_Malloc ("");
	int slen_delim = strlen(s_delimiter);
	for (int idx = 0; idx < plist->numstrings; idx ++) {
		char *s = plist->strings[idx];
		if (idx > 0)
			BakerString_Cat_No_Collide (bs_malloc, slen_delim, s_delimiter);
		BakerString_Cat_No_Collide (bs_malloc, strlen(s), s);
	} // for
	return bs_malloc;
}

WARP_X_ (baker_string_t)
baker_string_t *stringlist_join_lines_bkalloc (stringlist_t *plist)
{
	baker_string_t *bs_malloc = stringlist_join_bkalloc (plist, NEWLINE);
	return bs_malloc;
}

// 2 = auto
char *stringlist_join_lines_zalloc (stringlist_t *plist)
{
	baker_string_t *bs_malloc = stringlist_join_bkalloc (plist, NEWLINE);
	char *s_za = NULL;
	//switch (shall_quote) {
	//case fs_quoted_auto_2:
	//	if (String_Is_All_AlphaNumeric_Underscore(bs_malloc->string)) {
	//		s_za = Z_StrDup_Len_Z (bs_malloc->string, bs_malloc->length);
	//		break;
	//	}
	//	// Isn't alpha -- fall through for quoted

	//case fs_quoted_true:
	//	s_za = Z_StrDupf (QUOTED_S, bs_malloc->string);
	//	break;

	//default:
		s_za = Z_StrDup_Len_Z (bs_malloc->string, bs_malloc->length);
//	} // sw

	// bs_malloc can't be null here, baker_string inits with an empty string alloc
	if (bs_malloc) BakerString_Destroy_And_Null_It (&bs_malloc);
	return s_za;
}

char *stringlist_join_spaced_zalloc (stringlist_t *plist)
{
	baker_string_t *bs_malloc = stringlist_join_bkalloc (plist, " ");
	char *s_za = Z_StrDup_Len_Z (bs_malloc->string, bs_malloc->length);

	// bs_malloc can't be null here, baker_string inits with an empty string alloc
	if (bs_malloc) BakerString_Destroy_And_Null_It (&bs_malloc);
	return s_za;
}

WARP_X_ (stringlistappend)
void floats_add1 (floatlist_s *flist, float p)
{
	int newcount = flist->count + 1 + 1; // + 1 null terminated last entry
	if (newcount >= flist->maxsize) {
		flist->maxsize += 4096;
		flist->floats = (float *)Mem_Realloc (zonemempool, flist->floats, sizeof(float) * flist->maxsize);
	}
	
	flist->floats[flist->count++] = p;
}

void floats_add2 (floatlist_s *flist, float px, float py)
{
	int newcount = flist->count + 2 + 1; // + 1 null terminated last entry
	if (newcount >= flist->maxsize) {
		flist->maxsize += 4096;
		flist->floats = (float *)Mem_Realloc (zonemempool, flist->floats, sizeof(float) * flist->maxsize);
	}
	
	flist->floats[flist->count++] = px;
	flist->floats[flist->count++] = py;
}

void floats_add3 (floatlist_s *flist, float px, float py, float pz)
{
	int newcount = flist->count + 3 + 1; // + 1 null terminated last entry
	if (newcount >= flist->maxsize) {
		flist->maxsize += 4096;
		flist->floats = (float *)Mem_Realloc (zonemempool, flist->floats, sizeof(float) * flist->maxsize);
	}
	
	flist->floats[flist->count++] = px;
	flist->floats[flist->count++] = py;
	flist->floats[flist->count++] = pz;
}


void floats_print (floatlist_s *flist, void (*myprintf)(const char *, ...))
{
	for (int idx = 0; idx < flist->count; idx ++) {
		float f = flist->floats[idx];
		myprintf ("%4d: " FLOAT_LOSSLESS_FORMAT, idx, f);
	} // for idx
}

WARP_X_ (stringlistprint)

void floats_dump (floatlist_s *flist)
{
	floats_print (flist, Con_PrintLinef);
}

char *int32s_join_delim_zalloc (int32list_s *ilist, ccs *s_delimiter_elements, int shall_quote)
{
	stringlist_t list = {0};
	for (int n = 0; n < ilist->count; n ++) {
		int d = ilist->ints[n];
		stringlistappendf (&list, "%d", d);
	} // for

	// Now join the spaced floats with commas
	char *s_comma_join_z = stringlist_join_delim_zalloc (&list, s_delimiter_elements, shall_quote);
	stringlistfreecontents (&list); // done with this, thank you

	return s_comma_join_z; 
}

char *floats_join_delim12_delim_zalloc (floatlist_s *flist, int number_per_vert, ccs *s_delimiter12, ccs *s_delimiter_elements, int shall_quote)
{
	if (number_per_vert == 0)
		return NULL;

	if (false == in_range(0, number_per_vert, 12)) // Too many or negative
		return NULL; // limit is 12

	if ( (flist->count modulo number_per_vert) != 0)
		return NULL;

	int num_groups = flist->count / number_per_vert;
	stringlist_t list = {0};
	for (int n = 0; n < flist->count; n ++) {
		float f = flist->floats[n];
		stringlistappendf (&list, FLOAT_LOSSLESS_FORMAT, f);
	} // for

	int idx = 0;
	// In groups of x, create a string
	stringlist_t group_of_spaced_floats = {0}; //
	for (int grp = 0; grp < num_groups; grp ++, idx += number_per_vert) {
		// Assume fairly reasonable size
		char buf[1024];
		c_strlcpy (buf, "");
		for (int seq = 0; seq < number_per_vert; seq ++) {
			int jidx = idx + seq;
			ccs *s = list.strings[jidx];
			
			if (seq > 0)
				c_strlcat (buf, s_delimiter12); // precat
			c_strlcat (buf, s);
		}
		stringlistappend (&group_of_spaced_floats, buf);
	} // for

	// Now join the spaced floats with commas
	char *s_comma_join_z = stringlist_join_delim_zalloc (&group_of_spaced_floats, s_delimiter_elements, shall_quote);

	stringlistfreecontents (&group_of_spaced_floats); // done with this, thank you
	stringlistfreecontents (&list); // done with this, thank you

	return s_comma_join_z; 
}


// Vertexes:"20 20, 500 400, 550 450"
// delimiter12 " " is the separator between the numbers in a vertex
// delimter_elements "," is the separator between the vertex doublets or triplets
// number_per_vert 2 means 2D where there are 2 numbers like "20 20" for a vertex

// Returns number of warnings.
int floats_append_parse_space_comma_num_elements_ignored (floatlist_s *flist, ccs *s_value, int number_per_vert, ccs *s_delimiter12, ccs *s_delimiter_elements)
{
	int num_elements_ignored = 0;

	stringlist_t rlist = {0};
	// Split the element pairs using comma split
	{
		char *s_z = Z_StrDup(s_value);
		String_Edit_DeQuote(s_z);
		stringlistappend_split (&rlist, s_z, s_delimiter_elements /*COMMA*/ );
		Z_FreeNull_ (s_z); // DISCHARGED
	}

	//const char **wordray = stringlist_nullterm_add (&rlist);
	stringlist_t tmplist = {0};

	for (int n = 0; n < rlist.numstrings; n ++) {
		// SPLIT ON SPACES	
		char *selem = rlist.strings[n];
		String_Edit_Trim(selem);

		stringlistappend_split (&tmplist, selem, s_delimiter12 /*SPACE*/ );
		if (tmplist.numstrings == number_per_vert) {
			for (int idx = 0; idx < tmplist.numstrings; idx ++) {
				char *spx = tmplist.strings[idx];
				float f = atof(spx);
				floats_add1 (flist, f );
			}
		} else {
			num_elements_ignored ++;
		}
		stringlistfreecontents(&tmplist);
	} // while

	stringlistfreecontents(&rlist);
	
	return num_elements_ignored;
}

void int32s_count_set (int32list_s *ilist, int newcount_in)
{
	int newcount = newcount_in + 1;//ilist->count + 3 + 1; // + 1 null terminated last entry
	if (newcount >= ilist->maxsize) {
		while (ilist->maxsize <= newcount) {
			ilist->maxsize += 4096;
		}
		ilist->ints = (int *)Mem_Realloc (zonemempool, ilist->ints, sizeof(int) * ilist->maxsize);
	}
	if (newcount_in > ilist->count)
		ilist->count = newcount_in;
}

void int32s_freecontents (int32list_s *ilist)
{
	// Unlike stringlist_t, no strings to free.
	ilist->count = 0;
	ilist->maxsize = 0;
	Z_FreeNull_ (ilist->ints);
}

void int32s_add1 (int32list_s *ilist, int p)
{
	int newcount = ilist->count + 1 + 1; // + 1 null terminated last entry
	if (newcount >= ilist->maxsize) {
		ilist->maxsize += 4096;
		ilist->ints = (int *)Mem_Realloc (zonemempool, ilist->ints, sizeof(int) * ilist->maxsize);
	}
	
	ilist->ints[ilist->count++] = p;
}

void int32s_add2 (int32list_s *ilist, int px, int py)
{
	int newcount = ilist->count + 2 + 1; // + 1 null terminated last entry
	if (newcount >= ilist->maxsize) {
		ilist->maxsize += 4096;
		ilist->ints = (int *)Mem_Realloc (zonemempool, ilist->ints, sizeof(int) * ilist->maxsize);
	}
	
	ilist->ints[ilist->count++] = px;
	ilist->ints[ilist->count++] = py;
}

void int32s_add3 (int32list_s *ilist, int px, int py, int pz)
{
#ifdef _DEBUG
	if (!in_range_beyond (1, px, 10000) || !in_range_beyond (1, py, 10000), !in_range_beyond (1, pz, 10000)) {
		int j = 5;
	}
#endif

	int newcount = ilist->count + 3 + 1; // + 1 null terminated last entry
	if (newcount >= ilist->maxsize) {
		ilist->maxsize += 4096;
		ilist->ints = (int *)Mem_Realloc (zonemempool, ilist->ints, sizeof(int) * ilist->maxsize);
	}
	
	ilist->ints[ilist->count++] = px;
	ilist->ints[ilist->count++] = py;
	ilist->ints[ilist->count++] = pz;
}

void int32s_print (int32list_s *ilist, void (*myprintf)(const char *, ...))
{
	for (int idx = 0; idx < ilist->count; idx ++) {
		int d = ilist->ints[idx];
		myprintf ("%4d: %d", idx, d);
	} // for idx
}

void int32s_dump (int32list_s *ilist)
{
	int32s_print (ilist, Con_PrintLinef);
}

int int32s_append_split_dequote (int32list_s *ilist, ccs *s_value, ccs *s_delimiter_elements)
{
	int num_elements_ignored = 0;

	stringlist_t rlist = {0};
	// Split the element pairs using comma split
	{
		char *s_z = Z_StrDup(s_value);
		String_Edit_DeQuote(s_z);
		stringlistappend_split (&rlist, s_z, s_delimiter_elements /*COMMA*/ );
		Z_FreeNull_ (s_z); // DISCHARGED
	}

	for (int n = 0; n < rlist.numstrings; n ++) {
		// SPLIT ON DELIMITER
		char *selem = rlist.strings[n];
		int d = atoi(selem);
		int32s_add1 (ilist, d);	
	} // while

	stringlistfreecontents(&rlist);
	
	return num_elements_ignored;
}

void floats_freecontents (floatlist_s *flist)
{
	// Unlike stringlist_t, no strings to free.
	flist->count = 0;
	flist->maxsize = 0;
	Z_FreeNull_ (flist->floats);
}


char *stringlist_join_delim_zalloc (stringlist_t *plist, ccs *s_delimiter, int shall_quote)
{
	baker_string_t *bs_malloc = stringlist_join_bkalloc (plist, s_delimiter);
	char *s_za = NULL;
	switch (shall_quote) {
	case fs_quoted_auto_2:
		if (String_Is_All_AlphaNumeric_Underscore(bs_malloc->string)) {
			s_za = Z_StrDup_Len_Z (bs_malloc->string, bs_malloc->length);
			break;
		}
		// Isn't alpha -- fall through for quoted

	case fs_quoted_true:
		s_za = Z_StrDupf (QUOTED_S, bs_malloc->string);
		break;

	default:
		s_za = Z_StrDup_Len_Z (bs_malloc->string, bs_malloc->length);
	} // sw

	// bs_malloc can't be null here, baker_string inits with an empty string alloc
	if (bs_malloc) BakerString_Destroy_And_Null_It (&bs_malloc);
	return s_za;
}

// Returns not_found_neg1 if not found
static int _stringlist_find_index (stringlist_t *p_stringlist, ccs *s_find_this, int caseinsensitive)
{
	typedef int (*comparemethod_fn_t) (const char *s1, const char *s2);
	comparemethod_fn_t compare_fn = caseinsensitive ? strcasecmp : strcmp;
	for (int idx = 0; idx < p_stringlist->numstrings; idx ++) {
		char *sxy = p_stringlist->strings[idx];
		if (compare_fn (sxy, s_find_this) == 0)
			return idx;
	} // for
	return not_found_neg1;
}

int stringlist_find_index_caseless (stringlist_t *p_stringlist, ccs *s_find_this)
{
	return _stringlist_find_index (p_stringlist, s_find_this, fs_caseless_true);
}

int stringlist_find_index (stringlist_t *p_stringlist, ccs *s_find_this)
{
	return _stringlist_find_index (p_stringlist, s_find_this, fs_caseless_false);
}

// Baker: 2024 April 20 - No callers, we were going to use for intermap, then did something else
void stringlist_from_delim (stringlist_t *p_stringlist, ccs *s_space_delimited)
{
	// This process depends on this s_space_delimited having items.
	if (s_space_delimited[0] == NULL_CHAR_0)
		return;

	const char	*s_space_delim		= " ";
	int			s_len			= (int)strlen(s_space_delimited);
	int			s_delim_len		= (int)strlen(s_space_delim);

	// Baker: This works the searchpos against s_space_delimited
	// finding the delimiter (space) and adding a list item until there are no more spaces
	// (an iteration with no space adds the rest of the string.

	// Baker: have we tested this against a single item without a space to see what happens?
	// It looks like it can handle that.

	// BUILD LIST

	int			searchpos		= 0;
	while (1) {
		char s_this_copy[MAX_INPUTLINE_16384];
		const char	*space_pos	= strstr (&s_space_delimited[searchpos], s_space_delim); // string_find_pos_start_at(s, s_delim, searchpos);
		int			endpos		= (space_pos == NULL) ? (s_len - 1) : ( (space_pos - s_space_delimited) - 1); // (commapos == not_found_neg1) ? (s_len -1) : (commapos -1);
		int			this_w		= (endpos - searchpos + 1); // string_range_width (searchpos, endpos); (endpos - startpos + 1)

		memcpy (s_this_copy, &s_space_delimited[searchpos], this_w);
		s_this_copy[this_w] = NULL_CHAR_0; // term

		stringlistappend (p_stringlist, s_this_copy);

		// If no space found, we added the rest of the string as an item, so get out!
		if (space_pos == NULL)
			break;

		searchpos = (space_pos - s_space_delimited) + s_delim_len;
	} // while
}

// Supply null or zero length string for optionals
// Returns num matches
int stringlistappend_from_dir_pattern (stringlist_t *p_stringlist, ccs *s_optional_dir_no_slash, ccs *s_optional_dot_extension, int wants_strip_extension)
{
	fssearch_t	*t;
	char		s_pattern[1024];
	int			num_matches = 0;
	int			j;
	int			is_ext = s_optional_dot_extension && s_optional_dot_extension[0];

	const char	*s_ext = is_ext ? s_optional_dot_extension : "";

	if (s_optional_dir_no_slash) {
			// "%s/*%s"
			c_strlcpy (s_pattern, s_optional_dir_no_slash);
			c_strlcat (s_pattern, "/");
			c_strlcat (s_pattern, "*");
			c_strlcat (s_pattern, s_ext);
	}
	else	c_dpsnprintf1 (s_pattern, "*%s", s_ext);
	
	t = FS_Search(s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (t && t->numfilenames > 0) {
		for (j = 0; j < t->numfilenames; j ++) {
			char *sxy = t->filenames[j];
			if (wants_strip_extension)
				File_URL_Edit_Remove_Extension (sxy);
			stringlistappend (p_stringlist, sxy);
			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}



