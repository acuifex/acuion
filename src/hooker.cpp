#include "hooker.h"

#include <link.h>
#include "Utils/patternfinder.h"
#include "Utils/util.h"
#include "Utils/vmt.h"
#include "Utils/xorstring.h"
#include "interfaces.h"
#include "offsets.h"


int* nPredictionRandomSeed = nullptr;
unsigned long* g_iModelBoneCounter = nullptr;
CMoveData* g_MoveData = nullptr;
bool* s_bOverridePostProcessingDisable = nullptr;
ConVar *cl_csm_enabled = nullptr;

VMT* panelVMT = nullptr;
VMT* clientVMT = nullptr;
VMT* modelRenderVMT = nullptr;
VMT* clientModeVMT = nullptr;
VMT* gameEventsVMT = nullptr;
VMT* viewRenderVMT = nullptr;
VMT* inputInternalVMT = nullptr;
VMT* materialVMT = nullptr;
VMT* surfaceVMT = nullptr;
VMT* launcherMgrVMT = nullptr;
VMT* engineVGuiVMT = nullptr;
VMT* soundVMT = nullptr;
VMT* uiEngineVMT = nullptr;

MsgFunc_ServerRankRevealAllFn MsgFunc_ServerRankRevealAll;
SendClanTagFn SendClanTag;
HostDisconnectFn Host_Disconnect;
SetLocalPlayerReadyFn SetLocalPlayerReady;

RecvVarProxyFn fnSequenceProxyFn;

StartDrawingFn StartDrawing;
FinishDrawingFn FinishDrawing;

GetLocalClientFn GetLocalClient;

LineGoesThroughSmokeFn LineGoesThroughSmoke;
InitKeyValuesFn InitKeyValues;
LoadFromBufferFn LoadFromBuffer;

panorama::PanelArray* panorama::panelArray = nullptr;

unsigned int Offsets::playerAnimStateOffset = 0;
unsigned int Offsets::playerAnimOverlayOffset = 0;

GetSequenceActivityFn GetSeqActivity;

CreateAnimStateFn CreateAnimState;
AnimStateUpdateFn AnimStateUpdate;
AnimStateResetFn AnimStateReset;

uintptr_t SetAbsOriginFnAddr;

RandomSeedFn RandomSeed;
RandomFloatFn RandomFloat;
RandomFloatExpFn RandomFloatExp;
RandomIntFn RandomInt;
RandomGaussianFloatFn RandomGaussianFloat;

SetNamedSkyBoxFn SetNamedSkyBox;

std::vector<dlinfo_t> libraries;

// taken form aixxe's cstrike-basehook-linux
bool Hooker::GetLibraryInformation(const char* library, uintptr_t* address, size_t* size) {
	if (libraries.size() == 0) {
		dl_iterate_phdr([] (struct dl_phdr_info* info, size_t, void*) {
			dlinfo_t library_info = {};

			library_info.library = info->dlpi_name;
			library_info.address = info->dlpi_addr + info->dlpi_phdr[0].p_vaddr;
			library_info.size = info->dlpi_phdr[0].p_memsz;

			libraries.push_back(library_info);

			return 0;
		}, nullptr);
	}

	for (const dlinfo_t& current: libraries) {
		if (!strcasestr(current.library, library))
			continue;

		if (address)
			*address = current.address;

		if (size)
			*size = current.size;

		return true;
	}

	return false;
}

bool Hooker::HookRecvProp(const char* className, const char* propertyName, std::unique_ptr<RecvPropHook>& recvPropHook)
{
	// FIXME: Does not search recursively.. yet.
	// Recursion is a meme, stick to reddit mcswaggens.
	for (ClientClass* pClass = client->GetAllClasses(); pClass; pClass = pClass->m_pNext)
	{
		if (strcmp(pClass->m_pNetworkName, className) == 0)
		{
			RecvTable* pClassTable = pClass->m_pRecvTable;

			for (int nIndex = 0; nIndex < pClassTable->m_nProps; nIndex++)
			{
				RecvProp* pProp = &pClassTable->m_pProps[nIndex];

				if (!pProp || strcmp(pProp->m_pVarName, propertyName) != 0)
					continue;

				recvPropHook = std::make_unique<RecvPropHook>(pProp);

				return true;
			}

			break;
		}
	}

	return false;
}

void Hooker::FindIClientMode()
{
    uintptr_t hudprocessinput = reinterpret_cast<uintptr_t>(getvtable(client)[10]);
	GetClientModeFn GetClientMode = reinterpret_cast<GetClientModeFn>(GetAbsoluteAddress(hudprocessinput + 11, 1, 5));

	clientMode = GetClientMode();
}

