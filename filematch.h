/*
Copyright (C) 2006-2021 DarkPlaces contributors

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

#ifndef FILEMATCH_H
#define FILEMATCH_H

#include "qtypes.h"

typedef struct stringlist_s {
	/// maxstrings changes as needed, causing reallocation of strings[] array
	int maxstrings;
	int numstrings;
	char **strings;
} stringlist_t;

// Baker: floatlist_s
// To store floats with intention of list to be null terminated
// at all times IF count > 0
// So ..
// while (*floats++) 
// .. would work

typedef struct _floatlist_s {
	float	*floats;
	int		maxsize;
	int		count;
} floatlist_s;

void floats_add1 (floatlist_s *flist, float p);
void floats_add2 (floatlist_s *flist, float px, float py);
void floats_add3 (floatlist_s *flist, float px, float py, float pz);


void floats_print (floatlist_s *flist, void (*myprintf)(const char *, ...));
void floats_dump (floatlist_s *flist);

char *floats_join_delim12_delim_zalloc (floatlist_s *flist, int number_per_vert, ccs *s_delimiter12, ccs *s_delimiter_elements, int shall_quote);

// Returns 0 if the parse of all elements went as expected
int floats_append_parse_space_comma_num_elements_ignored (floatlist_s *flist, ccs *s_value, int number_per_vert, ccs *s_delimiter12, ccs *s_delimiter_elements);

void floats_freecontents (floatlist_s *flist);

typedef struct _int32list_s {
	int		*ints;
	int		maxsize;
	int		count;
} int32list_s;

void int32s_add1 (int32list_s *ilist, int p);
void int32s_add2 (int32list_s *ilist, int px, int py);
void int32s_add3 (int32list_s *ilist, int px, int py, int pz);

void int32s_print (int32list_s *ilist, void (*myprintf)(const char *, ...));
void int32s_dump (int32list_s *ilist);

void int32s_count_set (int32list_s *ilist, int newcount); // increases only.

int int32s_append_split_dequote (int32list_s *ilist, ccs *s_value, ccs *s_delimiter_elements);
char *int32s_join_delim_zalloc (int32list_s *ilist, ccs *s_delimiter_elements, int shall_quote);


void int32s_freecontents (int32list_s *ilist);

int matchpattern(const char *in, const char *pattern, int caseinsensitive);
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one);
void stringlistinit(stringlist_t *list);
void stringlistfreecontents(stringlist_t *list);
void stringlistappend(stringlist_t *list, const char *text);
void stringlistappend_len (stringlist_t *list, const char *text, int text_len);
int stringlistappend_split_delimiter_linenumbers (stringlist_t *plist, ccs *s, ccs *delimiter);

int stringlistappendfilelines_did_load (stringlist_t *plist, ccs *file, int is_number_column);

const char **stringlist_nullterm_add (stringlist_t *list);

void stringlistappendf (stringlist_t *list, const char *fmt, ...)  DP_FUNC_PRINTF(2);
void stringlistappendf2 (stringlist_t *list, ccs *s, const char *fmt, ...) DP_FUNC_PRINTF(3);

void stringlistappend2 (stringlist_t *list, ccs *s, ccs *s2);
void stringlistappend3 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3);
void stringlistappend4 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3, ccs *s4);
void stringlistappend5 (stringlist_t *list, ccs *s, ccs *s2, ccs *s3, ccs *s4, ccs *s5);


// Moved to darkplaces.h so fssearch_t is defined
void stringlistappendlist (stringlist_t *plist, const stringlist_t *add_these); // Baker: Add a list to a list
void stringlistprint (stringlist_t *list, const char *title_optional, void (*myprintf)(const char *, ...) );
void stringlist_replace_at_index (stringlist_t *list, int idx, const char *text);
void stringlistsort(stringlist_t *list, qbool uniq);
void stringlist_condump (stringlist_t *plist);
void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) );

void stringlistappend_blob (stringlist_t *list, const byte *blob, size_t blobsize); // Put binary data in a stringlist

void stringlistsort_substring(stringlist_t *list, qbool uniq, int startpos, int slength); // Substring sort

void listdirectory(stringlist_t *list, const char *basepath, const char *path);

/*JOIN*/
void stringlist_join_buf (stringlist_t *p_stringlist, ccs *s_delimiter, char *buf, size_t buflen); // JOIN 

// Free with: baker_string_t *bs = NULL; if (bs) BakerString_Destroy_And_Null_It (&bs);
baker_string_t *stringlist_join_lines_bkalloc (stringlist_t *plist);
baker_string_t *stringlist_join_bkalloc (stringlist_t *p_stringlist, ccs *s_delimiter);

char *stringlist_join_lines_zalloc (stringlist_t *plist);
char *stringlist_join_spaced_zalloc (stringlist_t *plist);
char *stringlist_join_delim_zalloc (stringlist_t *plist, ccs *s_delimiter, int shall_quote);

/*SPLIT*/
void stringlistappend_split_len (stringlist_t *plist, ccs *text_in, int text_strlen, ccs *s_delimiter);
void stringlistappend_split (stringlist_t *plist, ccs *text_in, ccs *s_delimiter);

int stringlistappend_from_dir_pattern (stringlist_t *p_stringlist, const char *s_optional_dir_no_slash, const char *s_optional_dot_extension, int wants_strip_extension);


/*LINE WORD -> SPLIT*/
void stringlistappend_tokenize_qcquotes (stringlist_t *plist, const char *text);

/*LINE SPLIT*/
void stringlistappend_textlines_cr_scrub (stringlist_t *plist, const char *text_in);
void stringlistappend_textlines_len_cr_scrub (stringlist_t *plist, const char *text_in, int text_strlen);
#define stringlistappend_split_lines_cr_scrub(plist, s) stringlistappend_textlines_cr_scrub(plist, s)


void stringlist_from_delim (stringlist_t *p_stringlist, const char *s_space_delimited);
int stringlist_find_index (stringlist_t *p_stringlist, ccs *s_find_this);


#endif
