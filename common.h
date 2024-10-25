// common.h

/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2020 DarkPlaces contributors

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

#include <stdarg.h>
#include <assert.h>
#include "qtypes.h"
#include "qdefs.h"
#include "baker.h"

/// MSVC has a different name for several standard functions
#ifdef _WIN32
	# define strcasecmp _stricmp
	# define strncasecmp _strnicmp
#else
	#include "strings.h"
#endif

// Create our own define for Mac OS X
#if defined(__APPLE__) && defined(__MACH__)
# define MACOSX
#endif


//============================================================================

#define ContainerOf(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct sys_s
{
	int argc;
	const char **argv;
	int selffd;
	int outfd;
	int nicelevel;
	qbool nicepossible;
	qbool isnice;
} sys_t;

extern sys_t sys;

// END SYS

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

void COM_InitOnce_Commands(void);


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

#define PROTOCOL_VERSION_FTE1		(('F'<<0) + ('T'<<8) + ('E'<<16) + ('X' << 24))	//fte extensions.
#define PROTOCOL_VERSION_EZQUAKE1	(('M'<<0) + ('V'<<8) + ('D'<<16) + ('1' << 24)) //ezquake/mvdsv extensions

WARP_X_ (DEFS: ZIRCON_PEXT ZIRCON_EXT_CHUNKED_2)
#define ZIRCON_EXT_CHUNKED_2							2
#define ZIRCON_EXT_FREEMOVE_4							4
#define ZIRCON_EXT_NONSOLID_FLAG_8						8
#define ZIRCON_EXT_32BIT_RENDER_FLAGS_16				16	// Baker: We send flags as a long instead of a byte
#define ZIRCON_EXT_STATIC_ENT_ALPHA_COLORMOD_SCALE_32	32  // Baker: Static entities write alpha, colormod, scale
#define ZIRCON_EXT_SERVER_SENDS_BBOX_TO_CLIENT_64		64
#define ZIRCON_EXT_WALKTHROUGH_PLAYERS_IS_ACTIVE_128	128 // sv_players_walk_thru_players 1

// to client 

#define CLIENT_SUPPORTED_ZIRCON_EXT		(			\
	ZIRCON_EXT_CHUNKED_2 +							\
	ZIRCON_EXT_FREEMOVE_4 +							\
	ZIRCON_EXT_NONSOLID_FLAG_8 +					\
	ZIRCON_EXT_32BIT_RENDER_FLAGS_16 +				\
	ZIRCON_EXT_STATIC_ENT_ALPHA_COLORMOD_SCALE_32 +	\
	ZIRCON_EXT_SERVER_SENDS_BBOX_TO_CLIENT_64 +		\
	ZIRCON_EXT_WALKTHROUGH_PLAYERS_IS_ACTIVE_128 +	\
	0 \
	) // Ender

// sv.zirconprotcolextensions_sv -- set in SV_SpawnServer
// CL: CL_ForwardToServerf ("prespawn %d", cl_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0)
// SV: unsigned int shared_flags = client->cl_zirconprotocolextensions & sv.zirconprotcolextensions_sv;
// SV: svc_stufftext -> //hint "zircon_ext %d", shared_flags
// CL: svc_stufftext -> //hint "zircon_ext %d" -> cls.zirconprotocolextensions = atoi (scmd_arg1);
// CL: 
//					if (Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE)
//							CL_ForwardToServerf ("download %s chunked", cl.model_name[cl.downloadmodel_current]);
//					else	CL_ForwardToServerf ("download %s", cl.model_name[cl.downloadmodel_current]);

// Chunks: We always ask for chunks, unless cl_pext 0
// Reason: This might too early to read server extensions.
// How do we know if we get chunks?
// SV: --> "cl_downloadbegin 3232323 maps/aerowalk.bsp deflate chunked"
// CL_DownloadBegin_DP_f: Cmd_Argc(cmd) >= 4 && String_Match(Cmd_Argv(cmd, 3), "chunked")

// Q: Does server ever disallow chunked downloads?

// HARD: chunked downloads only.
#define Have_Zircon_Ext_Flag_CL_Hard(zext)		Have_Flag((cl_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0), zext)
#define Have_Zircon_Ext_Flag_CLS(zext)			Have_Flag(cls.zirconprotocolextensions, zext)
#define Have_Zircon_Ext_CLHard_CHUNKS_ACTIVE	(Have_Zircon_Ext_Flag_CL_Hard(ZIRCON_EXT_CHUNKED_2) )

