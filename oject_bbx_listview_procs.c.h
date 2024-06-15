// oject_bbx_listview_procs.c.h

WARP_X_CALLERS_ (Form_MouseButtonAction, Lister_Spawn)
// If mouse down is set AFTER.
oject_s *O_ListView_ColumnDrag (oject_s *k, int x, int y, int isdone)
{
	// Do we get silly unrelated mouseups?  I bet we do
	float	delta_x		= x - k->ui.mouse_down_origin.x;
	int		oldwidth	= k->ui.mouse_down_valuei;
	int		newwidth	=  oldwidth + delta_x;
	int		col			= k->ui.mouse_down_thingi - LV_DRAG_COLSIZE_10;

	vclamp_lo (newwidth, LV_MIN_WIDTH_25);

	// We need the width it will go to.
	k->columnwidths_a.ints[col] = newwidth;

	O_ListView_Refresh (k);
	O_ScrollBar_Refresh (k->servo.kscrollx);
	O_ScrollBar_Refresh (k->servo.kscrolly);

	oject_s *kline = k->servo.kcolumnresize_line;
	
	int columnright = columnleftscreenx(k, col) + /*newwidth */k->columnwidths_a.ints[col];
	RECT_SET (
		kline->cm.relative_rect,				// Refresh sets r_screen to relative rect
		columnright /*x*/, // Needs locked to column position
		k->r_interior.top,
		2,
		k->r_interior.height
	);
	O_Rectangle_Refresh (k->servo.kcolumnresize_line);
	k->servo.kcolumnresize_line->is_hidden = isdone ? true : false;
	// Q: Why are we using form refresh here?
	// A: We changed scrollbar visibility, must recalc drawn list.
	Form_QueueRefresh (k->fctrl); // This is the only form refresh available right now!

	//Vid_SetWindowTitlef ("Listview mouse up sizing delta %d from %d to %d",
	//	delta_x,
	//	oldwidth,
	//	newwidth
	//);
	//Con_PrintLinef ("Listview mouse up sizing delta %d from %d to %d",
	//	delta_x,
	//	oldwidth,
	//	newwidth
	//);
	if (isdone) {
		k->ui.mousepointer = mousepointer_arrow_default_0;
	}

	return NULL;
}

WARP_X_CALLERS_ (Property_Set_Float)
void O_ListView_SetPct (oject_s *k, float pct, oject_s *ksb)
{
	Lister_SetPct (k, pct, ksb);
}



// -1 if not found.  Otherwise the column being sized.
int Hit_Sizer_Return_Column (oject_s *k, int x, int y)
{
	viewcalc_s *vcx = &k->viewcalcx;
	rect_s r_cursor = k->xl.r_header;
	r_cursor.left -= k->r_scrollport.left;

	for (int col = 0 ; col < k->columncount; col ++, r_cursor.left += r_cursor.width) {
		r_cursor.top = k->xl.r_header.top; // Reset to top
		r_cursor.height = k->xl.headerrowheight;
		r_cursor.width	= colwidthx (k,col);

		if (RECT_RIGHTOF(r_cursor) <= k->xl.r_header.left)		continue;
		if (RECT_RIGHTOF(k->xl.r_header) <= r_cursor.left)		break; // I guess?

		// First 4 and last 4 of a column
		rect_s r_hot = r_cursor;
		r_hot.width = 4;
		if (RECT_HIT(r_hot,x,y))
			return col - 1;
		
		r_hot.left = RECT_RIGHTOF(r_cursor) - 4;
		
		if (RECT_HIT(r_hot,x,y))
			return col;
	} // for

	return not_found_neg1;
}


// pr_canvas is interior typically, right?
WARP_X_ (O_ListView_Refresh)
viewcalc_s *ViewCalcColumns_Set (oject_s *k, viewcalc_s *vc, const rect_s *pr_interior)
{
	memset (vc, 0, sizeof(*vc));

	if (k->columncount == 0)
		return vc;

	// First visible column
	//int intox = k->r_scrollport.left;
	k->xl.columnsallwidths = colwidths(k, &k->xl.is_phantom, &k->xl.phantom_width);

	if (k->xl.columnsallwidths <= k->r_interior.width) {
		vc->show_scrollbar = false;
		vc->firstcolumn = 0;
		vc->lastcolumn = k->columncount - 1;
		vc->leftdrawoffset = 0;
		return vc;
	}

	int cume = -k->r_scrollport.left;
	vc->firstcolumn = not_found_neg1;
	vc->lastcolumn = not_found_neg1;
	//int first_col = not_found_neg1;
	int vleft = 0;//-k->r_scrollport.left;
	int vbeyond = vleft + k->r_interior.width;
	int cleft;
	int cbeyond;
	int w;
	int col;
	for (col = 0; col < k->columncount; col ++) { // NOTE < not <= no phantom here
		w = colwidthx(k, col);
		cleft = cume;
		cbeyond = cume + w;
		int is_seen_left = in_range_beyond (vleft, cleft, vbeyond);
		int is_seen_right = in_range_beyond (vleft, cbeyond, vbeyond);
		int is_seen = is_seen_left || is_seen_right;
		int is_last = is_seen && cbeyond >= vbeyond; 
		if (vc->firstcolumn == not_found_neg1 && is_seen) {
			vc->firstcolumn = col;
			vc->leftdrawoffset =  - k->r_scrollport.left;
		}
		if (vc->lastcolumn == not_found_neg1 && is_last) {
			vc->lastcolumn = col;
			break;
		}
		cume = cbeyond;
	} // for

	if (vc->lastcolumn == not_found_neg1)
		vc->lastcolumn = k->columncount - 1;
	vc->pageamount = 0;// ?

	vc->show_scrollbar = true;

	return vc;
}

