// common_extras2.c.h

// extras2

///////////////////////////////////////////////////////////////////////////////
//  STRING: Baker - String
///////////////////////////////////////////////////////////////////////////////


// Short: Counts instances of a character in string
// Notes: None.
// Unit Test:
int String_Count_Char (const char *s, int ch_findchar)
{
	const char *s_end = String_Find_End (s);

	if (s_end)
		return String_Range_Count_Char (s, s_end, ch_findchar);
	return 0;
}


// Short: Counts newline characters (char 10) in string
// Notes: None.
int String_Count_Newlines (const char *s)
{
	return String_Count_Char (s, '\n');
}


// Short: Counts instances of string in string
// Notes: None.
int String_Count_String (const char *s, const char *s_find)
{
	size_t len = strlen(s_find);
	int count;
	const char *search;
	for (search = strstr(s, s_find), count = 0; search; search = strstr(search + len, s_find))
		count ++;

	return count;
}


int String_Count_Words (const char *s)
{
    int count = 0;
    //printf ("Word is %s and  ... ", s);

    while (*s)
    {
        if (*s != SPACE_CHAR_32) {
            count ++;
            while (*s && *s!=SPACE_CHAR_32)
                s ++;
        }
        else {
            while (*s && *s==SPACE_CHAR_32)
                s ++;
//            in_space = 1;
        }
    }
    //printf ("count is %d", count);
	return count;
}

int String_Count_First_Word_Length (const char *s)
{
    const char *first_space = strchr (s, ' ');
    if (!first_space) return (int)strlen(s); // First word is everything.
    if (first_space == s) return -1; // First "word" is a space.
    return first_space - s;
}

#if 0 // Baker: favor macro over this
#undef String_Contains
// Short: Returns 1 if string contains string, otherwise returns 0
// Notes: For reference only, function has no advantage over strstr.
int String_Contains (const char *s, const char *s_find)
{
	if (strstr (s, s_find))
		return 1;

	return 0;
}
#endif

int String_Contains_Caseless (const char *s, const char *s_find)
{
	if (dpstrcasestr (s, s_find))
		return 1;

	return 0;
}

// Short: Returns 1 if string contains spaces, otherwise returns 0
// Notes: Offers no advantage over strchr(s, SPACE_CHAR_32).
int String_Contains_Spaces (const char *s)
{
	if (strchr(s, SPACE_CHAR_32))
		return 1;

	return 0;
}


// Short: Returns 1 if delimited string contains string as delimited member, otherwise returns 0
// Notes: If the string to find contains the delimiter character, the function returns 0.
int String_Does_List_Delimited_Contain (const char *s, const char *s_find, int ch_delimiter)
{
	const char	*match, *next_start;
	size_t		s_find_len;

	// Check for disqualifying situations ...
	if (s == NULL || *s == 0 || s_find == NULL || *s_find == 0 || strchr (s_find, ch_delimiter))
		return 0;

	s_find_len = strlen (s_find);

	for (match = strstr (s, s_find) ; match ; match = strstr (next_start, s_find))
	{
		next_start = match + s_find_len;

		// The beginning must be (1) the start of the string or (2) preceded by the delimiter
		if (match == s || *(match - 1) == ch_delimiter)
		{
			// The matching text must be followed by (1) null at end of string or (2) a delimiter
			if (*next_start == 0 || *next_start == ch_delimiter)
				return 1;
		}
	}

	return 0;
}


// Short: Returns 1 if string ends with the suffix -- considering case, otherwise returns 0
// Notes: None.
int String_Ends_With (const char *s, const char *s_suffix)
{
	size_t s_len = strlen(s);
	size_t s_suffix_len = strlen (s_suffix);
	size_t /*ssize_t*/ suffix_spot = s_len - s_suffix_len;

	if (s_len && s_suffix_len && s_suffix_len <= s_len)
		return !strcmp (&s[suffix_spot], s_suffix);

	return 0;
}


// Short: Returns 1 if string ends with the suffix -- ignoring case, otherwise returns 0
// Notes: None.
int String_Ends_With_Caseless (const char *s, const char *s_suffix)
{
	size_t s_len = strlen(s);
	size_t s_suffix_len = strlen (s_suffix);
	size_t /*ssize_t*/ suffix_spot = s_len - s_suffix_len;

	if (s_len && s_suffix_len && s_suffix_len <= s_len)
		return !strcasecmp (&s[suffix_spot], s_suffix);

	return 0;
}


// Short: Returns 1 if string contains lower case characters, otherwise returns 0
// Notes: None.
int String_Has_Lowercase (const char *s)
{
	if (s)
	{
		const char *search;
		for (search = s ; *search ; search ++)
		{
			if (islower(*search))
				return 1;
		}
	}

	return 0;
}


// Short: Returns 1 if string begins with and ends with a quote and has length greater than 1.
// Note:  A string with length 1 where the only character was a quote would return 0.
// Macro for this
//int String_Is_Quoted (const char *s)
//{
//	size_t len = strlen(s);
//	if (len > 1 && s[0] == CHAR_DQUOTE_34 && s[len - 1] == CHAR_DQUOTE_34)
//		return 1;
//	return 0;
//}


// Short: Returns 1 if string contains upper case characters, otherwise returns 0
// Notes: None.
int String_Has_Uppercase (const char *s)
{
	if (s)
	{
		const char *search;
		for (search = s ; *search ; search ++)
		{
			if (isupper(*search))
				return 1;
		}
	}

	return 0;
}


#undef String_Match
// Short: Returns 1 if string is identical --- considering case, otherwise returns 0
// Notes: Offers no advantage over using !strcmp(s1,s2).
//int String_Match (const char *s1, const char *s2)
//{
//	return !strcmp(s1, s2);
//}


//#undef String_Match_Caseless
//// Short: Returns 1 if string is identical --- ignoring case, otherwise returns 0
//// Notes: Offers no advantage over using !strcasecmp(s1,s2)
//int String_Match_Caseless (const char *s1, const char *s2)
//{
//	return !strcasecmp(s1, s2);
//}


#undef String_Does_Match_Nullproof
// Short: Returns 1 if string is identical --- considering case --- even if one or both strings are NULL, otherwise returns 0
// Notes: If string pointers are identical, the function does not perform a byte-by-byte comparison and returns 1.
int String_Does_Match_Nullproof (const char *s1, const char *s2)
{
	return ( s1 == s2 ? 1 : ((s1 == NULL || s2 == NULL) ? 0 : !strcmp (s1, s2)));
}


#undef String_NOT_Match
// Short: Returns 1 if string is different --- considering case, otherwise returns 0
// Notes: Offers no advantage over strcmp(s1, s2).
//int String_NOT_Match (const char *s1, const char *s2)
//{
//	return !!strcmp(s1, s2);
//}


#undef String_Does_Not_Match_Caseless
// Short: Returns 1 if string is different --- ignoring case, otherwise returns 0
// Notes: Offers no advantage over strcasecmp(s1, s2).
int String_Does_Not_Match_Caseless (const char *s1, const char *s2)
{
	return !!strcasecmp(s1, s2);
}


#undef String_Does_Not_Start_With
// Short: Returns 1 if string does not begin with prefix --- considering case, otherwise returns 0
// Notes: None.
int String_Does_Not_Start_With (const char *s, const char *s_prefix)
{
	return !!strncmp(s, s_prefix, strlen(s_prefix));
}


#undef String_Does_Not_Start_With_Caseless
// Short: Returns 1 if string does not begin with prefix --- ignoring case, otherwise returns 0
// Notes: None.
int String_Does_Not_Start_With_Caseless (const char *s, const char *s_prefix)
{
	return !!strncasecmp(s, s_prefix, strlen(s_prefix));
}


//#undef String_Starts_With
//// Short: Returns 1 if string begins with prefix --- considering case, otherwise returns 0
//// Notes: None.
//int String_Starts_With (const char *s, const char *s_prefix)
//{
//	return !strncmp(s, s_prefix, strlen(s_prefix));
//}


