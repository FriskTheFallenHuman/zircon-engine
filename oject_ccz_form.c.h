// oject_ccw_form_parse.c.h

WARP_X_ (Lister_Spawn O_ContextMenu_Draw)

oject_s *Form_ContextMenu_Spawn (oject_s *f)
{

	// Baker: CM_MIDDLE_DOT_4 comes first, covers everything and doesn't draw.
	{
		int dd = CM_MIDDLE_DOT_4;
		oject_s *kdot = Object_Create_And_Assign_By_ClassEnum(class_sizedot, &f->servo.kdots[dd], f,
		SERVANT_KCAN_NULL, /*serv owner*/ f);

		kdot->direction = dd;
		kdot->is_hidden = true;
	}

	for (int dd = 0; dd < CM_DOTCOUNT_COUNT_9; dd ++) {
		if (dd == CM_MIDDLE_DOT_4)
			continue;

		oject_s *kdot = Object_Create_And_Assign_By_ClassEnum(class_sizedot, &f->servo.kdots[dd], f,
			SERVANT_KCAN_NULL, /*serv owner*/ f);

		kdot->direction = dd;
		kdot->is_hidden = true;
	}

//kmu_is_zsupreme_or_wont_work_in_edit_mode:
	oject_s *kmnu = Object_Create_And_Assign_By_ClassEnum(class_contextmenu, &f->servo.kcontextmenu, f, SERVANT_KCAN_NULL, /*serv owner*/ f);

		RECT_SET (kmnu->cm.relative_rect, 200, 200, 200, 0);
		kmnu->cellspacingypct = 0;

		WARP_X_ (O_ContextMenu_Draw)
		VectorCopyDestSrc (kmnu->backcolor, color3_gray_875);
		VectorCopyDestSrc (kmnu->backcolorselected, color3_red);
		//WARP_X_ (O_Rectangle_Refresh)
		things_s *po_list = Thing_Find_By_Enum_Id (prop_list);
		Property_Set_By_String_After_Freeing_Is_Ok(kmnu, po_list,
			//CM_0_TEXT_0
			//CM_1_CHECKED_1
			//CM_2_STRING_ID_2
			//CM_3_FUNCSTRING_3
			"Form State To Clipboard,0,Print,ObjectCommand,"
			" ,0,Option2String,Option2FunctionString,"
			" ,0,Option3String,Option3FunctionString,"
			"-,0,DASH,DASH,"
			"Edit Mode,0,EditModeString,EditModeToggle" // No trailing comma
		);

		kmnu->is_hidden = true;

	return kmnu;
}

oject_s *Form_Draw (oject_s *f)
{
	GoogleRobotoFont_Check ();

refresh_check:
	if (f->frm.refreshneeded || f->frm.refreshdots) {
		Form_Finalize_Refresh (f);
		f->frm.refreshneeded = 0;
		//f->frm.refreshdots = 0;  Done in the loop
	}

//	if (f->frm.kfocused == f)
//		int j = 5;
//	if (f->frm.kfocused == NULL)
//		int j = 5;


	// PREDRAW - A hook might do a refresh
	if (f->frm.k_mousehook && host.realtime >= f->frm.hook_nextthink) {
		// ACTIVE HOOK
		mouse_hook_fn_t hook_fn = f->frm.hook_fn;
		oject_s *k_hook = f->frm.k_mousehook;

		// CLEAR THE HOOK.  Function might set it again.
		f->frm.k_mousehook = NULL;
		f->frm.hook_fn = NULL;
		f->frm.hook_nextthink = 0;

		WARP_X_ (O_ScrollBar_MouseHook_Fire)
		hook_fn (k_hook); // FIRE!
	}

	// ACTUAL DRAW - FORM IS NOT in the list of drawn
	WARP_X_ (drawn_list_a)
	for_each_form_control_with_container_as_no_servants (n, f, f, k)
		Object_Draw_Recursive_Not_Form (k);
	for_each_end

	// Any form servants have container null just like a form does.
	// All servants are zebras (Z-Order on top which means they draw last)
	for_each_form_servant_owner_is_k (n, f, /*servant owner*/ f, kservant)
//		if (kservant->po->enum_id == class_rectangle)
//			int j = 5;

		if (kservant->is_hidden) continue; // Don't draw hidden.
		Object_Draw_Recursive_Not_Form (kservant);
	for_each_end


	Draw_Clip_Clear (); // Submits draw buffer
	return f;
}


