/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2021 DarkPlaces contributors

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
// cvar.c -- dynamic variable tracking


#include "quakedef.h"

const char *cvar_dummy_description = "custom cvar";
static const char *cvar_null_string = "";

cvar_state_t cvars_all;
cvar_state_t cvars_null;

void Cvar_Audit_f(cmd_state_t *cmd)
{
	stringlist_t cvars_list = {0};
	int cvar_count = 0;
	for (cvar_t *v = cvars_all.vars; v; v = v->next) {
		stringlistappend (&cvars_list, v->name);
		float fval = atof(v->string);
		qbool value_match = fval == v->value;
		int ival = atoi(v->string);
		qbool integer_match = ival == v->integer;
		if (value_match == false)
			Con_PrintLinef (QUOTED_S " failed value match, string is " QUOTED_S " val is %f", v->name, v->string, v->value);
		if (integer_match == false)
			Con_PrintLinef (QUOTED_S " failed integer match, string is " QUOTED_S " integer is %d", v->name, v->string, v->integer);
		if (Have_Flag (v->flags, CF_ALLOCATED) == false) {
			int default_match = String_Match (v->defstring, v->engine_string);
			if (default_match == false) 
				Con_PrintLinef (QUOTED_S " failed default match, string is " QUOTED_S " engine string is %s", v->name, v->defstring, v->engine_string);
		}
		cvar_count ++;
	} //

	stringlistsort (&cvars_list, fs_make_unique_true);

	stringlist_t hash_list = {0};
	int hash_count = 0;
	for (int hashindex = 0; hashindex < CVAR_HASHSIZE_65536; hashindex ++) {
		for (cvar_hash_t *hash = cvars_all.hashtable[hashindex]; hash; hash = hash->next) {
			stringlistappend (&hash_list, hash->cvar->name);
			hash_count ++;
		} // 
	} // n

	stringlistsort (&hash_list, fs_make_unique_true);

	Con_PrintLinef ("Hash count = %d", hash_count);
	Con_PrintLinef ("Cvar count = %d", cvar_count);
	Con_PrintLinef ("Is ok ? %d", hash_count == cvar_count);
	Con_PrintLinef ("Hash count = %d", cvars_list.numstrings);
	Con_PrintLinef ("Cvar count = %d", hash_list.numstrings);
	stringlistfreecontents (&cvars_list);
	stringlistfreecontents (&hash_list);
}


/*
============
Cvar_FindVar
============
*/
// Baker: "neededflags" is more than that.  give it ALL_FLAGS_ANTIZERO
// if anything goes, otherwise nothing will be returned.
cvar_t *Cvar_FindVar(cvar_state_t *cvars, const char *var_name, int neededflags)
{
	int hashindex;
	cvar_hash_t *hash;

	// use hash lookup to minimize search time
	hashindex = CRC_Block((const unsigned char *)var_name, strlen(var_name)) % CVAR_HASHSIZE_65536;
	for (hash = cvars->hashtable[hashindex]; hash; hash = hash->next) {
		if (String_Match (var_name, hash->cvar->name) && (hash->cvar->flags & neededflags))
			return hash->cvar;
		else
			for (char **alias = hash->cvar->aliases; alias && *alias; alias++) {
				if (String_Match (var_name, *alias) && (hash->cvar->flags & neededflags))
					return hash->cvar;
			} // for
	} // for
	return NULL;
}

cvar_t *Cvar_FindVarAfter(cvar_state_t *cvars, const char *prev_var_name, int neededflags)
{
	cvar_t *var;

	if (*prev_var_name)
	{
		var = Cvar_FindVar(cvars, prev_var_name, neededflags);
		if (!var)
			return NULL;
		var = var->next;
	}
	else
		var = cvars->vars;

	// search for the next cvar matching the needed flags
	while (var)
	{
		if (var->flags & neededflags)
			break;
		var = var->next;
	}
	return var;
}

static cvar_t *Cvar_FindVarLink(cvar_state_t *cvars, const char *var_name, cvar_t **parent, cvar_t ***link, cvar_t **prev_alpha, int neededflags)
{
	int hashindex;
	cvar_hash_t *hash;

	// use hash lookup to minimize search time
	hashindex = CRC_Block((const unsigned char *)var_name, strlen(var_name)) % CVAR_HASHSIZE_65536;
	if (parent) *parent = NULL;
	if (prev_alpha) *prev_alpha = NULL;
	if (link) *link = &cvars->hashtable[hashindex]->cvar;
	for (hash = cvars->hashtable[hashindex]; hash; hash = hash->next) {
		if (String_Match (var_name, hash->cvar->name) && (hash->cvar->flags & neededflags))
			goto match;
		else
			for (char **alias = hash->cvar->aliases; alias && *alias; alias++)
				if (String_Match (var_name, *alias) && (hash->cvar->flags & neededflags))
					goto match;
		if (parent) *parent = hash->cvar;
	}
	return NULL;
match:
	if (!prev_alpha || hash->cvar == cvars->vars)
		return hash->cvar;

	*prev_alpha = cvars->vars;
	// if prev_alpha happens to become NULL then there has been some inconsistency elsewhere
	// already - should I still insert '*prev_alpha &&' in the loop?
	while ((*prev_alpha)->next != hash->cvar)
		*prev_alpha = (*prev_alpha)->next;
	return hash->cvar;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValueOr(cvar_state_t *cvars, const char *var_name, float def, int neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return def;
	return atof (var->string);
}

float Cvar_VariableValue(cvar_state_t *cvars, const char *var_name, int neededflags)
{
	return Cvar_VariableValueOr(cvars, var_name, 0, neededflags);
}

/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableStringOr(cvar_state_t *cvars, const char *var_name, const char *def, int neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return def;
	return var->string;
}

const char *Cvar_VariableString(cvar_state_t *cvars, const char *var_name, int neededflags)
{
	return Cvar_VariableStringOr(cvars, var_name, cvar_null_string, neededflags);
}

/*
============
Cvar_VariableDefString
============
*/
const char *Cvar_VariableDefString(cvar_state_t *cvars, const char *var_name, int neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return cvar_null_string;
	return var->defstring;
}

/*
============
Cvar_VariableDescription
============
*/
const char *Cvar_VariableDescription(cvar_state_t *cvars, const char *var_name, int neededflags)
{
	cvar_t *var;

	var = Cvar_FindVar(cvars, var_name, neededflags);
	if (!var)
		return cvar_null_string;
	return var->description;
}


/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable(cvar_state_t *cvars, const char *partial, int neededflags)
{
	cvar_t		*cvar;
	size_t		len;

	len = strlen(partial);

	if (!len)
		return NULL;

// check functions
	for (cvar=cvars->vars ; cvar ; cvar=cvar->next)
		if (!strncasecmp (partial,cvar->name, len) && (cvar->flags & neededflags))
			return cvar->name;

	return NULL;
}


/*
	CVar_CompleteCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com

*/
int Cvar_CompleteCountPossible(cvar_state_t *cvars, const char *partial, int neededflags, int is_from_nothing)
{
	cvar_t	*cvar;
	size_t	len;
	int		h;

	h = 0;
	len = strlen(partial);

	if (!len && !is_from_nothing)
		return	0;

	// Loop through the cvars and count all possible matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next) {
		if (String_Starts_With_Caseless (cvar->name, partial) && (cvar->flags & neededflags)) {
			const char *sxy = cvar->name;
			SPARTIAL_EVAL_
			h++;
		}
		else {
			for (char **alias = cvar->aliases; alias && *alias; alias++) {
				if (String_Starts_With_Caseless(*alias, partial) && (cvar->flags & neededflags)) {
					const char *sxy = *alias; //cvar->name;
					SPARTIAL_EVAL_
					h++;
				}
			} // for alias of this cvar
		} // if
	} // for cvar

	return h;
}

