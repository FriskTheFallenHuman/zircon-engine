// filematch.h

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

typedef struct _voidlist_s {
	const void	**vloats;
	int			maxsize;
	int			count;
} voidlist_s;

void voids_add1 (voidlist_s *vlist, const void *p);
void voids_freecontents (voidlist_s *vlist);


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

#define stringlist_last(LISTX) LISTX.strings[LISTX.numstrings - 1]
#define stringlist_lastp(PLISTX) PLISTX->strings[PLISTX->numstrings - 1]

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

// SEPT 28 2024

typedef struct _brushrow_st {
	vec3_t a;			// ( 480 704 -260 )
	vec3_t b;			// ( 480 672 -260 )
	vec3_t c;			// ( 480 704 -132 )
	char *pbrtexture;	// common/caulk
	vec4_t xtra1;		// [ 0 1 0 0 ]
	vec4_t xtra2;		// [ 0 0 -1 0 ]
	int trail_count;	// There are 3 or more at the end... supposedly it can go up to 8
//( 32768 -34816 10240 ) ( 32768 -34816 -512 ) ( 32768 34816 10240 ) sky/treefall_nite_sky [ 0 1 0 -0 ] 
//		[ 0 0 -1 -0 ] 0 0.5 0.5 // 3 numbers
//( 34816 34816 10240 ) ( 34816 34816 -512 ) ( 34816 -34816 10240 ) common/caulk [ 0 1 0 -0 ] [ 0 0 -1 -0 ] 
//		              0 0.5 0.5 0 160 0 // 6 numbers

	double ftrail[8];	// -0 1 1 0 160 0
} brushrow_s;

typedef struct _brushrowlist_st {
	int				maxsize;
	int				count;
	brushrow_s		*brushrow;	// PAGE
} brushrowlist_t;

typedef struct _patchrow_st {
// ( -126 -192 128 8 0 ) ( -126 -192 384 8 -6 ) ( -126 -192 640 8 -12 )
	float flots[300];
} patchrow_s;

// list of // ( -126 -192 128 8 0 ) ( -126 -192 384 8 -6 ) ( -126 -192 640 8 -12 )
typedef struct _patchrowlist_st {
	int maxsize;
	int count;
	patchrow_s		*patchrow;	// PAGE
} patchrowlist_t;

typedef struct _brush_st {
	brushrowlist_t	brushrowlist;
	int				is_a_patch;

	char			*texture;	// liquids_lava/lava_blue_LX_1000_alpha_50  bare texture 
	float			rows;		// ( 9 3 536870920 16 1000 ) always 5 per column
	float			cols;
	float			other3[3];
	patchrowlist_t	patchrowlist; // ( -126 -192 128 8 0 ) ( -126 -192 384 8 -6 ) ( -126 -192 640 8 -12 )

} brush_s;

//	patchDef2
//	{
//		liquids_lava/lava_blue_LX_1000_alpha_50
//		( 9 3 536870920 16 1000 )
//		(
//			(
//				( -126 -192 128 0 0 )
//				( -126 -192 384 0 -6 )
//				( -126 -192 640 0 -12 )
//			)
//			(MORE LIKE THESE)
//			(
//				( -126 -192 128 8 0 ) ( -126 -192 384 8 -6 ) ( -126 -192 640 8 -12 )
//			)
//		)
//	}
//}

typedef struct _brushlist_st {
	/// maxstrings changes as needed, causing reallocation of strings[] array
	int				maxsize;
	int				count;
	brush_s			*brush;		// PAGE
} brushlist_t;

typedef struct _entity_st { // p_ent
	stringlist_t	pairslist;	
	brushlist_t		brushlist;
} entityx_t;


typedef struct _entitylist_st {
	/// maxstrings changes as needed, causing reallocation of strings[] array
	int maxsize;
	int count;
	entityx_t		*entity; // p_ent (PAGE)
} entitylist_t;

void brushrowlistfreecontents (brushrowlist_t *list);
brushrow_s *brushrowlist_add (brushrowlist_t *list);

void brushlistfreecontents(brushlist_t *list);
brush_s *brushlist_add (brushlist_t *list);

void patchrowlistfreecontents (patchrowlist_t *list);
patchrow_s *patchrowlist_add (patchrowlist_t *list);

void entitylistfreecontents(entitylist_t *list);
entityx_t *entitylist_add (entitylist_t *list);
void entitylist_print_console (entitylist_t *plist); // Baker: Recursive printing brushes and patches to console.

void entitylist_to_clipboard (entitylist_t *plist);

void entitylist_translate_epairs (entitylist_t *list, vec3_t vadd);
void entitylist_translate_brushes (entitylist_t *list, vec3_t vadd);

void entitylist_brush0_append (entitylist_t *list, entitylist_t *paste);
void entitylist_nonworld_append (entitylist_t *list, entitylist_t *paste);

void entitylist_set_replace_key_val (entitylist_t *plist, int entnum, ccs *key_force, ccs *val_force);

int entitylist_atomize_entities_num_made (entitylist_t *plist, ccs *s0_plus_timestamp);

void entitylist_nonworld_setthis (entitylist_t *plist, ccs *key_force, ccs *val_force);
void entitylist_nonworld_set (entitylist_t *plist, stringlist_t *plistpairset);

void entitylist_prefix_epairs (entitylist_t *plist, ccs *prefix);
void entitylist_prefix_epairslist (entitylist_t *plist, ccs *prefix, stringlist_t *plist_prefixes);

int entitylist_epairs_find_model_gen_entitynum (entitylist_t *plist, ccs *prefix);
void entitylist_gen_models (entitylist_t *plist, int ex /*entnum*/);

int entitylist_brush0_facer (entitylist_t *plist, int *pnum_faces);

int entitylist__originmake_num_made (entitylist_t *plist, ccs *svaluetowrite);

// BakerString_Destroy_And_Null_It (&bs);
baker_string_t *entitylist_maptext_bsalloc (entitylist_t *plist);

baker_string_t *CSG_Process_BSAlloc (ccs *datasrc); // Returns a (baker_string_t *) or NULL if no data

// Returns NULL or value for key for the entity
ccs *entitykeys_find_value (entityx_t *e, ccs *keyname);
int entitylist_parsemaptxt (entitylist_t *plist, ccs *txt);

#include "equat.h"

#endif // FILEMATCH_H
