// oject_bbx_textbox_mouse.c.h

WARP_X_ (Form_MouseButtonAction)

void TextBox_MouseSelection_Try_Extend (oject_s *k, int x, int y)
{
	if (k->text_a[0] == NULL_CHAR_0)
		return;

	int net_x		= x - (k->txta.r_textarea.left - k->r_scrollport.left);
	vclamp_lo (net_x, LENGTH_0); // Draw_String acts weird with negative draw width
	int charnum		= Draw_StringIntoCountAt(zdev_dpfont, k->text_a, k->fontsize, net_x, roundtype_0_50_0);
	int newsellen	= charnum - k->txta.cursor_pos;
	
	if (newsellen == k->txta.cursor_length)
		return; // Same length

	TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newsellen);
}

void TextBox_MouseSelection_Try_Set (oject_s *k, int x, int y)
{
	if (k->text_a[0] == NULL_CHAR_0)
		return;

	int net_x		= x - (k->txta.r_textarea.left - k->r_scrollport.left);
	vclamp_lo (net_x, LENGTH_0); // Draw_String acts weird with negative draw width
	int charnum		= Draw_StringIntoCountAt(zdev_dpfont, k->text_a, k->fontsize, net_x, roundtype_0_50_0);
	
	if (charnum == k->txta.cursor_pos && k->txta.cursor_length == 0)
		return; // Same cursorpos and same no selection we would set.

	TextBox_Cursor_Set_Refresh (k, charnum, LENGTH_0);
}

WARP_X_ (O_ListBox_MouseButtonAction Form_Mouse_Move)
oject_s *O_TextBox_MouseMove (oject_s *k, int x, int y)
{	
	// Baker: If we are down, don't test for rect collision in r_interior / r_textarea
	// to allow more leeway with mouse collision.
	int isshifted = KM_SHIFT;

	//if (RECT_HIT(k->txta.r_textarea, x,y) == false) return k;

	k->ui.mousepointer = mousepointer_text_beam_1;
	
	int isdown = (k->fctrl->frm.kmousedown == k);

	if (isdown)
		TextBox_MouseSelection_Try_Extend (k, x, y);

	
	//int isshifted = KM_SHIFT;
	//if (isdown) {
	//	if (RECT_HIT(k->txta.r_textarea, x,y)) {
	//		int net_x = x - (k->txta.r_textarea.left - k->r_scrollport.left);
	//		if (shiftonly) {
	//		default:			Class_TextBox_MouseMove_Selection_Try_Extend_ (k, x, y);  // SHIFTED: Extend selection. No round
	//		case_break false:	Class_TextBox_MouseMove_Selection_Try_Cursor_Set (k, x, y);
	//	}
	//} // isdown
	return k;
}

oject_s *O_TextBox_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	if (!isdown) {
		int wasdown = (k->fctrl->frm.kmousedown == k);
		if (wasdown)
			TextBox_MouseSelection_Try_Extend (k, x, y);
	
		return k;
	}
	
	int isshifted = KM_SHIFT;
	// r_textarea is too strict, use r_interior.
	if (RECT_HIT(k->r_interior, x,y) == false) return k;

	if (isshifted) TextBox_MouseSelection_Try_Extend (k, x, y);
	else		   TextBox_MouseSelection_Try_Set (k, x, y);
	
	return k;
}






