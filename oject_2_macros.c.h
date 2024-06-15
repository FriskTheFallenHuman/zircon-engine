// oject_2_macros.c.h

#define RECT_MOVE_XY(rect, xys) \
	(rect).left += (xys).x, (rect).top += (xys).y

#define EXO___

#define Form_Get(f,k)				oject_s *f = k->fctrl
#define Form_QueueRefresh(f)		f->frm.refreshneeded ++
#define Form_QueueDotsRefresh(f)	f->frm.refreshdots ++

#define Oject_Have_Focus(k) \
	((k)->fctrl->frm.kfocused == (k)) // Ender

#define TXT_CURSOR_AT(k) \
	((k)->txta.cursor_pos + (k)->txta.cursor_length)

#define TXT_STRLEN(k) \
	((int)strlen((k)->text_a))

// This the highlight left.
#define TXT_SELLEFT(K) \
	((K)->txta.cursor_length >= 0 ? (K)->txta.cursor_pos : TXT_CURSOR_AT(K))


// This the highlight right.
#define TXT_SELRIGHT(K) \
	((K)->txta.cursor_length >= 0 ? TXT_CURSOR_AT(K) : (K)->txta.cursor_pos)

// This is number of chars selected.
#define TXT_SELCOUNTABS(K) \
	(abs((K)->txta.cursor_length))

#define TXT_CONTENTS_WIDTH_PX(k) \
	(k)->r_scrollport.width

#define TXT_CURSOR_LEFT_PX(k) \
	(k)->txta._sel_left_px

#define TXT_CURSOR_LENGTH_PX(k) \
	(k)->txta._sel_len_px



#define LENGTH___



#define LIFE_FREE___			// Marks something important freed
#define LIFE_ALLO___			// Marks something important allocated
#define LIFE_CLEAR___			// Marks something important reset.

#define NPA__	// DANGEROUS .. manually free them!
#define PPA__	// Property, destroy object works ALL properties and destroys


#define RECT_SEND_ISO_CONTRACT(rect, ISOAMT) \
	(rect).left + (ISOAMT), (rect).top + (ISOAMT), (rect).width - 2 * (ISOAMT), (rect).height - 2 * (ISOAMT) // Ender

#define RECT_SET_CONTRACT(rect,r, ISOAMT) \
	(rect).left		= (r).left + (ISOAMT), \
	(rect).top		= (r).top  + (ISOAMT), \
	(rect).width	= (r).width + (- 2 * (ISOAMT)), \
	(rect).height	= (r).height + (- 2 * (ISOAMT)) // Ender

#define RECT_SET_ADJUST(RDST,rect, L,T,W,H)  (RDST).left   = ((rect).left + (L)),		\
											 (RDST).top    = ((rect).top + (T)),		\
											 (RDST).width  = ((rect).width + (W)),		\
											 (RDST).height = ((rect).height + (H))

#define RECT_SEND_ADJUST(rect,L,T,W,H)	((rect).left + (L)), ((rect).top + (T)), ((rect).width + (W)), ((rect).height + (H))

// unused
#define RECT_MOVE_LEFTOP(rect, r2) \
	(rect).left += (r2).left, (rect).top += (r2).top

#define oject_s			struct _oject_s
#define oject_ref_s		struct _oject_s			// Marked differently like container
#define oject_array_s	struct _oject_array_st
#define	things_s		struct _things_s

#ifdef _DEBUG
#define varpi(x) Con_PrintLinef(STRINGIFY(x) " = %d", x)
#define varpf(x) Con_PrintLinef(STRINGIFY(x) " = " FLOAT_LOSSLESS_FORMAT, x)
#endif

#define K_GET_RELATIVE_XY(x,y) \
	int x = x_frm - k->fp.dr_footprint.unclipped.left; \
	int y = y_frm - k->fp.dr_footprint.unclipped.top // Ender

#define Baker_Array_Erase(ba) \
	baker_array_ptr_erase ((baker_array_s *) ba)

#define Baker_Array_NewIndex(ba) \
	baker_array_ptr_new_item_index((baker_array_s *)ba);

