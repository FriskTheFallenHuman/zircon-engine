// oject_ccy_form_keymouse.c.h

WARP_X_CALLERS_ (ZForm_MouseMove)

oject_s *Form_MouseUp (oject_s *f, int key, int ascii)
{
	int x = in_windowmouse_x, y = in_windowmouse_y;
	oject_s *k_mouse = f->frm.kmousedown;
	
	if (k_mouse == NULL)
		return NULL;

	switch (k_mouse->po->enum_id) {
	case class_scrollbar:	O_ScrollBar_MouseButtonAction(k_mouse,x,y,MOUSEUP_0); break;
	case class_listview:	O_ListView_MouseButtonAction(k_mouse,x,y,MOUSEUP_0); break;
	//case class_tabselect:		O_TabSelect_MouseButtonAction (k_mouse, x, y,MOUSEUP_0);
	default:				break;
	} // sw

	if (f->frm.kmousedown) {
		// Assume it was processed, but still do a polite release
		oject_s *k = f->frm.kmousedown;
			k->mousedown_thing = 0;
			k->mousedown_when = 0;
			k->mousedown_at.x = k->mousedown_at.y = 0 ;
			k->mousedown_thumb_into.x = k->mousedown_thumb_into.y = 0;	// How many px into thumb are we?
			k->mousedown_thumb_at_mousedown_px.x = k->mousedown_thumb_at_mousedown_px.y = 0;	// How many px into thumb are we?
			k->mousedown_forced_thumb_pos.x = k->mousedown_forced_thumb_pos.y = 0;	// How many px into thumb are we?
			

			k->ui.mouse_down_time = 0;
			k->ui.mouse_down_origin.x = 0;
			k->ui.mouse_down_origin.y = 0;
			k->ui.mouse_down_thingi = 0;
			k->ui.mouse_down_valuei = 0;
			k->ui.is_move_thresh_met = 0;
			k->ui.mousepointer = mousepointer_arrow_default_0;
			//Vid_Cursor_Set (k->ui.mousepointer);
			RECT_SET (k->ui.mouse_down_rect, 0,0,0,0);

			f->frm.kmousedown = NULL;
	}

	//Con_PrintLinef ("Mouse release");
	return NULL;
}


oject_s *Form_Mouse_Move (oject_s *f, int x, int y)
{
	oject_s *k_mouse = f->frm.kmousedown;

	if (k_mouse == NULL) {
		// mouse button not pressed!
		oject_s *k = Oject_Hit_Drawn(f, x, y, isbuttonaction_false);

		if (k) { // MOUSEMOVE WITHOUT MOUSEDOWN		
			switch (k->po->enum_id) {
			//case class_scrollbar:	O_ScrollBar_MouseMove(k_mouse,x,y); break;
			case class_listview:	k = O_ListView_MouseMove (k,x,y); break;
			case class_sizedot:		k = O_SizeDot_MouseMove (k,x,y); break;
			case class_textbox:		k = O_TextBox_MouseMove (k,x,y); break;
			default:				k = NULL; break; // Not.
			} // sw
		}

		mousepointer_e mpdo = 
			k ? 
			k->ui.mousepointer : mousepointer_arrow_default_0;
		Vid_Cursor_Set (mpdo);

		return NULL;
	}

	//mousepointer_e ret = mousepointer_invalid_0;

	// MOUSE DOWN DURING MOVE
mousedown_mousemove:
	switch (k_mouse->po->enum_id) {
	case class_listview:	O_ListView_MouseMove	(k_mouse, x, y); break;
	case class_scrollbar:	O_ScrollBar_MouseMove	(k_mouse, x, y); break;
	case class_sizedot:		O_SizeDot_MouseMove		(k_mouse, x, y); break;
	case class_textbox:		O_TextBox_MouseMove		(k_mouse, x, y); break;
	} // sw

	//if (mpwanted == 0) {
	//	Vid_Cursor_Reset ();
	//} else {

	//}
	mousepointer_e mpdo = 
		k_mouse ? 
		k_mouse->ui.mousepointer : mousepointer_arrow_default_0;
	Vid_Cursor_Set (mpdo);


	return NULL;
}

