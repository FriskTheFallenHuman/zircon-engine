#if defined(_WIN32) && !defined(CORE_SDL)

/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vid_wgl.c -- NT GL vid component

static const char *WIN_CLASSNAME_CLASS1 = "ZirconWindowClass";

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#endif

#ifdef SUPPORTDIRECTX
// Include DX libs
#ifdef _MSC_VER
//#pragma comment(lib, "dinput8.lib")
#pragma comment (lib, "dinput.lib")

#pragma comment(lib, "dxguid.lib")
#endif
#ifndef DIRECTINPUT_VERSION
#	define DIRECTINPUT_VERSION 0x0500  /* Version 5.0 */
#endif
#endif

#include "darkplaces.h"
#include <windows.h>
#include <mmsystem.h>
#ifdef SUPPORTDIRECTX
#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined
#include <dsound.h>
#endif
#include "resource.h"
#include <commctrl.h>
#ifdef SUPPORTDIRECTX
#include <dinput.h>
#endif



extern HINSTANCE global_hInstance;

static HINSTANCE gldll;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#endif

// Tell startup code that we have a client
int cl_available = true;

qbool vid_supportrefreshrate = true;

static int (WINAPI *qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *);
static int (WINAPI *qwglDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
//static int (WINAPI *qwglGetPixelFormat)(HDC);
static BOOL (WINAPI *qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
static BOOL (WINAPI *qwglSwapBuffers)(HDC);
static HGLRC (WINAPI *qwglCreateContext)(HDC);
static BOOL (WINAPI *qwglDeleteContext)(HGLRC);
static HGLRC (WINAPI *qwglGetCurrentContext)(VOID);
static HDC (WINAPI *qwglGetCurrentDC)(VOID);
static PROC (WINAPI *qwglGetProcAddress)(LPCSTR);
static BOOL (WINAPI *qwglMakeCurrent)(HDC, HGLRC);
static BOOL (WINAPI *qwglSwapIntervalEXT)(int interval);
static const char *(WINAPI *qwglGetExtensionsStringARB)(HDC hdc);
static BOOL (WINAPI *qwglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
static BOOL (WINAPI *qwglGetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);

static dllfunction_t wglfuncs[] =
{
	{"wglChoosePixelFormat", (void **) &qwglChoosePixelFormat},
	{"wglDescribePixelFormat", (void **) &qwglDescribePixelFormat},
//	{"wglGetPixelFormat", (void **) &qwglGetPixelFormat},
	{"wglSetPixelFormat", (void **) &qwglSetPixelFormat},
	{"wglSwapBuffers", (void **) &qwglSwapBuffers},
	{"wglCreateContext", (void **) &qwglCreateContext},
	{"wglDeleteContext", (void **) &qwglDeleteContext},
	{"wglGetProcAddress", (void **) &qwglGetProcAddress},
	{"wglMakeCurrent", (void **) &qwglMakeCurrent},
	{"wglGetCurrentContext", (void **) &qwglGetCurrentContext},
	{"wglGetCurrentDC", (void **) &qwglGetCurrentDC},
	{NULL, NULL}
};

static dllfunction_t wglswapintervalfuncs[] =
{
	{"wglSwapIntervalEXT", (void **) &qwglSwapIntervalEXT},
	{NULL, NULL}
};

static dllfunction_t wglpixelformatfuncs[] =
{
	{"wglChoosePixelFormatARB", (void **) &qwglChoosePixelFormatARB},
	{"wglGetPixelFormatAttribivARB", (void **) &qwglGetPixelFormatAttribivARB},
	{NULL, NULL}
};

static DEVMODE gdevmode, initialdevmode;
static vid_mode_t desktop_mode;
static qbool vid_initialized = false;
static qbool vid_wassuspended = false;
static qbool vid_usingmouse = false;
static qbool vid_usinghidecursor = false;
static qbool vid_usingvsync = false;
static qbool vid_usevsync = false;
static HICON hIcon;

// used by cd_win.c and snd_win.c
HWND mainwindow;

static HDC	 baseDC;
static HGLRC baseRC;


static qbool vid_isfullscreen;


LONG WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AppActivate(BOOL fActive, BOOL minimize);
static void ClearAllStates(void);
qbool VID_InitModeGL(viddef_mode_t *mode);
qbool VID_InitModeSOFT(viddef_mode_t *mode);

//====================================

static int window_x, window_y;

static qbool mouseinitialized;

#ifdef SUPPORTDIRECTX
static qbool dinput;
#define DINPUT_BUFFERSIZE           16
#define iDirectInputCreate(a,b,c,d)	pDirectInputCreate(a,b,c,d)

static HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);
#endif

// LadyHavoc: thanks to backslash for this support for mouse buttons 4 and 5
/* backslash :: imouse explorer buttons */
/* These are #ifdefed out for non-Win2K in the February 2001 version of
   MS's platform SDK, but we need them for compilation. . . */
#ifndef WM_XBUTTONDOWN
   #define WM_XBUTTONDOWN      0x020B
   #define WM_XBUTTONUP      0x020C
#endif
#ifndef MK_XBUTTON1
   #define MK_XBUTTON1         0x0020
   #define MK_XBUTTON2         0x0040
#endif
#ifndef MK_XBUTTON3
// LadyHavoc: lets hope this allows more buttons in the future...
   #define MK_XBUTTON3         0x0080
   #define MK_XBUTTON4         0x0100
   #define MK_XBUTTON5         0x0200
   #define MK_XBUTTON6         0x0400
   #define MK_XBUTTON7         0x0800
#endif
/* :: backslash */

// mouse variables
static int			mouse_buttons;
static int			mouse_oldbuttonstate;

static unsigned int uiWheelMessage;
#ifdef SUPPORTDIRECTX
static qbool	dinput_acquired;

static unsigned int		mstate_di;
#endif

static cvar_t vid_forcerefreshrate = {0, "vid_forcerefreshrate", "0", "try to set the given vid_refreshrate even if Windows doesn't list it as valid video mode"};

#ifdef SUPPORTDIRECTX
static LPDIRECTINPUT		g_pdi;
static LPDIRECTINPUTDEVICE	g_pMouse;
static HINSTANCE hInstDI;
#endif

// forward-referenced functions
static void IN_StartupMouse (void);
static void AdjustWindowBounds(int fullscreen, int *width, int *height, viddef_mode_t *mode, DWORD WindowStyle, RECT *rect);

//====================================

qbool vid_reallyhidden = true;

void VID_Finish (void)
{
	vid_hidden = vid_reallyhidden;

	vid_usevsync = vid_vsync.integer && !cls.timedemo && qwglSwapIntervalEXT;

	if (!vid_hidden)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL20:
		case RENDERPATH_GLES2:
			if (vid_usingvsync != vid_usevsync)
			{
				vid_usingvsync = vid_usevsync;
				qwglSwapIntervalEXT (vid_usevsync);
			}
			if (r_speeds.integer == 2 || gl_finish.integer)
				GL_Finish();
			SwapBuffers(baseDC);
			break;
		} // switch
	}

	// make sure a context switch can happen every frame - Logitech drivers
	// input drivers sometimes eat cpu time every 3 seconds or lag badly
	// without this help
	Sleep(0);

	VID_UpdateGamma(false, 256);
}

//==========================================================================


static unsigned char scantokey[128] =
{
//  0           1        2     3     4     5       6           7      8         9      A          B           C       D            E           F
	0          ,K_ESCAPE,'1'  ,'2'  ,'3'  ,'4'    ,'5'        ,'6'   ,'7'      ,'8'   ,'9'       ,'0'        ,'-'    ,'='         ,K_BACKSPACE,K_TAB,//0
	'q'        ,'w'     ,'e'  ,'r'  ,'t'  ,'y'    ,'u'        ,'i'   ,'o'      ,'p'   ,'['       ,']'        ,K_ENTER,K_CTRL      ,'a'        ,'s'  ,//1
	'd'        ,'f'     ,'g'  ,'h'  ,'j'  ,'k'    ,'l'        ,';'   ,'\''     ,'`'   ,K_SHIFT   ,'\\'       ,'z'    ,'x'         ,'c'        ,'v'  ,//2
	'b'        ,'n'     ,'m'  ,','  ,'.'  ,'/'    ,K_SHIFT    ,'*'   ,K_ALT    ,' '   ,K_CAPSLOCK,K_F1       ,K_F2   ,K_F3        ,K_F4       ,K_F5 ,//3
	K_F6       ,K_F7    ,K_F8 ,K_F9 ,K_F10,K_PAUSE,K_SCROLLOCK,K_HOME,K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5 ,K_RIGHTARROW,K_KP_PLUS  ,K_END,//4
	K_DOWNARROW,K_PGDN  ,K_INSERT,K_DELETE,0    ,0      ,0          ,K_F11 ,K_F12    ,0     ,0         ,0          ,0      ,0           ,0          ,0    ,//5
	0          ,0       ,0    ,0    ,0    ,0      ,0          ,0     ,0        ,0     ,0         ,0          ,0      ,0           ,0          ,0    ,//6
	0          ,0       ,0    ,0    ,0    ,0      ,0          ,0     ,0        ,0     ,0         ,0          ,0      ,0           ,0          ,0     //7
};


