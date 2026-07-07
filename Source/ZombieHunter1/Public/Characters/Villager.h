// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CombatCharacter.h"
#include "Villager.generated.h"

class AAIController;

/**
 * 마을 주민 — 비전투 NPC.
 * HomeLocation 주변을 느긋하게 배회하며 마을에 생기를 준다. 공격하지 않고,
 * 체력/죽음은 베이스(ACombatCharacter)를 그대로 써서 적의 공격(AOE 등)에 맞으면 죽는다.
 *
 * 동작: 대기(1.5~4초) → 배회 반경 안 NavMesh 위 랜덤 지점으로 걸어감 → 도착하면 다시 대기.
 * 마을 생성기(AInfiniteMapGenerator)가 스폰하며 HomeLocation을 마을 중심으로 지정한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API AVillager : public ACombatCharacter
{
	GENERATED_BODY()

public:
	AVillager();

	/** 배회의 중심(월드). 생성기가 스폰 직후 마을 중심으로 설정한다. */
	UPROPERTY(BlueprintReadWrite, Category = "Villager")
	FVector HomeLocation = FVector::ZeroVector;

	/** 배회 반경(cm). HomeLocation에서 이 안의 NavMesh 위 지점만 목적지로 고른다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Villager")
	float WanderRadius = 600.0f;

	/** 도착 후 다음 이동까지 기다리는 시간 범위(초). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Villager")
	float WaitTimeMin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Villager")
	float WaitTimeMax = 4.0f;

	/** 걷기 속도(cm/s). 주민은 느긋하게 — 전투 캐릭터보다 느리게 기본값. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Villager")
	float WalkSpeed = 150.0f;

	/** 죽은 뒤 시체가 사라지기까지의 시간(초). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Villager")
	float CorpseLifeSpan = 3.0f;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** 죽음 전환 — 이동/콜리전을 끄고 잠시 후 시체 제거 (동료와 같은 패턴). */
	virtual void OnDeath() override;

	/** 빙의된 AI 컨트롤러 */
	UPROPERTY()
	AAIController* AICon = nullptr;

	/** 다음 이동까지 남은 대기 시간(초). 이동 중에는 줄지 않는다. */
	float WaitRemaining = 0.0f;

	/** 배회 반경 안 NavMesh 위 랜덤 지점을 골라 이동 명령. */
	void PickNewWanderPoint();
};
