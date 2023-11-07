// common.h

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

#ifndef COMMON_H
#define COMMON_H


/// MSVC has a different name for several standard functions
#ifdef _WIN32
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
#endif

// Create our own define for Mac OS X
#if defined(__APPLE__) && defined(__MACH__)
# define MACOSX
#endif

#ifdef SUNOS
#include <sys/file.h>		///< Needed for FNDELAY
#endif

//============================================================================

typedef struct sizebuf_s
{
	qbool	allowoverflow;	///< if false, do a Sys_Error
	qbool	overflowed;		///< set to true if the buffer size failed
	unsigned char		*data;
	int			maxsize;
	int			cursize;
	int			readcount;
	qbool	badread;		// set if a read goes beyond end of message
} sizebuf_t;

void SZ_Clear (sizebuf_t *buf);
unsigned char *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const unsigned char *data, int length);
void SZ_HexDumpToConsole(const sizebuf_t *buf);

void Com_HexDumpToConsole(const unsigned char *data, int size);

unsigned short CRC_Block(const unsigned char *data, size_t size);
unsigned short CRC_Block_CaseInsensitive(const unsigned char *data, size_t size); // for hash lookup functions that use strcasecmp for comparison

unsigned char COM_BlockSequenceCRCByteQW(unsigned char *base, int length, int sequence);

// these are actually md4sum (mdfour.c)
unsigned Com_BlockChecksum (void *buffer, int length);
void Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf);

void COM_Init_Commands(void);


//============================================================================
//							Endianess handling
//============================================================================

// check mem_bigendian if you need to know the system byte order

/*! \name Byte order functions.
 * @{
 */

// unaligned memory access crashes on some platform, so always read bytes...
#define BigShort(l) BuffBigShort((unsigned char *)&(l))
#define LittleShort(l) BuffLittleShort((unsigned char *)&(l))
#define BigLong(l) BuffBigLong((unsigned char *)&(l))
#define LittleLong(l) BuffLittleLong((unsigned char *)&(l))
#define BigFloat(l) BuffBigFloat((unsigned char *)&(l))
#define LittleFloat(l) BuffLittleFloat((unsigned char *)&(l))

/// Extract a big endian 32bit float from the given \p buffer.
float BuffBigFloat (const unsigned char *buffer);

/// Extract a big endian 32bit int from the given \p buffer.
int BuffBigLong (const unsigned char *buffer);

/// Extract a big endian 16bit short from the given \p buffer.
short BuffBigShort (const unsigned char *buffer);

/// Extract a little endian 32bit float from the given \p buffer.
float BuffLittleFloat (const unsigned char *buffer);

/// Extract a little endian 32bit int from the given \p buffer.
int BuffLittleLong (const unsigned char *buffer);

/// Extract a little endian 16bit short from the given \p buffer.
short BuffLittleShort (const unsigned char *buffer);

/// Encode a big endian 32bit int to the given \p buffer
void StoreBigLong (unsigned char *buffer, unsigned int i);

/// Encode a big endian 16bit int to the given \p buffer
void StoreBigShort (unsigned char *buffer, unsigned short i);

/// Encode a little endian 32bit int to the given \p buffer
void StoreLittleLong (unsigned char *buffer, unsigned int i);

/// Encode a little endian 16bit int to the given \p buffer
void StoreLittleShort (unsigned char *buffer, unsigned short i);
//@}

//============================================================================

