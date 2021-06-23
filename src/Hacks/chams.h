#pragma once

#include "../SDK/IInputSystem.h"
#include "../SDK/vector.h"
#include "../SDK/IVModelRender.h"
#include "../SDK/definitions.h"

namespace Chams
{
	//Hooks
	void DrawModelExecute(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld);
	void FrameStageNotify(ClientFrameStage_t stage);
	void CreateMove(CUserCmd* cmd);
}

extern CreateAnimStateFn CreateAnimState;
extern AnimStateUpdateFn AnimStateUpdate;
extern AnimStateResetFn AnimStateReset;