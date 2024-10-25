
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


void voids_add1 (voidlist_s *vlist, const void *p)
{
	int newcount = vlist->count + 1 + 1; // + 1 null terminated last entry
	if (newcount >= vlist->maxsize) {
		vlist->maxsize += 4096;
		vlist->vloats = (const void **)Mem_Realloc (zonemempool, vlist->vloats, sizeof(ccs *) * vlist->maxsize);
	}
	
	vlist->vloats[vlist->count++] = (const void *)p;
}

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

void voids_freecontents (voidlist_s *vlist)
{
	// Unlike stringlist_t, no strings to free.
	vlist->count = 0;
	vlist->maxsize = 0;
	Z_FreeNull_ (vlist->vloats);
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



// SEPT 28 2024


//
// brushrowlist -- entry free, list free, list add
//

static void brushrow_free(brushrow_s *p_brushrow)
{
	Mem_FreeNull_ (p_brushrow->pbrtexture);
}


void brushrowlistfreecontents (brushrowlist_t *list)
{
	for (int j = 0;j < list->count; j++) {
		brushrow_free (&list->brushrow[j]);
	}
	list->count = list->maxsize = 0;
	Mem_FreeNull_ (list->brushrow);
}

// ADD .... brush_s *p_brushrow = brushrowlist_add(&p_brush->brushrowlist);
brushrow_s *brushrowlist_add (brushrowlist_t *list)
{
	if (list->count >= list->maxsize) {
		list->maxsize += 4096;
		list->brushrow = (brushrow_s *)Mem_Realloc(zonemempool, 
			list->brushrow, list->maxsize * sizeof(brushrow_s) );
	}
	
	brushrow_s *p_brushrow = &list->brushrow[list->count ++]; // Incremented count
	return p_brushrow;
}

//
// brushlist -- entry free, list free, list add
//

static void brush_free(brush_s *p_brush)
{
//typedef struct _brush_st {
//	brushrowlist_t	brushrowlist;
//} brush_s;

	brushrowlistfreecontents	(&p_brush->brushrowlist);

	// As patch
	Mem_FreeNull_ (p_brush->texture);
	patchrowlistfreecontents	(&p_brush->patchrowlist);


}

void brushlistfreecontents(brushlist_t *list)
{
	for (int j = 0;j < list->count; j++) {
		brush_free (&list->brush[j]);
	}
	list->count = list->maxsize = 0;
	Mem_FreeNull_ (list->brush);
}

// ADD .... brush_s *p_brush = brushlist_add(&p_ent->brushlist);
brush_s *brushlist_add (brushlist_t *list)
{
	if (list->count >= list->maxsize) {
		list->maxsize += 4096;
		list->brush = (brush_s *)Mem_Realloc(zonemempool, 
			list->brush, list->maxsize * sizeof(brush_s) );		
	}
	
	brush_s *p_brush = &list->brush[list->count ++]; // Incremented count
	return p_brush;
}

//
// patchrowlist -- entry free, list free, list add
//

static void patchrow_free(patchrow_s *p_patchrow)
{
	// We do nothing!
}


void patchrowlistfreecontents (patchrowlist_t *list)
{
	for (int j = 0;j < list->count; j++) {
		patchrow_free (&list->patchrow[j]);
	}
	list->count = list->maxsize = 0;
	Mem_FreeNull_ (list->patchrow);
}

// ADD .... patch_s *p_patchrow = patchrowlist_add(&p_patch->patchrowlist);
patchrow_s *patchrowlist_add (patchrowlist_t *list)
{
	if (list->count >= list->maxsize) {
		list->maxsize += 4096;
		list->patchrow = (patchrow_s *)Mem_Realloc(zonemempool, 
			list->patchrow, list->maxsize * sizeof(patchrow_s) );
	}
	
	patchrow_s *p_patchrow = &list->patchrow[list->count ++]; // Incremented count
	return p_patchrow;
}

//
// entitylist -- entry free, list free, list add
//

static void entity_free(entityx_t *p_ent)
{
	stringlistfreecontents	(&p_ent->pairslist);
	brushlistfreecontents	(&p_ent->brushlist);
}

WARP_X_ (stringlistappend)

void entitylistfreecontents(entitylist_t *list)
{
	for (int j = 0;j < list->count; j++) {
		entity_free (&list->entity[j]);
	}
	list->count = list->maxsize = 0;
	Mem_FreeNull_ (list->entity);
}

// ADD .... entityx_t *p_ent = entitylist_add(&myentities_list);
entityx_t *entitylist_add (entitylist_t *list)
{
	if (list->count >= list->maxsize) {
		list->maxsize += 4096;
		list->entity = (entityx_t *)Mem_Realloc(zonemempool, 
			list->entity, list->maxsize * sizeof(entityx_t) );		
	}
	
	entityx_t *pe = &list->entity[list->count ++]; // Incremented count
	return pe;
}

entityx_t *entitylist_add_at_1_shiftup (entitylist_t *list)
{
	int newidx = list->count; // live: 480 

	// 0
	// 1
	// 99
	// 100 <--- us (newidx)
	// we move 1 to 99 to 2 100 ...
	entityx_t *pstructfake = entitylist_add(list);


	// Baker: This is a page array.  These aren't pointers!
	size_t startidx = 1;
	size_t move_num_entries = /*beyond*/ newidx /*100*/ - startidx /*1*/ ; // Result 99 is size of move

	entityx_t *pstart1 = &list->entity[1];
	entityx_t *p2 = &list->entity[2];
	entityx_t *p479 = &list->entity[newidx - 1];
	entityx_t *p480 = &list->entity[newidx];
	size_t struct_size = sizeof(*pstart1); // 1 to 479 get moved to 2 thru 480.  This 479 entries.
	
	entityx_t *pdest  = &list->entity[2];
	size_t movesize = struct_size * move_num_entries;
	memmove (p2, pstart1, movesize);
	entityx_t *pe  = &list->entity[1];
	memset (pe, 0, sizeof(*pe));
	return pe;
}



// BakerString_Destroy_And_Null_It (&bs);
void entitylist_to_clipboard (entitylist_t *plist)
{
	baker_string_t *bsa = entitylist_maptext_bsalloc (plist);
#ifdef _DEBUG
	int _Platform_Clipboard_Set_Text (const char *text_to_clipboard);
	_Platform_Clipboard_Set_Text (bsa->string);
#else
	Clipboard_Set_Text (bsa->string);
#endif
	//Con_PrintLinef ("Clipboarded it strlen = %d -- done", (int)bsa->length);
	BakerString_Destroy_And_Null_It (&bsa);
}

baker_string_t *entitylist_maptext_bsalloc (entitylist_t *plist)
{
//#pragma message ("Add a Zircon preprocess key to the worldspawn keys")
	baker_string_t *bs_maptext = BakerString_Create_Malloc ("");

	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		// Print keys
		//BakerString_CatCFmt (bs_maptext, "{ // Entity %d" NEWLINE, ex);
		BakerString_CatCFmt (bs_maptext, "{" NEWLINE);
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			ccs *val = p_ent->pairslist.strings[kx + 1];
			char sline[4096];
			c_dpsnprintf2 (sline, QUOTED_S " " QUOTED_S, key, val);
			BakerString_CatCFmt (bs_maptext, "%s" NEWLINE, sline);
		} // epairs
		for (int bx = 0; bx < p_ent->brushlist.count; bx ++) {
			brush_s *p_brush = &p_ent->brushlist.brush[bx];
			
			BakerString_CatCFmt (bs_maptext, "{" NEWLINE);

			if (p_brush->is_a_patch == false) {
				// REAL BRUSH
				for (int brow = 0; brow < p_brush->brushrowlist.count; brow ++) {
					brushrow_s *pbr = &p_brush->brushrowlist.brushrow[brow];

					BakerString_CatCFmt (bs_maptext, 					
						"( " VECTOR3_G  " ) " 
						"( " VECTOR3_G  " ) " 
						"( " VECTOR3_G  " ) "
						" %s "
						"[ " VECTOR4_G " ] "
						"[ " VECTOR4_G " ] "
						VECTOR3_G,
						 VECTOR3_SEND(pbr->a),
						 VECTOR3_SEND(pbr->b),
						 VECTOR3_SEND(pbr->c),
						 pbr->pbrtexture,
						 VECTOR4_SEND(pbr->xtra1),
						 VECTOR4_SEND(pbr->xtra2),
						 VECTOR3_SEND(pbr->ftrail)
						 );

					// Any extras add a space
					for (int j = 3; j < pbr->trail_count; j ++) {
						BakerString_CatCFmt (bs_maptext, " " FLOAT_LOSSLESS_FORMAT, pbr->ftrail[j] );
					}
					BakerString_CatC (bs_maptext, NEWLINE);
				} // brushrow
			} else {
i_am_patch:
					BakerString_CatCFmt (bs_maptext, "patchDef2" NEWLINE);
					BakerString_CatCFmt (bs_maptext, "{" NEWLINE);
					// caves/3i_sand2
					BakerString_CatCFmt (bs_maptext, "%s" NEWLINE, p_brush->texture);
					// ( 9 3 0 0 0 )
					BakerString_CatCFmt (bs_maptext, "( " FLOAT_LOSSLESS_FORMAT " " FLOAT_LOSSLESS_FORMAT " " FLOAT_LOSSLESS_FORMAT " " FLOAT_LOSSLESS_FORMAT " " FLOAT_LOSSLESS_FORMAT " )" NEWLINE, p_brush->rows, p_brush->cols, p_brush->other3[0], p_brush->other3[1], p_brush->other3[2]);
					BakerString_CatCFmt (bs_maptext, "(" NEWLINE);
		
					for (int prow = 0; prow < p_brush->patchrowlist.count; prow ++) {
						patchrow_s *ppr = &p_brush->patchrowlist.patchrow[prow];
						BakerString_CatCFmt (bs_maptext, "( " NEWLINE);
						for (int pcol = 0; pcol < p_brush->cols; pcol ++) {
							// These don't perfectly match J.A.C.K. output which has extra zeroes
							// in the decimal places sometimes.
							BakerString_CatCFmt (bs_maptext, "( %g %g %g %8g %8g )" NEWLINE,
								ppr->flots[pcol * 5 + 0],
								ppr->flots[pcol * 5 + 1],
								ppr->flots[pcol * 5 + 2],
								ppr->flots[pcol * 5 + 3],
								ppr->flots[pcol * 5 + 4]
							);
						}
						BakerString_CatCFmt (bs_maptext, " )" NEWLINE);
					}
					
					BakerString_CatCFmt (bs_maptext, ")" NEWLINE);
					BakerString_CatCFmt (bs_maptext, "}" NEWLINE);
			}

			BakerString_CatCFmt (bs_maptext, "}" NEWLINE);
		} // brush
		BakerString_CatCFmt (bs_maptext, "}" NEWLINE);
	} // entities in .map
	return bs_maptext;
}

