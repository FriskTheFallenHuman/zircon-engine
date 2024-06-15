#ifdef CORE_SDL

#ifdef _WIN32
	#include <io.h> // Include this BEFORE darkplaces.h because it uses strncpy which trips DP_STATIC_ASSERT
	#include "conio.h"
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/time.h>
#endif

#ifdef __ANDROID__
	#include <android/log.h>
#endif // __ANDROID__

#include <signal.h>

// Include this BEFORE darkplaces.h because it breaks wrapping
// _Static_assert. Cloudwalk has no idea how or why so don't ask.

#if defined(CODEBLOCKS_LINUX_IDE) || (defined(_MSC_VER) && _MSC_VER < 1900)
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif

#include "darkplaces.h"


#ifdef _WIN32
//	#include "quakedef.h" // Baker: Need cls here.

	#ifdef _MSC_VER
		#pragma comment(lib, "sdl2.lib")
		#pragma comment(lib, "sdl2main.lib")

		#pragma comment(lib, "shell32.lib")

	#endif // _MSC_VER

	unsigned *Sys_Clipboard_Get_Image_BGRA_Malloc (int *outwidth, int *outheight)
	{
	// Was extremely helpful info ... https://sites.google.com/site/michaelleesimons/clipboard
		byte *ptr = NULL;

		if (OpenClipboard(NULL)) {
			HBITMAP hBitmap = (HBITMAP)GetClipboardData (CF_BITMAP);
			BITMAP csBitmap;

			if (hBitmap && GetObject(hBitmap, sizeof(csBitmap), &csBitmap) && csBitmap.bmBitsPixel == BGRA_BPP_32) {
				// allocate buffer
				int bufsize = csBitmap.bmWidth * csBitmap.bmHeight * (csBitmap.bmBitsPixel / 8);

				csBitmap.bmBits = ptr = (byte *)malloc (bufsize); // "bmbits buffer"
				GetBitmapBits((HBITMAP)hBitmap, bufsize, csBitmap.bmBits );

#if 0 // We want BGRA
				// Convert BGRA --> RGBA, set alpha full since clipboard loses it somehow
				for (int i = 0; i < bufsize; i += RGBA_4)
				{
					byte temp = ptr[i + 0];
					ptr[i + 0] = ptr[i + 2];
					ptr[i + 2] = temp;
					ptr[i + 3] = 255; // Full alpha
				}
#endif
				*outwidth = csBitmap.bmWidth;
				*outheight = csBitmap.bmHeight;
			}
			CloseClipboard ();
		}
		return (unsigned *)ptr;
	}


	int Sys_Clipboard_Set_Image_BGRA_Is_Ok (const unsigned *bgra, int width, int height)
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
#else
	int Sys_Clipboard_Set_Image_BGRA_Is_Ok (const unsigned *bgra, int width, int height)
	{
		return false;
	}

	unsigned *Sys_Clipboard_Get_Image_BGRA_Malloc (int *outwidth, int *outheight)
	{
		return NULL;
	}

#endif //  _WIN32


// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
#ifdef __ANDROID__
	Sys_AllowProfiling(false);
#endif
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);
#endif
	fflush(stdout);
	SDL_Quit();
}

static qbool nocrashdialog;
/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char text[MAX_INPUTLINE_16384];

// change stdin to non blocking
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);
#endif

	va_start (argptr,error);
	dpvsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	Con_PrintLinef (CON_ERROR "Engine Error: %s", text);

	if (!nocrashdialog)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", text, NULL);

	//Host_Shutdown ();
	exit (1);
}

// Baker: This will actually print to the debug console in Visual Studio

#ifdef _WIN32
	#ifdef _DEBUG
		void DebugPrintf (const char *fmt, ...)
		{
			VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
			OutputDebugString (text);
			OutputDebugString ("\n");
			VA_EXPAND_ALLOC_FREE (text);
		}
	#endif
#endif


