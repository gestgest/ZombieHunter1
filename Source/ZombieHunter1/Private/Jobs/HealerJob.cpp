// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/HealerJob.h"
#include "MyPlayer.h"

UHealerJob::UHealerJob()
{
	JobName = TEXT("Healer");
	// 공격은 검사(USwordsmanJob)를 그대로 물려받는다. 차별점은 자가 회복.
}

void UHealerJob::TickJob(float DeltaTime)
{
	if (!OwnerPlayer)
	{
		return;
	}

	TimeSinceLastHeal += DeltaTime;
	if (TimeSinceLastHeal < HealInterval)
	{
		return;
	}
	TimeSinceLastHeal = 0.0f;

	// 최대 체력 미만일 때만 회복. SetHP가 HUD 체력바도 갱신한다.
	if (OwnerPlayer->HP < MaxHP)
	{
		const int32 NewHP = FMath::Min(OwnerPlayer->HP + HealAmount, MaxHP);
		OwnerPlayer->SetHP(NewHP);
	}
}
