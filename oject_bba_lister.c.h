// oject_bba_lister.c.h

oject_s *O_ListBox_Refresh (oject_s *k);
oject_s *O_TabSelect_Refresh_Plus_Early (oject_s *k);
oject_s *O_ListView_Refresh (oject_s *k);
oject_s *O_ContextMenu_Refresh_Plus_Early(oject_s *k);

oject_s *Generic_Refresh (oject_s *k)
{
	switch (k->po->enum_id) {
	case class_contextmenu:	return O_ContextMenu_Refresh_Plus_Early (k);
	case class_listbox:		return O_ListBox_Refresh (k);
	case class_tabselect:	return O_TabSelect_Refresh_Plus_Early (k);
	case class_listview:	return O_ListView_Refresh (k);
	} // sw

	c_assert_msg_ (0, "Impossible3");
	return NULL;
}

int Lister_ListCount (oject_s *k) // Row count
{
	switch (k->po->enum_id) {
	case class_contextmenu:	return k->list_strings_a.numstrings / CM_SERIES_COUNT_4;
	case class_listbox:		return k->list_strings_a.numstrings;

	case class_listview:	if (!k->columncount) return 0;
							return k->list_strings_a.numstrings / k->columncount;
	case class_tabselect:	return k->list_strings_a.numstrings;
	} // sw

	c_assert_msg_ (0, "Impossible2");
	return not_found_neg1;
}

// Tries to ensure an item is fully visible.
WARP_X_CALLERS_ (Lister_KeyDown, Lister_MouseButtonAction)
void Lister_VisRangeCheck_Refresh (oject_s *k, int page_movecount)
{
	int listcount = Lister_ListCount(k);

	// Make sure selected idx is seen as a full row
	viewcalc_s *vc =  ViewCalc_Set (&k->viewcalc, &k->r_interior, k->r_scrollport.top, listcount);

	int viewport_move_units = 0;

	if (page_movecount) {
		// Try to have the adjustment be an entire page
		viewport_move_units = page_movecount;// * vc->rowheight;
	}

	else if (k->selectedindex < vc->first_full_row) {
		viewport_move_units = k->selectedindex - vc->first_full_row;
	}
	else if (k->selectedindex > vc->last_full_row) {
		viewport_move_units = k->selectedindex - vc->last_full_row;
	}

	if (viewport_move_units)
		SET___ k->r_scrollport.top += viewport_move_units * vc->rowheight; // scrolltop += 12 * 10, moving top y down on screen

	SET___ vclamp (k->r_scrollport.top, 0, k->viewcalc.viewportmax);

	if (k->viewcalc.show_scrollbar) {
		float pct = k->r_scrollport.top / (float)k->viewcalc.viewportmax;
		k->in_recursion ++;
			if (k->servo.kscrolly)
				O_Scrollbar_SetPct (k->servo.kscrolly, pct);
		k->in_recursion --;
	}

	Generic_Refresh (k);
}

WARP_X_CALLERS_ (O_ListBox_Refresh O_ListView_Refresh)
oject_s *Lister_Refresh (oject_s *k)
{
	int listcount = Lister_ListCount (k);

	if (k->po->enum_id != class_listview)
		k->r_interior = k->r_screen; // FOR NOW, no border consideration.

	k->viewcalc.rowheight = ceil(k->fontsize);
	viewcalc_s *vc = ViewCalc_Set (&k->viewcalc, &k->r_interior, k->r_scrollport.top, listcount);

	// Determine scrollbar rect
	WARP_X_ (O_ListBox_Spawn Object_Refresh_Recursive)

tabselect_comes_here_too_have_no_scroller:
	if (k->servo.kscrolly) {
		oject_s *kscroll = k->servo.kscrolly;
		kscroll->is_hidden = (vc->show_scrollbar == false);

		if (isin1(k->po->enum_id, class_listview))
			vc->show_scrollbar = listcount > (int)vc->num_visiblef;

		if (vc->show_scrollbar) {
			rect_s r_scroll = k->r_interior;
			r_scroll.left	= RECT_RIGHTOF (k->r_interior) - KNOB_SIZE_16;
			r_scroll.width	= KNOB_SIZE_16;

			// PERFORM ADJUSTMENTS
			k->servo.kscrolly->cm.relative_rect = r_scroll;

			k->r_interior.width -= KNOB_SIZE_16;
		}
	}

	k->r_scrollport.width		= k->r_interior.width;
	k->r_scrollport.height		= k->viewcalc.rowheight * k->list_strings_a.numstrings;

first_row_calc:
	RECT_SET (
		k->viewcalc.r_row_first,
		k->r_interior.left + k->r_scrollport.left,
		k->r_interior.top - k->r_scrollport.top + k->viewcalc.first_vis_row * k->viewcalc.rowheight,
		k->r_interior.width,
		k->viewcalc.rowheight
	);

	//Vid_SetWindowTitlef ("scroll %d r_top " RECTI_4PRINTF, k->r_scrollport.top, RECT_SEND(k->viewcalc.r_row_first) );
	return k;
}

oject_s *Lister_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	if (isdown == false) {
		Form_Hook_Clear (k->fctrl, k);
		return NULL;
	}

	int newidx = ViewCalc_RowCollide (&k->viewcalc, x, y);

	if (newidx == not_found_neg1)
		return k;

	if (newidx != k->selectedindex) {
		k->selectedindex = newidx;
		Lister_VisRangeCheck_Refresh (k, /*movecount*/ 0);
	}

	return k;
}