// sv.zirconprotcolextensions_sv and Have_Zircon_Ext_Flag_SV_Hard as about the same thing.
// Except for SV/CL handshake, use Have_Zircon_Ext_Flag_SV_Hard to indicate hard.

// SV_HARD for signon buffer only. 

#define Have_Zircon_Ext_Flag_SV_HCL(zext)		Have_Flag(host_client->cl_zirconprotocolextensions, zext)
#define Have_Zircon_Ext_Flag_SV_Hard(zext)		Have_Flag(sv.zirconprotcolextensions_sv, zext)


//#define ZIRCON_EXT_SERVER (sv_pext.integer ? CLIENT_SUPPORTED_ZIRCON_EXT : 0) // sv.zirconprotcolextensions_sv

#define ZMOVE_IS_ENABLED \
	(Have_Zircon_Ext_Flag_CLS (ZIRCON_EXT_FREEMOVE_4) && \
	cl_zircon_move.integer && \
	(cl.zircon_warp_sequence_clock == 0 || cl.zircon_warp_sequence_clock < cl.time))


#define PROTOCOL_VERSION_QW_28		28

#define CLIENT_SUPPORTED_Z_EXTENSIONS (Z_EXT_JOIN_OBSERVE)
#define Z_EXT_JOIN_OBSERVE	(1<<5)	// 32 server: "join" and "observe" commands are supported
									// 32 client: on-the-fly spectator <-> player switching supported

