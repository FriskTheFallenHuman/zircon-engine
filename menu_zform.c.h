// menu_zform.c.h

#include "oject.c.h"

#include "menu_zform_connect.c.h"

WARP_X_ (DevInit)

WARP_X_ (M_ZDev_Draw)
static void M_ZForm_Draw (void)
{
	GoogleRobotoFont_Check ();
	oject_s *f = form1;

	if (!f)
		return;

	Form_Draw (f);
}

WARP_X_ (Object_Draw Form_MouseButtonAction)
static void M_ZForm_Key (cmd_state_t *cmd, int key, int ascii, int isdown)
{
	if (!form1)
		return;

	switch (key) {

	case K_ESCAPE:
		// Determine context
		if (isdown) {
			if (form1 && form1->servo.kcontextmenu->is_hidden == false) {
				// Context menu is active.
				Form_KeyDown (form1, key, ascii, isdown);
				break; // GET OUT
			}

			KeyDest_Set (key_game); menu_state_set_nova (m_none);  // simply leave menu
			//M_Menu_Main_f(cmd); up 1 level
			WARP_X_ (M_Menu_Main_f)
		}
		break;

	case K_MOUSE1:
		Form_MouseButtonAction (form1, key, ascii, isdown);
		break;

	case K_MOUSE2:
		if (isdown) Form_Mouse2Down (form1);
		break;

	case K_TAB:
		if (isdown)
		Form_Focus_Next (form1, KM_SHIFT ?  -1 :1);
		break;

	default:
		Form_KeyDown (form1, key, ascii, isdown);
		break;
	} // sw
}

int old_in_windowmouse_x = 0;
int old_in_windowmouse_y = 0;

WARP_X_ (Consel_MouseMove_Check M_ToggleMenu)
void ZForm_MouseMove (int x, int y)
{
	if (MVM_prog->loaded == false && isin1 (m_state, m_zform_30)) {
		if (old_in_windowmouse_x != x || old_in_windowmouse_y != y) {
			if (form1) {
				old_in_windowmouse_x = x;
				old_in_windowmouse_y = y;

				Form_Mouse_Move (form1, x, y);
			}
		}
	}
}

// Baker: This doesn't have enough "junk"
WARP_X_ (M_Menu_Keys_f)
void M_Menu_ZForm_f (cmd_state_t *cmd)
{
	KeyDest_Set (key_menu);
	menu_state_reenter = 0;
	menu_state_set_nova (m_zform_30);
	Con_CloseConsole_If_Client();
	m_entersound = true;

	//const char *sx = Cmd_Argc(cmd) >= 2 ? Cmd_Argv(cmd,1) : "form1.form,h";
	const char *sx = "form1.txt";

	// Dump it
	if (0 && form1) {
		stringlist_t lines = {0};
		Form_Dump (form1, &lines, DUMP_DETAIL_SAVE_FILE_0);
		stringlistprint (&lines, va32("Form Dump detail level (0 to 2) = %d", DUMP_DETAIL_SAVE_FILE_0), Con_PrintLinef);
		stringlistfreecontents (&lines);

		Form_Print_Zones (form1);
		Form_Draw_Dump (form1, &lines);
		stringlistprint (&lines, va32("Drawn list = %d", form1->frm.drawn_list_a->numitems), Con_PrintLinef);
		stringlistfreecontents (&lines);
	}

#ifdef _DEBUG
	Things_Audit_Debug (); // Checks enums and stuff
#endif

	GoogleRobotoFont_Check ();

	//const char *extension = File_URL_GetExtension (sx); // strrev, so minimal extension.  "" on none
	va_super (sbuf, MAX_QPATH_128, "engine/%s", sx); // Set form1.txt text

	// This does if no extension
	File_URL_Edit_Default_Extension (sbuf, ".txt", sizeof(sbuf));

	if (form1) {
		// Delete the form first
		form1 = Form_Destroy (form1);
	}

	char *s_zalloc = FS_LoadFile_Quiet_Temp (sbuf);
	if (!s_zalloc) {
		Con_PrintLinef ("Error loading from file %s", sbuf);
		return;
	}

	form1 = Form_Create_From_String (s_zalloc);

	if (!form1) {
		Con_PrintLinef ("Error parse %s", sbuf);
		return;
	}

	Mem_FreeNull_ (s_zalloc);

	// Dump it
	if (0 && form1) {
		stringlist_t lines = {0};
		Form_Dump (form1, &lines, DUMP_DETAIL_PLUS_CLIPPING_2);
		stringlistprint (&lines, va32("Form Dump detail level = %d", DUMP_DETAIL_PLUS_CLIPPING_2), Con_PrintLinef);
		stringlistfreecontents (&lines);
	}

}


