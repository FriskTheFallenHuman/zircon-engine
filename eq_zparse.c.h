// eq_zparse.c.h


#include "eq_c_functions.c.h"
#include "eq_c_constants.c.h"
#include "eq_calcs.c.h"
#include "eq_operators.c.h"

#include "eq_branch.c.h"
#include "eq_attach.c.h"

#include "eq_equations.c.h"


// returns SUCCESS_OK_0 or error code
int equat_calc_result (equat_t *eq, ccs *s_equation, double *pout, char *errbuff, size_t errbuffsize)
{
	ccs *s = s_equation; // Cmd_Argv(cmd, 1); // Arg 0 is cmd, arg 1
	ccs *data = s;
	operator_e opx;
	int num_nodes;

	int ret = SUCCESS_OK_0;

	ccs *len2_operators[] = {
		"!!", // Back to back logical not ===> convert boolean
		"<<",
		">>",
		"<=",
		">=",
		"==",
		"!=",
		"&&",
		"||",
	};
	int len2_operators_count = ARRAY_COUNT(len2_operators);
    word_e wordtype;
    int com_token_slen;
	while (wordtype = COM_ParseToken_EQ_Tokenize(&data, &eq->s_errstart, &eq->s_errbeyond)) {
		ccs *sxy = com_token;
		eq_node_t *pbr;
		//Con_PrintLinef ("Token: " QUOTED_S, sxy);

		switch (wordtype) {

		case word_numeric_2:
			if (eq->plast && eq->plast->opx == operator_value_neg2) {
				equat_raise_error_at_parse_f (eq, "Consecutive variables " QUOTED_S, sxy);
				break;
			}

acceptvar: // ADD A NODE
			pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, operator_value_neg2, wordtype);
			continue;

		case word_numericalpha_3:
		case word_string_5:
		case word_multichar_6:
		case word_invalid_9:
		case word_newline_10:
			equat_raise_error_at_parse_f (eq, "Received type %d", wordtype);
			break;

		case word_alphanumeric_1:

			if (String_Match(sxy, "pow")) {
				// Turn this into "exponent" operator
				opx = operator_exponent_33;
				num_nodes = operator_num_nodes (opx);
				goto dual2;
			}

			cconstant_t *cconstant = cconstants_lookup(sxy);
			if (cconstant) {
				// Add as an alphanumeric constant
				pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, operator_value_neg2, wordtype);
				pbr->cConst = cconstant;
				continue; // DONE
			}

			cfunc_t *cfunction = cfunctions_lookup (sxy);

			if (!cfunction) {
				equat_raise_error_at_parse_f (eq, "Received alphanumeric not matching known function", wordtype);
				break;
			}

accept_cfunc:
			pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, operator_value_neg2, wordtype);
			pbr->cFunc = cfunction;

			ccs *fakedata = data;
			COM_ParseToken_EQ_Tokenize(&fakedata, &eq->s_errstart, &eq->s_errbeyond);
			if (String_Match(com_token, "(") == false) {
				equat_raise_error_at_parse_f (eq, "Received alphanumeric not matching known function", wordtype);
				break;
			}

			ccs *paren_open_beyond = eq->s_errbeyond;
			ccs *paren_close_start = NULL;
			int fake_depth = 1;

			while (COM_ParseToken_EQ_Tokenize(&fakedata, &eq->s_errstart, &eq->s_errbeyond)) {
				if (String_Match(com_token, "("))
					fake_depth ++;
				else if (String_Match(com_token, ")")) {
					fake_depth --;
					if (fake_depth == 0) {
						paren_close_start = eq->s_errstart;
						break;
					}
				}
			} // While data and != depth 0

			if (!paren_close_start) {
				equat_raise_error_at_parse_f (eq, "No end ) for function", wordtype);
				break;
			}
			size_t slen3 = paren_close_start - paren_open_beyond;
			pbr->s_inside_parens = Z_StrDup_Len_Z (paren_open_beyond, slen3);
			WARP_X_ (node_free br_execute_variable_or_inline)
			// JIT ... pbr->equation_a = Mem_Alloc (zonemempool, sizeof(*pbr->equation_a) );
			// IF WE ARE HERE ...

			// Advance the cursor
			data = fakedata;

			continue;
		} // switch wordtype

		if (eq->errorcode)
			break;

		if (wordtype != word_punct_4) {
			// Should be impossible, we handled all the cases
			equat_raise_error_at_parse_f (eq, "Expected punct at " QUOTED_S, sxy); break;
		}

		com_token_slen = 0;
