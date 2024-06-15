// oject_1_const_enums.c.h


// Directives ...

#define CLIPPING_ON 						(k->fctrl->frm.disable_clipping == false)
#define DO_PRINT							1		// Debug print of property parser

#define LV_TEXTMARGIN_10					10
#define LV_HEADERROW_ADD_6					6
#define LV_DEFAULT_WIDTH_100				100
#define LV_MIN_WIDTH_25						25
#define LV_TEXTMARGIN_1						1		// Subtracted from right clip area
#define LV_DRAG_COLSIZE_10					10

#define DIR_BACKWARDS_N1					-1
#define DIR_FORWARD_1						1

#define	OK_NOERROR_NULL						NULL
#define	OK_NOERROR_0						0
#define	IERR_PROP_NOT_FOUND_1				1
#define IERR_PROP_SET_ERROR_2				2

#define	TXT_CURSOR_WIDTH_4					4
#define TXT_BORDER_WIDTH_2					2
#define TXT_MARGIN_4						4
#define TXT_SCROLL_HORZ_AMOUNT_0_25			0.25	// Percent of textarea

#define	DDOT_SIZE_8							8		// 
//#define DOT_GAP_0							0

#define LENGTH_0							0

// General ...
#define CREATE_NULL							NULL
#define FORM_NULL							NULL
#define FORM_KCAN_NULL						NULL	// Form have no container.
#define SERVANT_KCAN_NULL					NULL	// Servants do not have a container, 
													// draw with owner as a zebra (Z-Ordered Top).
#define	SERVANT_TO_NULL						NULL

#define BOXWIDTH_2							2
#define RECT_CULLED_ZERO					0
#define SCROLLBAR_HOOK_REFIRE_0_1			0.1
#define	KNOB_SIZE_16						16
#define WHITE_TEXTURE_NULL					NULL

typedef enum {
	DUMP_DETAIL_SAVE_FILE_0 =				0,		// Save file, no servants
	DUMP_DETAIL_PLUS_SERVANTS_1 =			1,
	DUMP_DETAIL_PLUS_CLIPPING_2 = 			2,
} dump_detail_e;


#define POLYGON_2D_2						2
#define POLYGON_2D_FLOAT_SEPARATOR_SPACE	" "
#define POLYGON_2D_ELEMENT_SEPARATOR_COMMA	","
#define COLUMN_SEPARATOR_COMMA				","
#define POLYGON_WHITE_TEXTURE_NULL			NULL	// Polygon draw


#define BAKER_ARRAY_BATCHSIZE_DEFAULT_128	128
#define PROPERTY_COLON_DELIM				":"		// Properties colon delimiter


#define OFCAN_FOCUS_2						2		// A things flag
#define OFCAN_MOUSE_4						4		// A things flag
#define OFCAN_CONTAIN_8						8		// A things flag
#define OFCAN_RUNTIME_ONLY_NO_SAVE_16		16		// Like servant to what
#define OFCAN_AUTOSIZE_32					32		// Can specify its own size
#define OFCAN_NOHIGHLIGHT_64				64		// Does not draw highlight
#define OFCAN_ONLYFOCUS_128					128		// Context Menu - disappears on lost focus
													// replies cancel

#define	isbuttonaction_true					true
#define isbuttonaction_false				false
#define shallrecurse_true					true
#define shallrecurse_false					false

//

#define CLIPPED_2							2
#define CULLED_1							1
#define UNCULLED_0							0

#define TRANSPARENT_FALSE					false
#define OUTSET_TRUE							true
#define OUTSET_FALSE						false
#define BORDER_TRUE							true
#define THREED_TRUE							true
#define THIN_TRUE							true
#define THIN_FALSE							false

#define	q_indent_true						true
#define	q_indent_false						false
#define q_sethook_true						true
#define q_sethook_false						false

#define	isdone_true							true
#define	isdone_false						false

#define	isdown_true							true
#define	isdown_false						false

// 
#define SB_ACTION_UP_0						0
#define SB_ACTION_DOWN_1					1
#define SB_ACTION_PGUP_2					2
#define SB_ACTION_PGDOWN_3					3
#define SB_ACTION_THUMB_4					4

//
#define MOUSEUP_0							0
#define MOUSEDOWN_1							1

// 
#define CM_DOTCOUNT_COUNT_9					9
#define CM_MIDDLE_DOT_4						4
#define	CM_SERIES_COUNT_4					4
#define	CM_0_TEXT_0							0
#define	CM_1_CHECKED_1						1	// Checked .. for value use string
#define	CM_2_STRING_ID_2					2	// String id
#define	CM_3_FUNCSTRING_3					3

