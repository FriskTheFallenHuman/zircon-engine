// oject_bbx_scrollbar.c.h

WARP_X_ (O_ScrollBar_Refresh)

void O_ScrollBar_MouseHook_Fire (oject_s *k);
oject_s *O_ScrollBar_Refresh (oject_s *k);

oject_s *O_ScrollBar_Draw (oject_s *k)
{
	//Draw_Rect	(&k->r_screen, color3_form208, alpha_1_0);
top:
	DrawKit_Border_Ex (
		border_Wall_none_0,
		OUTSET_TRUE,
		THIN_FALSE,
		BORDER_TRUE,
		THREED_TRUE,
		TRANSPARENT_FALSE,
		color3_form208,
		color3_black,
		&k->sb.r_top
	);
	DrawKit_Rect_Arrow (&k->sb.r_top, k->is_horizontal ? arrow4_left_0 : arrow4_up_2, /*arrow size*/ 7, color3_black);

bot:
	DrawKit_Border_Ex (
		border_Wall_none_0,
		OUTSET_TRUE,
		THIN_FALSE,
		BORDER_TRUE,
		THREED_TRUE,
		TRANSPARENT_FALSE,
		color3_form208 /*color3_form208*/,
		color3_black,
		&k->sb.r_bottom
	);
	DrawKit_Rect_Arrow (&k->sb.r_bottom, k->is_horizontal ? arrow4_right_1 : arrow4_down_3, /*arrow size*/ 7, color3_black);

track:
	Draw_Rect	(&k->sb.r_track, color3_gray_25, alpha_1_0);

thumb:
	DrawKit_Border_Ex (
		border_Wall_none_0,
		OUTSET_TRUE,
		THIN_FALSE,
		BORDER_TRUE,	// This uses border color
		THREED_TRUE,	// Causes draw
		TRANSPARENT_FALSE, // Supposedly transparent false uses backcolor
		color3_form208,	// backcolor .. drawn for some including 3D
		color3_black,	// bordercolor, used when?
		&k->sb.r_thumb
	);

	return k;
}

WARP_X_CALLERS_ (O_ScrollBar_Action /*O_ScrollBar_MouseMove*/ thumb)
void Property_Set_Float (oject_s *k, things_e prop, float v)
{
	if (prop == prop_value && k->po->enum_id == class_scrollbar) {
		SET___ k->value = CLAMP(0, v, k->maxvalue);
		if (k->servant_owner && isin3(k->servant_owner->po->enum_id, class_listbox, class_listview, class_tabselect)) {
			// ListBox scroll event
			k->in_recursion ++;
				Lister_SetPct (k->servant_owner, k->value, k);
			k->in_recursion --;
		}
		// We want a refresh.  Caller should provide it.
	}
}

WARP_X_CALLERS_ (O_ListBox_VisRangeCheck_Refresh)
void O_Scrollbar_SetPct (oject_s *k, float pct)
{
	if (k->in_recursion)
		return;

	Property_Set_Float (k, prop_value, pct);

	O_ScrollBar_Refresh (k);
}

