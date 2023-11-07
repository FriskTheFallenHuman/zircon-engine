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
// sys.h -- non-portable functions

#ifndef SYS_H
#define SYS_H

extern cvar_t sys_usenoclockbutbenchmark;

/* Preprocessor macros to identify platform
    DP_OS_NAME 	- "friendly" name of the OS, for humans to read
    DP_OS_STR	- "identifier" of the OS, more suited for code to use
    DP_ARCH_STR	- "identifier" of the processor architecture
 */

#if defined(__ANDROID__) /* must come first because it also defines linux */
	# define DP_OS_NAME		"Android"
	# define DP_OS_STR		"android"
	# define USE_GLES2		1

	# define USE_RWOPS		1	// Baker: this is not cutting it, either it is me or DP code

	//# define LINK_TO_ZLIB	1	// Baker: I didn't need this
								// Does Android supply it?

	# define DP_MOBILETOUCH	1			
		// Baker: CL_IPLog_Add, CL_IPLog_Load
		//	Cvar_SetValueQuick(&r_nearclip, 4);
		// 	Cvar_SetValueQuick(&r_farclip_base, 4096);
		//	Cvar_SetValueQuick(&r_farclip_world, 0);
		//	Cvar_SetValueQuick(&r_useinfinitefarclip, 0);
		// 	VID_Init: Cvar_SetValueQuick(&vid_touchscreen, 1);

	# define DP_FREETYPE_STATIC 1
	# define LINK_TO_LIBVORBIS 1
	# define LINK_TO_LIBJPEG
	
#elif TARGET_OS_IPHONE /* must come first because it also defines MACOSX */
	# define DP_OS_NAME		"iPhoneOS"
	# define DP_OS_STR		"iphoneos"
	# define USE_GLES2		1
	# define LINK_TO_ZLIB	1
	# define LINK_TO_LIBVORBIS 1
	# define DP_MOBILETOUCH	1
	# define DP_FREETYPE_STATIC 1
#elif defined(__linux__)
	# define DP_OS_NAME		"Linux"
	# define DP_OS_STR		"linux"
#elif defined(_WIN64)
	# define DP_OS_NAME		"Windows64"
	# define DP_OS_STR		"win64"
#elif defined(_WIN32)
	# define DP_OS_NAME		"Windows"
	# define DP_OS_STR		"win32"
#elif defined(__FreeBSD__)
	# define DP_OS_NAME		"FreeBSD"
	# define DP_OS_STR		"freebsd"
#elif defined(__NetBSD__)
	# define DP_OS_NAME		"NetBSD"
	# define DP_OS_STR		"netbsd"
#elif defined(__OpenBSD__)
	# define DP_OS_NAME		"OpenBSD"
	# define DP_OS_STR		"openbsd"
#elif defined(MACOSX)
	# define DP_OS_NAME		"Mac OS X"
	# define DP_OS_STR		"osx"
	
	// # define CORE_SDL
	# define CONFIG_MENU	// Baker: this really means not server only

    #ifdef CORE_XCODE
        # define LINK_TO_LIBVORBIS 1
        # define LINK_TO_LIBJPEG
        # define DP_FREETYPE_STATIC 1
    #endif
	
	// # define USEODE			
	// # define CONFIG_VIDEO_CAPTURE	
	// _FILE_OFFSET_BITS=64					// 32 bit helper for fseek etc for gcc _FILE_OFFSET_BITS
	// __KERNEL_STRICT_NAMES				// Linux thing


#elif defined(__MORPHOS__)
	# define DP_OS_NAME		"MorphOS"
	# define DP_OS_STR		"morphos"
#else
	# define DP_OS_NAME		"Unknown"
	# define DP_OS_STR		"unknown"
#endif

#if defined(__GNUC__)
	# if defined(__i386__)
		#  define DP_ARCH_STR		"686"
		#  define SSE_POSSIBLE
		#  ifdef __SSE__
			#define SSE_PRESENT
		#  endif
		#  ifdef __SSE2__
		#   define SSE2_PRESENT
		#  endif
	# elif defined(__x86_64__)
		#  define DP_ARCH_STR		"x86_64"
		#  define SSE_PRESENT
		#  define SSE2_PRESENT
	# elif defined(__powerpc__)
		#  define DP_ARCH_STR		"ppc"
	# endif
