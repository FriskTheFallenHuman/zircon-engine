// oject_bbx_contextmenu.c.h

WARP_X_ (Object_Create_And_Assign)

oject_s *Object_Refresh_Recursive (oject_s *k);

static void _RunDefaultString (oject_s *k, const char *s_string)
{
	stringlist_t kdeflist = {0};
	stringlistappend_tokenize_qcquotes (&kdeflist, s_string);

	// MaxValue:100
	const char **wordray = stringlist_nullterm_add (&kdeflist);
	for (/*nada*/ ; *wordray; *wordray ++) {
		things_s *thing = Thing_Find (*wordray);


		c_assert_msg_ (IsProperty(thing), "property is messed");


		// Advance past property
		*wordray ++; Word_Check_Fail_Message ("No more words after property");

		c_assert_msg_ (String_Match(*wordray, PROPERTY_COLON_DELIM), "no colon after property is messed");

		// Advance past semi-colon
		*wordray ++; Word_Check_Fail_Message ("No more words after property colon");

		// PROCESS PROPERTY
		ccs *s_value = *wordray;

		qbool is_ok;
		is_ok = Property_Set_By_String_After_Freeing_Is_Ok (k, thing, s_value);
	} // for words

}


EXO___ int Object_Property_Set_Fmt(oject_s *k, ccs *s_propertyname, ccs *fmt, ...)
{ // s_propertyvalue_unquoted
	things_s *po = Thing_Find(s_propertyname);
	if (!po) {
		return IERR_PROP_NOT_FOUND_1;
	}

	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	char *s_propertyvalue_unquoted = text;
	int isok = Property_Set_By_String_After_Freeing_Is_Ok(k, po, /*s_propertyvalue_unquoted*/ text);
	VA_EXPAND_ALLOC_FREE (text);

	if (!isok) return IERR_PROP_SET_ERROR_2;
	return OK_NOERROR_0;
}

EXO___ int Object_Property_Set(oject_s *k, ccs *s_propertyname, ccs *s_propertyvalue_unquoted)
{
	things_s *po = Thing_Find(s_propertyname);
	if (!po) return IERR_PROP_NOT_FOUND_1;

	int isok = Property_Set_By_String_After_Freeing_Is_Ok(k, po, s_propertyvalue_unquoted);
	if (!isok) return IERR_PROP_SET_ERROR_2;
	return OK_NOERROR_0;
}

void Object_Class_Default_Values (oject_s *k, things_s *po)
{
	if (po->sdefault == NULL || po->sdefault[0] == NULL_CHAR_0)
		return; // No defaults

	_RunDefaultString (k, po->sdefault);
}

void Object_Global_Default_Values (oject_s *k, things_s *po)
{
	if (k->po->enum_id != class_form_1) {
		things_s *poform = k->fctrl->po;
		_RunDefaultString (k, poform->sdefault);
	}
}

// Occurs in parser when a new object is about to be created or after last parse.
oject_s *Object_Parse_Done (oject_s *k)
{
	// We don't do anything right now.
	return k;
}

// Does a form come here? YES -> Form_Life
WARP_X_CALLERS_ (Form_Life)

oject_s *Object_Create_And_Assign (oject_s **pk, oject_s *f, oject_s *container, oject_s *servant_to, things_s *po)
{
	if (*pk) {
		c_assert_msg_ (0, "Object_Create_And_Assign object already exists!");
	}

	LIFE_ALLO___ oject_s *k = (oject_s *)Mem_ZMalloc_SizeOf (oject_s);

//	if (po->enum_id == class_form_1)
//		int j = 5;

	k->po				= po;
	k->fctrl			= f;
	k->container		= container;
	k->servant_owner	= servant_to;
	k->cm.creation_idx  = (po->enum_id == class_form_1) ? 0 :f->cm.creation_idx ++; // k is not a form here.

	// DEFAULT a NAME
	things_s *po_name = Thing_Find_By_Enum_Id (prop_name);

	if (k->po->enum_id == class_form_1) {
		Property_Set_By_String_After_Freeing_Is_Ok (k, po_name, "Form1");
	} else {
		char *sz = Z_StrDupf ("%s%d", k->po->name, k->cm.creation_idx);
		Property_Set_By_String_After_Freeing_Is_Ok (k, po_name, sz);
		Z_FreeNull_ (sz);
	}

	// FORM
	if (po->enum_id != class_form_1) {
	// FORM EXCLUDED: ADD K TO FORM COLLECTION
//collection_add_not_form:
		int idx = Baker_Array_NewIndex(f->frm.controls_a);
		f->frm.controls_a->pointers_array[idx] = k;
	}

	Object_Global_Default_Values (k, po);
	if (k->po->enum_id != class_form_1) {
		WARP_X_ (THEME_DEFAULTS)
inherit_from_modified_form_value:
		VectorCopyDestSrc  (k->forecolor, f->forecolor); // Black
		VectorCopyDestSrc  (k->backcolor, f->backcolor); // White
		VectorCopyDestSrc  (k->backcolorselected, f->backcolorselected); // Black
		VectorCopyDestSrc  (k->forecolorselected, f->forecolorselected); // White
		k->fontsize = f->fontsize;// 24; // Default 24 for now
	}

	Object_Class_Default_Values (k, po); // Object type

//EXAMPLE: IH	{"BackColor",			prop_backcolor,			vtrgb_21,		member_offsetof (form1, backcolor),		},

spawn_servants:

	switch (k->po->enum_id) {
	//case class_form_1:	Form_ContextMenu_Spawn (k);	break; // NO!  TOO EARLY NO control array yet!
	case class_listbox:		O_ListBox_Spawn (k);		break;
	case class_listview:	O_ListView_Spawn (k);		break;
	} // sw

	(*pk) = k; // return k;
	return k;
}