// these versions are purely for internal use, never sent in network protocol
// (use Protocol_EnumForNumber and Protocol_NumberToEnum to convert)
typedef enum protocolversion_e {
	PROTOCOL_UNKNOWN_0 = 0,
	PROTOCOL_DARKPLACES8, ///< added parting messages. WIP
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
	PROTOCOL_NEHAHRABJP3, ///< same as NEHAHRABJP2 but with some changes
	PROTOCOL_FITZQUAKE999,
	PROTOCOL_FITZQUAKE666,
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

void Msg_WriteByte_WriteStringf (sizebuf_t *sb, int your_clc, const char *fmt, ...) DP_FUNC_PRINTF(3);


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

float MSG_ReadAngle16f (sizebuf_t *sb); // FitzQuake 666 / RMQ 999

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
int COM_Wordwrap_Num_Rows_Drawn(const char *string, size_t length, float continuationSize, float maxWidth, COM_WordWidthFunc_t wordWidth, void *passthroughCW, COM_LineProcessorFunc processLine, void *passthroughPL);

extern char com_token[MAX_INPUTLINE_16384];

// Baker: This is the dominant simple form used to parse entities
// 80 of 101 uses of COM_ParseToken_Simple use false, false, true
// non-"false, false, true" is used for effects, models and other cases.
#define COM_Parse_Basic(p) COM_ParseToken_Simple(p, false, false, true)

// Baker: COM_ParseToken_Simple -- Returns false on failure, true on success
int COM_ParseToken_Simple(const char **datapointer, qbool returnnewline, qbool parsebackslash, qbool parsecomments);
int COM_ParseToken_QuakeC(const char **datapointer, qbool returnnewline);
int COM_ParseToken_VM_Tokenize(const char **datapointer, qbool returnnewline);
int COM_ParseToken_Console(const char **datapointer);

// Baker: Returns false on end of data or new line
typedef enum {
	word_none_0			= 0,
 	word_alphanumeric_1	= 1,		// alphanumeric or _	
 	word_numeric_2		= 2,		//	"123"
 	word_numericalpha_3	= 3,		//	"2a"
 	word_punct_4		= 4,		//	"," ... always a single char.
	word_string_5		= 5,
	word_multichar_6	= 6,
	word_invalid_9		= 9,		// control characters, non-ascii >= 127 
	word_newline_10		= 10,
} word_e;

WARP_X_ (operator_e)

word_e COM_ParseToken_EQ_Tokenize (const char **datapointer, const char **ps_start, const char **ps_beyond);

char *COM_Parse_FTE (const char *data, char *out, size_t outlen);

void COM_Init (void);
void COM_Shutdown (void);

char *va(char *buf, size_t buflen, const char *format, ...) DP_FUNC_PRINTF(3);

#define va_sizeof(x, format, ...) \
	va(x, sizeof(x), format, __VA_ARGS__) // Ender


// does a varargs printf into provided buffer, returns buffer (so it can be called in-line unlike dpsnprintf)

// Baker: Creates a variable, variadic prints to it.

// Example: --- >    va_super (tmp, 1024, "load %s // %s", s_savename, s_map);

// char tmp[1024];
// va (tmp, sizeof(tmp), "load %s // %s", , s_savename, s_map ) // Ender

#define va_super(varname,sizetwanted,format,...)	\
	char varname[sizetwanted];		\
	va (varname, sizeof(varname), format, __VA_ARGS__) // Ender


// GCC with -Werror=c++-compat will error out if static_assert is used even though the macro is valid C11...
#ifndef __cplusplus
#define DP_STATIC_ASSERT(expr, str) _Static_assert(expr, str)
#else
	#if defined(_MSC_VER) && _MSC_VER < 1900
		#define	DP_STATIC_ASSERT(expr, str)	\
			typedef int dummy_ ## str[(expr) * 2 - 1]
	#else
		#define DP_STATIC_ASSERT(expr, str) static_assert(expr, str)
	#endif
#endif

// snprintf and vsnprintf are NOT portable. Use their DP counterparts instead
#ifdef snprintf
# undef snprintf
#endif
#define snprintf DP_STATIC_ASSERT(0, "snprintf is forbidden for portability reasons. Use dpsnprintf instead.")
#ifdef vsnprintf
# undef vsnprintf
#endif
#define vsnprintf DP_STATIC_ASSERT(0, "vsnprintf is forbidden for portability reasons. Use dpvsnprintf instead.")

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

// Baker: variadic buffer cycle of 32 strings .. adapted from ancient DarkPlaces
char *va32 (const char *format, ...); // Helpful for engine testing

extern int dpsnprintf (char *buffer, size_t buffersize, const char *format, ...) DP_FUNC_PRINTF(3);
extern int dpvsnprintf (char *buffer, size_t buffersize, const char *format, va_list args);
extern char *dp_strstr_reverse(const char *s1, const char *s2); // Baker: 10000
extern char *dpstrcasestr(const char *s, const char *find); // Baker: 10001
extern char *dpreplacechar (char *s_edit, int ch_find, int ch_replace); // Baker: 10002

// A bunch of functions are forbidden for security reasons (and also to please MSVS 2005, for some of them)
// LadyHavoc: added #undef lines here to avoid warnings in Linux
#undef strcat
#define strcat DP_STATIC_ASSERT(0, "strcat is forbidden for security reasons. Use strlcat or memcpy instead.")
#undef strncat
#define strncat DP_STATIC_ASSERT(0, "strncat is forbidden for security reasons. Use strlcat or memcpy instead.")
#undef strcpy
#define strcpy DP_STATIC_ASSERT(0, "strcpy is forbidden for security reasons. Use strlcpy or memcpy instead.")
#undef strncpy
#define strncpy DP_STATIC_ASSERT(0, "strncpy is forbidden for security reasons. Use strlcpy or memcpy instead.")
#undef sprintf
#define sprintf DP_STATIC_ASSERT(0, "sprintf is forbidden for security reasons. Use dpsnprintf instead.")


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

void COM_ToLowerString (const char *in, char *out, size_t size_out);
void COM_ToUpperString (const char *in, char *out, size_t size_out);
int COM_StringBeginsWith(const char *s, const char *match);

int COM_ReadAndTokenizeLine(const char **text, char **argv, int maxargc, char *tokenbuf, int tokenbufsize, const char *commentprefix);

size_t COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid);
qbool COM_StringDecolorize(const char *in, size_t size_in, char *out, size_t size_out, qbool escape_carets);
void COM_ToLowerString (const char *in, char *out, size_t size_out);
void COM_ToUpperString (const char *in, char *out, size_t size_out);

// strlcat and strlcpy, from OpenBSD
// Most (all?) BSDs already have them
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(MACOSX)
# define HAVE_STRLCAT 1
# define HAVE_STRLCPY 1
#endif

#ifndef HAVE_STRLCAT
/*!
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t dsize);
#endif  // #ifndef HAVE_STRLCAT

#ifndef HAVE_STRLCPY
/*!
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t dsize);

#endif  // #ifndef HAVE_STRLCPY

void FindFraction(double val, int *num, int *denom, int denomMax);

// decodes XPM file to XPM array (as if #include'd)
char **XPM_DecodeString(const char *in);

size_t base64_encode(unsigned char *buf, size_t buflen, size_t outbuflen);

char *base64_encode_calloc (const unsigned char *data, size_t in_len, /*reply*/ size_t *numbytes);
unsigned char *base64_decode_calloc (const char *encoded_string, /*reply*/ size_t *numbytes);

