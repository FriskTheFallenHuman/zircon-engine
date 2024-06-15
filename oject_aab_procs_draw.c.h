// oject_aab_procs_draw.c.h

void Draw_Circle_Slice (const vec3_t circle_center_pos, float r, const vec3_t coloR, float pct1, float pct2, float alpha)
{
	float rg = pct2 - pct1;
	vec3_t pos;
	vec3_t texco_00 = {0,0};

	Draw_PolygonBegin (NULL); // WHITE_TEXTURE
	Draw_PolygonVertex2D (circle_center_pos, texco_00, coloR, alpha);
	for (float n = 0; n < m_num_segments_61; n++) {
		circle_setpos_for_pct (/*modify:*/ pos, circle_center_pos, r, pct1 + (n / m_num_segments_61) * rg);
		Draw_PolygonVertex2D (pos, texco_00, coloR, alpha);
	}
	Draw_PolygonEnd ();
}

#define radii 13  // min sizes apply
void sDraw_RectRound_Original (const vec3_t rect_pos, const vec3_t rect_size, const vec3_t rect_color3, float alpha)
{
	vec3_t posLTC = {rect_pos[0] +        radii,			rect_pos[1] +				radii,0 };
	vec3_t posRTC = {rect_pos[0] + rect_size[0] - radii,	rect_pos[1] +				radii,0 };
	vec3_t posLBC = {rect_pos[0] +        radii,			rect_pos[1] + rect_size[1] -	radii,0 };
	vec3_t posRBC = {rect_pos[0] + rect_size[0] - radii,	rect_pos[1] + rect_size[1] -	radii,0 };

	Draw_Circle_Slice (posLTC, radii, rect_color3, 0.75, 1.0 , alpha);
	Draw_Circle_Slice (posRTC, radii, rect_color3, 0   , 0.25, alpha);
	Draw_Circle_Slice (posLBC, radii, rect_color3, 0.5 , 0.75, alpha);
	Draw_Circle_Slice (posRBC, radii, rect_color3, 0.25, 0.5 , alpha);

	vec3_t posRW = {rect_pos[0]				, rect_pos[1] +        radii ,0};
	vec3_t posRH = {rect_pos[0] +  radii	, rect_pos[1]                ,0};

	vec3_t ss1 = {radii, rect_size[1] - radii * 2 ,0};
	Draw_Fill (posRW, ss1, rect_color3, alpha, DRAWFLAG_NORMAL_0);

	vec3_t ss2 = {rect_size[0] - radii * 2, rect_size[1],0};
	Draw_Fill (posRH, ss2, rect_color3, alpha, DRAWFLAG_NORMAL_0);

	vec3_t posRWx= {rect_pos[0] + rect_size[0] - radii            , rect_pos[1] + radii,0 };
	Draw_Fill (posRWx, ss1, rect_color3, alpha, DRAWFLAG_NORMAL_0);
}
#undef radii

void Draw_RectRound (const rect_s *pri, const vec3_t backcolor, float alpha)
{
	vec3_t pos	= { RECT_SEND_XY (*pri) };
	vec3_t size = { RECT_SEND_WH (*pri) };
	sDraw_RectRound_Original (pos, size, backcolor, alpha_1_0);
}


// Baker: u8_strlen is what is used for VM_strlen, it ignores color codes?
#if 0 // This supports null strings .. do not want at this time
oject_s *Draw_Caption (rect_s *pr, ccs *caption, float fontsize, vec3_t forecolor, float forealpha)
{
	if (!caption)
		return NULL;

	Draw_String (pr, caption, fontsize, forecolor, forealpha);
	return NULL;
}
#endif

WARP_X_ (SBar2D_PolygonBegin)
void Draw_PolygonBegin (const char *texname_or_null)//, float drawflags, float isdraw2d)
{
	if (texname_or_null == NULL || texname_or_null[0] == 0)
		texname_or_null = "$whiteimage";

	c_strlcpy (ojpoly.texname, texname_or_null);
	ojpoly.drawflags = DRAWFLAG_NORMAL_0;// drawflags;
	ojpoly.numvertices = 0;
}


