#include "chams.h"

#include "lagcomp.h"

#include <algorithm>
#include "../Utils/xorstring.h"
#include "../Utils/entity.h"
#include "../settings.h"
#include "../interfaces.h"


#include "../Utils/draw.h"

IMaterial* materialChams;
IMaterial* materialChamsIgnorez;
IMaterial* materialChamsFlat;
IMaterial* materialChamsFlatIgnorez;
IMaterial* materialChamsArms;
IMaterial* materialChamsWeapons;
matrix3x4_t fake_matrix[128];
QAngle currentAngle;

typedef void (*DrawModelExecuteFn) (void*, void*, void*, const ModelRenderInfo_t&, matrix3x4_t*);

static void DrawPlayer(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!Settings::ESP::Chams::enabled)
		return;

	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;

	C_BasePlayer* entity = (C_BasePlayer*) entityList->GetClientEntity(pInfo.entity_index);
	if (!entity
		|| entity->GetDormant()
		|| !entity->GetAlive())
		return;

	if (entity == localplayer && !Settings::ESP::Filters::localplayer)
		return;

	if (!Entity::IsTeamMate(entity, localplayer) && !Settings::ESP::Filters::enemies)
		return;

	if (entity != localplayer && Entity::IsTeamMate(entity,localplayer) && !Settings::ESP::Filters::allies)
		return;

	IMaterial* visible_material = nullptr;
	IMaterial* hidden_material = nullptr;

	switch (Settings::ESP::Chams::type)
	{
		case ChamsType::CHAMS:
		case ChamsType::CHAMS_XQZ:
			visible_material = materialChams;
			hidden_material = materialChamsIgnorez;
			break;
		case ChamsType::CHAMS_FLAT:
		case ChamsType::CHAMS_FLAT_XQZ:
			visible_material = materialChamsFlat;
			hidden_material = materialChamsFlatIgnorez;
			break;
	}
	
	ImColor visColor;
	ImColor color;
	
	if (entity == localplayer)
	{
		visColor = Settings::ESP::Chams::localplayerColor.Color(entity);
		color = visColor;
		
		color.Value.x *= 0.45f; // does same thing as color *= 0.45
		color.Value.y *= 0.45f;
		color.Value.z *= 0.45f;
	}
	else if (Entity::IsTeamMate(entity, localplayer))
	{
		visColor = Settings::ESP::Chams::allyVisibleColor.Color(entity);
		color = Settings::ESP::Chams::allyColor.Color(entity);
	}
	else if (!Entity::IsTeamMate(entity, localplayer))
	{
		visColor = Settings::ESP::Chams::enemyVisibleColor.Color(entity);
		color = Settings::ESP::Chams::enemyColor.Color(entity);
	}
	else
	{
		return;
	}
	
	visible_material->ColorModulate(visColor);
	hidden_material->ColorModulate(color);
	
	if (entity->GetImmune()) {
		visible_material->AlphaModulate(visColor.Value.w / 2);
		hidden_material->AlphaModulate(color.Value.w / 2);
	} else {
		visible_material->AlphaModulate(visColor.Value.w);
		hidden_material->AlphaModulate(color.Value.w);
	}

	if (!Settings::ESP::Filters::legit && (Settings::ESP::Chams::type == ChamsType::CHAMS_XQZ || Settings::ESP::Chams::type == ChamsType::CHAMS_FLAT_XQZ))
	{
		modelRender->ForcedMaterialOverride(hidden_material);
		modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, pCustomBoneToWorld);
	}

	modelRender->ForcedMaterialOverride(visible_material);
	// No need to call DME again, it already gets called in DrawModelExecute.cpp
}

