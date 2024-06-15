// keys_undo.c.h

/*
  things about undo
	  === depth, size, textsizemax, buffer pointer

	buffer entry
	- text
	- cursor
	- selection length
	- action (for compacting)

*/



//#define DEBUG_UNDO

//typedef enum {
//	uaction_del_neg1		= -1,
//	uaction_not_typing_0	= 0,	// ?
//	uaction_typing_1		= 1,
//} uaction_e;

typedef struct {
	char				text[MAX_INPUTLINE_16384];
	int					cursor;
	int					cursor_length;
	int /*uaction_e*/	action;						// -1 delete, 0 not typing, 1 typing
	int 				was_space;					// Was the char deleted or typed a space?
} undo_entry_s;

typedef struct {
	int					level;			// Depth into undo buffer.  0 to start, which should be treated like -1.
	int					count;			// Number of undos in buffer
	undo_entry_s		*undo_entries;	// We will grow this and move things around (realloc, memmove	
} undo_master_s;


undo_master_s mundo_buffer;

//void Undo_Set_Point (undo_master_s *u, const char *text, int cursor, int cursor_length, int action, cbool was_space);
//void Undo_Clear (undo_master_s *u);
//void Undo_Dump (undo_master_s *u);
//const char *Undo_Walk (undo_master_s *u, int change, char *text, int *cursor, int *cursor_length);

void Undo_Dump (undo_master_s *u)
{
	int n;
	Con_PrintLinef ("Undo ... Count = %d Undo Level is %d", u->count, u->level);
	
	for (n = 0; n < u->count; n ++) {
//		if (!u->undo_entries[n].text)
//			u=u;
		Con_PrintLinef ("%04d:%s " QUOTED_S, n, (n + 1 == u->level)  ? ">" : " ", u->undo_entries[n].text);
	}
	
}

// Clear all the entries, then the buffer
void Undo_Clear (undo_master_s *u)
{
	if (u->undo_entries) {
		freenull_ (u->undo_entries);
	}
	u->level = 0;
	u->count = 0;

#ifdef DEBUG_UNDO
	Con_PrintLinef ("Undo Clear Action");
	Undo_Dump (u);
#endif
}

void Undo_Remove_From_Top_ (undo_master_s *u, int num_deletes)
{
	int new_top = num_deletes;
	int new_size = u->count - num_deletes;

	if (new_size == 0) {
		freenull_ (u->undo_entries); // Wipe
	} else  {
		//int bytes = sizeof(struct undo_entry_s) * move_size;
		memmove (&u->undo_entries[0], &u->undo_entries[new_top], sizeof(undo_entry_s) * new_size);
		u->undo_entries = (undo_entry_s *)realloc (u->undo_entries, sizeof(undo_entry_s) * new_size);
	}

	u->count -= num_deletes;
}

// Concats similar actions into one undo point
void Undo_Compact_ (undo_master_s *u)
{
	int streak_count;
	int cur_action;
	int n;

#ifdef DEBUG_UNDO
	Con_PrintLinef ("Before compact");
	Undo_Dump (u);
#endif
	
	for (n = 1, streak_count = 0, cur_action = 0; n < u->count; n ++, streak_count ++) {
		undo_entry_s *e = &u->undo_entries[n];
		
		// If non-typing and non-deleting event, get out!
		if (!e->action)
			break;
		
		// If change in action, get out!
		if (cur_action) {
			if (e->action != cur_action)
				break; // Change in action
		}
		else cur_action = e->action; // This gets set once if non-zero.
	}

	if (streak_count > 1) // Only have a streak if you have 2.  No such thing as streak of 1.
	{
		int num_deletes = streak_count - 1;
		int new_size = u->count - num_deletes;
		int streak_end = streak_count; 
		int move_size = new_size - 1;
		
		memmove (&u->undo_entries[1], &u->undo_entries[streak_end], sizeof(undo_entry_s) * move_size);
		
		u->count -= num_deletes;
		u->undo_entries = (undo_entry_s *)realloc (u->undo_entries, sizeof(undo_entry_s) * u->count);
	
		u->undo_entries[1].action = 0; // Solidify
	}
}