/*
	CVar_CompleteBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
const char **Cvar_CompleteBuildList(cvar_state_t *cvars, const char *partial, int neededflags, int is_from_nothing)
{
	const cvar_t *cvar;
	size_t len = 0;
	size_t bpos = 0;
	size_t sizeofbuf = (Cvar_CompleteCountPossible(cvars, partial, neededflags, is_from_nothing) + 1) * sizeof(const char *);
	const char **buf;

	len = strlen(partial);
	buf = (const char **)Mem_Alloc(tempmempool, sizeofbuf + sizeof(const char *));
	// Loop through the alias list and print all matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
		if (!strncasecmp(partial, cvar->name, len) && (cvar->flags & neededflags))
			buf[bpos++] = cvar->name;
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strncasecmp(partial, *alias, len) && (cvar->flags & neededflags))
					buf[bpos++] = *alias;


	buf[bpos] = NULL;
	return buf;
}

// Baker: October 30 2024 - We get a crash with demo1 trying to autocomplete "mod"
// But DarkPlaces Beta does not?

// Zircon Beta is loading the map first. NO.
void Cvar_PrintHelp(cvar_t *cvar, const char *name, qbool full)
{
// BAKER: Probe for autocvar_ that is a vector
// if so and it has color ranges, color code it.
	char *s_color_escape_za = NULL;

	for (int prog_idx = 0; prog_idx <  PRVM_PROG_MAX_3; prog_idx ++) {
		prvm_prog_t *prog = &prvm_prog_list[prog_idx];

#if 1 // Baker: October 30 2024 - Bug fix for autocvar vector color detection that crashed autocomplete for demo1.dem
		if (!prog->loaded)
			continue; // Not loaded
#endif

		if (cvar->globaldefindex[prog_idx] < 0)
			continue;
		
		// autocvar_
		int glob_idx = cvar->globaldefindex[prog_idx];
		
		if ((prog->globaldefs[glob_idx].type & ~DEF_SAVEGLOBAL) != ev_vector)
			break; // Found and not a vector

		vec_t *v = PRVM_GLOBALFIELDVECTOR(prog->globaldefs[cvar->globaldefindex[prog_idx]].ofs);
		int is_empty = !v[0] && !v[1] && !v[2];
		if (is_empty)
			break; // No values at all

		s_color_escape_za = Color_Code_ZAlloc_Or_Null (v);
		
		// if s_color_escape_za == NULL, not a color otherwise it is.
		break;	
	} // for

// END BAKER MOD

	if (String_NOT_Match (cvar->name, name))
		Con_Printf (CON_BRONZE "%s" CON_WHITE " (alias of " CON_BRONZE "%s" 
		CON_WHITE ")", name, cvar->name); // Baker: purple to bronze.
	else
		Con_Printf (CON_BRONZE "%s" CON_WHITE, name);

	if (s_color_escape_za) {
		Con_Printf (" is " QUOTED_S CON_WHITE " (%sCOLOR" CON_WHITE ") [" QUOTED_S CON_WHITE "]", 
			((cvar->flags & CF_PRIVATE) ? "********"/*hunter2*/ : cvar->string), 
			s_color_escape_za,
			cvar->defstring);
		
		Mem_FreeNull_ (s_color_escape_za);
	} else {
		Con_Printf (" is " QUOTED_S CON_WHITE " [" QUOTED_S CON_WHITE "]", 
			((cvar->flags & CF_PRIVATE) ? "********"/*hunter2*/ : cvar->string), cvar->defstring);

	}
	if (full)
		Con_Printf (" %s", cvar->description);
	Con_Print("\n");
}

