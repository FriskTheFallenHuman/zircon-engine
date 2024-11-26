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
// AK new vm

#include "quakedef.h"
#include "progsvm.h"
#include "csprogs.h"

prvm_prog_t prvm_prog_list[PRVM_PROG_MAX_3];

int		prvm_type_size[8] = {1,sizeof(string_t)/4,1,3,1,1,sizeof(func_t)/4,sizeof(void *)/4};

prvm_eval_t prvm_badvalue; // used only for error returns

cvar_t prvm_language = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "prvm_language", "", "when set, loads PROGSFILE.LANGUAGENAME.po and common.LANGUAGENAME.po for string translations; when set to dump, PROGSFILE.pot is written from the strings in the progs"};

// Baker r7084: gamecommand autocomplete
cvar_t prvm_sv_gamecommands = {CF_SERVER | SV_RESETNEWMAP_0, "prvm_sv_gamecommands", "", "Space delimited list of GameCommands for SV.  Set via progs to provide engine information for console autocomplete of sv_cmd [Zircon]"};  // Baker r7103 gamecommand autocomplete
cvar_t prvm_sv_progfields = {CF_SERVER | SV_RESETNEWMAP_0, "prvm_sv_progfields", "", "Space delimited list of SV prog fieldnames.  Set via progs to provide engine information for console autocomplete of sv_cmd [Zircon]"};  // Baker r7103 gamecommand autocomplete

cvar_t prvm_cl_gamecommands = {CF_CLIENT | CL_RESETNEWMAP_0, "prvm_cl_gamecommands", "", "Space delimited list of GameCommands for CL.  Set via progs to provide engine information for console autocomplete of cl_cmd [Zircon]"};  // Baker r7103 gamecommand autocomplete
cvar_t prvm_cl_progfields = {CF_CLIENT | CL_RESETNEWMAP_0, "prvm_cl_progfields", "", "Space delimited list of CL prog fieldnames.  Set via progs to provide engine information for console autocomplete of cl_cmd [Zircon]"};  // Baker r7103 gamecommand autocomplete

cvar_t prvm_edictget_autostore = {CF_CLIENT, "prvm_edictget_autostore", "2", "1: Auto-store prvm_edictget to _last cvar, 2: to clipboard also [Zircon]"};  // Baker: August 8 2024
cvar_t _last = {CF_CLIENT, "_last", "", "last value of \"eq\" or prvm_edictget_autostore [Zircon]"};  /// Baker: August 8 2024 .. Baker: Changed to _last Oct 13 2024



//cvar_t prvm_menu_gamecommands = {CF_MENU, "prvm_menu_gamecommands", "", "Space delimited list of GameCommands for MENU.  Set via progs to provide engine information for console autocomplete of menu_cmd [Zircon]"};


// LadyHavoc: prints every opcode as it executes - warning: this is significant spew
cvar_t prvm_traceqc = {CF_CLIENT | CF_SERVER, "prvm_traceqc", "0", "prints every QuakeC statement as it is executed (only for really thorough debugging!)"};
// LadyHavoc: counts usage of each QuakeC statement
cvar_t prvm_statementprofiling = {CF_CLIENT | CF_SERVER, "prvm_statementprofiling", "0", "counts how many times each QuakeC statement has been executed, these counts are displayed in prvm_printfunction output (if enabled)"};
cvar_t prvm_timeprofiling = {CF_CLIENT | CF_SERVER, "prvm_timeprofiling", "0", "counts how long each function has been executed, these counts are displayed in prvm_profile output (if enabled)"};
cvar_t prvm_coverage = {CF_CLIENT | CF_SERVER, "prvm_coverage", "0", "report and count coverage events (1: per-function, 2: coverage() builtin, 4: per-statement)"};
cvar_t prvm_backtraceforwarnings = {CF_CLIENT | CF_SERVER, "prvm_backtraceforwarnings", "0", "print a backtrace for warnings too"};
cvar_t prvm_leaktest = {CF_CLIENT | CF_SERVER, "prvm_leaktest", "0", "try to detect memory leaks in strings or entities"};
cvar_t prvm_leaktest_follow_targetname = {CF_CLIENT | CF_SERVER, "prvm_leaktest_follow_targetname", "0", "if set, target/targetname links are considered when leak testing; this should normally not be required, as entities created during startup - e.g. info_notnull - are never considered leaky"};
cvar_t prvm_leaktest_ignore_classnames = {CF_CLIENT | CF_SERVER, "prvm_leaktest_ignore_classnames", "", "classnames of entities to NOT leak check because they are found by find(world, classname, ...) but are actually spawned by QC code (NOT map entities)"};
cvar_t prvm_errordump = {CF_CLIENT | CF_SERVER, "prvm_errordump", "0", "write a savegame on crash to crash-server.dmp"};
cvar_t prvm_breakpointdump = {CF_CLIENT | CF_SERVER, "prvm_breakpointdump", "0", "write a savegame on breakpoint to breakpoint-server.dmp"};
cvar_t prvm_reuseedicts_startuptime = {CF_CLIENT | CF_SERVER, "prvm_reuseedicts_startuptime", "2", "allows immediate re-use of freed entity slots during start of new level (value in seconds)"};
cvar_t prvm_reuseedicts_neverinsameframe = {CF_CLIENT | CF_SERVER, "prvm_reuseedicts_neverinsameframe", "1", "never allows re-use of freed entity slots during same frame"};
cvar_t prvm_garbagecollection_enable = {CF_CLIENT | CF_SERVER, "prvm_garbagecollection_enable", "1", "automatically scan for and free resources that are not referenced by the code being executed in the VM"};
cvar_t prvm_garbagecollection_notify = {CF_CLIENT | CF_SERVER, "prvm_garbagecollection_notify", "0", "print out a notification for each resource freed by garbage collection"};
cvar_t prvm_garbagecollection_scan_limit = {CF_CLIENT | CF_SERVER, "prvm_garbagecollection_scan_limit", "10000", "scan this many fields or resources per frame to free up unreferenced resources"};
cvar_t prvm_garbagecollection_strings = {CF_CLIENT | CF_SERVER, "prvm_garbagecollection_strings", "1", "automatically call strunzone() on strings that are not referenced"};
cvar_t prvm_stringdebug = {CF_CLIENT | CF_SERVER, "prvm_stringdebug", "0", "Print debug and warning messages related to strings"};

static double prvm_reuseedicts_always_allow = 0;
qbool prvm_runawaycheck = true;

//============================================================================
// mempool handling

/*
===============
PRVM_MEM_Alloc
===============
*/
static void PRVM_MEM_Alloc(prvm_prog_t *prog)
{
	int i;

	// reserve space for the null entity aka world
	// check bound of max_edicts
	prog->max_edicts = bound(1 + prog->reserved_edicts, prog->max_edicts, prog->limit_edicts);
	prog->num_edicts = bound(1 + prog->reserved_edicts, prog->num_edicts, prog->max_edicts);

	// edictprivate_size has to be min as big prvm_edict_private_t
	prog->edictprivate_size = max(prog->edictprivate_size,(int)sizeof(prvm_edict_private_t));

	// alloc edicts
	prog->edicts = (prvm_edict_t *)Mem_Alloc(prog->progs_mempool,prog->limit_edicts * sizeof(prvm_edict_t));

	// alloc edict private space
	prog->edictprivate = Mem_Alloc(prog->progs_mempool, prog->max_edicts * prog->edictprivate_size);

	// alloc edict fields
	prog->entityfieldsarea = prog->entityfields * prog->max_edicts;
	prog->edictsfields.fp = (prvm_vec_t *)Mem_Alloc(prog->progs_mempool, prog->entityfieldsarea * sizeof(prvm_vec_t));

	// set edict pointers
	for(i = 0; i < prog->max_edicts; i++)
	{
		prog->edicts[i].priv.required = (prvm_edict_private_t *)((unsigned char  *)prog->edictprivate + i * prog->edictprivate_size);
		prog->edicts[i].fields.fp = prog->edictsfields.fp + i * prog->entityfields;
	}
}

/*
===============
PRVM_MEM_IncreaseEdicts
===============
*/
void PRVM_MEM_IncreaseEdicts(prvm_prog_t *prog)
{
	int		i;

	if (prog->max_edicts >= prog->limit_edicts)
		return;

	prog->begin_increase_edicts(prog);

	// increase edicts
	prog->max_edicts = min(prog->max_edicts + 256, prog->limit_edicts);

	prog->entityfieldsarea = prog->entityfields * prog->max_edicts;
	size_t szthis = sizeof(prvm_vec_t);
	prog->edictsfields.fp = (prvm_vec_t*)Mem_Realloc(prog->progs_mempool, (void *)prog->edictsfields.fp, prog->entityfieldsarea * sizeof(prvm_vec_t));
	prog->edictprivate = (void *)Mem_Realloc(prog->progs_mempool, (void *)prog->edictprivate, prog->max_edicts * prog->edictprivate_size);

	//set e and v pointers
	for(i = 0; i < prog->max_edicts; i++)
	{
		prog->edicts[i].priv.required  = (prvm_edict_private_t *)((unsigned char  *)prog->edictprivate + i * prog->edictprivate_size);
		prog->edicts[i].fields.fp = prog->edictsfields.fp + i * prog->entityfields;
	}

	prog->end_increase_edicts(prog);
}

//============================================================================
// normal prvm

int PRVM_ED_FindFieldOffset(prvm_prog_t *prog, const char *field)
{
	mdef_t *d;
	d = PRVM_ED_FindField(prog, field);
	if (!d)
		return -1;
	return d->ofs;
}

int PRVM_ED_FindGlobalOffset(prvm_prog_t *prog, const char *global)
{
	mdef_t *d;
	d = PRVM_ED_FindGlobal(prog, global);
	if (!d)
		return -1;
	return d->ofs;
}

func_t PRVM_ED_FindFunctionOffset(prvm_prog_t *prog, const char *function)
{
	mfunction_t *f;
	f = PRVM_ED_FindFunction(prog, function);
	if (!f)
		return 0;
	return (func_t)(f - prog->functions);
}

/*
=================
PRVM_ProgFromString
=================
*/
prvm_prog_t *PRVM_ProgFromString(const char *str)
{
	if (String_Match(str, "server"))
		return SVVM_prog;
	if (String_Match(str, "client"))
		return CLVM_prog;
#ifdef CONFIG_MENU
	if (String_Match(str, "menu"))
		return MVM_prog;
#endif
	return NULL;
}

/*
=================
PRVM_FriendlyProgFromString
=================
*/
prvm_prog_t *PRVM_FriendlyProgFromString(const char *str)
{
	prvm_prog_t *prog = PRVM_ProgFromString(str);
	if (!prog)
	{
		Con_Printf ("%s: unknown program name\n", str);
		return NULL;
	}
	if (!prog->loaded)
	{
		Con_Printf ("%s: program is not loaded\n", str);
		return NULL;
	}
	return prog;
}