/*
=======
MapKey

Map from windows to quake keynums
=======
*/
static int MapKey (int key, int virtualkey)
{
	int result;
	int modified = (key >> 16) & 255;
	qbool is_extended = false;

	if (modified < 128 && scantokey[modified])
		result = scantokey[modified];
	else if (modified == 0 || virtualkey == 255) {
		result = 0;
		if (developer_insane.value) {
			Con_DPrintLinef("key 0x%02x (0x%8x, 0x%8x) has no translation", modified, key, virtualkey);
		}
	} else {
		result = 0;
		Con_DPrintf("key 0x%02x (0x%8x, 0x%8x) has no translation\n", modified, key, virtualkey);
	}

	if (key & (1 << 24))
		is_extended = true;

	if ( !is_extended )
	{
		if(((GetKeyState(VK_NUMLOCK)) & 0xffff) == 0)
			return result;

		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INSERT:
			return K_KP_INSERT;
		case K_DELETE:
			return K_KP_DELETE;
		default:
			return result;
		}
	}
	else
	{
		if(virtualkey == VK_NUMLOCK)
			return K_NUMLOCK;

		switch ( result )
		{
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
static void ClearAllStates (void)
{
	Key_ClearStates ();
	if (vid_usingmouse)
		mouse_oldbuttonstate = 0;
}

void AppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static qbool sound_active = false;  // initially blocked by Sys_InitConsole()

	vid_activewindow = fActive != FALSE;
	vid_reallyhidden = minimize != FALSE;

	vid.factive = vid_activewindow;
	vid.inactivefullscreen = !vid.factive && vid_isfullscreen;

	// enable/disable sound on focus gain/loss
	if ((!vid_reallyhidden && vid_activewindow) || !snd_mutewhenidle.integer)
	{
		if (!sound_active)
		{
			S_UnblockSound ();
			sound_active = true;
		}
	}
	else
	{
		if (sound_active)
		{
			S_BlockSound ();
			sound_active = false;
		}
	}

	if (fActive)
	{
		if (vid_isfullscreen)
		{
			if (vid_wassuspended)
			{
				vid_wassuspended = false;
				if (gldll)
				{
					ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
					ShowWindow(mainwindow, SW_SHOWNORMAL);
				}
			}

			// LadyHavoc: from dabb, fix for alt-tab bug in NVidia drivers
			if (gldll)
				MoveWindow(mainwindow,0,0,gdevmode.dmPelsWidth,gdevmode.dmPelsHeight,false);
		}
	}

	if (!fActive)
	{
		VID_SetMouse(false, false, false);
		Key_Release_Keys (); // Baker 1002
		if (vid_isfullscreen)
		{
			if (gldll)
				ChangeDisplaySettings (NULL, CDS_FULLSCREEN);
			vid_wassuspended = true;
		}
		VID_RestoreSystemGamma();
	}
}

//TODO: move it around in vid_wgl.c since I dont think this is the right position
void Sys_SendKeyEvents (void)
{
	MSG msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit (1);

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
}

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


static keynum_t buttonremap[16] =
{
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
	K_MOUSE9,
	K_MOUSE10,
	K_MOUSE11,
	K_MOUSE12,
	K_MOUSE13,
	K_MOUSE14,
	K_MOUSE15,
	K_MOUSE16,
};

#ifdef _DEBUG
typedef struct
{
	const char *keystring;
	unsigned value;
} keyvalue_s;
#define KEYVALUE(x) { #x , x }

static keyvalue_s wm_msgs_text [] = {
	KEYVALUE (WM_NULL                         ),
	KEYVALUE (WM_CREATE                       ),
	KEYVALUE (WM_DESTROY                      ),
	KEYVALUE (WM_MOVE                         ),
	KEYVALUE (WM_SIZE                         ),
	KEYVALUE (WM_ACTIVATE                     ),
	KEYVALUE (WM_SETFOCUS                     ),
	KEYVALUE (WM_KILLFOCUS                    ),
	KEYVALUE (WM_ENABLE                       ),
	KEYVALUE (WM_SETREDRAW                    ),
	KEYVALUE (WM_SETTEXT                      ),
	KEYVALUE (WM_GETTEXT                      ),
	KEYVALUE (WM_GETTEXTLENGTH                ),
	KEYVALUE (WM_PAINT                        ),
	KEYVALUE (WM_CLOSE                        ),
	KEYVALUE (WM_QUERYENDSESSION              ),
	KEYVALUE (WM_QUERYOPEN                    ),
	KEYVALUE (WM_ENDSESSION                   ),
	KEYVALUE (WM_QUIT                         ),
	KEYVALUE (WM_ERASEBKGND                   ),
	KEYVALUE (WM_SYSCOLORCHANGE               ),
	KEYVALUE (WM_SHOWWINDOW                   ),
	KEYVALUE (WM_WININICHANGE                 ),
	KEYVALUE (WM_SETTINGCHANGE                ),
	KEYVALUE (WM_DEVMODECHANGE                ),
	KEYVALUE (WM_ACTIVATEAPP                  ),
	KEYVALUE (WM_FONTCHANGE                   ),
	KEYVALUE (WM_TIMECHANGE                   ),
	KEYVALUE (WM_CANCELMODE                   ),
	KEYVALUE (WM_SETCURSOR                    ),
	KEYVALUE (WM_MOUSEACTIVATE                ),
	KEYVALUE (WM_CHILDACTIVATE                ),
	KEYVALUE (WM_QUEUESYNC                    ),
	KEYVALUE (WM_GETMINMAXINFO                ),
	KEYVALUE (WM_PAINTICON                    ),
	KEYVALUE (WM_ICONERASEBKGND               ),
	KEYVALUE (WM_NEXTDLGCTL                   ),
	KEYVALUE (WM_SPOOLERSTATUS                ),
	KEYVALUE (WM_DRAWITEM                     ),
	KEYVALUE (WM_MEASUREITEM                  ),
	KEYVALUE (WM_DELETEITEM                   ),
	KEYVALUE (WM_VKEYTOITEM                   ),
	KEYVALUE (WM_CHARTOITEM                   ),
	KEYVALUE (WM_SETFONT                      ),
	KEYVALUE (WM_GETFONT                      ),
	KEYVALUE (WM_SETHOTKEY                    ),
	KEYVALUE (WM_GETHOTKEY                    ),
	KEYVALUE (WM_QUERYDRAGICON                ),
	KEYVALUE (WM_COMPAREITEM                  ),
	KEYVALUE (WM_GETOBJECT                    ),
	KEYVALUE (WM_COMPACTING                   ),
	KEYVALUE (WM_COMMNOTIFY                   ),
	KEYVALUE (WM_WINDOWPOSCHANGING            ),
	KEYVALUE (WM_WINDOWPOSCHANGED             ),
	KEYVALUE (WM_POWER                        ),
	KEYVALUE (WM_COPYDATA                     ),
	KEYVALUE (WM_CANCELJOURNAL                ),
	KEYVALUE (WM_NOTIFY                       ),
	KEYVALUE (WM_INPUTLANGCHANGEREQUEST       ),
	KEYVALUE (WM_INPUTLANGCHANGE              ),
	KEYVALUE (WM_TCARD                        ),
	KEYVALUE (WM_HELP                         ),
	KEYVALUE (WM_USERCHANGED                  ),
	KEYVALUE (WM_NOTIFYFORMAT                 ),
	KEYVALUE (WM_CONTEXTMENU                  ),
	KEYVALUE (WM_STYLECHANGING                ),
	KEYVALUE (WM_STYLECHANGED                 ),
	KEYVALUE (WM_DISPLAYCHANGE                ),
	KEYVALUE (WM_GETICON                      ),
	KEYVALUE (WM_SETICON                      ),
	KEYVALUE (WM_NCCREATE                     ),
	KEYVALUE (WM_NCDESTROY                    ),
	KEYVALUE (WM_NCCALCSIZE                   ),
	KEYVALUE (WM_NCHITTEST                    ),
	KEYVALUE (WM_NCPAINT                      ),
	KEYVALUE (WM_NCACTIVATE                   ),
	KEYVALUE (WM_GETDLGCODE                   ),
	KEYVALUE (WM_SYNCPAINT                    ),
	KEYVALUE (WM_NCMOUSEMOVE                  ),
	KEYVALUE (WM_NCLBUTTONDOWN                ),
	KEYVALUE (WM_NCLBUTTONUP                  ),
	KEYVALUE (WM_NCLBUTTONDBLCLK              ),
	KEYVALUE (WM_NCRBUTTONDOWN                ),
	KEYVALUE (WM_NCRBUTTONUP                  ),
	KEYVALUE (WM_NCRBUTTONDBLCLK              ),
	KEYVALUE (WM_NCMBUTTONDOWN                ),
	KEYVALUE (WM_NCMBUTTONUP                  ),
	KEYVALUE (WM_NCMBUTTONDBLCLK              ),

	KEYVALUE (WM_NCXBUTTONDOWN                ),//
	KEYVALUE (WM_NCXBUTTONUP                  ),//
	KEYVALUE (WM_NCXBUTTONDBLCLK              ),//
	KEYVALUE (WM_INPUT_DEVICE_CHANGE          ),//
	KEYVALUE (WM_INPUT                        ),//

	KEYVALUE (WM_KEYFIRST                     ),
	KEYVALUE (WM_KEYDOWN                      ),
	KEYVALUE (WM_KEYUP                        ),
	KEYVALUE (WM_CHAR                         ),
	KEYVALUE (WM_DEADCHAR                     ),
	KEYVALUE (WM_SYSKEYDOWN                   ),
	KEYVALUE (WM_SYSKEYUP                     ),
	KEYVALUE (WM_SYSCHAR                      ),
	KEYVALUE (WM_SYSDEADCHAR                  ),

	KEYVALUE (WM_UNICHAR                      ),//

	KEYVALUE (WM_KEYLAST                      ),
	KEYVALUE (WM_KEYLAST                      ),
	KEYVALUE (WM_IME_STARTCOMPOSITION         ),
	KEYVALUE (WM_IME_ENDCOMPOSITION           ),
	KEYVALUE (WM_IME_COMPOSITION              ),
	KEYVALUE (WM_IME_KEYLAST                  ),
	KEYVALUE (WM_INITDIALOG                   ),
	KEYVALUE (WM_COMMAND                      ),
	KEYVALUE (WM_SYSCOMMAND                   ),
	KEYVALUE (WM_TIMER                        ),
	KEYVALUE (WM_HSCROLL                      ),
	KEYVALUE (WM_VSCROLL                      ),
	KEYVALUE (WM_INITMENU                     ),
	KEYVALUE (WM_INITMENUPOPUP                ),
	KEYVALUE (WM_MENUSELECT                   ),
	KEYVALUE (WM_MENUCHAR                     ),
	KEYVALUE (WM_ENTERIDLE                    ),
	KEYVALUE (WM_MENURBUTTONUP                ),
	KEYVALUE (WM_MENUDRAG                     ),
	KEYVALUE (WM_MENUGETOBJECT                ),
	KEYVALUE (WM_UNINITMENUPOPUP              ),
	KEYVALUE (WM_MENUCOMMAND                  ),

	KEYVALUE (WM_CHANGEUISTATE                ),//
	KEYVALUE (WM_UPDATEUISTATE                ),//
	KEYVALUE (WM_QUERYUISTATE                 ),//

	KEYVALUE (WM_CTLCOLORMSGBOX               ),
	KEYVALUE (WM_CTLCOLOREDIT                 ),
	KEYVALUE (WM_CTLCOLORLISTBOX              ),
	KEYVALUE (WM_CTLCOLORBTN                  ),
	KEYVALUE (WM_CTLCOLORDLG                  ),
	KEYVALUE (WM_CTLCOLORSCROLLBAR            ),
	KEYVALUE (WM_CTLCOLORSTATIC               ),
	KEYVALUE (WM_MOUSEFIRST                   ),
	KEYVALUE (WM_MOUSEMOVE                    ),
	KEYVALUE (WM_LBUTTONDOWN                  ),
	KEYVALUE (WM_LBUTTONUP                    ),
	KEYVALUE (WM_LBUTTONDBLCLK                ),
	KEYVALUE (WM_RBUTTONDOWN                  ),
	KEYVALUE (WM_RBUTTONUP                    ),
	KEYVALUE (WM_RBUTTONDBLCLK                ),
	KEYVALUE (WM_MBUTTONDOWN                  ),
	KEYVALUE (WM_MBUTTONUP                    ),
	KEYVALUE (WM_MBUTTONDBLCLK                ),
	KEYVALUE (WM_MOUSEWHEEL                   ),
	KEYVALUE (WM_XBUTTONDOWN                  ),
	KEYVALUE (WM_XBUTTONUP                    ),

	KEYVALUE (WM_XBUTTONDBLCLK                ),//
	KEYVALUE (WM_MOUSEHWHEEL                  ),// Horizontal?

	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_PARENTNOTIFY                 ),
	KEYVALUE (WM_ENTERMENULOOP                ),
	KEYVALUE (WM_EXITMENULOOP                 ),
	KEYVALUE (WM_NEXTMENU                     ),
	KEYVALUE (WM_SIZING                       ),
	KEYVALUE (WM_CAPTURECHANGED               ),
	KEYVALUE (WM_MOVING                       ),
	KEYVALUE (WM_POWERBROADCAST               ),
	KEYVALUE (WM_DEVICECHANGE                 ),
	KEYVALUE (WM_MDICREATE                    ),
	KEYVALUE (WM_MDIDESTROY                   ),
	KEYVALUE (WM_MDIACTIVATE                  ),
	KEYVALUE (WM_MDIRESTORE                   ),
	KEYVALUE (WM_MDINEXT                      ),
	KEYVALUE (WM_MDIMAXIMIZE                  ),
	KEYVALUE (WM_MDITILE                      ),
	KEYVALUE (WM_MDICASCADE                   ),
	KEYVALUE (WM_MDIICONARRANGE               ),
	KEYVALUE (WM_MDIGETACTIVE                 ),
	KEYVALUE (WM_MDISETMENU                   ),
	KEYVALUE (WM_ENTERSIZEMOVE                ),
	KEYVALUE (WM_EXITSIZEMOVE                 ),
	KEYVALUE (WM_DROPFILES                    ),
	KEYVALUE (WM_MDIREFRESHMENU               ),
	KEYVALUE (WM_IME_SETCONTEXT               ),
	KEYVALUE (WM_IME_NOTIFY                   ),
	KEYVALUE (WM_IME_CONTROL                  ),
	KEYVALUE (WM_IME_COMPOSITIONFULL          ),
	KEYVALUE (WM_IME_SELECT                   ),
	KEYVALUE (WM_IME_CHAR                     ),
	KEYVALUE (WM_IME_REQUEST                  ),
	KEYVALUE (WM_IME_KEYDOWN                  ),
	KEYVALUE (WM_IME_KEYUP                    ),
	KEYVALUE (WM_MOUSEHOVER                   ),
	KEYVALUE (WM_MOUSELEAVE                   ),
	KEYVALUE (WM_NCMOUSEHOVER                 ),
	KEYVALUE (WM_NCMOUSELEAVE                 ),
	KEYVALUE (WM_WTSSESSION_CHANGE            ),//
	KEYVALUE (WM_TABLET_FIRST                 ),//
	KEYVALUE (WM_TABLET_LAST                  ),//
	KEYVALUE (WM_CUT                          ),
	KEYVALUE (WM_COPY                         ),
	KEYVALUE (WM_PASTE                        ),
	KEYVALUE (WM_CLEAR                        ),
	KEYVALUE (WM_UNDO                         ),
	KEYVALUE (WM_RENDERFORMAT                 ),
	KEYVALUE (WM_RENDERALLFORMATS             ),
	KEYVALUE (WM_DESTROYCLIPBOARD             ),
	KEYVALUE (WM_DRAWCLIPBOARD                ),
	KEYVALUE (WM_PAINTCLIPBOARD               ),
	KEYVALUE (WM_VSCROLLCLIPBOARD             ),
	KEYVALUE (WM_SIZECLIPBOARD                ),
	KEYVALUE (WM_ASKCBFORMATNAME              ),
	KEYVALUE (WM_CHANGECBCHAIN                ),
	KEYVALUE (WM_HSCROLLCLIPBOARD             ),
	KEYVALUE (WM_QUERYNEWPALETTE              ),
	KEYVALUE (WM_PALETTEISCHANGING            ),
	KEYVALUE (WM_PALETTECHANGED               ),
	KEYVALUE (WM_HOTKEY                       ),
	KEYVALUE (WM_PRINT                        ),
	KEYVALUE (WM_PRINTCLIENT                  ),

	KEYVALUE (WM_APPCOMMAND                   ),//
	KEYVALUE (WM_THEMECHANGED                 ),//
	KEYVALUE (WM_CLIPBOARDUPDATE              ),//
	KEYVALUE (WM_DWMCOMPOSITIONCHANGED        ),//
	KEYVALUE (WM_DWMNCRENDERINGCHANGED        ),//
	KEYVALUE (WM_DWMCOLORIZATIONCOLORCHANGED  ),//
	KEYVALUE (WM_DWMWINDOWMAXIMIZEDCHANGE     ),//
	KEYVALUE (WM_GETTITLEBARINFOEX            ),//

	KEYVALUE (WM_HANDHELDFIRST                ),
	KEYVALUE (WM_HANDHELDLAST                 ),
	KEYVALUE (WM_AFXFIRST                     ),
	KEYVALUE (WM_AFXLAST                      ),
	KEYVALUE (WM_PENWINFIRST                  ),
	KEYVALUE (WM_PENWINLAST                   ),
	KEYVALUE (WM_APP                          ),
	KEYVALUE (WM_USER                         ),
NULL, 0}; // Null term