void Hooker::FindGlobalVars()
{
	uintptr_t HudUpdate = reinterpret_cast<uintptr_t>(getvtable(client)[11]);

	globalVars = *reinterpret_cast<CGlobalVars**>(GetAbsoluteAddress(HudUpdate + 13, 3, 7));
}

void Hooker::FindCInput()
{
	uintptr_t IN_ActivateMouse = reinterpret_cast<uintptr_t>(getvtable(client)[16]);

	input = **reinterpret_cast<CInput***>(GetAbsoluteAddress(IN_ActivateMouse, 3, 7));
}

void Hooker::FindGlowManager()
{
    // Call right above "Music.StopAllExceptMusic"
	uintptr_t instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																	(unsigned char*) XORSTR("\xE8\x00\x00\x00\x00\x48\x8B\x3D\x00\x00\x00\x00\xBE\x01\x00\x00\x00\xC7"),
																	XORSTR("x????xxx????xxxxxx"));

	glowManager = reinterpret_cast<GlowObjectManagerFn>(GetAbsoluteAddress(instruction_addr, 1, 5))();
}

void Hooker::FindPlayerResource()
{
	uintptr_t instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																	(unsigned char*) XORSTR("\x48\x8B\x05\x00\x00\x00\x00\x55\x48\x89\xE5\x48\x85\xC0\x74\x10\x48"),
																	XORSTR("xxx????xxxxxxxxxx"));

	csPlayerResource = reinterpret_cast<C_CSPlayerResource**>(GetAbsoluteAddress(instruction_addr, 3, 7));
}

void Hooker::FindGameRules()
{
	uintptr_t instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																	(unsigned char*) XORSTR("\x48\x8B\x05"
																									"\x00\x00\x00\x00"
																									"\x48\x8B\x38\x0F\x84"),
																	XORSTR("xxx????xxxxx"));

	csGameRules = *reinterpret_cast<C_CSGameRules***>(GetAbsoluteAddress(instruction_addr, 3, 7));
}

void Hooker::FindRankReveal()
{
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x55\x48\x89\xE5\x41\x54\x53\x48\x89\xFB\x48\x8B\x3D\x00\x00\x00\x00\x48\x85\xFF"),
																XORSTR("xxxxxxxxxxxxx????xxx"));

	MsgFunc_ServerRankRevealAll = reinterpret_cast<MsgFunc_ServerRankRevealAllFn>(func_address);
}

// "ClanTagChanged"
void Hooker::FindSendClanTag()
{
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("engine_client.so"),
																(unsigned char*) XORSTR("\x55\x48\x89\xE5\x41\x55\x49\x89\xFD\x41\x54\xBF\x48\x00\x00\x00\x49\x89\xF4\x53\x48\x83\xEC\x08\xE8"
                                                                                                "\x00\x00\x00\x00"
                                                                                                "\x48\x8D\x35"
                                                                                                "\x00\x00\x00\x00"
                                                                                                "\x48\x89\xC7\x48\x89\xC3\xE8"
                                                                                                "\x00\x00\x00\x00"
                                                                                                "\x48\x8D\x35"
                                                                                                "\x00\x00\x00\x00"
                                                                                                "\x4C\x89\xEA"),
																XORSTR("xxxxxxxxxxxxxxxxxxxxxxxxx"
                                                                               "????"
                                                                               "xxx"
                                                                               "????"
                                                                               "xxxxxxx"
                                                                               "????"
                                                                               "xxx"
                                                                               "????"
                                                                               "xxx" ));

	SendClanTag = reinterpret_cast<SendClanTagFn>(func_address);
}