/*
=================
PRVM_ED_ClearEdict

Sets everything to NULL.

Nota bene: this also marks the entity as allocated if it has been previously
freed and sets the allocation origin.
=================
*/
void PRVM_ED_ClearEdict(prvm_prog_t *prog, prvm_edict_t *e)
{
	memset(e->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
	e->free = false;
	e->freetime = host.realtime;
	if (e->priv.required->allocation_origin)
		Mem_Free((char *)e->priv.required->allocation_origin);
	e->priv.required->allocation_origin = PRVM_AllocationOrigin(prog);

	// AK: Let the init_edict function determine if something needs to be initialized
	prog->init_edict(prog, e);
}

const char *PRVM_AllocationOrigin(prvm_prog_t *prog)
{
	char *buf = NULL;
	if (prog->leaktest_active)
	if (prog->depth > 0) // actually in QC code and not just parsing the entities block of a map/savegame
	{
		buf = (char *)PRVM_Alloc(256);
		PRVM_ShortStackTrace(prog, buf, 256);
	}
	return buf;
}

/*
=================
PRVM_ED_CanAlloc

Returns if this particular edict could get allocated by PRVM_ED_Alloc
=================
*/
qbool PRVM_ED_CanAlloc(prvm_prog_t *prog, prvm_edict_t *e)
{
	if (!e->free)
		return false;
	if (prvm_reuseedicts_always_allow == host.realtime)
		return true;
	if (host.realtime <= e->freetime + 0.1 && prvm_reuseedicts_neverinsameframe.integer)
		return false; // never allow reuse in same frame (causes networking trouble)
	if (e->freetime < prog->starttime + prvm_reuseedicts_startuptime.value)
		return true;
	if (host.realtime > e->freetime + 1)
		return true;
	return false; // entity slot still blocked because the entity was freed less than one second ago
}

/*
=================
PRVM_ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
prvm_edict_t *PRVM_ED_Alloc(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *e;

	// the client qc dont need maxclients
	// thus it doesnt need to use svs.maxclients
	// AK:	changed i=svs.maxclients+1
	// AK:	changed so the edict 0 wont spawn -> used as reserved/world entity
	//		although the menu/client has no world
	for (i = prog->reserved_edicts + 1;i < prog->num_edicts;i++)
	{
		e = PRVM_EDICT_NUM(i);
		if (PRVM_ED_CanAlloc(prog, e))
		{
			PRVM_ED_ClearEdict (prog, e);
			return e;
		}
	}

	if (i == prog->limit_edicts)
		prog->error_cmd("%s: PRVM_ED_Alloc: no free edicts", prog->name);

	prog->num_edicts++;
	if (prog->num_edicts >= prog->max_edicts)
		PRVM_MEM_IncreaseEdicts(prog);

	e = PRVM_EDICT_NUM(i);

	PRVM_ED_ClearEdict(prog, e);
	return e;
}

/*
=================
PRVM_ED_Free

Marks the edict as free

FIXME: walk all entities and NULL out references to this entity
bones_was_here: do not want, that would break chains immediately!
Currently chains aren't broken by removing an entity, at least with prvm_reuseedicts_neverinsameframe 1
which is very handy and some QC code will depend on it.
=================
*/
void PRVM_ED_Free(prvm_prog_t *prog, prvm_edict_t *ed)
{
	// dont delete the null entity (world) or reserved edicts
	if (ed - prog->edicts <= prog->reserved_edicts)
		return;

	prog->free_edict(prog, ed);

	ed->free = true;
	ed->freetime = host.realtime;
	if (ed->priv.required->allocation_origin)
	{
		Mem_Free((char *)ed->priv.required->allocation_origin);
		ed->priv.required->allocation_origin = NULL;
	}
}

//===========================================================================

/*
============
PRVM_ED_GlobalAtOfs
============
*/
static mdef_t *PRVM_ED_GlobalAtOfs (prvm_prog_t *prog, unsigned int ofs)
{
	mdef_t		*def;
	int			i;

	for (i = 0;i < prog->numglobaldefs;i++)
	{
		def = &prog->globaldefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
PRVM_ED_FieldAtOfs
============
*/
mdef_t *PRVM_ED_FieldAtOfs (prvm_prog_t *prog, unsigned int ofs)
{
	mdef_t		*def;
	int			i;

	for (i = 0;i < prog->numfielddefs;i++)
	{
		def = &prog->fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
PRVM_ED_FindField
============
*/
mdef_t *PRVM_ED_FindField (prvm_prog_t *prog, const char *name)
{
	mdef_t *def;
	int i;

	for (i = 0;i < prog->numfielddefs;i++)
	{
		def = &prog->fielddefs[i];
		if (String_Match(PRVM_GetString(prog, def->s_name), name))
			return def;
	}
	return NULL;
}

/*
============
PRVM_ED_FindGlobal
============
*/
mdef_t *PRVM_ED_FindGlobal (prvm_prog_t *prog, const char *name)
{
	mdef_t *def;
	int i;

	for (i = 0;i < prog->numglobaldefs;i++)
	{
		def = &prog->globaldefs[i];
		if (String_Match(PRVM_GetString(prog, def->s_name), name))
			return def;
	}
	return NULL;
}

/*
============
PRVM_ED_FindGlobalEval
============
*/
prvm_eval_t *PRVM_ED_FindGlobalEval(prvm_prog_t *prog, const char *name)
{
	mdef_t *def = PRVM_ED_FindGlobal(prog, name);
	return def ? (prvm_eval_t *) &prog->globals.fp[def->ofs] : NULL;
}

/*
============
PRVM_ED_FindFunction
============
*/
mfunction_t *PRVM_ED_FindFunction (prvm_prog_t *prog, const char *name)
{
	mfunction_t		*func;
	int				i;

	for (i = 0;i < prog->numfunctions;i++)
	{
		func = &prog->functions[i];
		if (String_Match(PRVM_GetString(prog, func->s_name), name))
			return func;
	}
	return NULL;
}


/*
============
PRVM_ValueString

Returns a string describing *data in a type specific manner
=============
*/
static char *PRVM_ValueString (prvm_prog_t *prog, etype_t type, prvm_eval_t *val, char *line, size_t linelength)
{
	mdef_t *def;
	mfunction_t *f;
	int n;

	type = (etype_t)((int) type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		strlcpy (line, PRVM_GetString (prog, val->string), linelength);
		break;
	case ev_entity:
		n = val->edict;
		if (n < 0 || n >= prog->max_edicts)
			dpsnprintf (line, linelength, "entity %d (invalid!)", n);
		else
			dpsnprintf (line, linelength, "entity %d", n);
		break;
	case ev_function:
		if ((unsigned int)val->function < (unsigned int)prog->progs_numfunctions)
		{
			f = prog->functions + val->function;
			dpsnprintf (line, linelength, "%s()", PRVM_GetString(prog, f->s_name));
		}
		else
			dpsnprintf (line, linelength, "function %" PRVM_PRIi "() (invalid!)", val->function);
		break;
	case ev_field:
		def = PRVM_ED_FieldAtOfs ( prog, val->_int );
		if (def != NULL)
			dpsnprintf (line, linelength, ".%s", PRVM_GetString(prog, def->s_name));
		else
			dpsnprintf (line, linelength, "field %" PRVM_PRIi " (invalid!)", val->_int );
		break;
	case ev_void:
		dpsnprintf (line, linelength, "void");
		break;
	case ev_float:
		// LadyHavoc: changed from %5.1f to %10.4f
		dpsnprintf (line, linelength, PRVM_FLOAT_LOSSLESS_FORMAT, val->_float);
		break;
	case ev_vector:
		// LadyHavoc: changed from %5.1f to %10.4f
		dpsnprintf (line, linelength, "'" PRVM_VECTOR_LOSSLESS_FORMAT "'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		dpsnprintf (line, linelength, "pointer");
		break;
	default:
		dpsnprintf (line, linelength, "bad type %d", (int) type);
		break;
	}

	return line;
}

/*
============
PRVM_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char *PRVM_UglyValueString (prvm_prog_t *prog, etype_t type, prvm_eval_t *val, char *line, size_t linelength)
{
	int i;
	const char *s;
	mdef_t *def;
	mfunction_t *f;

	type = (etype_t)((int)type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		// Parse the string a bit to turn special characters
		// (like newline, specifically) into escape codes,
		// this fixes saving games from various mods
		s = PRVM_GetString (prog, val->string);

		// Baker: We want to eventually see if we can get the engine
		// to reliably save what it loads.
		// I am not sure it does.
		// "wad" "\editing\hammer\nar\apocrypha.wad;\editing\hammer\nar\armagon.wad;\editing
		// \hammer\nar\bchess.wad;\editing\hammer\nar\dk"
		for (i = 0;i < (int)linelength - 2 && *s;) {
			if (*s == '\n')
			{
				line[i++] = '\\';
				line[i++] = 'n';
			}
			else if (*s == '\r')
			{
				line[i++] = '\\';
				line[i++] = 'r';
			}
			// Baker: It is said this problematic.  SLASHX
			// However, on read how would it discover newlines in a string otherwise?
			// But also does it work currently with newlines?
			else if (*s == '\\')
			{
				line[i++] = '\\';
				line[i++] = '\\';
			}
			else if (*s == '"')
			{
				line[i++] = '\\';
				line[i++] = '"';
			}
			else
				line[i++] = *s;
			s++;
		}
		line[i] = '\0';
		break;
	case ev_entity:
		i = val->edict;
		dpsnprintf (line, linelength, "%d", i);
		break;
	case ev_function:
		if ((unsigned int)val->function < (unsigned int)prog->progs_numfunctions)
		{
			f = prog->functions + val->function;
			strlcpy (line, PRVM_GetString (prog, f->s_name), linelength);
		}
		else
			dpsnprintf (line, linelength, "bad function %" PRVM_PRIi " (invalid!)", val->function);
		break;
	case ev_field:
		def = PRVM_ED_FieldAtOfs ( prog, val->_int );
		if (def != NULL)
			dpsnprintf (line, linelength, ".%s", PRVM_GetString(prog, def->s_name));
		else
			dpsnprintf (line, linelength, "field %" PRVM_PRIi "(invalid!)", val->_int );
		break;
	case ev_void:
		dpsnprintf (line, linelength, "void");
		break;
	case ev_float:
		dpsnprintf (line, linelength, PRVM_FLOAT_LOSSLESS_FORMAT, val->_float);
		break;
	case ev_vector:
		dpsnprintf (line, linelength, PRVM_VECTOR_LOSSLESS_FORMAT, val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		dpsnprintf (line, linelength, "bad type %d", type);
		break;
	}

	return line;
}

/*
============
PRVM_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PRVM_GlobalString (prvm_prog_t *prog, int ofs, char *line, size_t linelength)
{
	char	*s;
	//size_t	i;
	mdef_t	*def;
	prvm_eval_t	*val;
	char valuebuf[MAX_INPUTLINE_16384];

	val = (prvm_eval_t *)&prog->globals.fp[ofs];
	def = PRVM_ED_GlobalAtOfs(prog, ofs);
	if (!def)
		dpsnprintf (line, linelength, "GLOBAL%d", ofs);
	else
	{
		s = PRVM_ValueString (prog, (etype_t)def->type, val, valuebuf, sizeof(valuebuf));
		dpsnprintf (line, linelength, "%s (=%s)", PRVM_GetString(prog, def->s_name), s);
	}

	//i = strlen(line);
	//for ( ; i<20 ; i++)
	//	strcat (line," ");
	//strcat (line," ");

	return line;
}

char *PRVM_GlobalStringNoContents (prvm_prog_t *prog, int ofs, char *line, size_t linelength)
{
	//size_t	i;
	mdef_t	*def;

	def = PRVM_ED_GlobalAtOfs(prog, ofs);
	if (!def)
		dpsnprintf (line, linelength, "GLOBAL%d", ofs);
	else
		dpsnprintf (line, linelength, "%s", PRVM_GetString(prog, def->s_name));

	//i = strlen(line);
	//for ( ; i<20 ; i++)
	//	strcat (line," ");
	//strcat (line," ");

	return line;
}


/*
=============
PRVM_ED_Print

For debugging
=============
*/
// LadyHavoc: optimized this to print out much more quickly (tempstring)
// LadyHavoc: changed to print out every 4096 characters (incase there are a lot of fields to print)
// Baker: shall_print_free means print "free" if the edict is free.
WARP_X_ (PRVM_ED_PrintEdict_SV_f PRVM_ED_PrintNum)
void PRVM_ED_Print(prvm_prog_t *prog, prvm_edict_t *ed, int shall_print_free, 
				   const char *wildcard_fieldname, 
				   const char *s_fieldname_partial, const char *s_fieldvalue_partial, stringlist_t *plist)
{
	size_t	slen;
	mdef_t	*d;
	prvm_eval_t	*val;
	int		n, j;
	const char	*name;
	int		type;
	char	tempstring[MAX_INPUTLINE_16384], tempstring2[260]; // temporary string buffers
	char	valuebuf[MAX_INPUTLINE_16384];
	
	if (ed->free) {
		if (shall_print_free) {
			//if (plist == NULL) // Baker: October 18 2024
			Con_PrintLinef ("%s: FREE", prog->name);
		}
		return;
	}

#if 1 // Baker: The match field/value I added.  This isn't part of print
	int ismatch = false;

	if (s_fieldname_partial && s_fieldname_partial[0]) {
		tempstring[0] = 0;
		for (n = 1; n < prog->numfielddefs; n ++) {
			d = &prog->fielddefs[n];
			name = PRVM_GetString(prog, d->s_name);
			if (strlen(name) > 1 && name[strlen(name)-2] == '_' 
				&& (name[strlen(name)-1] == 'x' 
				|| name[strlen(name)-1] == 'y' || name[strlen(name)-1] == 'z'))
				continue;	// skip _x, _y, _z vars

			val = (prvm_eval_t *)(ed->fields.fp + d->ofs);

			// if the value is still all 0, skip the field
			type = d->type & ~DEF_SAVEGLOBAL;

			for (j = 0; j < prvm_type_size[type]; j ++) {
				if (val->ivector[j])
					break;
			} // j

			if (j == prvm_type_size[type])
				continue;

			if (String_Contains (name, s_fieldname_partial /*"targetname"*/)) {
				name = PRVM_ValueString(prog, (etype_t)d->type, val, valuebuf, sizeof(valuebuf));

				if (strlen(name) > sizeof(tempstring2)-4) {
					memcpy (tempstring2, name, sizeof(tempstring2)-4);
					tempstring2[sizeof(tempstring2)-4] = tempstring2[sizeof(tempstring2)-3] = tempstring2[sizeof(tempstring2)-2] = '.';
					tempstring2[sizeof(tempstring2)-1] = 0;
					name = tempstring2;
				}
				c_strlcat(tempstring, name);
				if (String_Contains (tempstring, s_fieldvalue_partial) == false) {
					// disqual
					//return;

					// continue ...
				} else {
					ismatch = true;
				}
			} // if
		} // for n
		if (!ismatch)
			return;
	} // targetname match
#endif

	// Baker: name is name
	tempstring[0] = 0;
	dpsnprintf(tempstring, sizeof(tempstring), "\n%s EDICT %d:\n", prog->name, PRVM_NUM_FOR_EDICT(ed));

#if 1 // October 18 2024
	if (plist)
		stringlistappendf (plist, "%s EDICT %d", prog->name, PRVM_NUM_FOR_EDICT(ed));
#endif

	for (n = 1; n < prog->numfielddefs; n ++)
	{
		d = &prog->fielddefs[n];
		name = PRVM_GetString(prog, d->s_name);
		if (strlen(name) > 1 && name[strlen(name)-2] == '_' && (name[strlen(name)-1] == 'x' || name[strlen(name)-1] == 'y' || name[strlen(name)-1] == 'z'))
			continue;	// skip _x, _y, _z vars

		// Check Field Name Wildcard
		if (wildcard_fieldname)
			if ( !matchpattern(name, wildcard_fieldname, 1) )
				// Didn't match; skip
				continue;

		val = (prvm_eval_t *)(ed->fields.fp + d->ofs);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;

		for (j=0 ; j<prvm_type_size[type] ; j++)
			if (val->ivector[j])
				break;
		if (j == prvm_type_size[type])
			continue;

		if (strlen(name) > sizeof(tempstring2)-4) {
			memcpy (tempstring2, name, sizeof(tempstring2)-4);
			tempstring2[sizeof(tempstring2)-4] = tempstring2[sizeof(tempstring2)-3] = tempstring2[sizeof(tempstring2)-2] = '.';
			tempstring2[sizeof(tempstring2)-1] = 0;
			name = tempstring2;
		}
		strlcat(tempstring, name, sizeof(tempstring));
		if (plist)
			stringlistappend (plist, name); // Baker: October 18 2024 - "Name"

		// Baker: space pad to size 14 as far as I can tell.  Then add 1 space.  Then print value.
		for (slen = strlen(name);slen < 14;slen++)
			strlcat(tempstring, " ", sizeof(tempstring));
		strlcat(tempstring, " ", sizeof(tempstring));

		// Baker: name re-used as value string here.
		name = PRVM_ValueString(prog, (etype_t)d->type, val, valuebuf, sizeof(valuebuf));
		if (strlen(name) > sizeof(tempstring2)-4)
		{
			memcpy (tempstring2, name, sizeof(tempstring2)-4);
			tempstring2[sizeof(tempstring2)-4] = tempstring2[sizeof(tempstring2)-3] = tempstring2[sizeof(tempstring2)-2] = '.';
			tempstring2[sizeof(tempstring2)-1] = 0;
			name = tempstring2;
		}		

		strlcat(tempstring, name, sizeof(tempstring));

		if (plist) {
			qbool Edict_Value_Fixup_Did_Change (ccs *key, char *valuebuf, size_t valuebuf_size);
			char valuebuf[MAX_INPUTLINE_16384];
			c_strlcpy (valuebuf, name);

			char *sold = stringlist_lastp(plist);
			qbool did_replace = Edict_Value_Fixup_Did_Change (/*key*/ sold, /*value*/ valuebuf, sizeof(valuebuf));
			char *sa = Z_StrDupf ("%-14.14s %s", sold, valuebuf);
			Mem_FreeNull_ (sold); // Free then replace ...
			stringlist_lastp(plist)=sa; // Replace
			//stringlistappend (plist, name /*Baker: but is really the value*/); // Baker: October 18 2024 - "Value"
		}
			
		strlcat(tempstring, "\n", sizeof(tempstring));
		if (strlen(tempstring) >= sizeof(tempstring)/2) {
			if (plist == NULL) // Baker: October 18 2024
			Con_Print(tempstring);
			tempstring[0] = 0;
		}
	}
	if (tempstring[0])
		if (plist == NULL) // Baker: October 18 2024
			Con_Print(tempstring);
}

/*
=============
PRVM_ED_Write

For savegames
=============
*/

// Baker: if f == NULL --> use "Baker strcat" instead
WARP_X_ (PRVM_ED_WriteGlobals)
WARP_X_CALLERS_ (SV_Savegame_to)
void PRVM_ED_Write (prvm_prog_t *prog, qfile_t *f, prvm_edict_t *ed)
{
	mdef_t	*d;
	prvm_eval_t	*val;
	int		i, j;
	const char	*name;
	int		type;
	char vabuf[1024];
	char valuebuf[MAX_INPUTLINE_16384];

	Flex_Writef ("{" NEWLINE);

	if (ed->free) {
		Flex_Writef ("}" NEWLINE);
		return;
	}

	for (i = 1; i < prog->numfielddefs; i++)
	{
		d = &prog->fielddefs[i];
		name = PRVM_GetString(prog, d->s_name);

		if (developer_entityparsing.integer)
			Con_PrintLinef ("PRVM_ED_Write: at entity %d field %s", PRVM_NUM_FOR_EDICT(ed), name);

		//if (strlen(name) > 1 && name[strlen(name)-2] == '_' && (name[strlen(name)-1] == 'x' || name[strlen(name)-1] == 'y' || name[strlen(name)-1] == 'z'))
		if (strlen(name) > 1 && name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars, and ALSO other _? vars as some mods expect them to be never saved (TODO: a gameplayfix for using the "more precise" condition above?)

		val = (prvm_eval_t *)(ed->fields.fp + d->ofs);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j = 0; j < prvm_type_size[type]; j ++)
			if (val->ivector[j])
				break;
		if (j == prvm_type_size[type])
			continue;

		Flex_Writef (QUOTED_S " ", name);
		prog->statestring = va(vabuf, sizeof(vabuf), "PRVM_ED_Write, ent=%d, name=%s", i, name);
		Flex_Writef (QUOTED_S NEWLINE, PRVM_UglyValueString(prog, (etype_t)d->type, val, valuebuf, sizeof(valuebuf)));
		prog->statestring = NULL;
	}

	Flex_Writef ("}" NEWLINE);
}

void PRVM_ED_PrintNum (prvm_prog_t *prog, int ent, int shall_print_free, const char *wildcard_fieldname, const char *s_fieldname_partial, const char *s_fieldvalue_partial, stringlist_t *plist)
{
	PRVM_ED_Print (prog, PRVM_EDICT_NUM(ent), shall_print_free, wildcard_fieldname, s_fieldname_partial, s_fieldvalue_partial, plist);
}

/*
=============
PRVM_ED_PrintEdicts_f

For debugging, prints all the entities in the current server
=============
*/

// "prvm_edicts"
void PRVM_ED_PrintEdicts_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	int		i;
	const char *wildcard_fieldname;

	if (Cmd_Argc(cmd) < 2 || Cmd_Argc(cmd) > 3)
	{
		Con_PrintLinef ("prvm_edicts <program name> <optional field name wildcard>");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	if ( Cmd_Argc(cmd) == 3)
		wildcard_fieldname = Cmd_Argv(cmd, 2);
	else
		wildcard_fieldname = NULL;

	Con_PrintLinef ("%s: %d entities", prog->name, prog->num_edicts);
	for (i=0 ; i<prog->num_edicts ; i++)
		PRVM_ED_PrintNum (prog, i, q_vm_printfree_true, wildcard_fieldname,
			q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
}


void PRVM_ED_PrintEdicts_Either (cmd_state_t *cmd, prvm_prog_t *prog)
{
	int		edict_num;
	const char *wildcard_fieldname = NULL;
	const char *s_fieldname_partial = "";
	const char *s_fieldvalue_partial = "";

	if (Cmd_Argc(cmd) == 3) {
		// gcc unused ... const char *s_arg1 = Cmd_Argv(cmd, 1);
		//int isnumeric = isdigit(s_arg1[0]);
		s_fieldname_partial = Cmd_Argv(cmd, 1);
		s_fieldvalue_partial = Cmd_Argv(cmd, 2);
	}
	else if (Cmd_Argc(cmd) == 2) {
		s_fieldname_partial = "classname";
		s_fieldvalue_partial = Cmd_Argv(cmd, 1);
	}

	if (Cmd_Argc(cmd) == 3) {
		Con_PrintLinef ("%s: entities with field " QUOTED_S " containing " QUOTED_S,
			prog->name, s_fieldname_partial, s_fieldvalue_partial);
	} else if (Cmd_Argc(cmd) == 2) {
		Con_PrintLinef ("%s: entities with classname containing " QUOTED_S, prog->name, s_fieldvalue_partial);
	} else {
		Con_PrintLinef ("%s: %d entities", prog->name, prog->num_edicts);
	}

	// Baker: Any search criteria and we don't print "free"
	int shall_print_free = !s_fieldname_partial || s_fieldname_partial[0] == NULL_CHAR_0;

	for (edict_num = 0 ; edict_num < prog->num_edicts ; edict_num ++)
		PRVM_ED_PrintNum (prog, edict_num, shall_print_free, wildcard_fieldname, s_fieldname_partial, s_fieldvalue_partial, q_stringlist_NULL);
}

// "edicts"
void PRVM_ED_PrintEdicts_SV_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if (!(prog = PRVM_FriendlyProgFromString("server")))
		return;

	PRVM_ED_PrintEdicts_Either (cmd, prog);
}

// "csedicts"
void PRVM_ED_PrintEdicts_CL_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if (!(prog = PRVM_FriendlyProgFromString("client")))
		return;

	PRVM_ED_PrintEdicts_Either (cmd, prog);
}

/*
=============
PRVM_ED_PrintEdict_f

For debugging, prints a single edict
=============
*/

// "prvm_edict"
static void PRVM_ED_PrintEdict_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	int		edict_num;
	const char	*wildcard_fieldname;

	if (Cmd_Argc(cmd) < 3 || Cmd_Argc(cmd) > 4)
	{
		Con_Print("prvm_edict <program name> <edict number> <optional field name wildcard>\n");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	edict_num = atoi (Cmd_Argv(cmd, 2));
	if (edict_num >= prog->num_edicts)
	{
		Con_PrintLinef ("Bad edict number");
		return;
	}
	if ( Cmd_Argc(cmd) == 4)
		// Optional Wildcard Provided
		wildcard_fieldname = Cmd_Argv(cmd, 3);
	else
		// Use All
		wildcard_fieldname = NULL;
	PRVM_ED_PrintNum (prog, edict_num, q_vm_printfree_true, wildcard_fieldname, q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
}

WARP_X_ (PRVM_ED_Print)


#define	CON_COLOR_SOMETHING "^x88e"
#define CON_GRAY_50 "^3"
static qbool BitAtoms_Did_Change (char *valuebuf, size_t valuebuf_size)
{
	WARP_X_ (FS_BitAtomize_f)
	double dnumber = atof(valuebuf);
	uint64_t number = dnumber;

	// bust down to bits and then print
	int count = 0;
	for (uint64_t j = 0; j < 32; j ++) {
		uint64_t thisbit = 1 << j;
		uint64_t icommon = (number & thisbit);
		if (icommon)
			count ++;
	} // for

	// Don't change if count is <= 0
	if (count <= 0)
		return false;

	char s_work[16384];
	c_strlcpy (s_work, "(");
	
	for (uint64_t j = 0; j < 32; j ++) {
		uint64_t thisbit = 1 << j;
		uint64_t icommon = (number & thisbit);
		//Con_PrintLinef ("%02d %16.0f %16.0f %x", (int)j, (double)(miney), (double)(number & miney), (unsigned)(number & miney));
		if (!icommon) continue;

		// Add a space if we haven't done so
		if (s_work[1] != NULL_CHAR_0)
			c_strlcat (s_work, " + ");

		va_super (sthisbit,256,"%u",(uint32_t)(number & icommon));
		c_strlcat (s_work, sthisbit);
	} // for
	c_strlcat (s_work, ")");
	
	strlcat (valuebuf, " " CON_GRAY_50, valuebuf_size);
	strlcat (valuebuf, s_work, valuebuf_size);
	strlcat (valuebuf, CON_WHITE, valuebuf_size);
	return true;
}

static qbool Replacer_Did_Change (char *valuebuf, size_t valuebuf_size, ccs **pkeys, size_t keys_count)
{
	char s_work[16384];
	size_t idx = atoi (valuebuf);
	if (in_range_beyond (0, idx, keys_count) == false)
		return false; // Not in range
	ccs *s_enumeration_name = pkeys[idx];

	c_dpsnprintf2 (s_work, "%s "CON_GRAY_50 "(%s)" CON_WHITE, valuebuf, s_enumeration_name);
	strlcpy (valuebuf, s_work, valuebuf_size);
	return true;
}

// colormod
static qbool Colorizer_Did_Change (char *valuebuf, size_t valuebuf_size)
{
	ccs *data = valuebuf;
	//colormod       '0.378733814 0.378733814 0.378733814'
	if (data[0] == CHAR_SQUOTE_39)
		data ++;
	vec3_t vcolor = {0,0,0};
	COM_Parse_Basic(&data);	vcolor[0] = atof(com_token);
	COM_Parse_Basic(&data); vcolor[1] = atof(com_token);
	COM_Parse_Basic(&data);	vcolor[2] = atof(com_token);

	

	ccs *colorcode_za = Color_Code_ZAlloc_Or_Null(vcolor);
	if (!colorcode_za)
		return false; // No change
	
	va_super (colorstamp, 256, " (%sCOLOR" CON_WHITE")", colorcode_za);
	strlcat  (valuebuf, colorstamp, valuebuf_size);
	Mem_FreeNull_ (colorcode_za);
	return true;
}

qbool Edict_Value_Fixup_Did_Change (ccs *key, char *valuebuf, size_t valuebuf_size)
{	
	if (String_Match(key, "solid")) { // EASY
		ccs *solids[] = {
			"solid_not", 
			"solid_trigger", 
			"solid_bbox", 
			"solid_slidebox", 
			"solid_bsp", 
			"solid_corpse", 
		};
		
		return Replacer_Did_Change (valuebuf, valuebuf_size, solids, ARRAY_COUNT(solids));
	}
	
	if (String_Match(key, "movetype")) { // EASY
		ccs *movetypes[] = {
			"movetype_none",
			"movetype_anglenoclip",
			"movetype_angleclip",
			"movetype_walk",
			"movetype_step",
			"movetype_fly",
			"movetype_toss",
			"movetype_push",
			"movetype_noclip",
			"movetype_flymissile",
			"movetype_bounce",
			"movetype_bouncemissile",
		};
		
		return Replacer_Did_Change (valuebuf, valuebuf_size, movetypes, ARRAY_COUNT(movetypes));
	}
	if (String_Isin1(key, "colormod")) {
		return Colorizer_Did_Change(valuebuf, valuebuf_size);
	}
	if (String_Isin3(key, "flags", "spawnflags", "items")) {
		//ccs *fl_flags[] = {
		//	"FL_FLY",
		//	"FL_FLY",
		//	float	FL_SWIM_2				= 2;
		//	float	FL_CLIENT_8				= 8;	// set for all client edicts
		//	float	FL_INWATER_16			= 16;	// for enter / leave water splash
		//	float	FL_MONSTER_32			= 32;
		//	float	FL_GODMODE				= 64;	// player cheat
		//	float	FL_NOTARGET				= 128;	// player cheat
		//	float	FL_ITEM_256				= 256;	// extra wide size for bonus items
		//											// to make items easier to pick up and allow them to be grabbed off
		//											// of shelves, the abs sizes are expanded 15 x/y and 1 on z in engine
		//											// Baker: we are going to use FL_ITEM_256 as the definitive "touch is pickup" flag.
		//											// With the exception maybe of coins. See SPAWNFLAGS_THIN_TOUCH_BOX_524288



		//	float	FL_ONGROUND_512				= 512;	// standing on something
		//	float	FL_PARTIALGROUND		= 1024;	// not all corners are valid
		//	float	FL_WATERJUMP			= 2048;	// player jumping out of water
		//	float	FL_JUMPRELEASED			= 4096;	// for jump debouncing
		//};

		return BitAtoms_Did_Change (valuebuf, valuebuf_size);
	}
	

	return false;
	//if (String_Match(key,"movetype")) // EASY
	//if (String_Match(key,"flags")) // WORTHY
//	if (String_Match(key,"items")) // MEH
//		WARP_X_ (FS_BitAtomize_f)
//	if (String_Match(key,"waterlevel"))

}

static void PRVM_ED_PrintEdict_SV_f(cmd_state_t *cmd)
{
	stringlist_t list = {0};

	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("edict <edict number>");
		return;
	}

	prvm_prog_t *prog;
	if (!(prog = PRVM_FriendlyProgFromString("server")))
		return;

	int		edict_num = atoi (Cmd_Argv(cmd, 1));

	if (edict_num >= prog->num_edicts) {
		Con_PrintLinef ("Bad edict number");
		return;
	}

	if (0) {
		PRVM_ED_PrintNum (prog, edict_num, q_vm_printfree_true, q_wildcard_fieldname_null, 
			q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
		goto no_sort;
	}
	
sorted_print:

	
	WARP_X_ (PRVM_ED_Print)
	PRVM_ED_PrintNum (prog, edict_num, q_vm_printfree_true, q_wildcard_fieldname_null, q_vm_classname_NULL, q_vm_targetname_NULL, &list);
	// Baker: If was "free" it printed and we do nothing here
	
	if (list.numstrings == 0)
		goto no_strings_to_print;

	// PRESORT - the first line make it have a leading space because it is "server EDICT 84"
	// And needs to be on top of the sort.
	char chold = list.strings[0][0];
	list.strings[0][0] = SPACE_CHAR_32;
	
	
	// SORT
	stringlistsort (&list, fs_make_unique_false);

	list.strings[0][0] = chold; // restore " erver EDICT 84" => "server EDICT 84"

	// PRINT EACH
	for (int idx = 0; idx < list.numstrings; idx ++) {
		ccs *sxy = list.strings[idx];
		Con_PrintLinef ("%s", sxy);
	}


no_strings_to_print:
no_sort:

	stringlistfreecontents (&list);
}

// "edict 11"
static void PRVM_ED_PrintEdict_CL_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	int		edict_num;
	const char	*wildcard_fieldname = NULL;

	if (Cmd_Argc(cmd) < 2) {
		Con_PrintLinef ("csedict <edict number>");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString("client")))
		return;

	edict_num = atoi (Cmd_Argv(cmd, 1));
	if (edict_num >= prog->num_edicts) {
		Con_PrintLinef ("Bad edict number");
		return;
	}
	PRVM_ED_PrintNum (prog, edict_num, q_vm_printfree_true, wildcard_fieldname,
		q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);  // Baker r7101: edicts with criteria
}

/*
=============
PRVM_ED_Count

For debugging
=============
*/
// 2 possibilities : 1. just displaying the active edict count
//					 2. making a function pointer [x]
static void PRVM_ED_Count_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("prvm_count <program name>\n");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	prog->count_edicts(prog);
}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
PRVM_ED_WriteGlobals
=============
*/
// If f == NULL, uses "bakerstring" method.
// Baker: If f is NULL .. write to string buffer.
WARP_X_ (SV_Savegame_to PRVM_ED_Write)
void PRVM_ED_WriteGlobals (prvm_prog_t *prog, qfile_t *f)
{
	mdef_t		*def;
	int			i;
	const char		*name;
	int			type;
	char vabuf[1024];
	char valuebuf[MAX_INPUTLINE_16384];

	Flex_Writef ("{" NEWLINE);
	for (i = 0;i < prog->numglobaldefs;i++)
	{
		def = &prog->globaldefs[i];
		type = def->type;
		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string && type != ev_float && type != ev_entity)
			continue;

		name = PRVM_GetString(prog, def->s_name);

		if (developer_entityparsing.integer)
			Con_PrintLinef ("PRVM_ED_WriteGlobals: at global %s", name);

		prog->statestring = va(vabuf, sizeof(vabuf), "PRVM_ED_WriteGlobals, name=%s", name);
		Flex_Writef (QUOTED_S " ", name);
		Flex_Writef (QUOTED_S NEWLINE, PRVM_UglyValueString(prog, (etype_t)type, (prvm_eval_t *)&prog->globals.fp[def->ofs], valuebuf, sizeof(valuebuf)));
		prog->statestring = NULL;
	}
	Flex_Writef ("}" NEWLINE);
}

/*
=============
PRVM_ED_ParseGlobals
=============
*/
void PRVM_ED_ParseGlobals (prvm_prog_t *prog, const char *data)
{
	char keyname[MAX_INPUTLINE_16384];
	mdef_t *key;

	while (1)
	{
		// parse key
		if (!COM_ParseToken_Simple(&data, false, false, true))
			prog->error_cmd("PRVM_ED_ParseGlobals: EOF without closing brace");
		if (com_token[0] == '}')
			break;

		if (developer_entityparsing.integer)
			Con_PrintLinef ("Key: " QUOTED_S, com_token);

		c_strlcpy (keyname, com_token);

		// parse value
		if (!COM_ParseToken_Simple(&data, false, true, true))
			prog->error_cmd("PRVM_ED_ParseGlobals: EOF without closing brace");

		if (developer_entityparsing.integer)
			Con_PrintLinef (" " QUOTED_S, com_token);

		if (com_token[0] == '}')
			prog->error_cmd("PRVM_ED_ParseGlobals: closing brace without data");

		key = PRVM_ED_FindGlobal (prog, keyname);
		if (!key)
		{
			Con_DPrintLinef ("'%s' is not a global on %s", keyname, prog->name);
			continue;
		}

		if (!PRVM_ED_ParseEpair(prog, NULL, key, com_token, true))
			prog->error_cmd("PRVM_ED_ParseGlobals: parse error");
	}
}

//============================================================================


/*
=============
PRVM_ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
qbool PRVM_ED_ParseEpair(prvm_prog_t *prog, prvm_edict_t *ent, mdef_t *key, const char *s, qbool parsebackslash)
{
	int i, l;
	char *new_p;
	mdef_t *def;
	prvm_eval_t *val;
	mfunction_t *func;

	if (ent)
		val = (prvm_eval_t *)(ent->fields.fp + key->ofs);
	else
		val = (prvm_eval_t *)(prog->globals.fp + key->ofs);
	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		l = (int)strlen(s) + 1;
		val->string = PRVM_AllocString(prog, l, &new_p);

		// Baker: map wicked in something wicked single player
		// Baker: We want to eventually see if we can get the engine
		// to reliably save what it loads.
		// I am not sure it does.
		// "wad" "\editing\hammer\nar\apocrypha.wad;\editing\hammer\nar\armagon.wad;\editing
		// \hammer\nar\bchess.wad;\editing\hammer\nar\dk"
		for (i = 0;i < l;i++)
		{
			if (s[i] == '\\' && s[i+1] && parsebackslash)
			{
				i++;
				if (s[i] == 'n')
					*new_p++ = '\n';
				else if (s[i] == 'r')
					*new_p++ = '\r';
				else
					*new_p++ = s[i];
			}
			else
				*new_p++ = s[i];
		}
		break;

	case ev_float:
		while (*s && ISWHITESPACE(*s))
			s++;
		val->_float = atof(s);
		break;

	case ev_vector:
		for (i = 0;i < 3;i++)
		{
			while (*s && ISWHITESPACE(*s))
				s++;
			if (!*s)
				break;
			val->vector[i] = atof(s);
			while (!ISWHITESPACE(*s))
				s++;
			if (!*s)
				break;
		}
		break;

	case ev_entity:
		while (*s && ISWHITESPACE(*s))
			s++;
		i = atoi(s);
		if (i >= prog->limit_edicts)
			Con_Printf ("PRVM_ED_ParseEpair: ev_entity reference too large (edict %u >= MAX_EDICTS_32768 %u) on %s\n", (unsigned int)i, prog->limit_edicts, prog->name);
		while (i >= prog->max_edicts)
			PRVM_MEM_IncreaseEdicts(prog);
		// if IncreaseEdicts was called the base pointer needs to be updated
		if (ent)
			val = (prvm_eval_t *)(ent->fields.fp + key->ofs);
		val->edict = PRVM_EDICT_TO_PROG(PRVM_EDICT_NUM((int)i));
		break;

	case ev_field:
		if (*s != '.')
		{
			Con_DPrintf ("PRVM_ED_ParseEpair: Bogus field name %s in %s\n", s, prog->name);
			return false;
		}
		def = PRVM_ED_FindField(prog, s + 1);
		if (!def)
		{
			Con_DPrintf ("PRVM_ED_ParseEpair: Can't find field %s in %s\n", s, prog->name);
			return false;
		}
		val->_int = def->ofs;
		break;

	case ev_function:
		func = PRVM_ED_FindFunction(prog, s);
		if (!func)
		{
			Con_Printf ("PRVM_ED_ParseEpair: Can't find function %s in %s\n", s, prog->name);
			return false;
		}
		val->function = func - prog->functions;
		break;

	default:
		Con_Printf ("PRVM_ED_ParseEpair: Unknown key->type %d for key " QUOTED_S " on %s\n", key->type, PRVM_GetString(prog, key->s_name), prog->name);
		return false;
	}
	return true;
}

/*
=============
PRVM_GameCommand_f

Console command to send a string to QC function GameCommand of the
indicated progs

Usage:
  sv_cmd adminmsg 3 "do not teamkill"
  cl_cmd someclientcommand
  menu_cmd somemenucommand

All progs can support this extension; sg calls it in server QC, cg in client
QC, mg in menu QC.
=============
*/
static void PRVM_GameCommand(cmd_state_t *cmd, const char *whichprogs, const char *whichcmd)
{
	prvm_prog_t *prog;
	if (Cmd_Argc(cmd) < 1)
	{
		Con_PrintLinef ("%s text...", whichcmd);
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(whichprogs)))
		return;

	if (!PRVM_allfunction(GameCommand))
	{
		Con_PrintLinef ("%s program do not support GameCommand!", whichprogs);
	}
	else
	{
		int restorevm_tempstringsbuf_cursize;
		const char *s;

		s = Cmd_Args(cmd);

		restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;
		PRVM_G_INT(OFS_PARM0) = PRVM_SetTempString(prog, s ? s : "");
		prog->ExecuteProgram(prog, PRVM_allfunction(GameCommand), "QC function GameCommand is missing");
		prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
	}
}
static void PRVM_GameCommand_Server_f(cmd_state_t *cmd)
{
	PRVM_GameCommand(cmd, "server", "sv_cmd");
}
static void PRVM_GameCommand_Client_f(cmd_state_t *cmd)
{
	PRVM_GameCommand(cmd, "client", "cl_cmd");
}
static void PRVM_GameCommand_Menu_f(cmd_state_t *cmd)
{
	PRVM_GameCommand(cmd, "menu", "menu_cmd");
}

/*
=============
PRVM_ED_EdictGet_f

Console command to load a field of a specified edict
=============
*/
static void PRVM_ED_EdictGet_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	prvm_edict_t *ed;
	mdef_t *key;
	const char *s;
	prvm_eval_t *v;
	char valuebuf[MAX_INPUTLINE_16384];

	if (Cmd_Argc(cmd) != 4 && Cmd_Argc(cmd) != 5) {
		//               0              1              2              3       4
		Con_PrintLinef ("prvm_edictget <program name> <edict number> <field> [<cvar>]");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	ed = PRVM_EDICT_NUM(atoi(Cmd_Argv(cmd, 2)));

	if ((key = PRVM_ED_FindField(prog, Cmd_Argv(cmd, 3))) == 0) {
		Con_PrintLinef ("Key %s not found !", Cmd_Argv(cmd, 3));
		goto fail;
	}

	v = (prvm_eval_t *)(ed->fields.fp + key->ofs);
	s = PRVM_UglyValueString(prog, (etype_t)key->type, v, valuebuf, sizeof(valuebuf));

	// Baker: This does what?
	if (Cmd_Argc(cmd) == 5) {
		cvar_t *cvar = Cvar_FindVar(cmd->cvars, Cmd_Argv(cmd, 4), cmd->cvars_flagsmask);
		if (cvar)
			if (Cvar_Readonly(cvar, "prvm_edictget"))
				goto fail;

		Cvar_Get (cmd->cvars, Cmd_Argv(cmd, 4), s, cmd->cvars_flagsmask, NULL);
	}
	else {
		Con_PrintLinef ("%s", s);
		if (prvm_edictget_autostore.integer) {
			Cvar_SetQuick (&_last, s);
			if (prvm_edictget_autostore.integer >= 2) {
				Clipboard_Set_Text (s);
				Con_PrintLinef ("Value set to clipboard");
			}
		} // autostore
	}

fail:
	;
}

static void PRVM_ED_GlobalGet_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	mdef_t *key;
	const char *s;
	prvm_eval_t *v;
	char valuebuf[MAX_INPUTLINE_16384];

	if (Cmd_Argc(cmd) != 3 && Cmd_Argc(cmd) != 4) {
		Con_PrintLinef ("prvm_globalget <program name> <global> [<cvar>]");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	key = PRVM_ED_FindGlobal(prog, Cmd_Argv(cmd, 2));
	if (!key) {
		Con_PrintLinef ( "No global '%s' in %s!", Cmd_Argv(cmd, 2), Cmd_Argv(cmd, 1) );
		goto fail;
	}

	v = (prvm_eval_t *) &prog->globals.fp[key->ofs];
	s = PRVM_UglyValueString(prog, (etype_t)key->type, v, valuebuf, sizeof(valuebuf));
	if (Cmd_Argc(cmd) == 4) {
		cvar_t *cvar = Cvar_FindVar(cmd->cvars, Cmd_Argv(cmd, 3), cmd->cvars_flagsmask);
		if (cvar)
			if (Cvar_Readonly(cvar, "prvm_globalget"))
				goto fail;
		Cvar_Get(cmd->cvars, Cmd_Argv(cmd, 3), s, cmd->cvars_flagsmask, NULL);
	}
	else {
		Con_PrintLinef ("%s", s);
		if (prvm_edictget_autostore.integer) {
			Cvar_SetQuick (&_last, s);
			if (prvm_edictget_autostore.integer >= 2) {
				Clipboard_Set_Text (s);
				Con_PrintLinef ("Value set to clipboard");
			}
		} // autostore
	}

fail:
	;
}

//  0    1  2    3
// eset 40 scale 0.4 // 4
// eset 40 scale // 3
static void PRVM_ED_Eset_f(cmd_state_t *cmd)
{
	char val_3plus[1024];
	prvm_prog_t *prog = SVVM_prog;
	int is_get = false;
	int argc = Cmd_Argc(cmd);

	if (argc > 4) {
		int low_arg_num = 3; // 3 to ... cumulate
		baker_string_t *bsa = BakerString_Create_Malloc ("");
		for (int j = low_arg_num; j < Cmd_Argc(cmd); j ++) {
			ccs *sxy = Cmd_Argv (cmd, j);
			if (j > 1)
				BakerString_CatC (bsa, " ");
			BakerString_CatC (bsa, sxy);
		}
		c_strlcpy (val_3plus, bsa->string);
		BakerString_Destroy_And_Null_It (&bsa);
		Con_PrintLinef ("Accumulated value string = " QUOTED_S, val_3plus);
	}
	else if (argc == 4) {
		c_strlcpy (val_3plus, Cmd_Argv(cmd, 3));
	} else if (argc == 3) {		
		// Become get
		is_get = true;
	} else if (argc == 2) {		
		// Become "edict"
		stringlist_t list = {0};
		int		edict_num = atoi (Cmd_Argv(cmd, 1));

		if (edict_num >= prog->num_edicts || edict_num < 0) {
			Con_PrintLinef ("Bad edict number");
			return;
		}
		
sorted_print:

		PRVM_ED_PrintNum (prog, edict_num, q_vm_printfree_true, q_wildcard_fieldname_null, q_vm_classname_NULL, q_vm_targetname_NULL, &list);

		// Baker: If was "free" it printed and we do nothing here		
		if (list.numstrings == 0)
			goto no_strings_to_print;

		// PRESORT - the first line make it have a leading space because it is "server EDICT 84"
		// And needs to be on top of the sort.
		char chold = list.strings[0][0];
		list.strings[0][0] = SPACE_CHAR_32;
		
		// SORT
		stringlistsort (&list, fs_make_unique_false);

		list.strings[0][0] = chold; // restore " erver EDICT 84" => "server EDICT 84"

		// PRINT EACH
		for (int idx = 0; idx < list.numstrings; idx ++) {
			ccs *sxy = list.strings[idx];
			Con_PrintLinef ("%s", sxy);
		}

no_strings_to_print:
no_sort:

		stringlistfreecontents (&list);
		return;
	} else {
		Con_PrintLinef ("eset <edict number> <field> <value>");
		return;
	}

	int edict_num = atoi(Cmd_Argv(cmd, 1));

	if (edict_num >= prog->num_edicts || edict_num < 0) {
		Con_PrintLinef ("Bad edict number %d > num_edicts %d", edict_num, prog->num_edicts);
		return;
	}

	prvm_edict_t	*ed_1	= PRVM_EDICT_NUM(edict_num);

	mdef_t			*key_2	= PRVM_ED_FindField(prog, Cmd_Argv(cmd, 2));

	if (key_2 == NULL) {
		Con_PrintLinef ("Key %s not found!", Cmd_Argv(cmd, 2));
		return;
	}

	if (is_get) { // Baker: EDICT GET instead
		// Do edict get		
		char valuebuf[MAX_INPUTLINE_16384];

		prvm_eval_t *v	= (prvm_eval_t *)(ed_1->fields.fp + key_2->ofs);
		const char	*s	= PRVM_UglyValueString(prog, (etype_t)key_2->type, v, valuebuf, sizeof(valuebuf));
					
		Con_PrintLinef ("%s", s);
		if (prvm_edictget_autostore.integer) {
			Cvar_SetQuick (&_last, s);
			if (prvm_edictget_autostore.integer >= 2) {
				Clipboard_Set_Text (s);
				Con_PrintLinef ("Value set to clipboard");
			}
		} // autostore

		return;
	}
	
	PRVM_ED_ParseEpair(prog, ed_1, key_2, val_3plus, qp_parse_backslash_true);
}

/*
=============
PRVM_ED_EdictSet_f

Console command to set a field of a specified edict
=============
*/
static void PRVM_ED_EdictSet_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	prvm_edict_t *ed;
	mdef_t *key;
	
#if 1
	if (Cmd_Argc(cmd) == 4) { // Baker: Turn into prvm_edictget with 4 params
		PRVM_ED_EdictGet_f (cmd);
		return;
	}
#endif

	if (Cmd_Argc(cmd) != 5) {
		Con_PrintLinef ("prvm_edictset <program name> <edict number> <field> <value>");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	ed = PRVM_EDICT_NUM(atoi(Cmd_Argv(cmd, 2)));

	if ((key = PRVM_ED_FindField(prog, Cmd_Argv(cmd, 3))) == 0)
		Con_PrintLinef ("Key %s not found!", Cmd_Argv(cmd, 3));
	else
		PRVM_ED_ParseEpair(prog, ed, key, Cmd_Argv(cmd, 4), true);
}

/*
====================
PRVM_ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *PRVM_ED_ParseEdict (prvm_prog_t *prog, const char *data, prvm_edict_t *ent, qbool is_saveload)
{
	mdef_t *key;
	qbool anglehack;
	qbool init;
	char keyname[256];
	size_t n;

	init = false;

// go through all the dictionary pairs
	while (1)
	{
	// parse key
		//if (!COM_ParseToken_Simple(&data, false, false, true))
		//if (!COM_ParseToken_Simple(&data, false, is_saveload, true)) // ITK #6
		if (!COM_ParseToken_Simple(&data, false, false, true))
			prog->error_cmd("PRVM_ED_ParseEdict: EOF without closing brace");
		if (developer_entityparsing.integer)
			Con_Printf ("Key: " QUOTED_S, com_token);
		if (com_token[0] == '}')
			break;

		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if (String_Match(com_token, "angle"))
		{
			strlcpy (com_token, "angles", sizeof(com_token));
			anglehack = true;
		}
		else
			anglehack = false;

		// FIXME: change light to _light to get rid of this hack
		if (String_Match(com_token, "light"))
			strlcpy (com_token, "light_lev", sizeof(com_token));	// hack for single light def

		strlcpy (keyname, com_token, sizeof(keyname));

		// another hack to fix keynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value
		//if (!COM_ParseToken_Simple(&data, false, false, true))
		if (!COM_ParseToken_Simple(&data, false, is_saveload, true)) // ITK #X
			prog->error_cmd("PRVM_ED_ParseEdict: EOF without closing brace");
		if (developer_entityparsing.integer)
			Con_PrintLinef (" " QUOTED_S, com_token);

		if (com_token[0] == '}')
			prog->error_cmd("PRVM_ED_ParseEdict: closing brace without data");

		init = true;

		// ignore attempts to set key "" (this problem occurs in nehahra neh1m8.bsp)
		if (!keyname[0])
			continue;

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		key = PRVM_ED_FindField (prog, keyname);
		if (!key) {
			Con_DPrintLinef ("%s: '%s' is not a field", prog->name, keyname);
			continue;
		}

		if (anglehack)
		{
			char	temp[32];
			c_strlcpy (temp, com_token);
			c_dpsnprintf1 (com_token, "0 %s 0", temp);
		}

		if (!PRVM_ED_ParseEpair(prog, ent, key, com_token, strcmp(keyname, "wad") != 0))
			prog->error_cmd("PRVM_ED_ParseEdict: parse error");
	}

	if (!init) {
		ent->free = true;
		ent->freetime = host.realtime;
	}

	return data;
}

void PRVM_ED_CallPrespawnFunction(prvm_prog_t *prog, prvm_edict_t *ent)
{
	if (PRVM_serverfunction(SV_OnEntityPreSpawnFunction))
	{
		// self = ent
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SV_OnEntityPreSpawnFunction), "QC function SV_OnEntityPreSpawnFunction is missing");
	}
}

qbool PRVM_ED_CallSpawnFunction(prvm_prog_t *prog, prvm_edict_t *ent, const char *data, const char *start)
{
	const char *funcname;
	mfunction_t *func;
	prvm_eval_t *fulldata = NULL;
	char vabuf[1024];

//
// immediately call spawn function, but only if there is a self global and a classname
//
	if (!ent->free)
	{
		if (!PRVM_alledictstring(ent, classname))
		{
			Con_PrintLinef ("No classname for:");
			PRVM_ED_Print(prog, ent, q_vm_printfree_true, q_vm_wildcard_NULL, q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
			PRVM_ED_Free (prog, ent);
			return false;
		}
		/*
		 * This is required for FTE compatibility (FreeCS).
		 * It copies the key/value pairs themselves into a
		 * global for QC to parse on its own.
		 */
		else if (data && start)
		{
			if ((fulldata = PRVM_ED_FindGlobalEval(prog, "__fullspawndata")))
			{
				const char *in;
				char *spawndata;
				fulldata->string = PRVM_AllocString(prog, data - start + 1, &spawndata);
				for(in = start; in < data; )
				{
					char c = *in++;
					if (c == '\n')
						*spawndata++ = '\t';
					else
						*spawndata++ = c;
				}
				*spawndata = 0;
			}
		}

		// look for the spawn function
		funcname = PRVM_GetString(prog, PRVM_alledictstring(ent, classname));
		func = PRVM_ED_FindFunction (prog, va(vabuf, sizeof(vabuf), "spawnfunc_%s", funcname));
		if (!func)
			if (!PRVM_allglobalfloat(require_spawnfunc_prefix))
				func = PRVM_ED_FindFunction (prog, funcname);

		if (!func)
		{
			// check for OnEntityNoSpawnFunction
			if (PRVM_serverfunction(SV_OnEntityNoSpawnFunction))
			{
				// self = ent
				PRVM_serverglobalfloat(time) = sv.time;
				PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
				prog->ExecuteProgram(prog, PRVM_serverfunction(SV_OnEntityNoSpawnFunction), "QC function SV_OnEntityNoSpawnFunction is missing");
			}
			else
			{
				//                                                              1 = ALWAYS  : 0 = NEVER
				qbool shall_player_print = developer_spawnfunction_warnings.integer ? true : false;
				if (developer_spawnfunction_warnings.integer == -3) {
					// War
					qbool is_non_quake1 = sv_mapformat_is_quake2.integer ||
						sv_mapformat_is_quake3.integer || halflifebsp.integer;
					//                                   Q3      Q1
					shall_player_print = is_non_quake1 ? false : true;
				}
				if (shall_player_print) {
					Con_PrintLinef (CON_WARN "No spawn function for:");
					//if (developer.integer > 0) // don't confuse non-developers with errors
						PRVM_ED_Print(prog, ent, q_vm_printfree_true, q_vm_wildcard_NULL,
						q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
				} else {
					Con_DPrintLinef (CON_WARN "No spawn function for:");
					if (developer.integer > 0) // don't confuse non-developers with errors
						PRVM_ED_Print(prog, ent, q_vm_printfree_true, q_vm_wildcard_NULL,
						q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
				}
				PRVM_ED_Free (prog, ent);
				return false; // not included in "inhibited" count
			}
		}
		else
		{
			// self = ent
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_allglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
			prog->ExecuteProgram(prog, func - prog->functions, "");
		}
		return true;
	}
	PRVM_ED_Free(prog, ent);
	return false;
}

void PRVM_ED_CallPostspawnFunction (prvm_prog_t *prog, prvm_edict_t *ent)
{
	if (!ent->free)
	if (PRVM_serverfunction(SV_OnEntityPostSpawnFunction))
	{
		// self = ent
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SV_OnEntityPostSpawnFunction), "QC function SV_OnEntityPostSpawnFunction is missing");
	}
}

