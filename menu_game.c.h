// menu_game.c.h


//=============================================================================
/* GAME OPTIONS MENU */

typedef struct level_s
{
	const char	*name;
	const char	*description;
} level_t;

typedef struct episode_s
{
	const char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

typedef struct gamelevels_s
{
	const char *gamename;
	level_t *levels;
	episode_t *episodes;
	int numepisodes;
}
gamelevels_t;

static level_t quakelevels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

static episode_t quakeepisodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

 //MED 01/06/97 added hipnotic levels
static level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},  // 0

   {"hip1m1", "The Pumping Station"},          // 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},          // 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},       // 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},       // 16

   {"hipdm1", "The Edge of Oblivion"}           // 17
};

//MED 01/06/97  added hipnotic episodes
static episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
static level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
static episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

static level_t		nehahralevels[] =
{
	{"nehstart",	"Welcome to Nehahra"},
	{"neh1m1",	"Forge City1: Slipgates"},
	{"neh1m2",	"Forge City2: Boiler"},
	{"neh1m3",	"Forge City3: Escape"},
	{"neh1m4",	"Grind Core"},
	{"neh1m5",	"Industrial Silence"},
	{"neh1m6",	"Locked-Up Anger"},
	{"neh1m7",	"Wanderer of the Wastes"},
	{"neh1m8",	"Artemis System Net"},
	{"neh1m9",	"To the Death"},
	{"neh2m1",	"The Gates of Ghoro"},
	{"neh2m2",	"Sacred Trinity"},
	{"neh2m3",	"Realm of the Ancients"},
	{"neh2m4",	"Temple of the Ancients"},
	{"neh2m5",	"Dreams Made Flesh"},
	{"neh2m6",	"Your Last Cup of Sorrow"},
	{"nehsec",	"Ogre's Bane"},
	{"nehahra",	"Nehahra's Den"},
	{"nehend",	"Quintessence"}
};

static episode_t	nehahraepisodes[] =
{
	{"Welcome to Nehahra", 0, 1},
	{"The Fall of Forge", 1, 9},
	{"The Outlands", 10, 7},
	{"Dimension of the Lost", 17, 2}
};

// Map list for Transfusion
static level_t		transfusionlevels[] =
{
	{"e1m1",		"Cradle to Grave"},
	{"e1m2",		"Wrong Side of the Tracks"},
	{"e1m3",		"Phantom Express"},
	{"e1m4",		"Dark Carnival"},
	{"e1m5",		"Hallowed Grounds"},
	{"e1m6",		"The Great Temple"},
	{"e1m7",		"Altar of Stone"},
	{"e1m8",		"House of Horrors"},

	{"e2m1",		"Shipwrecked"},
	{"e2m2",		"The Lumber Mill"},
	{"e2m3",		"Rest for the Wicked"},
	{"e2m4",		"The Overlooked Hotel"},
	{"e2m5",		"The Haunting"},
	{"e2m6",		"The Cold Rush"},
	{"e2m7",		"Bowels of the Earth"},
	{"e2m8",		"The Lair of Shial"},
	{"e2m9",		"Thin Ice"},

	{"e3m1",		"Ghost Town"},
	{"e3m2",		"The Siege"},
	{"e3m3",		"Raw Sewage"},
	{"e3m4",		"The Sick Ward"},
	{"e3m5",		"Spare Parts"},
	{"e3m6",		"Monster Bait"},
	{"e3m7",		"The Pit of Cerberus"},
	{"e3m8",		"Catacombs"},

	{"e4m1",		"Butchery Loves Company"},
	{"e4m2",		"Breeding Grounds"},
	{"e4m3",		"Charnel House"},
	{"e4m4",		"Crystal Lake"},
	{"e4m5",		"Fire and Brimstone"},
	{"e4m6",		"The Ganglion Depths"},
	{"e4m7",		"In the Flesh"},
	{"e4m8",		"The Hall of the Epiphany"},
	{"e4m9",		"Mall of the Dead"},

	{"bb1",			"The Stronghold"},
	{"bb2",			"Winter Wonderland"},
	{"bb3",			"Bodies"},
	{"bb4",			"The Tower"},
	{"bb5",			"Click!"},
	{"bb6",			"Twin Fortress"},
	{"bb7",			"Midgard"},
	{"bb8",			"Fun With Heads"},
	{"dm1",			"Monolith Building 11"},
	{"dm2",			"Power!"},
	{"dm3",			"Area 15"},

	{"e6m1",		"Welcome to Your Life"},
	{"e6m2",		"They Are Here"},
	{"e6m3",		"Public Storage"},
	{"e6m4",		"Aqueducts"},
	{"e6m5",		"The Ruined Temple"},
	{"e6m6",		"Forbidden Rituals"},
	{"e6m7",		"The Dungeon"},
	{"e6m8",		"Beauty and the Beast"},
	{"e6m9",		"Forgotten Catacombs"},

	{"cp01",		"Boat Docks"},
	{"cp02",		"Old Opera House"},
	{"cp03",		"Gothic Library"},
	{"cp04",		"Lost Monastery"},
	{"cp05",		"Steamboat"},
	{"cp06",		"Graveyard"},
	{"cp07",		"Mountain Pass"},
	{"cp08",		"Abysmal Mine"},
	{"cp09",		"Castle"},
	{"cps1",		"Boggy Creek"},

	{"cpbb01",		"Crypt of Despair"},
	{"cpbb02",		"Pits of Blood"},
	{"cpbb03",		"Unholy Cathedral"},
	{"cpbb04",		"Deadly Inspirations"},

	{"b2a15",		"Area 15 (B2)"},
	{"b2bodies",	"BB_Bodies (B2)"},
	{"b2cabana",	"BB_Cabana"},
	{"b2power",		"BB_Power"},
	{"barena",		"Blood Arena"},
	{"bkeep",		"Blood Keep"},
	{"bstar",		"Brown Star"},
	{"crypt",		"The Crypt"},

	{"bb3_2k1",		"Bodies Infusion"},
	{"captasao",	"Captasao"},
	{"curandero",	"Curandero"},
	{"dcamp",		"DeathCamp"},
	{"highnoon",	"HighNoon"},
	{"qbb1",		"The Confluence"},
	{"qbb2",		"KathartiK"},
	{"qbb3",		"Caleb's Woodland Retreat"},
	{"zoo",			"Zoo"},

	{"dranzbb6",	"Black Coffee"},
	{"fragm",		"Frag'M"},
	{"maim",		"Maim"},
	{"qe1m7",		"The House of Chthon"},
	{"qdm1",		"Place of Two Deaths"},
	{"qdm4",		"The Bad Place"},
	{"qdm5",		"The Cistern"},
	{"qmorbias",	"DM-Morbias"},
	{"simple",		"Dead Simple"}
};

