// oject_bbx_checkbox.c.h

#if 0
	//Draw_Clip_Set_ (&k->r_box); // Submits draw buffer
	Draw_Clip_Set  (&k->r_box); // Submits draw buffer
	Draw_String	(&k->r_text, "THIS TEXT IS WAY TOO LONG"/*semp(k->caption_a)*/, k->fontsize, k->forecolor, alpha_1_0);
	Draw_Clip_Clear (); // Submits draw buffer
#endif

WARP_X_ (O_CheckBox_Refresh VM_drawsetcliparea VM_drawresetcliparea)
oject_s *O_CheckBox_Draw (oject_s *k)
{
	//ccs *spic = k->is_checked ? "engine/bicons/checked" :  "engine/bicons/unchecked";
	//cachepic_t *pic;
	//pic = Draw_CachePic_Flags (spic, CACHEPICFLAG_NOTPERSISTENT);// pw = Draw_GetPicWidth(pic);

	//Draw_RectRound	(&k->txt.r_box, k->backcolor, alpha_1_0);
	//Draw_Image		(&k->txt.r_image, pic, color3_white, alpha_1_0);

	return k;
}

WARP_X_ (O_CheckBox_Draw)
oject_s *O_CheckBox_Refresh (oject_s *k)
{
	
////	O_CheckBox_Draw
//	// Vertical padding takes supplied size less font size ... just how it is.
//	// Or should we use a static amount <---------
//
//	// Defaultish
//	k->cellpadding_x_pct = 0.50;	// how much to pad the text for a button
//	k->cellpadding_y_pct = 0.20;	// how much to pad the text for a button
//	k->cellspacing_pct = 0.05;		// percent of fontsize for row
//
//	k->txt.fstringwidth	= Draw_StringWidth (zdev_dpfont, semp(k->caption_a), k->fontsize);
//	k->txt.r_text.width		= k->txt.fstringwidth;
//	k->txt.r_text.height	= k->fontsize;
//
//	// Assume infinite size on finite canvas
//	k->descender_pct_rt	= zdev_dpfont->ft_baker_descend_pct;
//	k->descender_px_rt	= k->descender_pct_rt * k->fontsize;
//
//	// Assume image is a square of fontsize * fontsize
//	float padding_x = (k->cellpadding_x_pct * k->fontsize);
//	float padding_y = (k->cellpadding_y_pct * k->fontsize);
//	k->buttonwidth_px_rt	= (padding_x * 2.5) + ceil(k->fontsize) + ceil(k->txt.fstringwidth);
//	k->buttonheight_px_rt	= (padding_y * 2) + ceil(k->fontsize);
//
//	RECT_SET (k->txt.r_box,   k->cm.relative_rect.left, k->cm.relative_rect.top, k->buttonwidth_px_rt, k->buttonheight_px_rt);
//	RECT_SET (k->txt.r_image, k->cm.relative_rect.left + padding_x * 0.5, k->cm.relative_rect.top + padding_y, k->fontsize, k->fontsize);
//	RECT_SET (k->txt.r_text,  k->cm.relative_rect.left + padding_x * 1.5 + k->txt.r_image.width, k->txt.r_image.top, k->txt.fstringwidth, k->fontsize);
//
//	if (k->cm.relative_rect.width > k->txt.r_box.width)
//		k->txt.r_box.width = k->cm.relative_rect.width;
//
//	//Label_Rect_Edit_Align (&k->r_text, &k->dr_canvas, (alignment_e) k->alignment);
//
	return k;

}