// Returns NULL or value for key for the entity
ccs *entitykeys_find_value (entityx_t *e, ccs *keyname)
{
	// list_map1.entity[0]
	for (int j = 0; j < e->pairslist.numstrings; j += 2) {
		ccs *key = e->pairslist.strings[j + 0];
		ccs *val = e->pairslist.strings[j + 1];
		if (String_Match (key, keyname))
			return val;
	}
	return NULL;
}

int entity_key_idx_for_name (entityx_t *r_ent, ccs *keyname_wanted)
{
	for (int kx = 0; kx < r_ent->pairslist.numstrings; kx += 2) {
		ccs *key = r_ent->pairslist.strings[kx + 0];
		ccs *val = r_ent->pairslist.strings[kx + 1];
		if (String_Match(key, keyname_wanted))
			return kx;
	} // each epair

	return not_found_neg1;
}


ccs *entity_key_get_value (entityx_t *r_ent, ccs *keyname_wanted)
{
	for (int kx = 0; kx < r_ent->pairslist.numstrings; kx += 2) {
		ccs *key = r_ent->pairslist.strings[kx + 0];
		ccs *val = r_ent->pairslist.strings[kx + 1];
		if (String_Match (key, keyname_wanted))
			return val;
	}
	return NULL;
}

int entity_key_idx_set_value_is_ok (entityx_t *r_ent, int keyidx, ccs *snewval)
{
	ccs *key = r_ent->pairslist.strings[keyidx + 0];
	ccs *val = r_ent->pairslist.strings[keyidx + 1];
	Mem_FreeNull_ (r_ent->pairslist.strings[keyidx + 1]);
	r_ent->pairslist.strings[keyidx + 1] = Z_StrDup (snewval);
	return true;
}