#ifdef _WIN32

	#ifdef CONFIG_MENU
		// Client run as dedicated server with -dedicated (-dedicated)
		void Sys_PrintToTerminal_WinQuake (const char *text)
		{
			DWORD dummy;
			extern HANDLE houtput;

			//OutputDebugString(text);
			if ((houtput != 0) && (houtput != INVALID_HANDLE_VALUE))
				WriteFile(houtput, text, (DWORD) strlen(text), &dummy, NULL);
		}

	#else
		// For a theoretical Windows dedicated server
		void Sys_PrintToTerminal_Win32 (const char *text)
		{
			if (sys.outfd < 0)
				return;

			#define write _write

			while (*text) {
				fs_offset_t written = (fs_offset_t)write(sys.outfd, text, (int)strlen(text));
				if (written <= 0)
					break; // sorry, I cannot do anything about this error - without an output
				text += written;
			} // while
		}
	#endif
#else
	// Baker: Got sick of all the #ifdefs
	void Sys_PrintToTerminal_Unix (const char *text)
	{
		if (sys.outfd < 0)
			return;

		// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
		// this is because both go to /dev/tty by default!

		int origflags = fcntl (sys.outfd, F_GETFL, 0);
		fcntl (sys.outfd, F_SETFL, origflags & ~O_NONBLOCK);

		while (*text) {
			fs_offset_t written = (fs_offset_t)write(sys.outfd, text, (int)strlen(text));
			if (written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		} // while

		fcntl (sys.outfd, F_SETFL, origflags);
	}

#endif // Sys_PrintToTerminal_Win32 vs. Sys_PrintToTerminal_Unix

void Sys_PrintToTerminal(const char *text)
{
	#ifdef _WIN32
	#ifdef CONFIG_MENU
		Sys_PrintToTerminal_WinQuake (text);
	#else
		Sys_PrintToTerminal_Win32 (text);
	#endif
	#elif defined(__ANDROID__)
		#define CORE_ANDROID_LOG_TAG "CoreMain"
		__android_log_print(ANDROID_LOG_INFO, CORE_ANDROID_LOG_TAG, "%s", text);
	#else
		Sys_PrintToTerminal_Unix (text);
	#endif
}

#ifdef _WIN32

HANDLE				hinput, houtput;

void Sys_Console_InitOnce_WinQuake (void)
{
	houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	hinput = GetStdHandle (STD_INPUT_HANDLE);

	// LadyHavoc: can't check cls.state because it hasn't been initialized yet
	// if (cls.state == ca_dedicated)
	if (Sys_CheckParm("-dedicated")) {
		//if ((houtput == 0) || (houtput == INVALID_HANDLE_VALUE)) // LadyHavoc: on Windows XP this is never 0 or invalid, but hinput is invalid
		{
			if (!AllocConsole ())
				Sys_Error ("Couldn't create dedicated server console (error code %x)", (unsigned int)GetLastError());
			houtput = GetStdHandle (STD_OUTPUT_HANDLE);
			hinput = GetStdHandle (STD_INPUT_HANDLE);
		}
		if ((houtput == 0) || (houtput == INVALID_HANDLE_VALUE))
			Sys_Error ("Couldn't create dedicated server console");

	}
}

char *Sys_ConsoleInput_WinQuake (void)
{
	static char text[MAX_INPUTLINE_16384];
	static int len;
	INPUT_RECORD recs[1024];
	DWORD numread, numevents, dummy;

	// Baker: We are 100% dedicated here
	// if (cls.state != ca_dedicated)
	//	return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
		{
			// cls.state = ca_disconnected;
			Sys_Error ("Error getting # of console events (error code %x)", (unsigned int)GetLastError());
		}

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread)) {
			//cls.state = ca_disconnected;
			Sys_Error ("Error reading console input (error code %x)", (unsigned int)GetLastError());
		}

		if (numread != 1) {
			//cls.state = ca_disconnected;
			Sys_Error ("Couldn't read console input (error code %x)", (unsigned int)GetLastError());
		}

		if (recs[0].EventType == KEY_EVENT ) {
			int ch = recs[0].Event.KeyEvent.uChar.AsciiChar;
			int dws = recs[0].Event.KeyEvent.dwControlKeyState;
			// LEFT_CTRL_PRESSED 0x0008 RIGHT_CTRL_PRESSED 0x0004

			if (recs[0].Event.KeyEvent.bKeyDown == 0) {
				if ( (ch == 22 || ch == 118) && (dws & LEFT_CTRL_PRESSED /*0x0008*/)) {
					int slen; const char *s; // CTRL-V hack
					s = Clipboard_Get_Text_Line_Static ();
					//logc ("Pasting %d", s);
					slen = (int)strlen(s);
					if (slen) {
						// We need to jam it into the text buffer
						int n; for (n = 0; n < slen; n ++) {
							ch = s[n];
							WriteFile (houtput, &ch, 1, &dummy, NULL);
							text[len] = ch, len = (len + 1) & 0xff; // Wrap
						} // end for
					} // if slen

					break;
				} // Pastey - Jan 27 2020 - CTRL-V paste hack
				continue;
			}

			switch (ch)
			{
			case '\r': // Enter via carriage return character
				WriteFile (houtput, "\r\n", 2, &dummy, NULL); // We need a carriage return?

				if (len) {
					text[len] = 0, len = 0;

					return text; // Strip trailing newline, reset length to 0.
				}
				else if (1) // sc_return_on_enter)
				{
				// special case to allow exiting from the error handler on Enter
					text[0] = '\r';
					text[1] = 0;
					len = 0;
					return text;
				}
				break;

			case '\b': // Backspace character
				WriteFile (houtput, "\b \b", 3, &dummy, NULL);
				if (len)
					len--;
				break;

			default:
				if (ch >= 32) {
					WriteFile (houtput, &ch, 1, &dummy, NULL);
					text[len] = ch, len = (len + 1) & 0xff; // Wrap
				}
				break;
			} // End switch
		} // Endif

	} // End of for

	return NULL;
}

