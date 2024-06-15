// oject_2_structs.c.h

// Baker: These are pointer slot arrays (no need for sizeof).  Not good for triangles.
// DEF_COMPAT_ARRAY_STRUCT (polygon_array_s, oject_s);

typedef struct {
	int x, y;
} xy_pairi_s;

#define XY_SET(xy,x0,y0) (xy).x = x0, (xy).y = y0

typedef union _varpack_u_s {
	byte						bytep;
	short						shortp;
	int							intp;
	unsigned					unsignedp;						// Because we might want to print unsigned.
	int64_t						int64p;
	float						floatp;
	double						doublep;
	uint64_t					uint64p;
	uint32_t					uintp;
	rect_s						recti;
	frect						rectf;
	stringlist_t				stringlist;
	stringlist_t				*stringlist_p;
	floatlist_s					floatlist;
	int32list_s					int32list;
	void						*voidp;
	oject_s						*ojectp;
	vec3_t						vec3;
	union _varpack_u_s			*unwrap;
	/*const*/ char				*char_p;						// Not recommended.					

	byte						width_leader[64];				// kstring should always be width 56, but we'll use 64 here.
} varpack_u;

typedef struct {
	int		batchsize;					// Defaults to BAKER_ARRAY_BATCHSIZE_DEFAULT_128

	int		numitems;		
	int		maxitems;

	void	**pointers_array;			// union?
	size_t	datasize;
} baker_array_s;

typedef struct {
	char				texname[MAX_QPATH_128];
	int					drawflags;
	int					numvertices;
	int					maxvertices;
	float				*vertexdata;	// the vertex data
} ojpoly_s;

WARP_X_ (things_e)
typedef struct _things_s {
	char			*name;		// 1
	int				enum_id;	// 2
	union {
		int			vt;			// 3 Vartype
		int			oflags;		//   Can focus, can mouse flags.
	} w;
	size_t			moffsetof;	// 4 Field offset into the oject_s struct
	ccs				*sdefault;	// 5 
	ccs				*describe;	// 6 
	//parse_fn_t	parse_fn;	// 7 TODO
} things_st;

// 
// OJECT_S
//

// Baker: This is a struct declaration ... an array of object_s
DEF_COMPAT_ARRAY_STRUCT (oject_array_st, struct _oject_s);

typedef double timey_dbl_t;
typedef void (*mouse_hook_fn_t)(struct _oject_s *k);

typedef struct {
	int		rowheight;
	float	num_visiblef;
	int	num_visible_partials;

	// Validate all of these
	int		is_first_row_cutoff;
	int		first_vis_row;
	int		first_full_row;

	// Debatable because not clamped to list.
	int		y_beyond;

	// Debatable ... if the adjusted last_partial_row becomes "culled", this is wrong
	int		is_cutoff_bottom;

	// These get adjusted in a moment ..
	int		last_full_row;
	int		last_partial_row;

	int		rowcount;
	int		pageamount;

	rect_s	r_row_first;

	int		viewportmax;

	int		show_scrollbar;

///////
	int		firstcolumn;
	int		lastcolumn;
	//int		columnsallwidths; k->xl.columnsallwidths
	int		leftdrawoffset;
	int		num_left_positions;
} viewcalc_s;


