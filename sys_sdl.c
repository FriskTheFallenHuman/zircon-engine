#ifdef CORE_SDL

#ifdef _WIN32
	#include <io.h>
	#include "conio.h"
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/time.h>
#endif

#ifdef __ANDROID__
	#include <android/log.h>

	#ifndef FNDELAY
		#define FNDELAY		O_NDELAY
	#endif
#endif // __ANDROID__

#include <signal.h>

#if defined(_MSC_VER) || defined(CORE_XCODE)
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif // _MSC_VER


#ifdef _WIN32
	#ifdef _MSC_VER
			#pragma comment(lib, "sdl2.lib")
			#pragma comment(lib, "sdl2main.lib")

	#endif // _MSC_VER

#endif //  _WIN32

#include "darkplaces.h"

// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
#ifdef __ANDROID__
	Sys_AllowProfiling(false);
#endif
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
	SDL_Quit();
}


void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

#ifdef _WIN32
	MessageBox(NULL, string, "Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
#endif

	Host_Shutdown ();
	exit (1);
}

static int outfd = 1;
void Sys_PrintToTerminal(const char *text)
{
#ifdef __ANDROID__
	//if (developer.integer > 0)
	//{
#define CORE_ANDROID_LOG_TAG "CoreMain"
		//__android_log_write(ANDROID_LOG_INFO, CORE_ANDROID_LOG_TAG, text);
	__android_log_print(ANDROID_LOG_INFO, CORE_ANDROID_LOG_TAG, "%s", text);
#else // ! __ANDROID__
		if(outfd < 0)
			return;
	#ifdef FNDELAY
		// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
		// this is because both go to /dev/tty by default!
		{
			int origflags = fcntl (outfd, F_GETFL, 0);
			fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
	#endif // FNDELAY
	#ifdef _WIN32
		#define write _write
	#endif // _WIN32
			while(*text)
			{
				fs_offset_t written = (fs_offset_t)write(outfd, text, (int)strlen(text));
				if(written <= 0)
					break; // sorry, I cannot do anything about this error - without an output
				text += written;
			}
	#ifdef FNDELAY
			fcntl (outfd, F_SETFL, origflags);
		}
	#endif // FNDELAY
		//fprintf(stdout, "%s", text);
#endif // __ANDROID__
}

char *Sys_ConsoleInput(void)
{
		static char text[MAX_INPUTLINE];
		int len = 0;
#ifdef _WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			_putch (c);
			if (c == '\r')
			{
				text[len] = 0;
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == 8)
			{
				if (len)
				{
					_putch (' ');
					_putch (c);
					len--;
					text[len] = 0;
				}
				continue;
			}
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
#endif // ! _WIN32
	return NULL;
}


// This must return Z_Malloc text

char *Sys_GetClipboardData (void)
{

#ifdef CONFIG_MENU
	static char sbuf[MAX_INPUTLINE /*16384*/];
	char *_text = SDL_GetClipboardText();  //(!SDL_HasClipboardText())
	if (_text) {
		int slen = strlen(_text);
		int slen2 = Smallest( (MAX_INPUTLINE - 1), slen);
//		char *text_out  (char *)Z_Malloc (slen + ONE_CHAR_1);
		strlcpy (sbuf, _text, slen2 + ONE_CHAR_1);
		return sbuf;
	}
	return NULL;
#else
	return NULL;
#endif
}

WARP_X_ (Sys_GetClipboardData)
int Sys_SetClipboardData(const char *text_to_clipboard)
{
	return !SDL_SetClipboardText(text_to_clipboard);
}

void Sys_InitConsole (void)
{
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

	com_argc = argc;
	com_argv = (const char **)argv;

	//
    // Baker: zircon_command_line.txt
    //
    
#ifdef CORE_XCODE // MACOSX
	// This is to make zircon_command_line.txt work for a Mac .app
    if (strstr(com_argv[0], ".app/") && strstr(com_argv[0], "/Xcode/") == NULL) {
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
    
	if (com_argc == 1 /*only the exe*/) {
		char	cmdline_fake[MAX_INPUTLINE];
		
		const char *s_fp = 
#ifdef __ANDROID__
		
			"/sdcard/zircon/"
#endif
			"zircon_command_line.txt";

		if (File_Exists (s_fp)) {
			size_t s_temp_size = 0; const char *s_temp = (const char *)File_To_String_Alloc (s_fp, &s_temp_size);
			c_strlcpy (cmdline_fake, "quake_engine "); // arg0 is engine and ignored by Sys_CheckParm
			if (s_temp) {
				c_strlcat (cmdline_fake, s_temp);
				String_Edit_Whitespace_To_Space (cmdline_fake); // Make tabs, newlines into spaces.
				
				String_Command_String_To_Argv (/*destructive edit*/ cmdline_fake, &fake_argc, fake_argv, MAX_NUM_Q_ARGVS_50);
				com_argc = fake_argc;
				com_argv = (const char **)fake_argv;

				free ((void *)s_temp);
			} // if
		} // if exists
	} // if 1 cmd arg the exe

	//
    // End Baker: zircon_command_line.txt
    //

	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(Sys_CheckParm("-noterminal"))
		outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(Sys_CheckParm("-stderr"))
		outfd = 2;
	else
		outfd = 1;

#ifndef _WIN32
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif // !_WIN32

	// we don't know which systems we'll want to init, yet...
	SDL_Init(0);

	Host_Main();

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

// copies given image to clipbard
int Sys_Clipboard_Set_Image (unsigned *rgba, int width, int height)
{
	return 1; // I have not discovered a method for this for Linux yet
}

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
#ifdef CONFIG_MENU	// We are using this as the litmus test that we have a client
	return SDL_SetClipboardText(text_to_clipboard) == 0;
#else
	return 0;
#endif

}


#define MAX_OSPATH_EX_1024 1024 // Hopefully large enough

#ifdef _WIN32
	// empty
	SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash
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

	

	SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash

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
