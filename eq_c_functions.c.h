// eq_c_functions.c.h



static double c_abs(double d)		{ return abs(d);				}
static double c_atan(double d)		{ return atan(d);				}
static double c_ceil(double d)		{ return ceil(d);				}
static double c_cos(double d)		{ return cos(d);				}
static double c_deg2rad(double deg)	{ return deg * (M_PI / 180.0);	}  // 360 range to 7.28__ range (2PI)
static double c_floor(double d)		{ return floor(d);				}
static double c_log(double d)		{ return log(d);				}
static double c_rad2deg(double rad)	{ return rad * (180.0 / M_PI);	}	// 7.28__ to 360 range (2PI)
static double c_sin(double d)		{ return sin(d);				}
static double c_sqrt(double d)		{ return sqrt(d);				}
static double c_tan(double d)		{ return tan(d);				}

#define CFUNC_HERE(CNAME, CFN) \
	{ CNAME, CFN } // Ender

cfunc_t cfunctions[] = {
	CFUNC_HERE (  "abs",		c_abs		),
	CFUNC_HERE (  "atan",		c_atan		),
	CFUNC_HERE (  "ceil",		c_ceil		),
	CFUNC_HERE (  "cos",		c_cos		),
	CFUNC_HERE (  "deg2rad",	c_deg2rad	),
	CFUNC_HERE (  "floor",		c_floor		),
	CFUNC_HERE (  "log",		c_log		),
	CFUNC_HERE (  "rad2deg",	c_rad2deg	),
	CFUNC_HERE (  "sin",		c_sin		),
	CFUNC_HERE (  "sqrt",		c_sqrt		),
	CFUNC_HERE (  "tan",		c_tan		),
};

int cfunctions_count = ARRAY_COUNT(cfunctions);

void cfunctions_condump_as_line(void)
{
	for (int idx = 0; idx < cfunctions_count; idx ++) {
		cfunc_t *cfun = &cfunctions[idx];
		Con_Printf (" " "%s", cfun->cfuncname);
	}
}

cfunc_t *cfunctions_lookup(ccs *s)
{
	//cfunc_fn_t cfunc_to_use = NULL;
	for (int idx = 0; idx < cfunctions_count; idx ++) {
		cfunc_t *cfun = &cfunctions[idx];
		if (String_Match (s, cfun->cfuncname)) {
			// Function
			return cfun;
		}
	}
	return NULL;
}