void Hooker::FindHostDisconnect()
{
	//  xref cs_game_disconnected
	//   0059a510      48 8b 05        MOV        RAX,qword ptr [PTR_PTR_DAT_00ed2140]             = 00e2a460
	//                  29 7c 93 00
	//    0059a517      55              PUSH       RBP
	//    0059a518      31 c9           XOR        ECX,ECX
	//    0059a51a      48 8d 35        LEA        RSI,[s_cs_game_disconnected_00a26ec6]            = "cs_game_disconnected"
	//                  a5 c9 48 00
	//    0059a521      48 89 e5        MOV        RBP,RSP
	//    0059a524      41 54           PUSH       R12
	//    0059a526      53              PUSH       RBX
	//    0059a527      31 d2           XOR        EDX,EDX
	//    0059a529      89 fb           MOV        EBX,bShowMainMenu
	//    0059a52b      4c 8b 20        MOV        R12,qword ptr [RAX]=>PTR_DAT_00e2a460            = 010d0e00
	//    0059a52e      49 8b 04 24     MOV        RAX,qword ptr [R12]=>DAT_010d0e00                = ??
	//    0059a532      4c 89 e7        MOV        bShowMainMenu,R12
	//                              LAB_0059a535                                    XREF[1]:     00c0a1ed(*)
	//    0059a535      ff 50 40        CALL       qword ptr [RAX + 0x40]
	//    0059a538      48 85 c0        TEST       RAX,RAX
	//    0059a53b      74 0d           JZ         LAB_0059a54a
	//    0059a53d      49 8b 14 24     MOV        RDX,qword ptr [R12]=>DAT_010d0e00                = ??
	//    0059a541      48 89 c6        MOV        RSI,RAX
	//    0059a544      4c 89 e7        MOV        bShowMainMenu,R12
	//    0059a547      ff 52 50        CALL       qword ptr [RDX + 0x50]
	
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("engine_client.so"),
	                                                            (unsigned char*) XORSTR("\x48\x8b\x05\x00\x00\x00\x00\x55\x31\xc9\x48\x8d\x35\x00\x00\x00\x00\x48\x89\xe5\x41\x54\x53\x31\xd2\x89\xfb\x4c\x8b\x20\x49\x8b\x04\x24\x4c\x89\xe7\xff\x50\x00\x48\x85\xc0\x74\x00\x49\x8b\x14\x24\x48\x89\xc6\x4c\x89\xe7\xff\x52\x00"),
	                                                            XORSTR("xxx????xxxxxx????xxxxxxxxxxxxxxxxxxxxxx?xxxx?xxxxxxxxxxxx?" ));
	
	Host_Disconnect = reinterpret_cast<HostDisconnectFn>(func_address);
}

void Hooker::FindViewRender()
{
	// 48 8D 3D ?? ?? ?? ?? 48 8B 10 48 89 08 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 89 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3
    	// below "PrecacheCSViewScene" about 3 times, look for the one that gets loaded into rdi
	// 48 8D 05 07 D8 11 01    lea     rax, aPrecachecsview ; "PrecacheCSViewScene"
	// C7 05 27 9F 46 06 00 00+mov     cs:dword_6B6FB88, 0
	// 48 89 05 28 9F 46 06    mov     cs:qword_6B6FB90, rax
	// 48 8B 05 61 D5 A3 01    mov     rax, cs:off_21431D0
	// 48 8D 0D 0A 9F 46 06    lea     rcx, qword_6B6FB80
	// 48 8D 3D 63 A0 46 06    lea     rdi, g_ViewRender
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
										(unsigned char*) XORSTR("\x48\x8D\x3D\x00\x00\x00\x00\x48\x8B\x10\x48\x89\x08\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x89\x15\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xF3"),
										XORSTR("xxx????xxxxxxxxx????xxx????xxx????x????x"));

	viewRender = reinterpret_cast<CViewRender*>(GetAbsoluteAddress(func_address, 3, 7));
}

void Hooker::FindPrediction()
{
	uintptr_t seed_instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																		 (unsigned char*) XORSTR("\x48\x8B\x05"
                                                                                                         "\x00\x00\x00\x00"
                                                                                                         "\x8B\x38\xE8"
                                                                                                         "\x00\x00\x00\x00"
                                                                                                         "\x89\xC7"),
																		 XORSTR("xxx????xxx????xx"));
	uintptr_t helper_instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																		   (unsigned char*) XORSTR("\x00\x48\x89\x3D\x00\x00\x00\x00\xC3"),
																		   XORSTR("xxxx????x"));
	uintptr_t movedata_instruction_addr = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																			 (unsigned char*) XORSTR("\x48\x8B\x0D"
																											 "\x00\x00\x00\x00"
																											 "\x4C\x89\xEA"),
																			 XORSTR("xxx????xxx"));

	nPredictionRandomSeed = *reinterpret_cast<int**>(GetAbsoluteAddress(seed_instruction_addr, 3, 7));
	moveHelper = *reinterpret_cast<IMoveHelper**>(GetAbsoluteAddress(helper_instruction_addr + 1, 3, 7));
	g_MoveData = **reinterpret_cast<CMoveData***>(GetAbsoluteAddress(movedata_instruction_addr, 3, 7));
}

void Hooker::FindSetLocalPlayerReady()
{
	// xref: "deferred"
	uintptr_t func_address = PatternFinder::FindPatternInModule((XORSTR("/client_client.so")),
																(unsigned char*) XORSTR("\x55\x48\x89\xF7\x48\x8D\x35\x00\x00\x00\x00\x48\x89\xE5\xE8\x00\x00\x00\x00\x85\xC0"),
																XORSTR("xxxxxxx????xxxx????xx"));

	SetLocalPlayerReady = reinterpret_cast<SetLocalPlayerReadyFn>(func_address);
}