oject_s *Object_Create_And_Assign_By_ClassEnum (int class_id, oject_s **pk, oject_s *f, oject_s *container, oject_s *servant_to)
{
	things_s *po = Thing_Find_By_Enum_Id (class_id);
	return Object_Create_And_Assign (pk, f, container, servant_to, po);
}

// FORM does not come here to draw
WARP_X_CALLERS_ (Form_Draw)

WARP_X_ (Label_Rect_Edit_Align)



oject_s *Object_Draw_Recursive_Not_Form (oject_s *k)
{
	Form_Get(f,k);

	// OBJECT ENTIRELY CULLED
	if (CLIPPING_ON && k->r_halo_clipped.width == RECT_CULLED_ZERO)
		return NULL;  // CULLED HIGHLIGHT

	int have_focus = k->fctrl->frm.kfocused == k;

	// DRAW HIGHLIGHT =============================================================
	if (have_focus && Have_Flag(k->po->w.oflags, OFCAN_NOHIGHLIGHT_64) == false ) {
		if (CLIPPING_ON)
			Draw_Clip_Set_Rect (&k->r_halo_clipped);

		Draw_Box (&k->r_halo, BOXWIDTH_2, color3_gray_50, alpha_1_0);
	}

	// DRAW HIGHLIGHT =============================================================
	if (CLIPPING_ON && k->r_screen_clipped.width == RECT_CULLED_ZERO) {
		return NULL; // CULLED FOOTPRINT
	}

	if (CLIPPING_ON) {
		Draw_Clip_Set_Rect (&k->r_screen_clipped);
	}

	// DRAW CONTROL
	switch (k->po->enum_id) {
	case class_checkbox:	O_CheckBox_Draw(k);		break;
	case class_contextmenu:	O_ContextMenu_Draw(k);	break;
	case class_image:		O_Image_Draw(k);		break;
	case class_label:		O_Label_Draw(k);		break;
	case class_listbox:		O_ListBox_Draw(k);		break;
	case class_listview:	O_ListView_Draw(k);		break;
	case class_polygon:		O_Polygon_Draw(k);		break;
	case class_rectangle:	O_Rectangle_Draw(k);	break;
	case class_scrollbar:	O_ScrollBar_Draw(k);	break;
	case class_sizedot:		O_SizeDot_Draw (k);		break;
	case class_tabselect:	O_TabSelect_Draw(k);	break;
	case class_textbox:		O_TextBox_Draw(k);		break;
	default:				break;
		break;
	} // sw

	// RECURSE CONTAINED
	for_each_form_control_with_container_as_no_servants (n, f, k, kcontained)
		if (kcontained->is_hidden) continue; // Don't draw hidden.
		Object_Draw_Recursive_Not_Form (kcontained);
	for_each_end

	// ZEBRAZ
	for_each_form_servant_owner_is_k (n, f, /*servant owner*/ k, kservant)
//		if (kservant->po->enum_id == class_rectangle)
//			int j = 5;

		if (kservant->is_hidden) continue; // Don't draw hidden.
		Object_Draw_Recursive_Not_Form (kservant);
	for_each_end

	if (CLIPPING_ON) {
		Draw_Clip_Clear (); // Submits draw buffer as side-effect.
	}
	return k;
}