/*
================
PRVM_ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
PRVM_ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call PRVM_ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void PRVM_ED_LoadFromFile (prvm_prog_t *prog, const char *data)
{
	prvm_edict_t *ent;
	const char *start;
	int parsed, inhibited, spawned, died;

	parsed = 0;
	inhibited = 0;
	spawned = 0;
	died = 0;

	prvm_reuseedicts_always_allow = host.realtime;

	// parse ents
	while (1)
	{
		start = data;

		// parse the opening brace
		if (!COM_ParseToken_Simple(&data, false, false, true))
			break;
		if (com_token[0] != '{')
			prog->error_cmd("PRVM_ED_LoadFromFile: %s: found %s when expecting {", prog->name, com_token);

		// CHANGED: this is not conform to PR_LoadFromFile
		if (prog->loadintoworld)
		{
			prog->loadintoworld = false;
			ent = PRVM_EDICT_NUM(0);
		}
		else
			ent = PRVM_ED_Alloc(prog);

		// clear it
		if (ent != prog->edicts)	// hack
			memset (ent->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));

		data = PRVM_ED_ParseEdict (prog, data, ent, q_is_saveload_false); // ITK #3
		parsed++;

		// remove the entity ?
		if (!prog->load_edict(prog, ent))
		{
			PRVM_ED_Free(prog, ent);
			inhibited++;
			continue;
		}

		PRVM_ED_CallPrespawnFunction(prog, ent);

		if (ent->free)
		{
			inhibited++;
			continue;
		}

		SV_LinkEdict(ent);

		if (!PRVM_ED_CallSpawnFunction(prog, ent, data, start))
			continue;

		PRVM_ED_CallPostspawnFunction(prog, ent);

		spawned++;
		if (ent->free)
			died++;
	}

	Con_DPrintf ("%s: %d new entities parsed, %d new inhibited, %d (%d new) spawned (whereas %d removed self, %d stayed)\n", prog->name, parsed, inhibited, prog->num_edicts, spawned, died, spawned - died);

	prvm_reuseedicts_always_allow = 0;
}

static void PRVM_FindOffsets(prvm_prog_t *prog)
{
	// field and global searches use -1 for NULL
	memset(&prog->fieldoffsets, -1, sizeof(prog->fieldoffsets));
	memset(&prog->globaloffsets, -1, sizeof(prog->globaloffsets));
	// function searches use 0 for NULL
	memset(&prog->funcoffsets, 0, sizeof(prog->funcoffsets));
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x) prog->fieldoffsets.x = PRVM_ED_FindFieldOffset(prog, #x);
#define PRVM_DECLARE_global(x) prog->globaloffsets.x = PRVM_ED_FindGlobalOffset(prog, #x);
#define PRVM_DECLARE_function(x) prog->funcoffsets.x = PRVM_ED_FindFunctionOffset(prog, #x);
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
}

// not used
/*
typedef struct dpfield_s
{
	int type;
	char *string;
}
dpfield_t;

#define DPFIELDS (sizeof(dpfields) / sizeof(dpfield_t))

dpfield_t dpfields[] =
{
};
*/

