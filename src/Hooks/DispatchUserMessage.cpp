#include "hooks.h"

#include "../interfaces.h"

#include "../SDK/bitbuf.h"


typedef void (*DispatchUserMessageFn) (void*, unsigned int, unsigned int, unsigned int, const void*);

void Hooks::DispatchUserMessage(void* thisptr, unsigned int msg_type, unsigned int unk1, unsigned int nBytes, const void* msg_data)
{

	clientVMT->GetOriginalMethod<DispatchUserMessageFn>(38)(thisptr, msg_type, unk1, nBytes, msg_data);

//	if(msg_type == 6){
//		bf_read read = bf_read(reinterpret_cast<uintptr_t>(msg_data));
//		read.setOffset(1);
//		int ent_index = read.readByte();
//		read.skip(3);
//		std::string msg_name = read.readString();
//		std::string player_name = read.readString();
//		std::string message = read.readString();
//		std::string location = read.readString();
//		std::string unk0 = read.readString();
//		bool all_chat = read.readBool();
//		cvar->ConsoleDPrintf("ent_index: %d \n msg_name: %s \n player_name: %s \n message: %s \n location: %s \n unk0: %s \n all_chat (boolean): %d \n", ent_index, msg_name, player_name, message, location, unk0, all_chat);
//	}
}