// these versions are purely for internal use, never sent in network protocol
// (use Protocol_EnumForNumber and Protocol_NumberToEnum to convert)
typedef enum protocolversion_e
{
	PROTOCOL_UNKNOWN,
	PROTOCOL_DARKPLACES7, ///< added QuakeWorld-style movement protocol to allow more consistent prediction
	PROTOCOL_DARKPLACES6, ///< various changes
	PROTOCOL_DARKPLACES5, ///< uses EntityFrame5 entity snapshot encoder/decoder which is based on a Tribes networking article at http://www.garagegames.com/articles/networking1/
	PROTOCOL_DARKPLACES4, ///< various changes
	PROTOCOL_DARKPLACES3, ///< uses EntityFrame4 entity snapshot encoder/decoder which is broken, this attempted to do partial snapshot updates on a QuakeWorld-like protocol, but it is broken and impossible to fix
	PROTOCOL_DARKPLACES2, ///< various changes
	PROTOCOL_DARKPLACES1, ///< uses EntityFrame entity snapshot encoder/decoder which is a QuakeWorld-like entity snapshot delta compression method
	PROTOCOL_QUAKEDP, ///< darkplaces extended quake protocol (used by TomazQuake and others), backwards compatible as long as no extended features are used
	PROTOCOL_NEHAHRAMOVIE, ///< Nehahra movie protocol, a big nasty hack dating back to early days of the Quake Standards Group (but only ever used by neh_gl.exe), this is potentially backwards compatible with quake protocol as long as no extended features are used (but in actuality the neh_gl.exe which wrote this protocol ALWAYS wrote the extended information)
	PROTOCOL_QUAKE, ///< quake (aka netquake/normalquake/nq) protocol
	PROTOCOL_QUAKEWORLD, ///< quakeworld protocol
	PROTOCOL_NEHAHRABJP, ///< same as QUAKEDP but with 16bit modelindex
	PROTOCOL_NEHAHRABJP2, ///< same as NEHAHRABJP but with 16bit soundindex
	PROTOCOL_NEHAHRABJP3 ///< same as NEHAHRABJP2 but with some changes
}
protocolversion_t;

/*! \name Message IO functions.
 * Handles byte ordering and avoids alignment errors
 * @{
 */

