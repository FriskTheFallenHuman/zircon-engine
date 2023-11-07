// keys_darkplaces.h

WARP_X_ (SCR_DrawScreen)

	// Advanced Console Editing by Radix radix@planetquake.com
	// Added/Modified by EvilTypeGuy eviltypeguy@qeradiant.com
	// Enhanced by [515]
	// Enhanced by terencehill

	// DPKEY CTRL-L clears console as if "clear" leaving input
	if (key == 'l' && keydown[K_CTRL]) {
		Cbuf_AddTextLine ("clear");
		return;
	}

	// DPKEY CTRL-U del cur line
	if (key == 'u' && keydown[K_CTRL]) { // like vi/readline ^u: delete currently edited line
		
		// clear line
		key_line[0] = ']';
		key_line[1] = 0;
		key_linepos = 1;
		
		Partial_Reset (); Con_Undo_Clear (); Selection_Line_Reset_Clear (); // CTRL U
		return;
	}

	// DPKEY CTRL-Q del cur line
	if (key == 'q' && keydown[K_CTRL]) { // like zsh ^q: push line to history, don't execute, and clear
		// clear line
		Key_History_Push ();
		key_line[0] = ']';
		key_line[1] = 0;
		key_linepos = 1; 
		
		Partial_Reset (); Con_Undo_Clear (); Selection_Line_Reset_Clear (); // Ctrl-Q
		return;
	}

	// DPKEY CTRL-P
	if (key == K_UPARROW || key == K_KP_UPARROW || (key == 'p' && keydown[K_CTRL])) {
		Selection_Line_Reset_Clear ();
		Key_History_Up();
		Partial_Reset (); Con_Undo_Clear ();  // Key_History_Up, cursor to EOL
		return;
	}

	// DPKEY CTRL-N
	if (key == K_DOWNARROW || key == K_KP_DOWNARROW || (key == 'n' && keydown[K_CTRL]) ) {
		Selection_Line_Reset_Clear (); 
		Key_History_Down();
		Partial_Reset (); Con_Undo_Clear (); // Key_History_Down, cursor to EOL
		return;
	}

	if (keydown[K_CTRL] && key == 'f') { // DPKEY CTRL-F: Key_History_Find_All prints all the matching commands
		Key_History_Find_All();
		return;
	}

	// Search forwards/backwards, pointing the history's index to the
	// matching command but without fetching it to let one continue the search.
	// To fetch it, it suffices to just press UP or DOWN.
	// DPKEY CTRL-R CTRL-SHIFT-R:  Key_History_Find_Forwards
	if (keydown[K_CTRL] && key == 'r') {
		if (keydown[K_SHIFT])	Key_History_Find_Forwards();
		else					Key_History_Find_Backwards();
		return;
	}
		
	// go to the last/first command of the history
	// DPKEY CTRL-, : Key_History_First
	if (keydown[K_CTRL] && key == ',') {
		Selection_Line_Reset_Clear (); 
		Key_History_First();
		Partial_Reset (); Con_Undo_Clear (); // Key_History_First EOL
		return;
	}
	
	// DPKEY CTRL-. :  Key_History_Last
	if (keydown[K_CTRL] && key == '.') {
		Partial_Reset (); Con_Undo_Clear ();
		Key_History_Last();
		Selection_Line_Reset_Clear (); // Key_History_Last .. EOL
		return;
	}

	if (key == K_PGUP || key == K_KP_PGUP) {
		if(keydown[K_CTRL])			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
		else						con_backscroll += ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN) {
		if(keydown[K_CTRL])			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
		else						con_backscroll -= ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
		return;
	}
 
	if (key == K_MWHEELUP) {
		if(keydown[K_CTRL])			con_backscroll += 1;
		else if(keydown[K_SHIFT])	con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
		else						con_backscroll += 5;
		return;
	}

	if (key == K_MWHEELDOWN) {
		if(keydown[K_CTRL])			con_backscroll -= 1;
		else if(keydown[K_SHIFT])	con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
		else						con_backscroll -= 5;
		return;
	}

	if (keydown[K_CTRL]) {
		// text zoom in
		// DPKEY CTRL-+ // Text size plus
		if (key == '+' || key == K_KP_PLUS) {
			if (con_textsize.integer < 128)	Cvar_SetValueQuick(&con_textsize, con_textsize.integer + 1);
			return;
		}
		// text zoom out
		// DPKEY CTRL-- // Text size minus
		if (key == '-' || key == K_KP_MINUS) {
			if (con_textsize.integer > 1)	Cvar_SetValueQuick(&con_textsize, con_textsize.integer - 1);
			return;
		}
		// DPKEY CTRL-0 // text zoom reset
		if (key == '0' || key == K_KP_INSERT) {
			Cvar_SetValueQuick(&con_textsize, atoi(Cvar_VariableDefString("con_textsize")));
			return;
		}
	}