int entity_key_set_value_is_ok (entityx_t *r_ent, ccs *keyname_wanted, ccs *value_to_set)
{
	int keyidx = entity_key_idx_for_name (r_ent, keyname_wanted);
	if (keyidx == not_found_neg1)
		return false;
	return entity_key_idx_set_value_is_ok (r_ent, keyidx, value_to_set);
}


void entitylist_translate_epairs (entitylist_t *plist, vec3_t vadd)
{
	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			if (String_Match(key,"origin") == false)
				continue;
			ccs *val = p_ent->pairslist.strings[kx + 1];

			vec3_t vorg;
			COM_Parse_Basic(&val);	vorg[0] = atof (com_token);
			COM_Parse_Basic(&val);	vorg[1] = atof (com_token);
			COM_Parse_Basic(&val);	vorg[2] = atof (com_token);
			VectorAdd (vadd, vorg, vorg); // Translate
			
			char *newval = Z_StrDupf (VECTOR3_LOSSLESS, VECTOR3_SEND(vorg));
			Mem_FreeNull_ (p_ent->pairslist.strings[kx + 1]);
			p_ent->pairslist.strings[kx + 1] = newval;

			// ASSUME MAXIMUM OF A SINGLE "origin" per entity, 
			// SO ..
			break;	// GET OUT!
		} // epairs
	} // entities in .map
}

// For brushes with targ
#if 0 // It's a good idea.
void entitylist_origin_brush_ensure (entitylist_t *plist, ccs *id)
{
	&list_paste, "main");
}
#endif

void entitylist_translate_brushes (entitylist_t *plist, vec3_t vadd)
{
	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];

		for (int bx = 0; bx < p_ent->brushlist.count; bx ++) {
			brush_s *p_brush = &p_ent->brushlist.brush[bx];
			if (p_brush->is_a_patch == false) {
				// REAL BRUSH
				for (int brow = 0; brow < p_brush->brushrowlist.count; brow ++) {
					brushrow_s *pbr = &p_brush->brushrowlist.brushrow[brow];
					VectorAdd (vadd, pbr->a, pbr->a); // Translate
					VectorAdd (vadd, pbr->b, pbr->b); // Translate
					VectorAdd (vadd, pbr->c, pbr->c); // Translate
#if 1 // OCTOBER 3 2024 - TEXTURELOCK
					//             matrix factor * distance * 
					// [ 1 0 0 459.715 ] [ 0 0 -1 -122.714 ] 0 0.7 0.35 134217728 0 0
					//if (String_Contains(pbr->pbrtexture, "armu")) {
					//	int j = 5;
					//}
					double xscale  = pbr->ftrail[1];  // 
					double yscale  = pbr->ftrail[2]; 
#if 0
					double xshift = pbr->xtra1[0] * vadd[0] / xscale;
					double yshift = pbr->xtra2[0] * vadd[2] / yscale;
#else
					double xshift = (pbr->xtra1[0] * -vadd[0] + pbr->xtra1[1] * -vadd[1] + pbr->xtra1[2] * -vadd[2]) / xscale;
					double yshift = (pbr->xtra2[0] * -vadd[0] + pbr->xtra2[1] * -vadd[1] + pbr->xtra2[2] * -vadd[2]) / yscale;
#endif
					double newxshift = pbr->xtra1[3] + xshift;
					double newyshift = pbr->xtra2[3] + yshift;
					pbr->xtra1[3] = newxshift;
					pbr->xtra2[3] = newyshift;
					
#endif
				} // row
			} else {
				// PATCH - Translate all vertexes
				for (int prow = 0; prow < p_brush->patchrowlist.count; prow ++) {
					patchrow_s *ppr = &p_brush->patchrowlist.patchrow[prow];
					for (int pcol = 0; pcol < p_brush->cols; pcol ++) {
						float *pvec = &ppr->flots[pcol * 5 + 0];
						VectorAdd (vadd, pvec, pvec); // Translate
					} // col
				} // row
			} // if
		} // brush		
	} // entities in .map
}

