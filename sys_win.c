#if defined(_WIN32) && !defined(CORE_SDL)
// Baker: command line .txt support
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
// sys_win.c -- Win32 system interface code
#ifdef SUPPORTDIRECTX
#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined
#endif

#include <windows.h>
#include <mmsystem.h>
#include <direct.h>
#ifdef SUPPORTDIRECTX
#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined
#include <dsound.h>
#endif

#include "qtypes.h"

#include "darkplaces.h"
#include <errno.h>
#include "resource.h"
#include "conproc.h"

HANDLE				hinput, houtput;

#ifdef QHOST
static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;
#endif


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[MAX_INPUTLINE];
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error2 = 0;
	static int	in_sys_error3 = 0;

	va_start (argptr, error);
	dpvsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", text);

	// close video so the message box is visible, unless we already tried that
	if (!in_sys_error0 && cls.state != ca_dedicated)
	{
		in_sys_error0 = 1;
		VID_Shutdown();
	}

	if (!in_sys_error3 && cls.state != ca_dedicated)
	{
		in_sys_error3 = true;
		MessageBox(NULL, text, "Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown ();
	}

// shut down QHOST hooks if necessary
	if (!in_sys_error2)
	{
		in_sys_error2 = 1;
		Sys_Shutdown ();
	}

	exit (1);
}

void Sys_Shutdown (void)
{
#ifdef QHOST
	if (tevent)
		CloseHandle (tevent);
#endif

	if (cls.state == ca_dedicated)
		FreeConsole ();

#ifdef QHOST
// shut down QHOST hooks if necessary
	DeinitConProc ();
#endif
}

void Sys_PrintToTerminal(const char *text)
{
	DWORD dummy;
	extern HANDLE houtput;

	//OutputDebugString(text);
	if ((houtput != 0) && (houtput != INVALID_HANDLE_VALUE))
		WriteFile(houtput, text, (DWORD) strlen(text), &dummy, NULL);
}

#ifdef _DEBUG
void Sys_PrintToTerminal2(const char *text)
{
//	DWORD dummy;
	extern HANDLE houtput;

	OutputDebugString(text);
	OutputDebugString("\n");
//	if ((houtput != 0) && (houtput != INVALID_HANDLE_VALUE))
//		WriteFile(houtput, text, (DWORD) strlen(text), &dummy, NULL);
}
#endif

char *Sys_ConsoleInput (void)
{
	static char text[MAX_INPUTLINE];
	static int len;
	INPUT_RECORD recs[1024];
	int ch;
	DWORD numread, numevents, dummy;

	if (cls.state != ca_dedicated)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
		{
			cls.state = ca_disconnected;
			Sys_Error ("Error getting # of console events (error code %x)", (unsigned int)GetLastError());
		}

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
		{
			cls.state = ca_disconnected;
			Sys_Error ("Error reading console input (error code %x)", (unsigned int)GetLastError());
		}

		if (numread != 1)
		{
			cls.state = ca_disconnected;
			Sys_Error ("Couldn't read console input (error code %x)", (unsigned int)GetLastError());
		}

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= (int) (unsigned char) ' ')
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							text[len] = ch;
							len = (len + 1) & 0xff;
						}

						break;

				}
			}
		}
	}

	return NULL;
}

int Sys_SetClipboardData(const char *text_to_clipboard)
{
	char *clipboard_text;
	HGLOBAL hglbCopy;
	size_t len = strlen(text_to_clipboard) + 1;

	if (!OpenClipboard(NULL))
		return false;

	if (!EmptyClipboard()) {
		CloseClipboard();
		return false;
	}

	if ((hglbCopy = GlobalAlloc(GMEM_DDESHARE, len + 1)) == 0) {
		CloseClipboard();
		return false;
	}

	if ((clipboard_text = (char *)GlobalLock(hglbCopy)) == 0) {
		CloseClipboard();
		return false;
	}

	strlcpy ((char *) clipboard_text, text_to_clipboard, len);
	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_TEXT, hglbCopy);

	CloseClipboard();
	return true;

}

char *Sys_GetClipboardData (void)
{
	static char sbuf[MAX_INPUTLINE /*16384*/];
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard (NULL) != 0) {
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData (CF_TEXT)) != 0) {
			if ((cliptext = (char *)GlobalLock (hClipboardData)) != 0) {
				//size_t allocsize;
				size_t slen = GlobalSize (hClipboardData) + 1;
				//data = (char *)Z_Malloc (allocsize);

				int slen2 = (int) Smallest( (MAX_INPUTLINE - 1), slen) ;

				strlcpy (sbuf, cliptext, slen2);
				GlobalUnlock (hClipboardData);

				data = sbuf;
			}
		}
		CloseClipboard ();
	}
	return data;
}

