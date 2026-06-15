// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/SwordsmanJob.h"
#include "HealerJob.generated.h"

/**
 * 힐러(자가 서스테인) 직업.
 * 검사처럼 근접 공격을 하면서, 일정 간격으로 자기 체력을 회복한다.
 * (1인 플레이 게임이라 아군 힐 대신 자가 회복으로 버티는 컨셉)
 */
UCLASS()
class ZOMBIEHUNTER1_API UHealerJob : public USwordsmanJob
{
	GENERATED_BODY()

public:
	UHealerJob();

	/** 회복 간격(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	float HealInterval = 2.0f;

	/** 한 번에 회복하는 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	int32 HealAmount = 1;

	/** 이 값 이상으로는 회복하지 않음(오버힐/HUD 폭 폭주 방지). 보통 시작 체력과 동일하게. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	int32 MaxHP = 5;

	virtual void TickJob(float DeltaTime) override;

protected:
	/** 마지막 회복 이후 누적 시간 */
	float TimeSinceLastHeal = 0.0f;
};