// This is brush paste.
void entitylist_brush0_append (entitylist_t *plist, entitylist_t *paste)
{
	entityx_t	*r_ent = &paste->entity[0];	// R = READ
	entityx_t	*d_ent = &plist->entity[0];	// D = DEST

	for (int bx = 0; bx < r_ent->brushlist.count; bx ++) {
		brush_s *r_brush = &r_ent->brushlist.brush[bx];	// READ BRUSH FROM SRC
		brush_s *d_brush = brushlist_add (&d_ent->brushlist); // CREATE BRUSH FOR DEST

		d_brush->is_a_patch = r_brush->is_a_patch;

		if (r_brush->is_a_patch) {
			// If patch, copy patch stuff ...
			d_brush->texture = Z_StrDup (r_brush->texture);
			d_brush->rows = r_brush->rows;
			d_brush->cols = r_brush->cols;
			d_brush->other3[0] = r_brush->other3[0];
			d_brush->other3[1] = r_brush->other3[1];
			d_brush->other3[2] = r_brush->other3[2];
		}

		if (r_brush->is_a_patch == false) {
			// REAL BRUSH
			for (int brow = 0; brow < r_brush->brushrowlist.count; brow ++) {
				brushrow_s *rbr = &r_brush->brushrowlist.brushrow[brow];
				brushrow_s *dbr = brushrowlist_add(&d_brush->brushrowlist);

				memcpy (dbr, rbr, sizeof(*rbr));
				dbr->pbrtexture = Z_StrDup (rbr->pbrtexture);
			} // brushrow
		} else {
			// PATCH		
			for (int prow = 0; prow < r_brush->patchrowlist.count; prow ++) {
				patchrow_s *rpr = &r_brush->patchrowlist.patchrow[prow];
				patchrow_s *dpr = patchrowlist_add (&d_brush->patchrowlist);

				// 5 x number of columns - mem copy all the numbers
				size_t size_to_copy = sizeof(dpr->flots); // Should be about 1200
//				int j = 5;
				memcpy (dpr->flots, rpr->flots, size_to_copy);
			}
		} // brush
	} // brushes in entity 0
}

void entitylist_nonworld_append (entitylist_t *plist, entitylist_t *paste)
{
	// ADD PASTE ENTS TO PLIST
	for (int rex = 1; rex < paste->count; rex ++) { // SKIP ENTITY 0
		entityx_t	*r_ent = &paste->entity[rex];	// R = READ
		entityx_t	*d_ent = entitylist_add(plist);	// D = DEST

		for (int kx = 0; kx < r_ent->pairslist.numstrings; kx += 2) {
			ccs *key = r_ent->pairslist.strings[kx + 0];
			ccs *val = r_ent->pairslist.strings[kx + 1];

			stringlistappend (&d_ent->pairslist, key); // KEY
			stringlistappend (&d_ent->pairslist, val); // VALUE
		} // epairs

		for (int bx = 0; bx < r_ent->brushlist.count; bx ++) {
			brush_s *r_brush = &r_ent->brushlist.brush[bx];	// READ BRUSH FROM SRC
			brush_s *d_brush = brushlist_add (&d_ent->brushlist); // CREATE BRUSH FOR DEST

			d_brush->is_a_patch = r_brush->is_a_patch;

			if (r_brush->is_a_patch) {
				// If patch, copy patch stuff ...
				d_brush->texture = Z_StrDup (r_brush->texture);
				d_brush->rows = r_brush->rows;
				d_brush->cols = r_brush->cols;
				d_brush->other3[0] = r_brush->other3[0];
				d_brush->other3[1] = r_brush->other3[1];
				d_brush->other3[2] = r_brush->other3[2];
			}

			if (r_brush->is_a_patch == false) {
				// REAL BRUSH
				for (int brow = 0; brow < r_brush->brushrowlist.count; brow ++) {
					brushrow_s *rbr = &r_brush->brushrowlist.brushrow[brow];
					brushrow_s *dbr = brushrowlist_add(&d_brush->brushrowlist);

					memcpy (dbr, rbr, sizeof(*rbr));
					dbr->pbrtexture = Z_StrDup (rbr->pbrtexture);
				} // brushrow
			} else {
				// PATCH		
				for (int prow = 0; prow < r_brush->patchrowlist.count; prow ++) {
					patchrow_s *rpr = &r_brush->patchrowlist.patchrow[prow];
					patchrow_s *dpr = patchrowlist_add (&d_brush->patchrowlist);

					// 5 x number of columns - mem copy all the numbers
					size_t size_to_copy = sizeof(dpr->flots); // Should be about 1200
//					int j = 5;
					memcpy (dpr->flots, rpr->flots, size_to_copy);
				}
			} // brush
		} // brushes in entity
	} // entities in .map
}


// For all brushes except world (world cannot have origin brushes)
// find entity key "_originmake"
// If so, convert brushes to origin.  Or just make one.

void entity_add_brush_origin_from_bbox (entityx_t *p_ent, vec3_t brmins, vec3_t brmaxs)
{
	brush_s *p_brush = brushlist_add (&p_ent->brushlist); // CREATE BRUSH FOR DEST

#define XH brmaxs[0]
#define YH brmaxs[1]
#define ZH brmaxs[2]
#define XL brmins[0]
#define YL brmins[1]
#define ZL brmins[2]


	brushrow_s *br = brushrowlist_add (&p_brush->brushrowlist);
	//          ( [XH] [YH] [ZH] )                ( [XH] [YH] [ZL] )          ( [XH] [YL] [ZH] ) 
	VectorSet (br->a, XH, YH, ZH); VectorSet (br->b, XH, YH, ZL); VectorSet (br->c, XH, YL, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	//553648128 16512 0
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&p_brush->brushrowlist); // 2 - all numbers identical to 1 except abc
	//( [XL] [YL] [ZH] ) ( [XL] [YL] [ZL] ) ( [XL] [YH] [ZH] ) common/caulk [ 0 1 0 -20.4 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YL, ZH); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XL, YH, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&p_brush->brushrowlist); // 3
	//( [XH] [YL] [ZH] ) ( [XH] [YL] [ZL] ) ( [XL] [YL] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XH, YL, ZL); VectorSet (br->c, XL, YL, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&p_brush->brushrowlist); // 4 - all numbers identical to 3 except abc
	// ( [XL] [YH] [ZH] ) ( [XL] [YH] [ZL] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZH); VectorSet (br->b, XL, YH, ZL); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&p_brush->brushrowlist); // 5
	// ( [XL] [YH] [ZL] ) ( [XL] [YL] [ZL] ) ( [XH] [YH] [ZL] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZL); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XH, YH, ZL);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");
	
	br = brushrowlist_add (&p_brush->brushrowlist); // 6 - all ident to 5 except a b c
	// ( [XH] [YL] [ZH] ) ( [XL] [YL] [ZH] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XL, YL, ZH); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

#undef XH
#undef YH
#undef ZH
#undef XL
#undef YL
#undef ZL

//{
//( -2045.41 193 129 ) ( -2045.41 193 127 ) ( -2045.41 191 129 ) common/origin [ 0 1 0 -40 ] [ 0 0 -1 0 ] 0 0.5 0.5 553648128 17536 0
//( -2049 191 129 ) ( -2049 191 127 ) ( -2049 193 129 ) common/origin [ 0 1 0 -40 ] [ 0 0 -1 0 ] 0 0.5 0.5 553648128 17536 0
//( -2045.41 191 129 ) ( -2045.41 191 127 ) ( -2049 191 129 ) common/origin [ 1 0 0 16 ] [ 0 0 -1 0 ] 0 0.5 0.5 553648128 17536 0
//( -2049 193 129 ) ( -2049 193 127 ) ( -2045.41 193 129 ) common/origin [ 1 0 0 16 ] [ 0 0 -1 0 ] 0 0.5 0.5 553648128 17536 0
//( -2049 193 127 ) ( -2049 191 127 ) ( -2045.41 193 127 ) common/origin [ 1 0 0 16 ] [ 0 -1 0 40 ] 0 0.5 0.5 553648128 17536 0
//( -2049 191 129 ) ( -2049 193 129 ) ( -2045.41 191 129 ) common/origin [ 1 0 0 16 ] [ 0 -1 0 40 ] 0 0.5 0.5 553648128 17536 0
//}
// We add an origin brush to the entity.
}