static void DrawRecord(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!Settings::LagComp::enabled || !Settings::ESP::Chams::backtrackEnabled)
        return;

    C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;
	
	C_BasePlayer* entity = (C_BasePlayer*) entityList->GetClientEntity(pInfo.entity_index);
	if (!entity
	    || !entity->GetAlive())
		return;
		
	if (LagComp::ticks.empty())
		return;
	
	auto &tick = LagComp::ticks.back();
	for (auto &record : tick.records)
	{
		if (!record.boneMatrix || entity != record.entity)
			continue;
		
		IMaterial* visible_material = nullptr;
		IMaterial* hidden_material = nullptr;
		
		switch (Settings::ESP::Chams::backtrackType)
		{
			case ChamsType::CHAMS:
			case ChamsType::CHAMS_XQZ:
				visible_material = materialChams;
				hidden_material = materialChamsIgnorez;
				break;
			case ChamsType::CHAMS_FLAT:
			case ChamsType::CHAMS_FLAT_XQZ:
				visible_material = materialChamsFlat;
				hidden_material = materialChamsFlatIgnorez;
				break;
		}
		
		Color visColor = Color::FromImColor(Settings::ESP::Chams::backtrackVisibleColor.Color(entity));
		Color color = Color::FromImColor(Settings::ESP::Chams::backtrackColor.Color(entity));
		
		visible_material->ColorModulate(visColor);
		hidden_material->ColorModulate(color);
		
		visible_material->AlphaModulate(Settings::ESP::Chams::backtrackVisibleColor.Color(entity).Value.w);
		hidden_material->AlphaModulate(Settings::ESP::Chams::backtrackColor.Color(entity).Value.w);
		
		if (!Settings::ESP::Filters::legit && (Settings::ESP::Chams::backtrackType == ChamsType::CHAMS_XQZ || Settings::ESP::Chams::backtrackType == ChamsType::CHAMS_FLAT_XQZ))
		{
			modelRender->ForcedMaterialOverride(hidden_material);
			modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, (matrix3x4_t*)record.boneMatrix);
		}
		
		// pInfo.origin = record.origin;
		modelRender->ForcedMaterialOverride(visible_material);
		modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, (matrix3x4_t*)record.boneMatrix);
	}
}

static void DrawWeapon(const ModelRenderInfo_t& pInfo)
{
	if (!Settings::ESP::Chams::Weapon::enabled)
		return;

    	//turn weapon chams off while scoped so you can actually see with COD guns while scoped -Crazily.
    	C_BasePlayer *localPlayer = (C_BasePlayer *) entityList->GetClientEntity(engine->GetLocalPlayer());
    	if (localPlayer->IsScoped())
		return;

	std::string modelName = modelInfo->GetModelName(pInfo.pModel);
	IMaterial* mat = materialChamsWeapons;

	if (!Settings::ESP::Chams::Weapon::enabled)
		mat = material->FindMaterial(modelName.c_str(), TEXTURE_GROUP_MODEL);

	mat->ColorModulate(Settings::ESP::Chams::Weapon::color.Color());
	mat->AlphaModulate(Settings::ESP::Chams::Weapon::color.Color().Value.w);

	mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, Settings::ESP::Chams::Weapon::type == WeaponType::WIREFRAME);
	mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, Settings::ESP::Chams::Weapon::type == WeaponType::NONE);
	modelRender->ForcedMaterialOverride(mat);
}

static void DrawArms(const ModelRenderInfo_t& pInfo)
{
	if (!Settings::ESP::Chams::Arms::enabled)
		return;

	std::string modelName = modelInfo->GetModelName(pInfo.pModel);
	IMaterial* mat = materialChamsArms;

	if (!Settings::ESP::Chams::Arms::enabled)
		mat = material->FindMaterial(modelName.c_str(), TEXTURE_GROUP_MODEL);

	mat->ColorModulate(Settings::ESP::Chams::Arms::color.Color());
	mat->AlphaModulate(Settings::ESP::Chams::Arms::color.Color().Value.w);

	mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, Settings::ESP::Chams::Arms::type == ArmsType::WIREFRAME);
	mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, Settings::ESP::Chams::Arms::type == ArmsType::NONE);
	modelRender->ForcedMaterialOverride(mat);
}

static void DrawPlayerReal(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer || !localplayer->GetAlive())
		return;
	
	C_BasePlayer* entity = (C_BasePlayer*) entityList->GetClientEntity(pInfo.entity_index);
	if (!entity || !entity->GetAlive())
		return;
	
	if (entity != localplayer)
		return;

	if (!fake_matrix)
		return;
	
	IMaterial* visible_material = materialChams;

	Color visColor = Color(255,0,0);
	
	visible_material->ColorModulate(visColor);
	
	visible_material->AlphaModulate(1);
	
	modelRender->ForcedMaterialOverride(visible_material);
	modelRenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(thisptr, context, state, pInfo, fake_matrix);

}

