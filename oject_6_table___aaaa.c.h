// oject_6_table.c.h


things_s things[] = {
	//char			*name;		// 1
	//int				enum_id;	// 2
	//union {
	//	int			vt;			// 3 Vartype
	//	int			oflags;		//   Can focus, can mouse flags.
	//} w;
	//size_t			moffsetof;	// 4 Field offset into the oject_s struct
	//ccs				*sdefault;	// 5 
	//ccs				*describe;	// 6 
	////parse_fn_t	parse_fn;	// 7 TODO	{"",					thing_none_0,						},

// On the startup of a form, these will be inherited by all controls.

#define THEME_DEFAULTS "BackAlpha:1.0 ForeColor:" QUOTED_STR ("RGB(0,0,0)") \
					" BackColor:" QUOTED_STR("RGB(255,255,255)") \
					" BackColorSelected:" QUOTED_STR("RGB(0,0,0)") \
					" ForeColorSelected:" QUOTED_STR("RGB(255,255,255)") \
					" FontSize:24 " // Ender


//											VALIDATE FN?
	{ "", /*KEEP ME*/ },
	{"Form",				class_form_1,		0,							0, THEME_DEFAULTS	}, // This must be idx 1 NOT ZERO


WARP_X_ (sdefault THEME_DEFAULTS Object_Default_Values)
	// THEME_DEFAULTS: ForeColor / BackColor / ForeColorDisabled / BackColorDisabled
	// We can do a string parse of the default string and if it matches, prevent writing/export
	WARP_X_ (Object_Create_And_Assign)

#define TABSELECT_DEFAULTS "AutoSize:1 CellPaddingXPct:0.50 CellPaddingYPct:0.50" \
	" CellSpacingYPct:0.0625 FontDescendPct:1.0" // 0.0625 is 1/16

#define CONTEXTMENU_DEFAULTS TABSELECT_DEFAULTS

	// 1					2					3								4
	{"CheckBox",			class_checkbox,		OFCAN_FOCUS_2 | OFCAN_MOUSE_4, 0, ""	},
	{"Button",				class_button,		OFCAN_FOCUS_2 | OFCAN_MOUSE_4, 0, ""	},
	{"ContextMenu",			class_contextmenu,	OFCAN_FOCUS_2 | OFCAN_MOUSE_4 | OFCAN_ONLYFOCUS_128, 0, CONTEXTMENU_DEFAULTS	},
	{"Image",				class_image,		0,							   0, ""	},
	{"Label",				class_label,		OFCAN_AUTOSIZE_32,			   0, "AutoSize:1"	},
	{"ListBox",				class_listbox,		OFCAN_FOCUS_2 | OFCAN_MOUSE_4, 0, 		},
	{"ListView",			class_listview,		OFCAN_FOCUS_2 | OFCAN_MOUSE_4, 0, 		},
	{"Polygon",				class_polygon,		0,							   0, ""	},	
	{"Rectangle",			class_rectangle,	0,							   0, },
	{"Scrollbar",			class_scrollbar,	OFCAN_MOUSE_4,				   0, "MaxValue:1"	},
	{"SizeDot",				class_sizedot,		OFCAN_MOUSE_4,				   0, "BackColor:" QUOTED_STR("RGB(255,255,255)")	},
	{"TabSelect",			class_tabselect,	OFCAN_FOCUS_2 | OFCAN_MOUSE_4 | 


		OFCAN_AUTOSIZE_32 | OFCAN_NOHIGHLIGHT_64, 0, TABSELECT_DEFAULTS
			},
	{"TextBox",				class_textbox,		OFCAN_FOCUS_2 | OFCAN_MOUSE_4, 0, ""	},
	
	{"SlideContainer",											},
	{"PictureBox",												},	// Interactable
	{"Viewport",												},
	{"ToolTip",													},
	{"ComboBox",												},
	{"MenuBox",													},
	{"MenuPane",												},

//Polygon		vertexes:"20 20, 500 400, 550 450" forecolor:"RGB(255,255,0)"
	
// 	

	WARP_X_ (oject_s)
//											
	{"Name",				prop_name,				vtstring_1,		member_offsetof (form1, cm.name_a),		},
	{"Rect",				prop_rect,				vtrecti_11,		member_offsetof (form1, cm.relative_rect),	},

#define IH // INHERITED MARKER for now.

// ORDER:
// Control is created.
// Form THEME defaults runs on control
// Class defaults runs on control
// Current values of form for inherited properties are COPIED to the object
//  This allows the form backcolor and such to be inherited to the control.

// Order should mirror
	{"Alignment",			prop_alignment,			vtinteger_19,	member_offsetof (form1, alignment),		},
	{"AutoSize",			prop_autosize,			vtinteger_19,	member_offsetof (form1, autosize),		},
	{"BackAlpha",			prop_backalpha,			vtfloat_2,		member_offsetof (form1, backalpha),		},
IH	{"BackColor",			prop_backcolor,			vtrgb_21,		member_offsetof (form1, backcolor),		},
IH	{"BackColorSelected",	prop_backcolorselected,	vtrgb_21,		member_offsetof (form1, backcolorselected),},
	{"FontDescendPct",		prop_fontdescendpct,	vtfloat_2,		member_offsetof (form1, fontdescendpct), "Scale font descent (overhang) by this factor when calculating text sizing and positioning, used to avoid vertically uncentered looking text (clampf)"},
	{"Caption",				prop_caption,			vtstring_1,		member_offsetof (form1, caption_a),		},
	{"CellPaddingXPct",		prop_cellpaddingxpct,	vtfloat_2,		member_offsetof (form1, cellpaddingxpct), "X Button Padding as Percent of FontSize on each side" },
	{"CellPaddingYPct",		prop_cellpaddingypct,	vtfloat_2,		member_offsetof (form1, cellpaddingypct), "Y Button Padding as Percent of FontSize on each side" },
	{"CellSpacingYPct",		prop_cellspacingypct,	vtfloat_2,		member_offsetof (form1, cellspacingypct), "Y Cellspacing as percent of FontSize between each cell" },
	{"ColumnCount",		prop_columncount,		vtinteger_19,	member_offsetof (form1, columncount),	},
	{"ColumnHeaders",		prop_columnheaders,		vtlist_str_20,	member_offsetof (form1, columnheaders_a),},
	{"ColumnWidths",		prop_columnwidths,		vtint32list_24,	member_offsetof (form1, columnwidths_a),},
IH  {"Container",			prop_container,			vtcontrolref_23,member_offsetof (form1, container),		},
IH	{"FontSize",			prop_fontsize,			vtfloat_2,		member_offsetof (form1, fontsize),		},
	{"ForeAlpha",			prop_forealpha,			vtfloat_2,		member_offsetof (form1, forealpha),		},
IH	{"ForeColor",			prop_forecolor,			vtrgb_21,		member_offsetof (form1, forecolor),		},
IH	{"ForeColorSelected",	prop_forecolorselected,	vtrgb_21,		member_offsetof (form1, forecolorselected),},
	{"ImageName",			prop_imagename,			vtstring_1,		member_offsetof (form1, image_name_a),	},
	{"IsHidden",			prop_ishidden,			vtinteger_19,	member_offsetof (form1, is_hidden),		},
	{"IsHorizontal",		prop_ishorizontal,		vtinteger_19,	member_offsetof (form1, is_horizontal),	},
	{"IsRounded",			prop_isrounded,			vtinteger_19,	member_offsetof (form1, is_rounded),	},
	{"List",				prop_list,				vtlist_str_20,	member_offsetof (form1, list_strings_a),},
	{"MaxValue",			prop_maxvalue,			vtfloat_2,		member_offsetof (form1, maxvalue),		},	// Not interactable at all
	{"ServantOwner",		prop_servantowner,		vtcontrolref_23,member_offsetof (form1, servant_owner),	},	// Not interactable at all
/*RT*/	{"SelectedIndex",	prop_selectedindex,		vtinteger_19,	member_offsetof (form1, selectedindex),	},	// Not interactable at all
	{"Text",				prop_text,				vtstring_1,		member_offsetof (form1, text_a),			},	// Not interactable at all
	{"Value",				prop_value,				vtfloat_2,		member_offsetof (form1, value),			},	// Not interactable at all
	{"Vertexes",			prop_vertexes,			vtfloatlist_22,	member_offsetof (form1, vertexlist_a),	"Spaced delimited pair of points"},	// Not interactable at all
	
//
WARP_X_ (Form_Event_Onload)
	{"onload",				events_onload,			vtstring_1,		member_offsetof (form1, eve.onload_a), "Occurs after finalize, all controls have been loaded.  No controls have had events fired."	},
	{"onkeypreview",		events_onkeypreview,	vtstring_1,		member_offsetof (form1, eve.onkeypreview_a),},
	{"onchange",			events_onchange,		vtstring_1,		member_offsetof (form1, eve.onchange_a),	"Occurs onload after form onload and whenever a the selectedindex changes."},
	
	
// builtin functions we know
 
	{"zform_load",			builtin_func,					}, // ? does what?
	{"zform_keypreview",	builtin_func,					},
	{"check1_click",		builtin_func,					},
	{"tabsel_click",		builtin_func,					},	// Probably fires the idx logic

// this list requires null for first field for last entry
	{NULL,													},
};

#undef IH // INHERITED MARKER FOR NOW

#define IsObjectType(x) in_range(class_start, x->enum_id, class_end)
#define IsProperty(x)	in_range(prop_start, x->enum_id, prop_end)
#define IsEvent(x)		in_range(events_start, x->enum_id, events_end)
#define IsBuiltin(x)	(x->enum_id == builtin_func)
#define ThingType(x) \
	(IsObjectType(x) ? "[Object]" :	\
	IsProperty(x) ? "[Property]" :	\
	IsEvent(x) ? "[Event]" :		\
	IsBuiltin(x) ? "[Builtin Function]" : \
	"[Unknown]") // Ender


