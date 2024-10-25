// keys_other_select.c.h

extern conbuffer_t con;
#define CONBUFFER_LINES(buf, i) (buf)->clines[((buf)->lines_first + (i)) % (buf)->maxlines]

WARP_X_ (conbuffer_t Key_Event)
int RealRowCollide (int irows_up, int lastrowvisible)
{
	int mask_mustnot_dev = (developer.integer > 0) ? 0 : CON_MASK_DEVELOPER;

	// Baker: We are going to avoid the wraparound stuff for now.
	int physical_rows_up = 0;
	for (int current_row = lastrowvisible; current_row >= 0; current_row --) {
		con_lineinfo_t *curline = &CONBUFFER_LINES(&con, current_row);
		//const char *s_text = curline->pstart;
		int drawn_numrows = curline->line_num_rows_height;
		int Con_LineHeight(int lineno);

		if (Have_Flag(curline->mask, mask_mustnot_dev)) {
			continue;
		}

		int physical_rows_up_here =  physical_rows_up;
		int physical_rows_up_higher  =  physical_rows_up + (drawn_numrows - 1);

		// IN RANGE? GET OUT!
		if (in_range (physical_rows_up_here, irows_up, physical_rows_up_higher)) {
			return current_row;
		}

		physical_rows_up += drawn_numrows;
	} // for

	return -1; // This can happen sometimes?
}

WARP_X_ (KeyDest_Set Con_ToggleConsole Key_Event consel_t CON_MASK_DEVELOPER)

static void Consel_MouseDown (consel_t *m, float mx, float my, int row, int col)
{
	int was_a_selection = m->a.drag_state == drag_state_drag_completed_3;
	memset (&m->a, 0, sizeof(m->a));

	// MOUSE DOWN FOR FIRST TIME
	c_assert_ (m->a.drag_state == drag_state_none_0);

	m->a.drag_state				= drag_state_awaiting_threshold_1; // CHANGE TO AWAITING
	DebugPrintLinef ("Mouse down at virtual x %f y %f", mx, my);
	m->a.mousedown_fx			= mx;
	m->a.mousedown_fy			= my;
	m->a.mousedown_time			= Sys_DirtyTime ();

	m->a.mousedown_row_index	= row;
	m->a.mousedown_col_index	= col;
	m->a.prior_was_selection	= was_a_selection;
}




static void Consel_MouseUp_After_Dragging (consel_t *m, float mx, float my, int row, int col)
{
	c_assert_ (m->a.drag_state == drag_state_dragging_2);
	m->a.drag_state = drag_state_drag_completed_3; // CHANGE TO COMPLETED
	DebugPrintLinef ("To completed 3");
}

static void Consel_MouseMove_Extend_Selection (consel_t *m, float mx, float my, int row, int col)
{
//	if (row < 0)
//		int j = 4;
	m->a.mousedown_end_row_index = row;
	m->a.mousedown_end_col_index = col;
	DebugPrintLinef ("Extend select rc %d %d to rc %d %d", m->a.mousedown_row_index, m->a.mousedown_col_index, m->a.mousedown_end_row_index, m->a.mousedown_end_col_index);
}

WARP_X_ (Con_DrawConsoleLine_Num_Rows_Drawn)

static void Consel_MouseMove_Check_Threshold (consel_t *m, float mx, float my, int row, int col)
{
	c_assert_ (m->a.drag_state == drag_state_awaiting_threshold_1);

	float delta_x			= m->a.mousedown_fx - mx;
	float delta_y			= m->a.mousedown_fy - my;
	int did_hit_threshold	= (abs(delta_x) > THRESHOLD_5) ||  (abs(delta_y) > THRESHOLD_5);

	if (did_hit_threshold) {
		m->a.drag_state = drag_state_dragging_2; // CHANGE TO DRAG
		DebugPrintLinef ("Drag ACTIVATED mx my %f %f", mx, my);
	}
	DebugPrintLinef ("MouseMove No Drag deltax was %f deltay was %f", delta_x, delta_y);
}

//
// Public Facing Functions
//

// Baker: Right now lose focus isn't calling this and maybe it shouldn't?
void Consel_MouseReset (const char *reason)
{
#ifdef CONFIG_MENU
	Vid_Cursor_Reset ();
#endif // #ifdef CONFIG_MENU
	consel_t *m = &g_consel;
	memset (&m->a, 0, sizeof(m->a));
	DebugPrintLinef ("Consel_MouseReset");
}


