// eq_calcs.c.h

void i_math_solo (operator_e opcode, const double *pa, double *pout, int *perrcode)
{
	double d = 0;
	switch (opcode) {
	default:									*perrcode = 1; return;
	case_break operator_negation_17:			d = -*pa;
	case_break operator_affirmation_16:			d = *pa;
	}

	// Transport ...
	if (DOUBLE_IS_NAN(d)) { *perrcode = 1; return; } // Baker: Negation at a boundary?
	*pout = d;
}

void i_logical_solo (operator_e opcode, const double *pa, double *pout, int *perrcode)
{
	double d;
	switch (opcode) {
	default:									*perrcode = 1; return;
	case_break operator_logical_not_7:			d = !*pa;
	}
	// Transport ...
	if (DOUBLE_IS_NAN(d)) { *perrcode = 1; return; } // Baker: Negation at a boundary?
	*pout = d;
}

#define did_short_true		true
#define did_not_short_false false

// Baker: There is super strange case I'm not sure this addresses
// c = c || c // Dumb but possible.
// c = c && c // Dumb but possible.
qbool i_logical_dual_shortcircuiting_first_did_short (operator_e opcode, const double *pa, double *pout, int *perrcode)
{	
	switch (opcode) {
	default:							*perrcode = 1; return false;
	case_break operator_logical_and_29:	
		// x == 1 && y == 1
		if (*pa == (double)false) {
			// Automatic false because first leg is false.
			*pout = false; // RESULT SET TO FALSE without need to check b.
			return did_short_true; // Short circuited evaluates to false
		}
		break;
	case_break operator_logical_or_30:	
		if (*pa) {
			*pout = true; // RESULT SET TO TRUE without need to check b.
			return did_short_true; // Short circuited evaluates to true
		}
		break;
	}

	// We didn't short-circuit ... no value is set ... leg 2 will decide all.
	return did_not_short_false;
}

void i_logical_dual_shortcircuiting_second (operator_e opcode, const double *pb, double *pout, int *perrcode)
{
	switch (opcode) {
	default:							*perrcode = 1; return;
	case_break operator_logical_and_29:	*pout = !!*pb;	// 1 && 1 ... first leg was true.  If we are true,  result is true.
	case_break operator_logical_or_30:	*pout = !!*pb;	// 0 || 1 ... first leg was false. If we are true,  result is true.
	}
}


// If both operands are int32 or lower, use int.  Otherwise use int64
void i_bitwise_dual (operator_e opcode, const double *pa, const double *pb, double *pout, int *perrcode)
{	
	double d;
	int32_t a = (int32_t)*pa;
	int32_t b = (int32_t)*pb;
	
	switch (opcode) {
	default:									*perrcode = 1; return;
	case_break operator_bitwise_and_26:			d = a  &   b;
	case_break operator_bitwise_xor_27:			d = a  ^   b;
	case_break operator_bitwise_or_28:			d = a  |   b;
	case_break operator_bitwise_shiftleft_18:	d = a  <<  b;
	case_break operator_bitwise_shiftright_19:	d = a  >>  b;
	}
	// Transport ...
	if (DOUBLE_IS_NAN(d)) { *perrcode = 1; return; } // Baker: Negation at a boundary?
	*pout = d;
}

void i_relational_dual (operator_e opcode, const double *pa, const double *pb, double *pout, int *perrcode)
{
	switch (opcode) {
	default:									*perrcode = 1; return;
    case_break operator_relational_lt_20:		*pout = *pa <  *pb;
    case_break operator_relational_le_21:		*pout = *pa <= *pb;
    case_break operator_relational_gt_22:		*pout = *pa >  *pb;
	case_break operator_relational_ge_23:		*pout = *pa >= *pb;
    case_break operator_relational_eq_24:		*pout = *pa == *pb;
    case_break operator_relational_ne_25:		*pout = *pa != *pb;
	}
	//if (DOUBLE_IS_NAN(d)) { *perrcode = 1; return; } // Baker: Should be impossible ...
}


void i_bitwise_not_solo (operator_e opcode, const double *a, double *pout, int *perrcode)
{
	*pout = ~((int32_t)*a);
}

void i_math_dual (operator_e opcode, const double *pa, const double *pb, double *pout, int *perrcode)
{
	double d;
	switch (opcode) {
	default:								*perrcode = 1; return;
	case_break operator_multiply_9:			d = *pa * *pb;
	case_break operator_addition_14:		d = *pa + *pb;				// NOTE: Complications if string.
	case_break operator_subtract_15:		d = *pa - *pb;
	case_break operator_dividedouble_10:	if (*pb == 0) { *perrcode = 1; return; }
											d = *pa / *pb;
	case_break operator_divideint_11:		if ((int64_t) *pb == 0) { *perrcode = 1; return; }
											d =  (int64_t)*pa / (int64_t)*pb;
	case_break operator_exponent_33:		d = pow(*pa, *pb);
	case_break operator_remainder_fmod_12:	if (*pb == 0) { *perrcode = 1; return; }
											d =  fmod(*pa, *pb);
	}
	if (DOUBLE_IS_NAN(d)) {
		*perrcode = 1; return;
	}
	*pout = d;
}