WARP_X_CALLERS_ (/*Object_Parse_Done*/  Form_Create_From_String /*Finalize Refresh*/)
WARP_X_ (Object_Event_OnChange_Recursive)
void Form_Event_Onload (oject_s *f)
{
	if (f->eve.onload_a && f->eve.onload_a[0]) {
		exofn_t onload_fn = Event_Find_Function(f->eve.onload_a);
		if (onload_fn) {
			char *s_error_za = onload_fn (f);
			if (s_error_za) {
				Con_PrintLinef ("ERROR: %s: " QUOTED_S, f->eve.onload_a, s_error_za);
				Z_FreeNull_ (s_error_za);
			}
		} else {
			c_assert_msg_ (0, "Form_Event_Onload not found!");
		}
	}

	Object_Event_OnChange_Recursive (f, shallrecurse_true); // Form ONLOAD
}


void Form_Finalize_Refresh (oject_s *f)
{
	GoogleRobotoFont_Check ();

	// Allow the form to be in the array.
	NPA__ Baker_Array_Erase (f->frm.drawn_list_a);

	// Seal the form
	Object_Finalize_Recursive (f); // Runs finalize function
	Object_Refresh_Recursive (f); // Refresh the footprint

	// If no focus set, set focus to first one that we can
	if (f->frm.kfocused == NULL) {
		for_each_form_all_controls (n, f, k)
			if (Oject_Will_Focus_Normally(k) == false) continue;
			f->frm.kfocused = k;
//			if (k == f)
//				int j = 5;
			break;
		for_each_end
	} // find something to focus
}

void Form_ZOrder (oject_s *kzlast)
{
	Form_Get (f, kzlast);
	int fidx = Form_Find_Idx(f, kzlast);
	int fcount = f->frm.controls_a->numitems;
	if (fidx == fcount -1)
		return; // Already last

	f->frm.controls_a->pointers_array[fidx] = NULL;
	// So we nulled out #20 or #0
	//0 = 1, 1 = 0
	//1 = 2, 2 = 0
	//for count-1 to 1

	// Shift all down from fidx to end.  We could memcpy to do it better.
	// WATCHOUT must do --> f->frm.controls_a->numitems - 1 or we write out of bounds
	// because array[count - 1 + 1] is beyond limit
	for (int jidx = fidx; jidx < f->frm.controls_a->numitems - 1; jidx ++) {
		oject_s *kthis  = f->frm.controls_a->pointers_array[jidx + 0];
		c_assert_msg_ (kthis == NULL, "Form_ZOrder this shouldn't be null");
		oject_s *kafter = f->frm.controls_a->pointers_array[jidx + 1];
		f->frm.controls_a->pointers_array[jidx + 0] = kafter;
		f->frm.controls_a->pointers_array[jidx + 1] = NULL;
	} // for

	f->frm.controls_a->pointers_array[f->frm.controls_a->numitems - 1] = kzlast;
}

void Form_Print_Zones(oject_s *f)
{
	for (int j = 0; j < f->frm.drawn_list_a->numitems; j ++) {
		oject_s *k = f->frm.drawn_list_a->pointers_array[j];

		Con_PrintLinef ("%4d " S_FMT_LEFT_PAD_20 " r = " RECTI_4PRINTF " clipped r = " RECTI_4PRINTF,
			j, k->po->name, RECT_SEND(k->cm.relative_rect), RECT_SEND(k->r_screen_clipped));
	} // for
}

void Form_Dump (oject_s *f, stringlist_t *plist, dump_detail_e detail_level)
{
	Object_Dump_Recursive (f, plist, detail_level);
}


void Form_Draw_Dump (oject_s *f, stringlist_t *plist)
{
	for (int j = 0; j < f->frm.drawn_list_a->numitems; j ++) {
		oject_s *kdrawn = f->frm.drawn_list_a->pointers_array[j];
		stringlistappendf (plist, "%4d: " S_FMT_LEFT_PAD_20 " " RECTI_4PRINTF ,
			j,
			kdrawn->cm.name_a,
			RECT_SEND (kdrawn->r_screen_clipped)
		);
	}
}

// This does ALL controls
static int sForm_Get_Focused_Idx (oject_s *f, oject_s *kfocused, int *p_num_can_focus)
{
	int kfocus_idx = not_found_neg1;

	for_each_form_all_controls(n,f,k) // ALL CONTROLS!
		if (Oject_Will_Focus_Normally(k) == false) continue;

		(*p_num_can_focus) ++;
		if (k == kfocused)
			kfocus_idx = n; // Found it.
	for_each_end

	return kfocus_idx;
}

