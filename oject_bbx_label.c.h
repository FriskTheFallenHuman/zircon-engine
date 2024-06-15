// oject_bbx_label.c.h

oject_s *O_Label_Finalize (oject_s *k)
{
	if (k->caption_a == NULL)				// If we are lacking text, spawn it.
		k->caption_a = Z_StrDup ("");
	return k;

}

oject_s *O_Label_Draw (oject_s *k)
{
	Draw_String (&k->txta.r_textarea, k->caption_a, k->fontsize, k->forecolor, alpha_1_0);
	return k;
}

WARP_X_ (Object_Refresh_Recursive)

oject_s *O_Label_Refresh_Plus_Early (oject_s *k)
{
	k->txta.stringwidthi_px = Draw_StringWidthInt (zdev_dpfont, k->caption_a, k->fontsize);

	k->txta.r_textarea.width	= k->txta.stringwidthi_px;
	k->txta.r_textarea.height	= ceil(k->fontsize);
	
	// Autosize forces minimal footprint.
	if (k->autosize || k->cm.relative_rect.width == 0 || k->cm.relative_rect.height == 0)
		RECT_SET_SIZE (k->cm.relative_rect, k->txta.r_textarea.width, k->txta.r_textarea.height);

	Label_Rect_Edit_Align (&k->txta.r_textarea, &k->r_screen, (alignment_e) k->alignment);

	return k;
}