WARP_X_CALLERS_ (Key_EventIN_Move)
int Consel_Key_Event_Check_Did_Action (int is_down)
{
	float	canvas_x_multiplier		= g_consel.draww.conscalewidth / vid.width;		// 640 / 1920 = 0.3
	float	canvas_y_multiplier		= g_consel.draww.conscaleheight / vid.height;		// 480 / 1080 = 0.2
	float	mouse_canvas_x			= in_windowmouse_x * canvas_x_multiplier;
	float	mouse_canvas_y			= in_windowmouse_y * canvas_y_multiplier;
	float	canvas_beyond_bottom	= g_consel.draww.draw_row_top_pixel_y + con_textsize.value;
	int		is_off_canvas			= mouse_canvas_y >= canvas_beyond_bottom; //
	float	rows_up					= ((canvas_beyond_bottom /*378*/ - mouse_canvas_y /*334*/) / con_textsize.value);
	int		irows_up				= (int) rows_up;
	//int	irow_frac				= rows_up -  irows_up; // We have 0.0 to 1.00
	int		real_row_collide		= is_off_canvas ? -2 : RealRowCollide (irows_up, g_consel.draww.draw_row_last_index);

	consel_t *m = &g_consel;

	if (is_down && isin2 (m->a.drag_state, drag_state_none_0, drag_state_drag_completed_3)) {
		if (is_off_canvas && real_row_collide >= 0) {
			//Vid_SetWindowTitlef ("Offgrid at %f bot is %f", mouse_canvas_y, canvas_beyond_bottom);
			return TREAT_IGNORE_0; // IGNORE IT
		}

		// MOUSE DOWN - NOT OFF CANVAS - START!
		Consel_MouseDown (m, mouse_canvas_x, mouse_canvas_y, real_row_collide, /*column*/ 0);
		return TREAT_CONSUMED_MOUSE_ACTION_1;
	}

	if (is_down == false && isin2 (m->a.drag_state, drag_state_none_0, drag_state_awaiting_threshold_1)) {
		// MOUSE UP - DID NOT MOVE FAR ENOUGH - TREAT LIKE MOUSE UP

		if (m->a.prior_was_selection) {
			DebugPrintLinef ("TREAT_IGNORED MOUSEUP");
			Consel_MouseReset ("Mouseup with previous selection that failed threshold");
			return TREAT_CONSUMED_MOUSE_ACTION_1; // Allow mouse action is simply clear the selection
		}
		DebugPrintLinef ("TREAT_MOUSEUP_2");
		return TREAT_MOUSEUP_2; // Treat like a mouse up
	}

	if (is_down == false && m->a.drag_state == drag_state_dragging_2) {
		Consel_MouseUp_After_Dragging (m, mouse_canvas_x, mouse_canvas_y, real_row_collide, /*column*/ 0);
		return TREAT_CONSUMED_MOUSE_ACTION_1;
	}

	if (is_off_canvas) {
		//Vid_SetWindowTitlef ("Something weird off-canvas down = %d ds = %d", is_down, m->a.drag_state);
		return TREAT_IGNORE_0; // IGNORE IT
	}

	//Vid_SetWindowTitlef ("Something weird on-canvas down = %d ds = %d", is_down, m->a.drag_state);
	return TREAT_CONSUMED_MOUSE_ACTION_1;
}
WARP_X_CALLERS_ (IN_Move)
void Consel_MouseMove_Check (void)
{
	extern cvar_t vid_mouse_show_coords;

	if (vid_mouse_show_coords.integer) {
		Vid_SetWindowTitlef ("%6.1f, %6.1f", in_windowmouse_x, in_windowmouse_y);
	}

	consel_t *m = &g_consel;

	// If not console or not dragging or completed get out.
	int is_check_move =key_consoleactive &&
		isin2(m->a.drag_state, drag_state_awaiting_threshold_1, drag_state_dragging_2);
	if (false == is_check_move)
		return;

	DebugPrintLinef ("IN_Move: Check drag = %d", m->a.drag_state);

	float	canvas_x_multiplier		= g_consel.draww.conscalewidth / vid.width;		// 640 / 1920 = 0.3
	float	canvas_y_multiplier		= g_consel.draww.conscaleheight / vid.height;		// 480 / 1080 = 0.2
	float	mouse_canvas_x			= in_windowmouse_x * canvas_x_multiplier;
	float	mouse_canvas_y			= in_windowmouse_y * canvas_y_multiplier;
	float	canvas_beyond_bottom	= g_consel.draww.draw_row_top_pixel_y + con_textsize.value;
	int		is_off_canvas			= mouse_canvas_y >= canvas_beyond_bottom; //
	float	rows_up					= ((canvas_beyond_bottom /*378*/ - mouse_canvas_y /*334*/) / con_textsize.value);
	int		irows_up				= (int) rows_up;
	//int	irow_frac				= rows_up -  irows_up; // We have 0.0 to 1.00
	int		real_row_collide		= is_off_canvas ? -2 : RealRowCollide (irows_up, g_consel.draww.draw_row_last_index);

	if (m->a.drag_state == drag_state_awaiting_threshold_1) {
		Consel_MouseMove_Check_Threshold (m, mouse_canvas_x, mouse_canvas_y, real_row_collide, /*column*/ 0);
	}

	if (m->a.drag_state == drag_state_dragging_2) {
		if (is_off_canvas == false && real_row_collide >= 0)
		Consel_MouseMove_Extend_Selection (m, mouse_canvas_x, mouse_canvas_y, real_row_collide, /*column*/ 0);
	}
}