void Hooker::FindSurfaceDrawing()
{
	uintptr_t start_func_address = PatternFinder::FindPatternInModule(XORSTR("vguimatsurface_client.so"),
																	  (unsigned char*) XORSTR("\x55\x48\x89\xE5\x53\x48\x89\xFB\x48\x83\xEC\x28\x80\x3D"),
																	  XORSTR("xxxxxxxxxxxxxx"));
	StartDrawing = reinterpret_cast<StartDrawingFn>(start_func_address);

	uintptr_t finish_func_address = PatternFinder::FindPatternInModule(XORSTR("vguimatsurface_client.so"),
																	   (unsigned char*) XORSTR("\x55\x31\xFF\x48\x89\xE5\x53"),
																	   XORSTR("xxxxxxx"));
	FinishDrawing = reinterpret_cast<FinishDrawingFn>(finish_func_address);
}

void Hooker::FindGetLocalClient()
{
	uintptr_t GetLocalPlayer = reinterpret_cast<uintptr_t>(getvtable(engine)[12]);
	GetLocalClient = reinterpret_cast<GetLocalClientFn>(GetAbsoluteAddress(GetLocalPlayer + 9, 1, 5));
}

void Hooker::FindLineGoesThroughSmoke()
{
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x40\x0F\xB6\xFF\x55"),
																XORSTR("xxxxx"));
	LineGoesThroughSmoke = reinterpret_cast<LineGoesThroughSmokeFn>(func_address);
}

void Hooker::FindInitKeyValues()
{
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x81\x27\x00\x00\x00\xFF\x55\x31\xC0\x48\x89\xE5\x5D"),
																XORSTR("xxxxxxxxxxxxx"));
	InitKeyValues = reinterpret_cast<InitKeyValuesFn>(func_address);
}

void Hooker::FindLoadFromBuffer()
{
	// xref "%s.ctx" to ReadEncryptedKVFile()
	// LoadFromBuffer is called near the end, right before _MemFreeScratch()
	// 55 48 89 E5 41 57 41 56 41 55 41 54 49 89 D4 53 48 81 EC ?? ?? ?? ?? 48 85
	// Start of LoadFromBuffer()
	// 55                      push    rbp
	// 48 89 E5                mov     rbp, rsp
	// 41 57                   push    r15
	// 41 56                   push    r14
	// 41 55                   push    r13
	// 41 54                   push    r12
	// 49 89 D4                mov     r12, rdx
	// 53                      push    rbx
	// 48 81 EC 88 00 00 00    sub     rsp, 88h
	// 48 85 D2                test    rdx, rdx
	uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x55"
																						"\x48\x89\xE5"
																						"\x41\x57"
																						"\x41\x56"
																						"\x41\x55"
																						"\x41\x54"
																						"\x49\x89\xD4"
																						"\x53"
																						"\x48\x81\xEC\x00\x00\x00\x00"
																						"\x48\x85"),
																XORSTR("xxxxxxxxxxxxxxxxxxx????xx"));
	LoadFromBuffer = reinterpret_cast<LoadFromBufferFn>(func_address);
}

void Hooker::FindVstdlibFunctions()
{
	void* handle = dlopen(XORSTR("./bin/linux64/libvstdlib_client.so"), RTLD_NOLOAD | RTLD_NOW);

	RandomSeed = reinterpret_cast<RandomSeedFn>(dlsym(handle, XORSTR("RandomSeed")));
	RandomFloat = reinterpret_cast<RandomFloatFn>(dlsym(handle, XORSTR("RandomFloat")));
	RandomFloatExp = reinterpret_cast<RandomFloatExpFn>(dlsym(handle, XORSTR("RandomFloatExp")));
	RandomInt = reinterpret_cast<RandomIntFn>(dlsym(handle, XORSTR("RandomInt")));
	RandomGaussianFloat = reinterpret_cast<RandomGaussianFloatFn>(dlsym(handle, XORSTR("RandomGaussianFloat")));

	dlclose(handle);
}

void Hooker::FindOverridePostProcessingDisable()
{
	uintptr_t bool_address = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x80\x3D"
                                                                                                "\x00\x00\x00\x00\x00"
                                                                                                "\x89\xB5"
                                                                                                "\x00\x00"
                                                                                                "\xFF\xFF"),
																XORSTR("xx?????xx??xx"));
	bool_address = GetAbsoluteAddress(bool_address, 2, 7);

	s_bOverridePostProcessingDisable = reinterpret_cast<bool*>(bool_address);
}