// written by LadyHavoc
void Cvar_CompleteCvarPrint(cvar_state_t *cvars, const char *partial, int neededflags, int is_from_nothing)
{
	cvar_t *cvar;
	size_t len = strlen(partial);
	// Loop through the command list and print all matches
	for (cvar = cvars->vars; cvar; cvar = cvar->next)
		if (!strncasecmp(partial, cvar->name, len) && (cvar->flags & neededflags))
			Cvar_PrintHelp(cvar, cvar->name, /*full?*/ true);
		else
			for (char **alias = cvar->aliases; alias && *alias; alias++)
				if (!strncasecmp (partial, *alias, len) && (cvar->flags & neededflags))
					Cvar_PrintHelp(cvar, *alias, /*full?*/ true);


}

// check if a cvar is held by some progs
// Baker: What if prog is unloaded?
WARP_X_CALLERS_ (Cvar_RestoreInitState)
// Baker: Used exclusively to try to decide if we are able to destroy a cvar
static qbool Cvar_IsAutoCvar(cvar_t *var)
{
	int i;
	prvm_prog_t *prog;
	for (i = 0; i < PRVM_PROG_MAX_3; i ++) {
		prog = &prvm_prog_list[i];
		if (prog->loaded && var->globaldefindex[i] >= 0)
			return true;
	}
	return false;
}

// we assume that prog is already set to the target progs
static void Cvar_UpdateAutoCvar(cvar_t *var)
{
	int i;
	int j;
	const char *s;
	vec3_t v;
	prvm_prog_t *prog;
	for (i = 0;i < PRVM_PROG_MAX_3;i++)
	{
		prog = &prvm_prog_list[i];
		if (prog->loaded && var->globaldefindex[i] >= 0)
		{
			// MUST BE SYNCED WITH prvm_edict.c PRVM_LoadProgs
			switch(prog->globaldefs[var->globaldefindex[i]].type & ~DEF_SAVEGLOBAL)
			{
			case ev_float:
				PRVM_GLOBALFIELDFLOAT(prog->globaldefs[var->globaldefindex[i]].ofs) = var->value;
				break;
			case ev_vector:
				s = var->string;
				VectorClear(v);
				for (j = 0;j < 3;j++)
				{
					while (*s && ISWHITESPACE(*s))
						s++;
					if (!*s)
						break;
					v[j] = atof(s);
					while (!ISWHITESPACE(*s))
						s++;
					if (!*s)
						break;
				}
				VectorCopy(v, PRVM_GLOBALFIELDVECTOR(prog->globaldefs[var->globaldefindex[i]].ofs));
				break;
			case ev_string:
				PRVM_ChangeEngineString(prog, var->globaldefindex_stringno[i], var->string);
				PRVM_GLOBALFIELDSTRING(prog->globaldefs[var->globaldefindex[i]].ofs) = var->globaldefindex_stringno[i];
				break;
			}
		}
	}
}

// called after loading a savegame
void Cvar_UpdateAllAutoCvars(cvar_state_t *cvars)
{
	cvar_t *var;
	for (var = cvars->vars ; var ; var = var->next)
		Cvar_UpdateAutoCvar(var);
}

void Cvar_Callback(cvar_t *var)
{
	if (var == NULL)
	{
		Con_Print("Cvar_Callback: var == NULL\n");
		return;
	}

	if (var->callback)
		var->callback(var);
}

/*
============
Cvar_Set
============
*/
extern cvar_t sv_disablenotify;
static void Cvar_SetQuick_Internal (cvar_t *var, const char *value)
{
	qbool changed;
	size_t valuelen;

	changed = strcmp(var->string, value) != 0;
	// LadyHavoc: don't reallocate when there is no change
	if (!changed)
		return;

	// LadyHavoc: don't reallocate when the buffer is the same size
	valuelen = strlen(value);
	if (!var->string || strlen(var->string) != valuelen)
	{
		Z_Free ((char *)var->string);	// free the old value string

		var->string = (char *)Z_Malloc (valuelen + 1);
	}
	memcpy ((char *)var->string, value, valuelen + 1);
	var->value = atof (var->string);
	var->integer = (int) var->value;
	if ((var->flags & CF_NOTIFY) && sv.active && !sv_disablenotify.integer)
		SV_BroadcastPrintf("\003^3Server cvar " QUOTED_S " changed to " QUOTED_S NEWLINE, var->name, var->string);
#if 0
	// TODO: add infostring support to the server?
	if ((var->flags & CF_SERVERINFO) && changed && sv.active)
	{
		InfoString_SetValue(svs.serverinfo, sizeof(svs.serverinfo), var->name, var->string);
		if (sv.active)
		{
			MSG_WriteByte (&sv.reliable_datagram, svc_serverinfostring);
			MSG_WriteString (&sv.reliable_datagram, var->name);
			MSG_WriteString (&sv.reliable_datagram, var->string);
		}
	}
#endif
	if (var->flags & CF_USERINFO)
		CL_SetInfo(var->name, var->string, true, false, false, false);

	Cvar_UpdateAutoCvar(var);

	// Call the function stored in the cvar for bounds checking, cleanup, etc
	Cvar_Callback(var);
}

void Cvar_SetQuick (cvar_t *var, const char *value)
{
	if (var == NULL)
	{
		Con_PrintLinef ("Cvar_SetQuick: var == NULL");
		return;
	}

	if (!(var->flags & CF_REGISTERED) && !(var->flags & CF_ALLOCATED))
	{
		Con_PrintLinef (CON_WARN "Warning: Cvar_SetQuick() cannot set unregistered cvar " QUOTED_S, var->name);
		return; // setting an unregistered engine cvar crashes
	}

	if (developer_extra.integer)
		Con_DPrintLinef ("Cvar_SetQuick({" QUOTED_S ", " QUOTED_S ", %d, " QUOTED_S "}, " QUOTED_S ");", var->name, var->string, var->flags, var->defstring, value);

	Cvar_SetQuick_Internal(var, value);
}