/*
===============
PRVM_ResetProg
===============
*/

#define PO_HASHSIZE 16384
typedef struct po_string_s
{
	char *key, *value;
	struct po_string_s *nextonhashchain;
}
po_string_t;
typedef struct po_s
{
	po_string_t *hashtable[PO_HASHSIZE];
}
po_t;
static void PRVM_PO_UnparseString(char *out, const char *in, size_t outsize)
{
	for(;;)
	{
		switch(*in)
		{
			case 0:
				*out++ = 0;
				return;
			case '\a': if (outsize >= 2) { *out++ = '\\'; *out++ = 'a'; outsize -= 2; } break;
			case '\b': if (outsize >= 2) { *out++ = '\\'; *out++ = 'b'; outsize -= 2; } break;
			case '\t': if (outsize >= 2) { *out++ = '\\'; *out++ = 't'; outsize -= 2; } break;
			case '\r': if (outsize >= 2) { *out++ = '\\'; *out++ = 'r'; outsize -= 2; } break;
			case '\n': if (outsize >= 2) { *out++ = '\\'; *out++ = 'n'; outsize -= 2; } break;
			case '\\': if (outsize >= 2) { *out++ = '\\'; *out++ = '\\'; outsize -= 2; } break;
			case '"': if (outsize >= 2) { *out++ = '\\'; *out++ = '"'; outsize -= 2; } break;
			default:
				if (*in >= 0 && *in <= 0x1F)
				{
					if (outsize >= 4)
					{
						*out++ = '\\';
						*out++ = '0' + ((*in & 0700) >> 6);
						*out++ = '0' + ((*in & 0070) >> 3);
						*out++ = '0' +  (*in & 0007)      ;
						outsize -= 4;
					}
				}
				else
				{
					if (outsize >= 1)
					{
						*out++ = *in;
						outsize -= 1;
					}
				}
				break;
		}
		++in;
	}
}
static void PRVM_PO_ParseString(char *out, const char *in, size_t outsize)
{
	for(;;)
	{
		switch(*in)
		{
			case 0:
				*out++ = 0;
				return;
			case '\\':
				++in;
				switch(*in)
				{
					case 'a': if (outsize > 0) { *out++ = '\a'; --outsize; } break;
					case 'b': if (outsize > 0) { *out++ = '\b'; --outsize; } break;
					case 't': if (outsize > 0) { *out++ = '\t'; --outsize; } break;
					case 'r': if (outsize > 0) { *out++ = '\r'; --outsize; } break;
					case 'n': if (outsize > 0) { *out++ = '\n'; --outsize; } break;
					case '\\': if (outsize > 0) { *out++ = '\\'; --outsize; } break;
					case '"': if (outsize > 0) { *out++ = '"'; --outsize; } break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
						if (outsize > 0)
							*out = *in - '0';
						++in;
						if (*in >= '0' && *in <= '7')
						{
							if (outsize > 0)
								*out = (*out << 3) | (*in - '0');
							++in;
						}
						if (*in >= '0' && *in <= '7')
						{
							if (outsize > 0)
								*out = (*out << 3) | (*in - '0');
							++in;
						}
						--in;
						if (outsize > 0)
						{
							++out;
							--outsize;
						}
						break;
					default:
						if (outsize > 0) { *out++ = *in; --outsize; }
						break;
				}
				break;
			default:
				if (outsize > 0)
				{
					*out++ = *in;
					--outsize;
				}
				break;
		}
		++in;
	}
}
static po_t *PRVM_PO_Load(const char *filename, const char *filename2, mempool_t *pool)
{
	po_t *po = NULL;
	const char *p, *q;
	int mode;
	char inbuf[MAX_INPUTLINE_16384];
	char decodedbuf[MAX_INPUTLINE_16384];
	size_t decodedpos;
	int hashindex;
	po_string_t thisstr;
	int i;

	for (i = 0; i < 2; ++i)
	{
		const char *buf = (const char *)
			FS_LoadFile((i > 0 ? filename : filename2), pool, fs_quiet_true, fs_size_ptr_null);
		// first read filename2, then read filename
		// so that progs.dat.de.po wins over common.de.po
		// and within file, last item wins

		if (!buf)
			continue;

		if (!po)
		{
			po = (po_t *)Mem_Alloc(pool, sizeof(*po));
			memset(po, 0, sizeof(*po));
		}

		memset(&thisstr, 0, sizeof(thisstr)); // hush compiler warning

		p = buf;
		while(*p)
		{
			if (*p == '#')
			{
				// skip to newline
				p = strchr(p, '\n');
				if (!p)
					break;
				++p;
				continue;
			}
			if (*p == '\r' || *p == '\n')
			{
				++p;
				continue;
			}
			if (!strncmp(p, "msgid \"", 7))
			{
				mode = 0;
				p += 6;
			}
			else if (!strncmp(p, "msgstr \"", 8))
			{
				mode = 1;
				p += 7;
			}
			else
			{
				p = strchr(p, '\n');
				if (!p)
					break;
				++p;
				continue;
			}
			decodedpos = 0;
			while(*p == '"')
			{
				++p;
				q = strchr(p, '\n');
				if (!q)
					break;
				if (*(q-1) == '\r')
					--q;
				if (*(q-1) != '"')
					break;
				if ((size_t)(q - p) >= (size_t) sizeof(inbuf))
					break;
				strlcpy(inbuf, p, q - p); // not - 1, because this adds a NUL
				PRVM_PO_ParseString(decodedbuf + decodedpos, inbuf, sizeof(decodedbuf) - decodedpos);
				decodedpos += strlen(decodedbuf + decodedpos);
				if (*q == '\r')
					++q;
				if (*q == '\n')
					++q;
				p = q;
			}
			if (mode == 0)
			{
				if (thisstr.key)
					Mem_Free(thisstr.key);
				thisstr.key = (char *)Mem_Alloc(pool, decodedpos + 1);
				memcpy(thisstr.key, decodedbuf, decodedpos + 1);
			}
			else if (decodedpos > 0 && thisstr.key) // skip empty translation results
			{
				thisstr.value = (char *)Mem_Alloc(pool, decodedpos + 1);
				memcpy(thisstr.value, decodedbuf, decodedpos + 1);
				hashindex = CRC_Block((const unsigned char *) thisstr.key, strlen(thisstr.key)) % PO_HASHSIZE;
				thisstr.nextonhashchain = po->hashtable[hashindex];
				po->hashtable[hashindex] = (po_string_t *)Mem_Alloc(pool, sizeof(thisstr));
				memcpy(po->hashtable[hashindex], &thisstr, sizeof(thisstr));
				memset(&thisstr, 0, sizeof(thisstr));
			}
		}

		Mem_Free((char *) buf);
	}

	return po;
}
static const char *PRVM_PO_Lookup(po_t *po, const char *str)
{
	int hashindex = CRC_Block((const unsigned char *) str, strlen(str)) % PO_HASHSIZE;
	po_string_t *p = po->hashtable[hashindex];
	while(p)
	{
		if (String_Match(str, p->key))
			return p->value;
		p = p->nextonhashchain;
	}
	return NULL;
}
static void PRVM_PO_Destroy(po_t *po)
{
	int i;
	for(i = 0; i < PO_HASHSIZE; ++i)
	{
		po_string_t *p = po->hashtable[i];
		while(p)
		{
			po_string_t *q = p;
			p = p->nextonhashchain;
			Mem_Free(q->key);
			Mem_Free(q->value);
			Mem_Free(q);
		}
	}
	Mem_Free(po);
}

void PRVM_LeakTest(prvm_prog_t *prog);
void PRVM_Prog_Reset(prvm_prog_t *prog)
{
	if (prog->loaded)
	{
		if (prog->tempstringsbuf.cursize)
			Mem_Free(prog->tempstringsbuf.data);
		prog->tempstringsbuf.cursize = 0;
		PRVM_LeakTest(prog);
		prog->reset_cmd(prog);
		Mem_FreePool(&prog->progs_mempool);
		if (prog->po)
			PRVM_PO_Destroy((po_t *) prog->po);
	}
	memset(prog,0,sizeof(prvm_prog_t));
	prog->break_statement = -1;
	prog->watch_global_type = ev_void;
	prog->watch_field_type = ev_void;
}

/*
===============
PRVM_LoadLNO
===============
*/
static void PRVM_LoadLNO( prvm_prog_t *prog, const char *progname ) {
	fs_offset_t filesize;
	unsigned char *lno;
	unsigned int *header;
	char filename[512];

	FS_StripExtension( progname, filename, sizeof( filename ) );
	strlcat( filename, ".lno", sizeof( filename ) );

	lno = FS_LoadFile( filename, tempmempool, fs_quiet_FALSE, &filesize );
	if ( !lno ) {
		return;
	}

/*
<Spike>    SafeWrite (h, &lnotype, sizeof(int));
<Spike>    SafeWrite (h, &version, sizeof(int));
<Spike>    SafeWrite (h, &numglobaldefs, sizeof(int));
<Spike>    SafeWrite (h, &numpr_globals, sizeof(int));
<Spike>    SafeWrite (h, &numfielddefs, sizeof(int));
<Spike>    SafeWrite (h, &numstatements, sizeof(int));
<Spike>    SafeWrite (h, statement_linenums, numstatements*sizeof(int));
*/
	if ((unsigned int)filesize < (6 + prog->progs_numstatements) * sizeof(int))
	{
		Mem_Free(lno);
		return;
	}

	header = (unsigned int *) lno;
	if ( header[ 0 ] == *(unsigned int *) "LNOF" &&
		LittleLong( header[ 1 ] ) == 1 &&
		(unsigned int)LittleLong( header[ 2 ] ) == (unsigned int)prog->progs_numglobaldefs &&
		(unsigned int)LittleLong( header[ 3 ] ) == (unsigned int)prog->progs_numglobals &&
		(unsigned int)LittleLong( header[ 4 ] ) == (unsigned int)prog->progs_numfielddefs &&
		(unsigned int)LittleLong( header[ 5 ] ) == (unsigned int)prog->progs_numstatements )
	{
		prog->statement_linenums = (int *)Mem_Alloc(prog->progs_mempool, prog->progs_numstatements * sizeof( int ) );
		memcpy( prog->statement_linenums, header + 6, prog->progs_numstatements * sizeof( int ) );

		/* gmqcc suports columnums */
		if ((unsigned int)filesize > ((6 + 2 * prog->progs_numstatements) * sizeof( int )))
		{
			prog->statement_columnnums = (int *)Mem_Alloc(prog->progs_mempool, prog->progs_numstatements * sizeof( int ) );
			memcpy( prog->statement_columnnums, header + 6 + prog->progs_numstatements, prog->progs_numstatements * sizeof( int ) );
		}
	}
	Mem_Free( lno );
}