void Hooker::FindModelBoneCounter()
{
	//                             LAB_008eec70                                    XREF[1]:     008ee6d7(j)
	//    008eec70      48 8b 05        MOV        RAX,qword ptr [DAT_0237b650]                     = ??
	//                  d9 c9 a8 01
	//    008eec77      c7 83 90        MOV        dword ptr [RBX + 0x2f90],0xff7fffff
	//                  2f 00 00
	//                  ff ff 7f ff
	//    008eec81      48 83 e8 01     SUB        RAX,0x1
	uintptr_t invalidatebonecachesnippet = PatternFinder::FindPatternInModule(XORSTR("/client_client.so"),
																(unsigned char*) XORSTR("\x48\x8b\x05\x00\x00\x00\x00\xc7\x83\x00\x00\x00\x00\xff\xff\x7f\xff\x48\x83\xe8\x01"),
																XORSTR("xxx????xx????xxxxxxxx"));
	
	g_iModelBoneCounter = reinterpret_cast<unsigned long*>(GetAbsoluteAddress(invalidatebonecachesnippet, 3, 7));
}

void Hooker::FindSDLInput()
{
	/*
	    0F 95 83 AC 01 00 00    setnz   byte ptr [rbx+1ACh]
		E8 E2 B7 FF FF          call    _memcpy
		E8 FD D8 02 00          call    LauncherMgrCreateFunc <------
	 */
	uintptr_t startAddr = PatternFinder::FindPatternInModule(XORSTR("launcher_client.so"),
																(unsigned char*) XORSTR("\x0F\x95\x83"
																								"\x00\x00\x00\x00"
																								"\xE8"
																								"\x00\x00\x00\x00"
																								"\xE8"),
																XORSTR("xxx????x????x"));
	ILauncherMgrCreateFn createFunc = reinterpret_cast<ILauncherMgrCreateFn>(GetAbsoluteAddress(startAddr + 12, 1, 5));
	launcherMgr = createFunc();
}

void Hooker::FindSetNamedSkybox()
{
	//55 4C 8D 05 ?? ?? ?? ?? 48 89 E5 41
    // xref for "skybox/%s%s"
    uintptr_t func_address = PatternFinder::FindPatternInModule(XORSTR("engine_client.so"),
                                                                (unsigned char*) XORSTR("\x55\x4C\x8D\x05"
                                                                                                "\x00\x00\x00\x00" //??
                                                                                                "\x48\x89\xE5\x41"),
                                                                XORSTR("xxxx????xxxx"));

    SetNamedSkyBox = reinterpret_cast<SetNamedSkyBoxFn>(func_address);
}

void Hooker::FindPanelArrayOffset()
{
	/*
	 * CUIEngine::IsValidPanelPointer()
	 *
	   55                      push    rbp
	   48 81 C7 B8 01 00 00    add     rdi, 1B8h <--------
	 */
	uintptr_t IsValidPanelPointer = reinterpret_cast<uintptr_t>(getvtable( panoramaEngine->AccessUIEngine() )[37]);
	int32_t offset = *(unsigned int*)(IsValidPanelPointer + 4);
	panorama::panelArray = *(panorama::PanelArray**) ( ((uintptr_t)panoramaEngine->AccessUIEngine()) + offset + 8);
}

void Hooker::FindPlayerAnimStateOffset()
{
	// In C_CSPlayer::Spawn, it references the offset of each player's animation state so it can do a reset.
	// mov     rdi, [rbx+4148h] <----- this offset(0x4148)
	// test    rdi, rdi
	// jz      short loc_C50837
	// call    AnimState__Reset
	// 55 48 89 E5 53 48 89 FB 48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8B 00
	uintptr_t C_CSPlayer__Spawn = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
																	  ( unsigned char* ) XORSTR("\x55\x48\x89\xE5\x53\x48\x89\xFB\x48\x83\xEC\x28\x48\x8B\x05"
																										"\x00\x00\x00\x00" //??
																										"\x48\x8B\x00"),
																	  XORSTR( "xxxxxxxxxxxxxxx????xxx" ) );
	Offsets::playerAnimStateOffset = *(unsigned int*)(C_CSPlayer__Spawn + 52);
}