// Baker use ARRAY_COUNT instead (name differece only)
// A size implies the number of bytes.
// We do not want the number of bytes of the array, we
// want a count of the number of elements
//#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

float Com_CalcRoll (const vec3_t angles, const vec3_t velocity, const vec_t angleval, const vec_t velocityval);


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

#define String_Match_Caseless(s1,s2)				(!strcasecmp(s1, s2))
#define String_NOT_Match_Caseless(s1,s2)			(!!strcasecmp(s1, s2))
#define String_Match(s1,s2)							(!strcmp(s1, s2))
#define String_NOT_Match(s1,s2)						(!!strcmp(s1, s2))

#define String_Is_Quoted(s, slen) \
	(s[0] == CHAR_DQUOTE_34 && slen >= 2 && s[slen - 1] == CHAR_DQUOTE_34) // Ender


#define String_Isin1(sthis,s0)							( String_Match(sthis, s0) )
#define String_Isin2(sthis,s0,s1)						( String_Match(sthis, s0) || String_Match(sthis, s1) )
#define String_Isin3(sthis,s0,s1,s2)					( String_Match(sthis, s0) || String_Match(sthis, s1) || String_Match(sthis, s2) )
#define String_Isin4(sthis,s0,s1,s2,s3)					( String_Match(sthis, s0) || String_Match(sthis, s1) || String_Match(sthis, s2) || String_Match(sthis, s3) )

#define String_Isin1_Caseless(sthis,s0)					( String_Match_Caseless(sthis, s0) )
#define String_Isin2_Caseless(sthis,s0,s1)				( String_Match_Caseless(sthis, s0) || String_Match_Caseless(sthis, s1) )
#define String_Isin3_Caseless(sthis,s0,s1,s2)			( String_Match_Caseless(sthis, s0) || String_Match_Caseless(sthis, s1) || String_Match_Caseless(sthis, s2) )

#define String_Ends_With_Caseless_3(sthis,s0,s1,s2)	( String_Ends_With_Caseless(sthis, s0) || String_Ends_With_Caseless(sthis, s1) || String_Ends_With_Caseless(sthis, s2) )


#define String_Starts_With(s,s_prefix)					(!strncmp(s, s_prefix, strlen(s_prefix)))
#define String_Starts_With_Caseless(s,s_prefix)			(!strncasecmp(s, s_prefix, strlen(s_prefix)))

#define String_Starts_With_PRE(s,s_prefix)				(!strncmp(s, s_prefix, sizeof(s_prefix) - 1 ))
#define String_NOT_Start_With_PRE(s,s_prefix)			(!!strncmp(s, s_prefix, sizeof(s_prefix) - 1 ))
#define String_Starts_With_Caseless_PRE(s,s_prefix)		(!strncasecmp(s, s_prefix, sizeof(s_prefix) - 1 ))
#define String_NOT_Start_With_Caseless_PRE(s,s_prefix)	(!!strncasecmp(s, s_prefix, sizeof(s_prefix) - 1 ))

#define STRINGLEN(s_literal) (sizeof(s_literal) - 1)

// FN ...

int String_Range_Count_Char (const char *s_start, const char *s_end, int ch_findchar);
char *String_Find_End (const char *s); // Returns pointer to last character of string or NULL if zero length string.

char *String_Skip_WhiteSpace_Excluding_Space (const char *s);
char *String_Skip_WhiteSpace_Including_Space (const char *s);
char *String_Replace_Len_Count_Malloc (const char *s, const char *s_find, const char *s_replace, /*reply*/ int *created_length, replyx size_t *created_bufsize, replyx int *replace_count);
char *String_Range_Find_Char (const char *s_start, const char *s_end, int ch_findchar);
char *String_Edit_Whitespace_To_Space (char *s_edit);
char *String_Edit_Unquote (char *s_edit);
#define String_Edit_DeQuote String_Edit_Unquote
char *String_Edit_Trim (char *s_edit);
char *String_Edit_Replace (char *s_edit, size_t s_size, const char *s_find, const char *s_replace); // no alloc
char *String_Edit_Replace_Float (char *s_edit, size_t s_size, const char *s_find, float mynumber);
char *String_Replace_Malloc (const char *s, const char *s_find, const char *s_replace);
char *String_Edit_RTrim_Whitespace_Including_Spaces (char *s_edit);

int String_Edit_Remove_This_Trailing_Character (char *s_edit, int ch);
char *String_Edit_RemoveTrailingUnixSlash (char *s_edit);