/*
===============
PRVM_LoadProgs
===============
*/
static void PRVM_UpdateBreakpoints(prvm_prog_t *prog);
// Baker r1414 - automatic unload of fte csqc-lite csprogs.dat
const char *PRVM_Prog_Load(prvm_prog_t *prog, const char * filename, unsigned char * data, fs_offset_t size, int numrequiredfunc, const char **required_func, int numrequiredfields, prvm_required_field_t *required_field, int numrequiredglobals, prvm_required_field_t *required_global)
{
	int i;
	dprograms_t *dprograms;

	dstatement16_t *instatements16;
	dstatement32_t *instatements32;
	ddef16_t *infielddefs16;
	ddef32_t *infielddefs32;
	ddef16_t *inglobaldefs16;
	ddef32_t *inglobaldefs32;

	int *inglobals;
	dfunction_t *infunctions;
	char *instrings;
	fs_offset_t filesize;
	int requiredglobalspace;
	opcode_t op;
	int a;
	int b;
	int c;
	union
	{
		unsigned int i;
		float f;
	}
	u;
	unsigned int d;
	char vabuf[1024];
	char vabuf2[1024];
	cvar_t *cvar;
	int structtype = 0;

	if (prog->loaded)
		prog->error_cmd("PRVM_LoadProgs: there is already a %s program loaded!", prog->name );

	Host_LockSession(); // all progs can use the session cvar
	Crypto_LoadKeys(); // all progs might use the keys at init time

	if (data)
	{
		dprograms = (dprograms_t *) data;
		filesize = size;
	}
	else
		dprograms = (dprograms_t *)FS_LoadFile (filename, prog->progs_mempool, fs_quiet_FALSE, &filesize);
	if (dprograms == NULL || filesize < (fs_offset_t)sizeof(dprograms_t))
		prog->error_cmd("PRVM_LoadProgs: couldn't load %s for %s", filename, prog->name);
	// TODO bounds check header fields (e.g. numstatements), they must never go behind end of file

	prog->profiletime = Sys_DirtyTime();
	prog->starttime = host.realtime;

	requiredglobalspace = 0;
	for (i = 0;i < numrequiredglobals;i++)
		requiredglobalspace += required_global[i].type == ev_vector ? 3 : 1;

	prog->filecrc = CRC_Block((unsigned char *)dprograms, filesize);

// byte swap the header
	prog->progs_version = LittleLong(dprograms->version);
	prog->progs_crc = LittleLong(dprograms->crc);
	if (prog->progs_version == PROG_VERSION_FTE_7) {
		dprograms_v7_t *v7 = (dprograms_v7_t*)dprograms;
		structtype = LittleLong(v7->secondaryversion);
		if (structtype == PROG_SECONDARYVERSION16 ||
			structtype == PROG_SECONDARYVERSION32) // barely supported
			Con_PrintLinef (CON_WARN "WARNING: %s: %s targets FTEQW, for which support is incomplete. Proceed at your own risk.", prog->name, filename);
		else
			prog->error_cmd("%s: %s targets unknown engine", prog->name, filename);

		if (v7->numbodylessfuncs != 0 || v7->numtypes != 0 || v7->blockscompressed != 0)
			prog->error_cmd("%s: %s uses unsupported features.", prog->name, filename);
	}
	else if (prog->progs_version != PROG_VERSION_6)
		prog->error_cmd("%s: %s has wrong version number (%d should be %d)", prog->name, filename, prog->progs_version, PROG_VERSION_6);
	instatements16 = (dstatement16_t *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_statements));
	instatements32 = (dstatement32_t *)instatements16;
	prog->progs_numstatements = LittleLong(dprograms->numstatements);
	inglobaldefs16 = (ddef16_t *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_globaldefs));
	inglobaldefs32 = (ddef32_t *)inglobaldefs16;
	prog->progs_numglobaldefs = LittleLong(dprograms->numglobaldefs);
	infielddefs16 = (ddef16_t *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_fielddefs));
	infielddefs32 = (ddef32_t *)infielddefs16;
	prog->progs_numfielddefs = LittleLong(dprograms->numfielddefs);
	infunctions = (dfunction_t *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_functions));
	prog->progs_numfunctions = LittleLong(dprograms->numfunctions);
	instrings = (char *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_strings));
	prog->progs_numstrings = LittleLong(dprograms->numstrings);
	inglobals = (int *)((unsigned char *)dprograms + LittleLong(dprograms->ofs_globals));
	prog->progs_numglobals = LittleLong(dprograms->numglobals);
	prog->progs_entityfields = LittleLong(dprograms->entityfields);

	prog->numstatements = prog->progs_numstatements;
	prog->numglobaldefs = prog->progs_numglobaldefs;
	prog->numfielddefs = prog->progs_numfielddefs;
	prog->numfunctions = prog->progs_numfunctions;
	prog->numstrings = prog->progs_numstrings;
	prog->numglobals = prog->progs_numglobals;
	prog->entityfields = prog->progs_entityfields;

	if (LittleLong(dprograms->ofs_strings) + prog->progs_numstrings > (int)filesize)
		prog->error_cmd("%s: %s strings go past end of file", prog->name, filename);
	prog->strings = (char *)Mem_Alloc(prog->progs_mempool, prog->progs_numstrings);
	memcpy(prog->strings, instrings, prog->progs_numstrings);
	prog->stringssize = prog->progs_numstrings;

	prog->numknownstrings = 0;
	prog->maxknownstrings = 0;
	prog->knownstrings = NULL;
	prog->knownstrings_flags = NULL;

	Mem_ExpandableArray_NewArray(&prog->stringbuffersarray, prog->progs_mempool, sizeof(prvm_stringbuffer_t), 64);

	// we need to expand the globaldefs and fielddefs to include engine defs
	prog->globaldefs = (mdef_t *)Mem_Alloc(prog->progs_mempool, (prog->progs_numglobaldefs + numrequiredglobals) * sizeof(mdef_t));
	prog->globals.fp = (prvm_vec_t *)Mem_Alloc(prog->progs_mempool, (prog->progs_numglobals + requiredglobalspace + 2) * sizeof(prvm_vec_t));
		// + 2 is because of an otherwise occurring overrun in RETURN instruction
		// when trying to return the last or second-last global
		// (RETURN always returns a vector, there is no RETURN_F instruction)
	prog->fielddefs = (mdef_t *)Mem_Alloc(prog->progs_mempool, (prog->progs_numfielddefs + numrequiredfields) * sizeof(mdef_t));
	// we need to convert the statements to our memory format
	prog->statements = (mstatement_t *)Mem_Alloc(prog->progs_mempool, prog->progs_numstatements * sizeof(mstatement_t));
	// allocate space for profiling statement usage
	prog->statement_profile = (double *)Mem_Alloc(prog->progs_mempool, prog->progs_numstatements * sizeof(*prog->statement_profile));
	prog->explicit_profile = (double *)Mem_Alloc(prog->progs_mempool, prog->progs_numstatements * sizeof(*prog->statement_profile));
	// functions need to be converted to the memory format
	prog->functions = (mfunction_t *)Mem_Alloc(prog->progs_mempool, sizeof(mfunction_t) * prog->progs_numfunctions);

	// Baker: Quakespasm equivalent sec 2
	for (i = 0;i < prog->progs_numfunctions;i++)
	{
		prog->functions[i].first_statement = LittleLong(infunctions[i].first_statement);
		prog->functions[i].parm_start = LittleLong(infunctions[i].parm_start);
		prog->functions[i].s_name = LittleLong(infunctions[i].s_name);
		prog->functions[i].s_file = LittleLong(infunctions[i].s_file);
		prog->functions[i].numparms = LittleLong(infunctions[i].numparms);
		prog->functions[i].locals = LittleLong(infunctions[i].locals);
		memcpy(prog->functions[i].parm_size, infunctions[i].parm_size, sizeof(infunctions[i].parm_size));
		if (prog->functions[i].first_statement >= prog->numstatements)
			prog->error_cmd("PRVM_LoadProgs: out of bounds function statement (function %d) in %s", i, prog->name);
		// TODO bounds check parm_start, s_name, s_file, numparms, locals, parm_size
	}

	// copy the globaldefs to the new globaldefs list
	switch(structtype)
	{
	case PROG_SECONDARYVERSION32:
		for (i=0 ; i<prog->numglobaldefs ; i++)
		{
			prog->globaldefs[i].type = LittleLong(inglobaldefs32[i].type);
			prog->globaldefs[i].ofs = LittleLong(inglobaldefs32[i].ofs);
			prog->globaldefs[i].s_name = LittleLong(inglobaldefs32[i].s_name);
			// TODO bounds check ofs, s_name
		}
		break;
	default:
		for (i=0 ; i<prog->numglobaldefs ; i++)
		{
			prog->globaldefs[i].type = (unsigned short)LittleShort(inglobaldefs16[i].type);
			prog->globaldefs[i].ofs = (unsigned short)LittleShort(inglobaldefs16[i].ofs);
			prog->globaldefs[i].s_name = LittleLong(inglobaldefs16[i].s_name);
			// TODO bounds check ofs, s_name
		}
		break;
	}

	// Baker: Mostly a DarkPlaces thing

	// append the required globals
	for (i = 0;i < numrequiredglobals;i++)
	{
		prog->globaldefs[prog->numglobaldefs].type = required_global[i].type;
		prog->globaldefs[prog->numglobaldefs].ofs = prog->numglobals;
		prog->globaldefs[prog->numglobaldefs].s_name = PRVM_SetEngineString(prog, required_global[i].name);
		if (prog->globaldefs[prog->numglobaldefs].type == ev_vector)
			prog->numglobals += 3;
		else
			prog->numglobals++;
		prog->numglobaldefs++;
	}

	// copy the progs fields to the new fields list
	switch(structtype)
	{
	case PROG_SECONDARYVERSION32:
		for (i = 0;i < prog->numfielddefs;i++)
		{
			prog->fielddefs[i].type = LittleLong(infielddefs32[i].type);
			if (prog->fielddefs[i].type & DEF_SAVEGLOBAL)
				prog->error_cmd("PRVM_LoadProgs: prog->fielddefs[i].type & DEF_SAVEGLOBAL in %s", prog->name);
			prog->fielddefs[i].ofs = LittleLong(infielddefs32[i].ofs);
			prog->fielddefs[i].s_name = LittleLong(infielddefs32[i].s_name);
			// TODO bounds check ofs, s_name
		}
		break;
	default:
		for (i = 0;i < prog->numfielddefs;i++)
		{
			prog->fielddefs[i].type = (unsigned short)LittleShort(infielddefs16[i].type);
			if (prog->fielddefs[i].type & DEF_SAVEGLOBAL)
				prog->error_cmd("PRVM_LoadProgs: prog->fielddefs[i].type & DEF_SAVEGLOBAL in %s", prog->name);
			prog->fielddefs[i].ofs = (unsigned short)LittleShort(infielddefs16[i].ofs);
			prog->fielddefs[i].s_name = LittleLong(infielddefs16[i].s_name);
			// TODO bounds check ofs, s_name
		}
		break;
	}

	// append the required fields
	for (i = 0;i < numrequiredfields;i++)
	{
		prog->fielddefs[prog->numfielddefs].type = required_field[i].type;
		prog->fielddefs[prog->numfielddefs].ofs = prog->entityfields;
		prog->fielddefs[prog->numfielddefs].s_name = PRVM_SetEngineString(prog, required_field[i].name);
		if (prog->fielddefs[prog->numfielddefs].type == ev_vector)
			prog->entityfields += 3;
		else
			prog->entityfields++;
		prog->numfielddefs++;
	}

	// LadyHavoc: TODO: reorder globals to match engine struct
	// LadyHavoc: TODO: reorder fields to match engine struct
#define remapglobal(index) (index)
#define remapfield(index) (index)

	// copy globals
	// FIXME: LadyHavoc: this uses a crude way to identify integer constants, rather than checking for matching globaldefs and checking their type
	for (i = 0;i < prog->progs_numglobals;i++)
	{
		u.i = LittleLong(inglobals[i]);
		// most globals are 0, we only need to deal with the ones that are not
		if (u.i)
		{
			d = u.i & 0xFF800000;
			if ((d == 0xFF800000) || (d == 0))
			{
				// Looks like an integer (expand to int64)
				prog->globals.ip[remapglobal(i)] = u.i;
			}
			else
			{
				// Looks like a float (expand to double)
				prog->globals.fp[remapglobal(i)] = u.f;
			}
		}
	}

	// copy, remap globals in statements, bounds check
	for (i = 0;i < prog->progs_numstatements;i++)
	{
		switch(structtype)
		{
		case PROG_SECONDARYVERSION32:
			op = (opcode_t)LittleLong(instatements32[i].op);
			a = (unsigned int)LittleLong(instatements32[i].a);
			b = (unsigned int)LittleLong(instatements32[i].b);
			c = (unsigned int)LittleLong(instatements32[i].c);
			break;
		default:
			op = (opcode_t)LittleShort(instatements16[i].op);
			a = (unsigned short)LittleShort(instatements16[i].a);
			b = (unsigned short)LittleShort(instatements16[i].b);
			c = (unsigned short)LittleShort(instatements16[i].c);
			break;
		}
		switch (op)
		{
		case OP_IF:
		case OP_IFNOT:
			b = (short)b;
			if (a >= prog->progs_numglobals || b + i < 0 || b + i >= prog->progs_numstatements)
				prog->error_cmd("PRVM_LoadProgs: out of bounds IF/IFNOT (statement %d) in %s", i, prog->name);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = remapglobal(a);
			prog->statements[i].operand[1] = -1;
			prog->statements[i].operand[2] = -1;
			prog->statements[i].jumpabsolute = i + b;
			break;
		case OP_GOTO:
			a = (short)a;
			if (a + i < 0 || a + i >= prog->progs_numstatements)
				prog->error_cmd("PRVM_LoadProgs: out of bounds GOTO (statement %d) in %s", i, prog->name);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = -1;
			prog->statements[i].operand[1] = -1;
			prog->statements[i].operand[2] = -1;
			prog->statements[i].jumpabsolute = i + a;
			break;
		default:
			Con_DPrintf ("PRVM_LoadProgs: unknown opcode %d at statement %d in %s\n", (int)op, i, prog->name);

			//make sure its something well defined.
			prog->statements[i].op = OP_BOUNDCHECK;
			prog->statements[i].operand[0] = 0;
			prog->statements[i].operand[1] =
			prog->statements[i].operand[2] = op;
			prog->statements[i].jumpabsolute = -1;
			break;
		case OP_STORE_I:
		case OP_ADD_I:
		case OP_ADD_FI:
		case OP_ADD_IF:
		case OP_SUB_I:
		case OP_SUB_FI:
		case OP_SUB_IF:
		case OP_CONV_IF:
		case OP_CONV_FI:
		case OP_LOAD_I:
		case OP_STOREP_I:
		case OP_BITAND_I:
		case OP_BITOR_I:
		case OP_MUL_I:
		case OP_DIV_I:
		case OP_EQ_I:
		case OP_NE_I:
		case OP_NOT_I:
		case OP_DIV_VF:
		case OP_STORE_P:
		case OP_LE_I:
		case OP_GE_I:
		case OP_LT_I:
		case OP_GT_I:
		case OP_LE_IF:
		case OP_GE_IF:
		case OP_LT_IF:
		case OP_GT_IF:
		case OP_LE_FI:
		case OP_GE_FI:
		case OP_LT_FI:
		case OP_GT_FI:
		case OP_EQ_IF:
		case OP_EQ_FI:
		case OP_MUL_IF:
		case OP_MUL_FI:
		case OP_MUL_VI:
		case OP_DIV_IF:
		case OP_DIV_FI:
		case OP_BITAND_IF:
		case OP_BITOR_IF:
		case OP_BITAND_FI:
		case OP_BITOR_FI:
		case OP_AND_I:
		case OP_OR_I:
		case OP_AND_IF:
		case OP_OR_IF:
		case OP_AND_FI:
		case OP_OR_FI:
		case OP_NE_IF:
		case OP_NE_FI:
		case OP_GSTOREP_I:
		case OP_GSTOREP_F:
		case OP_GSTOREP_ENT:
		case OP_GSTOREP_FLD:
		case OP_GSTOREP_S:
		case OP_GSTOREP_FNC:
		case OP_GSTOREP_V:
//		case OP_GADDRESS:
		case OP_GLOAD_I:
		case OP_GLOAD_F:
		case OP_GLOAD_FLD:
		case OP_GLOAD_ENT:
		case OP_GLOAD_S:
		case OP_GLOAD_FNC:
		case OP_BOUNDCHECK:
		case OP_GLOAD_V:

		// global global global
		case OP_ADD_F:
		case OP_ADD_V:
		case OP_SUB_F:
		case OP_SUB_V:
		case OP_MUL_F:
		case OP_MUL_V:
		case OP_MUL_FV:
		case OP_MUL_VF:
		case OP_DIV_F:
		case OP_BITAND:
		case OP_BITOR:
		case OP_GE:
		case OP_LE:
		case OP_GT:
		case OP_LT:
		case OP_AND:
		case OP_OR:
		case OP_EQ_F:
		case OP_EQ_V:
		case OP_EQ_S:
		case OP_EQ_E:
		case OP_EQ_FNC:
		case OP_NE_F:
		case OP_NE_V:
		case OP_NE_S:
		case OP_NE_E:
		case OP_NE_FNC:
		case OP_ADDRESS:
		case OP_LOAD_F:
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
		case OP_LOAD_V:
			if (a >= prog->progs_numglobals || b >= prog->progs_numglobals || c >= prog->progs_numglobals)
				prog->error_cmd("PRVM_LoadProgs: out of bounds global index (statement %d)", i);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = remapglobal(a);
			prog->statements[i].operand[1] = remapglobal(b);
			prog->statements[i].operand[2] = remapglobal(c);
			prog->statements[i].jumpabsolute = -1;
			break;
		// global none global
		case OP_NOT_F:
		case OP_NOT_V:
		case OP_NOT_S:
		case OP_NOT_FNC:
		case OP_NOT_ENT:
			if (a >= prog->progs_numglobals || c >= prog->progs_numglobals)
				prog->error_cmd("PRVM_LoadProgs: out of bounds global index (statement %d) in %s", i, prog->name);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = remapglobal(a);
			prog->statements[i].operand[1] = -1;
			prog->statements[i].operand[2] = remapglobal(c);
			prog->statements[i].jumpabsolute = -1;
			break;
		// 2 globals
		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:
		case OP_STOREP_S:
		case OP_STOREP_FNC:
			if (c)	//Spike -- DP is alergic to pointers in QC. Try to avoid too many nasty surprises.
				Con_DPrintf ("PRVM_LoadProgs: storep-with-offset is not permitted in %s\n", prog->name);
			//fallthrough
		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:
		case OP_STORE_S:
		case OP_STORE_FNC:
		case OP_STATE:
		case OP_STOREP_V:
		case OP_STORE_V:
			if (a >= prog->progs_numglobals || b >= prog->progs_numglobals)
				prog->error_cmd("PRVM_LoadProgs: out of bounds global index (statement %d) in %s", i, prog->name);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = remapglobal(a);
			prog->statements[i].operand[1] = remapglobal(b);
			prog->statements[i].operand[2] = -1;
			prog->statements[i].jumpabsolute = -1;
			break;
		// 1 global
		case OP_CALL0:
			if ( a < prog->progs_numglobals)
				if ( prog->globals.ip[remapglobal(a)] >= 0 )
					if ( prog->globals.ip[remapglobal(a)] < prog->progs_numfunctions )
						if ( prog->functions[prog->globals.ip[remapglobal(a)]].first_statement == -642 )
							++prog->numexplicitcoveragestatements;
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
		case OP_DONE:
		case OP_RETURN:
			if ( a >= prog->progs_numglobals)
				prog->error_cmd("PRVM_LoadProgs: out of bounds global index (statement %d) in %s", i, prog->name);
			if (b || c)	//Spike -- added this check just as a diagnostic...
				Con_DPrintf ("PRVM_LoadProgs: unexpected offset on call opcode in %s. Hexen2 format is not supported\n", prog->name);
			prog->statements[i].op = op;
			prog->statements[i].operand[0] = remapglobal(a);
			prog->statements[i].operand[1] = -1;
			prog->statements[i].operand[2] = -1;
			prog->statements[i].jumpabsolute = -1;
			break;
		}
	}
	if (prog->numstatements < 1)
	{
		prog->error_cmd("PRVM_LoadProgs: empty program in %s", prog->name);
	}
	else switch(prog->statements[prog->numstatements - 1].op)
	{
		case OP_RETURN:
		case OP_GOTO:
		case OP_DONE:
			break;
		default:
			prog->error_cmd("PRVM_LoadProgs: program may fall off the edge (does not end with RETURN, GOTO or DONE) in %s", prog->name);
			break;
	}

	// we're done with the file now
	if (!data)
		Mem_Free(dprograms);
	dprograms = NULL;

	// check required functions
	for(i=0 ; i < numrequiredfunc ; i++) {
		if (PRVM_ED_FindFunction(prog, required_func[i]) == 0) {
			return required_func[i]; // Baker r1414 - automatic unload of fte csqc-lite csprogs.dat
			// prog->error_cmd("%s: %s not found in %s",prog->name, required_func[i], filename);

		}
	}

	PRVM_LoadLNO(prog, filename);

	PRVM_Init_Exec(prog);

	if (*prvm_language.string)
	// in CSQC we really shouldn't be able to change how stuff works... sorry for now
	// later idea: include a list of authorized .po file checksums with the csprogs
	{
		qbool deftrans = prog == CLVM_prog;
		const char *realfilename = (prog != CLVM_prog ? filename : csqc_progname.string);
		if (deftrans) // once we have dotranslate_ strings, ALWAYS use the opt-in method!
		{
			for (i=0 ; i<prog->numglobaldefs ; i++)
			{
				const char *name;
				name = PRVM_GetString(prog, prog->globaldefs[i].s_name);
				if ((prog->globaldefs[i].type & ~DEF_SAVEGLOBAL) == ev_string)
				if (name && !strncmp(name, "dotranslate_", 12))
				{
					deftrans = false;
					break;
				}
			}
		}

		if (String_Match(prvm_language.string, "dump")) {
			qfile_t *f = FS_OpenRealFile(va(vabuf, sizeof(vabuf), "%s.pot", realfilename), "w", fs_quiet_FALSE); // WRITEON - prog dump (DONE)
			Con_PrintLinef ("Dumping to %s.pot", realfilename);
			if (f)
			{
				for (i=0 ; i<prog->numglobaldefs ; i++)
				{
					const char *name;
					name = PRVM_GetString(prog, prog->globaldefs[i].s_name);
					if (deftrans ? (!name || strncmp(name, "notranslate_", 12)) : (name && !strncmp(name, "dotranslate_", 12)))
					if ((prog->globaldefs[i].type & ~DEF_SAVEGLOBAL) == ev_string)
					{
						prvm_eval_t *val = PRVM_GLOBALFIELDVALUE(prog->globaldefs[i].ofs);
						const char *value = PRVM_GetString(prog, val->string);
						if (*value)
						{
							char buf[MAX_INPUTLINE_16384];
							PRVM_PO_UnparseString(buf, value, sizeof(buf));
							FS_Printf(f, "msgid " QUOTED_S "\nmsgstr \"\"\n\n", buf);
						}
					}
				}
				FS_Close(f);
			}
		}
		else
		{
			po_t *po = PRVM_PO_Load(
					va(vabuf, sizeof(vabuf), "%s.%s.po", realfilename, prvm_language.string),
					va(vabuf2, sizeof(vabuf2), "common.%s.po", prvm_language.string),
					prog->progs_mempool);
			if (po)
			{
				for (i=0 ; i<prog->numglobaldefs ; i++)
				{
					const char *name;
					name = PRVM_GetString(prog, prog->globaldefs[i].s_name);
					if (deftrans ? (!name || strncmp(name, "notranslate_", 12)) : (name && !strncmp(name, "dotranslate_", 12)))
					if ((prog->globaldefs[i].type & ~DEF_SAVEGLOBAL) == ev_string)
					{
						prvm_eval_t *val = PRVM_GLOBALFIELDVALUE(prog->globaldefs[i].ofs);
						const char *value = PRVM_GetString(prog, val->string);
						if (*value)
						{
							value = PRVM_PO_Lookup(po, value);
							if (value)
								val->string = PRVM_SetEngineString(prog, value);
						}
					}
				}
			}
		}
	}

	for (cvar = prog->console_cmd->cvars->vars; cvar; cvar = cvar->next)
		cvar->globaldefindex[prog - prvm_prog_list] = -1;

	for (i=0 ; i<prog->numglobaldefs ; i++)
	{
		const char *name;
		name = PRVM_GetString(prog, prog->globaldefs[i].s_name);
		//Con_Printf ("found var %s\n", name);
		if (name
			&& !strncmp(name, "autocvar_", 9)
			&& !(strlen(name) > 1 && name[strlen(name)-2] == '_' && 
			(name[strlen(name)-1] == 'x' || name[strlen(name)-1] == 'y' || name[strlen(name)-1] == 'z'))
		)
		{
			prvm_eval_t *val = PRVM_GLOBALFIELDVALUE(prog->globaldefs[i].ofs);
			cvar = Cvar_FindVar(prog->console_cmd->cvars, name + 9, prog->console_cmd->cvars_flagsmask);
			//Con_Printf ("PRVM_LoadProgs: autocvar global %s in %s, processing...\n", name, prog->name);
			if (!cvar)
			{
				const char *value;
				char buf[128];
				int prec[3];
				float f;
				Con_DPrintf ("PRVM_LoadProgs: no cvar for autocvar global %s in %s, creating...\n", name, prog->name);
				switch(prog->globaldefs[i].type & ~DEF_SAVEGLOBAL)
				{
					case ev_float:
						if ((float)((int)(val->_float)) == val->_float)
							dpsnprintf(buf, sizeof(buf), "%d", (int)(val->_float));
						else
						{
							// ftos_slow
							f = val->_float;
							for (int precision = 7; precision <= 9; ++precision) {
								dpsnprintf(buf, sizeof(buf), "%.*g", precision, f);
								if ((float)atof(buf) == f) {
									break;
								}
							}
						}
						value = buf;
						break;
					case ev_vector:
						for (i = 0; i < 3; ++i)
						{
							prec[i] = 9;
							f = val->vector[i];
							for (int precision = 7; precision <= 9; ++precision) {
								dpsnprintf(buf, sizeof(buf), "%.*g", precision, f);
								if ((float)atof(buf) == f) {
									prec[i] = precision;
									break;
								}
							}
						}
						dpsnprintf(buf, sizeof(buf), "%.*g %.*g %.*g", prec[0], val->vector[0], prec[1], val->vector[1], prec[2], val->vector[2]);
						value = buf;
						break;
					case ev_string:
						value = PRVM_GetString(prog, val->string);
						break;
					default:
						Con_Printf ("PRVM_LoadProgs: invalid type of autocvar global %s in %s\n", name, prog->name);
						goto fail;
				}
				// 9 is length of "autocvar_" prefix
				cvar = Cvar_Get(prog->console_cmd->cvars, name + 9, 
					value, prog->console_cmd->cvars_flagsmask, /*description*/ NULL);
				if ((prog->globaldefs[i].type & ~DEF_SAVEGLOBAL) == ev_string) {
					val->string = PRVM_SetEngineString(prog, cvar->string);
					cvar->globaldefindex_stringno[prog - prvm_prog_list] = val->string;
				}
				if (!cvar)
					prog->error_cmd("PRVM_LoadProgs: could not create cvar for autocvar global %s in %s", name, prog->name);
				cvar->globaldefindex[prog - prvm_prog_list] = i;
			}
			else if (Have_Flag(cvar->flags, CF_PRIVATE) == 0)
			{
				// MUST BE SYNCED WITH cvar.c Cvar_Set
				int j;
				const char *s;
				switch(prog->globaldefs[i].type & ~DEF_SAVEGLOBAL)
				{
					case ev_float:
						val->_float = cvar->value;
						break;
					case ev_vector:
						s = cvar->string;
						VectorClear(val->vector);
						for (j = 0;j < 3;j++)
						{
							while (*s && ISWHITESPACE(*s))
								s++;
							if (!*s)
								break;
							val->vector[j] = atof(s);
							while (!ISWHITESPACE(*s))
								s++;
							if (!*s)
								break;
						}
						break;
					case ev_string:
						val->string = PRVM_SetEngineString(prog, cvar->string);
						cvar->globaldefindex_stringno[prog - prvm_prog_list] = val->string;
						break;
					default:
						Con_Printf ("PRVM_LoadProgs: invalid type of autocvar global %s in %s\n", name, prog->name);
						goto fail;
				}
				cvar->globaldefindex[prog - prvm_prog_list] = i;
			}
			else
				Con_Printf ("PRVM_LoadProgs: private cvar for autocvar global %s in %s\n", name, prog->name);
		}
fail:
		;
	}

	SET___ prog->loaded = true;

	PRVM_UpdateBreakpoints(prog);

	// set flags & mdef_ts in prog

	prog->flag = 0;

	PRVM_FindOffsets(prog);

	prog->init_cmd(prog);

	// init mempools
	PRVM_MEM_Alloc(prog);

	Con_Printf ("%s: program loaded (crc %d, size %dK)\n", prog->name, prog->filecrc, (int)(filesize/1024));

	// Inittime is at least the time when this function finished. However,
	// later events may bump it.
	prog->inittime = host.realtime;
	return NULL; // Baker r1414 - automatic unload of fte csqc-lite csprogs.dat
}


