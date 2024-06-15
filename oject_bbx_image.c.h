// oject_bbx_image.c.h




oject_s *O_Image_Draw (oject_s *k)
{
	cachepic_t *p = Draw_CachePic_Flags (k->image_name_a, CACHEPICFLAG_NOTPERSISTENT);
	Draw_ImageCachePic (&k->r_screen, p, k->backcolor, k->backalpha);
	return k;
}

