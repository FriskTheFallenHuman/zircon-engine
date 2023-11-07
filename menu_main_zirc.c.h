// menu_main_zirc.c.h

WARP_X_ (M_Main_Draw )

//	Mat4_Identity_Set	(&mm.projection);
//	switch (wdo->appar->screen_portrait) {
//	default:			Mat4_Rotate			(&mm.projection, 90, 0, 0, 1); // Well, it's what it says.
//						Mat4_Perspective	(&mm.projection, M_FOV_WANTED_45_DEGREES /*fov degrees*/, wdo->received.client_canvas.width / wdo->received.client_canvas.width, M_ZNEAR_1, M_ZFAR_8193);
//
//	case_break false:	Mat4_Perspective	(&mm.projection, M_FOV_WANTED_45_DEGREES /*fov degrees*/, wdo->received.client_canvas.width / wdo->received.client_canvas.width, M_ZNEAR_1, M_ZFAR_8193);
//	}
//
//	// PHYSICS
//
//	Mat4_Identity_Set	(&mm.view);
//		Mat4_Rotate			(&mm.view, -90, 1, 0, 0);	    // put Z going up
//		Mat4_Rotate			(&mm.view, -mm.camera.pitch, 1, 0, 0);
//		Mat4_Rotate			(&mm.view, -mm.camera.roll,  0, 1, 0);
//		Mat4_Rotate			(&mm.view,  mm.camera.yaw,   0, 0, 1);
//
//		Mat4_Translate		(&mm.view, -mm.camera.x, -mm.camera.y, -mm.camera.z); // Final?
//
//	{ int n; for (n = 0; n < mm.entities_count; n ++) { // PHYSICS
//		mm_entities_t *e = &mm.entities[n];
//
//		float my_degrees = angle_maybe_wrap (mm.sun.yaw + e->itemdata /* my_start_degrees */, NULL);
//		float my_radians = DEGREES_TO_RADIANS(my_degrees);
//
//		VectorSet (e->location.position3, sin(my_radians) * mm.sun.width/2.0 + mm.sun.x, cos(my_radians) * mm.sun.width/2.0 + mm.sun.y, 0);
//		e->location.yaw = -my_degrees; // Perpendicular to center.
//
////		alert ("%3.1f: #%d %3.1f %3.1f %3.1f",  m.sun.yaw, n, e->location.x, e->location.y, e->location.z);
//
//		Mat4_Copy (&e->modelview, &mm.view);
//			Mat4_Translate (&e->modelview, e->location.x, e->location.y, e->location.z);
//			if (e->location.pitch)	Mat4_Rotate (&e->modelview,  e->location.pitch, 1, 0, 0);
//			if (e->location.roll)	Mat4_Rotate (&e->modelview, -e->location.roll, 0, 1, 0);
//			if (e->location.yaw)	Mat4_Rotate (&e->modelview,  e->location.yaw, 0, 0, 1);
//	}}
//
//
//	// DRAW
//	GL_State_Set (&wdo->request.rstate_custom, &wdo->platus.rstate_real); // right after makecurrent
//
//	glMatrixMode(GL_PROJECTION);	glLoadMatrixf (mm.projection.m16); // Select The Projection Matrix
//	glMatrixMode(GL_MODELVIEW);
//
//	GL_State_Set_Easy	(RD_TEXTURES_OFF | RD_VERT_ARRAY_ON | RD_CULLFACE_OFF | RD_BLEND_OFF, NULL /* no color */, &wdo->platus.rstate_real);
//
//	{ int n; for (n = 0; n < mm.entities_count; n ++) {
//		mm_entities_t *e = &mm.entities[n];
//		glLoadMatrixf			(e->modelview.m16);	   // Select The Modelview Matrix
//		GL_State_Set_Easy		( RD_COLOR_SUPPLIED, e->colorx.c4, &wdo->platus.rstate_real);
//		glVertexPointer			(3, GL_FLOAT, 0, faceVerticesf);		
//		glDrawArrays			(GL_TRIANGLE_FAN, 0, 4 /* 4 to a face */);
//	}}
//	////wdo->platus.dirty_count ++;  This sucks because we already cleared.
//
//typedef struct {
//	l
//} ent_s;

static void M_Main_Draw_Zirc (void)
{
	float w, h;
	if (1) { //!vid_conscale_auto.value) { // PUSH
		w = vid_conwidth.value;
		h = vid_conheight.value;
		Cvar_SetValueQuick (&vid_conwidth,  vid.width);
		Cvar_SetValueQuick (&vid_conheight, vid.height);
	}


	//cachepic_t	*p;
	M_Background(1200, 768);
	//M_DrawPic (16, 4, "gfx/qplaque");
	//p = Draw_CachePic ("gfx/zirc/main");
	M_DrawPic ( 10, 4, CPC("gfx/zirc/main"), NO_HOTSPOTS_0, NA0, NA0);

	//p = Draw_CachePic ("gfx/zirc/main2");
	M_DrawPic ( 10, 200, CPC("gfx/zirc/main2"), NO_HOTSPOTS_0, NA0, NA0);

	// take square ... 3d space coords .. make 2 space coords

		//M_DrawPic (72, 32, "gfx/mainmenu");

//	f = (int)(realtime * 10)%6;

	//M_DrawPic (54, 32 + m_main_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1));



	//if (scr_scaleauto.value) { // POP
	//		if (in_range_beyond (0.125, scr_scaleauto.value, 16) == false) {
	//		Cvar_SetValueQuick(&scr_scaleauto, bound(0.125, scr_scaleauto.value, 16));

		Cvar_SetValueQuick (&vid_conwidth,  w);
		Cvar_SetValueQuick (&vid_conheight, h);

	//} else { // 


	//}


	

}


