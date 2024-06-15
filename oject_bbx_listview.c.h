// oject_bbx_listview.c.h

viewcalc_s *ViewCalcColumns_Set (oject_s *k, viewcalc_s *vc, const rect_s *pr_interior);

WARP_X_ (O_ListView_ColumnDrag)
oject_s *O_ListView_Spawn (oject_s *k)
{
	return Lister_Spawn(k); // Does listview specific things too.
}


// Draw enters with clip set to screen
oject_s *O_ListView_Draw (oject_s *k)
{
	if (!k->columncount) {
		Draw_Rect	(&k->r_screen, k->backcolor, alpha_1_0);
		return NULL;
	}

	int listcount = Lister_ListCount(k);
	if (!listcount) return NULL;

	Draw_Rect	(&k->r_screen, color3_gray_50, alpha_1_0);
	Draw_Rect	(&k->xl.r_header, color3_gray_75, alpha_1_0); // Header button backcolor
	Draw_Rect	(&k->r_interior, k->backcolor, alpha_1_0);

	// Draw each column
	rect_s r_text, r_textarea;
	r_text.height = ceil(k->fontsize);

	// There is a clipzone for each column
	viewcalc_s *vcx = &k->viewcalcx;
	rect_s r_cursor = k->xl.r_header;
	r_cursor.left -= k->r_scrollport.left; // Scroll

	// We draw 1 column too many for a phantom column if it is needed to fill the interior
	for (int col = 0; col <= k->columncount; col ++, r_cursor.left += r_cursor.width) {
		r_cursor.top = k->r_screen.top; // Reset to top
		r_cursor.height = k->xl.headerrowheight;
		r_cursor.width	= colwidthx (k,col);

		if (RECT_RIGHTOF(r_cursor) <= k->xl.r_header.left) continue;
		if (RECT_RIGHTOF(k->xl.r_header) <= r_cursor.left) continue; // Not break due to phantom

//phantom:
		// PHANTOM HEADER - extra column
		int is_phantom_now = col >= k->columncount && k->xl.is_phantom;
		if (col == k->xl.beyondcol) {
			if (k->xl.is_phantom) {
				r_cursor.width = k->xl.phantom_width;
				INCREASE___ Draw_Clip_Set_Rect (&k->xl.r_header_clipped); // Object_Draw sets to r_screen
				DrawKit_Button_Except_Face (&r_cursor, drawbut_command_up_4);
			}
			break; // End phantom
		}

//header:
		// Column header
		ccs *s_header = colheaderx(k, col);
		rect_s r_column, r_column_clipped;
		Rect_Calc_Clipped_Unclipped_From_XYWH (
			SET___ &r_column,
			SET___ &r_column_clipped,
			&k->xl.r_header_clipped,
			r_cursor.left,
			r_cursor.top,
			r_cursor.width,
			k->xl.r_header.height
		);

		INCREASE___ Draw_Clip_Set_Rect (&r_column_clipped);

		DrawKit_Button_Except_Face (&r_cursor, drawbut_command_up_4);
		r_text.width = Draw_StringWidthInt (zdev_dpfont, s_header, k->fontsize);

		RECT_SET_ADJUST (r_textarea, r_cursor, +LV_TEXTMARGIN_10, +LV_TEXTMARGIN_10, -LV_TEXTMARGIN_10*2, -LV_TEXTMARGIN_10*2); // Margin 3
		Label_Rect_Edit_Align (&r_text, &r_textarea, alignment_middle_left_3);
		Draw_String (&r_text, s_header, k->fontsize, color3_black, alpha_1_0);

	} // for

	rect_s r_selected = k->r_interior;
	r_selected.top = k->r_interior.top - k->r_scrollport.top  + k->selectedindex * k->viewcalc.rowheight;
	r_selected.height =k->viewcalc.rowheight;

draw_selection:
	if (1) {
		INCREASE___ Draw_Clip_Set_Rect (&k->r_interior_clipped); // Object_Draw sets to r_screen
		float thisalpha = Oject_Have_Focus (k) ? alpha_1_0 : alpha_0_25;
		Draw_Rect (&r_selected, k->backcolorselected, thisalpha);
	}

	r_cursor = k->r_interior;
	r_cursor.left -= k->r_scrollport.left; // Subtract out the positive scroll.

draw_text:
	for (int col = 0 ; col < k->columncount; col ++, r_cursor.left += r_cursor.width) {
		r_cursor.height = k->viewcalc.rowheight;
		r_cursor.top = k->r_interior.top;
		r_cursor.width	= colwidthx (k,col);

		if (RECT_RIGHTOF(r_cursor) <= k->r_interior.left) continue;
		if (RECT_RIGHTOF(k->r_interior) <= r_cursor.left) break;

		rect_s r_column, r_column_clipped;
		Rect_Calc_Clipped_Unclipped_From_XYWH (
			SET___ &r_column,
			SET___ &r_column_clipped,
			&k->r_interior_clipped,
			r_cursor.left,
			r_cursor.top,
			r_cursor.width - LV_TEXTMARGIN_1, // This is less Y scrollbar and text margin
			k->r_interior.height // This is less X scrollbar
		);

		INCREASE___ Draw_Clip_Set_Rect (&r_column_clipped);

		for (int row = k->viewcalc.first_vis_row; row <= k->viewcalc.last_partial_row; row ++) {
			int		is_selected	= (row == k->selectedindex);
			vec_t	*forecolor	= is_selected ? k->forecolorselected :k->forecolor;
			int		listidx		= rowcol_2idx(k,row, col);
			char	*s			= k->list_strings_a.numstrings > listidx ? k->list_strings_a.strings[listidx] : NULL;

			if (s) {
				Draw_String_XY (r_cursor.left + LV_TEXTMARGIN_10, r_cursor.top, s, k->fontsize, forecolor, alpha_1_0);
			}
			r_cursor.top += r_cursor.height;
		}
	} // for

	return k;
}

