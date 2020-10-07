#include <string>
#include "votecast.h"
#include "../Utils/xorstring.h"
#include "../Utils/entity.h"
#include "../settings.h"
#include "../fonts.h"
#include "../interfaces.h"
#include "../Utils/draw.h"
#include "../Hooks/hooks.h"

void voteCast::FireGameEvent(IGameEvent *event){

	if (!Settings::voteCast::enabled)
        return;
	
	if (!strstr(event->GetName(), XORSTR("vote_cast")))
		return;
	
	int vote_player_id = event->GetInt(XORSTR("entityid"));
	IEngineClient::player_info_t playerInfo;
	engine->GetPlayerInfo( vote_player_id, &playerInfo );
	int option = event->GetInt("vote_option");
	
	cvar->ConsoleDPrintf(std::string(playerInfo.name).c_str());
	if (option == 0)
		cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0),XORSTR(" Voted yes\n"));
	else
		cvar->ConsoleColorPrintf(ColorRGBA(225, 0, 0),XORSTR(" Voted no\n"));
	
	switch (Settings::voteCast::type){
	    case voteCastType::Console:
	        break;
	    case voteCastType::Chat: // using std::string is probably a bad idea but i'm lazy.
		    {
		    	std::string votestring = "Say ";
				votestring += playerInfo.name;
				votestring += option == 0 ? " Voted yes" : " Voted no";
				engine->ExecuteClientCmd(votestring.c_str());
		    }
	        break;
		case voteCastType::TeamSideChat:
			{
				std::string votestring = "Say_team ";
				votestring += playerInfo.name;
				votestring += option == 0 ? " Voted yes" : " Voted no";
				engine->ExecuteClientCmd(votestring.c_str());
			}
			break;
	}
}