void Cvar_Set(cvar_state_t *cvars, const char *var_name, const char *value)
{
	cvar_t *var;
	var = Cvar_FindVar(cvars, var_name, ALL_FLAGS_ANTIZERO);
	if (var == NULL)
	{
		Con_PrintLinef ("Cvar_Set: variable %s not found", var_name);
		return;
	}
	Cvar_SetQuick(var, value);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValueQuick(cvar_t *var, float value)
{
	char val[MAX_INPUTLINE_16384];

	if ((float)((int)value) == value)
		dpsnprintf(val, sizeof(val), "%d", (int)value);
	else
		dpsnprintf(val, sizeof(val), "%f", value);
	Cvar_SetQuick(var, val);
}

void Cvar_SetValue(cvar_state_t *cvars, const char *var_name, float value)
{
	char val[MAX_INPUTLINE_16384];

	if ((float)((int)value) == value)
		dpsnprintf(val, sizeof(val), "%d", (int)value);
	else
		dpsnprintf(val, sizeof(val), "%f", value);
	Cvar_Set(cvars, var_name, val);
}

void Cvar_RegisterCallback(cvar_t *variable, void (*callback)(cvar_t *))
{
	if (variable == NULL)
	{
		Con_Print("Cvar_RegisterCallback: var == NULL\n");
		return;
	}
	variable->callback = callback;
}

void Cvar_RegisterVariableAlias(cvar_t *variable, const char *name )
{
	cvar_state_t *cvars = &cvars_all;
	cvar_hash_t *hash;
	int hashindex;

	if (cls.state == ca_dedicated && !(variable->flags & CF_SERVER))
		return;

	if (!*name)
	{
		Con_Printf (CON_WARN "Cvar_RegisterVariableAlias: invalid virtual cvar name\n");
		return;
	}

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, name))
	{
		Con_Printf (CON_WARN "Cvar_RegisterVariableAlias: %s is a command\n", name);
		return;
	}

	if (Cvar_FindVar(&cvars_all, name, 0))
	{
		Con_Printf (CON_WARN "Cvar_RegisterVariableAlias: %s is a cvar\n", name);
		return;
	}

	// Resize the variable->aliases list to have room for another entry and a null terminator.
	// This zero-pads when resizing, so we don't need to write the NULL terminator manually here.
	// Also if aliases is NULL this allocates fresh for the correct size, so it's fine to just do this.
	variable->aliases = (char **)Z_Realloc(variable->aliases, sizeof(char *) * (variable->aliases_size + 2));
	// Add the new alias, and increment the number of aliases in the list
	variable->aliases[variable->aliases_size++] = (char *)Z_StrDup(name);

	// link to head of list in this hash table index
	hash = (cvar_hash_t *)Z_Malloc(sizeof(cvar_hash_t));
	hashindex = CRC_Block((const unsigned char *)name, strlen(name)) % CVAR_HASHSIZE_65536;
	hash->next = cvars->hashtable[hashindex];
	cvars->hashtable[hashindex] = hash;
	hash->cvar = variable;
}

/*
============
Cvar_Link

Links a variable to the variable list and hashtable
============
*/
static void Cvar_Link(cvar_t *variable, cvar_state_t *cvars)
{
	cvar_t *current, *next;
	cvar_hash_t *hash;
	int hashindex;
	/*
	 * Link the variable in
	 * alphanumerical order
	 */
	for (current = NULL, next = cvars->vars ; next && strcmp( next->name, variable->name ) < 0 ; current = next, next = next->next )
		;
	if (current)
		current->next = variable;
	else
		cvars->vars = variable;
	variable->next = next;

	// link to head of list in this hash table index
	hash = (cvar_hash_t *)Z_Malloc(sizeof(cvar_hash_t));
	hashindex = CRC_Block((const unsigned char *)variable->name, strlen(variable->name)) % CVAR_HASHSIZE_65536;
	hash->next = cvars->hashtable[hashindex];
	hash->cvar = variable;
	cvars->hashtable[hashindex] = hash;
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	cvar_state_t *cvars = NULL;
	cvar_t *current, *cvar;
	int i;

	switch (variable->flags & (CF_CLIENT | CF_SERVER))
	{
	case CF_CLIENT: // client-only cvar
		if (cls.state == ca_dedicated)
			return;
	case CF_SERVER:
	case CF_CLIENT | CF_SERVER:
		cvars = &cvars_all;
		break;
	case 0:
		Sys_Error ("Cvar_RegisterVariable({" QUOTED_S ", " QUOTED_S ", %d}) with no CF_CLIENT | CF_SERVER flags", variable->name, variable->string, variable->flags);
		break;
	default:
		Sys_Error ("Cvar_RegisterVariable({" QUOTED_S ", " QUOTED_S ", %d}) with weird CF_CLIENT | CF_SERVER flags", variable->name, variable->string, variable->flags);
		break;
	}

	if (developer_extra.integer)
		Con_PrintLinef ("Cvar_RegisterVariable({" QUOTED_S ", " QUOTED_S ", %d})", variable->name, variable->string, variable->flags);

	// first check to see if it has already been defined
	cvar = Cvar_FindVar(cvars, variable->name, ALL_FLAGS_ANTIZERO);
	if (cvar) {
		if (Have_Flag (cvar->flags, CF_ALLOCATED)) {
			if (developer_extra.integer)
				Con_DPrintLinef ("...  replacing existing allocated cvar {" QUOTED_S ", " QUOTED_S ", %d}", cvar->name, cvar->string, cvar->flags);
			// fixed variables replace allocated ones
			// (because the engine directly accesses fixed variables)
			// NOTE: this isn't actually used currently
			// (all cvars are registered before config parsing)
			variable->flags &= ~CF_ALLOCATED;
			// cvar->string is now owned by variable instead
			variable->string = cvar->string;
			variable->defstring = cvar->defstring;
			variable->value = atof (variable->string);
			variable->integer = (int) variable->value;
			// Preserve autocvar status.
			memcpy(variable->globaldefindex, cvar->globaldefindex, sizeof(variable->globaldefindex));
			memcpy(variable->globaldefindex_stringno, cvar->globaldefindex_stringno, sizeof(variable->globaldefindex_stringno));
			// replace cvar with this one...
			variable->next = cvar->next;
			if (cvars->vars == cvar)
			{
				// head of the list is easy to change
				cvars->vars = variable;
			}
			else
			{
				// otherwise find it somewhere in the list
				for (current = cvars->vars;current->next != cvar;current = current->next)
					;
				current->next = variable;
			}

			// get rid of old allocated cvar
			// (but not cvar->string and cvar->defstring, because we kept those)
			Z_Free((char *)cvar->name);
			Z_Free(cvar);
		} // allocated - replace
		else // Baker: registered cvar, cannot replace ... print a message
			// Baker: Does this happen on gamedir change?
			Con_DPrintLinef ("Can't register variable %s, already defined", variable->name);
		return;
	} // existing cvar

	// NEW CVAR

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, variable->name)) {
		Con_PrintLinef ("Cvar_RegisterVariable: %s is a command", variable->name); // AS-IS, not debug
		return;
	}

	//const char *s = ;

	// copy the value off, because future sets will Z_Free it
	variable->name = (char *)Mem_strdup(zonemempool, variable->name);

	variable->engine_string = Z_StrDup (variable->string);
