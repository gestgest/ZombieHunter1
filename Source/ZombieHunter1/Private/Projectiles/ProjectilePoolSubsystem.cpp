// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectiles/ProjectilePoolSubsystem.h"
#include "Projectiles/Projectile.h"
#include "Engine/World.h"

AProjectile* UProjectilePoolSubsystem::Acquire(TSubclassOf<AProjectile> ProjectileClass,
	const FVector& Location, const FRotator& Rotation,
	AActor* InOwner, APawn* InInstigator)
{
	if (!ProjectileClass)
	{
		return nullptr;
	}

	AProjectile* Projectile = nullptr;

	// 1) 풀에서 재사용 시도. 파괴돼 null이 된 항목은 버리면서 유효한 것을 찾는다.
	if (FProjectilePoolEntry* Entry = Pools.Find(ProjectileClass))
	{
		while (Entry->FreeList.Num() > 0 && !Projectile)
		{
			Projectile = Entry->FreeList.Pop();
			if (!IsValid(Projectile))
			{
				Projectile = nullptr;
			}
		}
	}

	if (Projectile)
	{
		// 재사용: 새 발사 지점으로 순간이동 (물리 텔레포트 — 스윕/충돌 없이)
		Projectile->SetOwner(InOwner);
		Projectile->SetInstigator(InInstigator);
		Projectile->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		// 2) 풀이 비었으면 새로 스폰 (풀은 수요만큼 자연스럽게 커진다)
		FActorSpawnParameters Params;
		Params.Owner = InOwner;
		Params.Instigator = InInstigator;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, Location, Rotation, Params);

#if WITH_EDITOR
		if (Projectile)
		{
			Projectile->SetFolderPath(TEXT("Spawned/Projectiles")); // 아웃라이너 정리용 (에디터 전용)
		}
#endif
	}

	if (Projectile)
	{
		Projectile->ActivateFromPool(); // 충돌/이동/수명타이머 재가동
	}
	return Projectile;
}

void UProjectilePoolSubsystem::Release(AProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}
	Pools.FindOrAdd(Projectile->GetClass()).FreeList.Add(Projectile);
}
