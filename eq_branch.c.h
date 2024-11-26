// eq_branch.c.h


static void br_execute_variable_or_inline (equat_t *eq, eq_node_t *pbr)
{
	if (eq->exec_is_constant_check) {
		if (pbr->word_type == word_numeric_2) {
			pbr->is_constant_value = true;
			return; // Get out, touch nothing.
		}
		
		// TODO: Check const equation here
		pbr->is_constant_value =  false;
		return;
	}

	switch (pbr->word_type) {
	default:
		Con_PrintLinef (CON_CYAN "Unknown word type");
		break;

	case word_alphanumeric_1:
		if (pbr->cConst) {
			// Baker: PI or such
			pbr->val = pbr->cConst->val;
			break;
		}
		if (pbr->cFunc) {
			size_t size_eq = sizeof(*pbr->equation_a);
			pbr->equation_a = (equat_t *)Mem_Alloc (zonemempool, size_eq );
			double dbl_result = -9999;
			char errbuf[2048];
			int errcode = equat_calc_result (pbr->equation_a, pbr->s_inside_parens, &dbl_result, errbuf, sizeof(errbuf));

			if (errcode) {
				Con_PrintLinef (CON_CYAN "Failed");
				Con_PrintLinef (CON_CYAN "%s", errbuf);
				eq->errorcode = 1;
				break;
			} 
			// SUCCESS
			double dblcfuncresult;
			dblcfuncresult = pbr->cFunc->cfn(dbl_result);
			pbr->val = dblcfuncresult;
			break;
		}
		
		Con_PrintLinef (CON_CYAN "Error alphanumeric variables not support yet");
		
		break;

	case word_numeric_2:
		if (String_Starts_With (pbr->word, "0x")) {
			// Convert the hexadecimal string to a long long integer
			int64_t ourhex64 = STRTOI64_FN (pbr->word, /*end pointer*/ NULL, HEX_BASE_16);   // aka strtoll str to long long
			pbr->val = ourhex64;
		} else {
			pbr->val = atof (pbr->word);
		}
		break;
	}

	// TODO: Lookup alphanumerics
}


static void br_execute_fork_recursive (equat_t *eq, eq_node_t *pbr);

typedef void (*i_func_solo_t) (operator_e opcode, const double *pa, double *out, int *perrcode);
typedef void (*i_func_dual_t) (operator_e opcode, const double *pa, const double *pb, double *out, int *perrcode);

WARP_X_ (i_math_solo, i_math_dual)


static void br_execute_operator_solo_recursive (equat_t *eq, eq_node_t *pbr)
{
	if (!pbr->branches[0]) {
		Con_PrintLinef ("Error solo has no branch 0");
		return;
	}

	eq_node_t *pbr_a = pbr->branches[0];
	
	br_execute_fork_recursive (eq, pbr_a); // REFRESH A - must occur before slot query

	if (eq->exec_is_constant_check) {
		pbr->is_constant_value = pbr_a->is_constant_value;
		return; // GET OUT!
	}

	WARP_X_ (i_math_solo)
	
	i_func_solo_t myFunc = (i_func_solo_t)operator_get_func(pbr->opx);

	myFunc (pbr->opx, &pbr_a->val, &pbr->val, &eq->errorcode); // LIVE
}



static void br_execute_operator_dual_recursive (equat_t *eq, eq_node_t *pbr)
{
	i_func_dual_t myFunc = (i_func_dual_t)operator_get_func(pbr->opx);
	eq_node_t *pbr_a = pbr->branches[0];
	eq_node_t *pbr_b = pbr->branches[1];

	br_execute_fork_recursive (eq, pbr_a); // REFRESH A - must occur before slot query
	br_execute_fork_recursive (eq, pbr_b); // REFRESH B - must occur before slot query

	if (eq->exec_is_constant_check) {
		pbr->is_constant_value = (pbr_a->is_constant_value && pbr_b->is_constant_value);
		return; // GET OUT!
	}
	
	myFunc (pbr->opx, &pbr_a->val, &pbr_b->val, &pbr->val, &eq->errorcode); // LIVE
}

static void br_execute_operator_dual_shorting_recursive (equat_t *eq, eq_node_t *pbr)
{
	eq_node_t *pbr_a = pbr->branches[0];
	eq_node_t *pbr_b = pbr->branches[1];

	br_execute_fork_recursive (eq, pbr_a);

	if (eq->exec_is_constant_check) {
		br_execute_fork_recursive (eq, pbr_b);
		pbr->is_constant_value = (pbr_a->is_constant_value && pbr_b->is_constant_value);
		return; // GET OUT!
	}
	int did_short = i_logical_dual_shortcircuiting_first_did_short (pbr->opx, &pbr_a->val, &pbr->val, &eq->errorcode);
	if (did_short)
		return; // We short-circuited!  Value was written
	
	br_execute_fork_recursive (eq, pbr_b);

	i_logical_dual_shortcircuiting_second (pbr->opx, &pbr_b->val, &pbr->val, &eq->errorcode);
}

WARP_X_CALLERS_ (equat_run)
WARP_X_ (branch_soloist_attach_ret_self branch_dualist_attach_ret_self)

static void br_execute_fork_recursive (equat_t *eq, eq_node_t *pbr)
{
	if (ISOPERATOR(pbr) == false)		br_execute_variable_or_inline (eq, pbr);
	else if (pbr->operand_count == 1)	br_execute_operator_solo_recursive (eq, pbr);
	else if (isin2(pbr->opx, 
		operator_logical_and_29, 
		operator_logical_or_30))		br_execute_operator_dual_shorting_recursive (eq, pbr);
	else if (pbr->operand_count == 2)	br_execute_operator_dual_recursive (eq, pbr);
	
	else {
		Con_PrintLinef ("%s: Error occurred operator type unhandled", __func__);
	}
}



