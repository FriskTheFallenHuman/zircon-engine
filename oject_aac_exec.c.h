// oject_aac_exec.c.h

WARP_X_CALLERS_ (Oject_Focus_Set Form_ContextMenu_Spawn)
void EditModeSetDotsObject (oject_s *k)
{
	Form_Get (f, k);

	if (k->po->enum_id == class_contextmenu)
		return; // SILENT DENY RIGHT NOW.

	for (int dd = 0; dd < CM_DOTCOUNT_COUNT_9; dd ++) {
		oject_s *kdot = f->servo.kdots[dd];
		XY_Set_Dot (&kdot->cm.dot_origin, &k->r_screen, (alignment_e) kdot->direction);
		O_SizeDot_Refresh (kdot);
		kdot->is_hidden = false;
	}

	// CM_MIDDLE_DOT_4 .. overlays entire thing, own cursor.  Moves
	oject_s *kmid = f->servo.kdots[CM_MIDDLE_DOT_4];
	kmid->cm.relative_rect = k->r_screen;
	kmid->is_hidden = false;
	O_SizeDot_Refresh (kmid);

	//Form_QueueRefresh (f); // need to refresh drawn objects list
}

int _SetEditMode (oject_s *f, int newval)
{
	if (newval == f->frm.editmode)
		return 0; // no change

	f->frm.editmode = newval;
	if (f->frm.editmode) {
		WARP_X_ (CM_DOTCOUNT_COUNT_9)
		oject_s *kfoc = f->frm.kfocused; // contextmenu disappears first, kfocus should be ok

		// ContextMenu is DENIED!  Should be ok.
		for (int dd = 0; dd < CM_DOTCOUNT_COUNT_9; dd ++) {
			oject_s *kdot = f->servo.kdots[dd];
			kdot->is_hidden = false;
		}

		Form_QueueDotsRefresh (f);
	} else {
		// f->frm.editmode == 0
		for (int dd = 0; dd < CM_DOTCOUNT_COUNT_9; dd ++) {
			oject_s *kdot = f->servo.kdots[dd];
			kdot->is_hidden = true;
		}
	}

	Form_QueueRefresh (f);
	return 0;
}


static int _zcmdObjectCommand (oject_s *f, oject_s *k, int listidx, ccs *s)
{
	Con_Clear_f (cmd_local);
	Con_PrintLinef ("Object command!");
	//if (String_Match_Caseless (s, "print")) {
	//	int j = 5;
	//}
	//Form_Dump (f, DUMP_DETAIL_SAVE_FILE_0);
		stringlist_t lines = {0};
		Form_Dump (form1, &lines, DUMP_DETAIL_SAVE_FILE_0);

#if 0
		stringlistprint (&lines, va32("Form Dump detail level (0 to 2) = %d", DUMP_DETAIL_SAVE_FILE_0), Con_PrintLinef);
#else
		char *s_z = stringlist_join_lines_zalloc(&lines);
		Clipboard_Set_Text (s_z);
		Z_FreeNull_ (s_z);

#endif

		stringlistfreecontents (&lines);

		//Form_Print_Zones (form1);
		//Form_Draw_Dump (form1, &lines);
		//stringlistprint (&lines, va32("Drawn list = %d", form1->frm.drawn_list_a->numitems), Con_PrintLinef);
		//stringlistfreecontents (&lines);



	return 0;
}

static int _zcmdEditModeToggle (oject_s *f, oject_s *k, int listidx, ccs *s)
{
	char *s_show	= k->list_strings_a.strings[listidx + CM_0_TEXT_0];
	char *s_checked	= k->list_strings_a.strings[listidx + CM_1_CHECKED_1];
	char *s_str		= k->list_strings_a.strings[listidx + CM_2_STRING_ID_2];
	char *s_fn		= k->list_strings_a.strings[listidx + CM_3_FUNCSTRING_3];

	// As much as we hate this ...
	int oldval = s_checked[0] == '1';
	int	newval = !oldval;

	Z_StrDupf_Realloc(&k->list_strings_a.strings[listidx + CM_1_CHECKED_1], "%d", newval ? 1 : 0);

	_SetEditMode (f, newval);

	//Con_PrintLinef ("EditMode set to %d", newval);
	return 0;
}

typedef int (*zexecfn_t) (oject_s *f, oject_s *k, int listidx, ccs *s);
typedef struct {
	ccs			*name;
	zexecfn_t	fn;
} zexecitem_s;

WARP_X_CALLERS_ (O_Context_Menu_Exec)

WARP_X_ (Form_ContextMenu_Spawn)
zexecitem_s zexecitems[] = {
	{"EditModeToggle",	_zcmdEditModeToggle	},
	{"ObjectCommand",	_zcmdObjectCommand	},
};

static int zexeccmd (oject_s *f, oject_s *k, int listidx, ccs *s_fn, ccs *s)
{
	for (int j = 0; j < (int)ARRAY_COUNT(zexecitems); j ++) {
		zexecitem_s *zi = &zexecitems[j];
		if (String_Match_Caseless(zi->name, s_fn)) {
			return zi->fn (f, k, listidx, s);
		}
	}
	Con_PrintLinef ("zexeccmd: command function " QUOTED_S " not found", s_fn);
	return 0;
}