void Chams::DrawModelExecute(void* thisptr, void* context, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	if (!engine->IsInGame())
		return;

	if (!Settings::ESP::enabled)
		return;

	if (!pInfo.pModel)
		return;

	static bool materialsCreated = false;
	if (!materialsCreated)
	{
		materialChams = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsIgnorez = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), true, true, true, true, true);
		materialChamsFlat = Util::CreateMaterial(XORSTR("UnlitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsFlatIgnorez = Util::CreateMaterial(XORSTR("UnlitGeneric"), XORSTR("VGUI/white_additive"), true, true, true, true, true);
		materialChamsArms = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialChamsWeapons = Util::CreateMaterial(XORSTR("VertexLitGeneric"), XORSTR("VGUI/white_additive"), false, true, true, true, true);
		materialsCreated = true;
	}

	const char *modelName = modelInfo->GetModelName(pInfo.pModel);

	if (strstr(modelName, XORSTR("models/player")))
	{
		DrawRecord(thisptr, context, state, pInfo, pCustomBoneToWorld);
		// DrawPlayerReal(thisptr, context, state, pInfo, pCustomBoneToWorld);
		DrawPlayer(thisptr, context, state, pInfo, pCustomBoneToWorld);
	}
	else if (strstr(modelName, XORSTR("arms")))
	{
		DrawArms(pInfo);
	}
	else if (strstr(modelName, XORSTR("weapon")))
	{
		DrawWeapon(pInfo);
	}
}

void Chams::CreateMove(CUserCmd* cmd){
	currentAngle = cmd->viewangles;
}

void Chams::FrameStageNotify(ClientFrameStage_t stage) // blatant c+p https://www.unknowncheats.me/forum/counterstrike-global-offensive/353751-proper-desync-chams.html
{
    return;
    
    if (stage != ClientFrameStage_t::FRAME_RENDER_START)
		return;

    if (!engine->IsInGame())
		return;

    C_BasePlayer* localplayer = (C_BasePlayer*)entityList->GetClientEntity(engine->GetLocalPlayer());
    if (!localplayer || !localplayer->GetAlive())
		return;

    static CBaseHandle* handle = localplayer->GetRefEHandle();
    static float spawntime = localplayer->GetSpawnTime();
    static CCSGOAnimState* fake_anim_state = nullptr;

    bool allocate = (!fake_anim_state);
    bool change = (!allocate) && (localplayer->GetRefEHandle() != handle);
    bool reset = (!allocate && !change) && (localplayer->GetSpawnTime() != spawntime);

    if (change)
    {
		free(fake_anim_state);
		fake_anim_state = nullptr;
    }

    if (reset)
    {
		AnimStateReset(fake_anim_state);
		spawntime = localplayer->GetSpawnTime();
    }

    if (allocate || change)
    {
		fake_anim_state = CreateAnimState(localplayer);
		handle = localplayer->GetRefEHandle();
		spawntime = localplayer->GetSpawnTime();
    }
    else if (localplayer->GetSimulationTime() != localplayer->GetOldSimulationTime())
    {
		std::array<AnimationLayer, 15> networked_layers;
		std::copy_n(localplayer->GetAnimOverlay()->m_Memory.m_pMemory, 15, networked_layers.begin());

		QAngle backup_abs_angles = localplayer->GetAbsAngles();

		float backup_poses[24];
		// std::copy(localplayer->GetPoseParameters(), localplayer->GetPoseParameters() + 24, backup_poses);
        for ( int i=0;i<24;i++ )
        {
            backup_poses[i] = localplayer->GetPoseParameters()[i];
        }

		AnimStateUpdate(fake_anim_state, 0.0f, 1.0f, false);
		localplayer->InvalidateBoneCache();
		localplayer->SetupBones(fake_matrix, 128, 0x7FF00, globalVars->curtime);

		std::copy(networked_layers.begin(), networked_layers.end(), localplayer->GetAnimOverlay()->m_Memory.m_pMemory);
		// std::copy(backup_poses, backup_poses + 24, localplayer->GetPoseParameters());

        for ( int i=0;i<24;i++ )
        {
            localplayer->GetPoseParameters()[i] = backup_poses[i];
        }

		QAngle& fuck = const_cast<QAngle&>(localplayer->GetAbsAngles());
		fuck = backup_abs_angles;

		if (localplayer->GetAbsAngles() != backup_abs_angles)
		    cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0), XORSTR("abs angles are different!\n"));

		if (std::equal(networked_layers.begin(), networked_layers.end(), localplayer->GetAnimOverlay()->m_Memory.m_pMemory))
		    cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0), XORSTR("networked layers are different! (animoverlay size is %d btw)\n"), localplayer->GetAnimOverlay()->Count());

        for ( int i=0;i<24;i++ )
        {
	    	if(localplayer->GetPoseParameters()[i] != backup_poses[i])
        		cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0), XORSTR("poses are different! index: %d was: %f now: %f\n"), i, backup_poses[i], localplayer->GetPoseParameters()[i]);
        }
	        // if (std::equal(std::begin(backup_poses), std::end(backup_poses), localplayer->GetPoseParameters()))
		//     cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0), XORSTR("poses are different!\n"));
	    }
}