static void PRVM_Fields_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	int i, j, ednum, used, usedamount;
	int *counts;
	char tempstring[MAX_INPUTLINE_16384], tempstring2[260];
	const char *name;
	prvm_edict_t *ed;
	mdef_t *d;
	prvm_eval_t *val;

	// TODO
	/*
	if (!sv.active)
	{
		Con_Print("no progs loaded\n");
		return;
	}
	*/

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Print("prvm_fields <program name>\n");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	counts = (int *)Mem_Alloc(tempmempool, prog->numfielddefs * sizeof(int));
	for (ednum = 0;ednum < prog->max_edicts;ednum++)
	{
		ed = PRVM_EDICT_NUM(ednum);
		if (ed->free)
			continue;
		for (i = 1;i < prog->numfielddefs;i++)
		{
			d = &prog->fielddefs[i];
			name = PRVM_GetString(prog, d->s_name);
			if (name[strlen(name)-2] == '_')
				continue;	// skip _x, _y, _z vars
			val = (prvm_eval_t *)(ed->fields.fp + d->ofs);
			// if the value is still all 0, skip the field
			for (j = 0;j < prvm_type_size[d->type & ~DEF_SAVEGLOBAL];j++)
			{
				if (val->ivector[j])
				{
					counts[i]++;
					break;
				}
			}
		}
	}
	used = 0;
	usedamount = 0;
	tempstring[0] = 0;
	for (i = 0;i < prog->numfielddefs;i++)
	{
		d = &prog->fielddefs[i];
		name = PRVM_GetString(prog, d->s_name);
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
		switch(d->type & ~DEF_SAVEGLOBAL)
		{
		case ev_string:
			strlcat(tempstring, "string   ", sizeof(tempstring));
			break;
		case ev_entity:
			strlcat(tempstring, "entity   ", sizeof(tempstring));
			break;
		case ev_function:
			strlcat(tempstring, "function ", sizeof(tempstring));
			break;
		case ev_field:
			strlcat(tempstring, "field    ", sizeof(tempstring));
			break;
		case ev_void:
			strlcat(tempstring, "void     ", sizeof(tempstring));
			break;
		case ev_float:
			strlcat(tempstring, "float    ", sizeof(tempstring));
			break;
		case ev_vector:
			strlcat(tempstring, "vector   ", sizeof(tempstring));
			break;
		case ev_pointer:
			strlcat(tempstring, "pointer  ", sizeof(tempstring));
			break;
		default:
			dpsnprintf (tempstring2, sizeof(tempstring2), "bad type %d ", d->type & ~DEF_SAVEGLOBAL);
			strlcat(tempstring, tempstring2, sizeof(tempstring));
			break;
		}
		if (strlen(name) > sizeof(tempstring2)-4)
		{
			memcpy (tempstring2, name, sizeof(tempstring2)-4);
			tempstring2[sizeof(tempstring2)-4] = tempstring2[sizeof(tempstring2)-3] = tempstring2[sizeof(tempstring2)-2] = '.';
			tempstring2[sizeof(tempstring2)-1] = 0;
			name = tempstring2;
		}
		strlcat(tempstring, name, sizeof(tempstring));
		for (j = (int)strlen(name);j < 25;j++)
			strlcat(tempstring, " ", sizeof(tempstring));
		dpsnprintf(tempstring2, sizeof(tempstring2), "%5d", counts[i]);
		strlcat(tempstring, tempstring2, sizeof(tempstring));
		strlcat(tempstring, "\n", sizeof(tempstring));
		if (strlen(tempstring) >= sizeof(tempstring)/2)
		{
			Con_Print(tempstring);
			tempstring[0] = 0;
		}
		if (counts[i])
		{
			used++;
			usedamount += prvm_type_size[d->type & ~DEF_SAVEGLOBAL];
		}
	}
	Mem_Free(counts);
	Con_Printf ("%s: %d entity fields (%d in use), totalling %d bytes per edict (%d in use), %d edicts allocated, %d bytes total spent on edict fields (%d needed)\n", prog->name, prog->entityfields, used, prog->entityfields * 4, usedamount * 4, prog->max_edicts, prog->entityfields * 4 * prog->max_edicts, usedamount * 4 * prog->max_edicts);
}


const char *PRVM_Prog_Dump_Functions (prvm_prog_t *prog)
{


	// Baker: Quakespasm equivalent sec 2
	for (int j = 0; j < prog->progs_numfunctions; j ++) {
		mfunction_t *func = &prog->functions[j];
		const char *s_f = PRVM_GetString(prog, func->s_name);
		Con_PrintLinef ("%05d " S_FMT_LEFT_PAD_20 " 1st state: %d", j, s_f, /*int32_t*/ func->first_statement);
	}
	return 0;
}

static void PRVM_Globals_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	int i;
	const char *wildcard;
	int numculled;
		numculled = 0;
	// TODO
	/*if (!sv.active)
	{
		Con_Print("no progs loaded\n");
		return;
	}*/
	if (Cmd_Argc (cmd) < 2 || Cmd_Argc(cmd) > 3)
	{
		Con_Print("prvm_globals <program name> <optional name wildcard>\n");
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	if ( Cmd_Argc(cmd) == 3)
		wildcard = Cmd_Argv(cmd, 2);
	else
		wildcard = NULL;

	Con_Printf ("%s :", prog->name);

	for (i = 0;i < prog->numglobaldefs;i++)
	{
		if (wildcard)
			if ( !matchpattern( PRVM_GetString(prog, prog->globaldefs[i].s_name), wildcard, 1) )
			{
				numculled++;
				continue;
			}
		Con_Printf ("%s\n", PRVM_GetString(prog, prog->globaldefs[i].s_name));
	}
	Con_Printf ("%d global variables, %d culled, totalling %d bytes\n", prog->numglobals, numculled, prog->numglobals * 4);
}

/*
===============
PRVM_Global
===============
*/
static void PRVM_Global_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	mdef_t *global;
	char valuebuf[MAX_INPUTLINE_16384];
	if ( Cmd_Argc(cmd) != 3 ) {
		Con_Printf ( "prvm_global <program name> <global name>\n" );
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	global = PRVM_ED_FindGlobal( prog, Cmd_Argv(cmd, 2) );
	if ( !global )
		Con_Printf ( "No global '%s' in %s!\n", Cmd_Argv(cmd, 2), Cmd_Argv(cmd, 1) );
	else
		Con_Printf ( "%s: %s\n", Cmd_Argv(cmd, 2), PRVM_ValueString( prog, (etype_t)global->type, PRVM_GLOBALFIELDVALUE(global->ofs), valuebuf, sizeof(valuebuf) ) );
}

/*
===============
PRVM_GlobalSet
===============
*/
static void PRVM_GlobalSet_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;
	mdef_t *global;
	if ( Cmd_Argc(cmd) != 4 ) {
		Con_Printf ( "prvm_globalset <program name> <global name> <value>\n" );
		return;
	}

	if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
		return;

	global = PRVM_ED_FindGlobal( prog, Cmd_Argv(cmd, 2) );
	if ( !global )
		Con_Printf ( "No global '%s' in %s!\n", Cmd_Argv(cmd, 2), Cmd_Argv(cmd, 1) );
	else
		PRVM_ED_ParseEpair( prog, NULL, global, Cmd_Argv(cmd, 3), true );
}

/*
======================
Break- and Watchpoints
======================
*/
typedef struct
{
	char break_statement[256];
	char watch_global[256];
	int watch_edict;
	char watch_field[256];
}
debug_data_t;
static debug_data_t debug_data[PRVM_PROG_MAX_3];

void PRVM_Breakpoint(prvm_prog_t *prog, int stack_index, const char *text)
{
	char vabuf[1024];
	Con_PrintLinef ("PRVM_Breakpoint: %s", text);
	PRVM_PrintState(prog, stack_index);
	if (prvm_breakpointdump.integer)
		SV_Savegame_to(prog, q_savestring_NULL, va(vabuf, sizeof(vabuf), "breakpoint-%s.dmp", prog->name), q_is_siv_write_false, /*totaltimeatlastexit*/ 0);
}

void PRVM_Watchpoint(prvm_prog_t *prog, int stack_index, const char *text, etype_t type, prvm_eval_t *o, prvm_eval_t *n)
{
	size_t sz = sizeof(prvm_vec_t) * ((type & ~DEF_SAVEGLOBAL) == ev_vector ? 3 : 1);
	if (memcmp(o, n, sz))
	{
		char buf[1024];
		char valuebuf_o[128];
		char valuebuf_n[128];
		PRVM_UglyValueString(prog, type, o, valuebuf_o, sizeof(valuebuf_o));
		PRVM_UglyValueString(prog, type, n, valuebuf_n, sizeof(valuebuf_n));
		dpsnprintf(buf, sizeof(buf), "%s: %s -> %s", text, valuebuf_o, valuebuf_n);
		PRVM_Breakpoint(prog, stack_index, buf);
		memcpy(o, n, sz);
	}
}

static void PRVM_UpdateBreakpoints(prvm_prog_t *prog)
{
	debug_data_t *debug = &debug_data[prog - prvm_prog_list];
	if (!prog->loaded)
		return;
	if (debug->break_statement[0])
	{
		if (debug->break_statement[0] >= '0' && debug->break_statement[0] <= '9')
		{
			prog->break_statement = atoi(debug->break_statement);
			prog->break_stack_index = 0;
		}
		else
		{
			mfunction_t *func;
			func = PRVM_ED_FindFunction (prog, debug->break_statement);
			if (!func)
			{
				Con_Printf ("%s progs: no function or statement named %s to break on!\n", prog->name, debug->break_statement);
				prog->break_statement = -1;
			}
			else
			{
				prog->break_statement = func->first_statement;
				prog->break_stack_index = 1;
			}
		}
		if (prog->break_statement >= -1)
			Con_Printf ("%s progs: breakpoint is at statement %d\n", prog->name, prog->break_statement);
	}
	else
		prog->break_statement = -1;

	if (debug->watch_global[0])
	{
		mdef_t *global = PRVM_ED_FindGlobal( prog, debug->watch_global );
		if ( !global )
		{
			Con_Printf ( "%s progs: no global named '%s' to watch!\n", prog->name, debug->watch_global );
			prog->watch_global_type = ev_void;
		}
		else
		{
			size_t sz = sizeof(prvm_vec_t) * ((global->type  & ~DEF_SAVEGLOBAL) == ev_vector ? 3 : 1);
			prog->watch_global = global->ofs;
			prog->watch_global_type = (etype_t)global->type;
			memcpy(&prog->watch_global_value, PRVM_GLOBALFIELDVALUE(prog->watch_global), sz);
		}
		if (prog->watch_global_type != ev_void)
			Con_Printf ("%s progs: global watchpoint is at global index %d\n", prog->name, prog->watch_global);
	}
	else
		prog->watch_global_type = ev_void;

	if (debug->watch_field[0])
	{
		mdef_t *field = PRVM_ED_FindField( prog, debug->watch_field );
		if ( !field )
		{
			Con_Printf ( "%s progs: no field named '%s' to watch!\n", prog->name, debug->watch_field );
			prog->watch_field_type = ev_void;
		}
		else
		{
			size_t sz = sizeof(prvm_vec_t) * ((field->type & ~DEF_SAVEGLOBAL) == ev_vector ? 3 : 1);
			prog->watch_edict = debug->watch_edict;
			prog->watch_field = field->ofs;
			prog->watch_field_type = (etype_t)field->type;
			if (prog->watch_edict < prog->num_edicts)
				memcpy(&prog->watch_edictfield_value, PRVM_EDICTFIELDVALUE(PRVM_EDICT_NUM(prog->watch_edict), prog->watch_field), sz);
			else
				memset(&prog->watch_edictfield_value, 0, sz);
		}
		if (prog->watch_edict != ev_void)
			Con_Printf ("%s progs: edict field watchpoint is at edict %d field index %d\n", prog->name, prog->watch_edict, prog->watch_field);
	}
	else
		prog->watch_field_type = ev_void;
}

static void PRVM_Breakpoint_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if ( Cmd_Argc(cmd) == 2 ) {
		if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
			return;
		{
			debug_data_t *debug = &debug_data[prog - prvm_prog_list];
			debug->break_statement[0] = 0;
		}
		PRVM_UpdateBreakpoints(prog);
		return;
	}
	if ( Cmd_Argc(cmd) != 3 ) {
		Con_Printf ( "prvm_breakpoint <program name> <function name | statement>\n" );
		return;
	}

	if (!(prog = PRVM_ProgFromString(Cmd_Argv(cmd, 1))))
		return;

	{
		debug_data_t *debug = &debug_data[prog - prvm_prog_list];
		strlcpy(debug->break_statement, Cmd_Argv(cmd, 2), sizeof(debug->break_statement));
	}
	PRVM_UpdateBreakpoints(prog);
}

static void PRVM_GlobalWatchpoint_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if ( Cmd_Argc(cmd) == 2 ) {
		if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
			return;
		{
			debug_data_t *debug = &debug_data[prog - prvm_prog_list];
			debug->watch_global[0] = 0;
		}
		PRVM_UpdateBreakpoints(prog);
		return;
	}
	if ( Cmd_Argc(cmd) != 3 ) {
		Con_Printf ( "prvm_globalwatchpoint <program name> <global name>\n" );
		return;
	}

	if (!(prog = PRVM_ProgFromString(Cmd_Argv(cmd, 1))))
		return;

	{
		debug_data_t *debug = &debug_data[prog - prvm_prog_list];
		strlcpy(debug->watch_global, Cmd_Argv(cmd, 2), sizeof(debug->watch_global));
	}
	PRVM_UpdateBreakpoints(prog);
}

static void PRVM_EdictWatchpoint_f(cmd_state_t *cmd)
{
	prvm_prog_t *prog;

	if ( Cmd_Argc(cmd) == 2 ) {
		if (!(prog = PRVM_FriendlyProgFromString(Cmd_Argv(cmd, 1))))
			return;
		{
			debug_data_t *debug = &debug_data[prog - prvm_prog_list];
			debug->watch_field[0] = 0;
		}
		PRVM_UpdateBreakpoints(prog);
		return;
	}
	if ( Cmd_Argc(cmd) != 4 ) {
		Con_Printf ( "prvm_edictwatchpoint <program name> <edict number> <field name>\n" );
		return;
	}

	if (!(prog = PRVM_ProgFromString(Cmd_Argv(cmd, 1))))
		return;

	{
		debug_data_t *debug = &debug_data[prog - prvm_prog_list];
		debug->watch_edict = atoi(Cmd_Argv(cmd, 2));
		strlcpy(debug->watch_field, Cmd_Argv(cmd, 3), sizeof(debug->watch_field));
	}
	PRVM_UpdateBreakpoints(prog);
}


/*
===============
PRVM_InitProg
===============
*/
void PRVM_Prog_Init(prvm_prog_t *prog, cmd_state_t *cmd)
{
	PRVM_Prog_Reset(prog);
	prog->leaktest_active = prvm_leaktest.integer != 0;
	prog->console_cmd = cmd;
}

