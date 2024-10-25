// zeq_tests.c.h


typedef struct {
	double result;
	ccs		*s_equation;
} battery_t;

		#define FORMULA_CHECK(ANSWER_DOUBLE, FORMULA) \
			{ ANSWER_DOUBLE, FORMULA } // Ender

battery_t battery[] = {
        FORMULA_CHECK (1,     "0 < 1 "					),
        FORMULA_CHECK (1,     "1 > 0 "					),
        FORMULA_CHECK (1,     "1 <= 1 "					),
        FORMULA_CHECK (1,     "1 <= 2 "					),
        FORMULA_CHECK (1,     "2 > 1 "					),
        FORMULA_CHECK (1,     "2 >= 2 "					),
        FORMULA_CHECK (1,     "(1 & 1) "				),
        FORMULA_CHECK (1,     "(0 | 1) "				),
        FORMULA_CHECK (1,     "(1 | 0) "				),
        FORMULA_CHECK (1,     "0 == (1 & 0) "			),
        FORMULA_CHECK (1,     "0 == (0 & 1) "			),
        
        FORMULA_CHECK (1,     "1 == (4 == 2 + 2) "		),
        FORMULA_CHECK (1,     "1 == 1 "					),
        FORMULA_CHECK (1,     "0 != 1 "					),

        // Math like
        FORMULA_CHECK (20,    "10 + 10 "                ),
        FORMULA_CHECK (20,    "5 * 4 "                  ),
        FORMULA_CHECK (20,    "21 - 1 "                 ),
        FORMULA_CHECK (5,     "20 / 4 "                 ),
		FORMULA_CHECK (0.5,   "20 / 40 "                ),	// Float division QuakeC style

        FORMULA_CHECK (0.2,    "1 / 5 "					),  // C doesn't assume floating point with division.

        FORMULA_CHECK (0,     "4 % 4 "					),
        FORMULA_CHECK (1,     "1 % 4 "					),
        FORMULA_CHECK (3,     "3 % 4 "					),
        FORMULA_CHECK (1,     "5 % 4 "					),
		FORMULA_CHECK (0.5,   "4.5 % 4 "				),	// % is fmod in QuakeC
        
        // Order like
        FORMULA_CHECK (20,    "10 + 5 * 2 "				),
        FORMULA_CHECK (21,    "3 * (3 + 4) "			),
        FORMULA_CHECK (13,    "10 + 6 / 2 "				),

        // Syntax like
        FORMULA_CHECK (7,     "(+ + +7) "				),
        FORMULA_CHECK (-7,    "(- - -7) "				),
        FORMULA_CHECK (14,    "(7 - -7) "				),
        FORMULA_CHECK (14,    "+(7 - -7) "				),
        FORMULA_CHECK (14,    "+(+7 - -7) "				),
        FORMULA_CHECK (0,     "+(-7 - -7) "				),
        FORMULA_CHECK (0,     "+(-7 - -7) "				),
        FORMULA_CHECK (14,    "-(-7 - - -7) "			),
        FORMULA_CHECK (14,    "(7 + +7) "				),
        FORMULA_CHECK (14,    "(7 - -7) "				),
        FORMULA_CHECK (14,    "(7 + +7) "				),
        FORMULA_CHECK (-49,   "(7 *-7) "				),
        FORMULA_CHECK (49,    "(7 *+7) "				),
        FORMULA_CHECK (-49,   "(7 *- 7) "				),
        FORMULA_CHECK (49,    "(7 *+ 7) "				),

        // Bitwise
        FORMULA_CHECK (2,     "(3 & 2) "				),
        FORMULA_CHECK (4,     "(5 & 6) "				),
        FORMULA_CHECK (7,     "(5 | 6) "				),

		// Right to leftness
		FORMULA_CHECK (0,     "1 == 4 == 2 + 2"			), // Baker: allegedly this should be false

		// C99ness
		FORMULA_CHECK (-5,    "-5 % 10"					), // C99 must be -5
};

void SCR_eqtests_f (cmd_state_t *cmd)
{
	int count = ARRAY_COUNT (battery);
	int numfails = 0;

	for (int n = 0; n < count; n ++) {
		battery_t *b = &battery[n];
		ccs *sxy = b->s_equation;
		Con_PrintLinef ("Test %3d of %3d: " QUOTED_S, n, count, b->s_equation);

		double dbl_result = -9999;
		char errbuf[2048];
		equat_t _eq = {0};
		int errcode = equat_calc_result (&_eq, b->s_equation, &dbl_result, errbuf, sizeof(errbuf));

		if (errcode) {
			Con_PrintLinef (CON_CYAN "Failed");
			Con_PrintLinef (CON_CYAN "%s", errbuf);
			numfails ++;
		} else {
			if (dbl_result == b->result) {
				Con_PrintLinef (CON_GREEN "Result ok! " FLOAT_LOSSLESS_FORMAT, (float)dbl_result );

			} else {
				Con_PrintLinef (CON_BRONZE "Expected " FLOAT_LOSSLESS_FORMAT " Result = " FLOAT_LOSSLESS_FORMAT " (cvar _last has been set)", (float)b->result, (float)dbl_result );
				numfails ++;
			}
			
		}
	}

	if (numfails == 0) {
		Con_PrintLinef (CON_GREEN "%3d tests NO FAILS", count);
	} else {
		Con_PrintLinef (CON_CYAN "%3d tests with %d fails", count, numfails);
	}
}