const char *msg_to_str (int val)
{
	int n;
	for (n = 0; n < ARRAY_COUNT(wm_msgs_text); n++) {
		keyvalue_s *r = &wm_msgs_text[n];
		if (val == (int)r->value) {
			return r->keystring;
		} // if
	} // for
	return "unknown";
}
#endif

/* main window procedure */
LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM  wParam, LPARAM lParam)
{
	LONG    lRet = 1;
	int		fActive, fMinimized, temp;
	unsigned char state[256];
	const unsigned int UNICODE_BUFFER_LENGTH = 4;
	WCHAR unicode[UNICODE_BUFFER_LENGTH];
	int		vkey;
	int		charlength;
	qbool down = false;

	if ( uMsg == uiWheelMessage )
		uMsg = WM_MOUSEWHEEL;


	#if 0 //def _DEBUG
		const char *sx = msg_to_str ((unsigned)uMsg);
		Sys_PrintToTerminal2 (sx);
		Sys_PrintToTerminal2 (va3 ( "wparam: %x lparam %x", wParam, lParam));
	#endif // DEBUG

	switch (uMsg) {
		case WM_KILLFOCUS:
			//if (vid_isfullscreen)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);

			break;

		case WM_ERASEBKGND:

			return 1; // MH: treachery!!! see your MSDN!

		case WM_GETMINMAXINFO:	// Sent before size change; can be used to override default mins/maxs
			fMinimized = true;
			AppActivate(/*active*/ false, fMinimized);

			// Baker: this stops a crash I don't feel like dealing with right now

			if (fMinimized && vid.minimized == false) {
				vid.savedgamma = v_gamma.value;
				vid.savedcontrast = v_contrast.value;

				Cvar_SetValueQuick(&v_gamma, 1);
				Cvar_SetValueQuick(&v_contrast, 1);
				VID_UpdateGamma(false, 256);
				//Sys_PrintToTerminal ("Gamma save");
			}
			vid.minimized = fMinimized;


			return 0;


		case WM_CREATE:
			break;

		case WM_MOVE:
			window_x = (int) LOWORD(lParam);
			window_y = (int) HIWORD(lParam);
			VID_SetMouse(false, false, false);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			down = true;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			vkey = MapKey(lParam, wParam);
			GetKeyboardState (state);
			// alt/ctrl/shift tend to produce funky ToAscii values,
			// and if it's not a single character we don't know care about it
			charlength = ToUnicode(wParam, lParam >> 16, state, unicode, UNICODE_BUFFER_LENGTH, 0);
			if(vkey == K_ALT || vkey == K_CTRL || vkey == K_SHIFT || charlength == 0)
				unicode[0] = 0;
			else if(charlength == 2)
				unicode[0] = unicode[1];
			if (!VID_JoyBlockEmulatedKeys(vkey))
				Key_Event(vkey, unicode[0], down);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

		case WM_SYSCOMMAND:
			// prevent screensaver from occuring while the active window
			// note: password-locked screensavers on Vista still work
			if (vid_activewindow && ((wParam & 0xFFF0) == SC_SCREENSAVE || (wParam & 0xFFF0) == SC_MONITORPOWER))
				lRet = 0;
			else
				lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:   // backslash :: imouse explorer buttons
		case WM_XBUTTONUP:      // backslash :: imouse explorer buttons
		case WM_MOUSEMOVE:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			/* backslash :: imouse explorer buttons */
			if (wParam & MK_XBUTTON1)
				temp |= 8;

			if (wParam & MK_XBUTTON2)
				temp |= 16;
			/* :: backslash */

			// LadyHavoc: lets hope this allows more buttons in the future...
			if (wParam & MK_XBUTTON3)
				temp |= 32;
			if (wParam & MK_XBUTTON4)
				temp |= 64;
			if (wParam & MK_XBUTTON5)
				temp |= 128;
			if (wParam & MK_XBUTTON6)
				temp |= 256;
			if (wParam & MK_XBUTTON7)
				temp |= 512;

#ifdef SUPPORTDIRECTX
			if (!dinput_acquired)
#endif
			{
				// perform button actions
				int i;
				for (i=0 ; i<mouse_buttons && i < 16 ; i++)
					if ((temp ^ mouse_oldbuttonstate) & (1<<i))
						Key_Event (buttonremap[i], 0, (temp & (1<<i)) != 0);
				mouse_oldbuttonstate = temp;
			}

			break;

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL:
			if ((short) HIWORD(wParam) > 0) {
				Key_Event(K_MWHEELUP, 0, true);
				Key_Event(K_MWHEELUP, 0, false);
			} else {
				Key_Event(K_MWHEELDOWN, 0, true);
				Key_Event(K_MWHEELDOWN, 0, false);
			}
			break;

		case WM_SIZE:

			if (wParam== SIZE_MINIMIZED) {
				fMinimized = true;
				AppActivate(/*active*/ false, fMinimized);

				// Baker: this stops a crash I don't feel like dealing with right now

				if (fMinimized && vid.minimized == false) {
					vid.savedgamma = v_gamma.value;
					vid.savedcontrast = v_contrast.value;

					Cvar_SetValueQuick(&v_gamma, 1);
					Cvar_SetValueQuick(&v_contrast, 1);
					VID_UpdateGamma(false, 256);
					//Sys_PrintToTerminal ("Gamma save");
				}
				vid.minimized = fMinimized;




			}

			if (wParam == SIZE_RESTORED) {
				fMinimized = false;
				if (fMinimized == false && vid.minimized) {
					//Sys_PrintToTerminal ("vid.unminimized");
					vid.unminimized=true;
				}
				vid.minimized = fMinimized;

			}


			break;

		case WM_CLOSE:
			//if (MessageBox (mainwindow, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES) // Baker 1012.1

			Host_Quit_f (); //Sys_Quit (0);


			break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			AppActivate(!(fActive == WA_INACTIVE), fMinimized);

			// Baker: this stops a crash I don't feel like dealing with right now
			if (fMinimized == false && vid.minimized) {
				//Sys_PrintToTerminal2 ("vid.unminimized");
				vid.unminimized=true;
			}

			if (fMinimized && vid.minimized == false) {
				vid.savedgamma = v_gamma.value;
				vid.savedcontrast = v_contrast.value;

				Cvar_SetValueQuick(&v_gamma, 1);
				Cvar_SetValueQuick(&v_contrast, 1);
				VID_UpdateGamma(false, 256);
				//Sys_PrintToTerminal ("Gamma save");
			}
			vid.minimized = fMinimized;


		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			ClearAllStates ();

			break;

		//case WM_DESTROY:
		//	PostQuitMessage (0);
		//	break;

		case MM_MCINOTIFY:
			// Baker: lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);

			break;

		default:
			/* pass all unhandled messages to DefWindowProc */
			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
		break;
	}

	/* return 1 if handled message, 0 if not */
	return lRet;
}