// "Things" 
typedef enum {
	thing_none_0, 

	class_start,
	class_form_1 = 1,	// Form must be 1 and the first property.  Hardcoded elsewhere.
	
	class_button,		// This should be in alphabetical order.  Values mean nothing.
	class_contextmenu,
	class_checkbox,
	class_sizedot,
	class_image, 
	class_label, 
	class_listbox, 
	class_listview, 
	class_polygon, 
	class_rectangle, 
	class_scrollbar, 
	class_tabselect, 
	class_textbox, 
	
	class_end,

	prop_start,
	prop_name = 100, 
	prop_rect = 101,

// Sort these
	prop_alignment, 
	prop_autosize,
	prop_backalpha,
	prop_backcolor,
	prop_backcolorselected,
	prop_caption,
	prop_cellpaddingxpct,
	prop_cellpaddingypct,
	prop_cellspacingypct,
	prop_columncount,
	prop_columnheaders,
	prop_columnwidths,
	prop_container,
	prop_fontdescendpct,
	prop_fontsize,
	prop_forealpha,
	prop_forecolor,
	prop_forecolorselected,
	prop_imagename, 
	prop_ishidden, 
	prop_ishorizontal, 
	prop_isrounded, 
	prop_list,
	prop_maxvalue,
	prop_servantowner,
	prop_selectedindex,
	prop_text,
	prop_value,
	prop_vertexes,

// Events are sort of like properties actually.
//	events_start,
	events_onload, 
	events_onkeypreview, 
	events_onchange,
//	events_end,

	prop_end,


// These things are special funtions we know
	builtin_func,
} things_e;

WARP_X_ (things oject_s)

typedef enum _vartype_e {
	vtstring_1		= 1,	// ev_string_1
	vtfloat_2		= 2,	// ev_float_2
	vtrectf_10		= 10,	// float rect
	vtrecti_11		= 11,	// float rect
	vtinteger_19	= 19,	// Best used for boolean, enumerations
	vtlist_str_20	= 20,	// list of strings
	vtrgb_21		= 21,	// rgb triplet of floats 0.0 to 1.0 (maybe higher too? up to 8?)
	vtfloatlist_22	= 22,	// List of polygon vertex
	vtcontrolref_23	= 23,	// List of integers
	vtint32list_24	= 24,	// List of polygon vertex

	vtevent_100		= 100,	// list of strings
} vartype_e;

WARP_X_ (Label_Rect_Edit_Align XY_Set_Dot)
typedef enum {
	//ENUM_FORCE_INT_GCC_ (alignment)
	alignment_top_left_0			= 0,
	alignment_top_center_1			= 1,
	alignment_top_right_2			= 2,
	alignment_middle_left_3			= 3,
	alignment_middle_center_4		= 4,
	alignment_middle_right_5		= 5,
	alignment_bottom_left_6			= 6,
	alignment_bottom_center_7		= 7,
	alignment_bottom_right_8		= 8,
} alignment_e;

typedef enum {
	border_Wall_none_0				= 0,
	border_Wall_left_1				= 1,
	border_Wall_top_2				= 2,
	border_Wall_right_4				= 4,  
	border_Wall_topless_1_4_8_13	= 13,
	border_Wall_bottomless_1_2_4_7	= 7,
	border_Wall_bottom_8			= 8,
} border_wall_e;

typedef enum {
	drawbut_thin_inset_0			= 0,
	drawbut_thin_outset_1			= 1,
	drawbut_normal_inset_2			= 2,
	drawbut_normal_outset_3			= 3,
	drawbut_command_up_4			= 4,
	drawbut_command_pressed_5		= 5,
	drawbut_command_thick_up_6		= 6,
//	drawbut_command_thick_pressed_7 = 7,
} drawbut_e;

typedef enum {
	arrow4_left_0					= 0,
	arrow4_right_1					= 1,
	arrow4_up_2						= 2,
	arrow4_down_3					= 3,
} arrow4_e;

typedef enum {
	refresh_reason_normal_0			= 0,	// Size change or simple recalc
	refresh_reason_preautosize_1	= 1,	// Early refresh ask autosize control its size 
	refresh_reason_finalized_2		= 2,	// Single pass when a form is fully parsed
											// Chance to fill in "required" data that is missing
											// Like column widths for a listview
} refresh_reason_e;

COMPILE_TIME_ASSERT (refresh_reason_e, sizeof(refresh_reason_e) == 4);

typedef enum {
	roundtype_none_0				= 0,	// current behavior that does what?
	roundtype_0_50_0				= 1,	// out_x exits if width of ch / 2 passes
} roundtype_e;





