// oject_bbx_textbox_procs.c.h

int TextBox_Recalc_Translation_Did_Move (oject_s *k);

void TextBox_Cursor_Recalc (oject_s *k)
{
	k->txta._cursor_length_abs = abs(k->txta.cursor_length);

	// DON'T READ THIS, KNOW THAT IT IS RIGHT AND MOVE ALONG ...
	k->txta._cursor_start  = k->txta.cursor_pos;
	k->txta._cursor_beyond = k->txta.cursor_pos + k->txta.cursor_length;
	if (k->txta._cursor_start > k->txta._cursor_beyond)
		swapi (&k->txta._cursor_start, &k->txta._cursor_beyond);

	// CALCULATE HIGHLIGHT TEXT
	//  TEXT BEFORE HIGHLIGHTED TEXT
	//  TEXT AFTER HIGHLIGHTED TEXT

	char *sleft = &k->text_a[k->txta._cursor_start];
	int ncount = k->txta._cursor_length_abs;
	k->txta._sel_left_px = Draw_StringNWidth (zdev_dpfont, k->txta._cursor_start, k->text_a, k->fontsize);
	k->txta._sel_len_px = Draw_StringNWidth (zdev_dpfont, ncount, sleft, k->fontsize);
	k->txta._sel_real_px = k->txta._sel_left_px;
	if (k->txta.cursor_length > 0) // positive means cursor at end
		k->txta._sel_real_px += (k->txta._sel_len_px - TXT_CURSOR_WIDTH_4);

	// RECALC VIEWPORTAGE
	TextBox_Recalc_Translation_Did_Move (k); // No action required it is just scroll?

	int thinline_cursorpos_px = k->txta._sel_left_px;
	if (k->txta.cursor_length > 0)
		thinline_cursorpos_px += (k->txta._sel_len_px - TXT_CURSOR_WIDTH_4);

	RECT_SET (
		k->txta.r_cursor,
		k->txta.r_textarea.left + thinline_cursorpos_px,
		k->txta.r_textarea.top,
		TXT_CURSOR_WIDTH_4,
		k->fontsize
	);

	RECT_SET (
		k->txta.r_cursor_highlight,
		k->txta.r_textarea.left + k->txta._sel_left_px,
		k->txta.r_textarea.top,
		k->txta._sel_len_px,
		k->fontsize
	);

	Z_StrDup_Len_Z_Realloc (&k->txta.seg1_a, &k->text_a[0], k->txta._cursor_start);
	k->txta.seg1_left_px = k->txta.r_textarea.left + 0;

	Z_StrDup_Len_Z_Realloc (&k->txta.seg2_a, &k->text_a[k->txta._cursor_start], k->txta._cursor_length_abs);
	k->txta.seg2_left_px = k->txta.r_textarea.left + k->txta._sel_left_px;

	Z_StrDup_Realloc_ (k->txta.seg3_a, &k->text_a[k->txta._cursor_beyond]);
	k->txta.seg3_left_px = k->txta.r_textarea.left + k->txta._sel_left_px + k->txta._sel_len_px;
}

int TextBox_Recalc_Translation_Did_Move (oject_s *k)
{
	int ret = false;
	k->txta.stringwidthi_px = Draw_StringWidthInt (zdev_dpfont, k->text_a, k->fontsize);
	k->r_scrollport.width	= k->txta.stringwidthi_px + TXT_CURSOR_WIDTH_4;
	k->txta.is_viewported_x = k->txta.r_textarea.width < k->r_scrollport.width;

	// If we do not have focus or are not viewported, there is no scroll
	if (k->txta.is_viewported_x == false || Oject_Have_Focus(k) == false) {
		k->r_scrollport.left = 0;
		return false;
	}

	int step_amount_0_25_w = (int)(k->txta.r_textarea.width * TXT_SCROLL_HORZ_AMOUNT_0_25);

	// CONCERN:  Characters wider than the textarea.
	if (k->txta._sel_real_px - k->r_scrollport.left < 0) {
		// Too much scroll for cursor to be visible.  Avoid "leftspace"
		ret = true;
		do {
			k->r_scrollport.left -= step_amount_0_25_w; // REDUCE SCROLL step amt
			vclamp_lo (k->r_scrollport.left, 0);
		} while (k->txta._sel_real_px - k->r_scrollport.left < 0);
	}

	else if (k->txta._sel_real_px - k->r_scrollport.left >= k->txta.r_textarea.width) {
		// INCREASE SCROLL.  Pull right into view.
		ret = true;
		do {
			k->r_scrollport.left += step_amount_0_25_w; // INCREASE SCROLL step amt
			if (k->r_scrollport.left >= k->r_scrollport.width - k->txta.r_textarea.width) {
				k->r_scrollport.left = k->r_scrollport.width - k->txta.r_textarea.width; // Less headache inducing I guess. 0 - 199  100 to 199  200 - 100.  100 is ok max scroll
				break;
			}
		} while (k->txta._sel_real_px - k->r_scrollport.left >= k->txta.r_textarea.width);
	}

	return ret;
}



void TextBox_Copy_Selection (oject_s *k)
{
	char *s_z = Z_StrDup_Len_Z(&k->text_a[TXT_SELLEFT(k)], TXT_SELCOUNTABS(k));
	Clipboard_Set_Text (s_z);
	Z_FreeNull_ (s_z);
}

// Immediate cursor draw (force_draw_cursor) set in keypress, not here
void TextBox_Cursor_Set_Refresh (oject_s *k, int start, int length)
{
	if (start == k->txta.cursor_pos && k->txta.cursor_length == length)
		goto must_refresh_exit; // Nothing changed.  We are required to refresh.

	k->txta.cursor_pos		= start;
	k->txta.cursor_length	= length;

//	if (false == in_range_beyond (0, k->txta.cursor_pos, (TXT_STRLEN(k) + 1) )) {
//		int j = 5;
//	}

//	if (false == in_range_beyond (0, TXT_CURSOR_AT(k), (TXT_STRLEN(k) + 1))) {
//		int j = 5;
//	}

must_refresh_exit:
	O_TextBox_Refresh (k);
}

void TextBox_Delete_Selection_Refresh (oject_s *k)
{
	if (k->txta.cursor_length) {
		TextBox_Edit_Delete_At_NumChars (k, TXT_SELLEFT(k), TXT_SELCOUNTABS(k));
		TextBox_Cursor_Set_Refresh (k, TXT_SELLEFT(k), LENGTH_0);
	}
}