void bounds_expand (const vec3_t v, vec3_t loz, vec3_t hiz)
{
	if (v[0] > hiz[0])  hiz[0] = v[0];
	if (v[1] > hiz[1])  hiz[1] = v[1];
	if (v[2] > hiz[2])  hiz[2] = v[2];

	if (v[0] < loz[0])  loz[0] = v[0];
	if (v[1] < loz[1])  loz[1] = v[1];
	if (v[2] < loz[2])  loz[2] = v[2];
}

int entity_make_origin_brush_num_made (entityx_t *p_ent, ccs *s0_plus_timestamp)
{
	int num_made = 0;
	
	vec3_t brmins = { 999999,  999999,  999999};
	vec3_t brmaxs = {-999999, -999999, -999999};

	for (int bx = 0; bx < p_ent->brushlist.count; bx ++) {
		brush_s *p_brush = &p_ent->brushlist.brush[bx];

		if (p_brush->is_a_patch)
			continue; // PATCH, SKIP

		if (p_brush->brushrowlist.count && num_made == 0)
			num_made = 1;

		// REAL BRUSH
		for (int brow = 0; brow < p_brush->brushrowlist.count; brow ++) {
			brushrow_s *pbr = &p_brush->brushrowlist.brushrow[brow];
			bounds_expand (pbr->a, brmins, brmaxs);
			bounds_expand (pbr->b, brmins, brmaxs);
			bounds_expand (pbr->c, brmins, brmaxs);
		} // brush row
	} // brush

	// ADD A ORIGIN BRUSH from MINS/MAXS BBOX
	if (num_made) {
		entity_add_brush_origin_from_bbox (p_ent, brmins, brmaxs);
		entity_key_set_value_is_ok (p_ent, "_originmake", s0_plus_timestamp);
		//char *newvaluestring = Z_StrDup (svaluetowrite);
		//Mem_FreeNull_ (*pval);
		//*pval = newvaluestring;
	}
	return num_made;
}

// svaluetowrite looks like "0 - 20241005 10:12 AM"
// We set _originmake value to "0 - 20241005 10:12 AM" so it is known it happened.
// if you read the .map source.
int entitylist__originmake_num_made (entitylist_t *plist, ccs *s0_plus_timestamp)
{
	int num_made = 0;
	for (int ex = 1 /*after world!*/; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		char **pval = NULL;
		//int val_idx = not_found_neg1;
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			ccs *val = p_ent->pairslist.strings[kx + 1];

			// It "_originmake" value has anything that is NOT a leading 0
			// "" or "value" or "1"
			// ... we run it

			// "0" or "0 - something here" ... we do not run it.
			if (String_Match(key, "_originmake")) {
				if (val[0] != '0') {
					if (entity_key_idx_for_name (p_ent, "_atomize") != not_found_neg1) {
						pval = NULL; // NEVER DO THIS ENTITY
						break;
					}
					pval = &p_ent->pairslist.strings[kx + 1]; //wants__originmake = true;
					break;
				}
			}
		} // epairs

		if (pval == NULL /*wants__originmake == false*/)
			continue;

		// SO WE 
		vec3_t brmins = { 999999,  999999,  999999};
		vec3_t brmaxs = {-999999, -999999, -999999};

		int are_any_brushes = false;

		for (int bx = 0; bx < p_ent->brushlist.count; bx ++) {
			brush_s *p_brush = &p_ent->brushlist.brush[bx];

			if (p_brush->is_a_patch)
				continue; // PATCH, SKIP

			if (p_brush->brushrowlist.count && are_any_brushes == false)
				are_any_brushes = true;

			// REAL BRUSH
			for (int brow = 0; brow < p_brush->brushrowlist.count; brow ++) {
				brushrow_s *pbr = &p_brush->brushrowlist.brushrow[brow];
				bounds_expand (pbr->a, brmins, brmaxs);
				bounds_expand (pbr->b, brmins, brmaxs);
				bounds_expand (pbr->c, brmins, brmaxs);
			} // brush row
		} // brush

		// ADD A ORIGIN BRUSH from MINS/MAXS BBOX
		if (are_any_brushes) {
			num_made ++;
			entity_add_brush_origin_from_bbox (p_ent, brmins, brmaxs);
			

#if 1
			entity_key_set_value_is_ok (p_ent, "_originmake", s0_plus_timestamp);
#else
			char *newvaluestring = Z_StrDup (svaluetowrite);
			Mem_FreeNull_ (*pval);
			*pval = newvaluestring;
#endif
		}

	} // entities in .map


	return num_made;
}


