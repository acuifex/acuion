#pragma once

#include "../SDK/IGameEvent.h"
#include "../SDK/definitions.h"

namespace AutoDisconnect
{
	//Hooks
	void FireGameEvent(IGameEvent* event);
}


extern HostDisconnectFn Host_Disconnect;