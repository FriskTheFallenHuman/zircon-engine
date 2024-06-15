// oject_bbx_contextmenu.c.h

// Context menu / 3  (String, StringValue, Function)

WARP_X_ (Form_ContextMenu_Spawn Form_Mouse2Down)
oject_s *O_Context_Menu_Exec(oject_s *k)
{
	int listidx = k->selectedindex * CM_SERIES_COUNT_4;

	//char *s_show	= k->list_strings_a.strings[listidx + CM_0_TEXT_0];
	//char *s_checked	= k->list_strings_a.strings[listidx + CM_1_CHECKED_1];
	char *s_str		= k->list_strings_a.strings[listidx + CM_2_STRING_ID_2];
	char *s_fn		= k->list_strings_a.strings[listidx + CM_3_FUNCSTRING_3];

	WARP_X_ (Form_ContextMenu_Spawn)

	zexeccmd (k->fctrl, k, listidx, s_fn, s_str);
	
	return k;
}




WARP_X_ (Lister_ListCount)

oject_s *O_ContextMenu_Spawn (oject_s *k) { return Lister_Spawn(k); }

WARP_X_ (Form_ContextMenu_Spawn Form_Mouse2Down)
oject_s *O_ContextMenu_Draw (oject_s *k)
{
	Draw_Rect	(&k->r_interior, k->backcolor, alpha_1_0);

	rect_s	r_button	= k->tb.r_button;
	rect_s	r_text		= k->txta.r_textarea;

	Draw_Rect	(&k->tb.r_leftbar, color3_gray_75, alpha_0_5);

	for (int nn = 0; nn < k->list_strings_a.numstrings; nn += CM_SERIES_COUNT_4, r_button.top += k->tb.buttonstep, r_text.top += k->tb.buttonstep) {
		const char *s = k->list_strings_a.strings[nn];
		const char *sc = k->list_strings_a.strings[nn + CM_1_CHECKED_1];
		int ischecked = sc[0] == '1';
		int		trueidx			= nn / CM_SERIES_COUNT_4;
		int		is_selected		= (trueidx == k->selectedindex);
		vec_t	*backcolor		= is_selected ? k->backcolorselected : k->backcolor;
		vec_t	*forecolor		= is_selected ? k->forecolorselected : k->forecolor;

		if (k->is_rounded)	Draw_RectRound	(&r_button, backcolor, alpha_1_0);
		else				Draw_Rect		(&r_button, backcolor, alpha_1_0);

		if (ischecked) {
			rect_s r = { k->tb.r_leftbar.left, r_button.top, k->tb.r_leftbar.width, r_button.height };
			Draw_Rect		(&r, color3_blue, alpha_1_0);
		}
		Draw_String	(&r_text, s, k->fontsize, forecolor, alpha_1_0);
	} // for

	
	return k;
}







WARP_X_ (O_TabSelect_Refresh_Plus_Early)
oject_s *O_ContextMenu_Refresh_Plus_Early (oject_s *k)
{
	k->tb.stringwidthmaxpx	= stringlist_maxwidthint(&k->list_strings_a, zdev_dpfont, k->fontsize);

	// Assume infinite size on finite canvas
	float	descender_pct			= zdev_dpfont->ft_baker_descend_pct;
	float	descender_reduce_pct	= 1 - bound(0,k->fontdescendpct,1);
	int		descender_reduce_px		= ceil(descender_pct * descender_reduce_pct * k->fontsize);
	
	k->tb.cellpaddingx_i	= ceil(k->cellpaddingxpct * k->fontsize);
	k->tb.cellpaddingy_i	= ceil(k->cellpaddingypct * k->fontsize);
	k->tb.cellspacing_i		= ceil(k->cellspacingypct * k->fontsize);

	int left_extra = ceil(k->fontsize);

	k->tb.r_button.left		= k->cm.relative_rect.left;
	k->tb.r_button.top		= k->cm.relative_rect.top;
	k->tb.r_button.width	= (k->tb.cellpaddingx_i * 2) + left_extra + k->tb.stringwidthmaxpx;

	if (k->cm.relative_rect.width > k->tb.r_button.width) {
		k->tb.r_button.width = k->cm.relative_rect.width; // Are you sure?
	}

	k->tb.r_button.height	= (k->tb.cellpaddingy_i * 2) + ceil(k->fontsize) - descender_reduce_px;
	k->tb.buttonstep		= k->tb.r_button.height + k->tb.cellspacing_i;
	
	rect_s r_box = k->tb.r_button;

	// Compute bottom - due to cellspacing stepping is clearest

	int numrows = Lister_ListCount(k);
	for (int nn = 1; nn < numrows; nn ++, r_box.top += k->tb.buttonstep)
		; // Nada

	k->tb.r_leftbar = k->tb.r_button;
	k->tb.r_leftbar.width = left_extra;
	k->tb.r_leftbar.height = RECT_BOTTOMOF(r_box) - k->tb.r_button.top;

	RECT_SET (k->txta.r_textarea, 
		k->cm.relative_rect.left + left_extra + k->tb.cellpaddingx_i, 
		k->cm.relative_rect.top + k->tb.cellpaddingy_i, 
		k->tb.stringwidthmaxpx, 
		ceil(k->fontsize)
	);

autosize_go:
	k->cm.relative_rect.width = r_box.width;
	k->cm.relative_rect.height = RECT_BOTTOMOF(r_box) - k->cm.relative_rect.top;
	return k;
}

static double _lastime;
oject_s *O_ContextMenu_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	if (isdown == false)
		return NULL;

	double clickdelta = host.realtime - _lastime;

	int is_doubleclick =  (_lastime && clickdelta < DOUBLE_CLICK_0_5);
	
	_lastime = host.realtime;

	int newidx = Rect_Hit_Boxes_No_Miss (&k->tb.r_button, x, y, k->tb.buttonstep, k->list_strings_a.numstrings);

	if (newidx == not_found_neg1)
		return k;

	if (newidx != k->selectedindex) {
		k->selectedindex = newidx; Object_Event_OnChange_Recursive (k, shallrecurse_false);
		//k->ui.mouse_down_time = host.realtime;
		//k->ui.mouse_down_thingi = 
//		Lister_VisRangeCheck_Refresh (k, /*movecount*/ 0);
	}

	if (is_doubleclick) {
		Oject_Focus_Set (k->fctrl->frm.kcontextoldfocus);
		k->fctrl->frm.kcontextoldfocus = NULL;
		if (k->selectedindex != not_found_neg1)
			O_Context_Menu_Exec (k);
	}

	return k;

}


// Return NULL if input not processed.
oject_s *O_ContextMenu_KeyDown (oject_s *k, int key, int ascii)
{
	oject_s *kreply = Lister_KeyDown(k,key,ascii);

	if (kreply) return kreply;
	
	// NOT HANDLED
	switch (key) {
	case K_ENTER:	
		Oject_Focus_Set (k->fctrl->frm.kcontextoldfocus);
		k->fctrl->frm.kcontextoldfocus = NULL;
		if (k->selectedindex != not_found_neg1)
			O_Context_Menu_Exec (k);
		break;

	case K_ESCAPE:	
		Oject_Focus_Set (k->fctrl->frm.kcontextoldfocus);
		k->fctrl->frm.kcontextoldfocus = NULL;
		return k;

	default:		return NULL; // NO ACTION
	} // sw

	return NULL;
}