static episode_t	transfusionepisodes[] =
{
	{"The Way of All Flesh", 0, 8},
	{"Even Death May Die", 8, 9},
	{"Farewell to Arms", 17, 8},
	{"Dead Reckoning", 25, 9},
	{"BloodBath", 34, 11},
	{"Post Mortem", 45, 9},
	{"Cryptic Passage", 54, 10},
	{"Cryptic BloodBath", 64, 4},
	{"Blood 2", 68, 8},
	{"Transfusion", 76, 9},
	{"Conversions", 85, 9}
};

static level_t goodvsbad2levels[] =
{
	{"rts", "Many Paths"},  // 0
	{"chess", "Chess, Scott Hess"},                         // 1
	{"dot", "Big Wall"},
	{"city2", "The Big City"},
	{"bwall", "0 G like Psychic TV"},
	{"snow", "Wireframed"},
	{"telep", "Infinite Falling"},
	{"faces", "Facing Bases"},
	{"island", "Adventure Islands"},
};

static episode_t goodvsbad2episodes[] =
{
	{"Levels? Bevels!", 0, 8},
};

static level_t battlemechlevels[] =
{
	{"start", "Parking Level"},
	{"dm1", "Hot Dump"},                        // 1
	{"dm2", "The Pits"},
	{"dm3", "Dimber Died"},
	{"dm4", "Fire in the Hole"},
	{"dm5", "Clubhouses"},
	{"dm6", "Army go Underground"},
};

static episode_t battlemechepisodes[] =
{
	{"Time for Battle", 0, 7},
};

static level_t openquartzlevels[] =
{
	{"start", "Welcome to Openquartz"},

	{"void1", "The center of nowhere"},                        // 1
	{"void2", "The place with no name"},
	{"void3", "The lost supply base"},
	{"void4", "Past the outer limits"},
	{"void5", "Into the nonexistance"},
	{"void6", "Void walk"},

	{"vtest", "Warp Central"},
	{"box", "The deathmatch box"},
	{"bunkers", "Void command"},
	{"house", "House of chaos"},
	{"office", "Overnight office kill"},
	{"am1", "The nameless chambers"},
};

static episode_t openquartzepisodes[] =
{
	{"Single Player", 0, 1},
	{"Void Deathmatch", 1, 6},
	{"Contrib", 7, 6},
};