// LadyHavoc: turned PRVM_EDICT_NUM into a #define for speed reasons
unsigned int PRVM_EDICT_NUM_ERROR(prvm_prog_t *prog, unsigned int n, const char *filename, int fileline)
{
	prog->error_cmd("PRVM_EDICT_NUM: %s: bad number %d (called at %s:%d)", prog->name, n, filename, fileline);
	return 0;
}

#define PRVM_KNOWNSTRINGBASE 0x40000000

const char *PRVM_GetString(prvm_prog_t *prog, int num)
{
	if (num < 0)
	{
		// invalid
		if (prvm_stringdebug.integer)
			VM_Warningf(prog, "PRVM_GetString: Invalid string offset (%d < 0)\n", num);
		return "";
	}
	else if (num < prog->stringssize)
	{
		// constant string from progs.dat
		return prog->strings + num;
	}
	else if (num <= prog->stringssize + prog->tempstringsbuf.maxsize)
	{
		// tempstring returned by engine to QC (becomes invalid after returning to engine)
		num -= prog->stringssize;
		if (num < prog->tempstringsbuf.cursize)
			return (char *)prog->tempstringsbuf.data + num;
		else
		{
			if (prvm_stringdebug.integer)
				VM_Warningf(prog, "PRVM_GetString: Invalid temp-string offset (%d >= %d prog->tempstringsbuf.cursize)\n", num, prog->tempstringsbuf.cursize);
			return "";
		}
	}
	else if (num & PRVM_KNOWNSTRINGBASE)
	{
		// allocated string
		num = num - PRVM_KNOWNSTRINGBASE;
		if (num >= 0 && num < prog->numknownstrings)
		{
			if (!prog->knownstrings[num])
			{
				if (prvm_stringdebug.integer)
					VM_Warningf(prog, "PRVM_GetString: Invalid zone-string offset (%d has been freed)\n", num);
				return "";
			}
			// refresh the garbage collection on the string - this guards
			// against a certain sort of repeated migration to earlier
			// points in the scan that could otherwise result in the string
			// being freed for being unused
			prog->knownstrings_flags[num] = (prog->knownstrings_flags[num] & ~KNOWNSTRINGFLAG_GCPRUNE) | KNOWNSTRINGFLAG_GCMARK;
			return prog->knownstrings[num];
		}
		else
		{
			if (prvm_stringdebug.integer)
				VM_Warningf(prog, "PRVM_GetString: Invalid zone-string offset (%d >= %d)\n", num, prog->numknownstrings);
			return "";
		}
	}
	else
	{
		// invalid string offset
		if (prvm_stringdebug.integer)
			VM_Warningf(prog, "PRVM_GetString: Invalid constant-string offset (%d >= %d prog->stringssize)\n", num, prog->stringssize);
		return "";
	}
}

const char *PRVM_ChangeEngineString(prvm_prog_t *prog, int i, const char *s)
{
	const char *old;
	i = i - PRVM_KNOWNSTRINGBASE;
	if (i < 0 || i >= prog->numknownstrings)
		prog->error_cmd("PRVM_ChangeEngineString: string index %d is out of bounds", i);
	else if ((prog->knownstrings_flags[i] & KNOWNSTRINGFLAG_ENGINE) == 0)
		prog->error_cmd("PRVM_ChangeEngineString: string index %d is not an engine string", i);
	old = prog->knownstrings[i];
	prog->knownstrings[i] = s;
	return old;
}

static void PRVM_NewKnownString(prvm_prog_t *prog, int i, int flags, const char *s)
{
	if (i >= prog->numknownstrings)
	{
		if (i >= prog->maxknownstrings)
		{
			const char **oldstrings = prog->knownstrings;
			const unsigned char *oldstrings_flags = prog->knownstrings_flags;
			const char **oldstrings_origin = prog->knownstrings_origin;
			prog->maxknownstrings += 128;
			prog->knownstrings = (const char **)PRVM_Alloc(prog->maxknownstrings * sizeof(char *));
			prog->knownstrings_flags = (unsigned char *)PRVM_Alloc(prog->maxknownstrings * sizeof(unsigned char));
			if (prog->leaktest_active)
				prog->knownstrings_origin = (const char **)PRVM_Alloc(prog->maxknownstrings * sizeof(char *));
			if (prog->numknownstrings)
			{
				memcpy((char **)prog->knownstrings, oldstrings, prog->numknownstrings * sizeof(char *));
				memcpy((char **)prog->knownstrings_flags, oldstrings_flags, prog->numknownstrings * sizeof(unsigned char));
				if (prog->leaktest_active)
					memcpy((char **)prog->knownstrings_origin, oldstrings_origin, prog->numknownstrings * sizeof(char *));
			}
		}
		prog->numknownstrings++;
	}
	prog->firstfreeknownstring = i + 1;
	prog->knownstrings[i] = s;
	// it's in use right now, spare it until the next gc pass - that said, it is not freeable so this is probably moot
	prog->knownstrings_flags[i] = flags;
	if (prog->leaktest_active)
		prog->knownstrings_origin[i] = NULL;
}

int PRVM_SetEngineString(prvm_prog_t *prog, const char *s)
{
	int i;
	if (!s)
		return 0;
	if (s >= prog->strings && s <= prog->strings + prog->stringssize)
		prog->error_cmd("PRVM_SetEngineString: s in prog->strings area");
	// if it's in the tempstrings area, use a reserved range
	// (otherwise we'd get millions of useless string offsets cluttering the database)
	if (s >= (char *)prog->tempstringsbuf.data && s < (char *)prog->tempstringsbuf.data + prog->tempstringsbuf.maxsize)
		return prog->stringssize + (s - (char *)prog->tempstringsbuf.data);
	// see if it's a known string address
	for (i = 0;i < prog->numknownstrings;i++)
		if (prog->knownstrings[i] == s)
			return PRVM_KNOWNSTRINGBASE + i;
	// new unknown engine string
	if (developer_insane.integer)
		Con_DPrintLinef ("new engine string %p = " QUOTED_S, (void *)s, s);
	for (i = prog->firstfreeknownstring;i < prog->numknownstrings;i++)
		if (!prog->knownstrings[i])
			break;
	PRVM_NewKnownString(prog, i, KNOWNSTRINGFLAG_GCMARK | KNOWNSTRINGFLAG_ENGINE, s);
	return PRVM_KNOWNSTRINGBASE + i;
}

// temp string handling

// all tempstrings go into this buffer consecutively, and it is reset
// whenever PRVM_ExecuteProgram returns to the engine
// (technically each PRVM_ExecuteProgram call saves the cursize value and
//  restores it on return, so multiple recursive calls can share the same
//  buffer)
// the buffer size is automatically grown as needed

int PRVM_SetTempString(prvm_prog_t *prog, const char *s)
{
	int size;
	char *t;
	if (!s)
		return 0;
	size = (int)strlen(s) + 1;
	if (developer_insane.integer)
		Con_DPrintf ("PRVM_SetTempString: cursize %d, size %d\n", prog->tempstringsbuf.cursize, size);
	if (prog->tempstringsbuf.maxsize < prog->tempstringsbuf.cursize + size)
	{
		sizebuf_t old = prog->tempstringsbuf;
		if (prog->tempstringsbuf.cursize + size >= 1<<28)
			prog->error_cmd("PRVM_SetTempString: ran out of tempstring memory!  (refusing to grow tempstring buffer over 256MB, cursize %d, size %d)\n", prog->tempstringsbuf.cursize, size);
		prog->tempstringsbuf.maxsize = max(prog->tempstringsbuf.maxsize, 65536);
		while (prog->tempstringsbuf.maxsize < prog->tempstringsbuf.cursize + size)
			prog->tempstringsbuf.maxsize *= 2;
		if (prog->tempstringsbuf.maxsize != old.maxsize || prog->tempstringsbuf.data == NULL)
		{
			Con_DPrintf ("PRVM_SetTempString: enlarging tempstrings buffer (%dKB -> %dKB)\n", old.maxsize/1024, prog->tempstringsbuf.maxsize/1024);
			prog->tempstringsbuf.data = (unsigned char *) Mem_Alloc(prog->progs_mempool, prog->tempstringsbuf.maxsize);
			if (old.data)
			{
				if (old.cursize)
					memcpy(prog->tempstringsbuf.data, old.data, old.cursize);
				Mem_Free(old.data);
			}
		}
	}
	t = (char *)prog->tempstringsbuf.data + prog->tempstringsbuf.cursize;
	memcpy(t, s, size);
	prog->tempstringsbuf.cursize += size;
	return PRVM_SetEngineString(prog, t);
}

int PRVM_AllocString(prvm_prog_t *prog, size_t bufferlength, char **pointer)
{
	int i;
	char *s;
	if (!bufferlength)
	{
		if (pointer)
			*pointer = NULL;
		return 0;
	}
	for (i = prog->firstfreeknownstring;i < prog->numknownstrings;i++)
		if (!prog->knownstrings[i])
			break;
	s = (char *)PRVM_Alloc(bufferlength);
	PRVM_NewKnownString(prog, i, KNOWNSTRINGFLAG_GCMARK, s);
	if (prog->leaktest_active)
		prog->knownstrings_origin[i] = PRVM_AllocationOrigin(prog);
	if (pointer)
		*pointer = (char *)(prog->knownstrings[i]);
	return PRVM_KNOWNSTRINGBASE + i;
}

void PRVM_FreeString(prvm_prog_t *prog, int num)
{
	if (num == 0)
		prog->error_cmd("PRVM_FreeString: attempt to free a NULL string");
	else if (num >= 0 && num < prog->stringssize)
		prog->error_cmd("PRVM_FreeString: attempt to free a constant string");
	else if (num >= PRVM_KNOWNSTRINGBASE && num < PRVM_KNOWNSTRINGBASE + prog->numknownstrings)
	{
		num = num - PRVM_KNOWNSTRINGBASE;
		if (!prog->knownstrings[num])
			prog->error_cmd("PRVM_FreeString: attempt to free a non-existent or already freed string");
		if (!prog->knownstrings_flags[num])
			prog->error_cmd("PRVM_FreeString: attempt to free a string owned by the engine");
		PRVM_Free((char *)prog->knownstrings[num]);
		if (prog->leaktest_active)
			if (prog->knownstrings_origin[num])
				PRVM_Free((char *)prog->knownstrings_origin[num]);
		prog->knownstrings[num] = NULL;
		prog->knownstrings_flags[num] = 0;
		prog->firstfreeknownstring = min(prog->firstfreeknownstring, num);
	}
	else
		prog->error_cmd("PRVM_FreeString: invalid string offset %d", num);
}

static qbool PRVM_IsStringReferenced(prvm_prog_t *prog, string_t string)
{
	int i, j;

	for (i = 0;i < prog->numglobaldefs;i++)
	{
		mdef_t *d = &prog->globaldefs[i];
		if ((etype_t)((int) d->type & ~DEF_SAVEGLOBAL) != ev_string)
			continue;
		if (string == PRVM_GLOBALFIELDSTRING(d->ofs))
			return true;
	}

	for(j = 0; j < prog->num_edicts; ++j)
	{
		prvm_edict_t *ed = PRVM_EDICT_NUM(j);
		if (ed->free)
			continue;
		for (i=0; i<prog->numfielddefs; ++i)
		{
			mdef_t *d = &prog->fielddefs[i];
			if ((etype_t)((int) d->type & ~DEF_SAVEGLOBAL) != ev_string)
				continue;
			if (string == PRVM_EDICTFIELDSTRING(ed, d->ofs))
				return true;
		}
	}

	return false;
}

static qbool PRVM_IsEdictRelevant(prvm_prog_t *prog, prvm_edict_t *edict)
{
	char vabuf[1024];
	char vabuf2[1024];
	if (PRVM_NUM_FOR_EDICT(edict) <= prog->reserved_edicts)
		return true; // world or clients
	if (edict->freetime <= prog->inittime)
		return true; // created during startup
	if (prog == SVVM_prog)
	{
		if (PRVM_serveredictfloat(edict, solid)) // can block other stuff, or is a trigger?
			return true;
		if (PRVM_serveredictfloat(edict, modelindex)) // visible ent?
			return true;
		if (PRVM_serveredictfloat(edict, effects)) // particle effect?
			return true;
		if (PRVM_serveredictfunction(edict, think)) // has a think function?
			if (PRVM_serveredictfloat(edict, nextthink) > 0) // that actually will eventually run?
				return true;
		if (PRVM_serveredictfloat(edict, takedamage))
			return true;
		if (*prvm_leaktest_ignore_classnames.string)
		{
			if (strstr(va(vabuf, sizeof(vabuf), " %s ", prvm_leaktest_ignore_classnames.string), va(vabuf2, sizeof(vabuf2), " %s ", PRVM_GetString(prog, PRVM_serveredictstring(edict, classname)))))
				return true;
		}
	}
	else if (prog == CLVM_prog)
	{
		// TODO someone add more stuff here
		if (PRVM_clientedictfloat(edict, entnum)) // csqc networked
			return true;
		if (PRVM_clientedictfloat(edict, modelindex)) // visible ent?
			return true;
		if (PRVM_clientedictfloat(edict, effects)) // particle effect?
			return true;
		if (PRVM_clientedictfunction(edict, think)) // has a think function?
			if (PRVM_clientedictfloat(edict, nextthink) > 0) // that actually will eventually run?
				return true;
		if (*prvm_leaktest_ignore_classnames.string)
		{
			if (strstr(va(vabuf, sizeof(vabuf), " %s ", prvm_leaktest_ignore_classnames.string), va(vabuf2, sizeof(vabuf2), " %s ", PRVM_GetString(prog, PRVM_clientedictstring(edict, classname)))))
				return true;
		}
	}
	else
	{
		// menu prog does not have classnames
	}
	return false;
}

static qbool PRVM_IsEdictReferenced(prvm_prog_t *prog, prvm_edict_t *edict, int mark)
{
	int i, j;
	int edictnum = PRVM_NUM_FOR_EDICT(edict);
	const char *targetname = NULL;

	if (prog == SVVM_prog && prvm_leaktest_follow_targetname.integer)
		targetname = PRVM_GetString(prog, PRVM_serveredictstring(edict, targetname));

	if (targetname)
		if (!*targetname) // ""
			targetname = NULL;

	for(j = 0; j < prog->num_edicts; ++j)
	{
		prvm_edict_t *ed = PRVM_EDICT_NUM(j);
		if (ed->priv.required->mark < mark)
			continue;
		if (ed == edict)
			continue;
		if (targetname)
		{
			const char *target = PRVM_GetString(prog, PRVM_serveredictstring(ed, target));
			if (target)
				if (String_Match(target, targetname))
					return true;
		}
		for (i=0; i<prog->numfielddefs; ++i)
		{
			mdef_t *d = &prog->fielddefs[i];
			if ((etype_t)((int) d->type & ~DEF_SAVEGLOBAL) != ev_entity)
				continue;
			if (edictnum == PRVM_EDICTFIELDEDICT(ed, d->ofs))
				return true;
		}
	}

	return false;
}

static void PRVM_MarkReferencedEdicts(prvm_prog_t *prog)
{
	int i, j;
	qbool found_new;
	int stage;

	// Stage 1: world, all entities that are relevant, and all entities that are referenced by globals.
	stage = 1;
	for(j = 0; j < prog->num_edicts; ++j)
	{
		prvm_edict_t *ed = PRVM_EDICT_NUM(j);
		if (ed->free)
			continue;
		ed->priv.required->mark = PRVM_IsEdictRelevant(prog, ed) ? stage : 0;
	}
	for (i = 0;i < prog->numglobaldefs;i++)
	{
		mdef_t *d = &prog->globaldefs[i];
		prvm_edict_t *ed;
		if ((etype_t)((int) d->type & ~DEF_SAVEGLOBAL) != ev_entity)
			continue;
		j = PRVM_GLOBALFIELDEDICT(d->ofs);
		if (i < 0 || j >= prog->max_edicts) {
			Con_Printf ("Invalid entity reference from global %s.\n", PRVM_GetString(prog, d->s_name));
			continue;
		}
		ed = PRVM_EDICT_NUM(j);;
		ed->priv.required->mark = stage;
	}

	// Future stages: all entities that are referenced by an entity of the previous stage.
	do
	{
		found_new = false;
		for(j = 0; j < prog->num_edicts; ++j)
		{
			prvm_edict_t *ed = PRVM_EDICT_NUM(j);
			if (ed->free)
				continue;
			if (ed->priv.required->mark)
				continue;
			if (PRVM_IsEdictReferenced(prog, ed, stage))
			{
				ed->priv.required->mark = stage + 1;
				found_new = true;
			}
		}
		++stage;
	}
	while(found_new);
	Con_DPrintf ("leak check used %d stages to find all references\n", stage);
}

void PRVM_LeakTest(prvm_prog_t *prog)
{
	int i, j;
	qbool leaked = false;

	if (!prog->leaktest_active)
		return;

	// 1. Strings
	for (i = 0; i < prog->numknownstrings; ++i)
	{
		if (prog->knownstrings[i])
		if (prog->knownstrings_flags[i])
		if (prog->knownstrings_origin[i])
		if (!PRVM_IsStringReferenced(prog, PRVM_KNOWNSTRINGBASE + i))
		{
			Con_Printf ("Unreferenced string found!\n  Value: %s\n  Origin: %s\n", prog->knownstrings[i], prog->knownstrings_origin[i]);
			leaked = true;
		}
	}

	// 2. Edicts
	PRVM_MarkReferencedEdicts(prog);
	for(j = 0; j < prog->num_edicts; ++j)
	{
		prvm_edict_t *ed = PRVM_EDICT_NUM(j);
		if (ed->free)
			continue;
		if (!ed->priv.required->mark)
		if (ed->priv.required->allocation_origin)
		{
			Con_PrintLinef ("Unreferenced edict found!" NEWLINE "  Allocated at: %s", ed->priv.required->allocation_origin);
			PRVM_ED_Print(prog, ed, q_vm_printfree_true, q_vm_wildcard_NULL, q_vm_classname_NULL, q_vm_targetname_NULL, q_stringlist_NULL);
			Con_Print("\n");
			leaked = true;
		}

		ed->priv.required->mark = 0; // clear marks again when done
	}

	for (i = 0; i < (int)Mem_ExpandableArray_IndexRange(&prog->stringbuffersarray); ++i)
	{
		prvm_stringbuffer_t *stringbuffer = (prvm_stringbuffer_t*) Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, i);
		if (stringbuffer)
		if (stringbuffer->origin)
		{
			Con_Printf ("Open string buffer handle found!\n  Allocated at: %s\n", stringbuffer->origin);
			leaked = true;
		}
	}

	for(i = 0; i < PRVM_MAX_OPENFILES; ++i)
	{
		if (prog->openfiles[i])
		if (prog->openfiles_origin[i])
		{
			Con_Printf ("Open file handle found!\n  Allocated at: %s\n", prog->openfiles_origin[i]);
			leaked = true;
		}
	}

	for(i = 0; i < PRVM_MAX_OPENSEARCHES; ++i)
	{
		if (prog->opensearches[i])
		if (prog->opensearches_origin[i])
		{
			Con_Printf ("Open search handle found!\n  Allocated at: %s\n", prog->opensearches_origin[i]);
			leaked = true;
		}
	}

	if (!leaked)
		Con_Printf ("Congratulations. No leaks found.\n");
}