#endif

char *Sys_ConsoleInput (void) // CONSOLUS
{
	static char text[MAX_INPUTLINE_16384];
	int len = 0;

#ifdef _WIN32
	int c;

	// read a line out
	while (_kbhit ()) {
		c = _getch ();
		_putch (c);

		if (c == '\r') {
			text[len] = 0;
			_putch ('\n');
			len = 0;
			return text;
		} // carriage return

		if (c == CHAR_BACKSPACE_8) {
			if (len) {
				_putch (' ');
				_putch (c);
				len--;
				text[len] = 0;
			}
			continue;
		} // backspace

		text[len] = c;
		len++;
		text[len] = 0;
		if (len == sizeof (text))
			len = 0;
	}
#else // !_WIN32
	fd_set fdset;
	struct timeval timeout;
	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
	{
		len = read (0, text, sizeof(text));
		if (len >= 1)
		{
			// rip off the \n and terminate
			text[len-1] = 0;
			return text;
		}
	}
#endif // !_WIN32
	return NULL;
}

WARP_X_ (Sys_GetClipboardData_Alloc)
// Returns 1 on success, 0 on failure
int Sys_SetClipboardData(const char *text_to_clipboard)
{
	return !SDL_SetClipboardText(text_to_clipboard);
}




char *Sys_GetClipboardData_Alloc (void)
{
	char *data = NULL;
	char *cliptext;

	cliptext = SDL_GetClipboardText();
	if (cliptext != NULL) {
		size_t allocsize;
		allocsize = min(MAX_INPUTLINE_16384, strlen(cliptext) + 1);
		data = (char *)Z_Malloc (allocsize);
		strlcpy (data, cliptext, allocsize);
		SDL_free(cliptext);
	}

	return data;
}

char *Sys_GetClipboardData_Unlimited_ZAlloc (void)
{
	char *data = NULL;
	char *cliptext;

	cliptext = SDL_GetClipboardText();
	if (cliptext != NULL) {
		data = Z_StrDup (cliptext);
		SDL_free(cliptext);
	}

	return data;
}

