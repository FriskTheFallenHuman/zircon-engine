// eq_attach.c.h


// Self return
eq_node_t *branch_assume_head_ret_self (equat_t *eq, eq_node_t *pbr)
{
	pbr->parent = NULL;
	eq->head_node = pbr;
	eq->plast = pbr; // FIRST
	return pbr;
}

void branch_add_as_parent (equat_t *eq, eq_node_t *pbr, eq_node_t *your_child)
{
	// Take other's child spot or assume head node if no parent.
	//new_branch->child_nthness	= your_child->child_nthness;  
	pbr->parent	= your_child->parent;

	// If no parent, take head node.  If the child has a parent, take that spot in the parent' list.
	if (your_child->parent == NULL) {
		eq->head_node = pbr;				// Become head
	}
	else {
		int our_child_nthness = your_child->parent->branches_count - 1;
		eq_node_t *expect_your_child = your_child->parent->branches[our_child_nthness];
		if (expect_your_child != your_child) {
			Con_PrintLinef ("Error .. not expected child");
			return;
		}
		your_child->parent->branches[our_child_nthness] = pbr;  // Parent, take child spot
	}

	// Set relationship between new_branch and displaced child.
    your_child->parent						= pbr;
	pbr->branches[pbr->branches_count ++]	= your_child;
}



eq_node_t *branch_dualist_attach_ret_self (equat_t *eq, eq_node_t *pbr)
{
	// DUALIST - WE ARE ALWAYS SOMEONE'S PARENT
	// A dualist must grab the current node.
	if (eq->plast == NULL && pbr->operand_count != 1) {
		Con_PrintLinef ("ERROR! Non-solo operator with no current node");
		return NULL;
	}

	eq_node_t *test_branch;
	for (test_branch = eq->plast; /*nada*/ ; test_branch = test_branch->parent) {
		if (test_branch->parent == NULL)
			break;		// Top. Cannot rise further.

		// EXAMINE PARENTHESIS LEVEL
		if (pbr->depth_level < test_branch->parent->depth_level)	continue;	// WON, RISE
		if (pbr->depth_level > test_branch->parent->depth_level)	break;		// LOST

		// LOOK AT OPERATOR LEVEL  + (4) <= * (3)  
		// If our operator level is lower, we lose.
		if (pbr->opx_level < test_branch->parent->opx_level) 
			break; // Lost on lower operator level.

		// TIED SOLOIST.  Tied Solo operators rise on a tie and work right to left.  
		// "- - -7" or - "Bit.Not 7"
		if (test_branch->parent->operand_count == 1)
			if (pbr->opx_level == test_branch->parent->opx_level) break; // Lost on sololist tie.		
	}

	//Branch_Add_As_Parent (ex, new_branch, test_branch);	
	branch_add_as_parent (eq, pbr, test_branch);

	// Branch_Add_As_Child	
	//eq->plast->branches[eq->plast->branches_count ++] = pbr;
	return pbr;
}

eq_node_t *branch_soloist_attach_ret_self (equat_t *eq, eq_node_t *pbr)
{
	ccs *s = pbr->word;
	// Branch_Asssume_Head?
	if (eq->plast == NULL)
		return branch_assume_head_ret_self (eq, pbr);

#if 1
	eq_node_t *test_branch;
	for (test_branch = eq->plast; /*nada*/ ; test_branch = test_branch->parent) {
		if (test_branch->parent == NULL)
			break;		// Top. Cannot rise further.

		// EXAMINE PARENTHESIS LEVEL
		if (pbr->depth_level < test_branch->parent->depth_level)	continue;	// WON, RISE
		if (pbr->depth_level > test_branch->parent->depth_level)	break;		// LOST

		// TIED SOLOIST.  Tied Solo operators rise on a tie and work right to left.  
		// "- - -7" or - "Bit.Not 7"
		if (test_branch->parent->operand_count == 1)
			if (pbr->opx_level == test_branch->parent->opx_level) break; // Lost on sololist tie.
	}
#endif

	// What are some error conditions here?
	if (eq->plast->branches_count >= eq->plast->operand_count) {
		// Branches full
		Con_PrintLinef ("ERROR!  Branches full");
		return NULL;
	}

	if (eq->plast->branches_count == 0 && eq->plast->operand_count > 1) {
		// Branches is 0 for existing operator that is dual or higher.
		Con_PrintLinef ("ERROR! Dual operator with no node a SET");
		return NULL;
	}

	// Add the value as the last to the current node
	// Branch_Add_As_Child	
	pbr->parent = eq->plast;
	eq->plast->branches[eq->plast->branches_count ++] = pbr;
	return pbr;
}