void Draw_PolygonEnd (void)
{
	int i;
	int e0 = 0, e1 = 0, e2 = 0;
	float *o;
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	texture_t *tex;
	int materialflags;

	// determine if vertex alpha is being used so we can provide that hint to GetTexture...
	qbool hascolor = false;
	qbool hasalpha = false;
	for (i = 0; i < ojpoly.numvertices; i++) {
		o = ojpoly.vertexdata + 10 * i;
		if (o[6] != 1.0f || o[7] != 1.0f || o[8] != 1.0f)
			hascolor = true;
		if (o[9] != 1.0f)
			hasalpha = true;
	} // for

	// create the surface, looking up the best matching texture/shader
	materialflags = MATERIALFLAG_WALL;
	if (csqc_polygons_defaultmaterial_nocullface.integer /*d: 0*/)
		materialflags |= MATERIALFLAG_NOCULLFACE;
	if (hascolor)
		materialflags |= MATERIALFLAG_VERTEXCOLOR;
	if (hasalpha)
		materialflags |= MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
	tex = Mod_Mesh_GetTexture(mod, ojpoly.texname, ojpoly.drawflags,
		TEXF_ALPHA, materialflags);
	surf = Mod_Mesh_AddSurface(mod, tex, false);

	// create triangle fan
	for (i = 0; i < ojpoly.numvertices; i++) {
		o = ojpoly.vertexdata + 10 * i;
		e2 = Mod_Mesh_IndexForVertex(mod, surf, o[0], o[1], o[2], 0, 0, 0, o[3], o[4], 0, 0, o[6], o[7], o[8], o[9]);
		if (i >= 2)
			Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
		else if (i == 0)
			e0 = e2;
		e1 = e2;
	} // for

	// build normals (since they are not provided)
	Mod_BuildNormals(surf->num_firstvertex, surf->num_vertices, surf->num_triangles, mod->surfmesh.data_vertex3f, mod->surfmesh.data_element3i + 3 * surf->num_firsttriangle, mod->surfmesh.data_normal3f, true);

	ojpoly.texname[0] = 0;
	ojpoly.drawflags = 0;
	ojpoly.numvertices = 0;
}

WARP_X_ (CL_Mesh_UI)

void Draw_PolygonVertex2D (const float *v, const float *tc, const float *color3, float alpha)
{	
	if (ojpoly.maxvertices <= ojpoly.numvertices) {
		ojpoly.maxvertices = max(16, ojpoly.maxvertices * 2); // Double it
		
		ojpoly.vertexdata = (float *)Mem_Realloc(zonemempool, 
			ojpoly.vertexdata, ojpoly.maxvertices * sizeof(float[10]));
	}

	float *o = ojpoly.vertexdata + ojpoly.numvertices++ * 10;

	o[0] = v[0];
	o[1] = v[1];
	o[2] = 0;				// v[2]; // Avoid issue with accessing unrelated member of floatlist
	o[3] = tc[0];
	o[4] = tc[1];
	o[5] = tc[2];
	o[6] = color3[0];
	o[7] = color3[1];
	o[8] = color3[2];
	o[9] = alpha;
}


void Draw_Clip_Set (int left, int top, int width, int height)
{
	int x0 = bound(0, left, vid_conwidth.integer);
	int y0 = bound(0, top, vid_conheight.integer);
	int w0 = bound(0, left + width  - x0, (vid_conwidth.integer  - x0));
	int h0 = bound(0, top + height - y0, (vid_conheight.integer - y0));
	//rect_s r_new = RECT_SET
	DrawQ_SetClipArea(x0, y0, w0, h0);
}