#if 0
	variable->string = (char *)Mem_strdup(zonemempool, variable->engine_string);
#else
	variable->string = (char *)Mem_strdup(zonemempool, variable->string);
#endif

	

	variable->defstring = (char *)Mem_strdup(zonemempool, variable->string);
	variable->value = atof (variable->string);
	variable->integer = (int) variable->value;
	variable->aliases = NULL;
	variable->aliases_size = 0;
	variable->initstate = NULL;

	// Mark it as not an autocvar.
	for (i = 0;i < PRVM_PROG_MAX_3;i++)
		variable->globaldefindex[i] = -1;

	// Safe for Cvar_SetQuick()
	variable->flags |= CF_REGISTERED;

	Cvar_Link(variable, cvars);
}

/*
============
Cvar_Get

Adds a newly allocated variable to the variable list or sets its value.
============
*/
// Baker: This can create a cvar (autocvar_)
cvar_t *Cvar_Get(cvar_state_t *cvars, const char *name, const char *value, int flags, const char *newdescription)
{
	cvar_t *cvar;
	int i;

	if (developer_extra.integer)
		Con_DPrintLinef ("Cvar_Get(" QUOTED_S ", " QUOTED_S ", %d);", name, value, flags);

	// first check to see if it has already been defined
	cvar = Cvar_FindVar(cvars, name, ALL_FLAGS_ANTIZERO);
	if (cvar)
	{
		cvar->flags |= flags;
		Cvar_SetQuick_Internal (cvar, value);
		if (newdescription && (cvar->flags & CF_ALLOCATED))
		{
			if (cvar->description != cvar_dummy_description)
				Z_Free((char *)cvar->description);

			if (*newdescription)
				cvar->description = (char *)Mem_strdup(zonemempool, newdescription);
			else
				cvar->description = cvar_dummy_description;
		}
		return cvar;
	}

	// check for pure evil
	if (!*name) {
		Con_PrintLinef ("Cvar_Get: invalid variable name");
		return NULL;
	}

	// check for overlap with a command
	if (Cmd_Exists(cmd_local, name)) {
		Con_PrintLinef ("Cvar_Get: %s is a command", name); // ASIS
		return NULL;
	}

	// allocate a new cvar, cvar name, and cvar string
	// TODO: factorize the following code with the one at the end of Cvar_RegisterVariable()
	// FIXME: these never get Z_Free'd
	cvar = (cvar_t *)Z_Malloc(sizeof(cvar_t));
	cvar->flags = flags | CF_ALLOCATED;
	cvar->name = (char *)Mem_strdup(zonemempool, name);
	cvar->string = (char *)Mem_strdup(zonemempool, value);
	cvar->defstring = (char *)Mem_strdup(zonemempool, value);
	cvar->value = atof (cvar->string);
	cvar->integer = (int) cvar->value;
	cvar->aliases = NULL;
	cvar->aliases_size = 0;
	cvar->initstate = NULL;

	if (newdescription && *newdescription)
		cvar->description = (char *)Mem_strdup(zonemempool, newdescription);
	else
		cvar->description = cvar_dummy_description; // actually checked by VM_cvar_type

	// Mark it as not an autocvar.
	for (i = 0;i < PRVM_PROG_MAX_3;i++)
		cvar->globaldefindex[i] = -1;

	Cvar_Link(cvar, cvars);

	return cvar;
}

qbool Cvar_Readonly (cvar_t *var, const char *cmd_name)
{
	if (var->flags & CF_READONLY)
	{
		if (cmd_name)
			Con_Printf ("%s: ",cmd_name);
		Con_Printf ("%s", var->name);
		Con_PrintLinef (" is read-only");
		return true;
	}
	return false;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qbool	Cvar_Command (cmd_state_t *cmd)
{
	cvar_state_t	*cvars = cmd->cvars;
	cvar_t			*v;

	// check variables
	v = Cvar_FindVar(cvars, Cmd_Argv(cmd, 0), (cmd->cvars_flagsmask));
	if (!v)
		return false;

	// perform a variable print or set
	if (Cmd_Argc(cmd) == 1)
	{
		Cvar_PrintHelp(v, Cmd_Argv(cmd, 0), true);
		return true;
	}

	if (developer_extra.integer)
		Con_DPrint("Cvar_Command: ");

	if (Cvar_Readonly(v, NULL))
		return true;

	Cvar_SetQuick(v, Cmd_Argv(cmd, 1));
	if (developer_extra.integer)
		Con_DPrint("\n");
	return true;
}

void Cvar_UnlockDefaults(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// unlock the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
		var->flags &= ~CF_DEFAULTSET;
}

void Cvar_LockDefaults_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// lock in the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if (Have_Flag(var->flags, CF_DEFAULTSET) == false)
		{
			size_t alloclen;

			//Con_Printf ("locking cvar %s (%s -> %s)\n", var->name, var->string, var->defstring);
			Flag_Add_To (var->flags, CF_DEFAULTSET); // |= CF_DEFAULTSET;
			Z_Free((char *)var->defstring);
			alloclen = strlen(var->string) + 1;
			var->defstring = (char *)Z_Malloc(alloclen);
			memcpy ((char *)var->defstring, var->string, alloclen);
			if (var->initstate) {
				var->initstate->defstring = var->defstring; // Baker r9061: Update pointer
			}
		}
	}
}

