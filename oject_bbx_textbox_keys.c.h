// oject_bbx_textbox_keys.c.h

oject_s *O_TextBox_KeyUp (oject_s *k, int key, int ascii)
{
	return k;
}

oject_s *O_TextBox_KeyDown (oject_s *k, int key, int ascii)
{
	k->txta.force_draw_cursor ++; // Force a cursor draw immediately no matter what the key was

	// Is it a printable character?
	if (in_range (SPACE_CHAR_32, ascii, CHAR_TILDE_126)) {
		if (k->txta.cursor_length) {
			// DELETE SELECTION FIRST
			TextBox_Edit_Delete_At_NumChars (k, TXT_SELLEFT(k), TXT_SELCOUNTABS(k));	
			TextBox_Cursor_Set_Refresh (k, TXT_SELLEFT(k), LENGTH_0);
		}
		TextBox_Edit_Insert_Char_At (k, ascii, k->txta.cursor_pos);
		TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos + 1, LENGTH_0);
		return k;
	}

	if (KM_NONE) goto no_shifts;
	if (KM_CTRL) goto ctrl_only;
	if (KM_SHIFT) goto shift_only;
	if (KM_CTRL_SHIFT) goto ctrl_shift;
	return k;

no_shifts: // UNSHIFTED (KM_NONE)
	
	switch (key) {
	case K_LEFTARROW:
		{
			int newpos = TXT_CURSOR_AT(k); 
			if (TXT_CURSOR_AT(k) > 0) 
				newpos --; // No move still must deselect
			TextBox_Cursor_Set_Refresh (k, newpos, LENGTH_0);
		}
		break;

	case K_RIGHTARROW:
		{
			int newpos = TXT_CURSOR_AT(k); 
			if (TXT_CURSOR_AT(k) < TXT_STRLEN(k)) 
				newpos ++; // No move still must deselect
			TextBox_Cursor_Set_Refresh (k, newpos, LENGTH_0);
		}
		break;

	case K_HOME:							
		TextBox_Cursor_Set_Refresh (k, 0, LENGTH_0);
		break;

	case K_END:
		TextBox_Cursor_Set_Refresh (k, TXT_STRLEN(k), LENGTH_0);
		break;

	case K_BACKSPACE:
		if (TXT_SELCOUNTABS(k)) {
			TextBox_Delete_Selection_Refresh (k);
		} else {
			// Backwards delete
			if (k->txta.cursor_pos > 0) {
				TextBox_Edit_Delete_At_NumChars (k, k->txta.cursor_pos - ONE_CHAR_1, ONE_CHAR_1);
				TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos - ONE_CHAR_1, LENGTH_0);
			}
		}
		break;

	case K_DELETE:						
		if (TXT_SELCOUNTABS(k)) {
			TextBox_Delete_Selection_Refresh (k);
		} else {
			// Forward delete
			if (k->txta.cursor_pos < TXT_STRLEN(k)) {
				TextBox_Edit_Delete_At_NumChars (k, k->txta.cursor_pos, ONE_CHAR_1);
				TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, LENGTH_0);
			}
		}
		break;
	} // sw


	return k;

ctrl_only:
	
	switch (key) {
	case K_LEFTARROW:
		if (TXT_CURSOR_AT(k) > 0) {
			// Move cursor to start of previous word
			int newpos = String_Find_Previous_Word_At(k->text_a, k->txta.cursor_pos);
			TextBox_Cursor_Set_Refresh (k, newpos, LENGTH_0);
		}
		break;

	case K_RIGHTARROW:
		if (TXT_CURSOR_AT(k) < TXT_STRLEN(k)) {
			// Move cursor to start of next word
			int newpos = String_Find_Next_Word_At(k->text_a, k->txta.cursor_pos);
			TextBox_Cursor_Set_Refresh (k, newpos, LENGTH_0);
		}
		break;

	case 'a': // CTRL-A - Select all
		TextBox_Cursor_Set_Refresh (k, TXT_STRLEN(k), -TXT_STRLEN(k));
		break;

	case 'c': // CTRL-C - Copy
		TextBox_Copy_Selection (k);
		break;

	case 'v': // CTRL-V - Get clipboard and insert it at current position, 
		// replace existing selection and cursor is at end of inserted text
		TextBox_Delete_Selection_Refresh (k);
		{
			char *s_z = Sys_GetClipboardData_Alloc (); // MAX_INPUTLINE_16384
			int newcursor = TextBox_Edit_Insert_String_At (k, s_z, k->txta.cursor_pos);
			Z_FreeNull_ (s_z);
			TextBox_Cursor_Set_Refresh (k, newcursor, LENGTH_0);
		}
		break;

	case 'x': // CTRL-X - Copy and then delete
		TextBox_Copy_Selection (k);
		TextBox_Delete_Selection_Refresh (k);
		break;
	} // sw

	return k;

shift_only:

	switch (key) {
	case K_LEFTARROW: // Selection extend left ONE
		if (TXT_CURSOR_AT(k) > 0)
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, k->txta.cursor_length - ONE_CHAR_1);
		break;

	case K_RIGHTARROW: // Selection extend right ONE
		if (TXT_CURSOR_AT(k) < TXT_STRLEN(k))
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, k->txta.cursor_length + ONE_CHAR_1);
		break;

	case K_HOME: // Selection extend to beginning
		{ // New selection length is current length + distance from cursor from 0
			int movement = TXT_CURSOR_AT(k); // distance from 0
			int newlen = k->txta.cursor_length - movement; // why negative? All movement to 0 is negative
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;

	case K_END: // Selection extend to end
		{ // New selection length is current length + distance to end of text
			int movement = TXT_STRLEN(k) - TXT_CURSOR_AT(k); // always positive or zero
			int newlen = k->txta.cursor_length + movement;
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;
	} // sw

	return k;

ctrl_shift: // CTRL + SHIFT

	switch (key) { // CTRL + SHIFT	
	case K_LEFTARROW:
		if (TXT_CURSOR_AT(k) > 0) {
			// Extend selection to previous word
			int newpos = String_Find_Previous_Word_At(k->text_a, TXT_CURSOR_AT(k));
			int newlen = k->txta.cursor_length - (TXT_CURSOR_AT(k) - newpos); // Delta like 4 - 1
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;

	case K_RIGHTARROW:
		if (TXT_CURSOR_AT(k) < TXT_STRLEN(k)) {
			// Extend selection to next word
			int newpos = String_Find_Next_Word_At(k->text_a, TXT_CURSOR_AT(k));
			int newlen = k->txta.cursor_length - (TXT_CURSOR_AT(k) - newpos);
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;

	case K_HOME: // Selection extend to beginning
		{ // New selection length is current length + distance from cursor from 0
			int newlen = k->txta.cursor_length - TXT_CURSOR_AT(k);
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;

	case K_END: // Selection extend to end
		{ // New selection length is current length + distance to end of text
			int newlen = TXT_STRLEN(k) - TXT_CURSOR_AT(k);
			TextBox_Cursor_Set_Refresh (k, k->txta.cursor_pos, newlen);
		}
		break;

	} // sw
	return k;

}