static level_t defeatindetail2levels[] =
{
	{"atac3",	"River Crossing"},
	{"atac4",	"Canyon Chaos"},
	{"atac7",	"Desert Stormer"},
};

static episode_t defeatindetail2episodes[] =
{
	{"ATAC Campaign", 0, 3},
};

static level_t prydonlevels[] =
{
	{"curig2", "Capel Curig"},	// 0

	{"tdastart", "Gateway"},				// 1
};

static episode_t prydonepisodes[] =
{
	{"Prydon Gate", 0, 1},
	{"The Dark Age", 1, 1}
};

static gamelevels_t sharewarequakegame = {"Shareware Quake", quakelevels, quakeepisodes, 2};
static gamelevels_t registeredquakegame = {"Quake", quakelevels, quakeepisodes, 7};
static gamelevels_t hipnoticgame = {"Scourge of Armagon", hipnoticlevels, hipnoticepisodes, 6};
static gamelevels_t roguegame = {"Dissolution of Eternity", roguelevels, rogueepisodes, 4};
static gamelevels_t nehahragame = {"Nehahra", nehahralevels, nehahraepisodes, 4};
static gamelevels_t transfusiongame = {"Transfusion", transfusionlevels, transfusionepisodes, 11};
static gamelevels_t goodvsbad2game = {"Good Vs. Bad 2", goodvsbad2levels, goodvsbad2episodes, 1};
static gamelevels_t battlemechgame = {"Battlemech", battlemechlevels, battlemechepisodes, 1};
static gamelevels_t openquartzgame = {"OpenQuartz", openquartzlevels, openquartzepisodes, 3};
static gamelevels_t defeatindetail2game = {"Defeat In Detail 2", defeatindetail2levels, defeatindetail2episodes, 1};
static gamelevels_t prydongame = {"Prydon Gate", prydonlevels, prydonepisodes, 2};

typedef struct gameinfo_s
{
	gamemode_t gameid;
	gamelevels_t *notregistered;
	gamelevels_t *registered;
}
gameinfo_t;

static gameinfo_t gamelist[] =
{
	{GAME_NORMAL, &sharewarequakegame, &registeredquakegame},
	{GAME_HIPNOTIC, &hipnoticgame, &hipnoticgame},
	{GAME_ROGUE, &roguegame, &roguegame},
	{GAME_QUOTH, &sharewarequakegame, &registeredquakegame},
	{GAME_NEHAHRA, &nehahragame, &nehahragame},
	{GAME_TRANSFUSION, &transfusiongame, &transfusiongame},
	{GAME_GOODVSBAD2, &goodvsbad2game, &goodvsbad2game},
	{GAME_BATTLEMECH, &battlemechgame, &battlemechgame},
	{GAME_OPENQUARTZ, &openquartzgame, &openquartzgame},
	{GAME_DEFEATINDETAIL2, &defeatindetail2game, &defeatindetail2game},
	{GAME_PRYDON, &prydongame, &prydongame},
};

static gamelevels_t *gameoptions_levels  = NULL;

static int	startepisode;
static int	startlevel;
static int maxplayers;
static qbool m_serverInfoMessage = false;
static double m_serverInfoMessageTime;

void M_Menu_GameOptions_f (void)
{
	int i;
	key_dest = key_menu;
	menu_state_set_nova (m_gameoptions);
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = min(8, MAX_SCOREBOARD);
	// pick game level list based on gamemode (use GAME_NORMAL if no matches)
	gameoptions_levels = registered.integer ? gamelist[0].registered : gamelist[0].notregistered;
	for (i = 0;i < (int)(sizeof(gamelist)/sizeof(gamelist[0]));i++)
		if (gamelist[i].gameid == gamemode)
			gameoptions_levels = registered.integer ? gamelist[i].registered : gamelist[i].notregistered;
}


