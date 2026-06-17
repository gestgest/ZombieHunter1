// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CombatCharacter.h"

//전방 선언
class AAIController;
//const FPathFollowingResult&를 해결하기 위한 전반 선언
struct FAIRequestID;
struct FPathFollowingResult;

//FBranchingPointNotifyPayload
struct FBranchingPointNotifyPayload;

#include "Enemy.generated.h"


UCLASS()
class ZOMBIEHUNTER1_API AEnemy : public ACombatCharacter
{
	GENERATED_BODY()

	bool hit();
	void DebugHPShow();

	AAIController* aiController;

	float attackRange = 100.0f;
	int enemy_id;
	bool CanAttack;

public:
	// Sets default values for this character's properties
	AEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//virtual void PossessedBy(AController* NewController) override;

	// 공격 몽타주의 "Attack" Notify가 들어오면 자체 타격(hit)을 수행한다.
	virtual void HandleAttackNotify(FName NotifyName) override;

	// 죽음/부활 전환 처리 — HP/IsDead 전환은 베이스(ACombatCharacter)가 호출한다.
	virtual void OnDeath() override;
	virtual void OnRevive() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//변수 — 체력/데미지/죽음/공격몽타주는 베이스(ACombatCharacter)로 이동.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* AttackSound; //MS


	//함수
	UFUNCTION(BlueprintCallable)
	void TrackingPlayer();


	//UFUNCTION(BlueprintCallable)
	void MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void SetID(int id);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void DeadEnemySignal(int index);


	void SetAIController();

};