punct:
		// We have length 1 operator like "=".
		// TRY TO EXTEND OPERATOR .. "=" to "==" or ">" to ">="
		/*int */ com_token_slen = strlen(com_token);
		if (com_token_slen < (int)sizeof(com_token) - 1) {
			for (int n = 0; n < len2_operators_count; n ++) {
				ccs *sop2 = len2_operators[n];
				if (String_Starts_With(eq->s_errstart, sop2)) {
					com_token[com_token_slen++] = *data; *data ++;
					com_token[com_token_slen++] = 0;
					eq->s_errbeyond ++; // One more.
					break;
				}
			} // for
		}

		/*operator_e */ opx = operator_for_atom(sxy);

		// TRY TO MODIFY + or - from subtraction to negation if situation fits (-5 * 2).
		if (isin2(opx,operator_addition_14, operator_subtract_15)) {
			// If nothing behind us -a + 0 or operator behind us 7 -- 7, become negation/affirmation
			// if (!eq->plast || eq->plast->depth_level != eq->eq_parse_depth || (eq->plast->depth_level == eq->eq_parse_depth && ISOPERATOR(eq->plast)) ) {
			if (!eq->plast || eq->plast->depth_level < eq->eq_parse_depth || (eq->plast->depth_level == eq->eq_parse_depth && ISOPERATOR(eq->plast)) ) {
				if (opx == operator_addition_14)
					opx = operator_affirmation_16;
				else if (opx == operator_subtract_15)
					opx = operator_negation_17;
			}
		} // negation, affirmation

		if (opx == operator_none_0) {
			equat_raise_error_at_parse_f (eq, "Operator for " QUOTED_S " not known", sxy); break;
		}

		// REGULAR OPERATOR
		if (opx == operator_open_level_1) {
			eq_node_t *pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, opx, word_none_0); // ACCEPT
			eq->eq_parse_depth ++;
			continue;
		}

		if (opx == operator_close_level_2) {
			if (!eq->open_paren) {
				equat_raise_error_at_parse_f (eq, "Close parenthesis without open " QUOTED_S, sxy); break;
			}
			eq_node_t *pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, opx, word_none_0); // ACCEPT

			if (eq->eq_parse_depth == 0) {
				// REMOVE ME CAN'T HIT ANYMORE?
				equat_raise_error_at_parse_f (eq, "Close parenthesis without open " QUOTED_S, sxy); break;
			}
			eq->eq_parse_depth --;
			continue;
		}

		// We read num nodes early ... add node will do it.
		/*int */ num_nodes = operator_num_nodes (opx); // Good enough for now.

		if (isin2 (num_nodes, eqnodetype_operator_solo_1, eqnodetype_operator_dual_2) == false) {
			equat_raise_error_at_parse_f (eq, "Not ready for non-solo/non-dual operator yet " QUOTED_S, sxy);
			break;
		}

		if (num_nodes == eqnodetype_operator_dual_2) goto dual2;

		// SOLOIST
		// A soloist like "! a" expects another soloist to a variable to follow it
		// It must have it's node A filled.

solo:
		pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, opx, word_none_0); // ACCEPT
		continue;

		// DUALIST