void String_Edit_To_Single_Line (char *s_edit); // Strips newlines, carriage returns and backspaces

char *Clipboard_Get_Text_Line_Static (void);

char *String_Num_To_Thousands_Sbuf (int64_t num64);
char *String_Worldspawn_Value_For_Key_Sbuf (const char *s_entities_string, const char *find_keyname);

// returns number of keys printed
int String_Worldspawn_Value_For_Key_Con_PrintLine (const char *s_entities_string);


qbool String_Is_All_AlphaNumeric_Underscore(ccs *s);

// String_Ends_With is too complex for PRE (preprocessed version) .. move along ...
int String_Ends_With (const char *s, const char *s_suffix); 

#define		String_Is_Dot(s)	(s[0] == '.' && s[1] == NULL_CHAR_0)
#define		String_Is_DotDot(s)	(s[0] == '.' && s[1] == '.' && s[2] == NULL_CHAR_0)

#if 1
	#define String_Contains(s,s_find) (!!strstr(s,s_find))
#else
	int String_Contains (const char *s, const char *s_find);
#endif

int String_Contains_Caseless (const char *s, const char *s_find);
int String_Ends_With_Caseless (const char *s, const char *s_suffix);
int String_Has_Uppercase (const char *s);
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
int File_String_To_File (const char *path_to_file, const char *s);

SBUF___ const char *File_Getcwd_SBuf (void); // No trailing slash
char *File_URL_Edit_SlashesBack_Like_Windows (char *unix_path_to_file);
char *File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (char *path_to_file);
const char *File_URL_SkipPath (const char *path_to_file); // last path component
char *File_URL_Remove_Trailing_Unix_Slash (char *path_to_file);

int File_Exists (const char *path_to_file_);
int File_Is_Existing_File (const char *path_to_file);

#define File_To_String_Alloc(FILENAME, REPLY_BYTES_OPTIONAL_SIZE_T) File_To_Memory_Alloc (FILENAME, REPLY_BYTES_OPTIONAL_SIZE_T) // Reply is a blob.

int String_Count_Char (const char *s, int ch_findchar);
char *String_Instance_Malloc_Base1 (const char *s, int ch_delim, int nth_instance, replyx int *len);

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


// Baker: Zip support

qbool Zip_List_Print_Is_Ok (const char *zipfile_url);

// bufsize like SIV_DECOMPRESS_BUFSIZE_16_MB

unsigned char *string_zlib_compress_alloc (const char *s_text_to_compress, /*reply*/ size_t *size_out, size_t buffersize);
char *string_zlib_decompress_alloc (unsigned char *data_binary_of_compressed_text, size_t datasize, size_t buffersize);

// Baker: All of this is to allow fast string concatenation.  Simplest way to do it.

// "baker_string_t" allows NULLs inside the string, supports fast concatenation
// and tends to over-allocate by 128 to avoid reallocations for fast strcat.
// The performance with "strcat" (we aren't actually ever using strcat, rather memmove)
// is roughly 10-30 times faster than strcat in heavy strcat situations.

typedef struct {
	/*ALLOC___*/	const char					*string;						// On creation, a reference to dynamic_empty_string
					size_t						length;
					size_t						bufsize;						// Size of the buffer.
} baker_string_t; // We aren't going to need big strings, so using simpler version.

//baker_string_t *BakerString_Destroy (/*modify*/ baker_string_t *dst);
void BakerString_Destroy_And_Null_It (baker_string_t **pdst);
baker_string_t *BakerString_Create_Malloc (const char *s);


void BakerString_Set (baker_string_t *dst, int s_len, const char *s);

// Baker: Do not have string to cat be inside the string receiving cat. This version does not allow that.
void BakerString_Cat_No_Collide (/*modify*/ baker_string_t *dst, size_t s_len, const char *s);
void BakerString_CatC (/*modify*/ baker_string_t *dst, const char *s);
void BakerString_CatCFmt  (/*modify*/ baker_string_t *dst, const char *fmt, ...) DP_FUNC_PRINTF(2);


#ifdef _WIN32
	#define VA_EXPAND_ALLOC(_text, _len, _siz16, _fmt) \
		char		*_text; \
		int			_len; \
		size_t		_siz16; \
		va_list		argptr; \
		va_start	(argptr, _fmt); \
		_text =		_length_vsnprintf (false /* not a test */, &_len, &_siz16, _fmt, argptr); \
		va_end		(argptr) // Ender