//#undef String_Starts_With_Caseless
//// Short: Returns 1 if string begins with prefix --- ignoring case, otherwise returns 0
//// Notes: None.
//int String_Starts_With_Caseless (const char *s, const char *s_prefix)
//{
//	return !strncasecmp(s, s_prefix, strlen(s_prefix));
//}


// Inserts text in a buffer at offset (with buf maxsize of bufsize) and returns inserted character count
// Notes: None.
int String_Edit_Insert_At (char *s_edit, size_t s_size, const char *s_insert, size_t offset)
{
	// about our s_edit ...
	size_t buffer_maxsize = s_size - 1; // room for null
	size_t buffer_strlen = strlen (s_edit);
	size_t buffer_remaining = buffer_maxsize - buffer_strlen;	// max insertable chars
	size_t chars_after_insert_point = buffer_strlen - offset + 1; // Need to move null term

	// now factor in insertion text
	size_t insert_text_len = strlen (s_insert);
	size_t copy_length = insert_text_len > buffer_remaining ? buffer_remaining : insert_text_len;

	if (copy_length)
	{
		memmove (s_edit + offset + copy_length, s_edit + offset, chars_after_insert_point + 1);
		memcpy (s_edit + offset, s_insert, copy_length);
	}

	return (int)copy_length;
}


// Short: Removes all leading white-spaces (char < SPACE_CHAR_32) from string except for spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_LTrim_Whitespace_Excluding_Spaces (char *s_edit)
{
	char *s_end = String_Find_End (s_edit);
	char *s_past_white = String_Skip_WhiteSpace_Excluding_Space (s_edit);

	memmove (s_edit, s_past_white, s_end - s_past_white + 1);

	return s_edit;
}


// Short: Removes all leading white-spaces (char <=SPACE_CHAR_32 ) from string including spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_LTrim_Whitespace_Including_Spaces (char *s_edit)
{
	if (s_edit[0]) // Don't bother for 0 length string
	{
		char *s_end = String_Find_End (s_edit);
		char *s_past_white = String_Skip_WhiteSpace_Including_Space (s_edit);

		memmove (s_edit, s_past_white, (s_end - s_past_white + 1 /*range size*/) + 1 /* null term */);
	}

	return s_edit;
}


// Short: Removes all trailing white-spaces (char < SPACE_CHAR_32) from string except for spaces (char SPACE_CHAR_32) by replacing with null characters
// Notes: None.
char *String_Edit_RTrim_Whitespace_Excluding_Spaces (char *s_edit)
{
	int len = (int)strlen(s_edit), j;

	for (j = len - 1; j >= 0; j --)
	{
		if (s_edit[j] < SPACE_CHAR_32)
			s_edit[j] = 0;
		else break;
	}

	return s_edit;
}


// Short: Removes all trailing white-spaces (char <= SPACE_CHAR_32) from string including for spaces (char SPACE_CHAR_32) by replacing with null characters
// Notes: None.
char *String_Edit_RTrim_Whitespace_Including_Spaces (char *s_edit)
{
	int len = (int)strlen(s_edit), j;

	for (j = len - 1; j >= 0; j --)
	{
		if (s_edit[j] <= SPACE_CHAR_32)
			s_edit[j] = 0;
		else break;
	}

	return s_edit;
}

// Short: Removes last character from string by replacing it with null character
// Notes: None.
char *String_Edit_Remove_Last_Character (char *s_edit)
{
	int len = (int)strlen(s_edit);

	if (len >= 1)
		s_edit[len-1] = 0;

	return s_edit;
}

int String_Edit_Remove_This_Trailing_Character (char *s_edit, int ch)
{
	int len = (int)strlen(s_edit);

	if (len >= 1 && s_edit[len-1] == ch) {
		s_edit[len-1] = 0;
		return true;
	}

	return false;
}


// Short: Removes trailing spaces by replacing with null characters
// Notes: None.
char *String_Edit_RemoveTrailingSpaces (char *s_edit)
{
	int /*ssize_t*/ offset;
	for (offset = (int)strlen(s_edit) - 1; offset >= 0 && s_edit[offset] == SPACE_CHAR_32; offset --)
		s_edit[offset] = 0; // remove trailing spaces

	return s_edit;
}


char *String_Edit_RemoveTrailingUnixSlash (char *s_edit)
{
	if (String_Ends_With (s_edit, "/"))
		String_Edit_Remove_Last_Character (s_edit);

	return s_edit;
}


char *String_Edit_RemoveTrailingZeros (char *s_edit)
{
	char *cursor = strchr (s_edit, '\0');

	if (cursor && cursor > s_edit) {
		for (cursor --; cursor >= s_edit && *cursor == '0'; cursor --)
			*cursor = 0;

		if (*cursor == '.')
			*cursor = 0;
	}
	return s_edit;
}

// We leave at least 1 decimal after the dot.  Examples: 2.000 -> 2.0, 2.1000 -> 2.1
char *String_Edit_To_ProperFloat (char *s_edit)
{
	char *dot = strchr (s_edit, '.');
	char *decimal2 = dot && dot[1] && dot[2] ? &dot[2] : NULL;

	if (decimal2) {
		char *cursor = strchr (decimal2, '\0' /* like the null terminator escape 0 */);
		for (cursor-- ; cursor >= decimal2 && *cursor == '0' /* like the digit zero */; cursor --)
			*cursor = 0;
	}
	return s_edit;
}


// Short: Replaces string buffer with string repeated N times
// Notes: None.
char *String_Repeat_Alloc (char ch, int count)
{
	char *out = (char *)calloc (count + ONE_SIZEOF_NULL_TERM_1, ONE_SIZEOF_CHAR_1);
	int n;
	for (n = 0; n < count; n ++)
		out[n] = ch;

	return out;
}


// Short: Replaces string buffer with string repeated N times
// Notes: None.
char *String_Edit_Repeat (char *s_edit, size_t s_size, const char *s_repeat, int count)
{
	int i;
	s_edit[0] = 0;

	for (i = 0; i < count; i ++)
		strlcat (s_edit, s_repeat, s_size);

	return s_edit;
}

void c_swapb (char *a, char *b)
{
	char c = *a;
	*a = *b;
	*b = c;
}

//http://stackoverflow.com/questions/8534274/is-the-strrev-function-not-available-in-linux
char *String_Edit_Reverse (char *s_edit)
{
    int i = (int)strlen (s_edit) - 1, j = 0;

    while (i > j)
		c_swapb (&s_edit[i--], &s_edit[j++]);

	return s_edit;
}


// Short: Deletes count from
// Notes: No safety checks at this time.  Attempts to delete null terminator or exceed buffer are not prohibited.
char *String_Edit_Delete_At (char *s_edit, size_t s_size, size_t offset, size_t num)
{
//	if (offset + num - 1 >= s_size)
//	{
		// 012345
		// frog0   String_Edit_Delete_At(s, 256, 2, 2)
		// XX__0
		char *s_after = &s_edit[offset + num];
		char *s_dest = &s_edit[offset];
		size_t move_num = s_size - offset - num - 1; // +1 for null term, -1 for net length formula
		memmove (s_dest, s_after, move_num); // Take null term with us
		return s_edit;
//	}
//	return NULL;
}

// Short: Replaces all instances of a string within a string or until reaching s_size - 1, returns s_edit
// Notes: No safety checks
char *String_Edit_Range_Delete (char *s_edit, size_t s_size, const char *s_start, const char *s_end)
{
	return String_Edit_Delete_At (s_edit, s_size, s_start - s_edit, s_end - s_start + 1);
}

