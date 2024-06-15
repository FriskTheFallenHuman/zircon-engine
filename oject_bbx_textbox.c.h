// oject_bbx_textbox.c.h

WARP_X_ (Cursor_Recalc O_TabSelect_Draw, O_Label_Draw O_CheckBox_Draw )
oject_s *O_TextBox_Refresh (oject_s *k);

#include "oject_bbx_textbox_procs.c.h"

oject_s *O_TextBox_Finalize(oject_s *k)
{
	if (k->text_a == NULL)				// If we are lacking text, spawn it.
		k->text_a = Z_StrDup ("");

	k->txta._antibackcolor[0] = bound(0, 1 - k->backcolor[0], 1);
	k->txta._antibackcolor[1] = bound(0, 1 - k->backcolor[1], 1);
	k->txta._antibackcolor[2] = bound(0, 1 - k->backcolor[2], 1);
	k->txta._antiforecolor[0] = bound(0, 1 - k->forecolor[0], 1);
	k->txta._antiforecolor[1] = bound(0, 1 - k->forecolor[1], 1);
	k->txta._antiforecolor[2] = bound(0, 1 - k->forecolor[2], 1);
	
	return k;
}


// Cliparea cleared after all object draw.  If we were culled, this draw will not occur.
oject_s *O_TextBox_Draw (oject_s *k)
{
	DrawKit_Border_Ex ( 
		border_Wall_none_0,	// k->pp.border_walls, 
		false,				// k->pp.cc.is_outset_border, 
		false,				// k->pp.cc.is_thin_threed_border, 
		true,				// k->pp.is_border, 
		true,				// k->pp.is_threed, 
		false,				// k->pp.is_transparent_background, 
		color3_white,		// k->pp._backcolorage.c4, 
		color3_black,		// k->pp._bordercolorage.c4,			UNKNOWN
		&k->r_screen
	);

	Draw_Rect			(&k->txta.r_textarea, k->backcolor, alpha_1_0);//k->backalpha);
	Draw_Clip_Set_Rect	(&k->txta.r_textarea);

	if (Oject_Have_Focus(k)) {
		if (TXT_SELCOUNTABS(k)) {
			Draw_StringXY		(k->txta.seg1_left_px - k->r_scrollport.left, k->txta.r_textarea.top, k->txta.seg1_a, k->fontsize, k->forecolor, alpha_1_0);
			Draw_Rect_XYWH		(k->txta.r_cursor_highlight.left - k->r_scrollport.left, 
									k->txta.r_cursor_highlight.top,
									k->txta.r_cursor_highlight.width,
									k->txta.r_cursor_highlight.height,

									k->txta._antibackcolor, alpha_1_0);
			Draw_StringXY		(k->txta.seg2_left_px - k->r_scrollport.left, k->txta.r_textarea.top, k->txta.seg2_a, k->fontsize, k->txta._antiforecolor, alpha_1_0);
			Draw_StringXY		(k->txta.seg3_left_px - k->r_scrollport.left, k->txta.r_textarea.top, k->txta.seg3_a, k->fontsize, k->forecolor, alpha_1_0);
		}
		else {
			Draw_StringXY		(k->txta.r_textarea.left - k->r_scrollport.left, k->txta.r_textarea.top, k->text_a, k->fontsize, k->forecolor, alpha_1_0);

		}
		int shall_not_blink = (int)(host.realtime*  4) & 2; // 4 = con_cursorspeed
		if (shall_not_blink || k->txta.force_draw_cursor > 0) {
			const float *cursor_color = k->txta.cursor_length != 0 ? k->backcolor : color3_black;
			Draw_Rect_XYWH	(k->txta.r_cursor.left - k->r_scrollport.left, 
							k->txta.r_cursor.top,
							k->txta.r_cursor.width,
							k->txta.r_cursor.height,
							cursor_color, 
							alpha_1_0
							);
			k->txta.force_draw_cursor = 0;
		}
	} // focused so draw cursor.
	else {
		// NO FOCUS
		Draw_StringXY		(k->txta.r_textarea.left - k->r_scrollport.left, k->txta.r_textarea.top, k->text_a, k->fontsize, k->forecolor, alpha_1_0);
	}
	return k;
}

WARP_X_ (Object_Refresh_Recursive)

// Idea size is height = fontsize + 4 + 8 = fontsize + 12 
oject_s *O_TextBox_Refresh (oject_s *k)
{
	RECT_SET_CONTRACT (k->r_interior, k->r_screen, TXT_BORDER_WIDTH_2);	// Shrink by 4 (excludes border)
	RECT_SET_CONTRACT (k->txta.r_textarea, k->r_interior, TXT_MARGIN_4); // Shrink by 2

	TextBox_Cursor_Recalc (k);

	return k;
}

WARP_X_ (M_ZForm_Key Form_KeyDown Z_StrDupf)

#include "oject_bbx_textbox_keys.c.h"
#include "oject_bbx_textbox_mouse.c.h"