void Draw_Clip_SetF (float left, float top, float width, float height)
{
	float x0 = bound(0, left, vid_conwidth.integer);
	float y0 = bound(0, top, vid_conheight.integer);
	float w0 = bound(0, left + width  - x0, (vid_conwidth.integer  - x0));
	float h0 = bound(0, top + height - y0, (vid_conheight.integer - y0));
	DrawQ_SetClipArea(x0, y0, w0, h0);
}

//float drawfill(vector position, vector size, vector rgb, float alpha, float flag)
void Draw_Fill (const vec3_t pos, const vec3_t size, const vec3_t rgb, float alpha, float flags)
{
	DrawQ_Pic(pos[0], pos[1], WHITE_TEXTURE_NULL, size[0], size[1], rgb[0], rgb[1], rgb[2], alpha, flags);
}

void Draw_Fill_XYWH_Color (int x, int y, int w, int h, const float *rgb)
{
	DrawQ_Pic(x, y, WHITE_TEXTURE_NULL, w, h, rgb[0], rgb[1], rgb[2], alpha_1_0, DRAWFLAG_NORMAL_0);
}

#define Draw_Fill_Rect_Color(pr, rgb) \
	DrawQ_Pic((pr)->left, (pr)->top, WHITE_TEXTURE_NULL, (pr)->width, (pr)->height, rgb[0], rgb[1], rgb[2], alpha_1_0, DRAWFLAG_NORMAL_0)

WARP_X_ (Draw_Box)

void Draw_Box_Int_XYWH (int left, int top, int width, int height, const float *backcolor)
{
	int right = left + width - 1, bottom = top + height - 1;

	Draw_Fill_XYWH_Color (left, top,	width,		1, backcolor);
	Draw_Fill_XYWH_Color (left, bottom,	width,		1, backcolor);
	Draw_Fill_XYWH_Color (left, top,		1, height, backcolor);
	Draw_Fill_XYWH_Color (right, top,		1, height, backcolor);
}

void Draw_Box_Int_XYWH1 (const float *backcolor, int left, int top, int width, int height)
{
	int right = left + width - 1, bottom = top + height - 1;

	Draw_Fill_XYWH_Color (left, top,	width,		1, backcolor);
	Draw_Fill_XYWH_Color (left, bottom,	width,		1, backcolor);
	Draw_Fill_XYWH_Color (left, top,		1, height, backcolor);
	Draw_Fill_XYWH_Color (right, top,		1, height, backcolor);
}