void Hooker::FindPlayerAnimStateFunctions()
{
	//                             undefined __stdcall CreateAnimState(void * player)
	//              undefined         AL:1           <RETURN>
	//              void *            RDI:8          player
	//                              CreateAnimState                                 XREF[5]:     00f6e179(c), 01aa23ec,
	//                                                                                           01c6ccc0(*), 01e4cd29(*),
	//                                                                                           01e4cd31(*)
	//    00fcfd30      55              PUSH       RBP
	//    00fcfd31      48 85 ff        TEST       player,player
	//    00fcfd34      48 89 e5        MOV        RBP,RSP
	//    00fcfd37      41 54           PUSH       R12
	//    00fcfd39      53              PUSH       RBX
	//    00fcfd3a      48 89 fb        MOV        RBX,player
	//    00fcfd3d      74 0d           JZ         LAB_00fcfd4c
	//    00fcfd3f      48 8b 07        MOV        RAX,qword ptr [player]
	//                              LAB_00fcfd42                                    XREF[1]:     01e4cd27(*)
	//    00fcfd42      ff 90 88        CALL       qword ptr [RAX + 0x688]
	//                  06 00 00
	//    00fcfd48      84 c0           TEST       AL,AL
	//    00fcfd4a      75 24           JNZ        LAB_00fcfd70
	//                              LAB_00fcfd4c                                    XREF[1]:     00fcfd3d(j)
	//    00fcfd4c      45 31 e4        XOR        R12D,R12D
	uintptr_t funcAddr = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
	                                                                  ( unsigned char* ) XORSTR("\x55\x48\x85\xff\x48\x89\xe5\x41\x54\x53\x48\x89\xfb\x74\x00\x48\x8b\x07\xff\x90\x00\x00\x00\x00\x84\xc0\x75\x00\x45\x31\xe4"),
	                                                                  XORSTR( "xxxxxxxxxxxxxx?xxxxx????xxx?xxx" ) );
	CreateAnimState = reinterpret_cast<CreateAnimStateFn>( funcAddr );
	//                             void __stdcall Reset(void * this)
	//              void              <VOID>         <RETURN>
	//              void *            RDI:8          this
	//              undefined4        Stack[-0x1c]:4 local_1c                                XREF[3]:     00fcf705(W),
	//                                                                                                    00fcf70f(R),
	//                                                                                                    00fcfa7c(R)
	//                              CCSGOPlayerAnimState::Reset                     XREF[6]:     FUN_00f732c0:00f748dc(c),
	//                                                                                           FUN_00f75020:00f7505d(c),
	//                                                                                           FUN_00f7a110:00f7a1b7(c),
	//                                                                                           CCSGOPlayerAnimState::CCSGOPlaye
	//                                                                                           01aa23dc, 01c6cc68(*)
	//    00fcf6d0      55              PUSH       RBP
	//    00fcf6d1      48 8d 35        LEA        RSI,[DAT_019a4cc0]
	//                  e8 55 9d 00
	//    00fcf6d8      ba 01 00        MOV        EDX,0x1
	//                  00 00
	//    00fcf6dd      48 89 e5        MOV        RBP,RSP
	//    00fcf6e0      53              PUSH       RBX
	//    00fcf6e1      48 89 fb        MOV        RBX,this
	//    00fcf6e4      48 83 ec 18     SUB        RSP,0x18
	
	uintptr_t funcAddr1 = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
                                                  ( unsigned char* ) XORSTR("\x55\x48\x8d\x35\x00\x00\x00\x00\xba\x01\x00\x00\x00\x48\x89\xe5\x53\x48\x89\xfb\x48\x83\xec\x18"),
                                                  XORSTR( "xxxx????xxxxxxxxxxxxxxxx" ) );
	AnimStateReset = reinterpret_cast<AnimStateResetFn>( funcAddr1 );
	//                             void __stdcall CCSGOPlayerAnimState::Update(void * this,
	//              void              <VOID>         <RETURN>
	//              void *            RDI:8          this
	//              float             XMM0_Da:4      eyeYaw
	//              float             XMM1_Da:4      eyePitch
	//              bool              SIL:1          bForce
	//              undefined4        Stack[-0x30]:4 local_30                                XREF[1]:     00fcf518(W)
	//              undefined4        Stack[-0x34]:4 local_34                                XREF[1]:     00fcf51f(W)
	//              undefined4        Stack[-0x38]:4 local_38                                XREF[2]:     00fcf505(*),
	//                                                                                                    00fcf509(W)
	//              undefined4        Stack[-0x3c]:4 local_3c                                XREF[6]:     00fcf202(W),
	//                                                                                                    00fcf221(R),
	//                                                                                                    00fcf24b(W),
	//                                                                                                    00fcf255(R),
	//                                                                                                    00fcf2aa(W),
	//                                                                                                    00fcf2c4(R)
	//                              CCSGOPlayerAnimState::Update                    XREF[7]:     FUN_00f64f40:00f650b9(c),
	//                                                                                           FUN_00f75020:00f75096(c),
	//                                                                                           01aa23d4, 01c6cc38(*),
	//                                                                                           01e4ccf9(*), 01e4cd05(*),
	//                                                                                           01e4cd0a(*)
	//    00fcf1f0      55              PUSH       RBP
	//    00fcf1f1      48 89 e5        MOV        RBP,RSP
	//    00fcf1f4      41 56           PUSH       R14
	//    00fcf1f6      41 55           PUSH       R13
	//    00fcf1f8      41 54           PUSH       R12
	//    00fcf1fa      53              PUSH       RBX
	//    00fcf1fb      48 89 fb        MOV        RBX,this
	//    00fcf1fe      48 83 ec 20     SUB        RSP,0x20
	//    00fcf202      f3 0f 11        MOVSS      dword ptr [RBP + local_3c],eyePitch
	uintptr_t funcAddr2 = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
	                                                      ( unsigned char* ) XORSTR("\x55\x48\x89\xe5\x41\x56\x41\x55\x41\x54\x53\x48\x89\xfb\x48\x83\xec\x20\xf3\x0f\x11\x4d\xcc"),
	                                                      XORSTR( "xxxxxxxxxxxxxxxxxxxxxxx" ) );
	AnimStateUpdate = reinterpret_cast<AnimStateUpdateFn>( funcAddr2 );
}


