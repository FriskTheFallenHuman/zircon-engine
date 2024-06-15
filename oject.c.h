// oject.c.h

WARP_X_ (VM_stringwidth Draw_StringWidthf)
// SERVANT
// DOES NOT FOCUS, KFOCUS, ideally fires parent's event for it
// They are zebrasz

//  USES PARENT FOOTPRINT
//  USES PARENT CANVAS (INTERIOR)
//  DRAWS
//  IS NOT "CONTAINED" because that is coordinate system based off canvas interior.

	// INHERITED FROM FORM
	//size_t sizeofvec3_t = sizeof(vec3_t); // is this 12
	//typedef double dvec3_t[3];
	//size_t sizeofdvec3_t = sizeof(dvec3_t); // is this 12 or 24? ANSWER: 24 WORKS!
	// sizeofvec3_t does an expected!  DOES!


#include "oject_1_const_enums.c.h"
#include "oject_2_macros.c.h"
#include "oject_3_structs.c.h"
#include "oject_3_structs_oject.c.h"
#include "oject_4_globals.c.h"
#include "oject_5_fn_prototypes.c.h"
#include "oject_6_table___aaaa.c.h"

// CODE
#include "oject_aaa_procs.c.h"
#include "oject_aab_procs_draw.c.h"
#include "oject_aac_exec.c.h"

#include "oject_bba_lister.c.h"

#include "oject_bbx_button.c.h"
#include "oject_bbx_checkbox.c.h"
#include "oject_bbx_contextmenu.c.h"
#include "oject_bbx_image.c.h"
#include "oject_bbx_label.c.h"
#include "oject_bbx_listbox.c.h"
#include "oject_bbx_listview_procs.c.h"
#include "oject_bbx_listview.c.h"
#include "oject_bbx_polygon.c.h"
#include "oject_bbx_rectangle.c.h"
#include "oject_bbx_scrollbar.c.h"
#include "oject_bbx_sizedot.c.h"
#include "oject_bbx_tabselect.c.h"
#include "oject_bbx_textbox.c.h"

#include "oject_cca_properties.c.h"
#include "oject_ccb_object.c.h"
#include "oject_ccx_form_parse.c.h"
#include "oject_ccy_form_keymouse.c.h"
#include "oject_ccz_form.c.h"




