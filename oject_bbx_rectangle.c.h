// oject_bbx_rectangle.c.h


// Draw enters with clip set to screen
// Q: Does this use the "draw list"?
WARP_X_ (O_ListView_ColumnDrag Lister_Spawn Object_Draw_Recursive_Not_Form)
oject_s *O_Rectangle_Draw (oject_s *k)
{
	Draw_Rect	(&k->r_screen, k->backcolor, alpha_1_0);

	return k;
}

WARP_X_ (Object_Refresh_Recursive)
oject_s *O_Rectangle_Refresh (oject_s *k)
{
	//RECT_SET (kline->cm.relative_rect, 200, 200, 200, 200);
	return k;
}






