// oject_cca_properties.c.h

qbool Event_Set_By_String_Is_Ok (oject_s *k, things_s *h, things_s *hbuiltin)
{
	WARP_X_ (oject_s things_s things)

	size_t		offsetx	= h->moffsetof;
	//vartype_e	vt		= (vartype_e)h->vt;

	if (h->moffsetof == 0) {
		Con_PrintLinef ("Internal: Event %s has no offsetof", h->name);
		return false;
	}

	int is_ok = true;

	varpack_u *vp = (varpack_u *) ((byte *)(k) + offsetx);

	vp->char_p = Z_StrDup (hbuiltin->name);

	return is_ok; // is ok
}

qbool Property_Set_By_String_After_Freeing_Is_Ok (oject_s *k, things_s *h, const char *s_value)
{
	WARP_X_ (oject_s things_s things)

//	if (h->enum_id == prop_list)
//		int j  = 5;

	size_t		offsetx	= h->moffsetof;
	vartype_e	vt		= (vartype_e)h->w.vt;

	if (h->w.vt == 0) {
		Con_PrintLinef ("Property %s has no vartype", h->name);
		return false;
	}

	if (h->moffsetof == 0) {
		Con_PrintLinef ("Property %s has no offsetof", h->name);
		return false;
	}

	int is_ok = true;

	int s_value_slen = strlen(s_value);
	char *stemp_za = NULL;

	if (String_Is_Quoted(s_value, s_value_slen)) {
		// Remove quotes
		stemp_za = Z_StrDup_Len_Z (&s_value[1], s_value_slen - 2);
		s_value = stemp_za;
	}

	varpack_u *vp = (varpack_u *) ((byte *)(k) + offsetx);

	// These are required to full stomp and zero reset.
	#pragma message ("oject property set TODO: detect already set condition for non-zero values?")

these_must_free_existing:
	switch (vt) {
	default:				is_ok = false; break;
	case vtstring_1:		Z_StrDup_Realloc_ (vp->char_p, s_value);		break;
	case vtfloat_2:			vp->floatp = atof(s_value);					break;
	case vtinteger_19:
		if (String_Match_Caseless(s_value, "true"))
			vp->intp = 1;
		else if (String_Match_Caseless(s_value, "false"))
			vp->intp = 0;
		else
			vp->intp = atoi(s_value);
		break;
	case vtrectf_10:
		{ // The float rect left/top/width/height
			stringlist_t rlist = {0};
			stringlistappend_split (&rlist, s_value, ",");

			memset (&vp->rectf, 0, sizeof(vp->rectf) );
			const char **wordray = stringlist_nullterm_add (&rlist);

			// space skip - not needed! yay!
			// atof: Function discards any whitespace characters (as determined by isspace) until
			// first non-whitespace character is found. Then it takes as many characters as possible
			// to form a valid floating-point representation and converts them to a floating-point
			// value.

			if (*wordray) vp->rectf.left	= atof(*wordray++);
			if (*wordray) vp->rectf.top		= atof(*wordray++);
			if (*wordray) vp->rectf.width	= atof(*wordray++);
			if (*wordray) vp->rectf.height	= atof(*wordray++);
			stringlistfreecontents (&rlist);
		}
		break;
	case vtrecti_11:
		{ // The integer rect left/top/width/height

			stringlist_t rlist = {0};
			stringlistappend_split (&rlist, s_value, ",");

			memset (&vp->rectf, 0, sizeof(vp->recti) );
			const char **wordray = stringlist_nullterm_add (&rlist);

			// space skip - not needed! yay!
			// atof: Function discards any whitespace characters (as determined by isspace) until
			// first non-whitespace character is found. Then it takes as many characters as possible
			// to form a valid floating-point representation and converts them to a floating-point
			// value.

			if (*wordray) vp->recti.left	= atoi(*wordray++);
			if (*wordray) vp->recti.top		= atoi(*wordray++);
			if (*wordray) vp->recti.width	= atoi(*wordray++);
			if (*wordray) vp->recti.height	= atoi(*wordray++);
			stringlistfreecontents (&rlist);
		}
		break;

	case vtcontrolref_23:
		// We have a name.  Find the name ...
		{
			Form_Get(f,k);
			oject_s *kref = Object_Find_Name (f, s_value);
			vp->ojectp = kref;
//			int j = 5;
		}
		break;

//
// COMPLICATED: Color "RGB(255,255,255)" or "white" ...
//

	case vtrgb_21:
		// Word support

		// Skip the rgb
		{
			ccs *srgb		= dpstrcasestr	(s_value, "rgb(");

			if (!srgb) {
				// Try word
				char *s_z = Z_StrDupf(s_value);
				String_Edit_DeQuote (s_z);
				int was_set = false;
				if (String_Match_Caseless (s_z, "white")) {
					VectorSet(vp->vec3, 1.0, 1.0, 1.0);
					//vp->vec3[0] = 1.0; //colorxs[0] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[1] = 1.0; //colorxs[1] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[2] = 1.0; //colorxs[2] / 255.0; // 255 = 1, 0 = 0
					was_set = true;
				} else if (String_Match_Caseless (s_z, "black")) {
					VectorSet(vp->vec3, 0, 0, 0);
					//vp->vec3[0] = 0.0; //colorxs[0] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[1] = 0.0; //colorxs[1] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[2] = 0.0; //colorxs[2] / 255.0; // 255 = 1, 0 = 0
					was_set = true;
				}

				Z_FreeNull_ (s_z);
				if (was_set)
					break;
				break;
			}

			ccs *sendparen	= strstr		(s_value, ")");

			if (srgb && sendparen && sendparen > srgb) {
				int sizeofthis = sendparen - srgb - STRINGLEN("rgb(");
				if (sizeofthis >=0) {
					ccs *s_after_rgb = srgb + STRINGLEN("rgb(");
					byte colorxs[3] = {0};

					// These are integer values
					stringlist_t rlist = {0};
					stringlistappend_split_len (&rlist, s_after_rgb, sizeofthis, ",");

					vp->vec3[0] = vp->vec3[1] = vp->vec3[2] = 0;

					const char **wordray = stringlist_nullterm_add (&rlist);
					// space skip - not needed! yay!
					// atof: Function discards any whitespace characters (as determined by isspace) until
					// first non-whitespace character is found. Then it takes as many characters as possible
					// to form a valid floating-point representation and converts them to a floating-point
					// value.

					if (*wordray) colorxs[0]	= atoi(*wordray++);
					if (*wordray) colorxs[1]	= atof(*wordray++);
					if (*wordray) colorxs[2]	= atof(*wordray++);
					stringlistfreecontents (&rlist);

					VectorSet (vp->vec3, colorxs[0] / 255.0, colorxs[1] / 255.0, colorxs[2] / 255.0);
					//vp->vec3[0] = colorxs[0] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[1] = colorxs[1] / 255.0; // 255 = 1, 0 = 0
					//vp->vec3[2] = colorxs[2] / 255.0; // 255 = 1, 0 = 0
				}
			} else {
				c_assert_msg_ (0, "Rect didn't parse");
			}
		}
		break;

//
// ARRAYS
//

	case vtlist_str_20:
		stringlistfreecontents (&vp->stringlist);  // CLEAN IT OUT TO ZERO
		{ // Comma delimited string list.

			stringlistappend_split (&vp->stringlist, s_value, ",");
		}

		break;

	case vtfloatlist_22:
		// Vertexes:"20 20, 500 400, 550 450"
		floats_freecontents (&vp->floatlist);
		// NOT A BAKER ARRAY ... Baker_Array_Erase (&vp->floatlist); // CLEAN IT OUT TO ZERO
		{
			int num_bad =
				floats_append_parse_space_comma_num_elements_ignored
					(&vp->floatlist, s_value, POLYGON_2D_2,  POLYGON_2D_FLOAT_SEPARATOR_SPACE, POLYGON_2D_ELEMENT_SEPARATOR_COMMA);
			if (num_bad)
				Con_PrintLinef ("Float parse %d elements ignored on parse", num_bad);
#ifdef _DEBUG
			floats_dump (&vp->floatlist);
#endif
		}
		break;

	case vtint32list_24:
		// ColumnsWidths:"20 20, 500 400, 550 450"

		// THIS IS NOT A BAKER ARRAY
		///Baker_Array_Erase (&vp->int32list); // CLEAN IT OUT TO ZERO
		int32s_freecontents (&vp->int32list);
		{
			int32s_append_split_dequote (&vp->int32list, s_value, POLYGON_2D_ELEMENT_SEPARATOR_COMMA);
			//if (num_bad)
			//	Con_PrintLinef ("Int parse %d elements ignored on parse", num_bad);
#if 0 //def _DEBUG
			int32s_dump (&vp->int32list);
#endif
		}
		break;



	} // sw

	Z_FreeNull_ (stemp_za);
	return is_ok; // is ok
}