void Sys_InitConsole (void)
{
#ifdef QHOST
	int t;

	// initialize the windows dedicated server console if needed
	tevent = CreateEvent(NULL, false, false, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");
#endif

	houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	hinput = GetStdHandle (STD_INPUT_HANDLE);

	// LadyHavoc: can't check cls.state because it hasn't been initialized yet
	// if (cls.state == ca_dedicated)
	if (Sys_CheckParm("-dedicated"))
	{
		//if ((houtput == 0) || (houtput == INVALID_HANDLE_VALUE)) // LadyHavoc: on Windows XP this is never 0 or invalid, but hinput is invalid
		{
			if (!AllocConsole ())
				Sys_Error ("Couldn't create dedicated server console (error code %x)", (unsigned int)GetLastError());
			houtput = GetStdHandle (STD_OUTPUT_HANDLE);
			hinput = GetStdHandle (STD_INPUT_HANDLE);
		}
		if ((houtput == 0) || (houtput == INVALID_HANDLE_VALUE))
			Sys_Error ("Couldn't create dedicated server console");


#ifdef QHOST
#ifdef _WIN64
#define atoi _atoi64
#endif
	// give QHOST a chance to hook into the console
		if ((t = Sys_CheckParm ("-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = Sys_CheckParm ("-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = Sys_CheckParm ("-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)atoi (com_argv[t+1]);
		}

		InitConProc (hFile, heventParent, heventChild);
#endif
	}

// because sound is off until we become active
	S_BlockSound ();
}

/*
==============================================================================

WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
const char	*argv[MAX_NUM_ARGVS];
char		program_name[MAX_OSPATH];

// Baker 1015
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MEMORYSTATUS lpBuffer;
	char	cmdline_fake[MAX_INPUTLINE];
	char	*com_cmdline = lpCmdLine;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return 0;

	global_hInstance = hInstance;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	program_name[sizeof(program_name)-1] = 0;
	GetModuleFileNameA(NULL, program_name, sizeof(program_name) - 1);

	com_argc = 1;
	com_argv = argv;
	argv[0] = program_name;

	// Baker: I would prefer this to be in a platform neutral place
	// On Windows, arg 0 is exe name.  If we only have 1 arg, there is no command line.
	if (lpCmdLine[0] == 0) {
		const char *s_fp = "zircon_command_line.txt"; // File_Getcwd_SBuf not needed
		if (File_Exists (s_fp)) {
			size_t s_temp_size = 0; const char *s_temp = (const char *)File_To_String_Alloc (s_fp, &s_temp_size);
			if (s_temp)
				c_strlcpy (cmdline_fake, s_temp);
			else cmdline_fake[0] = 0; // Enough?
			String_Edit_Whitespace_To_Space (cmdline_fake); // Make tabs, newlines into spaces.
			com_cmdline = cmdline_fake;
			free ((void *)s_temp);
		}
	}

	// FIXME: this tokenizer is rather redundent, call a more general one
	while (*com_cmdline && (com_argc < MAX_NUM_ARGVS))
	{
		while (*com_cmdline && ISWHITESPACE(*com_cmdline))
			com_cmdline++;

		if (!*com_cmdline)
			break;

		if (*com_cmdline == '\"')
		{
			// quoted string
			com_cmdline++;
			argv[com_argc] = com_cmdline;
			com_argc++;
			while (*com_cmdline && (*com_cmdline != '\"'))
				com_cmdline++;
		}
		else
		{
			// unquoted word
			argv[com_argc] = com_cmdline;
			com_argc++;
			while (*com_cmdline && !ISWHITESPACE(*com_cmdline))
				com_cmdline++;
		}

		if (*com_cmdline)
		{
			*com_cmdline = 0;
			com_cmdline++;
		}
	}

	Sys_ProvideSelfFD();

	Host_Main();

	/* return success of application */
	return true;
}

#if 0
// unused, this file is only used when building windows client and vid_wgl provides WinMain() instead
int main (int argc, const char* argv[])
{
	MEMORYSTATUS lpBuffer;

	global_hInstance = GetModuleHandle (0);

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	program_name[sizeof(program_name)-1] = 0;
	GetModuleFileNameA(NULL, program_name, sizeof(program_name) - 1);

	com_argc = argc;
	com_argv = argv;

	Host_Main();

	return true;
}
#endif

qbool sys_supportsdlgetticks = false;
unsigned int Sys_SDL_GetTicks (void)
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
	char *clipboard_text;
	HGLOBAL hglbCopy;
	size_t len = strlen(text_to_clipboard) + 1;

	if (!OpenClipboard(NULL))
		return false;

	if (!EmptyClipboard()) {
		CloseClipboard();
		return false;
	}

	if ((hglbCopy = GlobalAlloc(GMEM_DDESHARE, len + 1)) == 0) {
		CloseClipboard();
		return false;
	}

	if ((clipboard_text = (char *)GlobalLock(hglbCopy)) == 0) {
		CloseClipboard();
		return false;
	}

	strlcpy ((char *) clipboard_text, text_to_clipboard, len);
	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_TEXT, hglbCopy);

	CloseClipboard();
	return true;
}

#define MAX_OSPATH_EX 256   // Technically 260 +/-

SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash
{
	static char workingdir[MAX_OSPATH_EX];
	int ok = 0;

	if (GetCurrentDirectory (sizeof(workingdir), workingdir)) { // No trailing slash in this or getcwd
		if (workingdir[strlen(workingdir)-1] == '/')
			workingdir[strlen(workingdir)-1] = 0;

		File_URL_Edit_SlashesForward_Like_Unix (workingdir);

		ok = 1;
	}

	if (ok == 0) return NULL;
		//log_fatal ("Couldn't determine current directory");

	return workingdir;
}

static char *windows_style_url_alloc (const char *path_to_file)
{
	RETURNING_ALLOC___ char *windows_style_url_o = strdup (path_to_file);
	File_URL_Edit_SlashesBack_Like_Windows (windows_style_url_o);
	RETURNING___ return windows_style_url_o;
}

#include "Shellapi.h" // Never needed this before?
// Folder must exist.  It must be a folder.
int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
	AUTO_ALLOC___ char *windows_style_url_a = windows_style_url_alloc (path_to_file);
	int ret = ShellExecute(0, "Open", "explorer.exe", windows_style_url_a, NULL, SW_NORMAL) != 0;
	AUTO_FREE___ free (windows_style_url_a);
	return ret;
}