int Form_Find_Idx (oject_s *f, oject_s *kfind)
{
	for_each_form_all_controls(n,f,k) // ALL CONTROLS!
		if (k == kfind)
			return n;
	for_each_end

	return not_found_neg1;
}

static oject_s *sFind_Next_Prev_Focus_Control(oject_s *f, oject_s *k_oldfocus, int focusidxstart, int dir)
{
	int iters = 0;
	for (int n = focusidxstart + dir; iters < f->frm.controls_a->numitems; n+= dir, iters ++) {
		if (dir > 0 && n >=  f->frm.controls_a->numitems)
			n = 0; // Loop high
		else if (dir < 0 && n < 0)
			n = f->frm.controls_a->numitems - 1; // Loop low

		oject_s *k = f->frm.controls_a->pointers_array[n];

		if (Oject_Will_Focus_Normally (k) == false || k == k_oldfocus)
			continue;

		// Found next or prev control that can focus
		return k;
	} // for

	return NULL;
}



void Form_Focus_Next (oject_s *f, int dir)
{ c_assert_ (isin2(dir, -1, 1));

	oject_s *koldfocus = f->frm.kfocused;

	// find our index so we can cycle through starting there.
	int numcanfocus = 0, focusidxstart = sForm_Get_Focused_Idx(f, koldfocus, &numcanfocus);

	if (numcanfocus <= 1) // Focus change impossible if 1 or less sub object
		return;

	f->frm.kfocused = sFind_Next_Prev_Focus_Control(f, koldfocus, focusidxstart, dir);
}




WARP_X_ (Object_Destroy_Recursive)
oject_s *Form_Life (oject_s *f)
{
	if (f == CREATE_NULL) {
		// Create Form
		oject_s *f = NULL;
		Object_Create_And_Assign_By_ClassEnum (class_form_1, &f, FORM_NULL, FORM_KCAN_NULL, SERVANT_TO_NULL);
		f->fctrl = f;
		NPA__ LIFE_ALLO___ f->frm.controls_a = (oject_array_s *)Mem_ZMalloc_SizeOf(*f->frm.controls_a);
		NPA__ LIFE_ALLO___ f->frm.drawn_list_a = (oject_array_s *)Mem_ZMalloc_SizeOf(*f->frm.drawn_list_a);

		Form_ContextMenu_Spawn (f);

		RECT_SET (f->cm.relative_rect, 0, 0, vid.width, vid.height);
		return f;
	}

	// DESTROY ALL CONTROLS (the form is not in its own list, we destroy it last).
	for_each_form_all_controls(n,f,k)
		f->frm.controls_a->pointers_array[n] = Object_Destroy_Not_Recursive (f->frm.controls_a->pointers_array[n]);
	for_each_end

	LIFE_FREE___ NPA__ Mem_FreeNull_ (f->frm.controls_a->pointers_array); // Baker_Array_Erase
	LIFE_FREE___ NPA__ Mem_FreeNull_ (f->frm.controls_a);
	LIFE_FREE___ NPA__ Mem_FreeNull_ (f->frm.drawn_list_a->pointers_array); // Baker_Array_Erase
	LIFE_FREE___ NPA__ Mem_FreeNull_ (f->frm.drawn_list_a);
	f = Object_Destroy_Not_Recursive (f);

	Vid_Cursor_Set (mousepointer_arrow_default_0);
	return f; // NULL
}

oject_s *Form_Destroy (oject_s *f)
{
	return Form_Life(f); // Destroys it
}

WARP_X_CALLERS_ (Form_Create_From_String)
oject_s *Form_Create (void)
{
	return Form_Life(CREATE_NULL); // Creates it
}


void Form_Hook_Set (oject_s *f, oject_s *k, mouse_hook_fn_t fn, timey_dbl_t hookwhen)
{
	f->frm.k_mousehook = k;
	f->frm.hook_fn = fn;
	f->frm.hook_nextthink = hookwhen;
}

void Form_Hook_Clear(oject_s *f, oject_s *k)
{
	if (f->frm.k_mousehook && f->frm.k_mousehook != k) {
		//int j = 5;
		return; // A control can only clear its own
	}
	f->frm.k_mousehook = NULL;
	f->frm.hook_fn = NULL;
	f->frm.hook_nextthink = 0;
}