int entitylist_atomize_entities_num_made (entitylist_t *plist, ccs *s0_plus_timestamp)
{
	int num_made = 0;
	for (int ex = 1 /*after world!*/; ex < plist->count; ex ++) {
		entityx_t	*r_ent = &plist->entity[ex];

		// Check early out scenario ...
		if (r_ent->brushlist.count == 0)
			continue; // No brushes

		int brush2ent_idx = entity_key_idx_for_name (r_ent, "_atomize");

		if (brush2ent_idx == not_found_neg1)
			continue; // "_atomize" Does not exist for entity.
		
		ccs *val = r_ent->pairslist.strings[brush2ent_idx + 1];

		// It "_atomize" value has anything that is NOT a leading 0 -- ... we run it
		// "" or "value" or "1" ... we run those
		// "0" or "0 - something here" ... we do not run it.
		if (val[0] == '0')
			continue; // Leading zero .. do not run

clone_brush_or_patch:
		for (int bx = 0; bx < r_ent->brushlist.count; bx ++) {
			brush_s *r_brush = &r_ent->brushlist.brush[bx];

			// TURN THIS BRUSH INTO A NEW ENTITY.
			entityx_t *d_ent = entitylist_add(plist);

			// STAGE: COPY ENTITY KEYS/VALUES except "_atomize"
			for (int kx = 0; kx < r_ent->pairslist.numstrings; kx += 2) {
				ccs *key = r_ent->pairslist.strings[kx + 0];
				ccs *val = r_ent->pairslist.strings[kx + 1];

				// Don't write "_atomize"
				if (String_Match (key, "_atomize"))
					continue;

				stringlistappend (&d_ent->pairslist, key); // KEY
				stringlistappend (&d_ent->pairslist, val); // VALUE
			} // epairs

			brush_s *d_brush = brushlist_add (&d_ent->brushlist); // CREATE BRUSH FOR DEST

			d_brush->is_a_patch = r_brush->is_a_patch; // Yes, but we aren't a patch.

			// STAGE: COPY PATCH STUFF STORED IN R_BRUSH  ...
			if (r_brush->is_a_patch) {
				// If patch, copy patch stuff ...
				d_brush->texture = Z_StrDup (r_brush->texture);
				d_brush->rows = r_brush->rows;
				d_brush->cols = r_brush->cols;
				d_brush->other3[0] = r_brush->other3[0];
				d_brush->other3[1] = r_brush->other3[1];
				d_brush->other3[2] = r_brush->other3[2];
			}

			// STAGE: COPY BRUSH OR PATCH ROWS
			if (r_brush->is_a_patch == false) {
				// REAL BRUSH
				for (int brow = 0; brow < r_brush->brushrowlist.count; brow ++) {
					brushrow_s *rbr = &r_brush->brushrowlist.brushrow[brow];
					brushrow_s *dbr = brushrowlist_add(&d_brush->brushrowlist);

					memcpy (dbr, rbr, sizeof(*rbr));
					dbr->pbrtexture = Z_StrDup (rbr->pbrtexture);
				} // brushrow
			} else {
				// PATCH		
				for (int prow = 0; prow < r_brush->patchrowlist.count; prow ++) {
					patchrow_s *rpr = &r_brush->patchrowlist.patchrow[prow];
					patchrow_s *dpr = patchrowlist_add (&d_brush->patchrowlist);

					// 5 x number of columns - mem copy all the numbers
					size_t size_to_copy = sizeof(dpr->flots); // Should be about 1200
//					int j = 5;
					memcpy (dpr->flots, rpr->flots, size_to_copy);
				}
			} // brush
		} // brushes in entity

		// THIS ENTITY HAD ATOMIZATION ... CHANGE THE CLASSNAME "_func_wall_processed"
		num_made ++;

		ccs *s_classname =  entity_key_get_value(r_ent, "classname");
		int isok = true;
		if (s_classname) {
			va_super (snewval, /*slen*/ 64, "_" "%s" "_processed", s_classname);

			
			// 1. CHANGE CLASSNAME.  (WHY?  So Quake doesn't know what it is and ignores it.)
			//       WHY?  Not ready to delete entities yet.
			// 2. CHANGE "_atomize" to 0
			isok = entity_key_set_value_is_ok(r_ent, "classname", snewval);
		}
		if (!isok) {
			int j = 5;
		}
		isok = entity_key_set_value_is_ok(r_ent, "_atomize", s0_plus_timestamp);
		if (!isok) {
			int j = 5;
		}
	} // entities in .map

	return num_made;
}


void entitylist_set_replace_key_val (entitylist_t *plist, int entnum, ccs *key_force, ccs *val_force)
{
	entityx_t *p_ent = &plist->entity[entnum];
	int did_set = false;

	for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
		ccs *key = p_ent->pairslist.strings[kx + 0];

		if (String_Match(key, key_force) == false)
			continue;
		//ccs *val = p_ent->pairslist.strings[kx + 1];
		
		char *newval = Z_StrDup (val_force);
		Mem_FreeNull_ (p_ent->pairslist.strings[kx + 1]);
		p_ent->pairslist.strings[kx + 1] = newval;
		did_set = true;

		// ASSUME MAXIMUM OF A SINGLE "origin" per entity, 
		// SO ..
		break;	// GET OUT!
	} // epairs
	
	if (!did_set) {
		// COULDN'T FIND FOR THIS ENTITY, ADD IT
		stringlistappend (&p_ent->pairslist, key_force); // KEY
		stringlistappend (&p_ent->pairslist, val_force); // VALUE
	}
}

