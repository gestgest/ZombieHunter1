// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JobComponent.generated.h"

class AMyPlayer;
class UAnimMontage;
class USoundBase;
class AProjectile;
class USkeletalMesh;
class USkeletalMeshComponent;

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

	/** true면 스폰한 발사체의 사거리/적중범위 디버그를 화면에 그린다(궁수/마법사). 범위 튜닝용. 기본 꺼짐. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Debug")
	bool bDebugProjectileRange = false;

	/** 직업이 손에 드는 무기 메시(검사=검, 궁수=활 등). 비우면 무기 없음. 직업 서브클래스가 기본값 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job|Weapon")
	USkeletalMesh* WeaponMesh = nullptr;

	/** 무기를 붙일 캐릭터 메시의 소켓 이름. 캐릭터 스켈레톤에 이 소켓이 있어야 손에 붙는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Job|Weapon")
	FName WeaponSocket = TEXT("hand_r");

	/** 소유 플레이어를 연결한다. 플레이어가 컴포넌트 생성 직후 호출. */
	void InitializeForOwner(AMyPlayer* Player);

	/** 공격 시도. 기본 구현은 AttackMontage를 재생한다. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void Attack();

	/** 공격 몽타주의 Notify에서 호출되는 실제 타격 판정. 기본 구현은 비어 있음. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void OnAttackNotify(FName NotifyName);

	/** 매 프레임 호출(플레이어 Tick). 조준과 무관한 패시브 효과(자가 회복 등)용. 기본 구현 없음. */
	virtual void TickJob(float DeltaTime);

protected:
	/** 소유 플레이어 (InitializeForOwner에서 설정) */
	UPROPERTY()
	AMyPlayer* OwnerPlayer = nullptr;

	/** 런타임에 생성된 무기 메시 컴포넌트 (WeaponMesh를 손 소켓에 붙인 것) */
	UPROPERTY()
	USkeletalMeshComponent* WeaponMeshComp = nullptr;

	/** WeaponMesh를 OwnerPlayer의 WeaponSocket에 붙인다. InitializeForOwner에서 호출. */
	void EquipWeapon();

	/** 공격 사운드를 소유자 위치에서 재생 */
	void PlayAttackSound();

	/**
	 * 전방으로 발사체를 스폰하고 직업의 Damage/속도를 적용해 반환한다 (궁수/마법사 공용).
	 * @param ProjectileClass  스폰할 발사체 클래스
	 * @param Speed            발사 속도(cm/s)
	 * @param MuzzleOffset     캐릭터 앞쪽으로 스폰하는 거리(cm)
	 * @param MuzzleHeight     스폰 높이 보정(cm)
	 * @return 스폰된 발사체(실패 시 nullptr). 호출 측에서 폭발반경 등 추가 설정 가능.
	 */
	AProjectile* SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight);
};