void Cvar_SaveInitState(cvar_state_t *cvars)
{
	cvar_t *c;
	for (c = cvars->vars;c;c = c->next)
	{
		c->initstate = (cvar_t *)Z_Malloc(sizeof(cvar_t));
		memcpy(c->initstate, c, sizeof(cvar_t));
	}
}

WARP_X_CALLERS_ (Cmd_RestoreInitState)
void Cvar_RestoreInitState(cvar_state_t *cvars)
{
	int hashindex;
	cvar_t *c, **cp;

	for (cp = &cvars->vars; (c = *cp); ) {

		if (c->initstate)
		{
			// restore this cvar, it existed at init
			if (   ((c->flags ^ c->initstate->flags) & CF_MAXFLAGSVAL) || // Baker: flags changed or string does not match default string?
					strcmp(c->defstring ? c->defstring : "", c->initstate->defstring ? c->initstate->defstring : "")
			 || strcmp(c->string ? c->string : "", c->initstate->string ? c->initstate->string : ""))
			{
				Con_DPrintLinef ("Cvar_RestoreInitState: Restoring cvar " QUOTED_S, c->name);

				// Baker r9002: DarkPlaces Beta gamedir switch crash bug-fix
				const char *s_copy_str = NULL;
				const char *s_copy_defstr = NULL;

				// Baker: I doubt c->initstate->string can be trusted
				if (c->string)		s_copy_str = Mem_strdup(zonemempool, c->initstate->defstring); // Baker r9061: don't trust c->initstate->string ever
				if (c->defstring)	s_copy_defstr = Mem_strdup(zonemempool, c->initstate->defstring);
				if (c->defstring)
					Z_Free((char *)c->defstring);
				c->defstring = c->initstate->defstring = s_copy_defstr;
				if (c->string)
					Z_Free((char *)c->string);
				c->string =  c->initstate->string = s_copy_str;

			}
			else {
				// Con_PrintLinef ("Cvar_RestoreInitState: " QUOTED_S " is unchanged", c->name);
				// Baker need to set string to default string
				if (String_Match (c->string, c->defstring) == false) {
					Z_Free((char *)c->string);
					c->string = Mem_strdup(zonemempool, c->defstring);
				}
			}
			c->flags = c->initstate->flags;
			c->value = c->initstate->value;
			c->integer = c->initstate->integer;

			if (1) {
				// AUDIT TIME
				float fval = atof(c->string);
				qbool value_match = fval == c->value;
				int ival = atoi(c->string);
				qbool integer_match = ival == c->integer;
				if (value_match == false)
					c->value = fval; // Baker: Sadly this happens
				if (integer_match == false)
					c->integer = ival; // Baker: Sadly this happens

			}
			VectorCopy(c->initstate->fvector, c->fvector);
			cp = &c->next;
		}
		else
		{
			if (Have_Flag(c->flags, CF_ALLOCATED) == false) {
				Con_DPrintLinef ("Cvar_RestoreInitState: Unable to destroy cvar " QUOTED_S ", it was registered after init!", c->name);
				// In this case, at least reset it to the default.
				if ((c->flags & CF_PERSISTENT) == 0)
					Cvar_SetQuick(c, c->defstring);
				cp = &c->next;
				continue;
			}
			// check if a cvar is held by some progs
			if (Cvar_IsAutoCvar(c)) {
				Con_DPrintLinef ("Cvar_RestoreInitState: Unable to destroy cvar " QUOTED_S ", it is an autocvar used by running progs!", c->name);
				// In this case, at least reset it to the default.
				if ((c->flags & CF_PERSISTENT) == 0)
					Cvar_SetQuick(c, c->defstring);
				cp = &c->next;
				continue;
			}
			// remove this cvar, it did not exist at init
			Con_DPrintLinef ("Cvar_RestoreInitState: Destroying cvar " QUOTED_S, c->name);
			// unlink struct from hash
			hashindex = CRC_Block((const unsigned char *)c->name, strlen(c->name)) % CVAR_HASHSIZE_65536;

			cvar_hash_t *hash_cur, *hash_prev = NULL;
#if 1
			// Baker r9002: DarkPlaces Beta gamedir switch crash bug-fix
			for (hash_cur = cvars->hashtable[hashindex]; hash_cur; hash_cur = hash_cur->next) {
				cvar_t *cvar_current = hash_cur->cvar;
				Con_DPrintLinef ("Unlinking %s", c->name);
				if (cvar_current == c) {
					// Destroy the hash struct, it was Z_ allocated
					if (hash_prev) {
						// Sys_PrintfToTerminal ("Rerouting next of %s was %s to %s (skipping us)\n", hash_prev->cvar->name, hash_cur->cvar->name, hash_cur->next->cvar->name);
						hash_prev->next = hash_cur->next; // Skip us
					}
					else {
						// Uh .. we were head.
						//Sys_PrintfToTerminal ("Head situation %s, nullifying?\n", hash_cur->cvar->name);
						cvars->hashtable[hashindex] = NULL; // Baker: there is no next ..
					}
					break;
				}

				// continue loop ...
				hash_prev = hash_cur;
			} // each hash
#else
			// Baker: This is confusing with variable means that don't describe what they are
			// and we know this chunk of code crashes and does not seem to z free the hash struct
			// , we know ... it unlinks from list let's rewrite this clearly
			for (cp2 = &cvars->hashtable[hashindex]->cvar;(c2 = *cp2);)
			{
				if (c2 == c) {
					// Baker r9002: DarkPlaces Beta gamedir switch crash bug-fix
// Baker: I'm not sure if this is right, we no check hash table head
					if (cvars->hashtable[hashindex]->next)
					*cp2 = cvars->hashtable[hashindex]->next->cvar;
					else
						cvars->hashtable[hashindex] = NULL; // Baker: there is no next ..
					break;
				}
				else
					cp2 = &cvars->hashtable[hashindex]->next->cvar;
			}
#endif
			// unlink struct from main list
			*cp = c->next;
			// free strings
			if (c->defstring)
				Z_Free((char *)c->defstring);
			if (c->string)
				Z_Free((char *)c->string);
			if (c->description && c->description != cvar_dummy_description)
				Z_Free((char *)c->description);
			// free struct
			Z_Free(c);
		}
	} // for each var
}

