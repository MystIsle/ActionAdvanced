// Fill out your copyright notice in the Description page of Project Settings.

#include "ACTargetingLibrary.h"

#include "GenericTeamAgentInterface.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif

namespace
{
	TAutoConsoleVariable CVarAutoTargetDebug(
		TEXT("AA.AutoTarget.Debug"),
		0,
		TEXT("0: off, 1: 오토타게팅이 발동 시 고른 타겟/콘을 표시"),
		ECVF_Cheat);
}

AActor* UACTargetingLibrary::FindBestTarget(ACharacter* Source, float Range, float HalfAngle,
                                            FVector AimDirection, float RangeWeightRate)
{
	if (Source == nullptr || Range <= 0.f)
	{
		return nullptr;
	}

	const UWorld* World = Source->GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const FVector Origin = Source->GetActorLocation();

	FVector Aim = AimDirection.GetSafeNormal2D();
	if (Aim.IsNearlyZero())
	{
		Aim = Source->GetActorForwardVector().GetSafeNormal2D();
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionQueryParams Params(FName("ACAutoTargetAcquire"), false, Source);
	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Range),
		Params
	);
	
#if ENABLE_DRAW_DEBUG
	const bool bDrawDebug = CVarAutoTargetDebug.GetValueOnGameThread() != 0;
		constexpr float DrawTime = 1.8f;
	
#endif

	AActor* Result = nullptr;
	float BestScore = -FLT_MAX;
	float BestDist = 0.f;
	float BestAngle = 0.f;

	TSet<const AActor *> RangedTargets;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (Candidate == nullptr || Candidate == Source)
		{
			continue;
		}
		
		bool bAlready = false;
		RangedTargets.Add(Candidate, &bAlready);
		if (bAlready)
		{
			continue;
		}

		if (FGenericTeamId::GetAttitude(Source, Candidate) != ETeamAttitude::Hostile)
		{
			continue;
		}

		const FVector To = Candidate->GetActorLocation() - Origin;
		const FVector To2D(To.X, To.Y, 0.f);
		const float Dist = To2D.Size();
		if (Dist <= KINDA_SMALL_NUMBER || Dist > Range)
		{
			continue;
		}

		const FVector ToDir = To2D / Dist;
		const float AngleDeg = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(FVector::DotProduct(Aim, ToDir), -1.f, 1.f)));
		if (AngleDeg > HalfAngle)
		{
			continue;
		}

		
#if ENABLE_DRAW_DEBUG
		if (bDrawDebug)
		{
			DrawDebugPoint(World, Candidate->GetActorLocation(), 24.f, FColor::Red, false, DrawTime);
			DrawDebugLine(World, Origin, Candidate->GetActorLocation(), FColor::Red, false, DrawTime, 0, 1.f);
		}
#endif

		const float LengthWeight = ((Range - Dist) / Range) * RangeWeightRate;
		const float AngleWeight = ((180.f - AngleDeg) / 180.f) * (1.f - RangeWeightRate);
		const float Score = LengthWeight + AngleWeight;
		if (Score > BestScore)
		{
			BestScore = Score;
			Result = Candidate;
			BestDist = Dist;
			BestAngle = AngleDeg;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		const FVector EdgeL = Aim.RotateAngleAxis(HalfAngle, FVector::UpVector) * Range;
		const FVector EdgeR = Aim.RotateAngleAxis(-HalfAngle, FVector::UpVector) * Range;
		DrawDebugLine(World, Origin, Origin + EdgeL, FColor::Yellow, false, DrawTime, 0, DrawTime);
		DrawDebugLine(World, Origin, Origin + EdgeR, FColor::Yellow, false, 3.0f, 0, DrawTime);
		DrawDebugDirectionalArrow(World, Origin, Origin + Aim * Range, 60.f, FColor::Green, false, DrawTime, 0, 3.f);
		if (Result)
		{
			DrawDebugString(World, Result->GetActorLocation(),
			                FString::Printf(TEXT("PICK\nScore %.2f\nDist %.0f\nAngle %.1f"), BestScore, BestDist, BestAngle),
			                nullptr, FColor::Yellow, DrawTime, true);
		}
	}
#endif

	return Result;
}
