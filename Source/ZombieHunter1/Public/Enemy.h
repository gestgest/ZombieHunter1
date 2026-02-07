// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

//전방 선언
class AAIController;
//const FPathFollowingResult&를 해결하기 위한 전반 선언
struct FAIRequestID;
struct FPathFollowingResult;

//FBranchingPointNotifyPayload
struct FBranchingPointNotifyPayload;

#include "Enemy.generated.h"


UCLASS()
class ZOMBIEHUNTER1_API AEnemy : public ACharacter
{
	GENERATED_BODY()

	bool CanAttack;
	float attackRange = 50;
	AAIController* aiController;

	bool hit();
	void DebugHPShow();
	

public:
	// Sets default values for this character's properties
	AEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//변수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
	int Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
	int32 HP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly , Category = "Enemy|Stats")
	bool IsDead;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* AttackSound; //MS


	//함수
	UFUNCTION(BlueprintCallable)
	void TrackingPlayer();


	//UFUNCTION(BlueprintCallable)
	void MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void AddHP(int add_hp);
	void SetHP(int new_hp);
};
