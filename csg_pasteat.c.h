// csg_pasteat.c.h

// Input map2.
// Output map2 A
// Output map2 B

// Allocate entity as keys and values
// Allocate brushes as whatever. With a count.
// No patch support ... for now.

// Do a growing array alloc.  100% pool it.
// 
//typedef struct _patch_entry_st {
//	vec3_t a;				// ( 480 704 -260 )
//	vec3_t b;				//( 480 672 -260 )
//	vec3_t c;				// ( 480 704 -132 )
//	char *texture;	// common/caulk
//	vec4_t xtra1;		// [ 0 1 0 0 ]
//	vec4_t xtra2;		// [ 0 0 -1 0 ]
//	float ftrail[6];	// -0 1 1 0 160 0
//	struct _brush_entry_st *next;
//} patch_entry_s;

typedef struct _brush_group_st {
	struct _brush_entry_st *brushes;
	int count;
} brush_group_s;

//typedef struct _patch_group_st {
//	struct _brush_entry_st *brushes;
//	int count;
//} patch_group_s;

// keypairs

// brush set

// Remember .. end of day this is just lines.
// For patches, we can detect numbers that aren't the first one and do the specific rows.
// Row type.
// Output rows.
// To some extent, knowing the lines numbers is more important for us.
// But we need to know what it is too!


WARP_X_ (CSG_Process_BSAlloc, BrushFacer_ZA brush_entry_s)

typedef struct {
	ccs				*text_start;
	ccs				*datasrc;
	int				depth;
//	int				in_patch;
	
	entitylist_t	*plist;
	entityx_t		*p_ent;
	brush_s			*p_brush;
	brushrow_s		*p_brushrow;
	patchrow_s		*p_patchrow;
//	patch_s			*p_patch;
} map_reader_s;



// Returns dbpool
WARP_X_ (BrushFacer_ZA brush_entry_s)

static int ReadBrushRow_Numbers_IsOk (map_reader_s *pread, brushrow_s *pbr)
{
#define READMORE if (COM_Parse_Basic(&pread->datasrc) == false) return false /*NOT OK*/

//( 480 704 -260 ) ( 480 672 -260 ) ( 480 704 -132 ) 
//				common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0

	READMORE;	pbr->a[0] = atof(com_token);			//  480
	READMORE;	pbr->a[1] = atof(com_token);			//  704
	READMORE;	pbr->a[2] = atof(com_token);			// -260
	READMORE;	if (com_token[0] != ')') return false;		// )
	
	READMORE;	if (com_token[0] != '(') return false;		// (
	READMORE;	pbr->b[0] = atof(com_token);				//	480
	READMORE;	pbr->b[1] = atof(com_token);				//	672
	READMORE;	pbr->b[2] = atof(com_token);				// -260
	READMORE;	if (com_token[0] != ')') return false;		// )
	
	READMORE;	if (com_token[0] != '(') return false;		// (
	READMORE;	pbr->c[0] = atof(com_token);				//	480
	READMORE;	pbr->c[1] = atof(com_token);				//	704
	READMORE;	pbr->c[2] = atof(com_token);				// -132
	READMORE;	if (com_token[0] != ')') return false;		// )

//( 480 704 -260 ) ( 480 672 -260 ) ( 480 704 -132 ) 
//				common/caulk [ 0 1 0 0 ] [ 0 0 -1 0 ] -0 1 1 0 160 0

// <x_shift> <y_shift>   ... <rotation> <x_scale> <y_scale> <content_flags> <surface_flags> <value>
	READMORE;	pbr->pbrtexture = Z_StrDup(com_token); //	common/caulk
	READMORE;	if (com_token[0] != '[') return false;		// [
	READMORE;	pbr->xtra1[0] = atof(com_token);			//	0
	READMORE;	pbr->xtra1[1] = atof(com_token);			//	1
	READMORE;	pbr->xtra1[2] = atof(com_token);			//	0
	READMORE;	pbr->xtra1[3] = atof(com_token);			//	0
	READMORE;	if (com_token[0] != ']') return false;		// ]
	READMORE;	if (com_token[0] != '[') return false;		// [
	READMORE;	pbr->xtra2[0] = atof(com_token);			//	0
	READMORE;	pbr->xtra2[1] = atof(com_token);			//	0
	READMORE;	pbr->xtra2[2] = atof(com_token);			// -1
	READMORE;	pbr->xtra2[3] = atof(com_token);			//	0
	READMORE;	if (com_token[0] != ']') return false;		// ]

//( 32768 -34816 10240 ) ( 32768 -34816 -512 ) ( 32768 34816 10240 ) sky/treefall_nite_sky [ 0 1 0 -0 ] 
//		[ 0 0 -1 -0 ] 0 0.5 0.5 // 3 numbers
//( 34816 34816 10240 ) ( 34816 34816 -512 ) ( 34816 -34816 10240 ) common/caulk [ 0 1 0 -0 ] [ 0 0 -1 -0 ] 
//		              0 0.5 0.5 0 160 0 // 6 numbers

	READMORE;	pbr->ftrail[0] = atof(com_token);			// -0
	READMORE;	pbr->ftrail[1] = atof(com_token);			//	1
	READMORE;	pbr->ftrail[2] = atof(com_token);			//	1

	pbr->trail_count = 3;
	ccs *rollback;

	rollback = pread->datasrc;
	READMORE;
	if (isin2(com_token[0], '(', '}') == false) {
		pbr->ftrail[3] = atof(com_token);			//	0
//		if (String_Starts_With (com_token, "536870")) {
//			int j = 5;
//		}
		pbr->trail_count ++; // 3 --> 4
	} else {
		pread->datasrc = rollback;
		return true;
	}

	rollback = pread->datasrc;
	READMORE;
	if (isin2(com_token[0], '(', '}') == false) {
		pbr->ftrail[4] = atof(com_token);			//	0
		pbr->trail_count ++; // 3 --> 4
	} else {
		pread->datasrc = rollback;
		return true;
	}

	rollback = pread->datasrc;
	READMORE;
	if (isin2(com_token[0], '(', '}') == false) {
		pbr->ftrail[5] = atof(com_token);			//	0
		pbr->trail_count ++; // 3 --> 4
	} else {
		pread->datasrc = rollback;
		return true;
	}


	return true; // SUCCESS!
}