int VID_SetGamma(unsigned short *ramps, int rampsize)
{
	if (qwglMakeCurrent)
	{
		HDC hdc = GetDC (NULL);
		int i = SetDeviceGammaRamp(hdc, ramps);
		ReleaseDC (NULL, hdc);
		return i; // return success or failure
	}
	else
		return 0;
}

int VID_GetGamma(unsigned short *ramps, int rampsize)
{
	if (qwglMakeCurrent)
	{
		HDC hdc = GetDC (NULL);
		int i = GetDeviceGammaRamp(hdc, ramps);
		ReleaseDC (NULL, hdc);
		return i; // return success or failure
	}
	else
		return 0;
}

static void GL_CloseLibrary(void)
{
	if (gldll)
	{
		FreeLibrary(gldll);
		gldll = 0;
		gl_driver[0] = 0;
		qwglGetProcAddress = NULL;
		gl_extensions = "";
		gl_platform = "";
		gl_platformextensions = "";
	}
}

static int GL_OpenLibrary(const char *name)
{
	Con_Printf("Loading OpenGL driver %s\n", name);
	GL_CloseLibrary();
	if (!(gldll = LoadLibrary(name)))
	{
		Con_Printf("Unable to LoadLibrary %s\n", name);
		return false;
	}
	strlcpy(gl_driver, name, sizeof(gl_driver));
	return true;
}

