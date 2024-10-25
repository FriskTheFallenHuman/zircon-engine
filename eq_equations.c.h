// eq_lib.c.h

WARP_X_ (equat_add_word equatfreecontents)

// Baker: Only word is allocated.  October 12 2024
void node_free(eq_node_t *pbr)
{
	pbr->val	= 0.0;
	pbr->opx	= operator_none_0;
	Mem_FreeNull_ (pbr->word);
	Mem_FreeNull_ (pbr->s_inside_parens);

	if (pbr->equation_a) {
		equatfreecontents (pbr->equation_a);
		Mem_FreeNull_ (pbr->equation_a);
	}
}


WARP_X_ (entitylistfreecontents)

eq_node_t *equat_calc (equat_t *plist)
{
	return NULL;
}

eq_node_t *equat_add (equat_t *plist)
{
	if (plist->count >= plist->maxsize) {
		plist->maxsize += 4096;
		plist->nodes = (eq_node_t *)Mem_Realloc(zonemempool, 
			plist->nodes, plist->maxsize * sizeof(eq_node_t) );
	}
	
	eq_node_t *pe = &plist->nodes[plist->count ++]; // Incremented count
	return pe;
}




WARP_X_ (branch_soloist_attach_ret_self branch_dualist_attach_ret_self)
eq_node_t *equat_add_word (equat_t *eq, ccs *s, ccs *s_start, ccs *s_beyond, operator_e opx, word_e wt)
{
	int newidx = eq->count;
	eq_node_t *pbr		= equat_add (eq);

	pbr->word			= Z_StrDup (s);
	pbr->word_idx		= newidx;
	pbr->word_type		= wt;
	pbr->word_flags		= 0;
	pbr->s_start		= s_start;
	pbr->s_beyond		= s_beyond;
	pbr->depth_level	= eq->eq_parse_depth;
	pbr->opx			= opx;					// operator_group_neg1

	if (isin1 (opx, operator_open_level_1)) {
		// PUSH ... These are inert and for printing only.
		pbr->open_paren = eq->open_paren;
		eq->open_paren = pbr;
		
		return pbr;
	}
	if (isin1 (opx, operator_close_level_2)) {
		// POP ... These are inert and for printing only.
		eq_node_t *popen = eq->open_paren;

		popen->close_paren = pbr;

		eq->open_paren = popen->open_paren;
		
		return pbr;
	}



	if (ISOPERATOR(pbr))
		goto operator_type;

	// VALUE

	// Branch_Soloist_Insertion
	return eq->plast = branch_soloist_attach_ret_self (eq, pbr);

operator_type:
	// OPERATOR
	pbr->opx_level		= operator_precedence (opx);			// not_found_neg1
	pbr->operand_count	= operator_num_nodes (opx); 
	
	// Branch_Soloist_Insertion ?
	if (pbr->operand_count == 1)
		return eq->plast = branch_soloist_attach_ret_self (eq, pbr);

	if (pbr->operand_count == 2)
		return eq->plast = branch_dualist_attach_ret_self (eq, pbr);

	Con_PrintLinef ("ERROR - Operator not handled");
	return NULL;
}

void equat_raise_error_at_node_f (equat_t *eq, eq_node_t *pbr, ccs *fmt, ...)
{
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	eq->errorcode = 1; 
	eq->s_errstart = pbr->s_start;
	eq->s_errbeyond = pbr->s_beyond;
	c_strlcpy (eq->error_description, text);
	VA_EXPAND_ALLOC_FREE (text);
}

void equat_raise_error_at_parse_f (equat_t *eq, ccs *fmt, ...)
{
	VA_EXPAND_ALLOC (text, text_slen, bufsiz, fmt);
	eq->errorcode = 1; 
	c_strlcpy (eq->error_description, text);
	VA_EXPAND_ALLOC_FREE (text);
}

void equat_condump (equat_t *eq)
{
	for (int idx = 0; idx < eq->count; idx++) {
		eq_node_t *pbr = &eq->nodes[idx];
		ccs	*sxy = pbr->word;

		Con_PrintLinef ("%4d: " QUOTED_S "(%s)", idx, sxy, pbr->opx == operator_value_neg2 ? "value" : "operator");
	} // for

}