static int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 140, 160, 168};
#define	NUM_GAMEOPTIONS	12
static int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	cachepic_t	*p0;
	int		x;
	char vabuf[1024];

	M_Background(320, 200);

	M_DrawPic (16, 4, CPC("gfx/qplaque"), NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic ( (320-p0->width)/2, 4, p0 /*"gfx/p_multi"*/, NO_HOTSPOTS_0, NA0, NA0);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_DrawTextBox (152, 32, 10, 1);
	M_Print(160, 40, "begin game");
	Hotspots_Add (menu_x + 48, menu_y + 40, (45 * 8) /*360*/, 8, 1, hotspottype_button);

	M_Print(0, 56, "      Max players");
	Hotspots_Add (menu_x + 48, menu_y + 96, (45 * 8) /*360*/, 8, 1, hotspottype_slider);

	M_Print(160, 56, va(vabuf, sizeof(vabuf), "%i", maxplayers) );

	if (1)
	{
		M_Print(0, 64, "        Game Type");
		Hotspots_Add (menu_x + 48, menu_y + 64, (45 * 8) /*360*/, 8, 1, hotspottype_slider);
		
		if (!coop.integer && !deathmatch.integer)
			Cvar_SetValue("deathmatch", 1);
		if (coop.integer)
			M_Print(160, 64, "Cooperative");
		else
			M_Print(160, 64, "Deathmatch");

		M_Print(0, 72, "        Teamplay");
		Hotspots_Add (menu_x + 48, menu_y + 72, (45 * 8) /*360*/, 8, 1, hotspottype_slider);

		if (gamemode == GAME_ROGUE)
		{
			const char *msg;

			switch((int)teamplay.integer)
			{
				case 1: msg = "No Friendly Fire"; break;
				case 2: msg = "Friendly Fire"; break;
				case 3: msg = "Tag"; break;
				case 4: msg = "Capture the Flag"; break;
				case 5: msg = "One Flag CTF"; break;
				case 6: msg = "Three Team CTF"; break;
				default: msg = "Off"; break;
			}
			M_Print(160, 72, msg);
		}
		else
		{
			const char *msg;

			switch (teamplay.integer)
			{
				case 0: msg = "Off"; break;
				case 2: msg = "Friendly Fire"; break;
				default: msg = "No Friendly Fire"; break;
			}
			M_Print(160, 72, msg);
		}

		M_Print(0, 80, "            Skill");
		Hotspots_Add (menu_x + 48, menu_y + 80, (45 * 8) /*360*/, 8, 1, hotspottype_slider);
		if (skill.integer == 0)
			M_Print(160, 80, "Easy difficulty");
		else if (skill.integer == 1)
			M_Print(160, 80, "Normal difficulty");
		else if (skill.integer == 2)
			M_Print(160, 80, "Hard difficulty");
		else
			M_Print(160, 80, "Nightmare difficulty");

		M_Print(0, 88, "       Frag Limit");
		Hotspots_Add (menu_x + 48, menu_y + 88, (45 * 8) /*360*/, 8, 1, hotspottype_slider);
		if (fraglimit.integer == 0)
			M_Print(160, 88, "none");
		else
			M_Print(160, 88, va(vabuf, sizeof(vabuf), "%i frags", fraglimit.integer));
		

		M_Print(0, 96, "       Time Limit");
		Hotspots_Add (menu_x + 48, menu_y + 96, (45 * 8) /*360*/, 8, 1, hotspottype_slider);
		if (timelimit.integer == 0)
			M_Print(160, 96, "none");
		else
			M_Print(160, 96, va(vabuf, sizeof(vabuf), "%i minutes", timelimit.integer));
		
	}

	M_Print(0, 104, "    Public server");
	M_Print(160, 104, (sv_public.integer == 0) ? "no" : "yes");
	Hotspots_Add (menu_x + 48, menu_y + 104, (45 * 8) /*360*/, 8, 1, hotspottype_slider);

	M_Print(0, 112, "   Server maxrate");
	M_Print(160, 112, va(vabuf, sizeof(vabuf), "%i", sv_maxrate.integer));
	Hotspots_Add (menu_x + 48, menu_y + 112, (45 * 8) /*360*/, 8, 1, hotspottype_slider);

	M_Print(0, 128, "      Server name");
	Hotspots_Add (menu_x + 48, menu_y + 128, (45 * 8) /*360*/, 8 + 8 + 1, 1, hotspottype_slider);

	M_DrawTextBox (0, 132, 38, 1);
	M_Print(8, 140, hostname.string);

	M_Print(0, 160, "         Episode");
	M_Print(160, 160, gameoptions_levels->episodes[startepisode].description);
	Hotspots_Add (menu_x + 48, menu_y + 160, (45 * 8) /*360*/, 8 + 1, 1, hotspottype_slider);

	M_Print(0, 168, "           Level");
	M_Print(160, 168, gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].description);
	M_Print(160, 176, gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].name);

	Hotspots_Add (menu_x + 48, menu_y + 168, (45 * 8) /*360*/, 8 + 1, 1, hotspottype_slider);

// line cursor
	if (gameoptions_cursor == 9)
		M_DrawCharacter (8 + 8 * strlen(hostname.string), gameoptions_cursor_table[gameoptions_cursor], 10+((int)(realtime*4)&1));
	else
		M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print(x, 146, " More than 255 players??");
			M_Print(x, 154, "  First, question your  ");
			M_Print(x, 162, "   sanity, then email   ");
			M_Print(x, 170, "darkplacesengine@gmail.com");
		}
		else
			m_serverInfoMessage = false;
	}

	drawsel_idx = gameoptions_cursor;
	PPX_DrawSel_End ();
}