// Short: Replaces all instances of a string within a string or until reaching s_size - 1
// Notes: None.
char *String_Edit_Replace (char *s_edit, size_t s_size, const char *s_find, const char *s_replace)
{
	char *first_match = s_find && s_find[0] ? strstr(s_edit, s_find) : NULL;

	// Make sure the find string actually exists in the string before going to any effort ...
	if (first_match)
	{
		int s_find_len = (int)strlen (s_find);
		int s_replace_len = (int)strlen(s_replace);
		int delta_len = s_replace_len - s_find_len;

    	// If replacement string length greater than find, duplicate source buffer.
    	// While it is possible to avoid creating this extra buffer
    	// and just memmove the remaining string after each replacement
    	// this would be inefficient.

		char *src_dup = (delta_len > 0) ? strdup (s_edit) : NULL;
		const char *src = src_dup ? src_dup : s_edit;


		// new match is first_match's offset into the source.
		// This may look repetitive but to keep Memory_Constant and regular source same
		char *new_match = (char *)&src[first_match - s_edit];

		char *dst = s_edit;
		const char *buf_start = &dst[0];
		const char *buf_end   = &dst[s_size - 1];
		const char *replace_src;

		while ( new_match )
		{
			// First copy over any characters we skipped
			while (src < new_match)
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *src ++;
			}

			// Now copy over replacement
			for (replace_src = s_replace; *replace_src; )
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *replace_src++;
			}

			// Now advance cursor past find results
			src += s_find_len;
			new_match = (char *)strstr(src, s_find);

		} // End of while loop

		// Copy over anything that remains in the buffer
		while (*src)
		{
			if (dst > buf_end)
				goto end_of_buffer;
			*dst++ = *src ++;
		}

end_of_buffer:

		// Null terminate it
		if (dst <= buf_end)
			*dst = 0;
		else if (dst > buf_start)
			dst[-1] = 0; // We would overflow

		if (src_dup)
			free (src_dup);
	}

	return s_edit;
}

char *String_Edit_Replace_Float (char *s_edit, size_t s_size, const char *s_find, float mynumber)
{
	char s_replace[64];
	c_dpsnprintf1 (s_replace, FLOAT_LOSSLESS_FORMAT, mynumber);
	return String_Edit_Replace (s_edit, s_size, s_find, s_replace);
}

char *String_Edit_Replace_Token_Array (char *s_edit, size_t s_size, const char **replace_tokens2)
{
	const char **curt;
	for (curt = replace_tokens2; *curt; curt +=2 ) {
		const char *s_find = curt[0], *s_replace = curt[1];

		String_Edit_Replace (s_edit, s_size, s_find, s_replace); // TODO: # replaces feedback would be nice :(

		//_alert ("key:\n\n%s\n\n%s", s_find, s_replace);
	}
	return s_edit;
}

typedef struct {
	int			length;	// Yes, it must be int to work with star
	ccs			*string;
} kcursor;
#define  KS_Fmt		  "%.*s"	//  printf ("%.*s", sa->length, sa->string)
#define kspv(ks) ((ks)->length), ((ks)->string)		//  printf ("%.*s", sa->length, sa->string)

// The idea is that pzstr was a clone of origtext
int String_Replace_Proxy_ZWork (char **pzstr, ccs *sorig, ccs *s_replace, ccs *sorig_start, ccs *sorig_beyond)
{
	char *s = *pzstr;
	
	size_t offset_start  = sorig_start - sorig;
	size_t offset_beyond = sorig_beyond - sorig;
	size_t replacelen = strlen(s_replace);
	
	kcursor s_start = { offset_start, s };
	kcursor s_repla = { strlen(s_replace), s_replace };
	kcursor s_after = { strlen(&s[offset_beyond]), &s[offset_beyond] }; // New string everything after offset
	size_t bufsize = {s_start.length + s_repla.length + s_after.length + ONE_CHAR_1 };
	char *snew = (char *)Mem_Alloc (zonemempool, bufsize);
	size_t pt2 = s_start.length + s_repla.length;
	size_t pt3 = pt2 + s_after.length;
	memcpy (&snew[0],				s_start.string,	s_start.length);
	memcpy (&snew[s_start.length],	s_repla.string,	s_repla.length);
	memcpy (&snew[pt2],	s_after.string,		s_after.length);
	snew[pt3] = NULL_CHAR_0; // Not technically necessary since Mem_Alloc should be all zeroes.

	Mem_FreeNull_ (*pzstr);
	*pzstr = snew;

	return true;
}

// Short: Replaces all instances of a string within a string or until reaching s_size - 1
// Notes: None.
char *String_Edit_Replace_Memory_Constant (char *s_edit, size_t s_size, const char *s_find, const char *s_replace)
{
	char *first_match = s_find && s_find[0] ? strstr(s_edit, s_find) : NULL;

	// Make sure the find string actually exists in the string before going to any effort ...
	if (first_match)
	{
		int s_find_len = (int)strlen (s_find);
		int s_replace_len = (int)strlen(s_replace);
		int delta_len = s_replace_len - s_find_len;

		const char *src = s_edit;

		// new match is first_match's offset into the source.
		// This may look repetitive but to keep Memory_Constant and regular source same
		char *new_match = (char *)&src[first_match - s_edit];

		char *dst = s_edit;
		const char *buf_start = &dst[0];
		const char *buf_end   = &dst[s_size - 1];

		const char *replace_src;

		while ( new_match )
		{
			// First copy over any characters we skipped
			while (src < new_match)
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *src ++;
			}

			// If replacement string is longer, memmove everything
			// after the current match forward the appropriate amount first ...
			if (delta_len > 0)
			{
				char *move_dst = &new_match[s_replace_len];
				char *move_src = &new_match[s_find_len];
				int move_len = buf_end - move_src - delta_len;
				if (move_len > 0)
					memmove (move_dst, move_src, move_len);

				src += delta_len;
			}

			// Now copy over replacement
			for (replace_src = s_replace; *replace_src; )
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *replace_src++;
			}

			// Now advance cursor past find results
			src += s_find_len;
			new_match = (char *)strstr(src, s_find);

		} // End of while loop

		// Copy over anything that remains in the buffer
		while (*src)
		{
			if (dst > buf_end)
				goto end_of_buffer;
			*dst++ = *src ++;
		}

	end_of_buffer:

		// Null terminate it
		if (dst <= buf_end)
			*dst = 0;
		else if (dst > buf_start)
			dst[-1] = 0; // We would overflow

	}

	return s_edit;
}


// Short: Replaces all instances of a character with another character
// Notes: None.
char *String_Edit_Replace_Char (char *s_edit, int ch_find, int ch_replace, replyx int *outcount)
{
	char *cursor;
	int n;
	int count;

	for (cursor = s_edit, count = 0, n = 0; *cursor; cursor ++, n++)
	{
		if (*cursor == ch_find)
		{
			//int c = *cursor;
			s_edit[n] = ch_replace;
			count ++;
		}
	}

	NOT_MISSING_ASSIGN(outcount, count);

	return s_edit;
}


// Short: Converts all upper case characters to lower case
// Notes: None.
char *String_Edit_To_Lower_Case (char *s_edit)
{
	char *cursor;
	for (cursor = s_edit; *cursor; cursor ++)
	{
		if ( isupper (*cursor) )
			*cursor = tolower (*cursor);
	}
	return s_edit;
}


// Short: Converts first character to upper case, remaining characters to lower case
// Notes: None.
char *String_Edit_To_Proper_Case (char *s_edit)
{
	char *cursor = s_edit;

	// Capitalize the first letter
	if (*cursor && islower(*cursor))
		*cursor = toupper(*cursor);

	cursor ++;

	// Now lower case ensure the rest
	String_Edit_To_Lower_Case (cursor);
	return s_edit;
}


// Short: Converts all lower case characters to upper case
// Notes: None.
char *String_Edit_To_Upper_Case (char *s_edit)
{
	char *cursor;
	for (cursor = s_edit; *cursor; cursor ++)
	{
		if ( islower (*cursor) )
			*cursor = toupper (*cursor);
	}
	return s_edit;
}