int DrawKit_Button_Except_Face (const rect_s *pr, drawbut_e style)
{
	switch (style) {
	default:
	case_break drawbut_thin_inset_0:
		Draw_Box_Int_XYWH1 (color3_white,		PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_gray_50,	RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1 (color3_form208,	RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
		return 2; // Yes it's a thin looking 2 (it's actually asymetrical too)

	case_break drawbut_thin_outset_1:
		Draw_Box_Int_XYWH1 (color3_gray_50,	PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_white,		RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1 (color3_form208,	RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
		return 2; // Yes it's a thin looking 2 (it's actually asymetrical too)

	case_break drawbut_normal_inset_2:
		// INSET DOUBLE - NO FILL
		Draw_Box_Int_XYWH1	(color3_white,   PRECT_SEND (pr));
		Draw_Box_Int_XYWH1	(color3_gray_50, RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1	(color3_form208, RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
		Draw_Box_Int_XYWH1	(color3_gray_25, RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
		return 2; // 2 on each side

	case_break drawbut_normal_outset_3:
		// OUTSET DOUBLE - NO FILL
		Draw_Box_Int_XYWH1	(color3_gray_25, PRECT_SEND (pr));
		Draw_Box_Int_XYWH1	(color3_form208, RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1	(color3_gray_50, RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
		Draw_Box_Int_XYWH1	(color3_white,   RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
		return 2;

	case_break drawbut_command_up_4:
		Draw_Box_Int_XYWH1 (color3_gray_25,	PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_white,	RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1 (color3_gray_50,	RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
		Draw_Box_Int_XYWH1 (color3_form208,	RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
		return 2;

	case_break drawbut_command_pressed_5:
		Draw_Box_Int_XYWH1 (color3_black,	PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_gray_50,	RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
		Draw_Box_Int_XYWH1 (color3_form208,	RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
		return 2;
	
	case_break drawbut_command_thick_up_6:
		Draw_Box_Int_XYWH1 (color3_black,	PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_gray_25,	PRECT_SEND (pr));
		Draw_Box_Int_XYWH1 (color3_white,	RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
		Draw_Box_Int_XYWH1 (color3_gray_50,	RECT_SEND_ADJUST(*pr,  2,  2, -4, -4));
		Draw_Box_Int_XYWH1 (color3_form208,	RECT_SEND_ADJUST(*pr,  2,  2, -5, -5));
		return 2;
	//case_break drawbut_command_thick_pressed_7:
	//	Draw_Rect (color3_black,	PRECT_SEND (pr));
	//	Draw_Rect (color3_gray_25,	PRECT_SEND (pr));
	//	Draw_Rect (color3_white,		RECT_SEND_ADJUST(*pr,  0,  0, -1, -1));
	//	Draw_Rect (color3_gray_50,	RECT_SEND_ADJUST(*pr,  1,  1, -2, -2));
	//	Draw_Rect (color3_form208,	RECT_SEND_ADJUST(*pr,  1,  1, -3, -3));
	//	return 2;

	}	
	return 0; // We don't know you ....
}

int DrawKit_Border_Ex (border_wall_e border_walls, qbool is_outset, qbool is_thin, 
			qbool is_border, qbool is_threed, qbool is_transparent, 
			const float *backcolor, const float *bordercolor, const rect_s *pr)
{
	// Border + Threed?
	if (is_border && border_walls) {
		// Draw black
		// THEN draw interior at x + 1 y = 0 h = h - 1 w = w - 2
		// |
		
		if (border_walls == border_Wall_topless_1_4_8_13) {
			rect_s r2 = *pr;
			Draw_Fill_Rect_Color (pr, bordercolor); // Like black
			
			r2.width -= 2;
			r2.height -= 1;				// Stomp the top  It's supposed to be h -=2, top +=1
			r2.top += 0; // Same		// Stomp the top black
			r2.left += 1;
			Draw_Fill_XYWH_Color (RECT_SEND(r2), backcolor);		// Like gray ---
			return 1; // i guess?
		}

		if (border_walls == border_Wall_bottomless_1_2_4_7) {
			rect_s r2 = *pr;
			Draw_Fill_Rect_Color (pr, bordercolor); // Like black
			
			r2.width -= 2;
			r2.height -= 1;		// Stomp the bottom instead
			r2.top += 1;			
			r2.left += 1;
			Draw_Fill_XYWH_Color (RECT_SEND(r2), backcolor); // Like black
			return 1; // i guess?
		}

	}

	if (is_border && is_threed)	{
		int sz;
		sz = DrawKit_Button_Except_Face (pr, (drawbut_e) (!(int)is_thin * 2 + (int)is_outset ) );
		
		if (is_transparent == false)
			Draw_Fill_XYWH_Color (RECT_SEND_ADJUST(*pr, 2, 2, -4, -4), backcolor); // Like black
		return sz;
	}
	
	// Border, not 3d
	if (is_transparent == false)
		Draw_Fill_Rect_Color (pr, backcolor); // Like black

	if (is_border == false)
		return 0; // No pixels for 0 px.

	Draw_Box_Int_XYWH (PRECT_SEND (pr), bordercolor); // Simple border.   Like black
	return 1; // Thickness of 1
}

#define RECT_BIG_LITTLE_CALC_CENTERED_TOP(BIGSIZE, LITTLESIZE) /* Find top for little rect to be centered in big one 20 - 10  = 5 */ \
	(     (int)(   (         ((BIGSIZE) - (LITTLESIZE)) + 0.5  )  / 2.0   ) /**/) // Ender

void Rect_Centered_Rect (/*modify*/ rect_s *pr_centered_out, const rect_s *pr_big, const rect_s *pr_little)
{
    int leftcent	= RECT_BIG_LITTLE_CALC_CENTERED_TOP (pr_big->width,  pr_little->width);
    int topcent		= RECT_BIG_LITTLE_CALC_CENTERED_TOP (pr_big->height, pr_little->height);
	PRECT_SET (pr_centered_out, pr_big->left + leftcent, pr_big->top + topcent, pr_little->width, pr_little->height);
}

// Not same as q_rint which rounds towards 0, this always rounds up
#define Math_Round(X) ((int)((X) + 0.5)) // 1.1 ---> 1  and 1.5 ---> 2 and 1.7 ---> 2

void DrawKit_Rect_Arrow (const rect_s *pr_area, arrow4_e arrowdir, int arrow_size, const vec3_t arrow_color3)
{
	int fatsize		= arrow_size;						// 7
    int thinsize	= Math_Round (arrow_size / 2.0);	// (7 / 2)  3.5 --> 4 
    int middle		= thinsize;

	switch (arrowdir) {
	rect_s r_arrow;
	case arrow4_left_0:		// Fall ...
	case arrow4_right_1: 
    
    //dp Spaced("FatSize", FatSize, "ThinSize", ThinSize)
		
			RECT_SET (r_arrow, 0, 0, thinsize, fatsize); // Thin horizontal, fatter vertical?
			Rect_Centered_Rect (&r_arrow, pr_area, &r_arrow);
		
			{	int cur = iif (arrowdir == arrow4_left_0, 0, thinsize - 1); // relative x
				int dir = iif (arrowdir == arrow4_left_0, 1, -1);
				int x; for (x = 0; x < thinsize; x ++, cur += dir) {
					Draw_Fill_XYWH_Color (r_arrow.left + x, r_arrow.top + middle - (cur + 1), 1, ((cur+1) * 2) - 1, arrow_color3);
				}
			}

			break;
			
	case arrow4_up_2:		// Fall ...
	case arrow4_down_3:
        
			RECT_SET (r_arrow, 0, 0, fatsize, thinsize); // Thin horizontal, fatter vertical?
			Rect_Centered_Rect (&r_arrow, pr_area, &r_arrow);
		
			{	int cur = iif (arrowdir == arrow4_up_2, 0, thinsize - 1); // relative x
				int dir = iif (arrowdir == arrow4_up_2, 1, -1);
				int y; for (y = 0; y < thinsize; y ++, cur += dir) {
					Draw_Fill_XYWH_Color (r_arrow.left + middle - (cur + 1), r_arrow.top + y, ( (cur+1) * 2) - 1, 1, arrow_color3);
				}
			}
    
	}
    
}


//int j = maxlen; // Number of characters of the string that render
// Baker: I hope note above is wrong.
float Draw_StringNWidth (dp_font_t *dpf, int ncount, ccs *s, float fontsize)
{
	if (ncount == 0)
		return 0;
	size_t maxlen = ncount;

	float fwidth = DrawQ_TextWidth_UntilWidth_TrackColors_Scale(
		s, 
		&maxlen, fontsize, fontsize, scale_1_0, scale_1_0, q_outcolor_null, q_ignore_color_codes_false, 
		dpf, /*maxwidth*/ 1000000000)
		;

	//int j = maxlen; // Number of characters of the string that render
	return fwidth;
}

int Draw_StringIntoCountAt(dp_font_t *dpf, ccs *s, float fontsize, int maxwidth, roundtype_e roundtype)
{
	//int slen = strlen(s);
	size_t maxlen = 0;

	float fwidth = DrawQ_TextWidth_RoundType (
		s, 
		&maxlen, fontsize, fontsize, scale_1_0, scale_1_0, q_outcolor_null, q_ignore_color_codes_false, 
		dpf, /*maxwidth*/ maxwidth, (int)roundtype)
		;

	//int j = maxlen; // Number of characters of the string that render
	//return fwidth;
	return maxlen;
}