// Baker r8001: zircon_command_line.txt support
#define MAX_NUM_Q_ARGVS_50	50
static int fake_argc; char *fake_argv[MAX_NUM_Q_ARGVS_50];


int main (int argc, char *argv[])
{
	signal(SIGFPE, SIG_IGN);

#ifdef __ANDROID__
	Sys_AllowProfiling(true);
#endif

	sys.selffd = -1;
	sys.argc = argc;
	sys.argv = (const char **)argv;

	//
    // Baker: zircon_command_line.txt
    //

#ifdef CORE_XCODE // MACOSX
	// This is to make zircon_command_line.txt work for a Mac .app
    if (strstr(sys.argv[0], ".app/") && strstr(sys.argv[0], "/Xcode/") == NULL) {
        char *split;
        strlcpy(fs_basedir, com_argv[0], sizeof(fs_basedir));
        split = strstr(fs_basedir, ".app/");

        // Baker: find first '/' after .app, 0 it out
        while (split > fs_basedir && *split != '/')
            split--;
        *split = 0;

        // Change to dir of .app, should make zircon_command_line.txt work
        chdir (fs_basedir);
    } // if .app in name, should be 100%
#endif // XCODE

	if (sys.argc == 1 /*only the exe*/) {
		char	cmdline_fake[MAX_INPUTLINE_16384];

		const char *s_fp =
#ifdef __ANDROID__

			"/sdcard/zircon/"
#endif
			"zircon_command_line.txt";

		if (File_Exists (s_fp)) {
			size_t s_temp_size = 0; char *s_temp = (char *)File_To_String_Alloc (s_fp, &s_temp_size);
			if (String_Contains (s_temp, "//")) {
				char *s_start = (char *)strstr ( s_temp, "//");
				*s_start = 0; // null it out

			}
			c_strlcpy (cmdline_fake, "quake_engine "); // arg0 is engine and ignored by Sys_CheckParm
			if (s_temp) {
				c_strlcat (cmdline_fake, s_temp);
				String_Edit_Whitespace_To_Space (cmdline_fake); // Make tabs, newlines into spaces.

				String_Command_String_To_Argv (/*destructive edit*/ cmdline_fake, &fake_argc, fake_argv, MAX_NUM_Q_ARGVS_50);
				sys.argc = fake_argc;
				sys.argv = (const char **)fake_argv;

				free ((void *)s_temp);
			} // if
		} // if exists
	} // if 1 cmd arg the exe

	//
    // End Baker: zircon_command_line.txt
    //


	// Sys_Error this early in startup might screw with automated
	// workflows or something if we show the dialog by default.
	nocrashdialog = true;

	Sys_ProvideSelfFD(); // Baker: windows is setting sys.selffd to 3 so we have a handle

	// COMMANDLINEOPTION: -nocrashdialog disables "Engine Error" crash dialog boxes
	if (!Sys_CheckParm("-nocrashdialog"))
		nocrashdialog = false;
	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if (Sys_CheckParm("-noterminal"))
		sys.outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if (Sys_CheckParm("-stderr"))
		sys.outfd = 2;
	else
		sys.outfd = 1;

#ifndef _WIN32
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | O_NONBLOCK);
#endif // !_WIN32

	// we don't know which systems we'll want to init, yet...
	SDL_Init(0);

	// used by everything
	Memory_Init();

	Host_Main();

	Sys_Quit(0);

	return 0;
}

qbool sys_supportsdlgetticks = true;
unsigned int Sys_SDL_GetTicks (void)
{
	return SDL_GetTicks();
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	SDL_Delay(milliseconds);
}