void *GL_GetProcAddress(const char *name)
{
	if (gldll)
	{
		void *p = NULL;
		if (qwglGetProcAddress != NULL)
			p = (void *) qwglGetProcAddress(name);
		if (p == NULL)
			p = (void *) GetProcAddress(gldll, name);
		return p;
	}
	else
		return NULL;
}

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042
#endif


static void IN_Init(void);
void Vid_CL_Frame_Start (void) // CLX
{
	if (!qwglMakeCurrent(baseDC, baseRC)) {
		Con_DPrintLinef ("wglMakeCurrent(%p, %p) failed", (void *)baseDC, (void *)baseRC);
//		int j = 5;
	}
}

qbool VID_InitModeGL(viddef_mode_t *mode)
{
	int i;
	HDC hdc;
	RECT rect;
	MSG msg;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,				// version number
		PFD_DRAW_TO_WINDOW 		// support window
		|  PFD_SUPPORT_OPENGL 	// support OpenGL
		|  PFD_DOUBLEBUFFER ,	// double buffered
		PFD_TYPE_RGBA,			// RGBA type
		24,				// 24-bit color depth
		0, 0, 0, 0, 0, 0,		// color bits ignored
		0,				// no alpha buffer
		0,				// shift bit ignored
		0,				// no accumulation buffer
		0, 0, 0, 0, 			// accum bits ignored
		32,				// 32-bit z-buffer
		0,				// no stencil buffer
		0,				// no auxiliary buffer
		PFD_MAIN_PLANE,			// main layer
		0,				// reserved
		0, 0, 0				// layer masks ignored
	};
	int windowpass;
	int pixelformat, newpixelformat;
	UINT numpixelformats;
	DWORD WindowStyle, ExWindowStyle;
	const char *gldrivername;
	int depth;
	DEVMODE thismode;
	qbool foundmode, foundgoodmode;
	int *a;
	float *af;
	int attribs[128];
	float attribsf[16];
	int bpp = mode->bitsperpixel;
	int width = mode->width;
	int height = mode->height;
	int refreshrate = (int)floor(mode->refreshrate+0.5);
	int stereobuffer = mode->stereobuffer;
	int samples = mode->samples;
	int fullscreen = mode->fullscreen;

	if (vid_initialized)
		Sys_Error("VID_InitMode called when video is already initialised");

	// if stencil is enabled, ask for alpha too
	if (bpp >= 32)
	{
		pfd.cRedBits = 8;
		pfd.cGreenBits = 8;
		pfd.cBlueBits = 8;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
	}
	else
	{
		pfd.cRedBits = 5;
		pfd.cGreenBits = 5;
		pfd.cBlueBits = 5;
		pfd.cAlphaBits = 0;
		pfd.cDepthBits = 16;
		pfd.cStencilBits = 0;
	}

	if (stereobuffer)
		pfd.dwFlags |= PFD_STEREO;

	a = attribs;
	af = attribsf;
	*a++ = WGL_DRAW_TO_WINDOW_ARB;
	*a++ = GL_TRUE;
	*a++ = WGL_ACCELERATION_ARB;
	*a++ = WGL_FULL_ACCELERATION_ARB;
	*a++ = WGL_DOUBLE_BUFFER_ARB;
	*a++ = true;

	if (bpp >= 32)
	{
		*a++ = WGL_RED_BITS_ARB;
		*a++ = 8;
		*a++ = WGL_GREEN_BITS_ARB;
		*a++ = 8;
		*a++ = WGL_BLUE_BITS_ARB;
		*a++ = 8;
		*a++ = WGL_ALPHA_BITS_ARB;
		*a++ = 8;
		*a++ = WGL_DEPTH_BITS_ARB;
		*a++ = 24;
		*a++ = WGL_STENCIL_BITS_ARB;
		*a++ = 8;
	}
	else
	{
		*a++ = WGL_RED_BITS_ARB;
		*a++ = 1;
		*a++ = WGL_GREEN_BITS_ARB;
		*a++ = 1;
		*a++ = WGL_BLUE_BITS_ARB;
		*a++ = 1;
		*a++ = WGL_DEPTH_BITS_ARB;
		*a++ = 16;
	}

	if (stereobuffer)
	{
		*a++ = WGL_STEREO_ARB;
		*a++ = GL_TRUE;
	}

	if (samples > 1)
	{
		*a++ = WGL_SAMPLE_BUFFERS_ARB;
		*a++ = 1;
		*a++ = WGL_SAMPLES_ARB;
		*a++ = samples;
	}

	*a = 0;
	*af = 0;

	gldrivername = "opengl32.dll";