WARP_X_CALLERS_ (Form_Complete)
// Forms do come here.  Servants should come here.
oject_s *Object_Refresh_Recursive (oject_s *k)
{
	Form_Get (f, k);
	// Resist thinking of using console width for magnification.  Blurry.
	// Future: Why is blurry?
	const rect_s r_fullscreen = {0, 0, vid.width, vid.height};
	const rect_s *pr_container_clipped = k->container ? &k->container->r_interior_clipped : &r_fullscreen;

	if (k->servant_owner) {
		// Special case: Shared footprint with controller.
		if (k->servant_owner->container)
			pr_container_clipped = &k->servant_owner->container->r_interior_clipped; // Form servant
		else pr_container_clipped = &r_fullscreen;
	}

variably_sized:
	// SOLIDIFY VARIABLE SIZED OBJECTS
	// refresh_reason_preautosize_1 - ask size before screen/interior sizes are calculated
	switch (k->po->enum_id) {
	case class_contextmenu:	O_ContextMenu_Refresh_Plus_Early(k);break;
	case class_label:		O_Label_Refresh_Plus_Early(k);		break;
	case class_tabselect:	O_TabSelect_Refresh_Plus_Early(k); break;
	} // sw

	k->r_screen = k->cm.relative_rect;
	if (k->container) {
		k->r_screen.left	+= k->container->r_interior.left;
		k->r_screen.top		+= k->container->r_interior.top;
	}

	SET___ k->r_interior = k->r_screen;

	//
	// Set clip areas
	//
	rect_s r_highlight = { RECT_SEND_EXPANDED(k->r_screen, BOXWIDTH_2) };

	// Calculate the focus area
	Rect_Calc_Clipped_Unclipped_From_XYWH (
		SET___ &k->r_halo,
		SET___ &k->r_halo_clipped,
		pr_container_clipped,
		RECT_SEND(r_highlight)
	);

	// Calculate the part we show
	Rect_Calc_Clipped_Unclipped_From_XYWH (
		SET___ &k->r_screen,
		SET___ &k->r_screen_clipped,
		pr_container_clipped,
		RECT_SEND(k->r_screen)
	);

	// Calculate our interior
	Rect_Calc_Clipped_Unclipped_From_XYWH (
		SET___ &k->r_interior,
		SET___ &k->r_interior_clipped,
		pr_container_clipped, // This is allowed to be null
		RECT_SEND(k->r_interior)
	);

	// Accumulate the draw
	while (1) {
		if (CLIPPING_ON && k->r_screen_clipped.width == RECT_CULLED_ZERO)
			break; // CULLED FOOTPRINT

		// Add object to the drawn list
		int idx = Baker_Array_NewIndex(k->fctrl->frm.drawn_list_a);
//		if (idx > 100)
//			int j = 5;
		k->fctrl->frm.drawn_list_a->pointers_array[idx] = k;

		break; // GET OUT
	} // while

	// Some of these solidify positioning.
	switch (k->po->enum_id) {
	case class_contextmenu:	O_ContextMenu_Refresh_Plus_Early(k);	break;
	case class_checkbox:	O_CheckBox_Refresh(k);	break;
	case class_label:		O_Label_Refresh_Plus_Early(k); break;
	case class_listbox:		O_ListBox_Refresh (k);	break;
	case class_listview:	O_ListView_Refresh(k);	break;
	case class_polygon:		O_Polygon_Refresh(k);	break;
	case class_rectangle:	O_Rectangle_Refresh(k);	break;
	case class_scrollbar:	O_ScrollBar_Refresh(k);	break;
	case class_tabselect:	O_TabSelect_Refresh_Plus_Early(k);	break;
	case class_textbox:		O_TextBox_Refresh(k);	break;
	default:				break;
	} // sw

	if (f->frm.refreshdots && f->frm.editmode && k == f->frm.kfocused) {
		EditModeSetDotsObject (f->frm.kfocused);
		f->frm.refreshdots = 0;
	}


refresh_contained_controls:
	for_each_form_control_with_container_as_no_servants (n, f, k, kcontained)
		if (kcontained->is_hidden) continue; // Don't draw hidden.
		Object_Refresh_Recursive (kcontained);
	for_each_end

	for_each_form_servant_owner_is_k (n, f, k, kservant)
//		if (kservant->po->enum_id == class_rectangle)
//			int j = 5;
		if (kservant->is_hidden) continue; // Don't draw hidden.
		Object_Refresh_Recursive (kservant);
	for_each_end

	return NULL;
}

