#include "hooks.h"

#include "../interfaces.h"


typedef void (*DispatchUserMessageFn) (void*, unsigned int, unsigned int, unsigned int, const void*);

void Hooks::DispatchUserMessage(void* thisptr, unsigned int msg_type, unsigned int unk1, unsigned int nBytes, const void* msg_data)
{
	switch (msg_type)
	{
	case 46: // vote start
		break;
	}

	clientVMT->GetOriginalMethod<DispatchUserMessageFn>(38)(thisptr, msg_type, unk1, nBytes, msg_data);
}
