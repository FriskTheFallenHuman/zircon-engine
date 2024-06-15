// oject_3_structs_oject.c.h

// Zebras -- top most things like popups or tooltips, zebras are exclusive to form (right?)


typedef struct _oject_s {
// Assume all objects have all properties

//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////
	oject_ref_s			*fctrl;				// Form fctrl is SELF!
	oject_ref_s			*container;			// Only a form has this NULL
	things_s			*po;
	oject_ref_s			*servant_owner;		// Unfocusable contained child (scrollbar, etc.)
	int					in_recursion;		// Setting a property that can trigger a chain.

	viewcalc_s			viewcalc;			// Listbox visibility info
	viewcalc_s			viewcalcx;			// Horiz

	struct {
			int			startcol;
			int			beyondcol;
			int			headerrowheight;
			int			columnsallwidths;
			int			fakecolumnwidth;
			int			is_phantom;
			int			phantom_width;

			// Part of footprint
			rect_s		r_header;											
			rect_s		r_header_clipped;
	} xl;

	struct {								// FIELDS: x, y, width, height
		    rect_s		relative_rect;		// Container interior relative
PPA__		char		*name_a;			// FIELDS: name
			int			creation_idx;		// Really used! For unnamed controls!  For controls only, assigned.  For form, given.

			xy_pairi_s	dot_origin;			// We only tell the dot a size.  It decides what to do with that information.
	} cm;

	// Z-Ordered Servants That Share our footprint, and do not use our interior canvas 
	// and draw last for the owner control. (ex: scrollbar for a listbox)
	// These can never receive focus, but can obtain mouse focus by hold clicking them.  
	// Therefore they cannot highlight.
	struct {
		oject_ref_s		*kscrollx;
		oject_ref_s		*kscrolly;
		oject_ref_s		*kcolumnresize_line;
		oject_ref_s		*kcontextmenu;
		oject_ref_s		*kdots[CM_DOTCOUNT_COUNT_9];
	} servo;

	struct {
		mousepointer_e	mousepointer;		// Form sees mousemove will use this as cursor if over control.
											// Probably also if off footprint if mouse1 pressed.
		//int			is_mouse_1_down;	// May 4 2024 - Right now, we only respond to mouse 1 + wheel
		xy_pairi_s		mouse_down_origin;	// 
		int				is_move_thresh_met;	// 
		timey_dbl_t		mouse_down_time;
		int				mouse_down_thingi;
		int				mouse_down_valuei;
		rect_s			mouse_down_rect;
		rect_s			mouse_down_shadow_rect;
	} ui;

//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////
	struct {
		oject_ref_s		*kfocused;			// Focused control
NPA__	oject_array_s	*controls_a;		// Everything is in this, EXCEPT NOT THE FORM.
NPA__	oject_array_s	*drawn_list_a;		// List of drawn controls.  But NOT THE FORM.

		oject_s			*k_mousehook;		// This is in a mousedown
		mouse_hook_fn_t	hook_fn;
		timey_dbl_t		hook_nextthink;		// Fire when

		qbool			disable_clipping;	// CLIPPING_ON, defaults off.
		oject_s			*kmousedown;		// A mousedown means the mouseup/mousemove owned by this.

		int				editmode;
		oject_s			*keditmode;

		int				refreshneeded;
		int				refreshdots;

		oject_s			*kcontextoldfocus;	// For escape / enter


	} frm;

	// ALL METRICS IN FP ARE IN REAL SCREEN
	
	// .clipped is for SetClip/Scissor Area only and footprint is used for mouse click collision
	// .unclipped is for all draws.

	WARP_X_ (Object_Refresh_Recursive Object_Draw_Recursive_Not_Form)
#define ORR
	rect_s	ORR			r_halo;				// Footprint + a fixed amount. BOXWIDTH_2
	rect_s	ORR			r_halo_clipped;		//

	// r_interior is set in Object_Refresh_Recursive (k->r_screen = k->cm.relative_rect;, k->r_screen.left	+= k->container->r_interior.left;)
	rect_s	ORR			r_screen;			// Here is your r_drawn
	rect_s	ORR			r_screen_clipped;

	// r_interior is set in Object_Refresh_Recursive (SET___ k->r_interior = k->r_screen;)
	rect_s	ORR			r_interior;			// The "interior" (footprint less border) -- does Object_Refresh set?
	rect_s	ORR			r_interior_clipped;
	rect_s				r_scrollport;		// height is row_height_px * members count

//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////
	struct {
		rect_s			r_textarea;			// Who uses?
		int				force_draw_cursor;	// Occurs 1 frame on any cursor or key action.
		int				cursor_pos;			//
		int				cursor_length;		// Non-zero is a selection.  Negative is right to left selection.
		int				_cursor_start;		// smallest of cursor_pos and cursor_pos + cursor_length
		int				_cursor_beyond;		// _cursor_start + abs(cursor_length)
		int				_cursor_length_abs;	// _cursor_start + abs(cursor_length)
		int				_sel_left_px;
		int				_sel_len_px;
		int				_sel_real_px;
		vec3_t			_antiforecolor;		// XOR color
		vec3_t			_antibackcolor;
		rect_s			r_cursor;			// Draw fill cursor area.  Size is ONE if length 0.
		rect_s			r_cursor_highlight; // Draw fill cursor area.  Size is ONE if length 0.
		NPA__ char		*seg1_a;
		int				seg1_left_px;
		NPA__ char		*seg2_a;
		int				seg2_left_px;
		NPA__ char		*seg3_a;
		int				seg3_left_px;
		int				is_viewported_x;	// Text length
		int				stringwidthi_px;	// ceil
	} txta;

// Q: How are these freed?
// A: Property destroy unfrees every property by vartype.

	// Keep this alpha and the names as close to a match
//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////

// REAL PROPERTIES ONLY ...
	int					alignment;			WARP_X_(alignment_e)
	int					autosize;
	float				backalpha;
	vec3_t				backcolor;
	//vec3_t			backcolordisabled;
	vec3_t				backcolorselected;
	PPA__ char          *caption_a;			// image how ...
	float               cellpaddingxpct;	// percent of fontsize how much to pad the text for a button
	float               cellpaddingypct;	// percent of fontsize how much to pad the text for a button
	float               cellspacingypct;	// percent of fontsize for row
	int					columncount;		// Listview
	PPA__ stringlist_t  columnheaders_a;
	PPA__ int32list_s	columnwidths_a;
	int					direction;
	
	// EVENTS
	struct {
		char			*onload_a;
		char 			*onkeypreview_a;
		char 			*onchange_a;
	} eve;

	float				fontdescendpct;
	float				fontsize;
	float				forealpha;
	vec3_t				forecolor;
	//vec3_t			forecolordisabled;
	vec3_t				forecolorselected;
	int					is_hidden;			// is_visible, except is_hidden can default false
	int					is_horizontal;
	int					is_rounded;
	PPA__ stringlist_t	list_strings_a;		// image how ..
	float				maxvalue;
	int /*runtime*/		selectedindex;
	PPA__ char          *text_a;			// different from caption how?
	float               value;
	PPA__ floatlist_s	vertexlist_a;

// dubious ...
//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////
    PPA__ char          *image_name_a;		// not used yet
    
	struct {
		int				cellpaddingx_i;		// percent of fontsize how much to pad the text for a button
		int				cellpaddingy_i;		// percent of fontsize how much to pad the text for a button
		int				cellspacing_i;		// percent of fontsize for row
		int				stringwidthmaxpx;
		rect_s			r_button;
		rect_s			r_leftbar;
		int				buttonstep;
		int				bottomof_last;
	} tb;
	//float				buttonheight_px_rt;	// fontsize * cellpadding
	//float				buttonwidth_px_rt;	// fontsize * cellpadding
	//float				cellspacing_px_rt;	// fontsize * cellspacing
	//
	//float				descender_pct_rt;
	//float				descender_px_rt;	// descender * fontsize

// Run-time
//  // 5 HEAD //		// 25				// 45 //////////////////////////////////////////////////////
	int					is_checked;


	struct {
		rect_s			r_top;
		rect_s			r_track;
		rect_s			r_bottom;

		rect_s			r_thumb;
		int				thumbsize_px;
		//int			empty_track_px;		// track less thumbsize -- 60 minus thumb 10 = 50
		int				num_top_positions;	// track_empty_px + 1
		int				track_empty_px;		// num_top_positions - 1
		
// value (0 to x - 1)
// maxvalue (num lines)
// number of top positions = tracksize less thumb - 1
// percent that each top position represents = maxvalue / (number of top positions)
// track position to value
	} sb;

// DEPRECATED
	int					mousedown_thing;
	timey_dbl_t			mousedown_when;
	xy_pairi_s			mousedown_at;
	xy_pairi_s			mousedown_thumb_into;	// How many px into thumb are we?

	xy_pairi_s			mousedown_thumb_at_mousedown_px;		// EXPLAIN
	xy_pairi_s			mousedown_forced_thumb_pos;			// SCROLLBAR FORCES THUMB TO THIS POSITION

	//rect_s			mousedown_rect;	// Sizedot wants


} oject_st;






