// menu_zdev.c.h

#define		local_count					mz->list.numstrings

#include "menu_zdev_head.c.h"


WARP_X_ (icons/floppy.png pk3_handle -- close read)
WARP_X_ (menu_form_s M_Menu_ZDev_f)



WARP_X_ (M_ZDev_Draw PRVM_ED_PrintEdicts_SV_f)



WARP_X_ (M_ZDev_Draw M_Menu_Load2_f font_shutdown)
static void M_ZDev_Key (cmd_state_t *cmd, int key, int ascii)
{
	//int lcase_ascii;
	//int header_idx;
	hotspotx_s *hotspot_hit;

	mz->scroll_is_blocked = false;

	switch (key) {
	case K_MOUSE2: // fall thru to K_ESCAPE and then exit
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1) 
			break;

		mz->scroll_is_blocked	= true; // PROTECT AGAINST AUTOSCROLL

		// Is it a header idx?
		hotspot_hit = &hotspotxs[hotspotx_hover];

		// Header IDX
		WARP_X_ (DEV_Category_Click)
		if (hotspot_hit->fire_fn) {
			hotspot_hit->fire_fn(hotspot_hit->fire_idx);
			break;
		}

		if (false == IsDoubleClick_Set_Cursor (&mz->click_time, &mz->cursor, mz->start_row, hotspotx_hover))
			break; // Too slow, focus item only

		// Fall through to fire K_ENTER

	case K_ENTER:
		//S_LocalSound ("sound/misc/menu2.wav");
		break;

	case K_SPACE:
		break;

	case K_HOME:
		if (local_count)
			mz->cursor = 0;
		break;

	case K_END:
		if (local_count)
			mz->cursor = local_count - 1;
		break;

	case K_TAB: // Can we detect shift here?
		DEV_Category_Rotate (KM_SHIFT ?  -1 :1); // Versus -1
		break;

	case K_PGUP:
		mz->cursor -= mz->visible_rows / 2;
		if (mz->cursor < 0) // PGUP does not wrap, stops at start
			mz->cursor = 0;
		break;

	case K_MWHEELUP:
		mz->cursor -= mz->visible_rows / 4;
		if (mz->cursor < 0) // K_MWHEELUP does not wrap, stops at start
			mz->cursor = 0;
		break;

	case K_PGDN:
		mz->cursor += mz->visible_rows / 2;
		if (mz->cursor >= local_count) // PGDN does not wrap, stops at end
			mz->cursor = local_count - 1;
		break;

	case K_MWHEELDOWN:
		mz->cursor += mz->visible_rows / 4;
		if (mz->cursor >= local_count) 
			mz->cursor = local_count - 1;
		break;

	case K_UPARROW:
		mz->cursor --;
		if (mz->cursor < 0) // K_UPARROW wraps around to end EXCEPT ON MEGA LISTS
			mz->cursor = 0; // MEGA EXCEPTION
		break;

	case K_DOWNARROW:
		mz->cursor ++;
		if (mz->cursor >= local_count) // K_DOWNARROW wraps around to start EXCEPT ON MEGA LISTS
			mz->cursor = local_count - 1 ; // MEGA EXCEPTION
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		break;
	} // sw

}
WARP_X_ (SavegameTextureCreate SavegameTextureCreate)

#include "menu_zdev_draw.c.h"

void M_Menu_ZDev_f (cmd_state_t *cmd)
{
	KeyDest_Set (key_menu);
	menu_state_set_nova (m_zdev_29);
	m_entersound = true;

	MZ_Reset ();
	menu_state_reenter = 0; // WHY?
}


#undef	local_count	