// Detail
void equat_condump_detail (equat_t *eq)
{
	if (eq->head_node)
		Con_PrintLinef ("Head # %d " QUOTED_S " val = " FLOAT_LOSSLESS_FORMAT, eq->head_node->word_idx, eq->head_node->word, eq->head_node->val);
	else Con_PrintLinef ("No head");

	Con_PrintLinef ("Num tiers = %d", eq->num_depths);
	Con_PrintLinef ("Num words count = %d", eq->count);
	for (int idx = 0; idx < eq->count; idx++) {
		eq_node_t *pbr = &eq->nodes[idx];
		ccs	*sxy = pbr->word;

		Con_PrintLinef ("%4d: " QUOTED_S "(%s)", idx, sxy, pbr->opx == operator_value_neg2 ? "value" : "operator");

		Con_PrintLinef ("%10s" " opx:                 %d", /*args*/	" ", pbr->opx);
		Con_PrintLinef ("%10s" " opx_level:           %d", /*args*/	" ", pbr->opx_level);
		Con_PrintLinef ("%10s" " pbr->operand_count:  %d", /*args*/	" ", pbr->operand_count);
		Con_PrintLinef ("%10s" " pbr->val:            " FLOAT_LOSSLESS_FORMAT, /*args*/	" ", pbr->val);
		Con_PrintLinef ("%10s" " pbr->branches_count: %d", /*args*/	" ", pbr->branches_count);
		Con_PrintLinef ("%10s" " pbr->depth_level:    %d", /*args*/	" ", pbr->depth_level);
		Con_PrintLinef ("%10s" " pbr->depth_testing:  %d", /*args*/	" ", pbr->depth_testing);
		
		if (pbr->parent)
			Con_PrintLinef	("%10s" "Parent: %d " QUOTED_S, /*args*/ " ", pbr->parent->word_idx, pbr->parent->word);
		else Con_PrintLinef	("%10s" "Parent: none", /*args*/ " " );
		if (pbr->branches_count) {
			Con_PrintLinef	("%10s" "Children (%d)", /*args*/ " ",  pbr->branches_count);
			for (int k = 0; k < pbr->operand_count; k ++) {
				eq_node_t *pbc = pbr->branches[k];
				if (!pbc) {
					Con_PrintLinef	("%10s" "Child #%3d: MISSING x0", /*args*/ " ", k);
				} else {
				Con_PrintLinef	("%10s" "Child #%3d: %d " QUOTED_S, /*args*/ " ", k, pbc->word_idx, pbc->word);
				}
			}
		}
		else Con_PrintLinef	("%10s" "Children: none", " ");



	} // for

}

void equat_depth_calc_recursive (equat_t *eq, eq_node_t *pbr, int depth)
{
	//Con_PrintLinef ("equat_depth_calc_recursive %d " QUOTED_S " at depth %d", pbr->word_idx, pbr->word, depth);
	if (depth + 1 > eq->num_depths)
		eq->num_depths = depth + 1;
	pbr->depth_testing = depth;
	for (int j = 0; j < pbr->branches_count; j ++) {
		eq_node_t *this_child = pbr->branches[j];
		//Con_PrintLinef ("Touch %d " QUOTED_S, this_child->word_idx, this_child->word);
		
		equat_depth_calc_recursive (eq, this_child, depth + 1);
	} // for
}


void equat_condump_tree (equat_t *eq)
{
	if (!eq->head_node)
		return; // Nothing to print

	equat_depth_calc_recursive (eq, eq->head_node, depth_0);
	int depth_count[512 /*eq->num_depths*/] = {0};
	int maxchars = 0;
	for (int my_depth = 0; my_depth < eq->num_depths; my_depth ++) {
		
		for (int idx = 0; idx < eq->count; idx ++) {
			eq_node_t *pbr = &eq->nodes[idx];
			if (pbr->depth_testing == my_depth) {
				depth_count[my_depth] ++;
				maxchars += strlen(pbr->word);
			}
		}
	//equat_depth_calc_recursive (eq);
		//Con_PrintLinef ("At depth %d there are %d", my_depth, depth_count[my_depth]);
	}

	
	for (int my_depth = 0; my_depth < eq->num_depths; my_depth ++) {
		baker_string_t *bs_line = BakerString_Create_Malloc ("");
		int colx = 0;	
		for (int idx = 0; idx < eq->count; idx ++) {
			eq_node_t *pbr = &eq->nodes[idx];
			if (pbr->depth_testing == my_depth) {
				depth_count[my_depth] ++;
				BakerString_CatC (bs_line, pbr->word);
			} else {
				char *sza = Z_StrRepeat_Z (SPACE_CHAR_32, strlen(pbr->word));
				BakerString_CatC (bs_line, sza);
				Mem_FreeNull_ (sza);
			}
			BakerString_CatC (bs_line, " ");
		}
	//equat_depth_calc_recursive (eq);
		Con_PrintLinef ("%s", bs_line->string);
		BakerString_Destroy_And_Null_It (&bs_line);
	}
}


void equat_run (equat_t *eq)
{
	br_execute_fork_recursive (eq, eq->head_node);
	//Con_PrintLinef (CON_GREEN "Returned " FLOAT_LOSSLESS_FORMAT, (float)eq->head_node->val);
}



// Baker: Only nodes are allocated
void equatfreecontents (equat_t *plist)
{
	for (int j = 0;j < plist->count; j++) {
		node_free (&plist->nodes[j]);
	}
	plist->count = plist->maxsize = 0;
	Mem_FreeNull_ (plist->nodes);
}
