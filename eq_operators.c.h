// eq_operators.c.h

void *operator_get_func (operator_e opx)
{
	switch (opx) {
	case operator_bitwise_not_6:			return i_bitwise_not_solo;
	case operator_logical_not_7:			return i_logical_solo;
	case operator_logical_convbool_8:		return i_logical_solo;
	case operator_multiply_9:				return i_math_dual;
	case operator_dividedouble_10:			return i_math_dual;
	case operator_divideint_11:				return i_math_dual;
	case operator_remainder_fmod_12:		return i_math_dual;
	case operator_string_cat_13:			return NULL;
	case operator_addition_14:				return i_math_dual;
	case operator_subtract_15:				return i_math_dual;
	case operator_affirmation_16:			return i_math_solo;
	case operator_negation_17:				return i_math_solo;
	case operator_bitwise_shiftleft_18:		return i_bitwise_dual;
	case operator_bitwise_shiftright_19:	return i_bitwise_dual;
	case operator_relational_lt_20:			return i_relational_dual;
	case operator_relational_le_21:			return i_relational_dual;
	case operator_relational_gt_22:			return i_relational_dual;
	case operator_relational_ge_23:			return i_relational_dual;
	case operator_relational_eq_24:			return i_relational_dual;
	case operator_relational_ne_25:			return i_relational_dual;
	case operator_bitwise_and_26:			return i_bitwise_dual;
	case operator_bitwise_xor_27:			return i_bitwise_dual;
	case operator_bitwise_or_28:			return i_bitwise_dual;
	case operator_logical_and_29:			return NULL; // Special
	case operator_logical_or_30:			return NULL; // Special
	case operator_conditional_31:			return NULL; // Not yet ...
	case operator_conditional_colon_32:		return NULL; // Not yet ...
	case operator_exponent_33:				return i_math_dual;

	}
	return NULL;
}

// 2 * (2 + 2)
// + has open level parent.

// lower means priority
int operator_precedence (operator_e opx)
{
	switch (opx) {
	//case operator_open_level_1:			return 1;
	//case operator_close_level_2: 			return 1;

	case operator_bitwise_not_6:			return 2;
	case operator_logical_not_7:			return 2;
	case operator_logical_convbool_8:		return 2;

	case operator_affirmation_16:			return 2;
	case operator_negation_17:				return 2;

	case operator_exponent_33:				return 3;

	case operator_multiply_9:				return 4;
	case operator_dividedouble_10:			return 4;
	case operator_divideint_11:				return 4;
	case operator_remainder_fmod_12:		return 4;

	case operator_string_cat_13:			return 5;
	case operator_addition_14:				return 5;
	case operator_subtract_15:				return 5;

	case operator_bitwise_shiftleft_18:		return 6;
	case operator_bitwise_shiftright_19:	return 6;

	case operator_relational_lt_20:			return 7;
	case operator_relational_le_21:			return 7;
	case operator_relational_gt_22:			return 7;
	case operator_relational_ge_23:			return 7;

	case operator_relational_eq_24:			return 8;
	case operator_relational_ne_25:			return 8;

	case operator_bitwise_and_26:			return 9;

	case operator_bitwise_xor_27:			return 10;

	case operator_bitwise_or_28:			return 11;

	case operator_logical_and_29:			return 12;

	case operator_logical_or_30:			return 13;
	}
	return not_found_neg1;
}

operator_e operator_for_atom (ccs *s)
{
	if (String_Match (s, "("))			return operator_open_level_1;
	if (String_Match (s, ")"))			return operator_close_level_2;

	if (String_Match (s, "~"))			return operator_bitwise_not_6;
	if (String_Match (s, "!"))			return operator_logical_not_7;
	if (String_Match (s, "!!"))			return operator_logical_convbool_8;
	if (String_Match (s, "*"))			return operator_multiply_9;
	if (String_Match (s, "/"))			return operator_dividedouble_10;
	if (String_Match (s, "floordiv"))	return operator_divideint_11;
	if (String_Match (s, "%"))			return operator_remainder_fmod_12;
	//if (String_Match (s, "+"))		return operator_string_cat_13; // Both must string
	if (String_Match (s, "+"))			return operator_addition_14;
	if (String_Match (s, "-"))			return operator_subtract_15;
	if (String_Match (s, "+"))			return operator_affirmation_16;	// superfluous + .. (+6 + 6)
	if (String_Match (s, "-"))			return operator_negation_17;	// sign - .. (-6 * 6)
	if (String_Match (s, "<<"))			return operator_bitwise_shiftleft_18;
	if (String_Match (s, ">>"))			return operator_bitwise_shiftright_19;

	if (String_Match (s, "<"))			return operator_relational_lt_20;
	if (String_Match (s, "<="))			return operator_relational_le_21;
	if (String_Match (s, ">"))			return operator_relational_gt_22;
	if (String_Match (s, ">="))			return operator_relational_ge_23;
	if (String_Match (s, "=="))			return operator_relational_eq_24;
	if (String_Match (s, "!="))			return operator_relational_ne_25;
	if (String_Match (s, "&"))			return operator_bitwise_and_26;
	if (String_Match (s, "^"))			return operator_bitwise_xor_27;
	if (String_Match (s, "|"))			return operator_bitwise_or_28;
	if (String_Match (s, "&&"))			return operator_logical_and_29;
	if (String_Match (s, "||"))			return operator_logical_or_30;

	if (String_Match (s, "pow"))		return operator_exponent_33;

	return operator_none_0;
}

eqnodetype_ei operator_num_nodes (operator_e opx)
{
	switch (opx) {
	case operator_none_0:				return 0;
	//case operator_open_level_1:		return eqnodetype_operator_solo_1; // Should never get here at all
	case operator_close_level_2:		return eqnodetype_operator_solo_1;
	case operator_bitwise_not_6:		return eqnodetype_operator_solo_1;	// solo
	case operator_logical_not_7:		return eqnodetype_operator_solo_1;	// solo
	case operator_logical_convbool_8:	return eqnodetype_operator_solo_1;	// solo
	case operator_affirmation_16:		return eqnodetype_operator_solo_1;	// solo
	case operator_negation_17:			return eqnodetype_operator_solo_1;	// solo
	}
	return eqnodetype_operator_dual_2;
}


