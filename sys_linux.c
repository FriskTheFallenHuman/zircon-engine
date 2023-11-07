#if 0 //ndef _MSC_VER

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include "conio.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#endif

#include <signal.h>

#include "darkplaces.h"

// =======================================================================
// General routines
// =======================================================================
void Sys_Shutdown (void)
{
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
}

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

static int outfd = 1;
void Sys_PrintToTerminal(const char *text)
{
	if(outfd < 0)
		return;
#ifdef FNDELAY
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl (outfd, F_GETFL, 0);
		fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
#endif
#ifdef _WIN32
#define write _write
#endif
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
#endif
	//fprintf(stdout, "%s", text);
}

char *Sys_ConsoleInput(void)
{
	//if (cls.state == ca_dedicated)
	{
		static char text[MAX_INPUTLINE];
		static unsigned int len = 0;
#ifdef _WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			if (c == '\r')
			{
				text[len] = '\0';
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == '\b')
			{
				if (len)
				{
					_putch (c);
					_putch (' ');
					_putch (c);
					len--;
				}
				continue;
			}
			if (len < sizeof (text) - 1)
			{
				_putch (c);
				text[len] = c;
				len++;
			}
		}
#else
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
		{
			len = read (0, text, sizeof(text) - 1);
			if (len >= 1)
			{
				// rip off the \n and terminate
				// div0: WHY? console code can deal with \n just fine
				// this caused problems with pasting stuff into a terminal window
				// so, not ripping off the \n, but STILL keeping a NUL terminator
				text[len] = 0;
				return text;
			}
		}
#endif
	}
	return NULL;
}


void Sys_InitConsole (void)
{
}

int main (int argc, char **argv)
{
	signal(SIGFPE, SIG_IGN);

	com_argc = argc;
	com_argv = (const char **)argv;
	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(Sys_CheckParm("-noterminal"))
		outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(Sys_CheckParm("-stderr"))
		outfd = 2;
	else
		outfd = 1;

#ifdef FNDELAY
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	Host_Main();

	return 0;
}

qbool sys_supportsdlgetticks = false;
unsigned int Sys_SDL_GetTicks (void)
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}


#define MAX_OSPATH_EX 1024 // Hopefully large enough

SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash

{ 
	static char workingdir[MAX_OSPATH_EX];
	
	if (getcwd (workingdir, sizeof(workingdir) - 1)) {
		return workingdir;
	} else return NULL;
}


void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}

// Folder must exist.  It must be a folder.
int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
//  xdg-open is a desktop-independent tool for configuring the default applications of a user
	if ( fork() == 0) {
		execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
		//cleanup_on_exit();  /* clean up before exiting */
        exit(3);
	}

    return true;
}

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

int Sys_SetClipboardData(const char *text_to_clipboard)
{
#ifdef CONFIG_MENU
	return SDL_SetClipboardText(text_to_clipboard) == 0;
#else
	return 0;
#endif
}

const char *Sys_Binary_URL_SBuf (void)
{
    static char binary_url[MAX_OSPATH];
	if (!binary_url[0]) {
		pid_t pid = getpid();
		int length;

		char linkname[MAX_OSPATH];
		c_snprintf1 (linkname, "/proc/%d/exe", pid);

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


#endif // 0 ! _MSC_VER