#elif defined(_WIN64)
	# define DP_ARCH_STR		"x86_64"
	# define SSE_PRESENT
	# define SSE2_PRESENT
#elif defined(_WIN32)
	# define DP_ARCH_STR		"x86"
	# define SSE_POSSIBLE
#endif

#ifdef SSE_PRESENT
	# define SSE_POSSIBLE
#endif

#ifdef NO_SSE
	# undef SSE_PRESENT
	# undef SSE_POSSIBLE
	# undef SSE2_PRESENT
#endif

#ifdef SSE_POSSIBLE
	// runtime detection of SSE/SSE2 capabilities for x86
	qbool Sys_HaveSSE(void);
	qbool Sys_HaveSSE2(void);
#else
	#define Sys_HaveSSE() false
	#define Sys_HaveSSE2() false
#endif // SSE_POSSIBLE

//
// DLL management
//

// Win32 specific
#ifdef _WIN32
# include <windows.h>
typedef HMODULE dllhandle_t;

// Other platforms
#else
  typedef void* dllhandle_t;
#endif

typedef struct dllfunction_s
{
	const char *name;
	void **funcvariable;
}
dllfunction_t;

/*! Loads a dependency library. 
 * \param dllnames a NULL terminated array of possible names for the DLL you want to load.
 * \param handle
 * \param fcts
 */
qbool Sys_LoadDependency (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts);
void Sys_FreeLibrary (dllhandle_t* handle);
void* Sys_GetProcAddress (dllhandle_t handle, const char* name);

// called early in Host_Init
void Sys_InitConsole (void);

/// called after command system is initialized but before first Con_Print
void Sys_Init_Commands (void);


/// \returns current timestamp
char *Sys_TimeString(const char *timeformat);

//
// system IO interface (these are the sys functions that need to be implemented in a new driver atm)
//

/// an error will cause the entire program to exit
void Sys_Error (const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;

/// (may) output text to terminal which launched program
void Sys_PrintToTerminal(const char *text);

#ifdef _DEBUG
void Sys_PrintToTerminal2(const char *text); // To Visual Studio Debug text only (when printing to console interferes with debugging)
#endif


void Sys_PrintfToTerminal(const char *fmt, ...);

/// INFO: This is only called by Host_Shutdown so we dont need testing for recursion
void Sys_Shutdown (void);
void Sys_Quit (int returnvalue);

/*! on some build/platform combinations (such as Linux gcc with the -pg
 * profiling option) this can turn on/off profiling, used primarily to limit
 * profiling to certain areas of the code, such as ingame performance without
 * regard for loading/shutdown performance (-profilegameonly on commandline)
 */
#ifdef __cplusplus
extern "C"
#endif
void Sys_AllowProfiling (qbool enable);

typedef struct sys_cleantime_s
{
	double dirtytime; // last value gotten from Sys_DirtyTime()
	double cleantime; // sanitized linearly increasing time since app start
}
sys_cleantime_t;

double Sys_DirtyTime(void);

void Sys_ProvideSelfFD (void);

char *Sys_ConsoleInput (void);

/// called to yield for a little bit so as not to hog cpu when paused or debugging
void Sys_Sleep(int microseconds);

/// Perform Key_Event () callbacks until the input que is empty
void Sys_SendKeyEvents (void);

char *Sys_GetClipboardData (void);
int Sys_SetClipboardData(const char *text_to_clipboard);

extern qbool sys_supportsdlgetticks;
unsigned int Sys_SDL_GetTicks (void); // wrapper to call SDL_GetTicks
void Sys_SDL_Delay (unsigned int milliseconds); // wrapper to call SDL_Delay

/// called to set process priority for dedicated servers
void Sys_InitProcessNice (void);
void Sys_MakeProcessNice (void);
void Sys_MakeProcessMean (void);

#endif // !SYS_H

