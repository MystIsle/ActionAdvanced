// Fill out your copyright notice in the Description page of Project Settings.

#include "ACCombatFeelSettings.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable CVarFeelMaster(
		TEXT("AA.Feel.Master"),
		1,
		TEXT("0: 모든 타격 연출 off, 1: on. 개별 AA.Feel.* 위의 마스터 스위치. (데모 토글)"),
		ECVF_Cheat);

	TAutoConsoleVariable CVarFeelHitStop(
		TEXT("AA.Feel.HitStop"),
		1,
		TEXT("0: 히트스톱(+VFX 타임스케일 싱크) off. (데모 토글)"),
		ECVF_Cheat);

	TAutoConsoleVariable CVarFeelHitFX(
		TEXT("AA.Feel.HitFX"),
		1,
		TEXT("0: 히트 나이아가라 FX off. (데모 토글)"),
		ECVF_Cheat);

	TAutoConsoleVariable CVarFeelFlash(
		TEXT("AA.Feel.Flash"),
		1,
		TEXT("0: 히트 점멸 off. (데모 토글)"),
		ECVF_Cheat);

	TAutoConsoleVariable CVarFeelCameraShake(
		TEXT("AA.Feel.CameraShake"),
		1,
		TEXT("0: 카메라 셰이크 off. (데모 토글)"),
		ECVF_Cheat);

	TAutoConsoleVariable CVarFeelMeshShake(
		TEXT("AA.Feel.MeshShake"),
		1,
		TEXT("0: 캐릭터(메시) 셰이크 off. (데모 토글)"),
		ECVF_Cheat);

	IConsoleVariable* GetLayerVariable(EACFeelLayer Layer)
	{
		switch (Layer)
		{
		case EACFeelLayer::HitStop:     return CVarFeelHitStop.AsVariable();
		case EACFeelLayer::HitFX:       return CVarFeelHitFX.AsVariable();
		case EACFeelLayer::Flash:       return CVarFeelFlash.AsVariable();
		case EACFeelLayer::CameraShake: return CVarFeelCameraShake.AsVariable();
		case EACFeelLayer::MeshShake:   return CVarFeelMeshShake.AsVariable();
		default:                        return nullptr;
		}
	}
}

bool UACCombatFeelSettings::IsFeelLayerEnabled(EACFeelLayer Layer)
{
	if (CVarFeelMaster.GetValueOnGameThread() == 0)
	{
		return false;
	}

	switch (Layer)
	{
	case EACFeelLayer::HitStop:     return CVarFeelHitStop.GetValueOnGameThread() != 0;
	case EACFeelLayer::HitFX:       return CVarFeelHitFX.GetValueOnGameThread() != 0;
	case EACFeelLayer::Flash:       return CVarFeelFlash.GetValueOnGameThread() != 0;
	case EACFeelLayer::CameraShake: return CVarFeelCameraShake.GetValueOnGameThread() != 0;
	case EACFeelLayer::MeshShake:   return CVarFeelMeshShake.GetValueOnGameThread() != 0;
	default:                        return true;
	}
}

void UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer Layer)
{
	if (IConsoleVariable* Var = GetLayerVariable(Layer))
	{
		Var->Set(Var->GetInt() == 0 ? TEXT("1") : TEXT("0"), ECVF_SetByConsole);
	}
}

void UACCombatFeelSettings::ToggleFeelMaster()
{
	if (IConsoleVariable* Var = CVarFeelMaster.AsVariable())
	{
		Var->Set(Var->GetInt() == 0 ? TEXT("1") : TEXT("0"), ECVF_SetByConsole);
	}
}

bool UACCombatFeelSettings::GetFeelLayerRaw(EACFeelLayer Layer)
{
	const IConsoleVariable* Var = GetLayerVariable(Layer);
	return Var && Var->GetInt() != 0;
}

bool UACCombatFeelSettings::GetFeelMaster()
{
	return CVarFeelMaster.GetValueOnGameThread() != 0;
}

const TCHAR* UACCombatFeelSettings::GetFeelLayerLabel(EACFeelLayer Layer)
{
	switch (Layer)
	{
	case EACFeelLayer::HitStop:     return TEXT("HitStop");
	case EACFeelLayer::HitFX:       return TEXT("HitFX");
	case EACFeelLayer::Flash:       return TEXT("Flash");
	case EACFeelLayer::CameraShake: return TEXT("CameraShake");
	case EACFeelLayer::MeshShake:   return TEXT("MeshShake");
	default:                        return TEXT("?");
	}
}