// This exists the standardize the loop
#define for_each_form_all_controls(NN,F,KK)\
	for (int NN = 0; NN < F->frm.controls_a->numitems; NN ++) { \
		oject_s *KK = F->frm.controls_a->pointers_array[NN]; // Ender

WARP_X_CALLERS_ ()
WARP_X_CALLERS_ (Form_Draw Object_Refresh_Recursive, Object_Draw_Recursive_Not_Form, Object_Dump_Recursive)
#define for_each_form_control_with_container_as_no_servants(NN,F,KCAN,KK)\
	for (int NN = 0; NN < F->frm.controls_a->numitems; NN ++) { \
		oject_s *KK = F->frm.controls_a->pointers_array[NN]; \
		if (KK->container != KCAN) continue; \
		if (KK->servant_owner) continue;  // Ender


#define for_each_form_servant_owner_is_k(NN,F,KSERVANT_OWNER,KK)\
	for (int NN = 0; NN < F->frm.controls_a->numitems; NN ++) { \
		oject_s *KK = F->frm.controls_a->pointers_array[NN]; \
		if (KK->servant_owner != KSERVANT_OWNER) continue;


WARP_X_CALLERS_ (Oject_Hit_Drawn)
#define for_each_form_drawn_reverse(NN,F,KK) \
	for (int NN = F->frm.drawn_list_a->numitems - 1; NN >= 0; NN --) { \
		oject_s *KK = F->frm.drawn_list_a->pointers_array[NN];

#define for_each_form_drawn_forward(NN,F,KK) \
	for (int NN = 0; NN < F->frm.drawn_list_a->numitems; NN ++) { \
		oject_s *KK = F->frm.drawn_list_a->pointers_array[NN]; // Ender


#define for_each_end }

#define Draw_Rect(PR,BACKCOLOR,BACKALPHA) \
	DrawQ_Fill (PRECT_SEND(PR), VECTOR3_SEND(BACKCOLOR), BACKALPHA, DRAWFLAG_NORMAL_0)

#define Draw_Rect_XYWH(X,Y,W,H,BACKCOLOR,BACKALPHA) \
	DrawQ_Fill (X,Y,W,H, VECTOR3_SEND(BACKCOLOR), BACKALPHA, DRAWFLAG_NORMAL_0)

void Draw_Box (const rect_s *pr, int boxwidth, const vec3_t color, float alpha)
{
	//DrawQ_Fill (pr->left,pr->top,pr->width,boxwidth, (color)[0], (color)[1], (color)[2], alpha, DRAWFLAG_NORMAL_0);

	Draw_Rect_XYWH (pr->left, pr->top,                       pr->width, boxwidth, color, alpha); // TOP
	Draw_Rect_XYWH (pr->left, PRECT_BOTTOMOF(pr) - boxwidth, pr->width, boxwidth, color, alpha); // B

	Draw_Rect_XYWH (pr->left,
						pr->top + boxwidth, boxwidth, pr->height - boxwidth * 2, color, alpha); // L
	Draw_Rect_XYWH (PRECT_RIGHTOF(pr) - boxwidth,
						pr->top + boxwidth, boxwidth, pr->height - boxwidth * 2, color, alpha); // R
}

void Draw_Rect_Adjust4F (const frect *prf, const vec3_t color, float alpha)
{
	frect r = { RECT_SEND_EXPANDED(*prf, 4) };
	DrawQ_Fill (RECT_SEND(r), VECTOR3_SEND(color), alpha, DRAWFLAG_NORMAL_0);
}

void Draw_Rect_Adjust4 (const rect_s *pr, const vec3_t color, float alpha)
{
	rect_s r = { RECT_SEND_EXPANDED(*pr, 4) };
	DrawQ_Fill (RECT_SEND(r), VECTOR3_SEND(color), alpha, DRAWFLAG_NORMAL_0);
}

#define Draw_Clip_Set_Rect(PR) \
	Draw_Clip_Set ((PR)->left, (PR)->top, (PR)->width, (PR)->height)

// cachepic_s *p = Draw_CachePic_Flags (k->image_name_a, CACHEPICFLAG_NOTPERSISTENT)
#define Draw_ImageCachePic(PR,CACHEPIC,BACKCOLOR,BACKALPHA) \
	DrawQ_Pic((PR)->left, (PR)->top, CACHEPIC, (PR)->width, (PR)->height, VECTOR3_SEND(BACKCOLOR), BACKALPHA, DRAWFLAG_NORMAL_0);

#define Draw_String_XY(x,y,S,FONTSIZE,FORECOLOR,FOREALPHA) \
		DrawQ_String_Scale (\
			x,\
			y,\
			S,\
			maxlen_0, \
			FONTSIZE,\
			FONTSIZE,\
			scale_1_0,\
			scale_1_0, \
			VECTOR3_SEND(FORECOLOR),\
			FOREALPHA, \
			DRAWFLAG_NORMAL_0, \
			OUTCOLOR_NULL, \
			ignorecolorcodes_false,		\
			zdev_dpfont\
		) // Ender

#define Draw_String(PR,S,FONTSIZE,FORECOLOR,FOREALPHA) \
		DrawQ_String_Scale (\
			(PR)->left,\
			(PR)->top,\
			S,\
			maxlen_0, \
			FONTSIZE,\
			FONTSIZE,\
			scale_1_0,\
			scale_1_0, \
			VECTOR3_SEND(FORECOLOR),\
			FOREALPHA, \
			DRAWFLAG_NORMAL_0, \
			OUTCOLOR_NULL, \
			ignorecolorcodes_false,		\
			zdev_dpfont\
		) // Ender

#define Draw_StringXY(x,y,S,FONTSIZE,FORECOLOR,FOREALPHA) \
		DrawQ_String_Scale (\
			x,\
			y,\
			S,\
			maxlen_0, \
			FONTSIZE,\
			FONTSIZE,\
			scale_1_0,\
			scale_1_0, \
			VECTOR3_SEND(FORECOLOR),\
			FOREALPHA, \
			DRAWFLAG_NORMAL_0, \
			OUTCOLOR_NULL, \
			ignorecolorcodes_false,		\
			zdev_dpfont\
		) // Ender

// Lists of strings and such - we aren't doing doing huge lists like
// triangles or vertexes, so 4096 is a bit large.


#define DEF_COMPAT_ARRAY_STRUCT(MYARRAYDATATYPE_S,MYPTRTYPE) \
	typedef struct _ ## MYARRAYDATATYPE_S {											\
		int					batchsize;		/* If not specified, defaults 128 */	\
		int					numitems;												\
		int					maxitems;												\
		MYPTRTYPE			**pointers_array;			/* */						\
		size_t				datasize;												\
	} MYARRAYDATATYPE_S // ENDER

// Draw_Clip_Clear will flushui draw everytime, does not check to see if clip area was set.
// But looks low cost and immediately exits.
#define Draw_Clip_Clear() \
		DrawQ_ResetClipArea ()


#define Word_Check_Fail_Message(s) \
	if (!*wordray) { \
		Con_PrintLinef (CON_ERROR s);\
		break; \
	} // Ender

#define OJ_Mouse1_Down_Save_Origin(K) \
	(K)->ui.mouse_1_down = true; \
	(K)->ui.mouse_down_origin.x = x; \
	(K)->ui.mouse_down_origin.y  = y; \
	(K)->ui.mouse_move_threshold_met = false  // Ender

// Sets threshold met to result. No mouse 1 requirement here.
#define OJ_Mouse1_Threshold_Pass(K, X, Y)						\
	( (K)->ui.mouse_move_threshold_met \
	|| ( /*assign*/(K)->ui.mouse_move_threshold_met = ( \
	fabs((X) - (K)->ui.mouse_down_origin.left) >= UI_SELECTION_TOLERANCE_PX_2  \
	|| \
    fabs((Y) - (K)->ui.mouse_down_origin.top)  >= UI_SELECTION_TOLERANCE_PX_2)  )) // Ender


//#define OJ_Mouse1_Hit_Canvas(K)									(mouseindex == MOUSEIDX_1 && RECT_HIT ((K)->r.canvas.clipped, x, y))
//#define OJ_Mouse1_Hit_TextArea(K)								(mouseindex == MOUSEIDX_1 && (K)->ui.mouse_over_text_area)
//#define OJ_Mouse1_Move_Is_On_Canvas_Pass_Threshold(K, X, Y)		((K)->ui.mouse_1_down && OJ_Mouse1_Threshold_Pass(K, X, Y) && RECT_HIT((K)->r.canvas.clipped, X, Y))
//
//#define OJ_Mouse1_Check_Release_Thump_Off(K, MOUSEINDEX)		if ((MOUSEINDEX) == MOUSEIDX_1) (K)->ui.mouse_1_down = false, (K)->ui.thump_wanted = false
//#define OJ_Mouse1_Thump_Reset(K)								(K)->ui.thump_last_time = mainus.event_time, (K)->ui.thump_wanted = false, (K)->ui.thump_next_time = 0  // Kill thumping unless we need more.
//#define OJ_Is_Mouse1_Release(K, MOUSEINDEX)						( (K)->ui.mouse_1_down && (MOUSEINDEX) == MOUSEIDX_1)
//
//
//
//#define OJ_Mouse1_Move_Pass_Threshold(K, X, Y)					((K)->ui.mouse_1_down && OJ_Mouse1_Threshold_Pass(K, X, Y) )
//
//#define OJ_Mouse1_Threshold_Reset(K, X, Y)						(K)->ui.mouse_move_threshold_met = false, (K)->ui.mouse_down_origin.left = x, (K)->ui.mouse_down_origin.top = y