// Short: Removes all trailing and leading spaces from a string
// Notes: None.
char *String_Edit_Trim (char *s_edit)
{
	String_Edit_RTrim_Whitespace_Including_Spaces (s_edit);
	String_Edit_LTrim_Whitespace_Including_Spaces (s_edit);
	return s_edit;
}


// Short: Dequotes a string, if it is quoted.  Otherwise does nothing.
// Notes: None.
char *String_Edit_Unquote (char *s_edit)
{
	size_t len = strlen(s_edit);
	if (len > 1 && s_edit[0] == '\"' && s_edit[len - 1] == '\"')
	{
		s_edit[len - 1] = 0;
		memmove (s_edit, &s_edit[1], len - 2 + 1); // size is reduced by 2, add +1 for null term for memmove op
	}
	return s_edit;
}


// Short: SPACE_CHAR_32 to 126 stays.  Everything else except \n and \r g
// Notes: None.
char *String_Edit_For_Stdout_Ascii (char *s_edit)
{
	/*unsigned */char *cursor;
	int c;
	int should_print;

	for (cursor = s_edit; *cursor; cursor ++)
	{
		c = *cursor;
		should_print = in_range (SPACE_CHAR_32, c, CHAR_TILDE_126) || c == '\n'; // // && /* c != '\r' && */  c != '\t')

		if (!should_print)
			*cursor = '#';
	}

	return s_edit;
}


//int String_Is_Only_Alpha_Numeric_Plus_Space (const char *s)
//{
//	return String_Is_Only_Alpha_Numeric_Plus_Charcode (s, SPACE_CHAR_32);
//}


int String_Is_Only_Alpha_Numeric_Plus_Charcode (const char *s, int charcode)
{
	const char *cursor;
	int ch;

	for (cursor = s; *cursor; cursor ++) {
		ch = *cursor;

		if (isdigit (ch) || isalpha (ch) || ch == charcode)
			continue;

		return false;
	}

	return true; // It's only alpha numeric plus specified character.
}


// Short: Replaces all white-space characters (char < SPACE_CHAR_32) with spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_Whitespace_To_Space (char *s_edit)
{
	char *cursor;
	int c;

	for (cursor = s_edit; *cursor; cursor ++)
	{
		c = *cursor;
		if (c < SPACE_CHAR_32)
			*cursor = SPACE_CHAR_32;
	}

	return s_edit;
}

char *String_Edit_Whitespace_To_Space_Except_Newline (char *s_edit)
{
	char *cursor;
	int c;

	for (cursor = s_edit; *cursor; cursor ++)
	{
		c = *cursor;
		if (c < SPACE_CHAR_32 && c!= NEWLINE_CHAR_10)
			*cursor = SPACE_CHAR_32;
	}

	return s_edit;
}

// Short: Returns pointer to occurence of string --- considering case --- or NULL if not found
// Notes: Offers no advantage over strstr.
char *String_Find (const char *s, const char *s_find)
{
	char *search = (char *)strstr (s, s_find);
	return search;
}

char * strcasestr(const char *s, const char *find)
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
		s--;
	}
	return ((char *)s);

}

// Short: Returns pointer to occurence of string --- ignoring case --- or NULL if not found
// Notes: Offers no advantage over strcasestr.
char *String_Find_Caseless (const char *s, const char *s_find)
{
	char *search = strcasestr (s, s_find);
	return search;
}


// Short: Returns pointer to first character after instance of find string --- considering case, NULL if not found
// Notes: None.
char *String_Find_Skip_Past (const char *s, const char *s_find)
{
	char *search = (char *)strstr (s, s_find);
	if (search)
	{
		size_t len = strlen(s_find);
		return &search[len];
	}
	return NULL;
}


// Short: Returns pointer to beginning of instance of find string --- considering case, NULL if not found
// Notes: Offers no advantage over strrstr.
char *String_Find_Reverse (const char *s, const char *s_find)
{
	char *search = dp_strstr_reverse (s, s_find);
	return search;
}


// Short: Returns pointer to first instance of character, NULL if not found
// Notes: Offers no advantage over strchr.
char *String_Find_Char (const char *s, int ch_findchar)
{
	const char *s_end = String_Find_End (s);

	if (s_end)
		return String_Range_Find_Char (s, s_end, ch_findchar);

	return NULL;
}


// Short: Returns pointer to Nth occurrence of character, NULL if no such instance
// Notes: // Human friendly.  First instance = 1
char *String_Find_Char_Nth_Instance (const char *s, int ch_findchar, int nth_instance)
{
	const char *search;
	int count;

	for (search = s, count = 0; *search; search ++)
	{
		if (*search == ch_findchar) {
			count ++;
			if (count == nth_instance)
			return (char *)search;
		}
	}

	return NULL;
}

// Human friendly.  First instance = 1
char *String_Instance_Base1 (const char *s, int ch_delim, int nth_instance, replyx int *len)
{
	// If we want the nth instance, we want the nth - 1 delimiter or the string itself.
	char *found = nth_instance > 1 ? String_Find_Char_Nth_Instance (s, ch_delim, nth_instance - 1) : (/*unconstanting*/ char *)s;
	char *s_start = NULL;
	int slen = 0;

	// If it wasn't found, there wasn't a delimiter at all.
	if (!found) {
		if (nth_instance == 1) {
			NOT_MISSING_ASSIGN (len, (int)strlen(s));
			return (char *)s;
		}
		return NULL; // Invalid.
	}

	// If the found is the search string beginning don't skip a character.
	s_start = found == s ? found : found + 1;
	found = strchr (s_start, ch_delim);
	if (!found) {
		// Last instance?  Must be
		NOT_MISSING_ASSIGN (len, (int)strlen(s_start));
		return s_start;
	}

	// frog,
	// 01234   4 - 0 = 4
	slen = found - s_start;
	NOT_MISSING_ASSIGN (len, slen);
	return s_start;
}

char *dpstrndup (const char *s, size_t n)
{
  size_t len = u8_strnlen  (s, n); // The strnlen() function returns the number of bytes in the string pointed to by s, excluding the terminating null byte ('\0'), but at most maxlen
  char *snew = (char *)malloc (len + ONE_SIZEOF_NULL_TERM_1);

  if (snew == NULL)
    return NULL;

  snew[len] = '\0';
  return (char *)memcpy (snew, s, len);
}

// Returns length of the match, not length of the copy
// This thing is 1 based?
// Baker: Use String_Count_String or String_Count_Char to figure out number of delims
char *String_Instance_Malloc_Base1 (const char *s, int ch_delim, int nth_instance, replyx int *len)
{
	int slen = -1; char *found = String_Instance_Base1 (s, ch_delim, nth_instance, &slen);
	if (!found) return NULL;
	else {
		NOT_MISSING_ASSIGN (len, slen);
		return dpstrndup (found, slen);
	}

}


// Short: Returns pointer to last instance of character, NULL if not found
// Notes: Offers no advantage over strrchr.
char *String_Find_Char_Reverse (const char *s, int ch_findchar)
{
	return (char *) strrchr(s, ch_findchar);
}


// Short: Returns pointer to last character of string or NULL if zero length string.
// Notes: None.
char *String_Find_End (const char *s)
{
	size_t len = strlen(s);
	const char *s_end = len > 0 ?  &s[len-1] : NULL; // 4 length = 0 through 3 = 4 - 1 = s

	return (char *)s_end;
}


// Short: Determines if Microsoft Excel would call the string a number.  Rather accurate.
// Note: Works for typical usage for simple strings meant for CSV export or copy/paste.

