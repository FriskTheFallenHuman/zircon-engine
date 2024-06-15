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
// common.c -- misc functions used in client and server

#include <stdlib.h>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "quakedef.h"
#include "common.h" // Courtesy
#include "utf8lib.h"

cvar_t registered = {CF_CLIENT | CF_SERVER, "registered","0", "indicates if this is running registered quake (whether gfx/pop.lmp was found)"};
cvar_t cmdline = {CF_CLIENT | CF_SERVER, "cmdline","0", "contains commandline the engine was launched with"};

// FIXME: Find a better place for these.
cvar_t cl_playermodel = {CF_CLIENT | CF_SERVER | CF_USERINFO | CF_ARCHIVE, "playermodel", "", "current player model in Nexuiz/Xonotic"};
cvar_t cl_playerskin = {CF_CLIENT | CF_SERVER | CF_USERINFO | CF_ARCHIVE, "playerskin", "", "current player skin in Nexuiz/Xonotic"};

char com_token[MAX_INPUTLINE_16384];

sys_t sys;
//===========================================================================

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

unsigned char *SZ_GetSpace (sizebuf_t *buf, int length)
{
	unsigned char *data;

	if (buf->cursize + length > buf->maxsize) {
		if (!buf->allowoverflow)
			Host_Error_Line ("SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			Host_Error_Line ("SZ_GetSpace: %d is > full buffer size", length);

		buf->overflowed = true;
		Con_Print("SZ_GetSpace: overflow\n");
		SZ_Clear (buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const unsigned char *data, int length)
{
	memcpy (SZ_GetSpace(buf,length),data,length);
}

// LadyHavoc: thanks to Fuh for bringing the pure evil of SZ_Print to my
// attention, it has been eradicated from here, its only (former) use in
// all of darkplaces.

static const char *hexchar = "0123456789ABCDEF";
void Com_HexDumpToConsole(const unsigned char *data, int size)
{
	int i, j, n;
	char text[1024];
	char *cur, *flushpointer;
	const unsigned char *d;
	cur = text;
	flushpointer = text + 512;
	for (i = 0;i < size;)
	{
		n = 16;
		if (n > size - i)
			n = size - i;
		d = data + i;
		// print offset
		*cur++ = hexchar[(i >> 12) & 15];
		*cur++ = hexchar[(i >>  8) & 15];
		*cur++ = hexchar[(i >>  4) & 15];
		*cur++ = hexchar[(i >>  0) & 15];
		*cur++ = ':';
		// print hex
		for (j = 0;j < 16;j++)
		{
			if (j < n)
			{
				*cur++ = hexchar[(d[j] >> 4) & 15];
				*cur++ = hexchar[(d[j] >> 0) & 15];
			}
			else
			{
				*cur++ = ' ';
				*cur++ = ' ';
			}
			if ((j & 3) == 3)
				*cur++ = ' ';
		}
		// print text
		for (j = 0;j < 16;j++)
		{
			if (j < n)
			{
				// color change prefix character has to be treated specially
				if (d[j] == STRING_COLOR_TAG)
				{
					*cur++ = STRING_COLOR_TAG;
					*cur++ = STRING_COLOR_TAG;
				}
				else if (d[j] >= (unsigned char) ' ')
					*cur++ = d[j];
				else
					*cur++ = '.';
			}
			else
				*cur++ = ' ';
		}
		*cur++ = '\n';
		i += n;
		if (cur >= flushpointer || i >= size)
		{
			*cur++ = 0;
			Con_Print(text);
			cur = text;
		}
	}
}

void SZ_HexDumpToConsole(const sizebuf_t *buf)
{
	Com_HexDumpToConsole(buf->data, buf->cursize);
}


//============================================================================

/*
==============
COM_Wordwrap

Word wraps a string. The wordWidth function is guaranteed to be called exactly
once for each word in the string, so it may be stateful, no idea what that
would be good for any more. At the beginning of the string, it will be called
for the char 0 to initialize a clean state, and then once with the string " "
(a space) so the routine knows how long a space is.

In case no single character fits into the given width, the wordWidth function
must return the width of exactly one character.

Wrapped lines get the isContinuation flag set and are continuationWidth less wide.

The sum of the return values of the processLine function will be returned.
==============
*/
int COM_Wordwrap_Num_Rows_Drawn(const char *string, size_t length, float continuationWidth, float maxWidth, COM_WordWidthFunc_t wordWidth, void *passthroughCW, COM_LineProcessorFunc processLine, void *passthroughPL)
{
	// Logic is as follows:
	//
	// For each word or whitespace:
	//   Newline found? Output current line, advance to next line. This is not a continuation. Continue.
	//   Space found? Always add it to the current line, no matter if it fits.
	//   Word found? Check if current line + current word fits.
	//     If it fits, append it. Continue.
	//     If it doesn't fit, output current line, advance to next line. Append the word. This is a continuation. Continue.

	qbool isContinuation = false;
	float spaceWidth;
	const char *startOfLine = string;
	const char *cursor = string;
	const char *end = string + length;
	float spaceUsedInLine = 0;
	float spaceUsedForWord;
	int result = 0;
	size_t wordLen;
	size_t dummy;

	dummy = 0;
	wordWidth(passthroughCW, NULL, &dummy, -1);
	dummy = 1;
	spaceWidth = wordWidth(passthroughCW, " ", &dummy, -1);

	for(;;)
	{
		char ch = (cursor < end) ? *cursor : 0;
		switch(ch)
		{
			case 0: // end of string
				result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
				goto out;
			case '\n': // end of line
				result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
				isContinuation = false;
				++cursor;
				startOfLine = cursor;
				break;
			case ' ': // space
				++cursor;
				spaceUsedInLine += spaceWidth;
				break;
			default: // word
				wordLen = 1;
				while(cursor + wordLen < end)
				{
					switch(cursor[wordLen])
					{
						case 0:
						case '\n':
						case ' ':
							goto out_inner;
						default:
							++wordLen;
							break;
					}
				}
				out_inner:
				spaceUsedForWord = wordWidth(passthroughCW, cursor, &wordLen, maxWidth - continuationWidth); // this may have reduced wordLen when it won't fit - but this is GOOD. TODO fix words that do fit in a non-continuation line
				if (wordLen < 1) // cannot happen according to current spec of wordWidth
				{
					wordLen = 1;
					spaceUsedForWord = maxWidth + 1; // too high, forces it in a line of itself
				}
				if (spaceUsedInLine + spaceUsedForWord <= maxWidth || cursor == startOfLine)
				{
					// we can simply append it
					cursor += wordLen;
					spaceUsedInLine += spaceUsedForWord;
				}
				else
				{
					// output current line
					result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
					isContinuation = true;
					startOfLine = cursor;
					cursor += wordLen;
					spaceUsedInLine = continuationWidth + spaceUsedForWord;
				}
		}
	}
	out:

	return result;

/*
	qbool isContinuation = false;
	float currentWordSpace = 0;
	const char *currentWord = 0;
	float minReserve = 0;

	float spaceUsedInLine = 0;
	const char *currentLine = 0;
	const char *currentLineEnd = 0;
	float currentLineFinalWhitespace = 0;
	const char *p;

	int result = 0;
	minReserve = charWidth(passthroughCW, 0);
	minReserve += charWidth(passthroughCW, ' ');

	if (maxWidth < continuationWidth + minReserve)
		maxWidth = continuationWidth + minReserve;

	charWidth(passthroughCW, 0);

	for(p = string; p < string + length; ++p)
	{
		char c = *p;
		float w = charWidth(passthroughCW, c);

		if (!currentWord)
		{
			currentWord = p;
			currentWordSpace = 0;
		}

		if (!currentLine)
		{
			currentLine = p;
			spaceUsedInLine = isContinuation ? continuationWidth : 0;
			currentLineEnd = 0;
		}

		if (c == ' ')
		{
			// 1. I can add the word AND a space - then just append it.
			if (spaceUsedInLine + currentWordSpace + w <= maxWidth)
			{
				currentLineEnd = p; // note: space not included here
				currentLineFinalWhitespace = w;
				spaceUsedInLine += currentWordSpace + w;
			}
			// 2. I can just add the word - then append it, output current line and go to next one.
			else if (spaceUsedInLine + currentWordSpace <= maxWidth)
			{
				result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
				currentLine = 0;
				isContinuation = true;
			}
			// 3. Otherwise, output current line and go to next one, where I can add the word.
			else if (continuationWidth + currentWordSpace + w <= maxWidth)
			{
				if (currentLineEnd)
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
				currentLine = currentWord;
				spaceUsedInLine = continuationWidth + currentWordSpace + w;
				currentLineEnd = p;
				currentLineFinalWhitespace = w;
				isContinuation = true;
			}
			// 4. We can't even do that? Then output both current and next word as new lines.
			else
			{
				if (currentLineEnd)
				{
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}
				result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
				currentLine = 0;
				isContinuation = true;
			}
			currentWord = 0;
		}
		else if (c == '\n')
		{
			// 1. I can add the word - then do it.
			if (spaceUsedInLine + currentWordSpace <= maxWidth)
			{
				result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
			}
			// 2. Otherwise, output current line, next one and make tabula rasa.
			else
			{
				if (currentLineEnd)
				{
					processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}
				result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
			}
			currentWord = 0;
			currentLine = 0;
			isContinuation = false;
		}
		else
		{
			currentWordSpace += w;
			if (
				spaceUsedInLine + currentWordSpace > maxWidth // can't join this line...
				&&
				continuationWidth + currentWordSpace > maxWidth // can't join any other line...
			)
			{
				// this word cannot join ANY line...
				// so output the current line...
				if (currentLineEnd)
				{
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}

				// then this word's beginning...
				if (isContinuation)
				{
					// it may not fit, but we know we have to split it into maxWidth - continuationWidth pieces
					float pieceWidth = maxWidth - continuationWidth;
					const char *pos = currentWord;
					currentWordSpace = 0;

					// reset the char width function to a state where no kerning occurs (start of word)
					charWidth(passthroughCW, ' ');
					while(pos <= p)
					{
						float w = charWidth(passthroughCW, *pos);
						if (currentWordSpace + w > pieceWidth) // this piece won't fit any more
						{
							// print everything until it
							result += processLine(passthroughPL, currentWord, pos - currentWord, currentWordSpace, true);
							// go to here
							currentWord = pos;
							currentWordSpace = 0;
						}
						currentWordSpace += w;
						++pos;
					}
					// now we have a currentWord that fits... set up its next line
					// currentWordSpace has been set
					// currentWord has been set
					spaceUsedInLine = continuationWidth;
					currentLine = currentWord;
					currentLineEnd = 0;
					isContinuation = true;
				}
				else
				{
					// we have a guarantee that it will fix (see if clause)
					result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace - w, isContinuation);

					// and use the rest of this word as new start of a line
					currentWordSpace = w;
					currentWord = p;
					spaceUsedInLine = continuationWidth;
					currentLine = p;
					currentLineEnd = 0;
					isContinuation = true;
				}
			}
		}
	}

	if (!currentWord)
	{
		currentWord = p;
		currentWordSpace = 0;
	}

	if (currentLine) // Same procedure as \n
	{
		// Can I append the current word?
		if (spaceUsedInLine + currentWordSpace <= maxWidth)
			result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
		else
		{
			if (currentLineEnd)
			{
				result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
				isContinuation = true;
			}
			result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
		}
	}

	return result;
*/
}

/*
==============
COM_ParseToken_Simple

Parse a token out of a string
==============
*/

// Baker: Returns false on failure, true on success
// Baker: Comment aware quote grouping white space delimited parsing
int COM_ParseToken_Simple(const char **datapointer, qbool returnnewline, qbool parsebackslash, qbool parsecomments)
{
	int len;
	int c;
	const char *data = *datapointer;

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (parsecomments && data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (parsecomments && data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"')
	{
		// quoted string
		for (data++;*data && *data != '\"';data++)
		{
			c = *data;
			if (*data == '\\' && parsebackslash)
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = 0;
		if (*data == '\"')
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data);data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
}

char *COM_Parse_FTE (const char *data, char *out, size_t outlen)
{
	int		c;
	int		len;

//	if (out == com_token)
//		COM_AssertMainThread("COM_ParseOut: com_token");

	len = 0;
	out[0] = 0;
#if 0
	if (toktype)
		*toktype = TTP_EOF;
#endif

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*')
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
#if 0
		if (toktype)
			*toktype = TTP_STRING;
#endif

		data++;
		while (1)
		{
			if (len >= (int)outlen-1)
			{
				out[len] = 0;
				return (char*)data;
			}

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// parse a regular word
#if 0
	if (toktype)
		*toktype = TTP_RAWTOKEN;
#endif
	do
	{
		if (len >= (int)outlen-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	out[len] = 0;
	return (char*)data;
}

/*
==============
COM_ParseToken_QuakeC

Parse a token out of a string
==============
*/
// Baker: Returns true if got one
int COM_ParseToken_QuakeC(const char **datapointer, qbool returnnewline)
{
	int len;
	int c;
	const char *data = *datapointer;

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"' || *data == '\'')
	{
		// quoted string
		char quote = *data;
		for (data++;*data && *data != quote;data++)
		{
			c = *data;
			if (*data == '\\')
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = 0;
		if (*data == quote)
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n' || *data == '{' || *data == '}' || *data == ')' || *data == '(' || *data == ']' || *data == '[' || *data == ':' || *data == ',' || *data == ';')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data) && *data != '{' && *data != '}' && *data != ')' && *data != '(' && *data != ']' && *data != '[' && *data != ':' && *data != ',' && *data != ';';data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
}

/*
==============
COM_ParseToken_VM_Tokenize

Parse a token out of a string
==============
*/
// Baker: This has 2 uses.
// LoadFont to parse out font sizes like "8 12 16"
WARP_X_CALLERS_ (VM_loadfont, VM_tokenize/*, bsp?*/)
// VM_tokenize is what versus tokenize console?

//Splits up the given string into its different components (what constitutes a token separator
//is not well defined and has been hacked about with over the years so have fun with that), returning
//the number of tokens that were found. Call argv(0 through ret-1) to retrieve each individual token.
//Take care to not use this recursively

int COM_ParseToken_VM_Tokenize(const char **datapointer, qbool returnnewline)
{
	int len;
	int c;
	const char *data = *datapointer;

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"' || *data == '\'')
	{
		char quote = *data;
		// quoted string
		for (data++;*data && *data != quote;data++)
		{
			c = *data;
			if (*data == '\\')
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = 0;
		if (*data == quote)
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n' || *data == '{' || *data == '}' || *data == ')' || *data == '(' || *data == ']' || *data == '[' || *data == ':' || *data == ',' || *data == ';')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data) && *data != '{' && *data != '}' && *data != ')' && *data != '(' && *data != ']' && *data != '[' && *data != ':' && *data != ',' && *data != ';';data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = 0;
		*datapointer = data;
		return true;
	}
}

/*
==============
COM_ParseToken_Console

Parse a token out of a string, behaving like the qwcl console
==============
*/
int COM_ParseToken_Console(const char **datapointer)
{
	int len;
	const char *data = *datapointer;

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	for (;ISWHITESPACE(*data);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	if (*data == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (*data == '\"')
	{
		// quoted string
		for (data++;*data && *data != '\"';data++)
		{
			// allow escaped " and \ case
			if (*data == '\\' && (data[1] == '\"' || data[1] == '\\'))
				data++;
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		}
		com_token[len] = 0;
		if (*data == '\"')
			data++;
		*datapointer = data;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data);data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = 0;
		*datapointer = data;
	}

	return true;
}

/*
===============
Com_CalcRoll

Used by view and sv_user
===============
*/
float Com_CalcRoll (const vec3_t angles, const vec3_t velocity, /*rollangle*/ const vec_t angleval, /*rollspeed*/ const vec_t velocityval)
{
	vec3_t	forward, right, up;
	float	sign;
	float	side;

	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	if (side < velocityval)
		side = side * angleval / velocityval;
	else
		side = angleval;

	return side*sign;

}

//===========================================================================

/*
================
COM_Init
================
*/
void COM_InitOnce_Commands (void)
{
	int i, j, n;
	char com_cmdline[MAX_INPUTLINE_16384];

	Cvar_RegisterVariable (&registered);
	Cvar_RegisterVariable (&cmdline);
	Cvar_RegisterVariable(&cl_playermodel);
	Cvar_RegisterVariableAlias(&cl_playermodel, "_cl_playermodel");
	Cvar_RegisterVariable(&cl_playerskin);
	Cvar_RegisterVariableAlias(&cl_playerskin, "_cl_playerskin");

	// reconstitute the command line for the cmdline externally visible cvar
	n = 0;
	for (j = 0; (j < MAX_NUM_ARGVS_50) && (j < sys.argc); j++) {
		i = 0;
		if (strstr(sys.argv[j], " "))
		{
			// arg contains whitespace, store quotes around it
			// This condition checks whether we can allow to put
			// in two quote characters.
			if (n >= ((int)sizeof(com_cmdline) - 2))
				break;
			com_cmdline[n++] = '\"';
			// This condition checks whether we can allow one
			// more character and a quote character.
			while ((n < ((int)sizeof(com_cmdline) - 2)) && sys.argv[j][i])
				// FIXME: Doesn't quote special characters.
				com_cmdline[n++] = sys.argv[j][i++];
			com_cmdline[n++] = '\"';
		}
		else
		{
			// This condition checks whether we can allow one
			// more character.
			while ((n < ((int)sizeof(com_cmdline) - 1)) && sys.argv[j][i])
				com_cmdline[n++] = sys.argv[j][i++];
		}
		if (n < ((int)sizeof(com_cmdline) - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}
	com_cmdline[n] = 0;
	Cvar_SetQuick(&cmdline, com_cmdline);
}

/*
============
va

varargs print into provided buffer, returns buffer (so that it can be called in-line, unlike dpsnprintf)
============
*/
char *va(char *buf, size_t buflen, const char *format, ...)
{
	va_list argptr;

	va_start (argptr, format);
	dpvsnprintf (buf, buflen, format,argptr);
	va_end (argptr);

	return buf;
}


//======================================

// snprintf and vsnprintf are NOT portable. Use their DP counterparts instead

#undef snprintf
#undef vsnprintf

#ifdef _WIN32
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#endif



/*
FUNCTION
	<<strcasestr>>---case-insensitive character string search

INDEX
	strcasestr

ANSI_SYNOPSIS
	#include <string.h>
	char *strcasestr(const char *<[s]>, const char *<[find]>);

TRAD_SYNOPSIS
	#include <string.h>
	int strcasecmp(<[s]>, <[find]>)
	char *<[s]>;
	char *<[find]>;

DESCRIPTION
	<<strcasestr>> searchs the string <[s]> for
	the first occurrence of the sequence <[find]>.  <<strcasestr>>
	is identical to <<strstr>> except the search is
	case-insensitive.

RETURNS

	A pointer to the first case-insensitive occurrence of the sequence
	<[find]> or <<NULL>> if no match was found.

PORTABILITY
<<strcasestr>> is in the Berkeley Software Distribution.

<<strcasestr>> requires no supporting OS subroutines. It uses
tolower() from elsewhere in this library.

QUICKREF
	strcasestr
*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The quadratic code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Linear algorithm Copyright (C) 2008 Eric Blake
 * Permission to use, copy, modify, and distribute the linear portion of
 * software is freely granted, provided that this notice is preserved.
 */

#include <ctype.h>
#include <string.h>

/*
 * Find the first occurrence of find in s, ignore case.
 */
char * dpstrcasestr(const char *s, const char *find)
{

  /* Less code size, but quadratic performance in the worst case.  */
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		//} while (String_Starts_With_Caseless (s, find, len) != 0);
		s--;
	}
	return ((char *)s);

}


/* Strrstr.c, included for those computers that do not have it. */
/* Written by Kent Irwin, irwin@leland.stanford.edu.  I am
   responsible for bugs */

char *dp_strstr_reverse(const char *s1, const char *s2)
{
	const char *sc2, *psc1, *ps1;

	if (*s2 == '\0')
		return((char *)s1);

	ps1 = s1 + strlen(s1);

	while(ps1 != s1) {
		--ps1;
		for (psc1 = ps1, sc2 = s2; ; )
			if (*(psc1++) != *(sc2++))
				break;
			else if (*sc2 == '\0')
				return ((char *)ps1);
	}
	return ((char *)NULL);
}

// Notes: There are 32 static temp buffers which are cycled.  Best for short-lived vars in non-recursive functions.

#define CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32 32
#define BUF_SIZE_1024 1024
char *va32 (const char *format, ...)
{

	static char 	buffers[CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32][BUF_SIZE_1024];
	//static size_t 	sizeof_a_buffer 	= sizeof(buffers[0]);
	static size_t 	num_buffers			= sizeof(buffers) / sizeof(buffers[0]);
	static size_t 	cycle = 0;

	char			*buffer_to_use = buffers[cycle];
	va_list 		args;

	va_start 		(args, format);

	//int result =
	dpvsnprintf (buffer_to_use, BUF_SIZE_1024, format, args);

	va_end 			(args);

	// Cycle through to next buffer for next time function is called
	if (++cycle >= num_buffers)
		cycle = 0;

	return buffer_to_use;
#undef BUF_SIZE_1024
}

int dpsnprintf (char *buffer, size_t buffersize, const char *format, ...)
{
	va_list args;
	int result;

	va_start (args, format);
	result = dpvsnprintf (buffer, buffersize, format, args);
	va_end (args);

	return result;
}


int dpvsnprintf (char *buffer, size_t buffersize, const char *format, va_list args)
{
	int result;

#if _MSC_VER >= 1400
	result = _vsnprintf_s (buffer, buffersize, _TRUNCATE, format, args);
#else
	result = vsnprintf (buffer, buffersize, format, args);
#endif
	if (result < 0 || (size_t)result >= buffersize)
	{
		buffer[buffersize - 1] = '\0';
		return -1;
	}

	return result;
}


//======================================

void COM_ToLowerString (const char *in, char *out, size_t size_out)
{
	if (size_out == 0)
		return;

	if (utf8_enable.integer)
	{
		*out = 0;
		while(*in && size_out > 1)
		{
			int n;
			Uchar ch = u8_getchar_utf8_enabled(in, &in);
			ch = u8_tolower(ch);
			n = u8_fromchar(ch, out, size_out);
			if (n <= 0)
				break;
			out += n;
			size_out -= n;
		}
		return;
	}

	while (*in && size_out > 1)
	{
		if (*in >= 'A' && *in <= 'Z')
			*out++ = *in++ + 'a' - 'A';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

void COM_ToUpperString (const char *in, char *out, size_t size_out)
{
	if (size_out == 0)
		return;

	if (utf8_enable.integer)
	{
		*out = 0;
		while(*in && size_out > 1)
		{
			int n;
			Uchar ch = u8_getchar_utf8_enabled(in, &in);
			ch = u8_toupper(ch);
			n = u8_fromchar(ch, out, size_out);
			if (n <= 0)
				break;
			out += n;
			size_out -= n;
		}
		return;
	}

	while (*in && size_out > 1)
	{
		if (*in >= 'a' && *in <= 'z')
			*out++ = *in++ + 'A' - 'a';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

// Baker: unused in code
int COM_StringBeginsWith(const char *s, const char *match)
{
	for (;*s && *match;s++, match++)
		if (*s != *match)
			return false;
	return true;
}

int COM_ReadAndTokenizeLine(const char **text, char **argv, int maxargc, char *tokenbuf, int tokenbufsize, const char *commentprefix)
{
	int argc, commentprefixlength;
	char *tokenbufend;
	const char *l;
	argc = 0;
	tokenbufend = tokenbuf + tokenbufsize;
	l = *text;
	commentprefixlength = 0;
	if (commentprefix)
		commentprefixlength = (int)strlen(commentprefix);
	while (*l && *l != '\n' && *l != '\r')
	{
		if (!ISWHITESPACE(*l))
		{
			if (commentprefixlength && !strncmp(l, commentprefix, commentprefixlength))
			{
				while (*l && *l != '\n' && *l != '\r')
					l++;
				break;
			}
			if (argc >= maxargc)
				return -1;
			argv[argc++] = tokenbuf;
			if (*l == '"')
			{
				l++;
				while (*l && *l != '"')
				{
					if (tokenbuf >= tokenbufend)
						return -1;
					*tokenbuf++ = *l++;
				}
				if (*l == '"')
					l++;
			}
			else
			{
				while (!ISWHITESPACE(*l))
				{
					if (tokenbuf >= tokenbufend)
						return -1;
					*tokenbuf++ = *l++;
				}
			}
			if (tokenbuf >= tokenbufend)
				return -1;
			*tokenbuf++ = 0;
		}
		else
			l++;
	}
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	if (*l == '\r')
		l++;
	if (*l == '\n')
		l++;
	*text = l;
	return argc;
}

/*
============
COM_StringLengthNoColors

calculates the visible width of a color coded string.

*valid is filled with true if the string is a valid colored string (that is, if
it does not end with an unfinished color code). If it gets filled with false, a
fix would be adding a STRING_COLOR_TAG at the end of the string.

valid can be set to NULL if the caller doesn't care.

For size_s, specify the maximum number of characters from s to use, or 0 to use
all characters until the zero terminator.
============
*/
size_t
COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid)
{
	const char *end = size_s ? (s + size_s) : NULL;
	size_t len = 0;
	for(;;)
	{
		switch((s == end) ? 0 : *s)
		{
			case 0:
				if (valid)
					*valid = true;
				return len;
			case STRING_COLOR_TAG:
				++s;
				switch((s == end) ? 0 : *s)
				{
					case STRING_COLOR_RGB_TAG_CHAR:
						if (s+1 != end && isxdigit(s[1]) &&
							s+2 != end && isxdigit(s[2]) &&
							s+3 != end && isxdigit(s[3]) )
						{
							s+=3;
							break;
						}
						++len; // STRING_COLOR_TAG
						++len; // STRING_COLOR_RGB_TAG_CHAR
						break;
					case 0: // ends with unfinished color code!
						++len;
						if (valid)
							*valid = false;
						return len;
					case STRING_COLOR_TAG: // escaped ^
						++len;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': // color code
						break;
					default: // not a color code
						++len; // STRING_COLOR_TAG
						++len; // the character
						break;
				}
				break;
			default:
				++len;
				break;
		}
		++s;
	}
	// never get here
}

/*
============
COM_StringDecolorize

removes color codes from a string.

If escape_carets is true, the resulting string will be safe for printing. If
escape_carets is false, the function will just strip color codes (for logging
for example).

If the output buffer size did not suffice for converting, the function returns
false. Generally, if escape_carets is false, the output buffer needs
strlen(str)+1 bytes, and if escape_carets is true, it can need strlen(str)*1.5+2
bytes. In any case, the function makes sure that the resulting string is
zero terminated.

For size_in, specify the maximum number of characters from in to use, or 0 to use
all characters until the zero terminator.
============
*/
qbool
COM_StringDecolorize(const char *in, size_t size_in, char *out, size_t size_out, qbool escape_carets)
{
#define APPEND(ch) do { if (--size_out) { *out++ = (ch); } else { *out++ = 0; return false; } } while(0)
	const char *end = size_in ? (in + size_in) : NULL;
	if (size_out < 1)
		return false;
	for(;;)
	{
		switch((in == end) ? 0 : *in)
		{
			case 0:
				*out++ = 0;
				return true;
			case STRING_COLOR_TAG:
				++in;
				switch((in == end) ? 0 : *in)
				{
					case STRING_COLOR_RGB_TAG_CHAR:
						if (in+1 != end && isxdigit(in[1]) &&
							in+2 != end && isxdigit(in[2]) &&
							in+3 != end && isxdigit(in[3]) )
						{
							in+=3;
							break;
						}
						APPEND(STRING_COLOR_TAG);
						if (escape_carets)
							APPEND(STRING_COLOR_TAG);
						APPEND(STRING_COLOR_RGB_TAG_CHAR);
						break;
					case 0: // ends with unfinished color code!
						APPEND(STRING_COLOR_TAG);
						// finish the code by appending another caret when escaping
						if (escape_carets)
							APPEND(STRING_COLOR_TAG);
						*out++ = 0;
						return true;
					case STRING_COLOR_TAG: // escaped ^
						APPEND(STRING_COLOR_TAG);
						// append a ^ twice when escaping
						if (escape_carets)
							APPEND(STRING_COLOR_TAG);
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': // color code
						break;
					default: // not a color code
						APPEND(STRING_COLOR_TAG);
						APPEND(*in);
						break;
				}
				break;
			default:
				APPEND(*in);
				break;
		}
		++in;
	}
	// never get here
#undef APPEND
}

//========================================================
// strlcat and strlcpy, from OpenBSD

/*
 * Copyright (c) 1998, 2015 Todd C. Miller <millert@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*	$OpenBSD: strlcat.c,v 1.19 2019/01/25 00:19:25 millert Exp $    */
/*	$OpenBSD: strlcpy.c,v 1.16 2019/01/25 00:19:25 millert Exp $    */


#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t dsize)
{
	const char *odst = dst;
	const char *osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end. */
	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	return(dlen + (src - osrc));	/* count does not include NUL */
}
#endif  // #ifndef HAVE_STRLCAT


#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

#endif  // #ifndef HAVE_STRLCPY

void FindFraction(double val, int *num, int *denom, int denomMax)
{
	int i;
	double bestdiff;
	// initialize
	bestdiff = fabs(val);
	*num = 0;
	*denom = 1;

	for(i = 1; i <= denomMax; ++i)
	{
		int inum = (int) floor(0.5 + val * i);
		double diff = fabs(val - inum / (double)i);
		if (diff < bestdiff)
		{
			bestdiff = diff;
			*num = inum;
			*denom = i;
		}
	}
}

// decodes an XPM from C syntax
char **XPM_DecodeString(const char *in)
{
	static char *tokens[257];
	static char lines[257][512];
	size_t line = 0;

	// skip until "{" token
	while(COM_ParseToken_QuakeC(&in, false) && strcmp(com_token, "{"));

	// now, read in succession: string, comma-or-}
	while(COM_ParseToken_QuakeC(&in, false))
	{
		tokens[line] = lines[line];
		strlcpy(lines[line++], com_token, sizeof(lines[0]));
		if (!COM_ParseToken_QuakeC(&in, false))
			return NULL;
		if (String_Match(com_token, "}"))
			break;
		if (strcmp(com_token, ","))
			return NULL;
		if (line >= sizeof(tokens) / sizeof(tokens[0]))
			return NULL;
	}

	return tokens;
}

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void base64_3to4(const unsigned char *in, unsigned char *out, int bytes)
{
	unsigned char i0 = (bytes > 0) ? in[0] : 0;
	unsigned char i1 = (bytes > 1) ? in[1] : 0;
	unsigned char i2 = (bytes > 2) ? in[2] : 0;
	unsigned char o0 = base64[i0 >> 2];
	unsigned char o1 = base64[((i0 << 4) | (i1 >> 4)) & 077];
	unsigned char o2 = base64[((i1 << 2) | (i2 >> 6)) & 077];
	unsigned char o3 = base64[i2 & 077];
	out[0] = (bytes > 0) ? o0 : '?';
	out[1] = (bytes > 0) ? o1 : '?';
	out[2] = (bytes > 1) ? o2 : '=';
	out[3] = (bytes > 2) ? o3 : '=';
}



size_t base64_encode(unsigned char *buf, size_t buflen, size_t outbuflen)
{
	size_t blocks, i;
	// expand the out-buffer
	blocks = (buflen + 2) / 3;
	if (blocks*4 > outbuflen)
		return 0;
	for(i = blocks; i > 0; )
	{
		--i;
		base64_3to4(buf + 3*i, buf + 4*i, (int)(buflen - 3*i));
	}
	return blocks * 4;
}

// extras2

#include "common_extras2.c.h"


char *dpreplacechar (char *s_edit, int ch_find, int ch_replace)
{
	return String_Edit_Replace_Char (s_edit, ch_find, ch_replace, /*reply count*/ NULL);
}

#include "zip.c.h"

static const char *base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline int is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

//static size_t base64_encode_length (size_t slen)
//{
//    return ((slen + 2) / 3 * 4) + 1; // +1 for null
//}


char *base64_encode_calloc (const unsigned char *data, size_t in_len, /*reply*/ size_t *numbytes)
{
	const unsigned char *src;
	int outlen = (in_len + 2) / 3 * 4;
	unsigned char *out = (unsigned char *)calloc (outlen + 1 /* for the null*/, ONE_SIZEOF_CHAR_1);
	unsigned char *dst = out;
	int remaining;

	for (src = data, dst = out, remaining = in_len; remaining > 0; dst += 4, src +=3, remaining -= 3) {
		dst[0] = /* can't fail */      base64_chars[((src[0] & 0xfc) >> 2)];
		dst[1] = /* can't fail */      base64_chars[((src[0] & 0x03) << 4) + ((src[1] & 0xf0) >> 4)];
		dst[2] = remaining < 2 ? '=' : base64_chars[((src[1] & 0x0f) << 2) + ((src[2] & 0xc0) >> 6)];
		dst[3] = remaining < 3 ? '=' : base64_chars[((src[2] & 0x3f)     )];
	}

	if (dst - out != outlen) {
		Con_PrintLinef ("base64_encode_a: dst - out != outlen");
	}
	NOT_MISSING_ASSIGN(numbytes, dst-out);
	return (char *)out;
}


/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
{
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

static int sBase64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

unsigned char *base64_decode_calloc (const char *encoded_string, /*reply*/ size_t *numbytes)
{
	size_t outlen = sBase64decode_len (encoded_string);
	RETURNING_ALLOC___ char *_ret = (char *)calloc (outlen, ONE_SIZEOF_CHAR_1);
	char *ret = _ret; // ret is increased writing bytes as we go, while _ret stays the same
	size_t in_len = strlen(encoded_string);
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3] = {0}; // gcc says char_array_3 used uninitialized, but scenario looks impossible


	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i < 4; i++)
				//char_array_4[i] = base64_chars.find(char_array_4[i]);
				char_array_4[i] = (int)(strchr (base64_chars, char_array_4[i]) - base64_chars);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				*ret++ = char_array_3[i];

			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			//char_array_4[j] = base64_chars.find(char_array_4[j]);
			char_array_4[j] = (int)(strchr (base64_chars, char_array_4[j]) - base64_chars);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			*ret++ = char_array_3[j];
	}


	NOT_MISSING_ASSIGN(numbytes, (ret - _ret));
	RETURNING___ return (unsigned char *)_ret;
}





unsigned char *string_zlib_compress_alloc (const char *s_text_to_compress, /*reply*/ size_t *size_out, size_t buffersize)
{
	size_t data_zipped_bufsize = buffersize; // Like 16 MB
	unsigned char *data_zipped_alloc = (unsigned char *)calloc (1, data_zipped_bufsize);//16384 * 1024, 1); // 16 MB .. largest save file I see is 320 KB
    // original string len = 36
    size_t slen = strlen(s_text_to_compress);

    Con_DPrintLinef ("Uncompressed size is: " PRINTF_INT64, (int64_t)slen);
    //Con_PrintLinef ("Uncompressed string is: %s\n", a);

    // STEP 1.
    // deflate a into b. (that is, compress a into b)

    // zlib struct
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // setup "a" as the input and "b" as the compressed output
    defstream.avail_in = (unsigned int)slen + ONE_CHAR_1; // size of input, string + terminator
    defstream.next_in = (unsigned char*)s_text_to_compress; // input char array
    defstream.avail_out = data_zipped_bufsize; //(unsigned int)sizeof(b); // size of output
    defstream.next_out = (unsigned char *)data_zipped_alloc; // output char array

    // the actual compression work.
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);



    // This is one way of getting the size of the output
	size_t outsize1 = strlen((char *)data_zipped_alloc) ;
	size_t outsize2 = (unsigned int)(defstream.next_out - data_zipped_alloc);
    Con_DPrintLinef ("Compressed size strlen is:   " PRINTF_INT64, (int64_t)outsize1);
	Con_DPrintLinef ("Compressed size ptr math is: " PRINTF_INT64, (int64_t)outsize2);
    //Con_PrintLinef ("Compressed string is: %s\n", b);

	(*size_out) = outsize2;

	return data_zipped_alloc; // NOTE -- NOT NULL TERMINATED!!!
}
    //printf("\n----------\n\n");

// Baker: This decompresses to text.
char *string_zlib_decompress_alloc (unsigned char *data_binary_of_compressed_text, size_t datasize, size_t buffersize)
{
	size_t s_unzipped_bufsize = buffersize; // Like 16 MB
	char *s_unzipped_alloc = (char *)calloc (1, s_unzipped_bufsize);//16384 * 1024, 1); // 16 MB .. largest save file I see is 320 KB
    // original string len = 36


    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = datasize; // size of input
    infstream.next_in =  data_binary_of_compressed_text; // input char array
    infstream.avail_out = (unsigned int)s_unzipped_bufsize; // size of output
    infstream.next_out = (unsigned char *)s_unzipped_alloc; // output char array

    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    //
    //printf("Uncompressed size is: %lu\n", strlen(c));
    //printf("Uncompressed string is: %s\n", c);
    //

    //// make sure uncompressed is exactly equal to original.
    //assert(strcmp(a,c)==0);
	size_t outsize1 = strlen(s_unzipped_alloc) ;
	size_t outsize2 = (unsigned int)(infstream.next_out - (unsigned char *)s_unzipped_alloc);
    Con_DPrintLinef ("DeCompressed size strlen is:   " PRINTF_INT64, (int64_t)outsize1);
	Con_DPrintLinef ("DeCompressed size ptr math is: " PRINTF_INT64, (int64_t)outsize2);
	return s_unzipped_alloc;
}

	#ifdef _WIN32
	// The performance decreases once really large sizes are hit.  But how often are we going to be dealing with super-massive strings.
	char *_length_vsnprintf (qbool just_test, /*reply*/ int *created_length, /*reply*/ size_t *created_bufsize, const char *fmt, va_list args)
	{
		size_t bufsize = 8; // Was 2.  Set to 8 which is a defacto 16.
		char *buffer = NULL;
		int length = -1;

		// We need a length of 1 greater
		//while (length == -1 || (size_t)length >= bufsize) {
		while (length == -1 || length >= (int)bufsize) {
			bufsize = bufsize * 2;
			buffer = (char *)realloc (buffer, bufsize);
	//		logd ("c_vsnprintf_alloc: buffer size %d", bufsize);

			// For _vsnprintf, if the number of bytes to write exceeds buffer, then count bytes are written
			// and ñ1 is returned.
			//ISSUE_X_ (2, "_vsnprintf appears to be returning incorrect string length for vr 137, it returns 96?", "It seems it is affected by null characters.")
			// September 14 2021: According to stackoverflow it is possible we need to re-initialize the args???
			//  But performing experimental, this does not seem to be true.
			// Plus we have extensively used this function for several years with countless combinations of strings
			// What is more likely is that the null character in the string affected the result.
			length = _vsnprintf (buffer, bufsize, fmt, args);

			//if (length == -1 || (size_t)length >= bufsize) {
			//	//va_end(args);
			//	//va_start(args, fmt);
			//	//continue;
			//}
		}

		// Reduce the allocation to a 16 padded size, which has nothing to do with alignment but rather our stringa spec.
		// To decrease reallocation for small string changes but using a blocksize.  Leave alignment to calloc/malloc.

		if (just_test)
			free  (buffer);
		else
		{
			bufsize = roundup_16 (length + ONE_SIZEOF_NULL_TERM_1);
			buffer = (char *)realloc (buffer, bufsize); // Reduce the allocation.
		}

	//	logd ("c_vsnprintf_alloc: '%s'" NEWLINE "Length is %d", buffer, length);
		NOT_MISSING_ASSIGN(created_length, length);
		NOT_MISSING_ASSIGN(created_bufsize, bufsize);
		return buffer;
	}


	#else // Non-Windows

	// The performance decreases once really large sizes are hit.  But how often are we going to be dealing with super-massive strings.
	char *_length_vsnprintf (qbool just_test, /*reply*/ int *created_length, /*reply*/ size_t *created_bufsize, const char *fmt, va_list args)
	{
		va_list args_copy;
		va_copy (args_copy, args);
		int length = vsnprintf(NULL, 0, fmt, args_copy);

		//Sys_PrintToTerminal (va32 ("Length for vsnprintf is %f" NEWLINE, (double)length ));

		va_end (args_copy);
		size_t bufsize = roundup_16 (length + ONE_SIZEOF_NULL_TERM_1);
		char *buffer = NULL;

		if (false == just_test) {
			va_list args_copy;
			buffer = (char *)calloc(bufsize, ONE_SIZEOF_CHAR_1);
			va_copy (args_copy, args);
			int length2 = vsnprintf (buffer, length + ONE_SIZEOF_NULL_TERM_1, fmt, args_copy); // I'm not sure this null terminates?
			va_end (args_copy);

			//buffer[length] = 0;
		}

		//	logd ("c_vsnprintf_alloc: '%s'" NEWLINE "Length is %d", buffer, length);
		NOT_MISSING_ASSIGN(created_length, length);
		NOT_MISSING_ASSIGN(created_bufsize, bufsize);
		return buffer;
	}
#endif



// Destroys.  Returns null.  No string table.
void BakerString_Destroy_And_Null_It (baker_string_t **pdst)
{
	baker_string_t *dst = (*pdst);
	const char *old_string_to_free = dst->string; //iif(dst->bufsize, dst->string, NULL);

	if (old_string_to_free) free ((void *)old_string_to_free);
	free (dst);
	(*pdst) = NULL;
}

// Baker: No acquire buffer here.  No custom allocation.
baker_string_t *BakerString_Create_Malloc (const char *s)
{
	baker_string_t *dst_out = (baker_string_t *)calloc (1, sizeof(baker_string_t));
	dst_out->string = strdup ("");
	dst_out->bufsize = 1;
	return dst_out; // Allocated
}

//MemDup_Size (font_mempool, mem, mem_length)
void *Mem_DupZ (mempool_t *mempool, const void *src, size_t len)
{
	size_t bufsize_made = len + 1;
	unsigned char *membuf = (unsigned char *) Mem_Alloc(mempool, bufsize_made);// Z_Malloc(bufsize_made);
	//unsigned char *zbuf = (unsigned char *) Z_Malloc(bufsize_made);
	memcpy (membuf, src, len);
	membuf[len] =0 ;
	return membuf;
}



void *Z_MemDup_Z (/*mempool_t *mempool, */const void *src, size_t len)
{
	size_t bufsize_made = len + 1;
	//unsigned char *zbuf = (unsigned char *) Mem_Alloc(mempool, bufsize_made);// Z_Malloc(bufsize_made);
	unsigned char *zbuf = (unsigned char *) Z_Malloc(bufsize_made);
	memcpy (zbuf, src, len);
	zbuf[len] =0 ;
	return zbuf;
}

void *core_memdup_z (const void *src, size_t len, /*modify*/ size_t *bufsize_made_out)
{
	size_t bufsize_made = len + 1;

	void *buf = calloc (1, bufsize_made); // Because we are a wrapper
	memcpy (buf, src, len);
	NOT_MISSING_ASSIGN (bufsize_made_out, bufsize_made);
	return buf;
}

// Baker: No support for zero sized uninitialized buffer
void BakerString_Set (baker_string_t *dst, int s_len, const char *s)
{
	const char *old_string_to_free = iif(dst->bufsize, dst->string, NULL);
	dst->string = (const char *)core_memdup_z (s, s_len, &dst->bufsize); // a size + 1 null term copy
	dst->length = s_len;

	free ((void *)old_string_to_free);
	return;
}

// Baker: Do not have string to cat be inside the string receiving cat. This version does not allow that.
void BakerString_Cat_No_Collide (/*modify*/ baker_string_t *dst, size_t s_len, const char *s)
{
	int new_len				= dst->length + s_len;

	// Sys_PrintToTerminal (va32 ("BakerString_Cat_No_Collide is %f" NEWLINE, (double)s_len));

	if (s_len == 0) {
		// Baker: Nothing to do
		return;
	}

	size_t bufsize_current	= dst->bufsize;
	size_t bufsize_needed	= new_len + ONE_CHAR_1; // +1 for null term

	//Sys_PrintToTerminal (va32 ("BCAT Length for bufsize_needed is %f" NEWLINE, (double)bufsize_needed ));

	if (bufsize_current >= bufsize_needed) { // Buffer size if big enough
		// Cat to end
		const char *s_beyond			= &dst->string[dst->length];
			  char *s_null_term_point	= (unconstanting char *) &dst->string[new_len]; // M

		// dst->bufsize is unchanged, no realloc was needed
		dst->length = new_len;

		memcpy ((void *)s_beyond, &s[0], s_len); // Catty cat
		s_null_term_point[0] = 0;
		return;
	} else {
		// Baker: The 128 is to reduce the frequency of reallocations in the event of many small concats
		const char *s_new				= (const char *)realloc ((void *)dst->string, (bufsize_needed += /*evil*/ 128)); // UNTRACKED REALLOC



		dst->string = s_new;

		const char *s_beyond			= &dst->string[dst->length]; // After updated
			  char *s_null_term_point	= (unconstanting char *) &dst->string[new_len]; // M

		dst->bufsize = bufsize_needed;
		dst->length = new_len;

		memcpy ((void *)s_beyond, &s[0], s_len); // Catty cat
		s_null_term_point[0] = 0;

		return;
	}
}

// Returns the seconds since midnight 1970
double File_Time (const char *path_to_file)
{
	struct stat st_buf = {0};

	int status = stat (path_to_file, &st_buf );
	if (status != 0)
		return 0;

	return (double)st_buf.st_mtime;
}

char *Z_StrDupf (const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	char *out = Z_StrDup (text);
	VA_EXPAND_ALLOC_FREE (text);
	return out;
}

WARP_X_ (Z_StrDup_Realloc Z_StrDup_Len_Z)
void Z_StrDupf_Realloc (char **ps, const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	if (*ps) {
		Z_FreeNull_ (*ps);
	}
	(*ps) = Z_StrDup_Len_Z (text, text_slen);

	VA_EXPAND_ALLOC_FREE (text);
}

WARP_X_ (Z_StrDup_Len_Z)
void Z_StrDup_Len_Z_Realloc (char **ps, const char *s, int slen)
{
	if (*ps) {
		Z_FreeNull_ (*ps);
	}
	(*ps) = Z_StrDup_Len_Z (s, slen);
}


// Null terminated copy
char *_c_strlcpy_size_z (char *dst, size_t dst_sizeof, const char *src, size_t src_length)
{
	int copy_length = src_length > dst_sizeof ? (dst_sizeof - 1) : src_length;
	memcpy	(dst, src, copy_length);
	dst[copy_length] = 0;
	return dst;
}

//#include "pak.c.h" // Works, don't do it

#include "baker_mem_read.c.h"

