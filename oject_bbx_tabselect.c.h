// oject_bbx_tabselect.c.h


WARP_X_ (O_TabSelect_Refresh_Plus_Early O_TabSelect_MouseDown)

// Baker: TabSelect used to scroll .. it no longer does.
// Plan?  Make it Y scroll without a scrollbar
// Have it calculate if the top item is in view ... 

WARP_X_ (O_ListBox_Draw)
oject_s *O_TabSelect_Draw (oject_s *k)
{
	rect_s	r_button	= k->tb.r_button;
	rect_s	r_text		= k->txta.r_textarea;

	int start_idx	= k->viewcalc.first_vis_row;
	int count		= k->viewcalc.rowcount;
	int beyond_idx	= start_idx + count;

	for (int n = start_idx; n < beyond_idx; n ++, r_button.top += k->viewcalc.rowheight, r_text.top += k->viewcalc.rowheight /*r_cursor.top += k->viewcalc.rowheight*/) {
//	for (int n = 0; n < k->list_strings_a.numstrings; n ++, r_button.top += k->tb.buttonstep, r_text.top += k->tb.buttonstep) {
		const char *s = k->list_strings_a.strings[n];
		
		int		is_selected		= (n == k->selectedindex);
		vec_t	*backcolor		= is_selected ? k->backcolorselected : k->backcolor;
		vec_t	*forecolor		= is_selected ? k->forecolorselected : k->forecolor;

		if (k->is_rounded)	Draw_RectRound (&r_button, backcolor, k->backalpha);
		else				Draw_Rect	(&r_button, backcolor, k->backalpha);
		Draw_String	(&r_text, s, k->fontsize, forecolor, alpha_1_0);
	} // for

	return k;	
}

// Early should expect to set cm.relative_rect width and height, but never left or top

WARP_X_ (TABSELECT_DEFAULTS)
oject_s *O_TabSelect_Refresh (oject_s *k)
{
	int listcount = Lister_ListCount(k);

	k->tb.stringwidthmaxpx	= stringlist_maxwidthint(&k->list_strings_a, zdev_dpfont, k->fontsize);

	// Assume infinite size on finite canvas
	float	descender_pct			= zdev_dpfont->ft_baker_descend_pct;
	float	descender_reduce_pct	= 1 - bound(0,k->fontdescendpct,1);
	int		descender_reduce_px		= ceil(descender_pct * descender_reduce_pct * k->fontsize);
	
	k->tb.cellpaddingx_i	= ceil(k->cellpaddingxpct * k->fontsize);
	k->tb.cellpaddingy_i	= ceil(k->cellpaddingypct * k->fontsize);
	k->tb.cellspacing_i		= ceil(k->cellspacingypct * k->fontsize);

	k->tb.r_button.left		= k->cm.relative_rect.left;
	k->tb.r_button.top		= k->cm.relative_rect.top;
	k->tb.r_button.width	= (k->tb.cellpaddingx_i * 2) + k->tb.stringwidthmaxpx;
	k->tb.r_button.height	= (k->tb.cellpaddingy_i * 2) + ceil(k->fontsize) - descender_reduce_px;
	k->tb.buttonstep		= k->tb.r_button.height + k->tb.cellspacing_i;
	
	rect_s r_box = { k->cm.relative_rect.left, k->cm.relative_rect.top, 
					k->tb.r_button.width, 
					k->tb.r_button.height };

	if (k->cm.relative_rect.width > r_box.width)
		r_box.width = k->cm.relative_rect.width; // Are you sure?

	// Compute bottom
	for (int n = 0; n < k->list_strings_a.numstrings - 1; n ++, r_box.top += k->tb.buttonstep)
		; // Nada

	k->tb.bottomof_last = k->tb.buttonstep;

	RECT_SET (k->txta.r_textarea, 
		k->cm.relative_rect.left + k->tb.cellpaddingx_i, 
		k->cm.relative_rect.top + k->tb.cellpaddingy_i, 
		k->tb.stringwidthmaxpx, 
		ceil(k->fontsize)
	);

autosize_go:
	if (k->autosize) {
		k->cm.relative_rect.width = r_box.width;
		k->cm.relative_rect.height = RECT_BOTTOMOF(r_box) - k->cm.relative_rect.top;
	}

	k->viewcalc.rowheight = k->tb.buttonstep; // ceil(fontsize);
	viewcalc_s *vc = ViewCalc_Set (&k->viewcalc, &k->r_interior, k->r_scrollport.top, listcount);

	return k;
}

// Define early.  Do we have fontsize yet?
// Early is asking for size before the loop calculates the clip area.
// If we are early, we actually run twice.
// Do we get added to drawlist twice?  No.  The loop does that once per object.

oject_s *O_TabSelect_Refresh_Plus_Early (oject_s *k)
{
	O_TabSelect_Refresh (k);
	return k;
}



oject_s *O_TabSelect_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	//K_GET_RELATIVE_XY (x,y);

	if (!isdown)
		return NULL;

	rect_s	r_button	= k->tb.r_button;
	int hitidx = Rect_Hit_Boxes_No_Miss(&k->tb.r_button, x, y, k->tb.buttonstep, k->list_strings_a.numstrings);

#ifdef _DEBUG
	if (hitidx == not_found_neg1) {
		Con_PrintLinef ("Void hit");
	}

	if (hitidx == k->selectedindex) {
		Con_PrintLinef ("Already selected");
	}
#endif

	if (isin2 (hitidx, not_found_neg1, k->selectedindex) == false) {
		k->selectedindex = hitidx; Object_Event_OnChange_Recursive (k, shallrecurse_false);
		// dirty? changeevent
	}

	return k;
}

oject_s *O_TabSelect_KeyDown (oject_s *k, int key, int ascii)
{
	return O_ListBox_KeyDown (k, key, ascii);
}