// Baker r3173: inc, dec, cvar_reset commands
void Cvar_Reset_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	int numargs = Cmd_Argc(cmd);
	cvar_t *cvCVar;
	switch (numargs) {

	default:// no args or such
			Con_PrintLinef ("cvar_reset - Usage" NEWLINE
							"  cvar_reset <variable> - reset to default value");
			break;

	case 2:	cvCVar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ALL_FLAGS_ANTIZERO );
			if (!cvCVar) {
				Con_PrintLinef ("Variable %s not found",  Cmd_Argv(cmd, 1) );
				break;
			}

			if (Have_Flag (cvCVar->flags, CF_PERSISTENT)) {
				// msg
				Con_PrintLinef ("%s is CF_PERSISTENT, no reset possible", Cmd_Argv(cmd, 1));
				break;
			}

			if (Have_Flag (cvCVar->flags, CF_READONLY)) {
				// msg
				Con_PrintLinef ("%s is CF_READONLY, no reset possible", Cmd_Argv(cmd, 1));
				break;
			}

			Cvar_SetQuick(cvCVar, cvCVar->defstring);
			break;
	} // sw


	// fail comes here
}

void Cvar_ResetToDefaults_All_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if (Have_Flag(var->flags, CF_PERSISTENT) == 0)
			Cvar_SetQuick(var, var->defstring);
	}
}

void Cvar_ResetToDefaults_NoSaveOnly_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if (Have_Flag(var->flags, (CF_PERSISTENT | CF_ARCHIVE)) == 0)
			Cvar_SetQuick(var, var->defstring);
	}
}


void Cvar_ResetToDefaults_SaveOnly_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *var;
	// restore the default values of all cvars
	for (var = cvars->vars ; var ; var = var->next)
	{
		if ((var->flags & (CF_PERSISTENT | CF_ARCHIVE)) == CF_ARCHIVE)
			Cvar_SetQuick(var, var->defstring);
	}
}

/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (cvar_state_t *cvars, qfile_t *f)
{
	cvar_t	*var;
	char buf1[MAX_INPUTLINE_16384], buf2[MAX_INPUTLINE_16384];

	// don't save cvars that match their default value
	for (var = cvars->vars ; var ; var = var->next) {
		if (Have_Flag (var->flags, CF_ARCHIVE) == false)
			continue; // Not a saved cvar

		// If the string value is different, save
		int shall_save = String_NOT_Match (var->string, var->defstring);

		// If string value is same
		// and it is allocated without a default (SETA), save it anyway
		if (shall_save == false) {
			// Baker we have a saved allocated cvar (SETA) and the default it not set?
			if (Have_Flag(var->flags, CF_ALLOCATED) && Have_Flag(var->flags, CF_DEFAULTSET) == false) {
				shall_save = true;
			} // if
		} // if

		if (shall_save == false)
			continue;

		Cmd_QuoteString(buf1, sizeof(buf1), var->name, "\"\\$", false);
		Cmd_QuoteString(buf2, sizeof(buf2), var->string, "\"\\$", false);
		FS_Printf (f, "%s" QUOTED_S " " QUOTED_S NEWLINE, var->flags & CF_ALLOCATED ? "seta " : "", buf1, buf2);
	} // for each cvar
}

void Cvar_WriteVariables_All_Changed (cvar_state_t *cvars, qfile_t *f)
{
	cvar_t	*var;
	char buf1[MAX_INPUTLINE_16384], buf2[MAX_INPUTLINE_16384];

	for (var = cvars->vars ; var ; var = var->next) {
		//if (Have_Flag (var->flags, CF_ARCHIVE) == false)
		//	continue; // Not a saved cvar

		// If the string value is different, save
		int shall_write = String_NOT_Match (var->string, var->defstring);

		if (shall_write == false)
			continue;

		Cmd_QuoteString(buf1, sizeof(buf1), var->name, "\"\\$", false);
		Cmd_QuoteString(buf2, sizeof(buf2), var->string, "\"\\$", false);
		FS_Printf (f, "%s" QUOTED_S " " QUOTED_S NEWLINE, var->flags & CF_ALLOCATED ? "seta " : "", buf1, buf2);
	} // for each cvar
}

void Cvar_WriteVariables_All (cvar_state_t *cvars, qfile_t *f)
{
	cvar_t	*var;
	char buf1[MAX_INPUTLINE_16384], buf2[MAX_INPUTLINE_16384];

	for (var = cvars->vars ; var ; var = var->next) {
		Cmd_QuoteString(buf1, sizeof(buf1), var->name, "\"\\$", false);
		Cmd_QuoteString(buf2, sizeof(buf2), var->string, "\"\\$", false);
		FS_Printf (f, "%s" QUOTED_S " " QUOTED_S NEWLINE, var->flags & CF_ALLOCATED ? "seta " : "", buf1, buf2);
	} // for each cvar
}

