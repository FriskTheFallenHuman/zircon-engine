// oject_bbx_listbox.c.h

// It is ok for the selected idx to fall out of range if the scrollbar did it.

// 
// 0  =====   -8  BOB     8 =======
//    BOB		  ======    MARY
//    MARY        MARY      TODD

oject_s *O_ListBox_Spawn (oject_s *k)
{

	return Lister_Spawn(k);
}

WARP_X_CALLERS_ (Property_Set_Float)
void O_ListBox_SetPct (oject_s *k, float pct, oject_s *ksb)
{
	Lister_SetPct (k, pct, ksb);
}

WARP_X_ (ViewCalc_Set)
oject_s *O_ListBox_Draw (oject_s *k)
{
	Draw_Rect	(&k->r_interior, k->backcolor, alpha_1_0);
	
	rect_s r_cursor = k->viewcalc.r_row_first;	// updated in refresh
	int start_idx	= k->viewcalc.first_vis_row;
	int count		= k->viewcalc.rowcount;
	int beyond_idx	= start_idx + count;

	for (int n = start_idx; n < beyond_idx; n ++, r_cursor.top += k->viewcalc.rowheight) {
		const char *s = k->list_strings_a.strings[n];
		
		int		is_selected		= (n == k->selectedindex);
		vec_t	*forecolor		= is_selected ? k->forecolorselected : k->forecolor;

		if (is_selected)
			Draw_Rect	(&r_cursor, k->backcolorselected, alpha_1_0);
		Draw_String	(&r_cursor, s, k->fontsize, forecolor, alpha_1_0);
	} // for

	return k;
}



// Inheritance from form is important -- fontsize, etc.

oject_s *O_ListBox_Refresh (oject_s *k)
{
	return Lister_Refresh (k);
}


oject_s *O_ListBox_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	return Lister_MouseButtonAction (k, x, y, isdown);
}


// Return NULL if input not processed.
oject_s *O_ListBox_KeyDown (oject_s *k, int key, int ascii)
{
	return Lister_KeyDown(k,key,ascii);
}