oject_s *Object_Finalize_Recursive (oject_s *k)
{
	Form_Get(f,k);

	switch (k->po->enum_id) {
	case class_label:		O_Label_Finalize(k);	break;
	case class_listview:	O_ListView_Finalize(k);	break;
	case class_textbox:		O_TextBox_Finalize(k);	break;
	default:				break;
	} // sw

finalize_contained_controls:

	for_each_form_control_with_container_as_no_servants (n, f, k, kcontained)
		if (kcontained->is_hidden) continue; // Don't draw hidden.
		Object_Finalize_Recursive (kcontained);
	for_each_end

	for_each_form_servant_owner_is_k (n, f, k, kservant)
		if (kservant->is_hidden) continue; // Don't draw hidden.
		Object_Finalize_Recursive (kservant);
	for_each_end

	return NULL;
}

WARP_X_CALLERS_ (Form_Dump)
void Object_Dump_Recursive (oject_s *k, stringlist_t *plist, dump_detail_e detail_level)
{
	Form_Get(f,k);
	char *s_ot = k->po->name;

//	if (k->po->enum_id == class_scrollbar)
//		int j =5 ;

	// Name and Rect Dump
	stringlistappendf (plist, "%s", s_ot);
	if (k->po->enum_id == class_form_1) {
		// Form does not print name or rect
	} else {
		Property_Dump_Maybe (k, Thing_Find_By_Enum_Id(prop_name), plist, q_indent_true);
		Property_Dump_Maybe (k, Thing_Find_By_Enum_Id(prop_rect), plist, q_indent_true);
	}

	// Property Dump
	for (things_s *p = &things[0]; p->name; p ++) {
		if (false == in_range (prop_rect + 1, p->enum_id, prop_end))
			continue;

		Property_Dump_Maybe (k, p, plist, q_indent_true);
	} // for each property

	// Event Dump
	//for (things_s *p = &things[0]; p->name; p ++) {
	//	if (false == in_range (events_start, p->enum_id, events_end))
	//		continue;

	//	Event_Dump_Maybe (k, p, plist, q_indent_true);
	//} // for each property

	if (detail_level == DUMP_DETAIL_PLUS_CLIPPING_2) {
		stringlistappendf (plist, "  " "r_halo " RECTI_PRINTF, RECT_SEND(k->r_halo));
		stringlistappendf (plist, "  " "r_halo_clipped " RECTI_PRINTF, RECT_SEND(k->r_halo_clipped));
		stringlistappendf (plist, "  " "r_screen " RECTI_PRINTF, RECT_SEND(k->r_screen));
		stringlistappendf (plist, "  " "r_screen_clipped " RECTI_PRINTF, RECT_SEND(k->r_screen_clipped));
		stringlistappendf (plist, "  " "r_interior " RECTI_PRINTF, RECT_SEND(k->r_interior));
		stringlistappendf (plist, "  " "r_interior_clipped " RECTI_PRINTF, RECT_SEND(k->r_interior_clipped));
	}

	// Dump contained
	for_each_form_control_with_container_as_no_servants (n, f, k, kcontained)
		// Servants are internal and do not write to file
		Object_Dump_Recursive (kcontained, plist, detail_level);
	for_each_end

	if (detail_level >= DUMP_DETAIL_PLUS_SERVANTS_1) {
		for_each_form_servant_owner_is_k (n, f, k, kservant)
			Object_Dump_Recursive (kservant, plist, detail_level);
		for_each_end
	} // detail plus servants

}


// Does not touch form control array
WARP_X_CALLERS_ (Form_Life)

// Q: Does a form come here? Yes. Last.
oject_s *Object_Destroy_Not_Recursive (oject_s *k)
{
	// Find each property.
	for (things_s *p = &things[0]; p->name; p ++) {
		if (IsProperty (p) == false) continue;
		Property_Destroy (k, p);
	}

	// zebra_servants OBJECTS get destroyed with form.
	// We need to destroy the array.
	//Mem_FreeNull_ (k->servo.zebra_servants);
	NPA__ LIFE_FREE___ /*oject_s*/ Mem_FreeNull_ (k);
	return k;
}

		// What is method in
