// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JobComponent.generated.h"

class AMyPlayer;
class UAnimMontage;
class USoundBase;

/**
 * 직업(Job) 베이스 컴포넌트.
 * 이동/조준은 AMyPlayer가 담당하고, "공격 방식"만 이 컴포넌트로 분리한다.
 * 직업마다 서브클래스(USwordsmanJob, UArcherJob...)에서 Attack()/OnAttackNotify()를 재정의한다.
 *
 * - Attack()        : 자동공격 타이밍에 플레이어가 호출. 기본 동작은 AttackMontage 재생.
 * - OnAttackNotify(): 공격 몽타주의 Notify 시점에 플레이어가 호출. 실제 피해/발사 판정을 여기서.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Job), meta = (BlueprintSpawnableComponent))
class ZOMBIEHUNTER1_API UJobComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJobComponent();

	/** 직업 이름 (UI/디버그용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job")
	FName JobName = TEXT("Job");

	/** 한 번 적중 시 가하는 피해량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Combat")
	int32 Damage = 1;

	/** 자동 공격 간격(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Combat")
	float AttackInterval = 0.4f;

	/** 공격 시 재생할 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Combat")
	UAnimMontage* AttackMontage = nullptr;

	/** 공격 적중 시 재생할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Combat")
	USoundBase* AttackSound = nullptr;

	/** 소유 플레이어를 연결한다. 플레이어가 컴포넌트 생성 직후 호출. */
	void InitializeForOwner(AMyPlayer* Player);

	/** 공격 시도. 기본 구현은 AttackMontage를 재생한다. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void Attack();

	/** 공격 몽타주의 Notify에서 호출되는 실제 타격 판정. 기본 구현은 비어 있음. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void OnAttackNotify(FName NotifyName);

protected:
	/** 소유 플레이어 (InitializeForOwner에서 설정) */
	UPROPERTY()
	AMyPlayer* OwnerPlayer = nullptr;

	/** 공격 사운드를 소유자 위치에서 재생 */
	void PlayAttackSound();
};