oject_s *O_ScrollBar_Refresh (oject_s *k)
{
	rect_s *prect = &k->cm.relative_rect;

	if (k->is_horizontal) {
		RECT_SET (k->sb.r_top,		prect->left, prect->top, KNOB_SIZE_16, KNOB_SIZE_16);
		RECT_SET (k->sb.r_track,	prect->left+ KNOB_SIZE_16, prect->top , prect->width - KNOB_SIZE_16 * 2, prect->height);
		RECT_SET (k->sb.r_bottom,	RECT_RIGHTOF(*prect)-KNOB_SIZE_16, prect->top, KNOB_SIZE_16, KNOB_SIZE_16 );
	} else {

		RECT_SET (k->sb.r_top,		prect->left, prect->top, KNOB_SIZE_16, KNOB_SIZE_16);
		RECT_SET (k->sb.r_track,	prect->left, prect->top + KNOB_SIZE_16, prect->width, prect->height - KNOB_SIZE_16 * 2);
		RECT_SET (k->sb.r_bottom,	prect->left, RECT_BOTTOMOF (*prect) - KNOB_SIZE_16, KNOB_SIZE_16, KNOB_SIZE_16 );
	}

	float thumb_size_pct = 0.50;

	if (k->is_horizontal) {
		k->sb.thumbsize_px		= thumb_size_pct * k->sb.r_track.width;
		vclamp (k->sb.thumbsize_px, KNOB_SIZE_16, k->sb.r_track.width - 1);
		k->sb.track_empty_px	= k->sb.r_track.width - k->sb.thumbsize_px;
	}  else {
		k->sb.thumbsize_px		= thumb_size_pct * k->sb.r_track.height;
		vclamp (k->sb.thumbsize_px, KNOB_SIZE_16, k->sb.r_track.height - 1);
		k->sb.track_empty_px	= k->sb.r_track.height - k->sb.thumbsize_px;
	}

	k->sb.num_top_positions = k->sb.track_empty_px + 1;
	// 0 to 49   track height 60  thumb height 10 ..
	// thumb can be at 0 through 9
	// thumb can be at 50 through 59
	// 50 available spots = means 51 positions.

	float pct_down = k->value / k->maxvalue /*d:1*/; // 0 to 1.0

	int pixels_down = Math_Round(pct_down * k->sb.track_empty_px);

	if (k->is_horizontal) {
		RECT_SET (k->sb.r_thumb,  prect->left+ KNOB_SIZE_16 + pixels_down, prect->top, k->sb.thumbsize_px, KNOB_SIZE_16);
	} else {
		RECT_SET (k->sb.r_thumb,  prect->left, prect->top + KNOB_SIZE_16 + pixels_down, KNOB_SIZE_16, k->sb.thumbsize_px);
	}

	if (k->fctrl->frm.kmousedown == k && k->mousedown_thing == SB_ACTION_THUMB_4) {
		// Thumb is mouse controlled
		if (k->is_horizontal) {
			k->sb.r_thumb.left    = k->mousedown_forced_thumb_pos.x;

			// LIMIT TO TRACK
			if (k->sb.r_thumb.left < k->sb.r_track.left)
				k->sb.r_thumb.left = k->sb.r_track.left;
			else if (RECT_RIGHTOF(k->sb.r_thumb) > RECT_RIGHTOF(k->sb.r_track)) {
				k->sb.r_thumb.left = RECT_RIGHTOF(k->sb.r_track) - k->sb.r_thumb.width;
			}

		} else {
			k->sb.r_thumb.top    = k->mousedown_forced_thumb_pos.y;

			// LIMIT TO TRACK
			if (k->sb.r_thumb.top < k->sb.r_track.top)
				k->sb.r_thumb.top = k->sb.r_track.top;
			else if (RECT_BOTTOMOF(k->sb.r_thumb) > RECT_BOTTOMOF(k->sb.r_track)) {
				k->sb.r_thumb.top = RECT_BOTTOMOF(k->sb.r_track) - k->sb.r_thumb.height;
			}
			//Con_PrintLinef ("Thumb at y %d", k->mousedown_thumb_pos.y);
		}
	}

	//Con_PrintLinef ("Thumb at y %d", k->mousedown_thumb_pos.y);

	return NULL;
}


void O_ScrollBar_Action (oject_s *k, int action, int is_sethook)
{
	float newvalue;
	switch (action) {
	case SB_ACTION_UP_0:		newvalue = k->value - 0.05; break;
	case SB_ACTION_DOWN_1:		newvalue = k->value + 0.05; break;
	case SB_ACTION_PGUP_2:		newvalue = k->value - 0.10; break;
	case SB_ACTION_PGDOWN_3:	newvalue = k->value + 0.10; break;
	default:
		return;
	} // sw

	Property_Set_Float (k, prop_value, newvalue);
	O_ScrollBar_Refresh (k);

	if (is_sethook && k->mousedown_when)
		Form_Hook_Set (k->fctrl, k, O_ScrollBar_MouseHook_Fire, host.realtime + SCROLLBAR_HOOK_REFIRE_0_1);
}

WARP_X_CALLERS_ (Form_Draw yikes!)
void O_ScrollBar_MouseHook_Fire (oject_s *k)
{
	O_ScrollBar_Action (k, k->mousedown_thing, q_sethook_true);
}