void PRVM_GarbageCollection(prvm_prog_t *prog)
{
	int limit = prvm_garbagecollection_scan_limit.integer;
	prvm_prog_garbagecollection_state_t *gc = &prog->gc;
	if (!prvm_garbagecollection_enable.integer)
		return;
	// philosophy:
	// we like to limit how much scanning we do so it doesn't put a significant
	// burden on the cpu, so each of these are not complete scans, we also like
	// to have consistent cpu usage so we do a bit of work on each category of
	// leaked object every frame
	switch (gc->stage)
	{
	case PRVM_GC_START:
		gc->stage++;
		break;
	case PRVM_GC_GLOBALS_MARK:
		for (; gc->globals_mark_progress < prog->numglobaldefs && (limit--) > 0; gc->globals_mark_progress++)
		{
			mdef_t *d = &prog->globaldefs[gc->globals_mark_progress];
			switch (d->type)
			{
			case ev_string:
				{
					prvm_int_t s = prog->globals.ip[d->ofs];
					if (s & PRVM_KNOWNSTRINGBASE)
					{
						prvm_int_t num = s - PRVM_KNOWNSTRINGBASE;
						if (!prog->knownstrings[num])
						{
							// invalid
							Con_DPrintf ("PRVM_GarbageCollection: Found bogus strzone reference in global %d (global name: " QUOTED_S "), erasing reference", d->ofs, PRVM_GetString(prog, d->s_name));
							prog->globals.ip[d->ofs] = 0;
							continue;
						}
						prog->knownstrings_flags[num] = (prog->knownstrings_flags[num] | KNOWNSTRINGFLAG_GCMARK) & ~KNOWNSTRINGFLAG_GCPRUNE;
					}
				}
				break;
			default:
				break;
			}
		}
		if (gc->globals_mark_progress >= prog->numglobaldefs)
			gc->stage++;
		break;
	case PRVM_GC_FIELDS_MARK:
		for (; gc->fields_mark_progress < prog->numfielddefs && limit > 0;)
		{
			mdef_t *d = &prog->fielddefs[gc->fields_mark_progress];
			switch (d->type)
			{
			case ev_string:
				//for (gc-> entityindex = 0; entityindex < prog->num_edicts; entityindex++)
				for (;gc->fields_mark_progress_entity < prog->num_edicts && (limit--) > 0;gc->fields_mark_progress_entity++)
				{
					int entityindex = gc->fields_mark_progress_entity;
					prvm_int_t s = prog->edictsfields.ip[entityindex * prog->entityfields + d->ofs];
					if (s & PRVM_KNOWNSTRINGBASE)
					{
						prvm_int_t num = s - PRVM_KNOWNSTRINGBASE;
						if (!prog->knownstrings[num])
						{
							// invalid
							Con_DPrintf ("PRVM_GarbageCollection: Found bogus strzone reference in edict %d field %d (field name: " QUOTED_S "), erasing reference", entityindex, d->ofs, PRVM_GetString(prog, d->s_name));
							prog->edictsfields.ip[entityindex * prog->entityfields + d->ofs] = 0;
							continue;
						}
						prog->knownstrings_flags[num] = (prog->knownstrings_flags[num] | KNOWNSTRINGFLAG_GCMARK) & ~KNOWNSTRINGFLAG_GCPRUNE;
					}
				}
				if (gc->fields_mark_progress_entity >= prog->num_edicts)
				{
					gc->fields_mark_progress_entity = 0;
					gc->fields_mark_progress++;
				}
				break;
			default:
				gc->fields_mark_progress_entity = 0;
				gc->fields_mark_progress++;
				break;
			}
		}
		if (gc->fields_mark_progress >= prog->numfielddefs)
			gc->stage++;
		break;
	case PRVM_GC_KNOWNSTRINGS_SWEEP:
		// free any strzone'd strings that are not marked
		if (!prvm_garbagecollection_strings.integer)
		{
			gc->stage++;
			break;
		}
		for (;gc->knownstrings_sweep_progress < prog->numknownstrings && (limit--) > 0;gc->knownstrings_sweep_progress++)
		{
			int num = gc->knownstrings_sweep_progress;
			if (prog->knownstrings[num] && (prog->knownstrings_flags[num] & (KNOWNSTRINGFLAG_GCMARK | KNOWNSTRINGFLAG_ENGINE)) == 0)
			{
				if (prog->knownstrings_flags[num] & KNOWNSTRINGFLAG_GCPRUNE)
				{
					// string has been marked for pruning two passes in a row
					if (prvm_garbagecollection_notify.integer)
						Con_DPrintLinef ("prvm_garbagecollection_notify: %s: freeing unreferenced string %d: " QUOTED_S, prog->name, num, prog->knownstrings[num]);
					Mem_Free((char *)prog->knownstrings[num]);
					prog->knownstrings[num] = NULL;
					prog->knownstrings_flags[num] = 0;
					prog->firstfreeknownstring = min(prog->firstfreeknownstring, num);
				}
				else
				{
					// mark it for pruning next pass
					prog->knownstrings_flags[num] |= KNOWNSTRINGFLAG_GCPRUNE;
				}
			}
		}
		if (gc->knownstrings_sweep_progress >= prog->numknownstrings)
			gc->stage++;
		break;
	case PRVM_GC_RESET:
	default:
		memset(gc, 0, sizeof(*gc));
	}
}

/*
===============
PRVM_InitOnce
===============
*/
void PRVM_InitOnce (void)
{
	Cmd_AddCommand(CF_SHARED, "prvm_edict", PRVM_ED_PrintEdict_f, "print all data about an entity number in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_edicts", PRVM_ED_PrintEdicts_f, "prints all data about all entities in the selected VM (server, client, menu)");

	Cmd_AddCommand (CF_SHARED, "edict", PRVM_ED_PrintEdict_SV_f, "print all data about an entity number in the server VM [Zircon]");  // Baker r1083: "edict" and "edicts"
	Cmd_AddCommand (CF_SHARED, "csedict", PRVM_ED_PrintEdict_CL_f, "print all data about an entity number in the client VM [Zircon]");  // Baker r1083: "edict" and "edicts"

	Cmd_AddCommand (CF_SHARED, "edicts", PRVM_ED_PrintEdicts_SV_f, "prints all data about all entities in the VM server.  edicts <classname> or edicts <fieldnamepartial> <fieldvaluepartial> [Zircon]");  // Baker r1083: "edict" and "edicts"
	Cmd_AddCommand (CF_SHARED, "csedicts", PRVM_ED_PrintEdicts_CL_f, "prints all data about all entities in the selected VM (server, client, menu) [Zircon]");  // Baker r1083: "edict" and "edicts"

	Cmd_AddCommand(CF_SHARED, "prvm_edictcount", PRVM_ED_Count_f, "prints number of active entities in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_profile", PRVM_Profile_f, "prints execution statistics about the most used QuakeC functions in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_childprofile", PRVM_ChildProfile_f, "prints execution statistics about the most used QuakeC functions in the selected VM (server, client, menu), sorted by time taken in function with child calls");
	Cmd_AddCommand(CF_SHARED, "prvm_callprofile", PRVM_CallProfile_f, "prints execution statistics about the most time consuming QuakeC calls from the engine in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_fields", PRVM_Fields_f, "prints usage statistics on properties (how many entities have non-zero values) in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_globals", PRVM_Globals_f, "prints all global variables in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED, "prvm_global", PRVM_Global_f, "prints value of a specified global variable in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED | CF_CHEAT, "prvm_globalset", PRVM_GlobalSet_f, "sets value of a specified global variable in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED | CF_CHEAT, "prvm_edictset", PRVM_ED_EdictSet_f, "changes value of a specified property of a specified entity in the selected VM (server, client, menu)");
	Cmd_AddCommand(CF_SHARED | CF_CHEAT, "eset", PRVM_ED_Eset_f, "prvm_edictset for server [Zircon]");
	Cmd_AddCommand(CF_SHARED, "prvm_edictget", PRVM_ED_EdictGet_f, "retrieves the value of a specified property of a specified entity in the selected VM (server, client menu) into a cvar or to the console");
	Cmd_AddCommand(CF_SHARED, "prvm_globalget", PRVM_ED_GlobalGet_f, "retrieves the value of a specified global variable in the selected VM (server, client menu) into a cvar or to the console");
	Cmd_AddCommand(CF_SHARED, "prvm_printfunction", PRVM_PrintFunction_f, "prints a disassembly (QuakeC instructions) of the specified function in the selected VM (server, client, menu)");
	
	Cmd_AddCommand(CF_SHARED, "cl_cmd", PRVM_GameCommand_Client_f, "calls the client QC function GameCommand with the supplied string as argument");
	Cmd_AddCommand(CF_SHARED, "menu_cmd", PRVM_GameCommand_Menu_f, "calls the menu QC function GameCommand with the supplied string as argument");
	Cmd_AddCommand(CF_SHARED, "sv_cmd", PRVM_GameCommand_Server_f, "calls the server QC function GameCommand with the supplied string as argument");

	Cmd_AddCommand(CF_SHARED, "prvm_breakpoint", PRVM_Breakpoint_f, "marks a statement or function as breakpoint (when this is executed, a stack trace is printed); to actually halt and investigate state, combine this with a gdb breakpoint on PRVM_Breakpoint, or with prvm_breakpointdump; run with just progs name to clear breakpoint");
	Cmd_AddCommand(CF_SHARED, "prvm_globalwatchpoint", PRVM_GlobalWatchpoint_f, "marks a global as watchpoint (when this is executed, a stack trace is printed); to actually halt and investigate state, combine this with a gdb breakpoint on PRVM_Breakpoint, or with prvm_breakpointdump; run with just progs name to clear watchpoint");
	Cmd_AddCommand(CF_SHARED, "prvm_edictwatchpoint", PRVM_EdictWatchpoint_f, "marks an entity field as watchpoint (when this is executed, a stack trace is printed); to actually halt and investigate state, combine this with a gdb breakpoint on PRVM_Breakpoint, or with prvm_breakpointdump; run with just progs name to clear watchpoint");

	// Baker r7084: gamecommand autocomplete assist
	Cvar_RegisterVariable (&prvm_sv_gamecommands); // Baker r7103 gamecommand autocomplete
	Cvar_RegisterVariable (&prvm_sv_progfields); // Baker r7103 gamecommand autocomplete

	Cvar_RegisterVariable (&prvm_cl_gamecommands); // Baker r7103 gamecommand autocomplete
	Cvar_RegisterVariable (&prvm_cl_progfields); // Baker r7103 gamecommand autocomplete

	Cvar_RegisterVariable (&prvm_edictget_autostore); // Baker: August 8 2024
	Cvar_RegisterVariable (&_last); // Baker: August 8 2024

	// #pragma message ("Baker: At some point revive prvm_menu_gamecommands, lack of CF_MENU is why we stall")
	// Cvar_RegisterVariable (&prvm_menu_gamecommands);
	// End

	Cvar_RegisterVariable (&prvm_language);
	Cvar_RegisterVariable (&prvm_traceqc);
	Cvar_RegisterVariable (&prvm_statementprofiling);
	Cvar_RegisterVariable (&prvm_timeprofiling);
	Cvar_RegisterVariable (&prvm_coverage);
	Cvar_RegisterVariable (&prvm_backtraceforwarnings);
	Cvar_RegisterVariable (&prvm_leaktest);
	Cvar_RegisterVariable (&prvm_leaktest_follow_targetname);
	Cvar_RegisterVariable (&prvm_leaktest_ignore_classnames);
	Cvar_RegisterVariable (&prvm_errordump);
	Cvar_RegisterVariable (&prvm_breakpointdump);
	Cvar_RegisterVariable (&prvm_reuseedicts_startuptime);
	Cvar_RegisterVariable (&prvm_reuseedicts_neverinsameframe);
	Cvar_RegisterVariable (&prvm_garbagecollection_enable);
	Cvar_RegisterVariable (&prvm_garbagecollection_notify);
	Cvar_RegisterVariable (&prvm_garbagecollection_scan_limit);
	Cvar_RegisterVariable (&prvm_garbagecollection_strings);
	Cvar_RegisterVariable (&prvm_stringdebug);

	// COMMANDLINEOPTION: PRVM: -norunaway disables the runaway loop check (it might be impossible to exit DarkPlaces if used!)
	prvm_runawaycheck = !Sys_CheckParm("-norunaway");

	//VM_Cmd_Init();
}


//void PRVM_ED_PrintEdicts_Feed (prvm_prog_t *prog, const char *s_fieldname_partial, const char *s_fieldvalue_partial)
//{
//	int		edict_num;
//	const char *wildcard_fieldname = NULL;
//	const char *s_fieldname_partial = "";
//	const char *s_fieldvalue_partial = "";
//
//	if (Cmd_Argc(cmd) == 3) {
//		// gcc unused ... const char *s_arg1 = Cmd_Argv(cmd, 1);
//		//int isnumeric = isdigit(s_arg1[0]);
//		s_fieldname_partial = Cmd_Argv(cmd, 1);
//		s_fieldvalue_partial = Cmd_Argv(cmd, 2);
//	}
//	else if (Cmd_Argc(cmd) == 2) {
//		s_fieldname_partial = "classname";
//		s_fieldvalue_partial = Cmd_Argv(cmd, 1);
//	}
//
//	if (Cmd_Argc(cmd) == 3) {
//		Con_PrintLinef ("%s: entities with field " QUOTED_S " containing " QUOTED_S,
//			prog->name, s_fieldname_partial, s_fieldvalue_partial);
//	} else if (Cmd_Argc(cmd) == 2) {
//		Con_PrintLinef ("%s: entities with classname containing " QUOTED_S, prog->name, s_fieldvalue_partial);
//	} else {
//		Con_PrintLinef ("%s: %d entities", prog->name, prog->num_edicts);
//	}
//
//	// Baker: Any search criteria and we don't print "free"
//	int shall_print_free = !s_fieldname_partial || s_fieldname_partial[0] == NULL_CHAR_0;
//
//	for (edict_num = 0 ; edict_num < prog->num_edicts ; edict_num ++)
//		PRVM_ED_PrintNum (prog, edict_num, shall_print_free, wildcard_fieldname, s_fieldname_partial, s_fieldvalue_partial);
//}

//typedef int stringlistsort_start_length_cmp(const void *_a, const void *_b)

WARP_X_ (PRVM_ED_FindGlobal)
void PRVM_Globals_Query (prvm_prog_t *prog, feed_fn_t myfeed_shall_stop)
{
	char vabuf[1024];
	char valuebuf[MAX_INPUTLINE_16384]; // QuakeC values written to this.
//	int count = 0;
	for (int idx = 0; idx < prog->numglobaldefs; idx ++) {
		mdef_t *def = &prog->globaldefs[idx];

		//if (false == String_Match(s, name))
		//	continue;

		int type = def->type;

		// Baker: Type can be zero -- examples .. "self"
		const char *s_key = PRVM_GetString(prog, def->s_name);

		if (false == Have_Flag (def->type, DEF_SAVEGLOBAL))
			continue; // self, other, ...

		int vartype = Flag_Remove(type, DEF_SAVEGLOBAL);

		// Baker: What are the types that are rejected here? .. vector  cause xyz is wriiten
		// filter _x _y _z
		if (false == isin4 (vartype, ev_string_1, ev_float_2, ev_vector_3, ev_entity_4))
			continue;

		// Any of the following suffix we ignore, covered by vector.
		if (vartype == ev_float_2 && String_Ends_With_Caseless_3 (s_key, "_x", "_y", "_z"))
			continue;


//		if (developer_entityparsing.integer)
//			Con_PrintLinef ("PRVM_ED_WriteGlobals: at global %s", name);

		// Baker: prog->statestring provides the current action in case of a crash
		prog->statestring = va(vabuf, sizeof(vabuf), "PRVM_ED_WriteGlobals, name=%s", s_key);
		PRVM_UglyValueString (prog, (etype_t)vartype, (prvm_eval_t *)&prog->globals.fp[def->ofs], valuebuf, sizeof(valuebuf));
		prog->statestring = NULL;

		qbool shall_stop = myfeed_shall_stop (idx, s_key, valuebuf, NULL, NULL, NULL, vartype, 1, 2);
		if (shall_stop)
			break;
	}
//	return count;
}

WARP_X_ (PRVM_ED_FindField ZDev_Fields_Feed_Shall_Stop_Fn)
// sendentity is 348 aiment is 101 and 214
// ammo_cells is 69 and 235
// 0 is "", model index is 1.
// There are 77 required vars in quake .. 14 are vectors that would be 3 fields so add 14*2 28 = 105
// But I still can't figure out the pattern, anyway, check unique.  There are dups
// The last one is float	modelindex (1 and 318); to string_t	noise3 (119/372);
WARP_X_ (entvars_s)
WARP_X_ (VM_numentityfields VM_entityfieldtype ZDev_Fields_Feed_Shall_Stop_Fn)
void PRVM_Fields_Query (prvm_prog_t *prog, feed_fn_t myfeed_shall_stop)
{
	//int count = 0;
	// For reasons not known at this time
	// There are system fields ...
	for (int idx = 0; idx < prog->numfielddefs; idx ++) {
		mdef_t		*def	= &prog->fielddefs[idx];
		int			type	= def->type;
		int			vartype	= Flag_Remove(type, DEF_SAVEGLOBAL);
		const char	*s_key	= PRVM_GetString(prog, def->s_name);

//		if (false == Have_Flag (def->type, DEF_SAVEGLOBAL))
//			continue; // helps or not?

		//if (in_range(105,idx,110))
		//	int k = 4;
		//if (String_Match(s_key, "modelindex"))
		//	int j = 4;

		if (vartype == ev_float_2 && String_Ends_With_Caseless_3 (s_key, "_x", "_y", "_z"))
			continue;

		qbool shall_stop = myfeed_shall_stop (idx, s_key, NULL, NULL, NULL, NULL,
			vartype, def->type, 2);
		if (shall_stop)
			break;
	}

//	return count;
}



WARP_X_ (PRVM_ED_PrintEdicts_Either PRVM_ED_PrintNum PRVM_ED_Print)

// Do whatever save game does.
void PRVM_Entities_Query_Fieldname (prvm_prog_t *prog, feed2_fn_t myfeed_shall_stop, ccs *s_fieldname_or_null /*like classname*/)
{
	WARP_X_ (PRVM_ED_PrintNum)
	qbool shall_stop = false;
	for (int edict_num = 0 ; edict_num < prog->num_edicts ; edict_num ++) {
		prvm_edict_t	*ed = PRVM_EDICT_NUM(edict_num);

		for (int fieldnum = 1; fieldnum < prog->numfielddefs; fieldnum ++) {
			mdef_t	*d = &prog->fielddefs[fieldnum];
			const char	*s_key = PRVM_GetString(prog, d->s_name);

			//if (s_key == NULL) {
			//	int j = 5;
			//}

			// Baker: No field name?  Return on first field
			if (s_fieldname_or_null == NULL) {
				shall_stop = myfeed_shall_stop (prog, edict_num, s_key, "", NULL, NULL, NULL, 0, fieldnum, 2);
				break; // We want out
			}

			prvm_eval_t	*val = (prvm_eval_t *)(ed->fields.fp + d->ofs);

		// if the value is still all 0, skip the field
			int		vartype = Flag_Remove(d->type , DEF_SAVEGLOBAL);//& ~DEF_SAVEGLOBAL;
			int vector_idx;
			for (vector_idx = 0; vector_idx < prvm_type_size[vartype]; vector_idx ++) {
				if (val->ivector[vector_idx])
					break;
			}

			if (vector_idx == prvm_type_size[vartype])
				continue;

			//Flex_Writef (QUOTED_S " ", name);
			//char vabuf[1024];
			va_super (crash_location, 1024, "PRVM_ED_Write, ent=%d, name=%s", edict_num, s_key);
			prog->statestring = crash_location;

			char valuebuf[MAX_INPUTLINE_16384];
			char *s_value = PRVM_UglyValueString(prog, (etype_t)d->type,
				val, valuebuf, sizeof(valuebuf));

			if (s_key && s_fieldname_or_null && String_Starts_With (s_key, s_fieldname_or_null)) {
				shall_stop = myfeed_shall_stop (prog, edict_num, s_key, s_value, NULL, NULL, NULL, vartype, fieldnum, 2);
				if (shall_stop)
					break; // Then we break again
			}
			prog->statestring = NULL;
		} // each ? um ..

		if (shall_stop)
			break;

	} // each entity


}

char *PRVM_Entities_Query_EdictNum_ZAlloc (prvm_prog_t *prog, int edict_num, ccs *fieldname)
{
	WARP_X_ (PRVM_ED_PrintNum)
	//for (int edict_num = 0 ; edict_num < prog->num_edicts ; edict_num ++) {
		prvm_edict_t	*ed = PRVM_EDICT_NUM(edict_num);
//		PRVM_ED_PrintNum (prog, edict_num, shall_print_free, wildcard_fieldname, s_fieldname_partial, s_fieldvalue_partial);

		for (int fieldnum = 1; fieldnum < prog->numfielddefs; fieldnum ++) {
			mdef_t	*d = &prog->fielddefs[fieldnum];
			const char	*s_key = PRVM_GetString(prog, d->s_name);

	#if 0 // Let details decide this?
			if (strlen(name) > 1 && name[strlen(name)-2] == '_')
				continue;	// skip _x, _y, _z vars, and ALSO other _? vars as some mods expect them to be never saved (TODO: a gameplayfix for using the "more precise" condition above?)
	#endif

			prvm_eval_t	*val = (prvm_eval_t *)(ed->fields.fp + d->ofs);

		// if the value is still all 0, skip the field
			int		vartype = Flag_Remove(d->type , DEF_SAVEGLOBAL);//& ~DEF_SAVEGLOBAL;
			int vector_idx;
			for (vector_idx = 0; vector_idx < prvm_type_size[vartype]; vector_idx ++)
				if (val->ivector[vector_idx])
					break;

			if (vector_idx == prvm_type_size[vartype])
				continue;

			//Flex_Writef (QUOTED_S " ", name);
			//char vabuf[1024];
			va_super (crash_location, 1024, "PRVM_ED_Write, ent=%d, name=%s", edict_num, s_key);
			prog->statestring = crash_location;

			char valuebuf[MAX_INPUTLINE_16384];
			char *s_value = PRVM_UglyValueString(prog, (etype_t)d->type,
				val, valuebuf, sizeof(valuebuf));

			if (String_Match (s_key, fieldname)) {

				return Z_StrDup (s_value);
				//qbool shall_stop =
				//myfeed_shall_stop (edict_num, s_key, s_value, NULL, NULL, NULL, vartype, 1, 2);
				//if (shall_stop)
				//	break;
			}
			prog->statestring = NULL;
		} // each ? fieldnum
//	} // each entity

	return NULL;
}