void Hooker::FindPlayerAnimOverlayOffset( )
{
    // C_BaseAnimatingOverlay::GetAnimOverlay - Adding 35 to get to the offset
    // 55 48 89 E5 41 56 41 55 41 89 F5 41 54 53 48 89 FB 8B
    Offsets::playerAnimOverlayOffset = *(unsigned int*)(PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
                                                                               ( unsigned char* ) XORSTR("\x55\x48\x89\xE5\x41\x56\x41\x55\x41\x89\xF5\x41\x54\x53\x48\x89\xFB\x8B"),
                                                                               XORSTR( "xxxxxxxxxxxxxxxxxx" ) ) + 35);
}

void Hooker::FindSequenceActivity()
{
    // C_BaseAnimating::GetSequenceActivity()
    // 83 FE FF                                cmp     esi, 0FFFFFFFFh
    // 74 6B                                   jz      short loc_7A1F40
    // 55                                      push    rbp
    // 48 89 E5                                mov     rbp, rsp
    // 53                                      push    rbx
    // 48 89 FB                                mov     rbx, rdi
    // 48 83 EC 18                             sub     rsp, 18h
    // 48 8B BF D0 2F 00 00                    mov     rdi, [rdi+2FD0h]
    // 48 85 FF                                test    rdi, rdi
    // 74 13                                   jz      short loc_7A1F00
    // loc_7A1EED:                             ; CODE XREF: GetSequenceActivity+5F↓j
    // 48 83 3F 00                             cmp     qword ptr [rdi], 0
    // 74 3E                                   jz      short loc_7A1F31
    // 48 83 C4 18                             add     rsp, 18h
    // 31 D2                                   xor     edx, edx
    // 83 FE FF 74 ?? 55 48 89 E5 53 48 89 FB 48 83 EC ?? 48 8B BF ?? ?? ?? ?? 48 85 FF 74 ?? 48 83 3F 00 74 ?? 48 83 C4 ?? 31
    uintptr_t funcAddr = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ), (unsigned char*) XORSTR( "\x83\xFE\xFF"
                                                                                                            "\x74\x00"
                                                                                                            "\x55"
                                                                                                            "\x48\x89\xE5"
                                                                                                            "\x53"
                                                                                                            "\x48\x89\xFB"
                                                                                                            "\x48\x83\xEC\x00"
                                                                                                            "\x48\x8B\xBF\x00\x00\x00\x00"
                                                                                                            "\x48\x85\xFF"
                                                                                                            "\x74\x00"
                                                                                                            "\x48\x83\x3F\x00"
                                                                                                            "\x74\x00"
                                                                                                            "\x48\x83\xC4\x00"
                                                                                                            "\x31"), XORSTR( "xxxx?xxxxxxxxxxx?xxx????xxxx?xxxxx?xxx?x" ) );

    GetSeqActivity = reinterpret_cast<GetSequenceActivityFn>( funcAddr );
}

void Hooker::FindAbsFunctions()
{
	// C_BaseEntity::SetAbsOrigin( )
	// 55 48 89 E5 41 55 41 54 49 89 F4 53 48 89 FB 48 83 EC ?? E8 ?? ?? ?? ?? F3
	SetAbsOriginFnAddr = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ),
															 ( unsigned char* ) XORSTR("\x55\x48\x89\xE5\x41\x55\x41\x54\x49\x89\xF4\x53\x48\x89\xFB\x48\x83\xEC"
																							   "\x00" //??
																							   "\xE8"
																							   "\x00\x00\x00\x00" //??
																							   "\xF3"),
															 XORSTR( "xxxxxxxxxxxxxxxxxx?x????x" ) );
}

