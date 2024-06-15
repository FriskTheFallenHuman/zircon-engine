// oject_bbx_polygon.c.h



oject_s *O_Polygon_Draw (oject_s *k)
{
	//vec3_t color3 = { 1, 1, 1}; // white
	vec3_t color31 = { RGBFLOT(21,43,39) }; // white   21 43 39  21 40 37, 11 101 84,  11 100 84
	vec3_t color32 = { RGBFLOT(11,101,84) }; // white
	vec3_t color33 = { RGBFLOT(11,100,84) }; // white
	vec3_t tc3 = { 0, 0, 0}; // white
	
	Draw_PolygonBegin (POLYGON_WHITE_TEXTURE_NULL);
	for (int idx = 0; idx < k->vertexlist_a.count; idx += POLYGON_2D_2) {
		float *f = &k->vertexlist_a.floats[idx];
		
		if (idx == 0)
			Draw_PolygonVertex2D (f, tc3, color31 /*k->forecolor*/, alpha_1_0);
		else if (idx == 2)
			Draw_PolygonVertex2D (f, tc3, color32, alpha_1_0);
		else if (idx == 4)
			Draw_PolygonVertex2D (f, tc3, color33, alpha_1_0);
	}
	Draw_PolygonEnd ();
	
	return k;
}

oject_s *O_Polygon_Refresh (oject_s *k)
{
	return k;
}