static int ReadBrushRows_Thru_CurlyClose_IsOk (map_reader_s *pread, brush_s *p_brush)
{
	while (1) {
		// We just read a "("

		brushrow_s	*p_brushrow = brushrowlist_add (&p_brush->brushrowlist);
		pread->p_brushrow = p_brushrow;
		int is_ok = ReadBrushRow_Numbers_IsOk (pread, p_brushrow);
		if (is_ok == false)
			return false;

		// READ NEXT -- IT IS EITHER "(" NEW ROW OR "}" END OF DATA 
		if (COM_Parse_Basic(&pread->datasrc) == false || isin2(com_token[0], '(', '}') == false) {
			Con_PrintLinef ("Expected ) or { got " QUOTED_S " in fn %s", com_token, __func__ );
			return false;
		}

		if (com_token[0] == '}') {
			// NORMAL END OF BRUSH
			pread->depth --;
			// DEPTH should be 1 now
			return true; // SUCCESS!
		}

		// We read a "(" loop back around and add a new row
	}

	// Unreachable!
	return false;
}

		//patchDef2
		//{ <---------------------------------- START
		//	sg_tomb/wall019
		//	( 3 3 0 0 0 )
		//	(
		//		( ( -749.894287 6939.077148 3254.350586 0.000000 0.561562 ) ( -678.014404 6939.077148 3254.350586 0.000000 0.280781 ) ( -606.134460 6939.077148 3254.350586 0.000000 0.000000 ) )
		//		( ( -749.894287 6942.136719 3314.468750 0.235141 0.561562 ) ( -678.014404 6942.136719 3314.468750 0.235141 0.280781 ) ( -606.134460 6942.136719 3314.468750 0.235141 0.000000 ) )
		//		( ( -749.894287 7006.000000 3325.045410 0.488005 0.561562 ) ( -678.014404 7006.000000 3325.045410 0.488005 0.280781 ) ( -606.134460 7006.000000 3325.045410 0.488005 0.000000 ) )
		//	)
		//}
static int ReadPatchRows_Thru_CurlyClose_IsOk (map_reader_s *pread, brush_s *p_brush)
{
	// We just read a "patchDef2"
		//{ <---------------------------------- START
		//	sg_tomb/wall019
		//	( 3 3 0 0 0 )
		//	(
		//		( ( -749.894287 6939.077148 3254.350586 0.000000 0.561562 ) ( -678.014404 6939.077148 3254.350586 0.000000 0.280781 ) ( -606.134460 6939.077148 3254.350586 0.000000 0.000000 ) )
		//		( ( -749.894287 6942.136719 3314.468750 0.235141 0.561562 ) ( -678.014404 6942.136719 3314.468750 0.235141 0.280781 ) ( -606.134460 6942.136719 3314.468750 0.235141 0.000000 ) )
		//		( ( -749.894287 7006.000000 3325.045410 0.488005 0.561562 ) ( -678.014404 7006.000000 3325.045410 0.488005 0.280781 ) ( -606.134460 7006.000000 3325.045410 0.488005 0.000000 ) )
		//	)
		//}
	READMORE; if (com_token[0] != '{') return false;
	READMORE; p_brush->texture = Z_StrDup (com_token);
	READMORE; if (com_token[0] != '(') return false;
	READMORE; p_brush->rows = atof(com_token); // ( 3 3 0 0 0 )   3 is rows
	READMORE; p_brush->cols = atof(com_token); //     3				is columns
	READMORE; p_brush->other3[0] = atof(com_token);
	READMORE; p_brush->other3[1] = atof(com_token);
	READMORE; p_brush->other3[2] = atof(com_token);
	READMORE; if (com_token[0] != ')') return false;
	READMORE; if (com_token[0] != '(') return false;

	// We read a series of rows
	// Read the closing )
	// Read the closing } and decrease depth

	// WE ARE ON A ROW

	for (int row = 0; row < p_brush->rows; row ++) {
		patchrow_s *p_row = patchrowlist_add (&p_brush->patchrowlist);
		READMORE; if (com_token[0] != '(') return false;
		for (int col = 0; col < p_brush->cols; col ++) {
			READMORE; if (com_token[0] != '(') return false;
			// Read 5 numbers
			READMORE; p_row->flots[col * 5 + 0] = atof(com_token);
			READMORE; p_row->flots[col * 5 + 1] = atof(com_token);
			READMORE; p_row->flots[col * 5 + 2] = atof(com_token);
			READMORE; p_row->flots[col * 5 + 3] = atof(com_token);
			READMORE; p_row->flots[col * 5 + 4] = atof(com_token);
			READMORE; if (com_token[0] != ')') return false;
		}
		READMORE; if (com_token[0] != ')') return false;
	}

	READMORE; if (com_token[0] != ')') return false;
	READMORE; if (com_token[0] != '}') return false;
	READMORE; if (com_token[0] != '}') return false;
	pread->depth --;
	return true;
}