#pragma message ("Is form in drawn array?")
oject_s *Oject_Hit_Drawn (oject_s *f, int x, int y, int isbuttonaction)
{
	oject_s *khit = NULL;
	for_each_form_drawn_reverse	(n,f,kdrawn)
		qbool did_hit = RECT_HIT (kdrawn->r_screen_clipped, x, y);
		if (did_hit == false)
			continue;

//		if (isbuttonaction)
//			int j = 5;

		// EDITMODE: Even non-mouseable objects can get focus if they are NOT a servant
		// Servant that should not get focus in editmode is like column line or xscroll for a listview
		if (!(Oject_Will_Mouse(kdrawn) || f->frm.editmode && kdrawn->servant_owner == NULL))
			continue;

		khit = kdrawn; // HIT
		break;
	for_each_end // each k
	return khit;
}




qbool Oject_Will_Focus_Normally (oject_s *k)
{
	// Visible.  Enabled.
	if (k->servant_owner)
		return false; // Servants are mouse interactive only

	if (Have_Flag (k->po->w.oflags, OFCAN_FOCUS_2))
		return true;

	if (k->is_hidden)
		return false;

	return false;
}

qbool Oject_Will_Mouse (oject_s *k)
{
	if (k->is_hidden)
		return false;

	if (Have_Flag (k->po->w.oflags, OFCAN_MOUSE_4)) {
		// If not disabled, hidden or such ..
		return true;
	}

	return false;
}

oject_s *Object_Find_Name (oject_s *f, const char *s_name_wanted)
{
	for_each_form_all_controls (n,f,k)
		if (k->cm.name_a) {
			if (String_Match(k->cm.name_a, s_name_wanted))
				return k;
		} else {
			c_assert_msg_ (0, "Object have no name");
		}
	for_each_end
	return NULL;
}

oject_s *Oject_Hidden_Set (oject_s *k, int newvalue)
{
	if (newvalue == (int)true && Oject_Have_Focus (k)) {
		Form_Focus_Next (k->fctrl, DIR_FORWARD_1);
		if (Oject_Have_Focus (k)) {
			k->fctrl->frm.kfocused = k->fctrl; // Set focus to form if all else fails
		}
	}

	k->is_hidden = newvalue;
	return k;
}

oject_s *Oject_Focus_Set (oject_s *k)
{
	oject_s *f = k->fctrl;

	if (f->frm.kfocused == k)
		return k; // Nothing to do

	//if (f->frm.kcontextoldfocus)
	//	f->frm.kcontextoldfocus = NULL;

	// Examine current focused object
	oject_s *kfok = f->frm.kfocused;
	if (kfok) {
		// About to lose focus
		if (Have_Flag (kfok->po->w.oflags, OFCAN_ONLYFOCUS_128)) {
			// Context menu or something that can only exist with focus
			kfok->is_hidden = true;
		}

	}

	// SET
//set_focus:
//	if (k == f)
//		int j = 5;
	f->frm.kfocused = k;
	if (f->frm.editmode) {
		Form_QueueDotsRefresh (f);
		//EditModeSetDotsObject (f);
	}

	return f;
}


exofn_t Event_Find_Function(ccs *s_fn)
{
	for (int j = 0; j < (int)ARRAY_COUNT(event_entries); j ++) {
		event_entry_s *zi = &event_entries[j];
		if (String_Match_Caseless(zi->name, s_fn)) {
			return zi->fn;// (f, k, listidx, s);
		}
	}
	//Con_PrintLinef ("zexeccmd: command function " QUOTED_S " not found", s_fn);
	return NULL;
}


WARP_X_ (DevInit Object_Draw_Recursive_Not_Form)
void Object_Event_OnChange_Recursive (oject_s *k, int shall_recurse)
{
	if (k->eve.onchange_a && k->eve.onchange_a[0]) {
		exofn_t onclick_fn = Event_Find_Function(k->eve.onchange_a);
		if (onclick_fn) {
			char *s_error_za = onclick_fn (k);
			if (s_error_za) {
				Con_PrintLinef ("ERROR: %s: " QUOTED_S, k->eve.onchange_a, s_error_za);
				Z_FreeNull_ (s_error_za);
			} // if error
		} else {
			// NOT found
			c_assert_msg_ (0, "Object_Event_OnChange not found!");
		}
	} // if text set

	if (shall_recurse) {
		Form_Get (f,k);
		for_each_form_control_with_container_as_no_servants (n, f, k, kcontained)
			if (kcontained->is_hidden) continue; // Don't draw hidden.
			Object_Event_OnChange_Recursive (kcontained, shall_recurse);
		for_each_end
	}
}



