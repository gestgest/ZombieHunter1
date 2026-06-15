// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

/**
 * 화살/투사체 액터.
 * 직선으로 날아가다가 적(AEnemy)과 겹치면 피해 + 넉백을 주고 사라진다.
 * 궁수/마법사 직업이 스폰해서 사용한다. 메시는 BP 서브클래스에서 지정.
 */
UCLASS()
class ZOMBIEHUNTER1_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	/** 적중 시 가하는 피해량. 스폰한 직업이 설정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	int32 Damage = 1;

	/** 적중 시 적을 밀어내는 힘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float KnockbackForce = 300.0f;

	/** >0이면 적중 지점 주변 이 반경(cm) 안의 모든 적에게 피해(범위 폭발). 0이면 단일 대상. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ExplosionRadius = 0.0f;

	/** 적중 시 재생할 사운드 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	USoundBase* HitSound = nullptr;

	/** 발사 속도(cm/s)를 설정한다. 직업이 스폰 직후 호출. */
	void SetInitialSpeed(float Speed);

protected:
	virtual void BeginPlay() override;

	/** 충돌 구체 (루트) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* CollisionSphere;

	/** 화살 시각 메시 (BP에서 메시 지정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* Mesh;

	/** 직선 비행 이동 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 적 하나에게 피해 + 폭발/발사 중심에서 바깥 방향으로 넉백 */
	void ApplyHit(class AEnemy* Enemy);
};
