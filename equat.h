// equat.h

//#define ISVALUE(NODE)		((NODE)->opx == operator_value_neg2)	// Use ISOPERATOR == false instead
#define ISOPERATOR(NODE)	((NODE)->opx > operator_value_neg2)

// Baker: Exponent operator will not be implemented, move along ---> use math.pow ()
typedef enum {
// Bitwise NOT has caveats.  What is NOT 2?  Depends on bitwidth?  NOT 32767 produces -32768 for short
// (~x) == (-x-1) for every signed integer x, no matter the length of x in bytes, AND no matter the machine's endianness 
	operator_cfunc_neg3				= -3,
	operator_value_neg2				= -2,	// a variable like "a" or a number like "b"
	operator_group_neg1				= -1,	// a paren grouped string
	operator_none_0					= 0,

	operator_open_level_1			= 1,
	operator_close_level_2			= 2,	// Don't link this node.  But give it a word for parsing.

	operator_bitwise_not_6			= 6,	// "~"
	operator_logical_not_7			= 7,	// "!"
	operator_logical_convbool_8		= 8,	// "!!" (bool)
	operator_multiply_9				= 9,	// "*"
	operator_dividedouble_10		= 10, 	// "/"  10 / 10.0

// Shoehorn as C can't truly do this one
	operator_divideint_11			= 11,	// "floordiv" .. 512/8 in C

// In C99 - -5 % 10 should be? -5
// https://stackoverflow.com/questions/3609572/does-either-ansi-c-or-iso-c-specify-what-5-10-should-be
	operator_remainder_fmod_12		= 12, 	// "%" integer
    
// ambiguous ... it's really not a problem.  supposedly (num) + (string) = (num)  if someone wants something else, use cat or do we make an operator?
	operator_string_cat_13			= 13,	// + when both are strings
	operator_addition_14			= 14, 	// "+"
	operator_subtract_15			= 15,	// "-"
// Subtract can be morphed into negation or acquired into an inline constant.  - can be turned into negation, + can be turned into affirmation

	operator_affirmation_16			= 16, 	// "+" Superfluous +
	operator_negation_17			= 17, 	// "-"
            
// Trouble in paradise here too.  Is VR bitshift endangered?
	operator_bitwise_shiftleft_18	= 18,	// "<<" as 32-bit?  or as 64-bit?
	operator_bitwise_shiftright_19	= 19,	// ">>"
    
	operator_relational_lt_20		= 20,	// "<"
	operator_relational_le_21		= 21, 	// "<="
	operator_relational_gt_22		= 22,	// ">"
	operator_relational_ge_23		= 23,	// ">="
    
	operator_relational_eq_24		= 24,	// "=="
	operator_relational_ne_25		= 25,	// !=
	operator_bitwise_and_26			= 26,	// "&"

// XOR with negative and positive numbers depends on bitwidth.
	operator_bitwise_xor_27			= 27,	// "^"
	operator_bitwise_or_28			= 28,	// "|"
    
// Shortcircuiting logical operators do not exist in QuakeC
	operator_logical_and_29			= 29,	// "&&"
	operator_logical_or_30			= 30,	// "||"

	operator_conditional_31			= 31,	// ?
	operator_conditional_colon_32   = 32,	// :
	operator_exponent_33			= 33,	// :

} operator_e;


#define eqnodetype_variable_neg1		-1
#define eqnodetype_invalid_0			0
#define eqnodetype_operator_solo_1		1
#define eqnodetype_operator_dual_2		2
#define eqnodetype_operator_ternary_3	3

#define eqnodetype_ei int	// ei = enumeration integer

WARP_X_ (operator_num_nodes)

#define depth_0 0

typedef double (*cfunc_fn_t) (double d);

typedef struct {
	ccs			*cfuncname;
	cfunc_fn_t	cfn;
} cfunc_t;

void cfunctions_condump_as_line(void);
cfunc_t *cfunctions_lookup(ccs *s);

