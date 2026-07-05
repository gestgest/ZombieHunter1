// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Characters/CombatCharacter.h"

//전방 선언
class AAIController;
//const FPathFollowingResult&를 해결하기 위한 전반 선언
struct FAIRequestID;
struct FPathFollowingResult;

//FBranchingPointNotifyPayload
struct FBranchingPointNotifyPayload;

class UWidgetComponent;

#include "Enemy.generated.h"


UCLASS()
class ZOMBIEHUNTER1_API AEnemy : public ACombatCharacter
{
	GENERATED_BODY()

	bool hit();
	void DebugHPShow();

	// UPROPERTY — 컨트롤러가 파괴되면 자동 null. 안 그러면 OnDeath 등에서 댕글링 역참조 위험.
	UPROPERTY()
	AAIController* aiController = nullptr;

	float attackRange = 100.0f;
	int enemy_id;
	bool CanAttack;

	/** 다음 추격 갱신이 허용되는 월드 시간(초). TrackingPlayer가 이 시간 전 호출되면 무시. */
	float NextTrackTime = 0.0f;

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

	/** 처치 시 플레이어에게 주는 경험치. 강한 적 BP에서 더 크게 설정하면 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	int32 ExpReward = 1;

	/** 머리 위 HP 바(스크린 스페이스). 위젯 클래스(WBP_EnemyHPBar)는 BP_Enemy에서 이 컴포넌트에 지정한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HPBarComponent;

	/** HP 변경 시 머리 위 HP 바를 갱신하고, 죽으면 숨긴다. */
	virtual void SetHP(int32 new_hp) override;

	//////////////////////////////////////////////////////////////////////////
	// 추격 갱신 간격 (AI 틱 최적화 — 매 프레임 경로 재요청 금지)

	/** 추격 갱신 간격 — 플레이어가 가까울 때(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TrackNearInterval = 0.25f;

	/** 추격 갱신 간격 — TrackFarDistance보다 멀 때(초). 멀리 있는 적은 드물게 갱신(거리 LOD) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TrackFarInterval = 0.8f;

	/** 이 거리(cm)보다 멀면 TrackFarInterval을 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TrackFarDistance = 2000.0f;


	//함수
	UFUNCTION(BlueprintCallable)
	void TrackingPlayer();


	//UFUNCTION(BlueprintCallable)
	void MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void SetID(int id);

	/** 풀 대기 상태로 전환 — 숨김 + 콜리전/이동/틱 정지. 게임모드 풀이 반납 시 호출.
	 *  SetActorHiddenInGame만으로는 중력이 계속 작동해서, 발밑 청크가 언로드되면
	 *  대기 중인 적이 통째로 낙하한다 → 반드시 이동까지 꺼야 한다. */
	void EnterPoolDormancy();

	/** 풀에서 깨어남 — EnterPoolDormancy가 껐던 것들을 복구. 게임모드가 위치 배치 후 호출. */
	void WakeFromPool();

	/** 리쉬 회수용 순간이동 — 낙하 속도/진행 중 경로를 정리하고 새 위치로 옮긴다.
	 *  (무한맵에서 뒤처져 지형을 잃은 적을 플레이어 근처로 재배치할 때 사용) */
	void TeleportForLeash(const FVector& NewLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void DeadEnemySignal(int index);


	void SetAIController();

};
