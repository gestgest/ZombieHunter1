// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatCharacter.h"
#include "Companion.generated.h"

class UJobComponent;
class AAIController;
class AEnemy;

/**
 * 아군 동료 AI 캐릭터.
 * 평소엔 리더(플레이어)를 졸졸 따라다니고(Following), 근처에 적이 나타나면
 * 그 적에게 다가가 직업(UJobComponent)으로 공격한다(Fighting).
 * 전투/이동 방식은 플레이어와 동일한 UJobComponent를 재활용한다 — 입력 대신 AI가 구동.
 *
 * 체력(HP)/데미지/죽음/공격몽타주 Notify 배선은 베이스(ACombatCharacter)가 제공한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API ACompanion : public ACombatCharacter
{
	GENERATED_BODY()

public:
	ACompanion();

	/** 따라다닐 대상(보통 플레이어). 섭외 시 RecruitCompanion에서 설정. */
	UPROPERTY(BlueprintReadWrite, Category = "Companion")
	AActor* Leader = nullptr;

	/** 시작 시 부착할 직업 클래스. 직업 BP 서브클래스(BP_SwordsmanJob 등)를 지정. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<UJobComponent> DefaultJobClass;

	/** 적 탐지 반경(cm). 이 안에 적이 있으면 교전 상태로 전환. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float DetectRadius = 900.0f;

	/** 공격 사거리(cm). 적이 이 거리 안이면 멈춰서 공격한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float AttackRange = 160.0f;

	/** 리더와 유지할 거리(cm). 이보다 멀어지면 따라간다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float FollowDistance = 250.0f;

	/** 자동 공격 간격(초). 직업에 값이 있으면 그쪽을 우선 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float AttackInterval = 0.6f;

	/** 죽은 뒤 시체가 사라지기까지의 시간(초). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float CorpseLifeSpan = 3.0f;

	// 디버그 토글은 베이스(ACombatCharacter::bDebugCombat)로 올림 — 플레이어/적과 공용.

	/** 런타임 생성된 직업 컴포넌트 */
	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	UJobComponent* CurrentJob = nullptr;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	/** 빙의된 AI 컨트롤러 */
	UPROPERTY()
	AAIController* AICon = nullptr;

	float TimeSinceAttack = 0.0f;

	/** 동료 상태 — 따라다님 / 교전 */
	enum class EState : uint8 { Following, Fighting };
	EState State = EState::Following;

	/** DetectRadius 안에서 가장 가까운 "살아있는" 적을 찾는다(없으면 nullptr). */
	AEnemy* FindNearestEnemy() const;

	/** 대상 액터를 바라보도록 yaw 회전(공격 방향을 적에게 맞춤). */
	void FaceActor(AActor* Target);

	/** 공격 몽타주의 Notify를 직업의 OnAttackNotify로 전달(검사 근접 판정 등). 배선은 베이스가 담당. */
	virtual void HandleAttackNotify(FName NotifyName) override;

	/** 죽음 전환 — AI/공격을 끊고 잠시 후 시체를 제거한다. */
	virtual void OnDeath() override;
};
