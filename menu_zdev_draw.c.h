// menu_zdev_draw.c.h

WARP_X_ (ZDev_Feed_Shall_Stop_Fn)
WARP_X_ (M_Menu_ZDev_f)
WARP_X_ (VM_drawsetcliparea DrawQ_ResetClipArea)

// Scaling and centering ...
// Viewport, scroll, selection, cursor hitting text, scaling at large.
// Does r_restart unload the fonts?  Do the fonts stay in memory?  Or are they freed see dds

static void M_ZDev_Draw (void)
{
	GoogleRobotoFont_Check ();

	int top_table_y_104 = 104;
	int left_200 = 200;
	Data_Dirty_Check ();

	#define scale_24 24

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	// M_Background (640, vid_conheight.integer, q_darken_true);

	drawcur_y = top_table_y_104;

	mz->usable_height = vid.height - top_table_y_104;
	mz->visible_rows = (mz->usable_height / scale_24) - 3; // Baker: Room for bottom

	// Baker: Do it this way because a short list may have more visible rows than the list count so using bound doesn't work.
	if (mz->scroll_is_blocked == false) {
		mz->start_row = mz->cursor - (mz->visible_rows / 2);

		if (mz->start_row > local_count - mz->visible_rows)
			mz->start_row = local_count - mz->visible_rows;
		if (mz->start_row < 0)
			mz->start_row = 0; // visible_rows can exceed local_count
	}

	mz->beyond_row = Smallest (mz->start_row + mz->visible_rows, local_count);

//	vec3_t pos = { 32, 32}, rgb = { 1,1,1}, scaleZ = { scale_24, scale_24 };

	WARP_X_ (VM_drawpic)
	WARP_X_ (VM_drawstring)

	// row_0_at_48
	drawcur_y = top_table_y_104; // Reset

	if (mz->start_row < mz->beyond_row) {
		for (int idx = mz->start_row; idx < mz->beyond_row; idx ++) {
			const char *s = mz->list.strings[idx];
			Hotspots_Add				(left_200, drawcur_y, /*wh*/ 320, scale_24 + 1, /*count*/ 1, hotspottype_button, q_force_scale_0); // PPX DUR
			//Draw_MenuString_XYScaleMax	(172, drawcur_y, scale_24, s, 40);

	DrawQ_String_Scale (left_200, drawcur_y, s, /*maxlen*/ 40, 24, 24, scale_1_0,
			scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, ignorecolorcodes_true, zdev_dpfont );


			if (idx == mz->cursor)
				drawsel_idx = (idx - mz->start_row); // 0 based
			drawcur_y += 24;//scale_24;
		} // for
	} // endrow > startrow
	else
	{
		DrawQ_String_Scale (left_200, drawcur_y, "(No data)", /*maxlen*/ 40, 24, 24, scale_1_0,
				scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, ignorecolorcodes_true, zdev_dpfont );
	}


	DrawQ_String_Scale (vid.width - 200, 0, va32("%d entries",local_count), /*maxlen*/ 40, 24, 24, scale_1_0,
			scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, ignorecolorcodes_true, zdev_dpfont );

	// Header
//headers:
	// Baker: We draw these late ...
	// ("engine/bicons/checked",
	vec3_t titlepos = {32,32};

	#define titlesize_56 56
	DrawQ_String_Scale (VECTOR2_SEND(titlepos), "^7dev.^xfa0info", maxlen_0, titlesize_56, titlesize_56, scale_1_0,
			scale_1_0, q_rgba_solid_white_4_parms, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, ignorecolorcodes_false, zdev_dpfont );


	DrawQ_String_Scale (36, 528, "Cycle = TAB", maxlen_0, 24, 24, scale_1_0,
			scale_1_0, RGBFLOT(255,192,0),alpha_1_0, DRAWFLAG_NORMAL_0, OUTCOLOR_NULL, ignorecolorcodes_false, zdev_dpfont );


	int button_id = -1;
	MZ_CheckBox			(1156, top_table_y_104, "Details", mz->is_more_detail, mz->checkbox_theme, mz->checkbox_scale, &button_id, DEV_Detail_Click);
	MZ_DrawTabSelect	(36, top_table_y_104, s_tabnames, s_tabnames_count, mz->list_what, mz->tabsel_theme, mz->tabsel_scale, &button_id, DEV_Category_Click);

	PPX_DrawSel_End_Full ();

	// Baker: Column headers are mouse interactive only and do not affect local cursor

}

WARP_X_ (M_ZDev_Key)