void entitylist_nonworld_setthis (entitylist_t *plist, ccs *key_force, ccs *val_force)
{
	for (int ex = 1; ex < plist->count; ex ++) { // NON-WORLD, WE START AT 1
		entityx_t	*p_ent = &plist->entity[ex];
		
		int did_set = false;

		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];

			if (String_Match(key, key_force) == false)
				continue;
			//ccs *val = p_ent->pairslist.strings[kx + 1];
			
			char *newval = Z_StrDup (val_force);
			Mem_FreeNull_ (p_ent->pairslist.strings[kx + 1]);
			p_ent->pairslist.strings[kx + 1] = newval;
			did_set = true;

			// ASSUME MAXIMUM OF A SINGLE "origin" per entity, 
			// SO ..
			break;	// GET OUT!
		} // epairs

		if (!did_set) {
			// COULDN'T FIND FOR THIS ENTITY, ADD IT
			stringlistappend (&p_ent->pairslist, key_force); // KEY
			stringlistappend (&p_ent->pairslist, val_force); // VALUE
		}
	} // entities in .map
}

void entitylist_nonworld_set (entitylist_t *plist, stringlist_t *plistpairset)
{
	for (int kx = 0; kx < plistpairset->numstrings; kx += 2) {
		ccs *key = plistpairset->strings[kx + 0];
		ccs *val = plistpairset->strings[kx + 1];
		entitylist_nonworld_setthis (plist, key, val);
	} // Each set
}

// target and targetname
// e11 ---> around_e11

void entitylist_prefix_epairs (entitylist_t *plist, ccs *prefix)
{
	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			ccs *val = p_ent->pairslist.strings[kx + 1];

			if (String_Isin2(key,"targetname","target")== false )
				continue;

			char *newval = Z_StrDupf ("%s%s", prefix, val);
			Mem_FreeNull_ (p_ent->pairslist.strings[kx + 1]);
			p_ent->pairslist.strings[kx + 1] = newval;

		} // epairs
	} // entities in .map
}

void entitylist_prefix_epairslist (entitylist_t *plist, ccs *prefix, stringlist_t *plist_prefixes)
{
	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			
			//if (String_Isin2(key,"targetname","target")== false )
			//	continue;

			int match_idx = stringlist_find_index (plist_prefixes, key);

			if (match_idx == not_found_neg1)
				continue;

			ccs *val = p_ent->pairslist.strings[kx + 1];

			char *newval = Z_StrDupf ("%s%s", prefix, val);
			Mem_FreeNull_ (p_ent->pairslist.strings[kx + 1]);
			p_ent->pairslist.strings[kx + 1] = newval;

		} // epairs
	} // entities in .map
}

// Starts with
int entitylist_epairs_find_model_gen_entitynum (entitylist_t *plist, ccs *prefix)
{
	for (int ex = 0; ex < plist->count; ex ++) {
		entityx_t	*p_ent = &plist->entity[ex];
		for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
			ccs *key = p_ent->pairslist.strings[kx + 0];
			if (String_Starts_With(key, prefix /*"model_gen"*/))
				return ex;
		} // epairs
	} // entities in .map
	return not_found_neg1;
}

