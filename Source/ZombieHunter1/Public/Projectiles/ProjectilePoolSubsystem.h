// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectilePoolSubsystem.generated.h"

class AProjectile;

/** 클래스 하나당 쉬고 있는(비활성) 발사체 목록. TMap 값으로 쓰기 위한 래퍼. */
USTRUCT()
struct FProjectilePoolEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AProjectile*> FreeList;
};

/**
 * 발사체 오브젝트 풀 (월드당 1개, 자동 생성).
 * 화살/파이어볼이 적중·수명만료로 사라질 때 Destroy 대신 여기로 반환되고,
 * 다음 발사 때 재활용된다 → 스폰/파괴/GC 비용 제거 (모바일 최적화).
 *
 * 풀은 발사체 "클래스별"로 나뉜다(BP_Arrow와 BP_Fireball은 서로 섞이지 않음).
 * 사용처: UJobComponent::SpawnProjectileForward가 Acquire, AProjectile::ReturnToPool이 Release.
 */
UCLASS()
class ZOMBIEHUNTER1_API UProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 풀에서 발사체를 꺼내(없으면 새로 스폰) 지정 위치/방향으로 활성화해 반환. 실패 시 nullptr. */
	AProjectile* Acquire(TSubclassOf<AProjectile> ProjectileClass,
		const FVector& Location, const FRotator& Rotation,
		AActor* InOwner, APawn* InInstigator);

	/** 발사체를 풀로 되돌린다. 호출 전에 이미 비활성화(DeactivateForPool)된 상태여야 한다.
	 *  AProjectile::ReturnToPool이 호출하므로 외부에서 직접 부를 일은 없다. */
	void Release(AProjectile* Projectile);

private:
	/** 발사체 클래스 → 쉬고 있는 인스턴스 목록. UPROPERTY라 파괴된 액터는 GC가 null로 청소한다. */
	UPROPERTY()
	TMap<TSubclassOf<AProjectile>, FProjectilePoolEntry> Pools;
};
