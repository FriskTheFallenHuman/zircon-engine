// oject_5_fn_prototypes.c.h

EXO___

WARP_X_ (EditModeSetDotsObject)
typedef char *(*exofn_t) (oject_s *f);

WARP_X_ (Event_Find_Function)
EXO___ char *DevInit (oject_s *f);
EXO___ char *DevTabSelectOnChange (oject_s *ktab);

typedef struct {
	ccs		*name;
	exofn_t	fn;
} event_entry_s;

#define DOENTRY(x) { STRINGIFY(x), x }
event_entry_s event_entries[] = {
	//{ "DevInit",	DevInit },
	//{ "DevOnIndex", DevOnIndex },
	DOENTRY(DevInit),
	DOENTRY(DevTabSelectOnChange),
};
#undef DOENTRY

//


int baker_array_ptr_new_item_index(baker_array_s *a);

void Draw_PolygonBegin (const char *texname_or_null);
void Draw_PolygonVertex2D (const float *v, const float *tc, const float *color3, float alpha);
void Draw_PolygonEnd (void);
void Draw_Fill (const vec3_t pos, const vec3_t size, const vec3_t rgb, float alpha, float flags);

int Rect_Clip_Area_Result (const rect_s *pr_rect, const rect_s *pr_visible, rect_s *pr_clipped_out);

oject_s *Object_Create_And_Assign (oject_s **pk, oject_s *f, oject_s *container, oject_s *servant_to, things_s *po);
oject_s *Object_Create_And_Assign_By_ClassEnum (int class_id, oject_s **pk, oject_s *f, oject_s *container, oject_s *servant_to);
oject_s *Object_Find_Name (oject_s *f, const char *s_value);
qbool Oject_Will_Mouse (oject_s *k);
//oject_s *Object_Spacing_Refresh (oject_s *k);
oject_s *Oject_Focus_Set (oject_s *k);
void Object_Event_OnChange_Recursive (oject_s *k, int shall_recurse);

oject_s *Form_ContextMenu_Spawn (oject_s *f);
void Form_Hook_Set (oject_s *f, oject_s *k, mouse_hook_fn_t fn, timey_dbl_t hookwhen);
void Form_Hook_Clear (oject_s *f, oject_s *k);
void Form_Finalize_Refresh (oject_s *f);
oject_s *Form_Create (void);
void Form_Focus_Next (oject_s *f, int dir);
void Form_ZOrder (oject_s *kzlast);
int Form_Find_Idx (oject_s *f, oject_s *kfind);
void Form_Event_Onload (oject_s *f);
void Form_Dump (oject_s *f, stringlist_t *plist, dump_detail_e detail_level);

oject_s *O_Rectangle_Refresh (oject_s *k);
oject_s *O_ScrollBar_Refresh (oject_s *k); // Since ListView and ListBox might call it.
void O_Scrollbar_SetPct (oject_s *k, float pct);
oject_s *O_SizeDot_Refresh (oject_s *k);