WARP_X_ (Lister_Spawn ViewCalc_Set O_ListBox_Draw Object_Draw_Recursive_Not_Form)

oject_s *O_ListView_Finalize (oject_s *k)
{
	// NO COLUMN WIDTHS?  TIME TO DEFAULT THEM.
	if (k->columnwidths_a.count == 0 && k->columncount) {
		for (int col = 0; col < k->columncount; col ++) {
			int32s_add1 (&k->columnwidths_a, LV_DEFAULT_WIDTH_100);
		} // for
	} //

	for (int col = 0; col < k->columncount; col ++) {
		vclamp_lo (k->columnwidths_a.ints[col], LV_MIN_WIDTH_25);
	} // for

	return k;
}

WARP_X_ (Lister_SetPctHorz)
oject_s *O_ListView_Refresh (oject_s *k)
{
	// Adjust interior.
	k->xl.headerrowheight = ceil(k->fontsize) + LV_HEADERROW_ADD_6;

	RECT_SET (k->r_interior, k->r_screen.left, k->r_screen.top + k->xl.headerrowheight,
		k->r_screen.width, k->r_screen.height - k->xl.headerrowheight);

	int calc_again_xscroll_showed = false;

xscroll_appeared_recalc_yscroll:
	Lister_Refresh (k);

	if (!k->columncount) return NULL;
	int listcount = Lister_ListCount(k);
	if (!listcount) return NULL;

	k->xl.startcol		= 0;
	k->xl.beyondcol		= k->columncount;

	// We always must recalc this, even if we are on 2nd recalc
	if (1) {
		viewcalc_s *vcx = ViewCalcColumns_Set (k, &k->viewcalcx, &k->r_interior);

		k->r_scrollport.width		= k->r_interior.width;
		k->r_scrollport.height		= k->viewcalc.rowheight * k->list_strings_a.numstrings;

		k->servo.kscrollx->is_hidden = (vcx->show_scrollbar == false);

		if (vcx->show_scrollbar) {
			rect_s r_scroll = k->r_screen; // This is after y scroll.
			r_scroll.left	= k->r_screen.left;
			r_scroll.width	= k->r_screen.width - k->viewcalc.show_scrollbar * KNOB_SIZE_16;
			r_scroll.top	= RECT_BOTTOMOF(k->r_screen) - KNOB_SIZE_16;
			r_scroll.height = KNOB_SIZE_16;

			WARP_X_ (Lister_SetPctHorz)

			// PERFORM ADJUSTMENTS
			k->servo.kscrollx->cm.relative_rect = r_scroll;

			k->r_interior.height -= KNOB_SIZE_16;

			// Y SCROLL WILL ALWAYS NEED TO RECALC IF WE DID THIS
			if (calc_again_xscroll_showed == false) {
				// Restore interior width so yscroll can calc position/cutaway again ...
				if (k->viewcalc.show_scrollbar)
					k->r_interior.width += KNOB_SIZE_16; // Undo Y scrollbar area
				calc_again_xscroll_showed = true;
				goto xscroll_appeared_recalc_yscroll;
			}
		}
	} // calc xscroll

	// Calculate header interior
	Rect_Calc_Clipped_Unclipped_From_XYWH (
		SET___ &k->xl.r_header,
		SET___ &k->xl.r_header_clipped,
		&k->r_screen_clipped,
		k->r_screen.left,
		k->r_screen.top,
		k->r_screen.width - k->viewcalc.show_scrollbar * KNOB_SIZE_16,  // Less Y scrollbar
		k->xl.headerrowheight
	);

	// Calculate interior
	// Calculate header interior
	Rect_Calc_Clipped_Unclipped_From_XYWH (
		SET___ &k->r_interior,
		SET___ &k->r_interior_clipped,
		&k->r_screen_clipped,
		k->r_screen.left,
		k->r_screen.top + k->xl.r_header.height,
		k->r_screen.width - k->viewcalc.show_scrollbar * KNOB_SIZE_16,  // Less Y scrollbar
		k->r_screen.height - k->viewcalcx.show_scrollbar * KNOB_SIZE_16 - k->xl.r_header.height	// Less X scrollbar
	);

	// Dead scrollbars mean scroll is 0.
	if (k->viewcalc.show_scrollbar == false && k->r_scrollport.top != 0)
		k->r_scrollport.top = 0;

	if (k->viewcalcx.show_scrollbar == false && k->r_scrollport.left != 0)
		k->r_scrollport.left = 0;

	if (k->ui.mouse_down_thingi >= LV_DRAG_COLSIZE_10) {
		WARP_X_ (O_ListView_ColumnDrag)
	}

	//Vid_SetWindowTitlef ("x scroll %d .. all cols %d r_data %d", k->r_scrollport.left, k->xl.columnsallwidths, k->xl.r_data.width);

	return k;
}