int String_Does_Have_Excel_Numeric_Representation (const char *s)
{
//	const char *numbers[] = { "5", "0", ".5", "5.", "\"5", "  5"," 5 ", NULL };
//	const char *not_numbers[] = { "x5", "5x", ".", "5..", "..5", "5\"","  5  6 ", NULL };

	char *sc_a = strdup (s), ch;
	int result, pos, anydigit, dots;

	// Leading and trailing spaces are ignored.
	String_Edit_Trim (sc_a); // memmoves

	for (result = 1, pos = dots = anydigit = 0; (ch = sc_a[pos]); pos++)
	{
		if (isdigit (ch))
		{
			if (!anydigit) anydigit = 1;
		}
		else if (ch == '.')
		{ // a period
			dots ++;
			if (dots > 1)
			{
				result = 0; // more than 1 dot is never a number
				break;
			}
		}
		else if (ch == '\"')
		{ // a quote
			if (pos == 0) // Ignore a leading quote
				continue;
			result = 0; // If quote isn't leading character
			break;
		}
		else
		{ // Non-digit
			result = 0; // Non digit and non-period is never a number
			break;
		}
	}

	free (sc_a);
	if (result && anydigit) return 1;
	return 0;
}


// Short: Counts the number of instances of a character in a sub-string from start through end, 0 if not found
// Notes: None.
int String_Range_Count_Char (const char *s_start, const char *s_end, int ch_findchar)
{
	int count;
	const char *search;
	for (search = s_start, count = 0; *search && search <= s_end; search ++)
		if (*search == ch_findchar)
			count ++;
	return count;
}


// Short: Finds a character in a sub-string, returns NULL if not found
// Notes: Advantage: strchr could waste time searching beyond end of scope desired (beyond end of range)
char *String_Range_Find_Char (const char *s_start, const char *s_end, int ch_findchar)
{
	const char *search;
	for (search = s_start; *search && search <= s_end; search ++)
	{
		if (*search == ch_findchar)
			return (char *)search;
	}
	return NULL;
}


// Short: Copes a sub-string into a string ensuring null termination.
// Notes: If buffer size is 0, no operation occurs.
char *String_Range_Copy (char *s_dest, size_t siz, const char *src_start, const char *src_end)
{
	char *dst;
	const char *src;
	size_t copied;

	if (siz > 0)
	{
		for (dst = s_dest, src = src_start, copied = 0; *src && src <= src_end; )
		{
			if (copied >= siz)
				break;

			*dst++ = *src++;
			copied ++;
		}

		*dst = 0; // NULL at end.
	}
	return s_dest;
}

// Short: Copes a sub-string into a string ensuring null termination.
// Notes: If buffer size is 0, no operation occurs.
char *String_Length_Copy (char *s_dest, size_t siz, const char *src_start, size_t length)
{
	if (length > siz)
		length = siz;

	if (length > 0)
		String_Range_Copy (s_dest, siz, src_start, &src_start[length - 1]);

	return s_dest;
}

// Short: Finds position of string.
// Notes: Returns -1 on not found
int String_Pos_Find (const char *s, const char *s_find)
{
	const char *sfound = strstr (s, s_find);
	if (sfound)
		return sfound - s;
	return -1;
}

// Short: Finds position of string.
// Notes: Returns -1 on not found
int String_Pos_Find_Char (const char *s, int ch_findchar)
{
	const char *sfound = String_Find_Char (s, ch_findchar);
	if (sfound)
		return sfound - s;
	return -1;
}

// Short: Missing
// Notes: Advantage: strrchr could waste time searching beyond end of scope desired (beyond end of range)
char *String_Range_Find_Char_Reverse (const char *s_start, const char *s_end, int ch_findchar)
{
	const char *search;
	for (search = s_end; *search && search >= s_start; search --)
	{
		if (*search == ch_findchar)
			return (char *)search;
	}
	return NULL;
}


// Short: Returns character past first instance of char to find, or returns string if none found
// Notes: Offers no advantage over strchr, but functions differently as strchr returns NULL if nothing found
char *String_Skip_Char (const char *s, int ch_findchar)
{
	const char *search = strchr (s, ch_findchar);
	return (search ? (char *)&search[1] : (char *)s);
}

char *String_Skip_Char_Reverse (const char *s, int ch_findchar)
{
	const char *search = strrchr (s, ch_findchar);

	return (search ? (char *)&search[1] : (char *)s);
}



