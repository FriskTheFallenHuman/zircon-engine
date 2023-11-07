// menu_quit.c.h


//=============================================================================
/* QUIT MENU */

static const char *m_quit_message[9];
static int		m_quit_prevstate;
static qbool	wasInMenus;


static int M_QuitMessage(const char *line1, const char *line2, const char *line3, const char *line4, const char *line5, const char *line6, const char *line7, const char *line8)
{
	m_quit_message[0] = line1;
	m_quit_message[1] = line2;
	m_quit_message[2] = line3;
	m_quit_message[3] = line4;
	m_quit_message[4] = line5;
	m_quit_message[5] = line6;
	m_quit_message[6] = line7;
	m_quit_message[7] = line8;
	m_quit_message[8] = NULL;
	return 1;
}

static int M_ChooseQuitMessage(int request)
{
	if (1 || m_missingdata) // Baker: 7017
	{
		// frag related quit messages are pointless for a fallback menu, so use something generic
		if (request-- == 0) return M_QuitMessage("Are you sure you want to quit?","Press Y to quit, N to stay",NULL,NULL,NULL,NULL,NULL,NULL);
		return 0;
	}
	//switch (gamemode)
	//{
	//case GAME_ZEROX:
	//case GAME_NORMAL:
	//case GAME_HIPNOTIC:
	//case GAME_ROGUE:
	//case GAME_QUOTH:
	//case GAME_NEHAHRA:
	//case GAME_DEFEATINDETAIL2:
	//	if (request-- == 0) return M_QuitMessage("Are you gonna quit","this game just like","everything else?",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Milord, methinks that","thou art a lowly","quitter. Is this true?",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Do I need to bust your","face open for trying","to quit?",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Man, I oughta smack you","for trying to quit!","Press Y to get","smacked out.",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Press Y to quit like a","big loser in life.","Press N to stay proud","and successful!",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("If you press Y to","quit, I will summon","Satan all over your","hard drive!",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Um, Asmodeus dislikes","his children trying to","quit. Press Y to return","to your Tinkertoys.",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("If you quit now, I'll","throw a blanket-party","for you next time!",NULL,NULL,NULL,NULL,NULL);
	//	break;
	//case GAME_GOODVSBAD2:
	//	if (request-- == 0) return M_QuitMessage("Press Yes To Quit","...","Yes",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Do you really want to","Quit?","Play Good vs bad 3!",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("All your quit are","belong to long duck","dong",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Press Y to quit","","But are you too legit?",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("This game was made by","e@chip-web.com","It is by far the best","game ever made.",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Even I really dont","know of a game better","Press Y to quit","like rougue chedder",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("After you stop playing","tell the guys who made","counterstrike to just","kill themselves now",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Press Y to exit to DOS","","SSH login as user Y","to exit to Linux",NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Press Y like you","were waanderers","from Ys'",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("This game was made in","Nippon like the SS","announcer's saying ipon",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("you","want to quit?",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Please stop playing","this stupid game",NULL,NULL,NULL,NULL,NULL,NULL);
	//	break;
	//case GAME_BATTLEMECH:
	//	if (request-- == 0) return M_QuitMessage("? WHY ?","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Leave now and your mech is scrap!","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Accept Defeat?","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Wait! There are more mechs to destroy!","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Where's your bloodlust?","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Your mech here is way more impressive","than your car out there...","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Quitting won't reduce your debt","Press Y to quit, N to keep fraggin'",NULL,NULL,NULL,NULL,NULL,NULL);
	//	break;
	//case GAME_OPENQUARTZ:
	//	if (request-- == 0) return M_QuitMessage("There is nothing like free beer!","Press Y to quit, N to stay",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("GNU is not Unix!","Press Y to quit, N to stay",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("You prefer free beer over free speech?","Press Y to quit, N to stay",NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Is OpenQuartz Propaganda?","Press Y to quit, N to stay",NULL,NULL,NULL,NULL,NULL,NULL);
	//	break;
	//default:
	//	if (request-- == 0) return M_QuitMessage("Tired of fragging already?",NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Quit now and forfeit your bodycount?",NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Are you sure you want to quit?",NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	//	if (request-- == 0) return M_QuitMessage("Off to do something constructive?",NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	//	break;
	//}
	//return 0;
}

void M_Menu_Quit_f (void)
{
	int n;
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu || key_dest == key_menu_grabbed);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	menu_state_set_nova (m_quit);
	m_entersound = true;
	// count how many there are
	for (n = 1;M_ChooseQuitMessage(n);n++);
	// choose one
	M_ChooseQuitMessage(rand() % n);
}


static void M_Quit_Key (int key, int ascii)
{
	switch (key) {
	case K_ESCAPE:	case K_MOUSE2:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = (enum m_state_e)m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			menu_state_set_nova (m_none);
		}
		break;
	case K_MOUSE1:
	case 'Y':
	case 'y':
		Host_Quit_f ();
		break;

	default:
		break;
	}
}

static void M_Quit_Draw (void)
{
	int i, l, linelength, firstline, lastline, lines;
	for (i = 0, linelength = 0, firstline = 9999, lastline = -1;m_quit_message[i];i++)
	{
		if ((l = (int)strlen(m_quit_message[i])))
		{
			if (firstline > i)
				firstline = i;
			if (lastline < i)
				lastline = i;
			if (linelength < l)
				linelength = l;
		}
	}
	lines = (lastline - firstline) + 1;
	M_Background(linelength * 8 + 16, lines * 8 + 16);
	if (!m_missingdata) //since this is a fallback menu for missing data, it is very hard to read with the box
		M_DrawTextBox(0, 0, linelength, lines); //this is less obtrusive than hacking up the M_DrawTextBox function
	for (i = 0, l = firstline;i < lines;i++, l++)
		M_Print(8 + 4 * (linelength - strlen(m_quit_message[l])), 8 + 8 * i, m_quit_message[l]);
}



//
//
//



static int M_ChooseNoSaveMessage()
{
	// frag related quit messages are pointless for a fallback menu, so use something generic
	return M_QuitMessage("Nothing to save!","OK",NULL,NULL,NULL,NULL,NULL,NULL);
}

void M_Menu_NoSave_f (void)
{
//	int n;
	key_dest = key_menu;
	
	menu_state_set_nova (m_nosave);
	m_entersound = true;
	M_ChooseNoSaveMessage();
}


static void M_NoSave_Key (int key, int ascii)
{
	switch (key) {
	case K_MOUSE2:
	case K_ESCAPE:	
	case K_MOUSE1: 
	case K_ENTER:
		M_Menu_SinglePlayer_f ();
	} // sw

	
}

static void M_NoSave_Draw (void)
{
	int i, l, linelength, firstline, lastline, lines;
	for (i = 0, linelength = 0, firstline = 9999, lastline = -1;m_quit_message[i];i++)
	{
		if ((l = (int)strlen(m_quit_message[i])))
		{
			if (firstline > i)
				firstline = i;
			if (lastline < i)
				lastline = i;
			if (linelength < l)
				linelength = l;
		}
	}
	lines = (lastline - firstline) + 1;
	M_Background(linelength * 8 + 16, lines * 8 + 16);
	if (!m_missingdata) //since this is a fallback menu for missing data, it is very hard to read with the box
		M_DrawTextBox(0, 0, linelength, lines); //this is less obtrusive than hacking up the M_DrawTextBox function
	for (i = 0, l = firstline;i < lines;i++, l++)
		M_Print(8 + 4 * (linelength - strlen(m_quit_message[l])), 8 + 8 * i, m_quit_message[l]);
}


