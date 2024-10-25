// oject_ccx_form_parse.c.h

// Space delimited default values.
WARP_X_CALLERS_ (Object_Create_And_Assign)


oject_s *Form_Create_From_String (ccs *s)
{
	qbool is_quirks_mode_allowed = true;
	qbool in_quirks_mode = false;
	oject_s *formy = Form_Create();
	stringlist_t words = {0};
	stringlistappend_tokenize_qcquotes (&words, s);
	
	oject_s *k = NULL;

	const char **wordray = stringlist_nullterm_add (&words);
	for (/*nada*/ ; *wordray; *wordray ++) {
		if (DO_PRINT) { DebugPrintLinef ("Word %s", *wordray); }

		things_s *thing = Thing_Find (*wordray);

		// Baker: We must be at a object, property or event.
		// If we don't know what it is, go to "quirks mode"
		// Ignore properties we don't know, but don't ignore object types?
		if (thing == NULL) {
			if (is_quirks_mode_allowed) {
				if (wordray[1] && wordray[1] && String_Match (wordray[1], PROPERTY_COLON_DELIM)) {
					Con_PrintLinef ("Entering quirks mode for unknown property " QUOTED_S 
						" and skipping " QUOTED_S " " QUOTED_S, wordray[0], wordray[1], wordray[2]);
					in_quirks_mode = true;
					*wordray ++; // Skip this one
					*wordray ++; // Skip the property value
					continue;
				}
			} // quirks mode
			Con_PrintLinef (CON_ERROR "Unknown " QUOTED_S, *wordray);
			break;
		}

		if (IsObjectType(thing)) {
			if (thing->enum_id == class_form_1)
				k = formy;
			else { 
				Object_Parse_Done (k);
				k = NULL;
				SET___ Object_Create_And_Assign (&k, formy, /*kcan:*/ formy, SERVANT_TO_NULL, thing);
			}
			continue;
		}

		if (IsProperty(thing)) {
			ccs *s_prop = *wordray;
			// Advance past property
			*wordray ++; Word_Check_Fail_Message ("No more words after property"); 

			if (String_Match(*wordray, PROPERTY_COLON_DELIM) == false) {
				Con_PrintLinef (CON_ERROR "No colon after property");
				break;
			}

			// Advance past semi-colon
			*wordray ++; Word_Check_Fail_Message ("No more words after property colon");  
			
			// PROCESS PROPERTY
			ccs *s_value = *wordray;
			if (DO_PRINT) {DebugPrintLinef ("Property %s = " QUOTED_S, s_prop, s_value); }
			qbool is_ok;
			is_ok = Property_Set_By_String_After_Freeing_Is_Ok (k, thing, s_value);
			continue;
		}

		//if (IsEvent(thing)) {
		//	ccs *s_event = *wordray;
		//	*wordray ++; Word_Check_Fail_Message ("No more words after event"); 
		//	if (String_Match(*wordray, PROPERTY_COLON_DELIM) == false) {
		//		Con_PrintLinef (CON_ERROR "No colon after event");
		//		break;
		//	}

		//	*wordray ++; Word_Check_Fail_Message ("No more words after event colon");
		//	
		//	things_s *builtin_thing = Thing_Find (*wordray);
		//	if (!builtin_thing || IsBuiltin(builtin_thing) == false) {
		//		Con_PrintLinef (CON_ERROR QUOTED_S " is not a builtin function for event %s", *wordray, s_event);
		//		break;
		//	}
		//	ccs *s_builtin = *wordray;
		//	if (DO_PRINT) {DebugPrintLinef ("Event %s fires " QUOTED_S, s_event, s_builtin);}
		//	qbool is_ok;
		//	is_ok = Event_Set_By_String_Is_Ok (k, thing, builtin_thing);
		//	continue;
		//}

		Con_PrintLinef (CON_ERROR "%s isn't anything", *wordray);
		break;
	} // for words

	int is_ok = *wordray == NULL;

	if (is_ok) {
		Object_Parse_Done (k);
		Form_Event_Onload (formy); // Runs before anything refreshes
		Form_Finalize_Refresh (formy);
	} else {
		oject_s *Form_Destroy (oject_s *f);
		formy = Form_Destroy (formy);
	}
	
	Con_DPrintLinef ("%s Parse was %s", in_quirks_mode ? "^6(QUIRKS MODE)^7" : "", is_ok ? "^5" "GOOD" : CON_ERROR "BAD");
	stringlistfreecontents (&words);
	return formy;
}