// This must return null if no action handled
oject_s *Lister_KeyDown (oject_s *k, int key, int ascii)
{
	int listcount = Lister_ListCount (k);
	if (listcount == 0) return NULL; // No listcount

	qbool is_paging = false; // Paging, we want the new index to be top or bottom.
	int oldsel = k->selectedindex;

	switch (key) {
	case K_UPARROW:		k->selectedindex --;	break;
	case K_DOWNARROW:	k->selectedindex ++;	break;
	case K_MWHEELUP:	k->selectedindex -=1; break;
	case K_MWHEELDOWN:	k->selectedindex +=1; break;
	case K_HOME:		k->selectedindex = 0; break;
	case K_PGUP:		k->selectedindex -= k->viewcalc.pageamount; is_paging = true; break;
	case K_PGDN:		k->selectedindex += k->viewcalc.pageamount; is_paging = true; break;
	case K_END:			k->selectedindex = listcount - 1; break;

	default:		return NULL; // NO ACTION
	} // sw

	// If we came here, we did something
	vclamp (k->selectedindex, 0, listcount - 1);

	int pagesize = 0;
	if (is_paging) {
		pagesize = k->selectedindex-oldsel;
	}
	Lister_VisRangeCheck_Refresh (k, pagesize);

	if (oldsel != k->selectedindex) {
		Object_Event_OnChange_Recursive (k, shallrecurse_false);
	}

	return k;
}

void Lister_SetPctHorz (oject_s *k, float pct, oject_s *ksb)
{
	if (k->in_recursion)
		return;

	// The idea here is to set the viewtop.
	//int listcount = Lister_ListCount(k);
	//int num_visible = (int)k->viewcalc.num_visiblef;
	//int not_seen_fully_rows = listcount - (int)k->viewcalc.num_visiblef; // 11 - 10 // 1
	//int num_top_positions = not_seen_fully_rows + 1; // 1 + 1 = 2
	//int this_top_row = Math_Round (pct * not_seen_fully_rows);
	int num_scrollbar_positions = ksb->sb.r_track.width - ksb->sb.r_thumb.width;
	int num_viewport_positions = k->xl.columnsallwidths - k->r_interior.width;
	if (num_scrollbar_positions > 0 && num_viewport_positions > 0) {
		//float pixels_per_position = (k->xl.columnsallwidths - k->r_interior.width) / (float)num_positions;
		k->r_scrollport.left = Math_Round (num_viewport_positions/*pixels_per_position*/ * pct);
		//Vid_SetWindowTitlef ("x scroll %d", k->r_scrollport.left);
//		int j = 5;
	} else {
		k->r_scrollport.left = 0;
		//c_assert_msg_ (0, "div 0 num_positions is 0");
	}

	Generic_Refresh (k);
}

void Lister_SetPct (oject_s *k, float pct, oject_s *ksb)
{
	if (k->in_recursion)
		return;

	if (ksb->is_horizontal) {
		Lister_SetPctHorz (k, pct, ksb);
		return;
	}

	// The idea here is to set the viewtop.
	int listcount = Lister_ListCount(k);
	int num_visible = (int)k->viewcalc.num_visiblef;
	int not_seen_fully_rows = listcount - (int)k->viewcalc.num_visiblef; // 11 - 10 // 1
	int num_top_positions = not_seen_fully_rows + 1; // 1 + 1 = 2
	int this_top_row = Math_Round (pct * not_seen_fully_rows);

	k->r_scrollport.top = this_top_row * k->viewcalc.rowheight;
	//Vid_SetWindowTitlef ("y scroll %d", k->r_scrollport.top);
	//varpf (pct);
	//varpi (listcount);
	//varpi (num_visible);
	//varpi (not_seen_fully_rows);
	//varpi (num_top_positions);
	//varpi (this_top_row);
	//varpi (k->r_scrollport.top);

#pragma message ("Baker: We aren't setting scrollbar value here?  Are we called by scrollbar?")
	// Property_Set_Float (k, prop_value, pct);

	Generic_Refresh (k);
}

// O_TabSelect does not come here, it does not spawn
oject_s *Lister_Spawn (oject_s *k)
{
	Object_Create_And_Assign_By_ClassEnum (class_scrollbar, &k->servo.kscrolly, k->fctrl, SERVANT_KCAN_NULL, k);

	if (isin1(k->po->enum_id, class_listview)) {
		Object_Create_And_Assign_By_ClassEnum(class_scrollbar, &k->servo.kscrollx, k->fctrl, SERVANT_KCAN_NULL, k);
		k->servo.kscrollx->is_horizontal = true;

		// Has a true ZEBRA ("Z-Order top draw servant that draws topmost on form")
		Form_Get(f,k);

		oject_s *kline =
			Object_Create_And_Assign_By_ClassEnum (
				class_rectangle,
				&k->servo.kcolumnresize_line,
				/*form*/ k->fctrl,
				/*container*/ SERVANT_KCAN_NULL,
				/*servant to*/ k->fctrl
			);
		RECT_SET (kline->cm.relative_rect, 200, 200, 200, 200);
		WARP_X_ (O_Rectangle_Refresh)
		kline->is_hidden = true;
		kline->backcolor[0] = 1.0;
		kline->backcolor[1] = 0.5;
		kline->backcolor[2] = 0.5;
	}


	return k;
}



