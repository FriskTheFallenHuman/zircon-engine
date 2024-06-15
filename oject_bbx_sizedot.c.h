// oject_bbx_rectangle.c.h


// Draw enters with clip set to screen
// Q: Does this use the "draw list"?
WARP_X_ (O_ListView_ColumnDrag Lister_Spawn Object_Draw_Recursive_Not_Form)

WARP_X_ (EditModeSetDotsObject)

oject_s *O_SizeDot_Draw (oject_s *k)
{
	if (k->direction == CM_MIDDLE_DOT_4)
		return k; // Middle dot doesn't draw.  It moves.

	//Draw_Rect	(&k->r_screen, k->backcolor, alpha_1_0);
	Draw_Rect (&k->tb.r_button, k->backcolor, alpha_1_0);
	

	return k;
}

WARP_X_ (Object_Refresh_Recursive O_TabSelect_Refresh_Plus_Early)
oject_s *O_SizeDot_Refresh (oject_s *k)
{
	// We have an origin, now set the position

	if (k->direction != alignment_middle_center_4) {
		xy_pairi_s *xy = &k->cm.dot_origin;
		int dsize = DDOT_SIZE_8 * 2;
		RECT_SET (k->cm.relative_rect, 
			xy->x - dsize / 2, 
			xy->y - dsize / 2, 
			dsize, 
			dsize
		);
		rect_s r_area = { RECT_SEND_ISO_CONTRACT(k->cm.relative_rect, 4) };
		k->tb.r_button = r_area;
	}
	return k;
}


mousepointer_e cursor_for_dir (int dir, int isdown)
{
	switch (dir) {
	case alignment_top_left_0:		return mousepointer_size_nwse_5;
	case alignment_top_center_1:	return mousepointer_size_northso_8;
	case alignment_top_right_2:		return mousepointer_size_nesw_6;
	case alignment_middle_left_3:	return mousepointer_size_weast_7;
	case alignment_middle_center_4:

		return isdown ? mousepointer_size_all_move_9 : mousepointer_arrow_default_0; // return mousepointer_move_6;
	case alignment_middle_right_5:	return mousepointer_size_weast_7;
	case alignment_bottom_left_6:	return mousepointer_size_nesw_6;
	case alignment_bottom_center_7:	return mousepointer_size_northso_8;
	case alignment_bottom_right_8:	return mousepointer_size_nwse_5;
	}
	return mousepointer_baker_column_20;
}

WARP_X_ (Form_MouseButtonAction O_ListView_ColumnDrag EditModeSetDotsObject)
oject_s *O_SizeDot_PlacementDrag (oject_s *k, int x, int y, int isdone)
{
	Form_Get (f,k);
	// Do we get silly unrelated mouseups?  I bet we do
	int		delta_x		= x - k->ui.mouse_down_origin.x;
	int 	delta_y		= y - k->ui.mouse_down_origin.y;
	oject_s *kshadow = f->frm.kfocused;

	// Calculate new size of shadowed object based on right bottom for the moment.
	rect_s r_shadow_new = k->ui.mouse_down_shadow_rect;//  kshadow->cm.relative_rect;

	Rect_Directional_Adjust (&r_shadow_new, (alignment_e) k->direction, delta_x, delta_y);
	//Rect_Directional_Bound_Mins (&r_shadow_original, 5, 5, 16, 16);

	kshadow->cm.relative_rect = r_shadow_new;

	Form_QueueDotsRefresh (f);



	if (isdone) {
		k->ui.mousepointer = mousepointer_arrow_default_0;
	} else {
		k->ui.mousepointer = cursor_for_dir(k->direction, isdown_true);
	}
	//Vid_SetWindowTitlef ("Mousepointer %d", k->ui.mousepointer);
	return k;
}

// isdown:  f->frm.kmousedown is SET by Form_MouseButtonAction AFTER we exit
// !isdown: f->frm.kmousedown should be us.
WARP_X_ (O_ScrollBar_MouseButtonAction O_ListView_ColumnDrag Oject_Focus_Set )

oject_s *O_SizeDot_MouseMove (oject_s *k, int x, int y)
{
	if (k->fctrl->frm.kmousedown == k)
		return O_SizeDot_PlacementDrag (k, x, y, isdone_false);

	k->ui.mousepointer = cursor_for_dir(k->direction, isdown_false);// mousepointer_size_both_10;
	return k;
}

oject_s *O_SizeDot_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	if (isdown == false) {
		k->ui.mousepointer = mousepointer_arrow_default_0;
		return O_SizeDot_PlacementDrag (k,x,y, isdone_true);
	}

	Form_Get (f, k);

	// We need to zorder is bastard now
	oject_s *kshadow = f->frm.kfocused;
	Form_ZOrder (kshadow);

	k->ui.mouse_down_time = host.realtime;
	XY_SET (k->ui.mouse_down_origin, x, y);

	k->ui.mouse_down_rect = k->cm.relative_rect;
	k->ui.mouse_down_shadow_rect = kshadow->cm.relative_rect;

	// NO HOOK THAT IS A PERPETUAL THUMP WITH NO MOUSEMOVEMENT
	Form_QueueRefresh (f);
	return k;
}