typedef struct {
	ccs			*cconstant;		// pi
	double		val;			// 3.1415 ... M_PI
} cconstant_t;

cconstant_t *cconstants_lookup(ccs *s);

void cconstants_condump_as_line(void);


typedef struct _eq_node_st { // p_ent
	REF___		struct _eq_node_st		*parent;		// Only one node doesn't have one.
	REF___		struct _eq_node_st		*branches[8];
				int						branches_count;

				double					val;			// processed

				cfunc_t					*cFunc;
				cconstant_t				*cConst;

	REF___		struct _eq_node_st		*open_paren;	// "(" stores
	REF___		struct _eq_node_st		*close_paren;	// ")" stores

	ALLOC___	ccs						*word;			// ALLOC!
				int						word_idx;		// Our idx
				word_e					word_type;		// word_alphanumeric_1, word_numeric_2, ..
				int						word_flags;		// Floating, etc.

	REF___		ccs						*s_start;
	REF___		ccs						*s_beyond;

				operator_e				opx;			WARP_X_ (operator_group_neg1 operator_value_neg2)
				int						opx_level;		WARP_X_ (operator_precedence)
				eqnodetype_ei			operand_count;	// MAX_BRANCHES

				int						depth_level;	// PARSE depth level while parsing
// WORK VARS
				int						depth_testing;

				int						is_constant_value;

	ALLOC___	struct _equat_st		*equation_a;
	ALLOC___	ccs 					*s_inside_parens;
} eq_node_t;

void node_free(eq_node_t *pbr);



typedef struct _equat_st {
	// maxstrings changes as needed, causing reallocation of strings[] array
				int						maxsize;
				int						count;
	ALLOC___	eq_node_t				*nodes;					// p_ent (PAGE)

	REF___		eq_node_t				*head_node;				// REF EQ should get to know this.
	REF___		eq_node_t				*plast;					// REF

				int						into_source_recursive;	//

	REF___		ccs						*s_errstart;
	REF___		ccs						*s_errbeyond;
				int						errorcode;
				char					error_description[256];

				int						eq_parse_depth;			// depth while parsing - for parens

	REF___		struct _eq_node_st		*open_paren;	// "(" stores
	REF___		struct _eq_node_st		*close_paren;	// ")" stores

	// WORKVARS
				int						num_depths;

				int						exec_is_constant_check;
} equat_t;

eq_node_t *equat_calc (equat_t *plist);
eq_node_t *equat_add_node (equat_t *plist);
eq_node_t *equat_add (equat_t *plist);

void equatfreecontents (equat_t *plist);

int operator_precedence (operator_e opx);
operator_e operator_for_atom (ccs *s);
eqnodetype_ei operator_num_nodes (operator_e opx);

void equat_condump (equat_t *eq);
void equat_condump_detail (equat_t *eq); // Detail
void equat_condump_tree (equat_t *eq); // Detail

void equat_run (equat_t *eq);

void equat_raise_error_at_parse_f (equat_t *eq, ccs *fmt, ...) DP_FUNC_PRINTF(2);
void equat_raise_error_at_node_f (equat_t *eq, eq_node_t *pbr, ccs *fmt, ...) DP_FUNC_PRINTF(3);

eq_node_t *equat_add_word (equat_t *eq, ccs *s, ccs *s_start, ccs *s_beyond, operator_e opx, word_e wt);

eq_node_t *branch_dualist_attach_ret_self (equat_t *eq, eq_node_t *pbr);
eq_node_t *branch_soloist_attach_ret_self (equat_t *eq, eq_node_t *pbr);
eq_node_t *branch_assume_head_ret_self (equat_t *eq, eq_node_t *pbr);

void equat_depth_calc_recursive (equat_t *eq, eq_node_t *pbr, int depth);

int equat_calc_result (equat_t *eq, ccs *s_equation, double *pout, char *errbuff, size_t errbuffsize);