oject_s *O_ListView_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	if (isdown == false && k->ui.mouse_down_thingi >= LV_DRAG_COLSIZE_10) {
		return O_ListView_ColumnDrag(k,x,y, /*isdone?*/ true);
	}

	if (isdown && k->list_strings_a.numstrings && k->columncount
		&& RECT_HIT (k->xl.r_header, x, y) ) {
			// Store the origin
			int column = Hit_Sizer_Return_Column(k,x,y);
			if (column == not_found_neg1) {
				return NULL;
			}

			k->ui.mouse_down_time = host.realtime;
			k->ui.mouse_down_origin.x = x;
			k->ui.mouse_down_origin.y = y;
			k->ui.mouse_down_thingi = LV_DRAG_COLSIZE_10 + column;
			k->ui.mouse_down_valuei = colwidthx (k, column);
			k->ui.is_move_thresh_met = true;
			k->ui.mousepointer = mousepointer_baker_column_20;
			//Vid_SetWindowTitlef ("Listview mouse down sizing %d", k->ui.mouse_down_valuei);
			//Con_PrintLinef ("Listview mouse down sizing %d", k->ui.mouse_down_valuei);
			return k;
			//Con_PrintLinef ("Mousedown at %d", k->mousedown_thumb_at_mousedown_px.y);
	}


	if (Lister_MouseButtonAction (k, x, y, isdown)) // This acts as listbox row check (interior collide)
		return k;

	return NULL;
}


// Return here is important. NULL means mousepointer returns to default.
oject_s *O_ListView_MouseMove (oject_s *k, int x, int y)
{
	if (k->fctrl->frm.kmousedown == k) {
		if (k->ui.mouse_down_thingi >= LV_DRAG_COLSIZE_10) {
		return O_ListView_ColumnDrag (k, x, y, /*isdone?*/ false);
		//return O_ListView_ColumnDrag (k,x,y);
		}
	}

	// See if we hit a column header hotspot.
	if (!k->columncount) return NULL;
	int listcount = Lister_ListCount(k);
	if (!listcount) return NULL;

	if (RECT_HIT (k->xl.r_header, x, y) == false)
		return NULL;

	int ret = Hit_Sizer_Return_Column(k,x,y);
	if (ret >=0) {
		k->ui.mousepointer = mousepointer_baker_column_20;
		return k;
	}

	return NULL;
}



// Return NULL if input not processed.
WARP_X_ (ZForm_MouseMove)
oject_s *O_ListView_KeyDown (oject_s *k, int key, int ascii)
{
	return Lister_KeyDown(k,key,ascii);
}