int entitylist_parsemaptxt (entitylist_t *plist, ccs *txt) // Returns a (baker_string_t *) or NULL if no data
{
	map_reader_s _reader = {0}, *pread = &_reader;
	pread->text_start = pread->datasrc = txt;
	
	pread->plist = plist;
	int iters = 0;
	while (1) {
		iters ++;
		// Read some datasrc ...
		if (COM_Parse_Basic(&pread->datasrc) == false) {
			if (pread->depth == 0)
				return true; // End of string data

			return false; // We expected more data because depth is not 0, therefore this is "bad"
		}

		if (com_token[0] == '}') {
			if (pread->depth == 1) { // (depth 1 ===> 0)
				// STATE_CHECKCLASS_0; // End of entity
			}
			pread->depth --;
			continue;
		} else if (com_token[0] == '{') {
			if (pread->depth == 0) {
				// Allocate a new entity. (depth 0 ===> 1)
				pread->p_ent = entitylist_add (pread->plist);
			}
			pread->depth ++;
			continue;
		}

read_brush:
		if (pread->depth >= 2) {
			if (com_token[0] == '(') { // THAT IS A PAREN
				// I suggest reading brush rows and then the "}" decreasing the depth
				// NEW BRUSH
				WARP_X_ (entityx_t)
				pread->p_brush = brushlist_add (&pread->p_ent->brushlist);
				int is_ok = ReadBrushRows_Thru_CurlyClose_IsOk (pread, pread->p_brush);
				if (is_ok == false) 
					return false; // Exited
				//pread->depth --;
				continue;
			} else if (String_Match (com_token, "patchDef2")) {
read_patch:
				//patchDef2
				//{ <---------------------------------- START
				//	sg_tomb/wall019
				//	( 3 3 0 0 0 )
				//	(
				//		( ( -749.894287 6939.077148 3254.350586 0.000000 0.561562 ) ( -678.014404 6939.077148 3254.350586 0.000000 0.280781 ) ( -606.134460 6939.077148 3254.350586 0.000000 0.000000 ) )
				//		( ( -749.894287 6942.136719 3314.468750 0.235141 0.561562 ) ( -678.014404 6942.136719 3314.468750 0.235141 0.280781 ) ( -606.134460 6942.136719 3314.468750 0.235141 0.000000 ) )
				//		( ( -749.894287 7006.000000 3325.045410 0.488005 0.561562 ) ( -678.014404 7006.000000 3325.045410 0.488005 0.280781 ) ( -606.134460 7006.000000 3325.045410 0.488005 0.000000 ) )
				//	)
				//}
				WARP_X_ (entityx_t)
				//pread->p_patch = patchlist_add (&pread->p_ent->patchlist);

				pread->p_brush = brushlist_add (&pread->p_ent->brushlist);
				pread->p_brush->is_a_patch = true;
				int is_ok = ReadPatchRows_Thru_CurlyClose_IsOk (pread, pread->p_brush);
				if (is_ok == false) 
					return false; // Exited

				continue;
			}
			// We shouldn't get here ...
			return false;
		}

		// DEPTH = 1 and not { or } or patchDef2 therefore
		// we are parsing epairs.
entity_read_key:

		stringlistappend (&pread->p_ent->pairslist, com_token); // KEY

entity_read_value:
		// Read another - Check for error condition
		if (COM_Parse_Basic(&pread->datasrc) == false) 
			return false; // End of string data

		stringlistappend (&pread->p_ent->pairslist, com_token); // KEY
	} // While

}