// Added by EvilTypeGuy eviltypeguy@qeradiant.com
// 2000-01-09 CvarList command By Matthias "Maddes" Buecher, http://www.inside3d.com/qip/
/*
=========
Cvar_List
=========
*/
void Cvar_List_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;
	const char *partial;
	int count;
	qbool ispattern;
	char vabuf[1024];
	int wants_changed = false;

	if (Cmd_Argc(cmd) == 2 && String_Match(Cmd_Argv(cmd, 1), "changed")) {
		wants_changed = true;
		c_strlcpy (vabuf, "*");
		partial = vabuf; //va(vabuf, sizeof(vabuf), "%s", "*");
		ispattern = false;
	} else
	if (Cmd_Argc(cmd) > 1) {
		partial = Cmd_Argv(cmd, 1);
		ispattern = (strchr(partial, '*') || strchr(partial, '?'));
		if (!ispattern)
			partial = va(vabuf, sizeof(vabuf), "%s*", partial);
	}
	else
	{
		c_strlcpy (vabuf, "*");
		partial = vabuf;//va(vabuf, sizeof(vabuf), "*");
		ispattern = false;
	}

	count = 0;
	for (cvar = cvars->vars; cvar; cvar = cvar->next) {
		if (matchpattern_with_separator(cvar->name, partial, false, "", false)) {
			int shall_write = wants_changed == false || String_NOT_Match (cvar->string, cvar->defstring);

			if (shall_write) {
				Cvar_PrintHelp(cvar, cvar->name, true);
				count++;
			}
		}
		for (char **alias = cvar->aliases; alias && *alias; alias++) {
			if (matchpattern_with_separator(*alias, partial, false, "", false)) {
			int shall_write = wants_changed == false|| String_NOT_Match (cvar->string, cvar->defstring);
				if (shall_write) {
					Cvar_PrintHelp(cvar, *alias, true);
					count++;
				}
			}
		} // for alias
	} // for

	if (Cmd_Argc(cmd) > 1 && wants_changed == false) {
		if (ispattern)
			Con_PrintLinef ("%d cvar%s matching " QUOTED_S, count, (count > 1) ? "s" : "", partial);
		else
			Con_PrintLinef ("%d cvar%s beginning with " QUOTED_S, count, (count > 1) ? "s" : "", Cmd_Argv(cmd,1));
	}
	else
		Con_PrintLinef ("%d cvar(s)", count);
}

// 2000-01-09 CvarList command by Maddes
// Baker: Xonotic example ...
// set hud_configure_teamcolorforced 0 "1 = force display of team colors in configure mode"

void Cvar_Set_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;

	// make sure it's the right number of parameters
	if (Cmd_Argc(cmd) < 3) {
		Con_PrintLinef ("Set: wrong number of parameters, usage: set <variablename> <value> [<description>]");
		return;
	}

	// check if it's read-only
	cvar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ALL_FLAGS_ANTIZERO);
	if (cvar)
		if (Cvar_Readonly(cvar,"Set"))
			return;

	if (developer_extra.integer)
		Con_DPrint("Set: ");

	// all looks ok, create/modify the cvar
	Cvar_Get(cvars, Cmd_Argv(cmd, 1), Cmd_Argv(cmd, 2), cmd->cvars_flagsmask, Cmd_Argc(cmd) > 3 ? Cmd_Argv(cmd, 3) : NULL);
}


// Baker: Xonotic example ...
// seta hud_configure_teamcolorforced 0 "1 = force display of team colors in configure mode"
void Cvar_SetA_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	cvar_t *cvar;

	// make sure it's the right number of parameters
	if (Cmd_Argc(cmd) < 3) {
		Con_PrintLinef ("SetA: wrong number of parameters, usage: seta <variablename> <value> [<description>]");
		return;
	}

	// check if it's read-only
	cvar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ALL_FLAGS_ANTIZERO);
	if (cvar)
		if (Cvar_Readonly(cvar,"SetA"))
			return;

	if (developer_extra.integer)
		Con_DPrint("SetA: ");

	// all looks ok, create/modify the cvar
	Cvar_Get(cvars,
		Cmd_Argv(cmd, 1),								// name
		Cmd_Argv(cmd, 2),								// value
		cmd->cvars_flagsmask | CF_ARCHIVE,				// flags
		Cmd_Argc(cmd) > 3 ? Cmd_Argv(cmd, 3) : NULL		// description
	);
}

void Cvar_Del_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	int neededflags = ~0;
	int i;
	cvar_t *parent, **link;
	cvar_t *cvar, *prev;

	if (Cmd_Argc(cmd) < 2)
	{
		Con_PrintLinef ("%s: wrong number of parameters, usage: unset <variablename1> [<variablename2> ...]", Cmd_Argv(cmd, 0));
		return;
	}
	for(i = 1; i < Cmd_Argc(cmd); ++i)
	{
		cvar = Cvar_FindVarLink(cvars, Cmd_Argv(cmd, i), &parent, &link, &prev, neededflags);

		if (!cvar)
		{
			Con_PrintLinef ("%s: %s is not defined", Cmd_Argv(cmd, 0), Cmd_Argv(cmd, i));
			continue;
		}
		if (Cvar_Readonly(cvar, Cmd_Argv(cmd, 0)))
			continue;
		if (!(cvar->flags & CF_ALLOCATED))
		{
			Con_PrintLinef ("%s: %s is static and cannot be deleted", Cmd_Argv(cmd, 0), cvar->name);
			continue;
		}
		if (cvar == cvars->vars)
		{
			cvars->vars = cvar->next;
		}
		else
		{
			// in this case, prev must be set, otherwise there has been some inconsistensy
			// elsewhere already... should I still check for prev != NULL?
			prev->next = cvar->next;
		}

		if (parent)
			parent->next = cvar->next;
		else if (link)
			*link = cvar->next;
		if (cvar->description != cvar_dummy_description)
			Z_Free((char *)cvar->description);

		Z_Free((char *)cvar->name);
		Z_Free((char *)cvar->string);
		Z_Free((char *)cvar->defstring);
		Z_Free(cvar);
	}
}