void Undo_Set_Point (undo_master_s *u, const char *text, int cursor, int cursor_length, int action, int was_space)
{
	//size_t check_size = sizeof(undo_entry_s);

	// If into undo buffer ( level 1 is redo) then remove everything above.
	if (u->level > 0) { // Size must be at least 2 if this is true (courtesy redo takes 1 space)
		Undo_Remove_From_Top_ (u, u->level - 1);
		u->level = 0;
	}

	// Don't add identical entries
	if (u->level == 0 && u->count) {
		undo_entry_s *e = &u->undo_entries[0];

		// Text didn't change, dup so get out ...
		if (String_Match (e->text, text)) {
			e->cursor = cursor;
			e->cursor_length = cursor_length;

			// But solidify the match if a hard action happened.
			if (action == 0 && e->action != 0)
				e->action = 0;
			return;
		}
	}

	// ADDING.  INCREASE COUNT.
	u->count ++;

	// If ptr is a null pointer, the realloc function behaves like the malloc function for the specified size.
	u->undo_entries = (undo_entry_s *)realloc (u->undo_entries, sizeof(undo_entry_s) * u->count);
	
//	if (!u->undo_entries)
//		System_Error ("Failed undo alloc on %d bytes", sizeof(struct undo_entry_s) * u->count);

	// If we have undo entries, move everything down one.
	if (u->undo_entries)
		memmove (&u->undo_entries[1], &u->undo_entries[0], sizeof(undo_entry_s) * (u->count - 1) );

	// fill in the new guy
	{
		undo_entry_s *e = &u->undo_entries[0];
		memset (e, 0, sizeof(*e)); // Baker Feb 7 2017
		c_strlcpy (e->text, text);

		e->cursor = cursor;
		e->cursor_length = cursor_length;
		e->action = action;
		e->was_space = was_space;
	}

	// A space trigger compacting consecutive "typed character" undos
	// Technically this block is only for undo_size > 1, but should pass through fine if 1.

	
	if (action && was_space)
		Undo_Compact_ (u);

}



// Undo
// Redo
// If level is -1 and change > 0 (undo at top level) do we add snapshot now?  Probably.
const char *Undo_Walk (undo_master_s *u, int change, char *text, int *cursor, int *cursor_length)
{
	if (!u->count)
		return NULL; //  No undos or redos

	if (change < 0 && u->level <= 1) return NULL; // Can't redo beyond current or we aren't walking undos
	if (change > 0 && u->level == u->count) return NULL; // maximum depth
	
	// We aren't walking undos, create a redo for the top
	if (change > 0 && u->level == 0) {
		// Make the redo slot.
		Undo_Set_Point (u, text, *cursor, *cursor_length, 0, 0);
		u->level = 2;
	}
	else u->level += change;

	if (in_range (0, u->level, u->count) == false) {
		//int j = 5;
		// We missed an event in keys.c, rather than crash
		// Just do something
		u->level = 0; //u->count - 1;
		{
			undo_entry_s *e = &u->undo_entries[u->level];
			return e->text;
		}
	}

	// Copy info
	{
		undo_entry_s *e = &u->undo_entries[u->level - 1];
		//strlcpy (*text, e->text, s_size);
		*cursor = e->cursor;
		*cursor_length = e->cursor_length;

		if (in_range_beyond(0, (*cursor), 16384) == false) {
			//int j = 5;
			// We missed an event in keys.c, rather than crash
			// Just do something
			u->level = 0; //u->count - 1;
			{
				undo_entry_s *e2 = &u->undo_entries[u->level];
				*cursor = e2->cursor;
				*cursor_length = e2->cursor_length;

				return e2->text;
			}

		}
		// Don't do cursor length?
		return e->text;
	}

}

// If we were in the undo buffer, we aren't now.
// Should happen any time a material change is made to text.
void Con_Undo_Point (int action, int was_space)
{
	Undo_Set_Point (&mundo_buffer, key_line, key_linepos, key_sellength, action, was_space);
}

int Con_Undo_Walk (int direction)
{
	const char *new_text = Undo_Walk (&mundo_buffer, direction, key_line, &key_linepos, &key_sellength);

//	Undo_Dump (&mundo_buffer);
	
	if (!new_text) // No new text
		return false;

	c_strlcpy (key_line, new_text);
	return true;
}

// If the cursor moves, set action to 0 for most recent entry
void Con_Undo_CursorMove (void)
{
	// What if into undo buffer?  I don't think it matters because top entry is redo which just gets nuked.
	if (mundo_buffer.count && mundo_buffer.undo_entries[0].action)
		mundo_buffer.undo_entries[0].action = 0;
}


// This should happen about every time that key_linepos is set to 1 or the line is cleared or history walked.
void Con_Undo_Clear (void)
{
	Undo_Clear (&mundo_buffer);
}