#ifdef _WIN32
sys_handle_t System_Process_Create (const char *path_to_file_unix, const char *args_not_unix_but_your_os, /*optional*/ const char *working_directory_url_unix)
{
	char	windows_path_to_file[MAX_OSPATH];
	char	windows_working_dir[MAX_OSPATH];

	// Sanity check
	if (File_Exists (path_to_file_unix) == false) {
		// logd ("path_to_file_unix " QUOTED_S " doesn't exist", path_to_file_unix);
		return NULL;
	}

	if (working_directory_url_unix && File_Exists (working_directory_url_unix) == false) {
		// logd ("working_directory_url_unix " QUOTED_S " doesn't exist", working_directory_url_unix);
		return NULL;
	}

	// 1. Construct windows version: dest: windows_path_to_file src: path_to_file_unix (supplied)
	c_strlcpy (windows_path_to_file, path_to_file_unix);
	File_URL_Edit_SlashesBack_Like_Windows (windows_path_to_file); // Need to "Windows-ize" for WIN32 call

	// If working directory specified, use that otherwise use binary's path as working dir
	if (working_directory_url_unix) {
		c_strlcpy (windows_working_dir, working_directory_url_unix); // So we can Windows-ize the path (c:/mypath --> c:\mypath)
		// UNIX slashes supplied by function
	}
	else  // No working directory provided, working directory is the path of the windows_path_to_file
	{
		c_strlcpy (windows_working_dir, windows_path_to_file);
		File_URL_Edit_Reduce_To_Parent_Path (windows_working_dir); // Strip off file name to parent path.
		File_URL_Edit_SlashesBack_Like_Windows (windows_working_dir);
		// Converted binary path to Unix Slashes
	}

	if (String_Ends_With_Caseless (windows_working_dir, "/")) { // TRAILING SLASH REMOVER except we own windows_working_dir
		int slen = (int)strlen (windows_working_dir);
		if (slen <= (int)sizeof(windows_working_dir /*1024*/)) { // Otherwise it is truncated ... which is bad.
			windows_working_dir[slen - 1] = 0; // String len is 10, we null at 9 to reduce length to 8.
		}
	} // May 18 2020 - Remove trailing slash V2


	{
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi;
		char 		command_line[1024];
		int ret;

		// Construct command line ...
		c_dpsnprintf2 (command_line, QUOTED_S " " "%s", windows_path_to_file, args_not_unix_but_your_os);

		//logd ("Process command line (native format) " QUOTED_S, command_line); // Get rid of this after testing
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE /*SW_SHOWMINNOACTIVE*/;
		si.dwFlags = STARTF_USESHOWWINDOW;

		ret = CreateProcess (
			NULL, 					//  _In_opt_     LPCTSTR lpApplicationName,
			command_line,			//  _Inout_opt_  LPTSTR lpCommandLine,
			NULL,					//  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
			NULL,					//  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
			FALSE,					//  _In_         BOOL bInheritHandles,
			GetPriorityClass(GetCurrentProcess()),
									//  _In_         DWORD dwCreationFlags,
			NULL,					//  _In_opt_     LPVOID lpEnvironment,
			windows_working_dir,	//  _In_opt_     LPCTSTR lpCurrentDirectory,
			&si, 					//  _In_         LPSTARTUPINFO lpStartupInfo,
			&pi						//  _Out_        LPPROCESS_INFORMATION lpProcessInformation
		);

		if (ret) return pi.hProcess; // Return the handle
	}
	//logd ("Create process failed");
	return 0;
}

// Don't give this a null pid
int System_Process_Is_Still_Running_Neg1_Error (sys_handle_t pid, /*reply*/ int *p_exit_code)
{
	DWORD ExitCode;

	// DEBUG_ASSERT (pid);
	if (pid == NULL)
		return not_found_neg1;

	if (GetExitCodeProcess((HANDLE)pid, &ExitCode) == false) {
		return not_found_neg1; // Error
	}

	if (ExitCode == STILL_ACTIVE) // 0x00000103L
		return true; // Still running

	NOT_MISSING_ASSIGN (p_exit_code, ExitCode);

	return false; // No longer running; completed
}


// ?
qbool System_Process_Terminate_Did_Terminate_Console_App (sys_handle_t pid)
{
	if (TerminateProcess ((HANDLE)pid, 0))
		return true;

	return false;
}


qbool System_Process_Close_Did_Close (sys_handle_t pid)
{
	// Send WM_CLOSE to ask process to quit
	if (CloseHandle ( (HANDLE)pid)) {
		return true;
	}

	return false;
}