static void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > MAX_SCOREBOARD)
		{
			maxplayers = MAX_SCOREBOARD;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		
		if (deathmatch.integer) // changing from deathmatch to coop
		{
			Cvar_SetValueQuick (&coop, 1);
			Cvar_SetValueQuick (&deathmatch, 0);
		}
		else // changing from coop to deathmatch
		{
			Cvar_SetValueQuick (&coop, 0);
			Cvar_SetValueQuick (&deathmatch, 1);
		}

		break;

	case 3:
		if (gamemode == GAME_ROGUE)
			count = 6;
		else
			count = 2;

		Cvar_SetValueQuick (&teamplay, teamplay.integer + dir);
		if (teamplay.integer > count)
			Cvar_SetValueQuick (&teamplay, 0);
		else if (teamplay.integer < 0)
			Cvar_SetValueQuick (&teamplay, count);
		break;

	case 4:
		Cvar_SetValueQuick (&skill, skill.integer + dir);
		if (skill.integer > 3)
			Cvar_SetValueQuick (&skill, 0);
		if (skill.integer < 0)
			Cvar_SetValueQuick (&skill, 3);

		break;

	case 5:
		Cvar_SetValueQuick (&fraglimit, fraglimit.integer + dir*10);
		if (fraglimit.integer > 100)
			Cvar_SetValueQuick (&fraglimit, 0);
		if (fraglimit.integer < 0)
			Cvar_SetValueQuick (&fraglimit, 100);
		break;

	case 6:
		Cvar_SetValueQuick (&timelimit, timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValueQuick (&timelimit, 0);
		if (timelimit.value < 0)
			Cvar_SetValueQuick (&timelimit, 60);
		break;

	case 7:
		Cvar_SetValueQuick (&sv_public, !sv_public.integer);
		break;

	case 8:
		Cvar_SetValueQuick (&sv_maxrate, sv_maxrate.integer + dir*500);
		if (sv_maxrate.integer < NET_MINRATE)
			Cvar_SetValueQuick (&sv_maxrate, NET_MINRATE);
		break;

	case 9:
		break;

	case 10:
		startepisode += dir;

		if (startepisode < 0)
			startepisode = gameoptions_levels->numepisodes - 1;

		if (startepisode >= gameoptions_levels->numepisodes)
			startepisode = 0;

		startlevel = 0;
		break;

	case 11:
		startlevel += dir;

		if (startlevel < 0)
			startlevel = gameoptions_levels->episodes[startepisode].levels - 1;

		if (startlevel >= gameoptions_levels->episodes[startepisode].levels)
			startlevel = 0;
		break;
	}
}

static void M_GameOptions_Key (int key, int ascii)
{
	int l;
	char hostnamebuf[128];
	char vabuf[1024];

	switch (key) {

	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { gameoptions_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_MOUSE1: if (!Hotspots_DidHit () ) { return;	}  gameoptions_cursor = hotspotx_hover; // PPX Key2 fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (gameoptions_cursor == 0) {
			if (sv.active)
				Cbuf_AddTextLine ("disconnect");
			Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "maxplayers %u", maxplayers) );

			Cbuf_AddTextLine (va(vabuf, sizeof(vabuf), "map %s", gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].name) );
			return;
		}

		M_NetStart_Change (1);
		break;

		case K_HOME: 
		gameoptions_cursor = 0;
		break;

	case K_END:
		gameoptions_cursor = NUM_GAMEOPTIONS - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
leftus:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("sound/misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("sound/misc/menu3.wav");
		M_NetStart_Change (1);
		break;


	case K_BACKSPACE:
		if (gameoptions_cursor == 9)
		{
			l = (int)strlen(hostname.string);
			if (l)
			{
				l = min(l - 1, 37);
				memcpy(hostnamebuf, hostname.string, l);
				hostnamebuf[l] = 0;
				Cvar_Set("hostname", hostnamebuf);
			}
		}
		break;

	default:
		if (ascii < 32)
			break;
		if (gameoptions_cursor == 9)
		{
			l = (int)strlen(hostname.string);
			if (l < 37)
			{
				memcpy(hostnamebuf, hostname.string, l);
				hostnamebuf[l] = ascii;
				hostnamebuf[l+1] = 0;
				Cvar_Set("hostname", hostnamebuf);
			}
		}
	}
}

