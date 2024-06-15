// menu_slist_support.c.h

WARP_X_ (stringlistsort_cmp)

// Baker: This is deterministic because no 2 servers can have same address
// (And if they did due to multiple master servers, it already got filtered out).
int slist_sort_cname_deterministic (const void *pa, const void *pb)
{
	int negator = 1;// slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.cname;
	const char *s2 = my_entry_b->info.cname;

	int diff = negator * strcasecmp(s1, s2);


	return diff;
}

int slist_sort_tiebreaker_bias_cname_deterministic (const void *pa, const void *pb)
{
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	// Baker: I am under the impression that smallest number wins
	int diff = -(my_entry_a->info.tiebreaker_bias - my_entry_b->info.tiebreaker_bias); // 10 - 9

	if (diff == 0) {
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_ping (const void *pa, const void *pb)
{
	int negator = slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	int n1 = my_entry_a->info.ping;
	int n2 = my_entry_b->info.ping;

	int diff = negator * (n1 - n2);

	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_tiebreaker_bias_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_numplayers (const void *pa, const void *pb)
{
	int negator = slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	int n1 = my_entry_a->info.numplayers;
	int n2 = my_entry_b->info.numplayers;

	int diff = negator * (n1 - n2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_tiebreaker_bias_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_description (const void *pa, const void *pb)
{
	int negator = slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.name;
	const char *s2 = my_entry_b->info.name;

	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_tiebreaker_bias_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_gamedir (const void *pa, const void *pb)
{
	int negator = slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.mod;
	const char *s2 = my_entry_b->info.mod;

	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_tiebreaker_bias_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_map (const void *pa, const void *pb)
{
	int negator = slist_sort_ascending == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.map;
	const char *s2 = my_entry_b->info.map;

	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time otherwise the server browser list jumps around
		// with tie entries randomly filling out the list differently every frame.
		return slist_sort_tiebreaker_bias_cname_deterministic (pa, pb);
	}

	return diff;
}

#define MAX_TIEBREAKERS_10 10

char tiebreakers[MAX_TIEBREAKERS_10][64];
int tiebreakers_count;

int SList_Tiebreaker_Bias (const char *s)
{
	for (int idx = 0; idx < tiebreakers_count;  idx ++) {
		char *sxy = tiebreakers[idx];
		if (String_Contains_Caseless (s, sxy))
			return MAX_TIEBREAKERS_10 + 1 - idx;
	}
	return 0;
}

WARP_X_ (net_slist_tiebreaker)
void SList_Tiebreaker_Changed_c (cvar_t *var)
{
	// Reset the count
	tiebreakers_count = 0;

	int			comma_items_count = String_Count_Char (var->string, ',') + 1;

	for (int idx = 0; idx < comma_items_count && idx < MAX_TIEBREAKERS_10; idx ++) {
		char *s_this =  String_Instance_Malloc_Base1 (var->string, ',' , idx + 1, q_reply_len_NULL);
		// gcc unused ... int sz = sizeof(tiebreakers[idx]);
		c_strlcpy (tiebreakers[idx], s_this);
		freenull_ (s_this);
		tiebreakers_count ++;
	} // idx
}


void M_ServerList_Rebuild (void)
{
	serverlist_list_count = 0; // Clear
	serverlist_list_query_time = Sys_DirtyTime ();
	// Find qualifying entries ..
	for (int idx = 0 ; idx < serverlist_viewlist_count; idx ++) {
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx);

		// Baker: Server without map name is disqualified (seems to be qwfwd or something)
		if (my_entry->info.map[0] == 0)
			continue; // DISQUALIFIED BECAUSE NO MAP NAME (qizmo forwards and such)

		// Word Filter?
		if (slist_filter_word[0]) {
			// Must contain
			if (String_Contains_Caseless (my_entry->info.cname, slist_filter_word))
				goto keep_me;
			if (String_Contains_Caseless (my_entry->info.name, slist_filter_word))
				goto keep_me;
			if (String_Contains_Caseless (my_entry->info.mod, slist_filter_word))
				goto keep_me;
			if (String_Contains_Caseless (my_entry->info.map, slist_filter_word))
				goto keep_me;

			continue; // Disqualified
		}

keep_me:
		// Players only?
		if (slist_filter_players_only) {
			if (my_entry->info.numplayers == 0)
				continue; // DISQUALIFIED
		}

		// ADD:
		serverlist_list[serverlist_list_count] = idx; serverlist_list_count ++;
	} // for

#if 000
	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx_nova);
		Con_PrintLinef ("RAW idx %03d of %03d/ idx_nova %03d %s", idx, serverlist_list_count, idx_nova, my_entry->info.name);
	}
#endif

	if (serverlist_list_count == 0)
		goto empty;

	switch (slist_sort_by) {
	case SLIST_SORTBY_PING_0:		qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_ping);
									break;
	case SLIST_SORTBY_PLAYERS_1:	qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_numplayers);
									break;
	case SLIST_SORTBY_NAME_2:		qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_description);
									break;
	case SLIST_SORTBY_GAME_3:		qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_gamedir);
									break;
	case SLIST_SORTBY_MAP_4:		qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_map);
									break;
	} // sw

empty:
	// Determine listindex
#if 0
	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx_nova);
		Con_PrintLinef ("SORTED idx %03d of %03d/ idx_nova %03d %s", idx, serverlist_list_count, idx_nova, my_entry->info.name);
	}
#endif

	if (last_nav_cname[0] == 0 || serverlist_list_count == 0) {
		goto cant_find;
	}


	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *this_entry = ServerList_GetViewEntry(idx_nova);
		if (String_Match (this_entry->info.cname, last_nav_cname)) {
			// Found it
			slist_cursor = idx;
			return;
		}
	} // for

cant_find:
	slist_cursor = 0;
}
