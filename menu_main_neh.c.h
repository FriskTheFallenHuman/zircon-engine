// menu_main_neh.c.h

			switch (NehGameType)
			{
			case TYPE_BOTH:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_SinglePlayer_f ();
					break;

				case 1:
					M_Menu_Demos_f ();
					break;

				case 2:
					M_Menu_MultiPlayer_f ();
					break;

				case 3:
					M_Menu_OptionsNova_f ();
					break;

				case 4:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine ("disconnect");
					Cbuf_AddTextLine ("playdemo endcred");
					break;

				case 5:
					M_Menu_Quit_f ();
					break;
				}
				break;
			case TYPE_GAME:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_SinglePlayer_f ();
					break;

				case 1:
					M_Menu_MultiPlayer_f ();
					break;

				case 2:
					M_Menu_OptionsNova_f ();
					break;

				case 3:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine ("disconnect");
					Cbuf_AddTextLine ("playdemo endcred");
					break;

				case 4:
					M_Menu_Quit_f ();
					break;
				}
				break;
			case TYPE_DEMO:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_Demos_f ();
					break;

				case 1:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine ("disconnect");
					Cbuf_AddTextLine ("playdemo endcred");
					break;

				case 2:
					M_Menu_OptionsNova_f ();
					break;

				case 3:
					M_Menu_Quit_f ();
					break;
				}
				break;
			} // sw