oject_s *Form_Mouse2Down (oject_s *f)
{
	// For now, disallow mouse2 action if mouse1 is doing something already.
	if (f->frm.kmousedown)
		return NULL;

	oject_s *kmnu = f->servo.kcontextmenu;
	int x = in_windowmouse_x, y = in_windowmouse_y;

	// If context menu opened in top half, drop it down.
	// Otherwise drop it up.
	int tophalf_click = y < vid.height / 2;
	kmnu->cm.relative_rect.left = x;
	kmnu->cm.relative_rect.top = tophalf_click ? y : y - kmnu->cm.relative_rect.height;

	if (RECT_RIGHTOF(kmnu->cm.relative_rect) > vid.width)
		kmnu->cm.relative_rect.left = vid.width - kmnu->cm.relative_rect.width;
	else if (kmnu->cm.relative_rect.left < 0)
		kmnu->cm.relative_rect.left = 0;
	if (RECT_BOTTOMOF(kmnu->cm.relative_rect) > vid.height)
		kmnu->cm.relative_rect.top = vid.height - kmnu->cm.relative_rect.height;
	else if (kmnu->cm.relative_rect.top < 0)
		kmnu->cm.relative_rect.top = 0;

	//if (kmnu->is_hidden == false) {
	oject_s *kold = f->frm.kfocused;// f->frm.kcontextoldfocus;
	//if (f->frm.kfocused->po->enum_id != class_contextmenu)
	//		f->frm.kcontextoldfocus = f->frm.kfocused;
	//	else {
	//		// Do not set old focus ever to context menu, leave it the same.
	//		int j = 5;
	//	}
	//}

	Oject_Focus_Set (kmnu);

	Oject_Hidden_Set (kmnu, false /*!kmnu->is_hidden*/);

	if (kold && kold->po->enum_id != class_contextmenu) {
		f->frm.kcontextoldfocus = kold; //f->frm.kfocused;
	}

	O_CheckBox_Refresh (kmnu);
	Form_QueueRefresh (f); // This is the only form refresh available right now!


	return f;
}


// Caller must ensure f is not null
oject_s *Form_MouseButtonAction(oject_s *f, int key, int ascii, int isdown)
{
	int x = in_windowmouse_x, y = in_windowmouse_y;
	
	if (isdown == false)
		return Form_MouseUp (f, key, ascii);

	oject_s *k = Oject_Hit_Drawn(f, x, y, isbuttonaction_true);

	if (!k) 
		return NULL;

	int is_new_focus;
	
	if (f->frm.editmode)
		// EDIT MODE: Objects that don't normally focus can get focused 
		// except NOT servants (scrollbars, sizing hint, etc.)
		// Context menus -- a servant -- don't receive their focus in this manner
		// and disappear on lost focus so that exception doesn't matter here.
		is_new_focus = f->frm.kfocused != k && k->servant_owner == NULL && k != f;	
	else
		is_new_focus = f->frm.kfocused != k && Oject_Will_Focus_Normally(k);	

	// Lost Focus event
	if (is_new_focus) {
		Oject_Focus_Set (k);
	}

	switch (k->po->enum_id) { // ISDOWN ONLY -- ISUP handled above by transfer to Form_MouseUp
	case class_contextmenu:	O_ContextMenu_MouseButtonAction(k, x, y, MOUSEDOWN_1); break;
	case class_listbox:		O_ListBox_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	case class_listview:	O_ListView_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	case class_scrollbar:	O_ScrollBar_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	case class_sizedot:		O_SizeDot_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	case class_tabselect:	O_TabSelect_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	case class_textbox:		O_TextBox_MouseButtonAction (k, x, y, MOUSEDOWN_1); break;
	} // sw

	f->frm.kmousedown = k;
	return k; // If we hit the form that doesn't count.
}

WARP_X_ (Form_MouseButtonAction)
oject_s *Form_KeyDown (oject_s *f, int key, int ascii, int is_down)
{
	oject_s *k = f->frm.kfocused;
	if (!k) return NULL;

	if (!is_down) {
		switch (k->po->enum_id) { // UP
		case class_textbox:		return O_TextBox_KeyUp(k, key, ascii);
		} // sw
		return NULL;
	}

	switch (k->po->enum_id) { // IS DOWN
	case class_contextmenu:	return O_ContextMenu_KeyDown(k, key, ascii);
	case class_listbox:		return O_ListBox_KeyDown(k, key, ascii);
	case class_listview:	return O_ListView_KeyDown(k, key, ascii);
	case class_tabselect:	return O_TabSelect_KeyDown(k, key, ascii);
	case class_textbox:		return O_TextBox_KeyDown(k, key, ascii);
	} // sw

	return NULL;
}