// Return is important, null means mousepointer goes to default.
oject_s *O_ScrollBar_MouseMove (oject_s *k, int x, int y)
{
	if (k->mousedown_thing == SB_ACTION_THUMB_4) {
		if (k->is_horizontal) {
			// Thumb moved
			int delta_x = x - k->mousedown_at.x; // 130 (now) - 200 (now) means -70 up
			int new_thumb_left = k->mousedown_thumb_at_mousedown_px.x + delta_x;
			vclamp (new_thumb_left, k->sb.r_track.left, k->sb.r_track.left + k->sb.track_empty_px);
			k->mousedown_forced_thumb_pos.x = new_thumb_left;
			int px_below_track_left = new_thumb_left - k->sb.r_track.left;
			float pct = px_below_track_left / (float)k->sb.track_empty_px; //(k->sb.num_top_positions - 1);

			//varpi (x);
			//varpi (k->mousedown_at.x);
			//varpi (delta_x);
			//varpi (new_thumb_left);
			//varpi (k->mousedown_forced_thumb_pos.x);
			Property_Set_Float (k, prop_value, pct);
		} else {
			// Thumb moved
			int delta_y = y - k->mousedown_at.y; // 130 (now) - 200 (now) means -70 up
			int new_thumb_top = k->mousedown_thumb_at_mousedown_px.y + delta_y;
			vclamp (new_thumb_top, k->sb.r_track.top, k->sb.r_track.top + k->sb.track_empty_px);
			k->mousedown_forced_thumb_pos.y = new_thumb_top;
			int px_below_track_top = new_thumb_top - k->sb.r_track.top;
			//varpi (delta_y);
			//varpi (new_thumb_top);
			//varpi (k->mousedown_forced_thumb_pos.y);
			float pct = px_below_track_top / (float)k->sb.track_empty_px; //(k->sb.num_top_positions - 1);
			Property_Set_Float (k, prop_value, pct);
		}
		O_ScrollBar_Refresh (k);
	}
	return NULL;
}

oject_s *O_ScrollBar_MouseButtonAction (oject_s *k, int x, int y, int isdown)
{
	//K_GET_RELATIVE_XY (x,y);
	if (isdown == false) {
		if (k->fctrl->frm.kmousedown == k && k->mousedown_thing == SB_ACTION_THUMB_4) {
			O_ScrollBar_MouseMove (k, x, y); // Perform final move?
			O_ScrollBar_Refresh (k); // Thumb to final position
		}

		Form_Hook_Clear (k->fctrl, k);
		return NULL;
	}

mousedown:
	// This might be able to repeat, don't let it.
	if (k->fctrl->frm.kmousedown == k && k->mousedown_thing == SB_ACTION_THUMB_4) {
		Con_PrintLinef ("Ignored thumb mousedown already down");
		return NULL;
	}

	// MOUSE DOWN
	if (RECT_HIT(k->sb.r_top, x, y)) {
		k->mousedown_when = host.realtime;
		k->mousedown_thing = SB_ACTION_UP_0;
		O_ScrollBar_Action (k, k->mousedown_thing, q_sethook_true);
	}

	else if (RECT_HIT(k->sb.r_bottom, x, y)) {
		k->mousedown_when = host.realtime;
		k->mousedown_thing = SB_ACTION_DOWN_1;
		O_ScrollBar_Action (k, k->mousedown_thing, q_sethook_true);
	}

	else if (RECT_HIT(k->sb.r_thumb, x, y)) {
		// Don't allow repeat!
		if (k->mousedown_thing != SB_ACTION_THUMB_4) {
			k->mousedown_when = host.realtime;
			k->mousedown_thing = SB_ACTION_THUMB_4; //is_above_thumb ? SB_ACTION_PGUP_2 : SB_ACTION_PGDOWN_3;

			XY_SET (k->mousedown_at, x, y);
			XY_SET (k->mousedown_thumb_at_mousedown_px, k->sb.r_thumb.left, k->sb.r_thumb.top);
			k->mousedown_thumb_into.x = k->mousedown_at.x - k->sb.r_thumb.left;
			k->mousedown_thumb_into.y = k->mousedown_at.y - k->sb.r_thumb.top;
			k->mousedown_forced_thumb_pos = k->mousedown_thumb_at_mousedown_px;
			//Con_PrintLinef ("Mousedown at %d", k->mousedown_thumb_at_mousedown_px.y);
		} else {
			//Con_PrintLinef ("Thumb2 ignored");
		}
	}

	else if (RECT_HIT(k->sb.r_track, x, y)) {

		int is_above_thumb = y <= k->sb.r_thumb.top;
		k->mousedown_when = host.realtime;
		k->mousedown_thing = is_above_thumb ? SB_ACTION_PGUP_2 : SB_ACTION_PGDOWN_3;
		O_ScrollBar_Action (k, k->mousedown_thing, q_sethook_true);

	}

	return NULL;
}