// make car brushes
static void genbrush (entitylist_t *plist, ccs *s)
{
// "models/vehicles/psx/van_car04.md3 188.9375 101.8125 82.25"
// For each
// Make a func_wall_modelgen
// "size" "INSERT"
// "mdl" "INSERT"
// in QuakeC, solid 0
	//entityx_t	*p_ent_world = &plist->entity[0];

	// Insert as first entity
	entityx_t	*d_ent = entitylist_add_at_1_shiftup(plist);

	// We want to be entity #1 so that we exist before all other entities except world.

	ccs *text = s;
	char model[MAX_QPATHX2_256];
	vec3_t vsize;
	COM_Parse_Basic(&text);	c_strlcpy (model, com_token);
	COM_Parse_Basic(&text);	vsize[0] = atof(com_token);
	COM_Parse_Basic(&text);	vsize[1] = atof(com_token);
	COM_Parse_Basic(&text);	vsize[2] = atof(com_token);

	stringlistappend	(&d_ent->pairslist, "classname"); // KEY
	stringlistappend	(&d_ent->pairslist, "func_collision"); // VAL

	stringlistappend	(&d_ent->pairslist, "mdl"); // KEY
	stringlistappend	(&d_ent->pairslist, model); // VAL

	stringlistappend	(&d_ent->pairslist, "size"); // KEY
	stringlistappendf	(&d_ent->pairslist, VECTOR3_G, VECTOR3_SEND(vsize) ); // VAL


#define XH vsize[0]
#define YH vsize[1]
#define ZH vsize[2]
#define XL 0
#define YL 0
#define ZL 0

	brush_s *d_brush;
	brushrow_s *br;
	
	d_brush = brushlist_add (&d_ent->brushlist); // CREATE BRUSH FOR DEST
	
	// 221 83.4 67.3
	
	// ( [XH] [YH] [ZH] ) ( [XH] [YH] [ZL] ) ( [XH] [YL] [ZH] ) common/caulk 
		// [ 0 1 0 -20.4 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0

	br = brushrowlist_add (&d_brush->brushrowlist);
	//          ( [XH] [YH] [ZH] )                ( [XH] [YH] [ZL] )          ( [XH] [YL] [ZH] ) 
	VectorSet (br->a, XH, YH, ZH); VectorSet (br->b, XH, YH, ZL); VectorSet (br->c, XH, YL, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");

	br = brushrowlist_add (&d_brush->brushrowlist); // 2 - all numbers identical to 1 except abc
	//( [XL] [YL] [ZH] ) ( [XL] [YL] [ZL] ) ( [XL] [YH] [ZH] ) common/caulk [ 0 1 0 -20.4 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YL, ZH); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XL, YH, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");

	br = brushrowlist_add (&d_brush->brushrowlist); // 3
	//( [XH] [YL] [ZH] ) ( [XH] [YL] [ZL] ) ( [XL] [YL] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	//( 221 0 67.3 ) ( 221 0 0 ) ( 0 0 67.3 ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XH, YL, ZL); VectorSet (br->c, XL, YL, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");

	br = brushrowlist_add (&d_brush->brushrowlist); // 4 - all numbers identical to 3 except abc
	// ( [XL] [YH] [ZH] ) ( [XL] [YH] [ZL] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZH); VectorSet (br->b, XL, YH, ZL); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");

	br = brushrowlist_add (&d_brush->brushrowlist); // 5
	// ( [XL] [YH] [ZL] ) ( [XL] [YL] [ZL] ) ( [XH] [YH] [ZL] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZL); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XH, YH, ZL);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");
	
	br = brushrowlist_add (&d_brush->brushrowlist); // 6 - all ident to 5 except a b c
	// ( [XH] [YL] [ZH] ) ( [XL] [YL] [ZH] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XL, YL, ZH); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 0, 160, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/caulk");

	d_brush = brushlist_add (&d_ent->brushlist); // CREATE BRUSH FOR DEST


	br = brushrowlist_add (&d_brush->brushrowlist);
	//          ( [XH] [YH] [ZH] )                ( [XH] [YH] [ZL] )          ( [XH] [YL] [ZH] ) 
	VectorSet (br->a, XH, YH, ZH); VectorSet (br->b, XH, YH, ZL); VectorSet (br->c, XH, YL, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	//553648128 16512 0
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&d_brush->brushrowlist); // 2 - all numbers identical to 1 except abc
	//( [XL] [YL] [ZH] ) ( [XL] [YL] [ZL] ) ( [XL] [YH] [ZH] ) common/caulk [ 0 1 0 -20.4 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YL, ZH); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XL, YH, ZH);
	Vector4Set (br->xtra1, 0, 1, 0, -20.4 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&d_brush->brushrowlist); // 3
	//( [XH] [YL] [ZH] ) ( [XH] [YL] [ZL] ) ( [XL] [YL] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XH, YL, ZL); VectorSet (br->c, XL, YL, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&d_brush->brushrowlist); // 4 - all numbers identical to 3 except abc
	// ( [XL] [YH] [ZH] ) ( [XL] [YH] [ZL] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 0 -1 6.29999 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZH); VectorSet (br->b, XL, YH, ZL); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, 0, -1, 6.29999 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

	br = brushrowlist_add (&d_brush->brushrowlist); // 5
	// ( [XL] [YH] [ZL] ) ( [XL] [YL] [ZL] ) ( [XH] [YH] [ZL] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XL, YH, ZL); VectorSet (br->b, XL, YL, ZL); VectorSet (br->c, XH, YH, ZL);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");
	
	br = brushrowlist_add (&d_brush->brushrowlist); // 6 - all ident to 5 except a b c
	// ( [XH] [YL] [ZH] ) ( [XL] [YL] [ZH] ) ( [XH] [YH] [ZH] ) common/caulk [ 1 0 0 20 ] [ 0 -1 0 20.4 ] 0 0.5 0.5 0 160 0
	VectorSet (br->a, XH, YL, ZH); VectorSet (br->b, XL, YL, ZH); VectorSet (br->c, XH, YH, ZH);
	Vector4Set (br->xtra1, 1, 0, 0, 20 ); Vector4Set (br->xtra2, 0, -1, 0, 20.4 ); 
	Vector6Set (br->ftrail, 0, 0.5, 0.5, 553648128, 16512, 0); br->trail_count = 6;
	br->pbrtexture = Z_StrDup ("common/origin");

#undef XH
#undef YH
#undef ZH
#undef XL
#undef YL
#undef ZL

}




void entitylist_gen_models (entitylist_t *plist, int ex /*entnum*/)
{
	// Baker: We are inserting at entity 1

	// We must perform the insertion AFTER the entities loop
	// Otherwise we will be messing up the entities loop by inserting entries
	// while working the loop.
	stringlist_t modelstoaddlist = {0};
	entityx_t	*p_ent = &plist->entity[ex];
	for (int kx = 0; kx < p_ent->pairslist.numstrings; kx += 2) {
		ccs *key = p_ent->pairslist.strings[kx + 0];
		ccs *val = p_ent->pairslist.strings[kx + 1];
		if (String_Starts_With(key, "model_gen"))
			stringlistappend (&modelstoaddlist, val);
	} // epairs

	// Add the accumulated entities
	for (int j = 0; j < modelstoaddlist.numstrings; j ++) {
		ccs *val = modelstoaddlist.strings[j];
		genbrush (plist, val);
	}

	// Free
	stringlistfreecontents (&modelstoaddlist);
}

// All upwards facing brushes get texture "textures/up"
int entitylist_brush0_facer (entitylist_t *plist, int *pnum_faces)
{
	int num_upwards_faces = 0;
	int num_faces = 0;
	for (int ex = 0; ex < 1 /*plist->count*/; ex ++) { // WOLRD ONLY
		entityx_t	*p_ent = &plist->entity[ex];

		for (int bx = 0; bx < p_ent->brushlist.count; bx ++) {
			brush_s *p_brush = &p_ent->brushlist.brush[bx];
			if (p_brush->is_a_patch)
				continue; // WE DON'T DO THESE
				
			// REAL BRUSH
			for (int brow = 0; brow < p_brush->brushrowlist.count; brow ++) {
				brushrow_s *pbr = &p_brush->brushrowlist.brushrow[brow];

				num_faces ++;

				vec3_t pba; VectorSubtract (pbr->b, pbr->a, pba);
				vec3_t pca; VectorSubtract (pbr->c, pbr->a, pca);
				
				vec3_t cross; CrossProduct (pba, pca, cross);

				int is_up_brush = false;
				if (cross[2] < 0) {
					// This one is facing up.
					is_up_brush = true;
					num_upwards_faces ++;

					Mem_FreeNull_ (pbr->pbrtexture);
					pbr->pbrtexture = Z_StrDup ("textures/up");
				}

			} // row

		} // brush		
	} // entities in .map
	if (pnum_faces)
		*pnum_faces = num_faces;
	return num_upwards_faces;
}


#include "eq_zparse.c.h"


