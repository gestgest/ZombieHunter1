// Fill out your copyright notice in the Description page of Project Settings.

#include "ArcherJob.h"
#include "Projectile.h"
#include "MyPlayer.h"
#include "Engine/World.h"

UArcherJob::UArcherJob()
{
	JobName = TEXT("Archer");

	// 기본 발사체: 순수 AProjectile(메시 없는 충돌 구체). BP_Arrow로 바꾸면 화살이 보인다.
	ProjectileClass = AProjectile::StaticClass();
}

void UArcherJob::Attack()
{
	// 공격 몽타주가 있으면 재생(시각 효과). 화살은 곧바로 발사한다.
	Super::Attack();

	FireArrow();
}

void UArcherJob::FireArrow()
{
	if (!OwnerPlayer || !ProjectileClass)
	{
		return;
	}

	UWorld* World = OwnerPlayer->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Forward = OwnerPlayer->GetActorForwardVector();
	const FVector SpawnLocation =
		OwnerPlayer->GetActorLocation() + Forward * MuzzleOffset + FVector(0, 0, MuzzleHeight);
	const FRotator SpawnRotation = Forward.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerPlayer;
	SpawnParams.Instigator = OwnerPlayer;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectile* Arrow = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Arrow)
	{
		Arrow->Damage = Damage; // 직업의 데미지를 화살에 전달
		Arrow->SetInitialSpeed(ProjectileSpeed);
	}
}
