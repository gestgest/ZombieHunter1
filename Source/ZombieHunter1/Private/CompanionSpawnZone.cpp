// Fill out your copyright notice in the Description page of Project Settings.

#include "CompanionSpawnZone.h"
#include "Characters/MyPlayer.h"
#include "Engine/Engine.h"

void ACompanionSpawnZone::HandleZoneFilled(AMyPlayer* Payer)
{
	Payer->RecruitCompanion();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan,
			TEXT("[SpawnZone] 게이지 완료 - 동료 소환!"));
	}
}
