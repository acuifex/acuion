#include "keyfix.h"

#include "../settings.h"
#include "../interfaces.h"

void KeyFix::CreateMove(CUserCmd* cmd){
	
	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	
	if (!localplayer || !localplayer->GetAlive())
		return;
	
	cmd->buttons &= ~IN_BACK; // reset keys
	cmd->buttons &= ~IN_FORWARD;
	cmd->buttons &= ~IN_MOVELEFT;
	cmd->buttons &= ~IN_MOVERIGHT;
	
	if (Settings::Skateboarding::enabled) { // do skateing
		if (cmd->forwardmove > 0)
			cmd->buttons |= IN_BACK;
		else if (cmd->forwardmove < 0)
			cmd->buttons |= IN_FORWARD;
		
		if (cmd->sidemove > 0)
			cmd->buttons |= IN_MOVELEFT;
		else if (cmd->sidemove < 0)
			cmd->buttons |= IN_MOVERIGHT;
	}
	else { // fix skateboarding
		if (cmd->forwardmove > 0)
			cmd->buttons |= IN_FORWARD;
		else if (cmd->forwardmove < 0)
			cmd->buttons |= IN_BACK;
		
		if (cmd->sidemove > 0)
			cmd->buttons |= IN_MOVERIGHT;
		else if (cmd->sidemove < 0)
			cmd->buttons |= IN_MOVELEFT;
	}
	
}