void MSG_InitReadBuffer (sizebuf_t *buf, unsigned char *data, int size);
void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, vec_t f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteUnterminatedString (sizebuf_t *sb, const char *s);
void MSG_WriteAngle8i (sizebuf_t *sb, vec_t f);
void MSG_WriteAngle16i (sizebuf_t *sb, vec_t f);
void MSG_WriteAngle32f (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord13i (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord16i (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord32f (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord (sizebuf_t *sb, vec_t f, protocolversion_t protocol);
void MSG_WriteVector (sizebuf_t *sb, const vec3_t v, protocolversion_t protocol);
void MSG_WriteAngle (sizebuf_t *sb, vec_t f, protocolversion_t protocol);

void MSG_BeginReading (sizebuf_t *sb);
int MSG_ReadLittleShort (sizebuf_t *sb);
int MSG_ReadBigShort (sizebuf_t *sb);
int MSG_ReadLittleLong (sizebuf_t *sb);
int MSG_ReadBigLong (sizebuf_t *sb);
float MSG_ReadLittleFloat (sizebuf_t *sb);
float MSG_ReadBigFloat (sizebuf_t *sb);
char *MSG_ReadString (sizebuf_t *sb, char *string, size_t maxstring);
int MSG_ReadBytes (sizebuf_t *sb, int numbytes, unsigned char *out);

#define MSG_ReadChar(sb) ((sb)->readcount >= (sb)->cursize ? ((sb)->badread = true, -1) : (signed char)(sb)->data[(sb)->readcount++])
#define MSG_ReadByte(sb) ((sb)->readcount >= (sb)->cursize ? ((sb)->badread = true, -1) : (unsigned char)(sb)->data[(sb)->readcount++])
#define MSG_ReadShort MSG_ReadLittleShort
#define MSG_ReadLong MSG_ReadLittleLong
#define MSG_ReadFloat MSG_ReadLittleFloat

float MSG_ReadAngle8i (sizebuf_t *sb);
float MSG_ReadAngle16i (sizebuf_t *sb);
float MSG_ReadAngle32f (sizebuf_t *sb);
float MSG_ReadCoord13i (sizebuf_t *sb);
float MSG_ReadCoord16i (sizebuf_t *sb);
float MSG_ReadCoord32f (sizebuf_t *sb);
float MSG_ReadCoord (sizebuf_t *sb, protocolversion_t protocol);
void MSG_ReadVector (sizebuf_t *sb, vec3_t v, protocolversion_t protocol);
float MSG_ReadAngle (sizebuf_t *sb, protocolversion_t protocol);
//@}
//============================================================================

typedef float (*COM_WordWidthFunc_t) (void *passthrough, const char *w, size_t *length, float maxWidth); // length is updated to the longest fitting string into maxWidth; if maxWidth < 0, all characters are used and length is used as is
typedef int (*COM_LineProcessorFunc) (void *passthrough, const char *line, size_t length, float width, qbool isContination);
int COM_Wordwrap(const char *string, size_t length, float continuationSize, float maxWidth, COM_WordWidthFunc_t wordWidth, void *passthroughCW, COM_LineProcessorFunc processLine, void *passthroughPL);

extern char com_token[MAX_INPUTLINE];

int COM_ParseToken_Simple(const char **datapointer, qbool returnnewline, qbool parsebackslash, qbool parsecomments);
int COM_ParseToken_QuakeC(const char **datapointer, qbool returnnewline);
int COM_ParseToken_VM_Tokenize(const char **datapointer, qbool returnnewline);
int COM_ParseToken_Console(const char **datapointer);

extern int com_argc;
extern const char **com_argv;
extern int com_selffd;

int Sys_CheckParm (const char *parm);
void COM_Init (void);
void COM_Shutdown (void);
void COM_InitGameType (void);

char *va(char *buf, size_t buflen, const char *format, ...) DP_FUNC_PRINTF(3);
char *va3(const char *format, ...) DP_FUNC_PRINTF(1);

// does a varargs printf into provided buffer, returns buffer (so it can be called in-line unlike dpsnprintf)


// snprintf and vsnprintf are NOT portable. Use their DP counterparts instead
#ifdef snprintf
# undef snprintf
#endif
#define snprintf DO_NOT_USE_SNPRINTF__USE_DPSNPRINTF
#ifdef vsnprintf
# undef vsnprintf
#endif
#define vsnprintf DO_NOT_USE_VSNPRINTF__USE_DPVSNPRINTF

// dpsnprintf and dpvsnprintf
// return the number of printed characters, excluding the final '\0'
// or return -1 if the buffer isn't big enough to contain the entire string.
// buffer is ALWAYS null-terminated


#define c_dpsnprintf1(_var,_fmt,_s1) dpsnprintf (_var, sizeof(_var), _fmt, _s1)
#define c_dpsnprintf2(_var,_fmt,_s1,_s2) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2)
#define c_dpsnprintf3(_var,_fmt,_s1,_s2,_s3) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3)
#define c_dpsnprintf4(_var,_fmt,_s1,_s2,_s3,_s4) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4)
#define c_dpsnprintf5(_var,_fmt,_s1,_s2,_s3,_s4,_s5) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5)
#define c_dpsnprintf6(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6)
#define c_dpsnprintf7(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7)
#define c_dpsnprintf8(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8)
#define c_dpsnprintf9(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9)
#define c_dpsnprintf10(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9,_s10) dpsnprintf (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9,_s10)


extern int dpsnprintf (char *buffer, size_t buffersize, const char *format, ...) DP_FUNC_PRINTF(3);
extern int dpvsnprintf (char *buffer, size_t buffersize, const char *format, va_list args);
extern char *dpstrrstr(const char *s1, const char *s2); // Baker: 10000 
extern char *dpstrcasestr(const char *s, const char *find); // Baker: 10001 
extern char *dpreplacechar (char *s_edit, int ch_find, int ch_replace); // Baker: 10002

// A bunch of functions are forbidden for security reasons (and also to please MSVS 2005, for some of them)
// LadyHavoc: added #undef lines here to avoid warnings in Linux
#undef strcat
#define strcat DO_NOT_USE_STRCAT__USE_STRLCAT_OR_MEMCPY
#undef strncat
#define strncat DO_NOT_USE_STRNCAT__USE_STRLCAT_OR_MEMCPY
#undef strcpy
#define strcpy DO_NOT_USE_STRCPY__USE_STRLCPY_OR_MEMCPY
#undef strncpy
#define strncpy DO_NOT_USE_STRNCPY__USE_STRLCPY_OR_MEMCPY
//#undef sprintf
//#define sprintf DO_NOT_USE_SPRINTF__USE_DPSNPRINTF



//============================================================================

extern	struct cvar_s	registered;
extern	struct cvar_s	cmdline;

typedef enum userdirmode_e
{
	USERDIRMODE_NOHOME, // basedir only
	USERDIRMODE_HOME, // Windows basedir, general POSIX (~/.)
	USERDIRMODE_MYGAMES, // pre-Vista (My Documents/My Games/), general POSIX (~/.)
	USERDIRMODE_SAVEDGAMES, // Vista (%USERPROFILE%/Saved Games/), OSX (~/Library/Application Support/), Linux (~/.config)
	USERDIRMODE_COUNT
}
userdirmode_t;

typedef enum gamemode_e
{
	GAME_NORMAL,
	GAME_HIPNOTIC,
	GAME_ROGUE,
	GAME_QUOTH,
	GAME_QUAKE3_QUAKE1,
	GAME_NEHAHRA,
	GAME_NEXUIZ,
//	GAME_ZIRCON,
	GAME_XONOTIC,
	GAME_TRANSFUSION,
	GAME_GOODVSBAD2,
	GAME_TEU,
	GAME_BATTLEMECH,
	GAME_ZYMOTIC,
	GAME_SETHERAL,
	GAME_TENEBRAE, // full of evil hackery
	GAME_NEOTERIC,
	GAME_OPENQUARTZ, //this game sucks
	GAME_PRYDON,
	GAME_DELUXEQUAKE,
	GAME_THEHUNTED,
	GAME_DEFEATINDETAIL2,
	GAME_DARSANA,
	GAME_CONTAGIONTHEORY,
	GAME_EDU2P,
	GAME_PROPHECY,
	GAME_BLOODOMNICIDE,
	GAME_STEELSTORM, // added by motorsep
	GAME_STEELSTORM2, // added by motorsep
	GAME_SSAMMO, // added by motorsep
	GAME_STEELSTORMREVENANTS, // added by motorsep 07/19/2015
	GAME_TOMESOFMEPHISTOPHELES, // added by motorsep
	GAME_STRAPBOMB, // added by motorsep for Urre
	GAME_MOONHELM,
	GAME_VORETOURNAMENT,
	GAME_COUNT
}
gamemode_t;

// Master switch for some hacks/changes that eventually should become cvars.
#define IS_NEXUIZ_DERIVED(g) ((g) == GAME_NEXUIZ || (g) == GAME_XONOTIC || (g) == GAME_VORETOURNAMENT)
// Pre-csqcmodels era.
#define IS_OLDNEXUIZ_DERIVED(g) ((g) == GAME_NEXUIZ || (g) == GAME_VORETOURNAMENT)

extern gamemode_t gamemode;

extern const char *gamename;
extern const char *gamenetworkfiltername;
extern const char *gamedirname1;
extern const char *gamedirname2;
extern const char *gamescreenshotname;
extern const char *gameuserdirname;
extern char com_modname[MAX_OSPATH];

void COM_ChangeGameTypeForGameDirs(void);

void COM_ToLowerString (const char *in, char *out, size_t size_out);
void COM_ToUpperString (const char *in, char *out, size_t size_out);
int COM_StringBeginsWith(const char *s, const char *match);

int COM_ReadAndTokenizeLine(const char **text, char **argv, int maxargc, char *tokenbuf, int tokenbufsize, const char *commentprefix);

size_t COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid);
qbool COM_StringDecolorize(const char *in, size_t size_in, char *out, size_t size_out, qbool escape_carets);
void COM_ToLowerString (const char *in, char *out, size_t size_out);
void COM_ToUpperString (const char *in, char *out, size_t size_out);

typedef struct stringlist_s
{
	/// maxstrings changes as needed, causing reallocation of strings[] array
	int maxstrings;
	int numstrings;
	char **strings;
} stringlist_t;

int matchpattern(const char *in, const char *pattern, int caseinsensitive);
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one);
void stringlistinit(stringlist_t *list);
void stringlistfreecontents(stringlist_t *list);
void stringlistappend(stringlist_t *list, const char *text);
void stringlistsort(stringlist_t *list, qbool uniq);
void listdirectory(stringlist_t *list, const char *basepath, const char *path);

char *InfoString_GetValue(const char *buffer, const char *key, char *value, size_t valuelength);
void InfoString_SetValue(char *buffer, size_t bufferlength, const char *key, const char *value);
void InfoString_Print(char *buffer);

// strlcat and strlcpy, from OpenBSD
// Most (all?) BSDs already have them
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(MACOSX)
# define HAVE_STRLCAT 1
# define HAVE_STRLCPY 1
#endif

#ifndef HAVE_STRLCAT
/*!
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz);
#endif  // #ifndef HAVE_STRLCAT

#ifndef HAVE_STRLCPY
/*!
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz);

#endif  // #ifndef HAVE_STRLCPY

void FindFraction(double val, int *num, int *denom, int denomMax);

// decodes XPM file to XPM array (as if #include'd)
char **XPM_DecodeString(const char *in);

size_t base64_encode(unsigned char *buf, size_t buflen, size_t outbuflen);


// extras2

// Baker dogma ... human friendly source code, especially for strings
// Say clearly what you are doing ... even a hardcore code can miss punctuation
// when reading a line.  This has a cost.  Efficient source code must be reader
// friendly to extent time permits and is reasonable.  Most source code is 
// written once, read many times.  Pay it forward by optimizing the "read many
// times" part.  Future productivity is part of productivity.

///////////////////////////////////////////////////////////////////////////////
//  STRING: Baker - String
///////////////////////////////////////////////////////////////////////////////

// Optimize these 2 for speed rather than debugging convenience ...

#define String_Does_Match_Caseless(s1,s2)				(!strcasecmp(s1, s2))
#define String_Does_Match(s1,s2)						(!strcmp(s1, s2))
#define String_Does_Not_Match(s1,s2)					(!!strcmp(s1, s2))


#define String_Isin1(sthis,s0)							( String_Does_Match(sthis, s0) )
#define String_Isin2(sthis,s0,s1)						( String_Does_Match(sthis, s0) || String_Does_Match(sthis, s1) )
#define String_Isin3(sthis,s0,s1,s2)					( String_Does_Match(sthis, s0) || String_Does_Match(sthis, s1) || String_Does_Match(sthis, s2) )

#define String_Isin1_Caseless(sthis,s0)					( String_Does_Match_Caseless(sthis, s0) )
#define String_Isin2_Caseless(sthis,s0,s1)				( String_Does_Match_Caseless(sthis, s0) || String_Does_Match_Caseless(sthis, s1) )
#define String_Isin3_Caseless(sthis,s0,s1,s2)			( String_Does_Match_Caseless(sthis, s0) || String_Does_Match_Caseless(sthis, s1) || String_Does_Match_Caseless(sthis, s2) )

#define String_Does_Start_With(s,s_prefix)				(!strncmp(s, s_prefix, strlen(s_prefix)))
#define String_Does_Start_With_Caseless(s,s_prefix)		(!strncasecmp(s, s_prefix, strlen(s_prefix)))

// FN ...

int String_Range_Count_Char (const char *s_start, const char *s_end, int ch_findchar);
char *String_Find_End (const char *s); // Returns pointer to last character of string or NULL if zero length string.

char *String_Skip_WhiteSpace_Excluding_Space (const char *s);
char *String_Skip_WhiteSpace_Including_Space (const char *s);
char *String_Replace_Len_Count_Alloc (const char *s, const char *s_find, const char *s_replace, /*reply*/ int *created_length, replyx size_t *created_bufsize, replyx int *replace_count);
char *String_Range_Find_Char (const char *s_start, const char *s_end, int ch_findchar);
char *String_Edit_Whitespace_To_Space (char *s_edit);
char *String_Edit_Trim (char *s_edit);
char *String_Edit_Replace (char *s_edit, size_t s_size, const char *s_find, const char *s_replace); // no alloc
char *String_Replace_Alloc (const char *s, const char *s_find, const char *s_replace);
char *String_Edit_RTrim_Whitespace_Including_Spaces (char *s_edit);

int String_Does_End_With (const char *s, const char *s_suffix);
int String_Does_Contain (const char *s, const char *s_find);
int String_Does_Contain_Caseless (const char *s, const char *s_find);
int String_Does_End_With_Caseless (const char *s, const char *s_suffix);
int String_Does_Have_Uppercase (const char *s);
char *String_Find_Skip_Past (const char *s, const char *s_find);

void String_Command_String_To_Argv (char *cmdline, int *numargc, char **argvz, int maxargs);

char *va2 (const char *format, ...) __core_attribute__((__format__(__printf__,1,2))) ;

char *dpstrndup (const char *s, size_t n);

///////////////////////////////////////////////////////////////////////////////
//  FILE INFORMATION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////

char *File_URL_Edit_SlashesForward_Like_Unix (char *windows_path_to_file);
void *File_To_Memory_Alloc (const char *path_to_file, replyx size_t *numbytes);
char *File_URL_Edit_Remove_Extension (char *path_to_file);
char *File_URL_Edit_Reduce_To_Parent_Path (char *path_to_file);


SBUF___ const char *File_Getcwd_SBuf (void); // No trailing slash
char *File_URL_Edit_SlashesBack_Like_Windows (char *unix_path_to_file);
char *File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (char *path_to_file);
const char *File_URL_SkipPath (const char *path_to_file); // last path component
char *File_URL_Remove_Trailing_Unix_Slash (char *path_to_file);

int File_Exists (const char *path_to_file_);
int File_Is_Existing_File (const char *path_to_file);

#define File_To_String_Alloc(FILENAME, REPLY_BYTES_OPTIONAL_SIZE_T) File_To_Memory_Alloc (FILENAME, REPLY_BYTES_OPTIONAL_SIZE_T) // Reply is a blob.


int Folder_Open (const char *path_to_file);




///////////////////////////////////////////////////////////////////////////////
//  IMAGE: OPERATIONS: Baker - Image
///////////////////////////////////////////////////////////////////////////////

void Image_Flip_Buffer (void *pels, int columns, int rows, int bytes_per_pixel);
void Image_Flip_RedGreen (void *rgba, size_t numbytes);

///////////////////////////////////////////////////////////////////////////////
//  CLIPBOARD: Baker - Clipboard
///////////////////////////////////////////////////////////////////////////////

int CGL_Clipboard_Texture_Copy (int ts, int miplevel); // debug, downloads from OpenGL for inspection
int Clipboard_Set_Text (const char *text_to_clipboard);

///////////////////////////////////////////////////////////////////////////////
//  TIME: Baker
///////////////////////////////////////////////////////////////////////////////

int Time_Hours (int seconds);
int Time_Minutes_Less_Hours (int seconds);
int Time_Minutes (int seconds);
int Time_Seconds (int seconds);


///////////////////////////////////////////////////////////////////////////////
//  MATH: Baker - Math
///////////////////////////////////////////////////////////////////////////////


void Math_Project (vec_t *src3d, vec_t *dest2d);
void Math_Unproject (vec_t *src2d, vec_t *dest3d);

#endif // COMMON_H