static int sShell_Clipboard_Set_Image_BGRA (const unsigned *bgra, int width, int height)
{
	HBITMAP hBitmap = CreateBitmap (width, height, 1, 32 /* bits per pixel is 32 */, bgra);

	OpenClipboard (NULL);

	if (EmptyClipboard()) {
		if ((SetClipboardData (CF_BITMAP, hBitmap)) == NULL) {
			//logd ("SetClipboardData failed"); // Was fatal.  But for clipboard?  Seriously?
			return false;
		}
	}

	CloseClipboard ();
	return true;
}

static void sSystem_Clipboard_Set_Image_RGBA_Maybe_Flip (const unsigned *rgba, int width, int height, int is_flip)
{
	int		pelscount = width * height;
	int		buffersize = pelscount * RGBA_4;
	byte    *bgra_data_a = (byte *)malloc (buffersize); // Clipboard From RGBA work
//	int		i;
//	byte	temp;

	memcpy (bgra_data_a, rgba, buffersize);

	// If flip ....
	if (is_flip)
		Image_Flip_Buffer (bgra_data_a, width, height, RGBA_4);

	// RGBA to BGRA so clipboard will take it
#if 1
	Image_Flip_RedGreen (bgra_data_a, width * height * RGBA_4);
#else
	for (i = 0 ; i < buffersize ; i += RGBA_4)
	{
		temp = bgra_data[i];

		bgra_data[i] = bgra_data[i + 2];
		bgra_data[i + 2] = temp;
	}
#endif

	sShell_Clipboard_Set_Image_BGRA ((unsigned *)bgra_data_a, width, height);
	free (bgra_data_a);
}

static int _Shell_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height)
{
	sSystem_Clipboard_Set_Image_RGBA_Maybe_Flip (rgba, width, height, false);
	return true;
}

// copies given image to clipbard
int Sys_Clipboard_Set_Image (unsigned *rgba, int width, int height)
{
	return _Shell_Clipboard_Set_Image_RGBA (rgba, width, height);
}

const char *Sys_Binary_URL_SBuf (void)
{
	static char binary_url[MAX_OSPATH];
	if (!binary_url[0]) {
		int length = GetModuleFileNameA (NULL, binary_url, sizeof(binary_url) - 1 );
		if (!length) {
			return NULL; //binary_url;
			//log_fatal ("Couldn't determine executable directory");
		}

		//MSDN: Windows XP:  The string is truncated to nSize characters and is not null-terminated.
		binary_url[length] = 0;
		File_URL_Edit_SlashesForward_Like_Unix (binary_url);
	}
	return binary_url;
}
#endif // defined(_WIN32) && !defined(CORE_SDL)