// COMMANDLINEOPTION: Windows WGL: -gl_driver <drivername> selects a GL driver library, default is opengl32.dll, useful only for 3dfxogl.dll or 3dfxvgl.dll, if you don't know what this is for, you don't need it
	i = Sys_CheckParm("-gl_driver");
	if (i && i < com_argc - 1)
		gldrivername = com_argv[i + 1];
	if (!GL_OpenLibrary(gldrivername))
	{
		Con_Printf("Unable to load GL driver %s\n", gldrivername);
		return false;
	}

	memset(&gdevmode, 0, sizeof(gdevmode));

	vid_isfullscreen = false;
	if (fullscreen)
	{
		if(vid_desktopfullscreen.integer)
		{
			foundmode = true;
			gdevmode = initialdevmode;
			width = mode->width = gdevmode.dmPelsWidth;
			height = mode->height = gdevmode.dmPelsHeight;
			bpp = mode->bitsperpixel = gdevmode.dmBitsPerPel;
		}
		else if(vid_forcerefreshrate.integer)
		{
			foundmode = true;
			gdevmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			gdevmode.dmBitsPerPel = bpp;
			gdevmode.dmPelsWidth = width;
			gdevmode.dmPelsHeight = height;
			gdevmode.dmSize = sizeof (gdevmode);
			if(refreshrate)
			{
				gdevmode.dmFields |= DM_DISPLAYFREQUENCY;
				gdevmode.dmDisplayFrequency = refreshrate;
			}
		}
		else
		{
			if(refreshrate == 0)
				refreshrate = initialdevmode.dmDisplayFrequency; // default vid_refreshrate to the rate of the desktop

			foundmode = false;
			foundgoodmode = false;

			thismode.dmSize = sizeof(thismode);
			thismode.dmDriverExtra = 0;
			for(i = 0; EnumDisplaySettings(NULL, i, &thismode); ++i)
			{
				if(~thismode.dmFields & (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY))
				{
					Con_DPrintf("enumerating modes yielded a bogus item... please debug this\n");
					continue;
				}
				if(developer_extra.integer)
					Con_DPrintf("Found mode %dx%dx%dbpp %dHz... ", (int)thismode.dmPelsWidth, (int)thismode.dmPelsHeight, (int)thismode.dmBitsPerPel, (int)thismode.dmDisplayFrequency);
				if(thismode.dmBitsPerPel != (DWORD)bpp)
				{
					if(developer_extra.integer)
						Con_DPrintf("wrong bpp\n");
					continue;
				}
				if(thismode.dmPelsWidth != (DWORD)width)
				{
					if(developer_extra.integer)
						Con_DPrintf("wrong width\n");
					continue;
				}
				if(thismode.dmPelsHeight != (DWORD)height)
				{
					if(developer_extra.integer)
						Con_DPrintf("wrong height\n");
					continue;
				}

				if(foundgoodmode)
				{
					// if we have a good mode, make sure this mode is better than the previous one, and allowed by the refreshrate
					if(thismode.dmDisplayFrequency > (DWORD)refreshrate)
					{
						if(developer_extra.integer)
							Con_DPrintf("too high refresh rate\n");
						continue;
					}
					else if(thismode.dmDisplayFrequency <= gdevmode.dmDisplayFrequency)
					{
						if(developer_extra.integer)
							Con_DPrintf("doesn't beat previous best match (too low)\n");
						continue;
					}
				}
				else if(foundmode)
				{
					// we do have one, but it isn't good... make sure it has a lower frequency than the previous one
					if(thismode.dmDisplayFrequency >= gdevmode.dmDisplayFrequency)
					{
						if(developer_extra.integer)
							Con_DPrintf("doesn't beat previous best match (too high)\n");
						continue;
					}
				}
				// otherwise, take anything

				memcpy(&gdevmode, &thismode, sizeof(gdevmode));
				if(thismode.dmDisplayFrequency <= (DWORD)refreshrate)
					foundgoodmode = true;
				else
				{
					if(developer_extra.integer)
						Con_DPrintf("(out of range)\n");
				}
				foundmode = true;
				if(developer_extra.integer)
					Con_DPrintf("accepted\n");
			}
		}

		if (!foundmode)
		{
			VID_Shutdown();
			Con_Printf("Unable to find the requested mode %dx%dx%dbpp\n", width, height, bpp);
			return false;
		}
		else if(ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			VID_Shutdown();
			Con_Printf("Unable to change to requested mode %dx%dx%dbpp\n", width, height, bpp);
			return false;
		}

		vid_isfullscreen = true;
		WindowStyle = WS_POPUP;
		ExWindowStyle = 0;//WS_EX_TOPMOST;
	}
	else
	{
		hdc = GetDC (NULL);
		i = GetDeviceCaps(hdc, RASTERCAPS);
		depth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
		ReleaseDC (NULL, hdc);
		if (i & RC_PALETTE)
		{
			VID_Shutdown();
			Con_Print("Can't run in non-RGB mode\n");
			return false;
		}
		if (bpp > depth)
		{
			VID_Shutdown();
			Con_Print("A higher desktop depth is required to run this video mode\n");
			return false;
		}

		WindowStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		ExWindowStyle = 0;
	}

	AdjustWindowBounds(fullscreen, &width, &height, mode, WindowStyle, &rect);

	pixelformat = 0;
	newpixelformat = 0;
	// start out at the final windowpass if samples is 1 as it's the only feature we need extended pixel formats for
	for (windowpass = samples == 1;windowpass < 2;windowpass++)
	{
		gl_extensions = "";
		gl_platformextensions = "";

		mainwindow = CreateWindowEx (ExWindowStyle, WIN_CLASSNAME_CLASS1,
			gamename, WindowStyle,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, global_hInstance, NULL); // PLX
		if (!mainwindow)
		{
			Con_Printf("CreateWindowEx(%d, %s, %s, %d, %d, %d, %d, %d, %p, %p, %p, %p) failed\n", (int)ExWindowStyle, WIN_CLASSNAME_CLASS1, gamename, (int)WindowStyle, (int)(rect.left), (int)(rect.top), (int)(rect.right - rect.left), (int)(rect.bottom - rect.top), (void *)NULL, (void *)NULL, (void *)global_hInstance, (void *)NULL);
			VID_Shutdown();
			return false;
		}

		baseDC = GetDC(mainwindow);

		if (!newpixelformat)
			newpixelformat = ChoosePixelFormat(baseDC, &pfd);
		pixelformat = newpixelformat;
		if (!pixelformat)
		{
			VID_Shutdown();
			Con_Printf("ChoosePixelFormat(%p, %p) failed\n", (void *)baseDC, (void *)&pfd);
			return false;
		}

		if (SetPixelFormat(baseDC, pixelformat, &pfd) == false)
		{
			VID_Shutdown();
			Con_Printf("SetPixelFormat(%p, %d, %p) failed\n", (void *)baseDC, pixelformat, (void *)&pfd);
			return false;
		}

		if (!GL_CheckExtension("wgl", wglfuncs, NULL, false))
		{
			VID_Shutdown();
			Con_Print("wgl functions not found\n");
			return false;
		}

		baseRC = qwglCreateContext(baseDC);
		if (!baseRC)
		{
			VID_Shutdown();
			Con_Print("Could not initialize GL (wglCreateContext failed).\n\nMake sure you are in 65536 color mode, and try running -window.\n");
			return false;
		}
		if (!qwglMakeCurrent(baseDC, baseRC))
		{
			VID_Shutdown();
			Con_Printf("wglMakeCurrent(%p, %p) failed\n", (void *)baseDC, (void *)baseRC);
			return false;
		}

		if ((qglGetString = (const GLubyte* (GLAPIENTRY *)(GLenum name))GL_GetProcAddress("glGetString")) == NULL)
		{
			VID_Shutdown();
			Con_Print("glGetString not found\n");
			return false;
		}
		if ((qwglGetExtensionsStringARB = (const char *(WINAPI *)(HDC hdc))GL_GetProcAddress("wglGetExtensionsStringARB")) == NULL)
			Con_Print("wglGetExtensionsStringARB not found\n");

		gl_extensions = (const char *)qglGetString(GL_EXTENSIONS);
		gl_platform = "WGL";
		gl_platformextensions = "";

		if (qwglGetExtensionsStringARB)
			gl_platformextensions = (const char *)qwglGetExtensionsStringARB(baseDC);

		if (!gl_extensions)
			gl_extensions = "";
		if (!gl_platformextensions)
			gl_platformextensions = "";

		// now some nice Windows pain:
		// we have created a window, we needed one to find out if there are
		// any multisample pixel formats available, the problem is that to
		// actually use one of those multisample formats we now have to
		// recreate the window (yes Microsoft OpenGL really is that bad)

		if (windowpass == 0)
		{
			if (!GL_CheckExtension("WGL_ARB_pixel_format", wglpixelformatfuncs, "-noarbpixelformat", false) || !qwglChoosePixelFormatARB(baseDC, attribs, attribsf, 1, &newpixelformat, &numpixelformats) || !newpixelformat)
				break;
			// ok we got one - do it all over again with newpixelformat
			qwglMakeCurrent(NULL, NULL);
			qwglDeleteContext(baseRC);baseRC = 0;
			ReleaseDC(mainwindow, baseDC);baseDC = 0;
			// eat up any messages waiting for us
			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
	}

	/*
	if (!fullscreen)
		SetWindowPos (mainwindow, NULL, CenterX, CenterY, 0, 0,SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
	*/

	ShowWindow (mainwindow, SW_SHOWDEFAULT); // 1
	UpdateWindow (mainwindow); // 2

	// now we try to make sure we get the focus on the mode switch, because
	// sometimes in some systems we don't.  We grab the foreground, then
	// finish setting up, pump all our messages, and sleep for a little while
	// to let messages finish bouncing around the system, then we put
	// ourselves at the top of the z order, then grab the foreground again,
	// Who knows if it helps, but it probably doesn't hurt



//	Sleep (100);

	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);  // 3

	SetForegroundWindow (mainwindow); // 4


	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) { // 5
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	Sleep (100);

	// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

// COMMANDLINEOPTION: Windows WGL: -novideosync disables WGL_EXT_swap_control
	GL_CheckExtension("WGL_EXT_swap_control", wglswapintervalfuncs, "-novideosync", false);

	GL_Init ();

	//vid_menudrawfn = VID_MenuDraw;
	//vid_menukeyfn = VID_MenuKey;
	vid_usingmouse = false;
	vid_usinghidecursor = false;
	vid_usingvsync = false;
	vid_reallyhidden = vid_hidden = false;
	vid_initialized = true;

	IN_StartupMouse ();

	if (qwglSwapIntervalEXT)
	{
		vid_usevsync = vid_vsync.integer != 0;
		vid_usingvsync = vid_vsync.integer != 0;
		qwglSwapIntervalEXT (vid_usevsync);
	}

	return true;
}