#else
	#define VA_EXPAND_ALLOC(_text, _len, _siz16, _fmt) \
		char		*_text; \
		int			_len; \
		size_t		_siz16; \
		va_list		argptr; \
		va_start	(argptr, _fmt); \
		va_list		copy; \
		va_copy		(copy, argptr); \
		_text =		_length_vsnprintf (false /* not a test */, &_len, &_siz16, _fmt, copy); \
		va_end		(copy); \
		va_end		(argptr) // Ender
#endif

#define VA_EXPAND_ALLOC_FREE(TEXT) free (TEXT)

char *_length_vsnprintf (qbool just_test, /*reply*/ int *created_length, /*reply*/ size_t *created_bufsize, const char *fmt, va_list args);

void *core_memdup_z (const void *src, size_t len, /*optional*/ size_t *bufsize_made);

void *Z_MemDup_Z (const void *src, size_t len); // null terminated z_malloc //Z_Free(s); to free

#define Z_StrDup_Len_Z(s,slen) (char *)Z_MemDup_Z (s,slen)

#define Z_StrDup_Realloc_(var, s) \
	if (var) { \
		Z_FreeNull_ (var); \
	} \
	var = Z_StrDup (s)  // Ender


double File_Time (const char *path_to_file);

char *String_Edit_Replace_Char (char *s_edit, int ch_find, int ch_replace, replyx int *outcount);

char *Z_StrDupf (const char *fmt, ...) DP_FUNC_PRINTF(1);
void Z_StrDupf_Realloc (char **ps, const char *fmt, ...) DP_FUNC_PRINTF(2);
void Z_StrDup_Len_Z_Realloc (char **ps, const char *s, int slen);
char *Z_StrRepeat_Z (char ch, int count);

char *_c_strlcpy_size_z (char *dst, size_t dst_sizeof, const char *src, size_t src_length);
#define c_strlcpy_size_z(dst,src,len)  _c_strlcpy_size_z(dst, sizeof(dst), src, len)

void *Mem_DupZ (mempool_t *mempool, const void *src, size_t len);

typedef struct {
	byte	*buf_malloc;
	int		cursor;
	size_t	filesize;
	int		did_malloc; // Baker: If we didn't malloc the data, we did malloc the bakerbuf_t
} bakerbuf_t;


int baker_read (bakerbuf_t *bb, void *dst, size_t readsize);
int baker_lseek(bakerbuf_t *bb, int offset, int whence);
unsigned short baker_read_usshort(bakerbuf_t *bb);
bakerbuf_t *baker_close (bakerbuf_t *bb);
qbool baker_open_actual_file_is_ok (bakerbuf_t *bb, const char *path);
qbool baker_open_from_memory_is_ok (bakerbuf_t *bb, const void *data, size_t datalen);

// Baker: Doesn't malloc the buffer (references it), but does malloc a bakerbuf_t
// So yeah poorly named for now.
bakerbuf_t *baker_open_from_memory_NO_MALLOC_is_ok (const void *data, size_t datalen);

#define return_if_msg_(cond,msg) \
	if (cond) { \
		Con_PrintLinef (msg); \
		return; \
	} // Ender


void File_URL_Edit_Default_Extension (char *path_to_file, const char *dot_new_extension, size_t bufsize);
const char *File_URL_GetExtension (const char *path_to_file);
// Returns extension with . (like .png) after last slash if exists, returns NULL if nothing found.
ccs *File_URL_GetExtensionEx (ccs *path_to_file);

int String_Replace_Proxy_ZWork (char **pzstr, ccs *text, ccs *s_replace, ccs *offset_start, ccs *offset_beyond);
char *String_Edit_Replace_Memory_Constant (char *s_edit, size_t s_size, const char *s_find, const char *s_replace);

bgra4 *Image_Bilinear_Resize_ZAlloc (const bgra4 *rgba, int width, int height, int new_width, int new_height);
int Image_Rect_Fill3 (void *pels, unsigned rowbytes, int x, int y, int paint_width, int paint_height, int pixel_bytes, unsigned fill_color);
void *Image_Enlarge_Canvas_ZAlloc (const void *pels, int width, int height, int pixel_bytes, int new_width, int new_height, unsigned fillcolor, int is_centered);
int Image_Save_JPEG_Is_Ok (ccs *filename, bgra4 *pels_bgra, int width, int height);
int StringToFileIsOk (ccs *filename, ccs *s);
int StringToFileConPrintIsOk (ccs *filename, ccs *s);

char *String_Find_Reverse (const char *s, const char *s_find);

#endif // ! COMMON_H
