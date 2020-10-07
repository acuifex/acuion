#include "profilechanger.h"

#include "../settings.h"
#include "../interfaces.h"

void ProfileChanger::UpdateProfile()
{
	if (!engine->IsInGame())
		return;

	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;

	const auto localplayer_index = localplayer->GetIndex();

	if (auto player_resource = *csPlayerResource){
		player_resource->SetActiveCoinRank()[localplayer_index] = Settings::ProfileChanger::coinID;
		player_resource->SetMusicID()[localplayer_index] = Settings::ProfileChanger::musicID;
		player_resource->SetCompetitiveRanking()[localplayer_index] = Settings::ProfileChanger::compRank;
	}
}