// Short: Returns a pointer beyond any leading white-space (char < SPACE_CHAR_32) in a string except for spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Skip_WhiteSpace_Excluding_Space (const char *s)
{
	for ( ; *s && *s < ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}


// Short: Returns a pointer beyond any leading white-space (char <= SPACE_CHAR_32) in a string including for spaces
// (char SPACE_CHAR_32).  Does not return NULL but rather would point to the end of a CString.
// Notes: None.
char *String_Skip_WhiteSpace_Including_Space (const char *s)
{
	for ( ; *s && *s <= ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}

char *String_Skip_NonWhiteSpace (const char *s)
{
	for ( ; *s && *s > ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}





char *String_Range_Dup_Alloc(const char *s_start, const char *s_end)
{
	int len = s_end - s_start + 1;

	return (len > 0) ? dpstrndup (s_start, len) : NULL;
}


char *String_Replace_Malloc (const char *s, const char *s_find, const char *s_replace)
{
	// Some day, preprocessor macros might be as reliable.  Or perhaps someday, headers will auto-generate and all functions should be explicitly defined.
	return String_Replace_Len_Count_Malloc (s, s_find, s_replace, NULL, NULL, NULL);
}

char *String_Replace_Len_Count_Malloc (const char *s, const char *s_find, const char *s_replace, replyx int *created_length, replyx size_t *created_bufsize, replyx int *replace_count)
{
	char *newstring_o;

	// CASE MATTERS!
	// Determine the number of instances of s_find
	int s_len = (int)strlen(s), find_len = (int)strlen(s_find), replace_len = (int)strlen(s_replace);
	int delta_instance_len = replace_len - find_len; // How much we increase per instance
	// Number of instances?
	// strcasestr
	int count = 0;
	const char *cursor;


	if (!find_len) {
//		logd ("Passed empty replacement string");

		NOT_MISSING_ASSIGN(replace_count, count);
		NOT_MISSING_ASSIGN(created_length, (int)strlen (s));
		NOT_MISSING_ASSIGN(created_bufsize, (int)strlen (s) + 1);

		return strdup(s);
	}

	for (cursor = s, count = 0; (cursor = strstr(cursor, s_find));  count ++)
		cursor += find_len; // Advance cursor

	{

	int delta_len = delta_instance_len * count;
	size_t bufsiz = s_len + delta_len + 1; // delta_len might be positive or negative
	size_t minbufsize = s_len + 1;
	size_t tempbufsize = bufsiz > minbufsize ? bufsiz : minbufsize; // Temp bufsize is greater of the 2

	newstring_o = (char *)malloc (tempbufsize);
	strlcpy (newstring_o, s, tempbufsize);

	// This is gonna get tested!
	String_Edit_Replace (newstring_o, tempbufsize, s_find, s_replace);

	if (tempbufsize > bufsiz) // Over allocation can now be corrected.
		newstring_o = (char *)realloc (newstring_o, bufsiz); // Shorten.  Or should we really bother?  Yes, let's bother.  What if source was a monster.

	NOT_MISSING_ASSIGN(replace_count, count);
	NOT_MISSING_ASSIGN(created_length, (int)(bufsiz - 1) );
	NOT_MISSING_ASSIGN(created_bufsize, bufsiz);

	}

	return newstring_o;
}

#define CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024 1024
#define CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32 32


// Short: sprintf to temp string (size of static temp string buffer is 1024) which does not need to be freed
// Notes: There are 32 static temp buffers which are cycled.  Best for short-lived vars in non-recursive functions.
char *va2 (const char *format, ...)
{
	static char 	buffers[CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32][CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024];
	static size_t 	sizeof_a_buffer 	= sizeof(buffers[0]);
	static size_t 	num_buffers			= sizeof(buffers) / sizeof(buffers[0]);
	static size_t 	cycle = 0;

	char			*buffer_to_use = buffers[cycle];
	va_list 		args;

	va_start 		(args, format);
	dpsnprintf 		(buffer_to_use, sizeof_a_buffer, format, args);
	va_end 			(args);

	// Cycle through to next buffer for next time function is called

	if (++cycle >= num_buffers)
		cycle = 0;

	return buffer_to_use;
}

//char *va2 (const char *format, ...) __core_attribute__((__format__(__printf__,1,2))) ;
#define _vas(STRING) va2("%s", STRING)								// Feb 12 2019 - Popular.  String direct to va.

char *String_Write_NiceFloatString (char *s_dest, size_t siz, double floatvalue)
{
	dpsnprintf (s_dest, siz, "%f", floatvalue);

	return String_Edit_RemoveTrailingZeros (s_dest); // Also removes period
}

char *String_Nice_Float_Vas (double floatvalue)
{
	static char s_double_buf[60];
	String_Write_NiceFloatString (s_double_buf, sizeof(s_double_buf), floatvalue);
	return _vas (s_double_buf);
}


// extras2

///////////////////////////////////////////////////////////////////////////////
//  FILE INFORMATION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////

int File_Is_Existing_File (const char *path_to_file)
{
	if (File_Exists(path_to_file) /*&& File_Is_Folder(path_to_file) == false*/)
		return true;
	return false;
}


#define FILE_READ_ALL_0		0
#define OFFSET_0			0



#define AUTO_FOPEN___			// Auto but file open
#define AUTO_FFREE___			// Auto but file close

void *File_To_Memory_Offset_Alloc (const char *path_to_file, replyx size_t *numbytes, size_t offset_into_file, size_t read_len_wanted)
{
	size_t filesize, read_beyond, read_len_to_try;
	AUTO_FOPEN___ FILE *f = NULL;
	//size_t File_Length2 (const char *path_to_file, required int *p_is_existing);
	int is_existing;

	// If we find the file size is 0, we get out  We must accept zero length - June 23 2020
	filesize = File_Length2 (path_to_file, &is_existing);
	if (filesize == 0 && is_existing == false) {
		return AUTO_FFREE___ NULL; // We never opened a file and this operation failed.
	}

	// If len = 0 is specified, we read as much as possible
	if (read_len_wanted == FILE_READ_ALL_0) {
		read_len_to_try = (filesize - offset_into_file ); // 1000 - 1000  or 1000 - 400 start = read len 600
	} else {
		read_len_to_try = read_len_wanted; // Which must be positive non-zero.
	}

	// Calc the read end, make sure file is long enough for read we want
	read_beyond = offset_into_file + read_len_to_try; // Like 0 + 1000
	if (filesize < read_beyond) return AUTO_FFREE___ NULL; // Apparently, if the read length is too long, we fail.  We don't clip.


	// Open file
//open_file:
	f = fopen (path_to_file, FS_MODE_READ_BINARY_RB); // FS_SysOpen .. no we take full url
	if (f == NULL) {
		return AUTO_FFREE___ NULL; // Failed to open
	}

	if (read_len_to_try == 0) { // This can be zero if automatic size and offset is "ok" like read at 1000 for a 1000 length file.
		// Now, while it may be stupid to have opened the file for 0 length, we are following same code pattern here.
		// If the caller cares that much, should be checking for 0 length and simply not asking us to read it.
		// Which doesn't make sense to begin with.


		// No point in doing the actual read, right?
		byte *membuf = (byte *)malloc (read_len_to_try + 1);
		size_t bytes_read = read_len_to_try; // Which is 0
		membuf[read_len_to_try] = 0; // NULL terminate the buffer.
		NOT_MISSING_ASSIGN(numbytes, bytes_read);
		AUTO_FFREE___ fclose (f);
		return membuf;
	}

	else
	{
		int ret = /*oof!*/ fseek (f, /*oof 32 bit on windows*/ (long)offset_into_file, SEEK_SET);
		byte *membuf = (byte *)malloc (read_len_to_try + 1);
		size_t bytes_read = fread (membuf, 1, read_len_to_try, f);
		int is_ok = ret != 0;
		is_ok = !!is_ok;

		// If numbytes read did not meet expectations say it
		if (bytes_read != read_len_to_try) {
#if 0 // NO DP
			DEBUG_ASSERT (0);
#ifdef _DEBUG
			logd ("bytesread != filesize: did not read entire file!"); // This is "very unexpected".  However, we do return bytes read.
#endif // _DEBUG
#endif
		}

		membuf[read_len_to_try] = 0;

		// Close file
		fclose (f);

		// If caller requested to know amount read, report it back
		NOT_MISSING_ASSIGN(numbytes, bytes_read);

		return membuf;
	}
}


int File_Memory_To_File (const char *path_to_file, const void *data, size_t numbytes) // Returns true if ok
{
	AUTO_FOPEN___ FILE *f = fopen (path_to_file, FS_MODE_WRITE_BINARY_WB);

	if ( f == NULL)										return false;
	if ( fwrite (data, 1, numbytes, f) != numbytes)		{ return AUTO_FFREE___ fclose (f); false; }

	AUTO_FFREE___ fclose (f);

	return true;
}

// Does not write trailing null.
int File_String_To_File (const char *path_to_file, const char *s)
{
	size_t numbytes = strlen(s);
	return File_Memory_To_File (path_to_file, s, numbytes);
}


void *File_To_Memory_Alloc (const char *path_to_file, replyx size_t *numbytes)
{
	return File_To_Memory_Offset_Alloc (path_to_file, numbytes, OFFSET_0, FILE_READ_ALL_0);

/* Obsolete but may be nice for reference because is simple
	FILE *f = core_fopen_read (path_to_file, FOPEN_READ_BINARY_RB);

	if (f)
	{
		size_t	filesize	= FileHandle_GetLength (f);
		byte	*membuf		= malloc (filesize);
		size_t	bytes_read	= fread (membuf, 1, filesize, f);

		if (bytes_read != filesize)
			logd ("bytesread != filesize: did not read entire file!");

		core_fclose (f);

		if (numbytes != NULL)  // A function might not request this be written
			*numbytes = bytes_read; //filesize; // Optimistic?  What if fread didn't read whole thing (rare I know )...

		return membuf;
	}

	return NULL;
*/
}


char *File_URL_Edit_SlashesForward_Like_Unix (char *windows_path_to_file)
{
	return String_Edit_Replace_Char (windows_path_to_file, '\\' /*find*/, '/' /*replace with*/, NULL /* don't want count */);
}


WARP_X_ (Con_Folder_f "zircon_command_line.txt")

SBUF___ const char *File_Getcwd_SBuf (void) // No trailing slash
{
	return Sys_Getcwd_SBuf ();
}


// Turns c:/mypath/something into c:\mypath\something
char *File_URL_Edit_SlashesBack_Like_Windows (char *unix_path_to_file)
{
	// Translate "/" to "\"
	return String_Edit_Replace_Char (unix_path_to_file, '/' /*find*/, '\\' /*replace with*/, NULL /* don't want count */);
}

// May 18 2020 - This does NOT leave a trailing "/".  It is also UNIX only.
char *File_URL_Edit_Reduce_To_Parent_Path_Trailing_Slash (char *path_to_file)
{
	char *terminate_point = strrchr (path_to_file, '/');

	if (terminate_point) {
		*terminate_point = '\0';
	} else {
		path_to_file[0] = '\0';
	}

	return path_to_file;
}



WARP_X_ (FS_FileWithoutPath)

char *File_URL_Remove_Trailing_Unix_Slash (char *path_to_file)
{
	if (String_Ends_With (path_to_file, "/")) {
		int slen = (int)strlen (path_to_file);
		path_to_file[slen - 1] = 0;
	}
	return path_to_file;;
}

const char *File_URL_SkipPath (const char *path_to_file)
{
	return String_Skip_Char_Reverse (path_to_file, '/');
}

WARP_X_ (FS_StripExtension)
char *File_URL_Edit_Remove_Extension (char *path_to_file)
{
	char *terminate_point = strrchr (path_to_file, '.');

	if (terminate_point)
		*terminate_point = '\0';

	return path_to_file;
}

// Doesn't return NULL rather emptystring, return the dot (".png", etc ..)
const char *File_URL_GetExtension (const char *path_to_file)
{
	const char *finddot = strrchr /*reverse*/ (path_to_file, '.');

	// Make sure we found it and that what we found wasn't a dot
	// in path
	if (finddot && strchr /*forward*/ (finddot, '/') == NULL )
	{
		return finddot;
	}

	return "";
}

// Returns extension with . (like .png) after last slash if exists, returns NULL if nothing found.
ccs *File_URL_GetExtensionEx (ccs *path_to_file)
{
	ccs *findsla = strrchr /*reverse*/ (path_to_file, '/');
	if (findsla)
		path_to_file = &findsla[1]; // After last slash
	ccs *finddot = strrchr /*reverse*/ (path_to_file, '.');
	
	if (finddot)
		return finddot;
	return NULL;
}

void File_URL_Edit_Default_Extension (char *path_to_file, const char *dot_new_extension, size_t bufsize)
{
	const char *extension = File_URL_GetExtension (path_to_file);

	if (extension[0] == 0)
	{
		// No extension so default it
		strlcat (path_to_file, dot_new_extension, bufsize);
	}
}



// May 18 2020 - This does NOT leave a trailing "/".  It is also UNIX only.
char *File_URL_Edit_Reduce_To_Parent_Path (char *path_to_file)
{
	char *terminate_point = strrchr (path_to_file, '/');

	if (terminate_point)
		*terminate_point = '\0';

	return path_to_file;
}


// general no have / like "c:/zircon/id1"
int Folder_Open (const char *path_to_file)
{
	// Check if folder?
	if (File_Exists (path_to_file) == false) {
		//logd ("Folder " QUOTED_S " does not exist to show", path_to_file);
		return false;
	}

	return Sys_Folder_Open_Folder_Must_Exist (path_to_file);
}

///////////////////////////////////////////////////////////////////////////////
//  IMAGE: OPERATIONS
///////////////////////////////////////////////////////////////////////////////

void Image_Flip_RedGreen (void *rgba, size_t numbytes)
{
	byte	*byte_rep = (byte *)rgba;
	byte	temp;

	// RGBA to BGRA so clipboard will take it
	size_t i; for (i = 0 ; i < numbytes ; i += RGBA_4)
	{
		temp = byte_rep[i + 0];

		byte_rep[i + 0] = byte_rep[i + 2];
		byte_rep[i + 2] = temp;
	}
}

void Image_Flip_Buffer (void *pels, int columns, int rows, int bytes_per_pixel)
{
					byte	*buffer		= (byte *)pels;
					int		bufsize		= columns * bytes_per_pixel; // bufsize = widthBytes;
	AUTO_ALLOC___	byte	*tb1		= (byte	*)malloc (bufsize); // Flip buffer
	AUTO_ALLOC___	byte	*tb2		= (byte	*)malloc (bufsize); // Flip buffer2

	int		i, offset1, offset2;

	for (i = 0; i < (rows + 1) / 2; i ++)
	{
		offset1 = i * bufsize;
		offset2 = ((rows - 1) - i) * bufsize;

		memcpy(tb1,				buffer + offset1, bufsize);
		memcpy(tb2,				buffer + offset2, bufsize);
		memcpy(buffer+offset1,	tb2,			  bufsize);
		memcpy(buffer+offset2,	tb1,			  bufsize);
	}

	AUTO_FREE___ free (tb1);
	AUTO_FREE___ free (tb2);
	return;
}



///////////////////////////////////////////////////////////////////////////////
//  CLIPBOARD: Baker - Clipboard
///////////////////////////////////////////////////////////////////////////////

WARP_X_ (CGL_Clipboard_Texture_Copy)

// copies given text to clipboard
int Clipboard_Set_Text (const char *text_to_clipboard)
{
	if (!text_to_clipboard)
		return false;
	int ret = Sys_Clipboard_Set_Text (text_to_clipboard);
	return ret;
}

#ifdef _DEBUG
	// copies given text to clipboard.  Text can't be NULL
	int _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
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
#endif // _DEBUG

///////////////////////////////////////////////////////////////////////////////
//  MATH: Baker - Math
///////////////////////////////////////////////////////////////////////////////

extern cvar_t v_flipped;


void Math_Unproject (vec_t *src3d, vec_t *dest3d)
{
	WARP_X_ (VM_CL_project)
	vec3_t temp;

	VectorSet(temp,
		src3d[2],
		(-1.0 + 2.0 * (src3d[0] / vid_conwidth.integer)) * src3d[2] * -r_refdef.view.frustum_x,
		(-1.0 + 2.0 * (src3d[1] / vid_conheight.integer)) * src3d[2] * -r_refdef.view.frustum_y);
	if(v_flipped.integer)
		temp[1] = -temp[1];
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, dest3d);
}

void Math_Project (vec_t *src3d, vec_t *dest2d)
{
	vec3_t v;
	matrix4x4_t m;

	Matrix4x4_Invert_Full(&m, &r_refdef.view.matrix);
	Matrix4x4_Transform(&m, src3d, v);
	if(v_flipped.integer)
		dest2d[1] = -dest2d[1];
	VectorSet(dest2d,
		vid_conwidth.integer * (0.5*(1.0+v[1]/v[0]/-r_refdef.view.frustum_x)),
		vid_conheight.integer * (0.5*(1.0+v[2]/v[0]/-r_refdef.view.frustum_y)),
		v[0]);
	// explanation:
	// after transforming, relative position to viewport (0..1) = 0.5 * (1 + v[2]/v[0]/-frustum_{x \or y})
	// as 2D drawing honors the viewport too, to get the same pixel, we simply multiply this by conwidth/height
}

///////////////////////////////////////////////////////////////////////////////
//  TIME: Baker
///////////////////////////////////////////////////////////////////////////////


int Time_Hours (int seconds)
{
	return seconds / 3600;
}

int Time_Minutes_Less_Hours (int seconds)
{
	int seconds2 = seconds % 3600;
	return seconds2 / 60;
}

int Time_Minutes (int seconds)
{
	return seconds / 60;
}

int Time_Seconds (int seconds)
{
	return seconds %60;
}

//#ifdef _WIN32
// Short: Command line to argv
// Notes: None.
// Unit Test:
#define MAX_ASCII_PRINTABLE_126		126 // TILDE
void String_Command_String_To_Argv (char *s_cmdline, int *numargc, char **argvz, int maxargs)
{
	// Baker: This converts a commandline in arguments and an argument count.
	// Requires cmd_line, pointer to argc, argv[], maxargs
	while (*s_cmdline && (*numargc < maxargs))
	{
#if 0
		const char *start = s_cmdline;
		int len;
#endif
		// Advance beyond spaces, white space, delete and non-ascii characters
		// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
		while (*s_cmdline && (*s_cmdline <= SPACE_CHAR_32 || MAX_ASCII_PRINTABLE_126 <= *s_cmdline ) ) // Was 127, but 127 is DELETE
			s_cmdline++;

		switch (*s_cmdline)
		{
		case 0:  // null terminator
			break;

		case '\"': // quote

			// advance past the quote
			s_cmdline++;

			argvz[*numargc] = s_cmdline;
			(*numargc)++;

			// continue until hit another quote or null terminator
			while (*s_cmdline && *s_cmdline != '\"')
				s_cmdline++;
#if 0
			len = s_cmdline - start;
#endif
			break;

		default:
			argvz[*numargc] = s_cmdline;
			(*numargc)++;

			// Advance until reach space, white space, delete or non-ascii
			while (*s_cmdline && (SPACE_CHAR_32 < *s_cmdline && *s_cmdline <= MAX_ASCII_PRINTABLE_126  ) ) // Was < 127
				s_cmdline++;
#if 0
			len = s_cmdline - start;
#endif
		} // End of switch

		// If more advance the cursor past what should be whitespace
		if (*s_cmdline)
		{
			*s_cmdline = 0;
			s_cmdline++;
		}

	} // end of while (*cmd_line && (*numargc < maxargs)
}

// Returns new length.
	/*
	\e	Write an <escape> character.
	\a	Write a <bell> character.
	\b	Write a <backspace> character.
	\f	Write a <form-feed> character.
	\n	Write a <new-line> character.
	\r	Write a <carriage return> character.
	\t	Write a <tab> character.
	\v	Write a <vertical tab> character.
	\'	Write a <single quote> character.
	\\	Write a backslash character.
	*/

// Strips newlines, carriage returns and backspaces.
void String_Edit_To_Single_Line (char *s_edit)
{
	int length = strlen(s_edit);

	const char *temp_a = strdup(s_edit);
	int remaining = length;

	const char *src = temp_a;
	char *dst = s_edit;			// Yes we rewrite

	// Truncate at any new line or carriage return or backspace character
	// BUT convert any whitespace characters that are not actual spaces into spaces.
	//while (*src && dst - cliptext < sizeof out - 1 && *src != '\n' && *src != '\r' && *src != '\b')
	while (*src && remaining > 0 && *src != '\n' && *src != '\r' && *src != '\b')
	{
		if (*src < ' ')
			*dst++ = ' ';
		else *dst++ = *src;
		src++;
		remaining --;
	}
	*dst = 0;
}

char *Clipboard_Get_Text_Line_Static (void)
{
	static char out[SYSTEM_STRING_SIZE_1024];
	char *cliptext_a = Sys_GetClipboardData_Alloc();

	out[0] = 0; // In case cliptext_a is NULL
	if (cliptext_a) {
		c_strlcpy (out, cliptext_a);
		Z_Free(cliptext_a); cliptext_a = NULL;
		String_Edit_To_Single_Line (out); // spaces < 32 except for newline, cr, backspace which it kills.
	}

	return out;
}

// Weaknesses?  Buf size.  Can't handle double.  Can't handle atoi64.  15 digit limit
// 2 000 000 000 (10) 2,000,000,000 13  minus sign 14.
// Strengths?  Think it can handle negative ok.
// Returns strlen
// Stack overflow says needs something like 1077 digits for super small fractional double.
// http://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
// But we are mostly talking

size_t str_format_int_grouped(char dst[16], int64_t num64)
{
    char src[16];
    char *p_src = src;
    char *p_dst = dst;

    const char separator = ',';
    int num_len, commas;

    num_len = dpsnprintf (src, sizeof(src), PRINTF_INT64, num64);

    if (*p_src == '-') {
        *p_dst++ = *p_src++;
        num_len--;
    }

#define DIGIT_GROUP_3 3// Thousands
    for (commas = 2 - num_len % DIGIT_GROUP_3; *p_src; commas = (commas + 1) % DIGIT_GROUP_3) {
        *p_dst++ = *p_src++;
        if (commas == 1) {
            *p_dst++ = separator;
        }
    }
    *--p_dst = '\0';

    return (size_t)(p_dst - dst);
}

// Baker: This bastard can only do integer at this time
char *String_Num_To_Thousands_Sbuf (int64_t num)
{
	static char sbuf[128];
	str_format_int_grouped(sbuf, num);

	return sbuf;
}

char *String_Worldspawn_Value_For_Key_Sbuf (const char *s_entities_string, const char *find_keyname)
{
	static char		valuestring[4096];
	char			current_key[128];
	const char		*data = s_entities_string;
	const char		*copy_start;

	// Read some data ...
	if (COM_Parse_Basic(&data) == false || com_token[0] != '{')	// Opening brace is start of worldspawn
		return NULL; // error

	while (1) {
		// Read some data ...
		if (COM_Parse_Basic(&data) == false)
			return NULL; // End of data

		if (com_token[0] == '}')	// Closing brace is end of worldspawn
			return NULL; // End of worldspawn

		// Copy data over, skipping a prefix of '_' in a keyname
		copy_start = &com_token[0];

		if (*copy_start == '_')
			copy_start ++;

		c_strlcpy (current_key, copy_start);

		String_Edit_RemoveTrailingSpaces (current_key);

		if (COM_Parse_Basic(&data) == false)
			return NULL; // error

		if (String_Match_Caseless (find_keyname, current_key)) {
			c_strlcpy (valuestring, com_token);
			return valuestring;
		}

	}

	return NULL;
}

// returns number of keys printed
int String_Worldspawn_Value_For_Key_Con_PrintLine (const char *s_entities_string)
{

	char			current_key[128];
	char			current_value[4096];
	const char		*data = s_entities_string;
	const char		*copy_start;
	int numkeysprinted = 0;

	// Read some data ...
	if (COM_Parse_Basic(&data) == false || com_token[0] != '{')	// Opening brace is start of worldspawn
		return -1; // error

	while (1) {
		// Read some data ...
		if (COM_Parse_Basic(&data) == false)
			return -1; // End of data

		if (com_token[0] == '}')	// Closing brace is end of worldspawn
			return numkeysprinted; // End of worldspawn

		// Copy data over, skipping a prefix of '_' in a keyname
		copy_start = &com_token[0];

		if (*copy_start == '_')
			copy_start ++;

		c_strlcpy (current_key, copy_start);

		String_Edit_RemoveTrailingSpaces (current_key);

		if (COM_Parse_Basic(&data) == false)
			return numkeysprinted; // error

		extern int con_linewidth;

		c_strlcpy (current_value, com_token);
		if ((int)strlen (current_value) > con_linewidth - 23)
			Con_PrintLinef (S_FMT_LEFT_PAD_20 NEWLINE NEWLINE QUOTED_S NEWLINE, current_key, current_value);
		else Con_PrintLinef (S_FMT_LEFT_PAD_20 " " QUOTED_S, current_key, current_value);
		numkeysprinted ++;
	} // while 1

	// Baker: We should have exited earlier all the time due to finding closing "}"
	return numkeysprinted;
}

void String_Worldspawn_Values_stringlistappend (stringlist_t *plist, ccs *s_entities_string)
{
	char			current_key[128];
	char			current_value[4096];
	const char		*data = s_entities_string;
	const char		*copy_start;
	int numkeysprinted = 0;

	// Read some data ...
	if (COM_Parse_Basic(&data) == false || com_token[0] != '{')	// Opening brace is start of worldspawn
		return; // error

	while (1) {
		// Read some data ...
		if (COM_Parse_Basic(&data) == false) {
			stringlistappend (plist, "IRREGULAR EXIT");
			stringlistappend (plist, "[Early end of data]");
			return;// -1; // End of data
		}

		if (com_token[0] == '}')	// Closing brace is end of worldspawn
			return;// numkeysprinted; // End of worldspawn

		// Copy data over, skipping a prefix of '_' in a keyname
		copy_start = &com_token[0];

		if (*copy_start == '_')
			copy_start ++;

		c_strlcpy (current_key, copy_start);

		String_Edit_RemoveTrailingSpaces (current_key);

		if (COM_Parse_Basic(&data) == false) {
			stringlistappend (plist, "IRREGULAR EXIT");
			stringlistappendf (plist, "[Unable to get value for key %s]", current_key);
			return;// numkeysprinted; // error
		}

		c_strlcpy (current_value, com_token);
		stringlistappend (plist, current_key);
		stringlistappend (plist, current_value);
		//numkeysprinted ++;
	} // while 1

	// Baker: We should have exited earlier all the time due to finding closing "}"
	//return numkeysprinted;
}

qbool String_Is_All_AlphaNumeric_Underscore(ccs *s)
{
	
	for (ccs *p = s; *p; *p++) {
		int ch = *p;
		if (isalpha(ch))	continue;
		if (isdigit(ch))	continue;
		if (isin1(ch, '_'))	continue;
		return false;
	}
	return true;
}
