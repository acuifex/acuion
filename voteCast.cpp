#include <string>
#include "voteCast.h"
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

switch (Settings::voteCast::type){
        case voteCastType::Console :
                if (strstr(event->GetName(), XORSTR("vote_cast"))){ 
                        int vote_player_id = event->GetInt(XORSTR("entityid"));
                        IEngineClient::player_info_t playerInfo;
                        engine->GetPlayerInfo( vote_player_id, &playerInfo );
                        int option = event->GetInt("vote_option");
                        cvar->ConsoleDPrintf(std::string(playerInfo.name).c_str());
                        if (option == 0){
                                cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0),XORSTR(" Voted yes\n"));
                        }else{
                                cvar->ConsoleColorPrintf(ColorRGBA(225, 0, 0),XORSTR(" Voted no\n"));
                        }
        }
        break;
        case voteCastType::Chat :
                if (strstr(event->GetName(), XORSTR("vote_cast"))){ 
                        int vote_player_id = event->GetInt(XORSTR("entityid"));
                        IEngineClient::player_info_t playerInfo;
                        engine->GetPlayerInfo( vote_player_id, &playerInfo );
                        int option = event->GetInt("vote_option");
                        std::string votestring = "Say ";
                        votestring += playerInfo.name;
                        cvar->ConsoleDPrintf(std::string(playerInfo.name).c_str());
                        if (option == 0){
				votestring += " Voted yes";
                                cvar->ConsoleDPrintf(XORSTR(" Voted yes"));
                        }else{
				votestring += " Voted no";
                                cvar->ConsoleDPrintf(XORSTR(" Voted no"));
                        }
			engine->ExecuteClientCmd(votestring.c_str());
        }
        break;
        case voteCastType::TeamSideChat :
                if (strstr(event->GetName(), XORSTR("vote_cast"))){ 
                        int vote_player_id = event->GetInt(XORSTR("entityid"));
                        IEngineClient::player_info_t playerInfo;
                        engine->GetPlayerInfo( vote_player_id, &playerInfo );
                        int option = event->GetInt("vote_option");
                        std::string votestring = "Say_team ";
                        votestring += playerInfo.name;
                        cvar->ConsoleDPrintf(std::string(playerInfo.name).c_str());
                        if (option == 0){
				votestring += " Voted yes";
                                cvar->ConsoleDPrintf(XORSTR(" Voted yes"));
                        }else{
				votestring += " Voted no";
                                cvar->ConsoleDPrintf(XORSTR(" Voted no"));
                        }
			engine->ExecuteClientCmd(votestring.c_str());
        }
}
        }