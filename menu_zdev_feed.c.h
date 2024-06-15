// menu_zdev_feed.c.h

// printfn_t
// Returns shall stop
qbool ZDev_Globals_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	if (mz->is_more_detail == false) {
		if (n0 == ev_string_1 && strlen(value) == 0)
			return false; // Empty string
		if (value[0] == '0' && value[1] == NULL_CHAR_0)
			return false; // Value of 0
	}
	va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	stringlistappend (&mz->list, sline);
	return false; // Shall stop
}



static ccs *VarType_For_EV (int ev)
{
	switch (ev) {
	case 0:				return "(type zero)";
	case ev_string_1:	return "string";
	case ev_float_2:	return "float";
	case ev_vector_3:	return "vector";
	case ev_entity_4:	return "entity";
	case ev_function_6:	return "function";
	case ev_pointer_7:	return "pointer";
	} // sw
	return "(not type 1 to 7)";
}

WARP_X_ (PRVM_Fields_Query Data_Dirty_Check)
qbool ZDev_Fields_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	if (key[0] == NULL_CHAR_0)
		return false;

	//CREATE TABLE CUSTOMERS ( ID INT NOT NULL, NAME VARCHAR (20) NOT NULL, AGE INT NOT NULL, ADDRESS CHAR (25), SALARY DECIMAL (18, 2), PRIMARY KEY (ID) );
	// classname, origin, model?
	ccs *vartype_str = VarType_For_EV (n0);
	
	va_super (sline, 1024, S_FMT_LEFT_PAD_20 " %4d t: %4d" S_FMT_LEFT_PAD_40  , key, idx, (int)n1, vartype_str);
	stringlistappend (&mz->list, sline);

	// What is our plan to get only the fields we want here?
	// Like column query ... hmmm ... 
	// SQL let's you pick field

	return false; // Shall stop
}


qbool ZDev_Entities_Feed_Shall_Stop_Fn (prvm_prog_t *prog, int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	if (key[0] == NULL_CHAR_0)
		return false;

	//ccs *vartype_str = VarType_For_EV (n0);
	
	va_super (sline, 1024, " %4d " S_FMT_LEFT_PAD_20, idx, /*key*/ value);
	stringlistappend (&mz->list, sline);
	return false; // Shall stop
}

WARP_X_ (Data_Dirty_Check)
qbool ZDev_Shaders_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] != NULL_CHAR_0)
		stringlistappend (&mz->list, key);
	return false; // Shall stop
}

void R_GetListWorldTextures_Feed (feed_fn_t myfeed_shall_stop);
qbool ZDev_WorldTextures_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] != NULL_CHAR_0)
		stringlistappend (&mz->list, key);
	return false; // Shall stop
}

qbool ZDev_Models_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	//va_super (sline, 1024, S_FMT_LEFT_PAD_20 " " S_FMT_LEFT_PAD_40, key, value);
	if (key[0] == NULL_CHAR_0)
		return false;

	// No detail then don't do worldmap brush models like *40 (door or such)
	if (mz->is_more_detail == false)
		if (key[0] == '*')
			return false;	
	
	stringlistappend (&mz->list, key);
	
	return false; // Shall stop
}

qbool ZDev_Sounds_Feed_Shall_Stop_Fn (int idx, ccs *key, ccs *value, ccs *a, ccs *b, ccs *c, 
							   int64_t n0, int64_t n1, int64_t n2)
{
	if (key[0] == NULL_CHAR_0)
		return false;
	
	stringlistappend (&mz->list, key);
	
	return false; // Shall stop
}