static void AdjustWindowBounds(int fullscreen, int *width, int *height, viddef_mode_t *mode, DWORD WindowStyle, RECT *rect)
{
	int CenterX, CenterY;

	rect->top = 0;
	rect->left = 0;
	rect->right = *width;
	rect->bottom = *height;
	AdjustWindowRectEx(rect, WindowStyle, false, 0);

	if (fullscreen)
	{
		CenterX = 0;
		CenterY = 0;
	}
	else
	{
		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &workArea, 0);
		int workWidth = workArea.right - workArea.left;
		int workHeight = workArea.bottom - workArea.top;

		// if height/width matches physical screen height/width, adjust it to available desktop size
		// and allow 2 pixels on top for the title bar so the window can be moved
		const int titleBarPixels = 2;
		if (*width == GetSystemMetrics(SM_CXSCREEN) && (*height == GetSystemMetrics(SM_CYSCREEN) || *height == workHeight - titleBarPixels))
		{
			rect->right -= *width - workWidth;
			*width = mode->width = workWidth;
			rect->bottom -= *height - (workHeight - titleBarPixels);
			*height = mode->height = workHeight - titleBarPixels;
			CenterX = 0;
			CenterY = titleBarPixels;
		}
		else
		{
			CenterX = max(0, (workWidth - *width) / 2);
			CenterY = max(0, (workHeight - *height) / 2);
		}
	}

	// x and y may be changed by WM_MOVE messages
	window_x = CenterX;
	window_y = CenterY;
	rect->left += CenterX;
	rect->right += CenterX;
	rect->top += CenterY;
	rect->bottom += CenterY;
}



qbool VID_InitMode(viddef_mode_t *mode)
{
	return VID_InitModeGL(mode);
}


static void IN_Shutdown(void);
void VID_Shutdown (void)
{
	qbool isgl;
	if(vid_initialized == false)
		return;

	VID_EnableJoystick(false);
	VID_SetMouse(false, false, false);
	VID_RestoreSystemGamma();

	vid_initialized = false;
	isgl = gldll != NULL;
	IN_Shutdown();
	gl_driver[0] = 0;
	gl_extensions = "";
	gl_platform = "";
	gl_platformextensions = "";

	if (qwglMakeCurrent)
		qwglMakeCurrent(NULL, NULL);
	qwglMakeCurrent = NULL;
	if (baseRC && qwglDeleteContext)
		qwglDeleteContext(baseRC);
	qwglDeleteContext = NULL;
	// close the library before we get rid of the window
	GL_CloseLibrary();
	if (baseDC && mainwindow)
		ReleaseDC(mainwindow, baseDC);
	baseDC = NULL;
	AppActivate(false, false);
	if (mainwindow)
		DestroyWindow(mainwindow);
	mainwindow = 0;
	if (vid_isfullscreen && isgl)
		ChangeDisplaySettings (NULL, CDS_FULLSCREEN);
	vid_isfullscreen = false;
}

void VID_SetMouse(qbool fullscreengrab, qbool relative, qbool hidecursor)
{
	static qbool restore_spi;
	static int originalmouseparms[3];

	if (!mouseinitialized)
		return;

	if (!vid.factive/*fActive*/) {
		if (fullscreengrab || relative || hidecursor) {
			//int j = 5;

		}

	}

	// relative? demoplay?
	if (relative) {
		if (!vid_usingmouse) {
			vid_usingmouse = true;
			cl_ignoremousemoves = 2;
#ifdef SUPPORTDIRECTX
			if (dinput && g_pMouse) {
				IDirectInputDevice_Acquire(g_pMouse);
				dinput_acquired = true;
			} else
#endif
			{
				RECT window_rect;
				//WINDOWINFO windowinfo;
				//windowinfo.cbSize = sizeof (WINDOWINFO);
				//GetWindowInfo (mainwindow, &windowinfo);	// client_area screen coordinates
				window_rect.left = window_x;
				window_rect.top = window_y;
				window_rect.right = window_x + vid.width;
				window_rect.bottom = window_y + vid.height;

				// change mouse settings to turn off acceleration
// COMMANDLINEOPTION: Windows GDI Input: -noforcemparms disables setting of mouse parameters (not used with -dinput, windows only)
				if (!Sys_CheckParm ("-noforcemparms") && SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0)) {
					int newmouseparms[3];
					newmouseparms[0] = 0; // threshold to double movement (only if accel level is >= 1)
					newmouseparms[1] = 0; // threshold to quadruple movement (only if accel level is >= 2)
					newmouseparms[2] = 0; // maximum level of acceleration (0 = off)
					restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0) != FALSE;
				}
				else
					restore_spi = false;
				SetCursorPos ((window_x + vid.width / 2), (window_y + vid.height / 2));

				SetCapture (mainwindow);
				ClipCursor (&window_rect);
				//ClipCursor (&windowinfo.rcClient);

			}
		}
	}
	else {
		if (vid_usingmouse) {
			vid_usingmouse = false;
			cl_ignoremousemoves = 2;
#ifdef SUPPORTDIRECTX
			if (dinput_acquired) {
				IDirectInputDevice_Unacquire(g_pMouse);
				dinput_acquired = false;
			}
			else
#endif
			{
				// restore system mouseparms if we changed them
				if (restore_spi)
					SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);
				restore_spi = false;
				ClipCursor (NULL);
				ReleaseCapture ();
			}
		}
	}

	if (vid_usinghidecursor != hidecursor)
	{
		vid_usinghidecursor = hidecursor;
		ShowCursor (!hidecursor);
	}
}

void VID_BuildJoyState(vid_joystate_t *joystate)
{
	VID_Shared_BuildJoyState_Begin(joystate);
	VID_Shared_BuildJoyState_Finish(joystate);
}

void VID_EnableJoystick(qbool enable)
{
	int index = joy_enable.integer > 0 ? joy_index.integer : -1;
	qbool success = false;
	int sharedcount = 0;
	sharedcount = VID_Shared_SetJoystick(index);
	if (index >= 0 && index < sharedcount)
		success = true;

	// update cvar containing count of XInput joysticks
	if (joy_detected.integer != sharedcount)
		Cvar_SetValueQuick(&joy_detected, sharedcount);

	if (joy_active.integer != (success ? 1 : 0))
		Cvar_SetValueQuick(&joy_active, success ? 1 : 0);
}

