// eq_c_constants.c.h


//typedef struct {
//	ccs			*cconstant;		// pi
//	double		val;			// 3.1415 ... M_PI
//} cconstant_t;

#define CCONST_HERE(CNAME, CFN) \
	{ CNAME, CFN } // Ender

cconstant_t cconstants[] = {
	CCONST_HERE (  "pi",		M_PI	),			// gcc 3.14159265358979323846
};

int cconstants_count = ARRAY_COUNT(cconstants);

void cconstants_condump_as_line(void)
{
	for (int idx = 0; idx < cconstants_count; idx ++) {
		cconstant_t *cconstant = &cconstants[idx];
		Con_Printf (" " "%s", cconstant->cconstant);
	}
}

cconstant_t *cconstants_lookup(ccs *s)
{
	for (int idx = 0; idx < cconstants_count; idx ++) {
		cconstant_t *cconst = &cconstants[idx];
		if (String_Match (s, cconst->cconstant)) {
			// Function
			return cconst;
		}
	}
	return NULL;
}