#include <shellapi.h>
qbool Sys_ShellExecute (const char *s)
{

//	char s_cmd[MAX_INPUTLINE_16384];

	size_t result = (size_t)ShellExecute (NULL,
             NULL,
             s /*"C:\\WINDOWS\\System32\\CALC.EXE"*/,
             NULL,
             NULL,
             SW_SHOWDEFAULT);

	// If the function succeeds, it returns a value greater than 32.
	return (result > 32);

}

#else
#if 0
///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: PROCESSES
///////////////////////////////////////////////////////////////////////////////

// Changes working directory
// execlp, realpath, waitpid
sys_handle_t System_Process_Create (const char *path_to_file_unix, const char *args_not_unix_but_your_os, optional const char* working_directory_url_unix)
{
	return (sys_handle_t)0;
}

// April 25 2021 - TODO
int System_Process_Is_Still_Running_Neg1_Error (sys_handle_t pid, reply EXIT_CODE_ZERO_IS_OK *p_exit_code)
{
	return false; // No longer running; completed
}

cbool System_Process_Close_Did_Close (sys_handle_t pid)
{
	return 0;
}
#endif

#endif

// Baker r3102: "folder" command
#ifdef _WIN32
	#include "Shellapi.h" // Never needed this before?
	// Folder must exist.  It must be a folder.
	static char *windows_style_url_alloc (const char *path_to_file)
	{
		RETURNING_ALLOC___ char *windows_style_url_o = strdup (path_to_file);
		File_URL_Edit_SlashesBack_Like_Windows (windows_style_url_o);
		RETURNING___ return windows_style_url_o;
	}

	int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
	{
		AUTO_ALLOC___ char *windows_style_url_a = windows_style_url_alloc (path_to_file);
		int ret = ShellExecute(0, "Open", "explorer.exe", windows_style_url_a, NULL, SW_NORMAL) != 0;
		AUTO_FREE___ free (windows_style_url_a);
		return ret;
	}
#else // !_WIN32
	// Folder must exist.  It must be a folder.
	int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
	{
#ifdef CORE_XCODE // MACOSX
        int _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file);
        return _Shell_Folder_Open_Folder_Must_Exist(path_to_file);
#else
	//  xdg-open is a desktop-independent tool for configuring the default applications of a user
		if ( fork() == 0) {
			execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
			//cleanup_on_exit();  /* clean up before exiting */
			exit(3);
		}
#endif // LINUX etc
		return true;
	}
#endif // _WIN32

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
#ifdef CONFIG_MENU	// We are using this as the litmus test that we have a client
	return SDL_SetClipboardText(text_to_clipboard) == 0;
#else
	return 0;
#endif

}



#ifdef _WIN32
	// empty
	SBUF___ char *Sys_Getcwd_SBuf (void) // No trailing slash
	{
		static char workingdir[MAX_OSPATH_EX_1024];
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
#else // ! _WIN32



	SBUF___ char *Sys_Getcwd_SBuf (void) // No trailing slash

	{
		static char workingdir[MAX_OSPATH_EX_1024];

		if (getcwd (workingdir, sizeof(workingdir) - 1)) {
			return workingdir;
		} else return NULL;
	}

	const char *Sys_Binary_URL_SBuf (void)
	{
		static char binary_url[MAX_OSPATH_EX_1024];
		if (!binary_url[0]) {
			pid_t pid = getpid();
			int length;

			char linkname[MAX_OSPATH_EX_1024];
			dpsnprintf (linkname, sizeof(linkname), "/proc/%d/exe", pid);

			length = readlink (linkname, binary_url, sizeof(binary_url)-1);

    		// In case of an error, leave the handling up to the caller
			if (length == -1 || length >= (int)sizeof(binary_url) ) {
				//log_fatal ("Couldn't determine executable directory");
				return NULL; //binary_url;
			}

    		binary_url[length] = 0;
		}
		return binary_url;
	}
#endif // _WIN32

#endif // CORE_SDL