dual2:
		// Baker: Some operators can lead an equation .. " !a " or " ~a "
		// But we aren't one of those cases here, we have an error
		if (eq->plast == NULL) {
			equat_raise_error_at_parse_f (eq, "Leading operator " QUOTED_S, sxy);
			break;
		}

		if (eq->plast && eq->plast->depth_level == eq->eq_parse_depth && eq->plast->opx != operator_value_neg2) {
			equat_raise_error_at_parse_f (eq, "Consecutive operators" QUOTED_S, sxy);  break;
		}

		pbr = equat_add_word (eq, sxy, eq->s_errstart, eq->s_errbeyond, opx, word_none_0); // ACCEPT

	} // while

	//if (eq->eq_parse_depth != 0) {
	if (!eq->errorcode && eq->open_paren /*eq->eq_parse_depth != 0*/) {
		equat_raise_error_at_node_f (eq, eq->open_paren, "Unclosed paren (1) " QUOTED_S, eq->open_paren->word);
		// No break ...
	}

	//if (eq-> open_paren /*eq->eq_parse_depth != 0*/) {
	//	equat_raise_error_at_node_f (eq, eq->open_paren, "Unclosed paren (2) " QUOTED_S, eq->open_paren->word);
	//	// No break ...
	//}

	if (!eq->errorcode && !eq->head_node) {
		va_super (line1, 1024, "%s", CON_CYAN "No headnode");

		strlcpy		(errbuff, "", errbuffsize);
		strlcat		(errbuff, line1, errbuffsize);

		*pout = -1;

		ret = 3;

		goto parse_error;
	}

	// Scan for open operand
	if (developer_execstring.integer) {
		Con_PrintLinef ("Tree:");
		equat_condump_tree (eq);
	}

	if (!eq->errorcode) {
		for (int j = 0; j < eq->count; j ++) {
			eq_node_t *pbr = &eq->nodes[j];
			if (pbr->branches_count != pbr->operand_count) {
				equat_raise_error_at_node_f (eq, pbr, "Trailing operator " QUOTED_S, pbr->word);
				break;
			} // if
		} // for
	}


	if (eq->errorcode) {
//error_code_happened:
		int into = eq->s_errstart - s_equation;
		int slen = eq->s_errbeyond - eq->s_errstart;
		char *sspaces_za = Z_StrRepeat_Z (SPACE_CHAR_32, into);
		char *sulines_za = Z_StrRepeat_Z ('_', slen);

		va_super (line1, 1024, "Line: %s\n", s_equation);
		va_super (line2, 1024, "      %s%s\n", sspaces_za, sulines_za);
		va_super (line3, 1024, CON_CYAN "Error: %s\n", eq->error_description);

		Mem_FreeNull_ (sspaces_za);
		Mem_FreeNull_ (sulines_za);

		strlcpy		(errbuff, "", errbuffsize);
		strlcat		(errbuff, line1, errbuffsize);
		strlcat		(errbuff, line2, errbuffsize);
		strlcat		(errbuff, line3, errbuffsize);

		*pout = -1;

		ret = eq->errorcode;

		goto parse_error;
	}


	// NO ERROR CODE
	equat_run (eq);

	if (eq->errorcode) {
		// RUN-TIME ERROR CODE
		va_super (line1, 1024, "%s", CON_CYAN "Run-time error (division by zero?)" );

		strlcpy		(errbuff, "", errbuffsize);
		strlcat		(errbuff, line1, errbuffsize);

		*pout = -1;

		ret = 5;

		goto runtime_error_happened;
	}

	*pout = eq->head_node->val;

parse_error:
	if (eq->errorcode && eq->head_node) {
		if (developer_execstring.integer) {
			Con_PrintLinef ("Detail:");
			equat_condump_detail (eq);
			Con_PrintLinef ("Tree:");
			equat_condump_tree (eq);
		}
	}

runtime_error_happened:
	equatfreecontents (eq);
	return ret;
}


