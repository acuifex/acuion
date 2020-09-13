#pragma once

#include "../SDK/IGameEvent.h"

namespace voteCast
{
	//Hooks
	void BeginFrame(float frameTime);
	void FireGameEvent(IGameEvent* event);
} // namespace voteCast