typedef CItemSystem* (* GetItemSystemFn)( );

void Hooker::FindItemSystem()
{
    //xref almost any weapon name "weapon_glock" or "weapon_ak47"
    //above the string find a very commonly used function that has about 100xrefs
    // ItemSystem() proc near
    // 55                      push    rbp
    // 48 89 E5                mov     rbp, rsp
    // 53                      push    rbx
    // 48 89 FB                mov     rbx, rdi
    // 48 83 EC 08             sub     rsp, 8
    // 48 89 37                mov     [rdi], rsi
    // E8 9C 69 E3 FF          call    GetItemSystemWrapper
    // 48 8B 33                mov     rsi, [rbx]
    // 48 8B 10                mov     rdx, [rax]
    // 48 89 C7                mov     rdi, rax
    // FF 92 48 01 00 00       call    qword ptr [rdx+148h]
    // 48 89 43 08             mov     [rbx+8], rax
    // E8 84 69 E3 FF          call    GetItemSystemWrapper
    // 8B 40 3C                mov     eax, [rax+3Ch]
    // 89 43 10                mov     [rbx+10h], eax
    // 48 83 C4 08             add     rsp, 8
    // 5B                      pop     rbx
    // 5D                      pop     rbp
    // C3                      retn
    // -- GetItemSystemWrapper() --
    // 55                      push    rbp
    // 48 89 E5                mov     rbp, rsp
    // E8 87 FB FD FF          call    GetItemSystem
    // 5D                      pop     rbp
    // 48 83 C0 08             add     rax, 8
    // C3                      retn


    uintptr_t funcAddr = PatternFinder::FindPatternInModule( XORSTR( "/client_client.so" ), (unsigned char*) XORSTR("\x55\x48\x89\xE5\x53\x48\x89\xFB\x48\x83\xEC\x08\x48\x89\x37\xE8"
																									"\x00\x00\x00\x00" // ??
																									"\x48"), XORSTR( "xxxxxxxxxxxxxxxx????x" ) );
    funcAddr += 15; // add 15 to get to (E8 9C 69 E3 FF          call    GetItemSystemWrapper)
    funcAddr = GetAbsoluteAddress( funcAddr, 1, 5 ); // Deref to GetItemSystemWrapper()
    funcAddr += 4; // add 4 to Get to GetItemSystem()
    funcAddr = GetAbsoluteAddress( funcAddr, 1, 5 ); // Deref again for the final address.


    GetItemSystemFn GetItemSystem = reinterpret_cast<GetItemSystemFn>( funcAddr );
    uintptr_t itemSys = (uintptr_t)GetItemSystem();
    cvar->ConsoleDPrintf("ItemSystem(%p)\n", itemSys);
    itemSys += sizeof(void*); // 2nd vtable
    itemSystem = (CItemSystem*)itemSys;
}

void Hooker::FindCSMEnabled()
{
    //  sub_856E60 proc near
    // "48 8B 05 ?? ?? ?? ?? 55 48 89 E5 41 54 53 48  89 FB 48 8D 3D ?? ?? ?? ?? FF 90 ?? ?? ?? ?? 48"
    //	48 8B 05 19 C2 A3 01    mov     rax, cs:_cl_csm_enabled
    //	55                      push    rbp
    //	48 89 E5                mov     rbp, rsp
    //	41 54                   push    r12
    //	53                      push    rbx
    //	48 89 FB                mov     rbx, rdi
    //	48 8D 3D 08 C2 A3 01    lea     rdi, _cl_csm_enabled
    //	FF 90 80 00 00 00       call    qword ptr [rax+80h]
    //	48 8D 3D FC 27 FC 00    lea     rdi, aCsmEnabledI ; "CSM enabled: %i\n"
    uintptr_t funcAddr = PatternFinder::FindPatternInModule( XORSTR("/client_client.so"),
 (unsigned char*)XORSTR("\x48\x8B\x05"
	"\x00\x00\x00\x00"
	"\x55\x48\x89\xE5\x41\x54\x53\x48\x89\xFB\x48\x8D\x3D"
	"\x00\x00\x00\x00"
	"\xFF\x90"
	"\x00\x00\x00\x00"
	"\x48"),
	XORSTR("xxx????xxxxxxxxxxxxx????xx????x") );

    cl_csm_enabled = reinterpret_cast<ConVar*>( GetAbsoluteAddress( funcAddr, 3, 7 ) );
}