#ifdef SUPPORTDIRECTX
/*
===========
IN_InitDInput
===========
*/
static qbool IN_InitDInput (void)
{
    HRESULT		hr;
	DIPROPDWORD	dipdw = {
		{
			sizeof(DIPROPDWORD),        // diph.dwSize
			sizeof(DIPROPHEADER),       // diph.dwHeaderSize
			0,                          // diph.dwObj
			DIPH_DEVICE,                // diph.dwHow
		},
		DINPUT_BUFFERSIZE,              // dwData
	};

	if (!hInstDI)
	{
		hInstDI = LoadLibrary("dinput.dll");

		if (hInstDI == NULL)
		{
			Con_Print("Couldn't load dinput.dll\n");
			return false;
		}
	}

	if (!pDirectInputCreate)
	{
		pDirectInputCreate = (HRESULT (__stdcall *)(HINSTANCE,DWORD,LPDIRECTINPUT *,LPUNKNOWN))GetProcAddress(hInstDI,"DirectInputCreateA");

		if (!pDirectInputCreate)
		{
			Con_Print("Couldn't get DI proc addr\n");
			return false;
		}
	}

// register with DirectInput and get an IDirectInput to play with.
	hr = iDirectInputCreate(global_hInstance, DIRECTINPUT_VERSION, &g_pdi, NULL);

	if (FAILED(hr))
	{
		return false;
	}

// obtain an interface to the system mouse device.
#ifdef __cplusplus
	hr = IDirectInput_CreateDevice(g_pdi, GUID_SysMouse, &g_pMouse, NULL);
#else
	hr = IDirectInput_CreateDevice(g_pdi, &GUID_SysMouse, &g_pMouse, NULL);
#endif

	if (FAILED(hr))
	{
		Con_Print("Couldn't open DI mouse device\n");
		return false;
	}

// set the data format to "mouse format".
	hr = IDirectInputDevice_SetDataFormat(g_pMouse, &c_dfDIMouse);

	if (FAILED(hr))
	{
		Con_Print("Couldn't set DI mouse format\n");
		return false;
	}

// set the cooperativity level.
	hr = IDirectInputDevice_SetCooperativeLevel(g_pMouse, mainwindow,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(hr))
	{
		Con_Print("Couldn't set DI coop level\n");
		return false;
	}


// set the buffer size to DINPUT_BUFFERSIZE elements.
// the buffer size is a DWORD property associated with the device
	hr = IDirectInputDevice_SetProperty(g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
	{
		Con_Print("Couldn't set DI buffersize\n");
		return false;
	}

	return true;
}
#endif


/*
===========
IN_StartupMouse
===========
*/
static void IN_StartupMouse (void)
{
	if (Sys_CheckParm ("-nomouse"))
		return;

	mouseinitialized = true;

#ifdef SUPPORTDIRECTX
// COMMANDLINEOPTION: Windows Input: -dinput enables DirectInput for mouse input
	if (Sys_CheckParm ("-dinput"))
		dinput = IN_InitDInput ();

	if (dinput)
		Con_Print("DirectInput initialized\n");
	else
		Con_Print("DirectInput not initialized\n");
#endif

	mouse_buttons = 10;
}


/*
===========
IN_MouseMove
===========
*/
static void IN_MouseMove (void)
{
	POINT current_pos;

	GetCursorPos (&current_pos);
	in_windowmouse_x = current_pos.x - window_x;
	in_windowmouse_y = current_pos.y - window_y;

	if (!vid_usingmouse)
		return;

#ifdef SUPPORTDIRECTX
	if (dinput_acquired)
	{
		int i;
		DIDEVICEOBJECTDATA	od;
		DWORD				dwElements;
		HRESULT				hr;

		for (;;)
		{
			dwElements = 1;

			hr = IDirectInputDevice_GetDeviceData(g_pMouse,
					sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
			{
				IDirectInputDevice_Acquire(g_pMouse);
				break;
			}

			/* Unable to read data or no data available */
			if (FAILED(hr) || dwElements == 0)
				break;

			/* Look at the element to see what happened */

			if ((int)od.dwOfs == DIMOFS_X)
				in_mouse_x += (LONG) od.dwData;
			if ((int)od.dwOfs == DIMOFS_Y)
				in_mouse_y += (LONG) od.dwData;
			if ((int)od.dwOfs == DIMOFS_Z)
			{
				if((LONG)od.dwData < 0)
				{
					Key_Event(K_MWHEELDOWN, 0, true);
					Key_Event(K_MWHEELDOWN, 0, false);
				}
				else if((LONG)od.dwData > 0)
				{
					Key_Event(K_MWHEELUP, 0, true);
					Key_Event(K_MWHEELUP, 0, false);
				}
			}
			if ((int)od.dwOfs == DIMOFS_BUTTON0)
				mstate_di = (mstate_di & ~1) | ((od.dwData & 0x80) >> 7);
			if ((int)od.dwOfs == DIMOFS_BUTTON1)
				mstate_di = (mstate_di & ~2) | ((od.dwData & 0x80) >> 6);
			if ((int)od.dwOfs == DIMOFS_BUTTON2)
				mstate_di = (mstate_di & ~4) | ((od.dwData & 0x80) >> 5);
			if ((int)od.dwOfs == DIMOFS_BUTTON3)
				mstate_di = (mstate_di & ~8) | ((od.dwData & 0x80) >> 4);
		}

		// perform button actions
		for (i=0 ; i<mouse_buttons && i < 16 ; i++)
			if ((mstate_di ^ mouse_oldbuttonstate) & (1<<i))
				Key_Event (buttonremap[i], 0, (mstate_di & (1<<i)) != 0);
		mouse_oldbuttonstate = mstate_di;
	}
	else
#endif
	{
		in_mouse_x += in_windowmouse_x - (int)(vid.width / 2);
		in_mouse_y += in_windowmouse_y - (int)(vid.height / 2);

		// if the mouse has moved, force it to the center, so there's room to move
		if (in_mouse_x || in_mouse_y)
			SetCursorPos ((window_x + vid.width / 2), (window_y + vid.height / 2));
	}
}


/*
===========
IN_Move
===========
*/
void IN_Move (void)
{
	vid_joystate_t joystate;
	if (vid_activewindow && !vid_reallyhidden)
		IN_MouseMove ();
	VID_EnableJoystick(true);
	VID_BuildJoyState(&joystate);
	VID_ApplyJoyState(&joystate);
}


static void IN_Init(void)
{
	uiWheelMessage = RegisterWindowMessage ( "MSWHEEL_ROLLMSG" );
	Cvar_RegisterVariable (&vid_forcerefreshrate);
}

static void IN_Shutdown(void)
{
#ifdef SUPPORTDIRECTX
	if (g_pMouse)
		IDirectInputDevice_Release(g_pMouse);
	g_pMouse = NULL;

	if (g_pdi)
		IDirectInput_Release(g_pdi);
	g_pdi = NULL;
#endif
}

vid_mode_t *VID_GetDesktopMode(void)
{
	return &desktop_mode;
}

size_t VID_ListModes(vid_mode_t *modes, size_t maxcount)
{
	int i;
	size_t k;
	DEVMODE thismode;

	thismode.dmSize = sizeof(thismode);
	thismode.dmDriverExtra = 0;
	k = 0;
	for(i = 0; EnumDisplaySettings(NULL, i, &thismode); ++i)
	{
		if(~thismode.dmFields & (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY))
		{
			Con_DPrintf("enumerating modes yielded a bogus item... please debug this\n");
			continue;
		}
		if(k >= maxcount)
			break;
		modes[k].width = thismode.dmPelsWidth;
		modes[k].height = thismode.dmPelsHeight;
		modes[k].bpp = thismode.dmBitsPerPel;
		modes[k].refreshrate = thismode.dmDisplayFrequency;
		modes[k].pixelheight_num = 1;
		modes[k].pixelheight_denom = 1; // Win32 apparently does not provide this (FIXME)
		++k;
	}
	return k;
}

// Baker 1014
#if 1
#include "Shellapi.h" // Never needed this before?
#endif

// DPI Awareness.

typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
typedef HRESULT (WINAPI *SetProcessDPIAwarenessFunc)(dpi_awareness value);

//static void System_SetDPIAware (void)
void Sys_Platform_Init_DPI(void) // Windows DPI awareness
{
	HMODULE hUser32, hShcore;
	SetProcessDPIAwarenessFunc setDPIAwareness;
	SetProcessDPIAwareFunc setDPIAware;

	/* Neither SDL 1.2 nor SDL 2.0.3 can handle the OS scaling our window.
	  (e.g. https://bugzilla.libsdl.org/show_bug.cgi?id=2713)
	  Call SetProcessDpiAwareness/SetProcessDPIAware to opt out of scaling.
	*/

	hShcore = LoadLibraryA ("Shcore.dll");
	hUser32 = LoadLibraryA ("user32.dll");
	setDPIAwareness = (SetProcessDPIAwarenessFunc) (hShcore ? GetProcAddress (hShcore, "SetProcessDpiAwareness") : NULL);
	setDPIAware = (SetProcessDPIAwareFunc) (hUser32 ? GetProcAddress (hUser32, "SetProcessDPIAware") : NULL);

	if (setDPIAwareness) /* Windows 8.1+ */
		setDPIAwareness (dpi_monitor_aware);
	else if (setDPIAware) /* Windows Vista-8.0 */
		setDPIAware ();

	if (hShcore)
		FreeLibrary (hShcore);
	if (hUser32)
		FreeLibrary (hUser32);
}

extern int iszircicon;
void VID_Init(void)
{
	Sys_Platform_Init_DPI();

	InitCommonControls();
#if 1
	//HINSTANCE		hInst = GetModuleHandle(NULL);
	HICON			hIconx = iszircicon ?
						LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON1)) :
						LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));	// Baker 1000.1

	HCURSOR			hCursor =LoadCursor (NULL,IDC_ARROW);
	WNDCLASS		wc = {CS_HREDRAW | CS_VREDRAW, (WNDPROC)MainWndProc, 0, 0, global_hInstance, hIconx, hCursor,
		/*background*/ NULL, /*menu name*/ NULL, WIN_CLASSNAME_CLASS1};
#endif



	if (!RegisterClass (&wc))
		Con_Printf ("Couldn't register window class\n");

	memset(&initialdevmode, 0, sizeof(initialdevmode));
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &initialdevmode);

	vid.desktop_width = desktop_mode.width = initialdevmode.dmPelsWidth;
	vid.desktop_height =desktop_mode.height = initialdevmode.dmPelsHeight;
	desktop_mode.bpp = initialdevmode.dmBitsPerPel;
	desktop_mode.refreshrate = initialdevmode.dmDisplayFrequency;
	desktop_mode.pixelheight_num = 1;
	desktop_mode.pixelheight_denom = 1; // Win32 apparently does not provide this (FIXME)

	IN_Init();
}

#endif // defined(_WIN32) && !defined(CORE_SDL)