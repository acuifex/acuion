#include "autodisconnect.h"

#include "../interfaces.h"
// #include "../SDK/INetChannel.h"
#include "../Utils/xorstring.h"
#include "../settings.h"
// #include "skinchanger.h" //GetLocalClient


void AutoDisconnect::FireGameEvent(IGameEvent* event) {
	
	if (!Settings::AutoDisconnect::enabled)
		return;
	
	if (!strstr(event->GetName(), XORSTR("cs_intermission")))
		return;
	
	cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0),XORSTR("i'm leaving bro\n"));
	// ((INetChannel*)engine->GetNetChannelInfo())->Shutdown(XORSTR("bye retards")); // no work
	// GetLocalClient(-1)->m_NetChannel->Shutdown(XORSTR("bye retards")); // also no work
	Host_Disconnect(1);
}