void Property_Dump_Maybe (oject_s *k, things_s *p, stringlist_t *plist, qbool is_indent)
{
	char *s_propname =  p->name;

	// We have a property - is it zero?
	vartype_e	property_vt		= (vartype_e)p->w.vt;
	size_t		offsetx			= p->moffsetof;
	varpack_u	*vp				= (varpack_u *)((byte *)k + offsetx);

	char *s_propval_z = VarPack_String_Zalloc (vp, property_vt);

	if (s_propval_z == NULL)
		return; // Nothing good to write (zero value, no string, etc.)

	stringlistappendf (plist, "%s" "%s:%s", is_indent ? "  " : "", s_propname, s_propval_z);

	Z_FreeNull_ (s_propval_z);
}

WARP_X_CALLERS_ (Object_Dump_Recursive) WARP_X_ (Event_Set_By_String_Is_Ok)
void Event_Dump_Maybe (oject_s *k, things_s *p, stringlist_t *plist, qbool is_indent)
{
	char *s_propname =  p->name;

	// We have a property - is it zero?
	//vartype_e	property_vt		= (vartype_e)p->vt;
	size_t		offsetx			= p->moffsetof;
	varpack_u	*vp				= (varpack_u *)((byte *)k + offsetx);

	char *s_propval_z = VarPack_String_Zalloc (vp, vtstring_1);

	if (s_propval_z == NULL)
		return; // Nothing good to write (zero value, no string, etc.)

	stringlistappendf (plist, "%s" "%s:%s", is_indent ? "  " : "", s_propname, s_propval_z);
	Z_FreeNull_ (s_propval_z);
}




// Destroys name_a or frees an array
qbool Property_Destroy (oject_s *k, things_s *h)
{
	WARP_X_ (oject_s things_s things)
	qbool is_ok = true;
	size_t		offsetx	= h->moffsetof;
	vartype_e	vt		= (vartype_e)h->w.vt;
	varpack_u	*vp		= (varpack_u *) ((byte *)(k) + offsetx);

	switch (vt) {
	case vtstring_1:
		Z_FreeNull_ (vp->char_p);
		break;

	case vtlist_str_20:
		stringlistfreecontents (&vp->stringlist);
		break;

	case vtfloatlist_22:
		floats_freecontents (&vp->floatlist);
		break;

	case vtint32list_24:
		int32s_freecontents (&vp->int32list);
		break;

	case vtcontrolref_23:
		// Just a reference.
		break;

	case vtfloat_2:		// fall thru
	case vtrectf_10:	// fall thru
	case vtrecti_11:	// fall thru
	case vtinteger_19:	// fall thru
	case vtrgb_21:		// fall thru
		break;

	default:
		c_assert_msg_ (0, "Property destroy: Invalid vartype");
	} // sw

	return is_ok; // is ok
}
